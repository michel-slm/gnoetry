# This is -*- Python -*-

import pango, gtk
import gnoetics, tilemodel

class PoemTileView(gtk.DrawingArea,
                   gnoetics.PoemListener):

    def __init__(self, poem=None):
        gtk.DrawingArea.__init__(self)
        gnoetics.PoemListener.__init__(self, poem)

        self.__x_start = 10
        self.__y_start = 10
        self.__x_spacing = 4
        self.__tiny_x_spacing = 1
        self.__y_spacing = 4
        self.__stanza_spacing = 5 * self.__y_spacing

        self.__word_x_left_pad = 4
        self.__word_x_right_pad = 3
        self.__word_y_pad = 2

        self.__x_size = 0
        self.__y_size = 0

        # Base color scheme
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

        self.__pixmap = None
        self.__tile_info = None
        self.__tile_info_index = {}

        self.__pointer_x   = None
        self.__pointer_y   = None
        self.__pointing_at = None
        
        self.connect("size_request",        PoemTileView.__size_request_handler)
        self.connect("configure_event",     PoemTileView.__configure_handler)
        self.connect("expose_event",        PoemTileView.__expose_handler)
        self.connect("motion_notify_event", PoemTileView.__motion_notify_handler)
        self.connect("enter_notify_event",  PoemTileView.__enter_notify_handler)
        self.connect("leave_notify_event",  PoemTileView.__leave_notify_handler)
        self.connect("button_press_event",  PoemTileView.__button_press_handler)

        self.set_events(gtk.gdk.EXPOSURE_MASK |
                        gtk.gdk.ENTER_NOTIFY_MASK |
                        gtk.gdk.LEAVE_NOTIFY_MASK |
                        gtk.gdk.BUTTON_PRESS_MASK |
                        gtk.gdk.POINTER_MOTION_MASK |
                        gtk.gdk.POINTER_MOTION_HINT_MASK)



    def __assemble_tile_info(self):

        if self.__tile_info is not None:
            return
        
        self.__tile_info = []
        self.__tile_info_index = {}

        poem = self.get_poem()
        if not poem:
            self.__x_size = 0
            self.__y_size = 0
            return

        x = self.__x_start
        y = self.__y_start

        line_h = 0
        max_x, max_y = 0, 0
        i = 0
        last_was_break = False

        for item in poem.to_list_with_breaks():

            if item == "end of line":
                x = self.__x_start
                y += line_h + self.__y_spacing
                line_h = 0
            elif item == "end of stanza":
                x = self.__x_start
                y += line_h + self.__stanza_spacing
                line_h = 0
            else:

                txt = None

                x_skip = 0
                
                if type(item) == gnoetics.Token:
                    if item.is_break():
                        last_was_break = True
                        i += 1
                    else:
                        txt = item.get_word()
                        if last_was_break:
                            txt = txt[0].upper() + txt[1:]
                        if txt == "i":
                            txt = "I"
                        if item.has_left_glue():
                            x_skip = self.__tiny_x_spacing
                        else:
                            x_skip = self.__x_spacing
                            if last_was_break:
                                x_skip *= 3
                        last_was_break = False
                else:
                    txt = item.to_string()
                    x_skip = self.__x_spacing
                    last_was_break = False

                if txt:
                    layout = pango.Layout(self.get_pango_context())
                    layout.set_markup(txt)
                    w, h = layout.get_pixel_size()
                    w += self.__word_x_left_pad + self.__word_x_right_pad
                    h += 2 * self.__word_y_pad

                    if x != self.__x_start:
                        x += x_skip

                    info = (i, x, y, w, h, txt, layout)
                    self.__tile_info.append(info)
                    self.__tile_info_index[i] = info

                    max_x = max(max_x, x+w)
                    max_y = max(max_y, y+h)

                    i += 1
                    x += w
                    line_h = max(line_h, h)

        self.__x_size = max_x + self.__x_start
        self.__y_size = max_y + self.__y_start


    def __find_tile_at(self, posx, posy):
        self.__assemble_tile_info()
        for i, x, y, w, h, txt, layout in self.__tile_info:
            if x <= posx < x+w and y <= posy < y+h:
                return i
        return None
                

    def __render_one_tile(self, i,
                          queue_redraw=False):

        i, x, y, w, h, txt, layout = self.__tile_info_index[i]

        poem = self.get_poem()

        highlighted = (i == self.__pointing_at)
        selected = poem[i].get_flag()

        if highlighted and selected:
            fg = self.__hisel_fg_color
            bg = self.__hisel_bg_color
        elif highlighted:
            fg = self.__hi_fg_color
            bg = self.__hi_bg_color
        elif selected:
            fg = self.__sel_fg_color
            bg = self.__sel_bg_color
        else:
            fg = self.__word_fg_color
            bg = self.__word_bg_color

        draw_box = True
        x_pad = self.__word_x_left_pad

        gc = self.get_style().bg_gc[gtk.STATE_NORMAL]
        old_fg = gc.foreground
        old_bg = gc.background
        gc.set_foreground(bg)
        self.__pixmap.draw_rectangle(gc, gtk.TRUE, x, y, w-1, h-1)
        gc.set_foreground(fg)
        gc.set_background(bg)

        self.__pixmap.draw_layout(gc,
                                  x + x_pad,
                                  y + self.__word_y_pad,
                                  layout)
        if draw_box:
            self.__pixmap.draw_rectangle(gc, gtk.FALSE, x, y, w-1, h-1)
        gc.set_foreground(old_fg)
        gc.set_background(old_bg)

        if queue_redraw:
            self.queue_draw_area(x, y, w, h)

        

    def __render(self):
        self.__assemble_tile_info()
        self.__pointing_at = None
        for i, x, y, w, h, txt, layout in self.__tile_info:
            if self.__pointer_x and self.__pointer_y:
                j = self.__find_tile_at(self.__pointer_x, self.__pointer_y)
                self.__point_at(j)
            self.__render_one_tile(i)


    def __unpoint_at(self, i):
        self.__render_one_tile(i, queue_redraw=True)


    def __point_at(self, i):
        if self.__pointing_at == i:
            return
        if self.__pointing_at is not None:
            j = self.__pointing_at
            self.__pointing_at = None
            self.__unpoint_at(j)
        self.__pointing_at = i

        if self.__pointing_at is None:
            return
        
        self.__render_one_tile(i, queue_redraw=True)
        

    def __size_request_handler(self, req):
        self.__assemble_tile_info()
        req.width = self.__x_size
        req.height = self.__y_size
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

        self.__render()

        return gtk.TRUE


    def __expose_handler(self, ev):
        x, y, w, h = ev.area
        gc = self.get_style().fg_gc[gtk.STATE_NORMAL]
        self.window.draw_drawable(gc, self.__pixmap, x, y, x, y, w, h)
        return gtk.FALSE


    def __enter_notify_handler(self, ev):
        return gtk.TRUE


    def __leave_notify_handler(self, ev):
        self.__pointer_x = None
        self.__pointer_y = None
        self.__point_at(None)
        
        return gtk.TRUE


    def __motion_notify_handler(self, ev):
        if ev.is_hint:
            x, y, state = ev.window.get_pointer()
        else:
            x, y = ev.x, ev.y
            state = ev.state

        self.__pointer_x = x
        self.__pointer_y = y

        i = self.__find_tile_at(x, y)
        self.__point_at(i)

        if i is not None and state & gtk.gdk.BUTTON1_MASK:
            flag = not (state & gtk.gdk.SHIFT_MASK)
            poem = self.get_poem()
            poem.set_flag(i, flag)
        
        return gtk.TRUE


    def __button_press_handler(self, ev):
        if self.__pointing_at is not None:
            poem = self.get_poem()
            flag = not (ev.state & gtk.gdk.SHIFT_MASK)
            poem.set_flag(self.__pointing_at, flag)

        return gtk.TRUE


    def poem_changed(self, poem):
        self.__tile_info = None
        if not self.flags() & gtk.REALIZED:
            return
        self.__render()
        self.queue_resize()


    def poem_changed_flag(self, poem, i):
        if self.__tile_info is None:
            return
        if not self.flags() & gtk.REALIZED:
            return
        u = self.__tile_info_index.get(i)
        if not u:
            return
        self.__render_one_tile(i, queue_redraw=True)

        

        
