
import gtk
import pixbuf

_about_gnoetry_win = None
_about_bob_win = None
_about_ubu_win = None

def show_about_ubu_roi():

    global _about_ubu_win

    if _about_ubu_win:
        _about_ubu_win.present()
        return

    _about_ubu_win = gtk.Dialog("About Ubu Roi")
    _about_ubu_win.set_resizable(0)

    b = _about_ubu_win.add_button(gtk.STOCK_OK, gtk.RESPONSE_CLOSE)
    b.connect("clicked", lambda b,w:w.destroy(), _about_ubu_win)

    def clear_ubu(w):
        global _about_ubu_win
        del _about_ubu_win
        _about_ubu_win = None
        
    _about_ubu_win.connect("destroy", clear_ubu)
    

    hbox = gtk.HBox()

    img = pixbuf.get_widget("ubu-roi-about")
    hbox.pack_start(img, expand=0, fill=0, padding=5)

    txt_vbox = gtk.VBox()
    hbox.pack_start(txt_vbox, expand=1, fill=1)

    lines = ("<big><b>UBU ROI</b></big>",
             "",
             "As the True portrait of Monsieur Ubu shows,",
             "he is the cause of all of our sorrows over",
             "and all of our revolts against biological survival.",
             "Pere Ubu is a human metabolism fuelled by",
             "near-scientific speculations.",
             "Ubu Roi says that in his country no state of being",
             "excludes its opposite;",
             "everything is not only possible but real.",
             "\"Pschitt!\"")
    for line in lines:
        label = gtk.Label("")
        label.set_markup(line)
        txt_vbox.pack_start(label, expand=0, fill=0, padding=2)

    _about_ubu_win.vbox.pack_start(hbox, padding=5)
    _about_ubu_win.vbox.show_all()

    _about_ubu_win.show_all()


