import pango, gtk
import gnoetics, tilemodel

class TileView(gtk.DrawingArea,
               tilemodel.TileModelListener):

    def __init__(self, model):
        gtk.DrawingArea.__init__(self)
        tilemodel.TileModelListener.__init__(self, model)
        
        self.__model = model
        self.__pixmap = None
        self.__layout_cache = {}
        self.__line_position_cache = [None] * self.__model.get_line_count()
        self.__tile_position_cache = {}

        self.__bg_color = self.get_colormap().alloc_color("lightgreen")
        self.__word_fg_color = self.get_colormap().alloc_color("black")
        self.__word_bg_color = self.get_colormap().alloc_color("white")

        # Color scheme for highlighted tiles
        self.__hi_fg_color = self.get_colormap().alloc_color("red")
        self.__hi_bg_color = self.get_colormap().alloc_color("white")

        # Color scheme for selected tiles
        self.__sel_fg_color = self.get_colormap().alloc_color("black")
        self.__sel_bg_color = self.get_colormap().alloc_color("yellow")

        # Color scheme for highlighted, selected tiles
        self.__hisel_fg_color = self.get_colormap().alloc_color("red")
        self.__hisel_bg_color = self.get_colormap().alloc_color("yellow")

        self.__x_start = 10
        self.__y_start = 10
        self.__x_spacing = 4
        self.__tiny_x_spacing = 1
        self.__y_spacing = 4
        self.__stanza_spacing = 3 * self.__y_spacing

        self.__word_x_left_pad = 4
        self.__word_x_right_pad = 3
        self.__word_y_pad = 2

        self.__hi_line_num = None
        self.__hi_col_num = None

        self.connect("size_request", TileView.__size_request_handler)
        self.connect("configure_event", TileView.__configure_handler)
        self.connect("expose_event", TileView.__expose_handler)
        self.connect("motion_notify_event", TileView.__motion_notify_handler)
        self.connect("enter_notify_event", TileView.__enter_notify_handler)
        self.connect("leave_notify_event", TileView.__leave_notify_handler)
        self.connect("button_press_event", TileView.__button_press_handler)

        self.set_events(gtk.gdk.EXPOSURE_MASK |
                        gtk.gdk.ENTER_NOTIFY_MASK |
                        gtk.gdk.LEAVE_NOTIFY_MASK |
                        gtk.gdk.BUTTON_PRESS_MASK |
                        gtk.gdk.POINTER_MOTION_MASK |
                        gtk.gdk.POINTER_MOTION_HINT_MASK)


    def __get_layout(self, text):
        layout = self.__layout_cache.get(text)
        if layout is None:
            layout = pango.Layout(self.get_pango_context())
            layout.set_markup(text)
            self.__layout_cache[text] = layout
        return layout


    def __get_tile_size(self, tile):
        if tile == "stanza break":
            return 0, self.__stanza_spacing


        # Handle textual tiles

        txt = "???"
        is_token = (type(tile) == gnoetics.Token)

        if is_token:
            txt = tile.get_word()
        else:
            txt = tile

        layout = self.__get_layout(txt)
        w, h = layout.get_pixel_size()
        h += 2 * self.__word_y_pad

        if is_token:
            w += self.__word_x_left_pad + self.__word_x_right_pad

        return w, h


    def __get_line_position(self, line_num):
        pos = self.__line_position_cache[line_num]
        if pos is None:
            n = self.__model.get_line_length(line_num)
            max_h = 0
            for i in range(n):
                tile = self.__model.get_tile(line_num, i)
                w, h = self.__get_tile_size(tile)
                max_h = max(max_h, h)

            if line_num == 0:
                y = self.__y_start
            else:
                prev_y, prev_h = self.__get_line_position(line_num-1)
                y = prev_y + prev_h + self.__y_spacing

            self.__line_position_cache[line_num] = pos = (y, max_h)
            
        return pos


    def __get_tile_position(self, line_num, col_num):

        val = self.__tile_position_cache.get((line_num, col_num))
        if val is not None:
            return val

        tile = self.__model.get_tile(line_num, col_num)
        w, h = self.__get_tile_size(tile)
        
        if col_num == 0:
            y, max_h = self.__get_line_position(line_num)
            x = self.__x_start
        else:
            ptile = self.__model.get_tile(line_num, col_num-1)
            px, py, pw, ph = self.__get_tile_position(line_num, col_num-1)

            x = px + pw
            y = py

            if (type(ptile) == gnoetics.Token and ptile.has_right_glue()) or \
               (type(tile) == gnoetics.Token and tile.has_left_glue()):
                x += self.__tiny_x_spacing
            else:
                x += self.__x_spacing

        self.__tile_position_cache[(line_num, col_num)] = (x, y, w, h)
        
        return (x, y, w, h)


    def __get_total_size(self):
        x1 = 0
        y1 = 0
        for i in range(self.__model.get_line_count()):
            j = self.__model.get_line_length(i)
            if j > 0:
                x, y, w, h = self.__get_tile_position(i, j-1)
                x1 = max(x1, x+w)
                y1 = max(y1, y+h)

        return x1 + self.__x_start, y1 + self.__y_start


    def __convert_y_to_line(self, y):
        if y >= self.__y_start:
            N = self.__model.get_line_count()
            for i in xrange(N):
                line_y, line_h = self.__get_line_position(i)
                if line_y <= y < line_y + line_h:
                    return i
        return None


    def __point_to_tile(self, x, y):
        if x < self.__x_start:
            return (None, None, None)
        line_num = self.__convert_y_to_line(y)
        if line_num is None:
            return (None, None, None)
        N = self.__model.get_line_length(line_num)
        x0 = self.__x_start
        prev_right_glue = 0
        for i in xrange(N):
            tile = self.__model.get_tile(line_num, i)
            w, h = self.__get_tile_size(tile)
            has_left_glue = 0
            is_token = (type(tile) == gnoetics.Token)
            if is_token:
                has_left_glue = tile.has_left_glue()
            if i > 0:
                if not (prev_right_glue or has_left_glue):
                    x0 += self.__x_spacing
                else:
                    x0 += self.__tiny_x_spacing
            if x0 <= x < x0+w:
                return line_num, i, tile
            x0 += w
            if is_token:
                has_right_glue = tile.has_right_glue()
        return (None, None, None)


    def __render_tile(self, tile, x, y, hi=0, selected=0):

        # If this is a stanza break, do nothing
        if tile == "stanza break":
            return

        if hi and selected:
            fg = self.__hisel_fg_color
            bg = self.__hisel_bg_color
        elif hi:
            fg = self.__hi_fg_color
            bg = self.__hi_bg_color
        elif selected:
            fg = self.__sel_fg_color
            bg = self.__sel_bg_color
        else:
            fg = self.__word_fg_color
            bg = self.__word_bg_color
            
        w, h = self.__get_tile_size(tile)
        if w <= 0 or h <= 0:
            return

        draw_box = 1
        x_pad = 0
        txt = "???"
        
        if type(tile) == gnoetics.Token:
            txt = tile.get_word()
            x_pad = self.__word_x_left_pad
        else:
            draw_box = 0
            bg = self.__bg_color
            txt = tile
        
        gc = self.get_style().bg_gc[gtk.STATE_NORMAL]
        old_fg = gc.foreground
        old_bg = gc.background
        gc.set_foreground(bg)
        self.__pixmap.draw_rectangle(gc, gtk.TRUE, x, y, w-1, h-1)
        gc.set_foreground(fg)
        gc.set_background(bg)
        layout = self.__get_layout(txt)

        self.__pixmap.draw_layout(gc,
                                  x + x_pad,
                                  y + self.__word_y_pad,
                                  layout)
        if draw_box:
            self.__pixmap.draw_rectangle(gc, gtk.FALSE, x, y, w-1, h-1)
        gc.set_foreground(old_fg)
        gc.set_background(old_bg)


    def __draw_tile(self, line_num, col_num, queue_redraw=0):
        tile = self.__model.get_tile(line_num, col_num)
        selflag = self.__model.get_selection_flag(line_num, col_num)
        x, y, w, h = self.__get_tile_position(line_num, col_num)
        hi = (line_num == self.__hi_line_num \
              and col_num == self.__hi_col_num)
        self.__render_tile(tile, x, y, hi=hi, selected=selflag)
        if queue_redraw:
            self.queue_draw_area(x, y, w, h)
            

    def __highlight_tile(self, line_num, col_num):

        if self.__hi_line_num == line_num and self.__hi_col_num == col_num:
            return

        old_hi_line_num = self.__hi_line_num
        old_hi_col_num = self.__hi_col_num

        self.__hi_line_num = line_num
        self.__hi_col_num = col_num

        if old_hi_line_num is not None:
            self.__draw_tile(old_hi_line_num, old_hi_col_num,
                             queue_redraw=1)

        if line_num is not None:
            self.__draw_tile(line_num, col_num, queue_redraw=1)



    def __draw_line(self, line_num, queue_redraw=0):
        n = self.__model.get_line_length(line_num)
        for i in range(n):
            self.__draw_tile(line_num, i, queue_redraw=queue_redraw)


    def __draw_all(self):
        n = self.__model.get_line_count()
        for i in range(n):
            h = self.__draw_line(i)


    def __size_request_handler(self, req):
        w, h = self.__get_total_size()
        req.width = w
        req.height = h
        return gtk.TRUE
    

    def __configure_handler(self, ev):
        win = self.window
        w, h = win.get_size()
        self.__pixmap = gtk.gdk.Pixmap(win, w, h)

        # Because of a pygtk bug we can't copy the gc
        gc = self.get_style().bg_gc[gtk.STATE_NORMAL]
        fg = gc.foreground
        gc.set_foreground(self.__bg_color)
        self.__pixmap.draw_rectangle(gc, gtk.TRUE, 0, 0, w, h)
        gc.set_foreground(fg)

        self.__draw_all()

        return gtk.TRUE


    def __expose_handler(self, ev):
        x, y, w, h = ev.area
        gc = self.get_style().fg_gc[gtk.STATE_NORMAL]
        self.window.draw_drawable(gc, self.__pixmap, x, y, x, y, w, h)
        return gtk.FALSE


    def __motion_notify_handler(self, ev):
        if ev.is_hint:
                x, y, state = ev.window.get_pointer()
        else:
                x = ev.x; y = ev.y
                state = ev.state
        line_num, col_num, tile = self.__point_to_tile(x, y)
        if line_num is not None and state & gtk.gdk.BUTTON1_MASK:
            if state & gtk.gdk.SHIFT_MASK:
                flag = 0
            else:
                flag = 1
            if state & gtk.gdk.CONTROL_MASK:
                self.__model.set_selection_flag_on_line(line_num, flag)
            else:
                self.__model.set_selection_flag(line_num, col_num, flag)
        self.__highlight_tile(line_num, col_num)

        return gtk.TRUE


    def __button_press_handler(self, ev):
        if ev.button == 1:
            line_num, col_num, tile = self.__point_to_tile(ev.x, ev.y)
            if line_num is not None:
                if ev.state & gtk.gdk.SHIFT_MASK:
                    flag = 0
                else:
                    flag = 1
                if ev.state & gtk.gdk.CONTROL_MASK:
                    self.__model.set_selection_flag_on_line(line_num, flag)
                else:
                    self.__model.set_selection_flag(line_num, col_num, flag)


    def __enter_notify_handler(self, ev):
        x = ev.x; y = ev.y
        line_num, col_num, tile = self.__point_to_tile(x, y)
        if line_num is not None:
            self.__highlight_tile(line_num, col_num)
        return gtk.TRUE
        

    def __leave_notify_handler(self, ev):
        self.__highlight_tile(None, None)
        return gtk.TRUE

    
    def do_changed_line(self, line_num):
        # Reset line pos cache
        self.__line_position_cache = [None] * self.__model.get_line_count()
        self.queue_resize()


    def do_changed_selection(self, line_num, col_num, flag):
        self.__draw_tile(line_num, col_num, queue_redraw=1)
