# This is -*- Python -*-

import pango, gtk
import gnoetics, tilemodel

class PoemTileView(gtk.DrawingArea,
                   gnoetics.PoemListener):

    def __init__(self, poem):
        gtk.DrawingArea.__init__(self)
        gnoetics.PoemListener.__init__(self, poem)

        self.__x_start = 10
        self.__y_start = 10
        self.__x_spacing = 4
        self.__tiny_x_spacing = 1
        self.__y_spacing = 4
        self.__stanza_spacing = 3 * self.__y_spacing

        self.__word_x_left_pad = 4
        self.__word_x_right_pad = 3
        self.__word_y_pad = 2

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
        self.__tile_info = []
        
        self.connect("size_request",        PoemTileView.__size_request_handler)
        self.connect("configure_event",     PoemTileView.__configure_handler)
        self.connect("expose_event",        PoemTileView.__expose_handler)


    def __assemble_tile_info(self):
        self.__tile_info = []

        poem = self.get_poem()
        if not poem:
            return

        x = self.__x_start
        y = self.__y_start

        line_h = 0
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
                    self.__tile_info.append((i, x, y, w, h, txt))
                    i += 1
                    x += w
                    line_h = max(line_h, h)

                

    def __render_one_tile(self, i, x, y, w, h, txt):
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

        layout = pango.Layout(self.get_pango_context())
        layout.set_markup(txt)

        self.__pixmap.draw_layout(gc,
                                  x + x_pad,
                                  y + self.__word_y_pad,
                                  layout)
        if draw_box:
            self.__pixmap.draw_rectangle(gc, gtk.FALSE, x, y, w-1, h-1)
        gc.set_foreground(old_fg)
        gc.set_background(old_bg)

        

    def __render(self):
        for i, x, y, w, h, txt in self.__tile_info:
            self.__render_one_tile(i, x, y, w, h, txt)


    def __size_request_handler(self, req):
        #w, h = self.__get_total_size()
        w, h = 400, 300
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

        self.__render()

        return gtk.TRUE


    def __expose_handler(self, ev):
        x, y, w, h = ev.area
        gc = self.get_style().fg_gc[gtk.STATE_NORMAL]
        self.window.draw_drawable(gc, self.__pixmap, x, y, x, y, w, h)
        return gtk.FALSE


    def poem_changed(self, poem):
        self.__assemble_tile_info()
        self.__render()
        self.queue_resize()
