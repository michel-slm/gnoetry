# This is -*- Python -*-

import gtk
import red_pixbuf

def splash(time_in_seconds,
           thunk):

    win = gtk.Window()
    img = red_pixbuf.get_widget("gnoetry-splash")
    win.add(img)

    win.set_decorated(False)
    win.set_position(gtk.WIN_POS_CENTER)

    win.show_all()

    def do_thunk():
        thunk()
        win.destroy()
        
    gtk.timeout_add(int(1000*time_in_seconds), do_thunk)
