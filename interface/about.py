
import gtk
import pixbuf

_about_dict = {}

def _show_about_window(title):
    global _about_dict

    # If we already have the about window in question, present it.

    if _about_dict.has_key(title):
        _about_dict[title].present()
        return None

    # Otherwise construct a nicely behaved dialog and return the
    # vbox to the caller.

    about = gtk.Dialog(title)
    _about_dict[title] = about
    
    b = about.add_button(gtk.STOCK_OK, gtk.RESPONSE_CLOSE)
    b.connect("clicked", lambda b, w: w.destroy(), about)

    def clear_about(w, title):
        global _about_dict
        if _about_dict.has_key(title):
            del _about_dict[title]
    about.connect("destroy", clear_about, title)

    about.set_resizable(0)

    def show_dialog(w):
        w.show_all()
        return 0
    gtk.idle_add(show_dialog, about)
    
    return about.vbox


#############################################################################


def show_about_gnoetry():

    about = _show_about_window("About Gnoetry")

    if about:
        about.pack_start(gtk.Label("Insert 'About Gnoetry' Here"),
                         expand=1, fill=1)



def show_about_beard_of_bees():

    about = _show_about_window("About Beard of Bees")

    if about:
        about.pack_start(gtk.Label("Insert 'About Beard of Bees' Here"),
                         expand=1, fill=1)



def show_about_ubu_roi():

    about = _show_about_window("About Ubu Roi")

    if about:

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

        about.pack_start(hbox, padding=5)



