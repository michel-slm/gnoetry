# This is -*- Python -*-

#import gnoetics
import gtk
import red_pixbuf

class PoemChoice:

    def __init__(self, label):
        self.__label = label

    def get_label(self):
        return self.__label

    def get_description(self):
        return None

    def get_widget(self):
        return None

    def build_poem(self):
        return None


class PoemChoice_Haiku(PoemChoice):

    def __init__(self):
        PoemChoice.__init__(self, "Haiku")

    def get_description(self):
        return "Classic Japanese 5-7-5 stuff"


class PoemChoice_Tanka(PoemChoice):

    def __init__(self):
        PoemChoice.__init__(self, "Tanka")

    def get_description(self):
        return "Classic Japanese 5-7-5 7-7 stuff"


class PoemChoice_BlankVerse(PoemChoice):

    def __init__(self):
        PoemChoice.__init__(self, "Blank Verse")

    def get_description(self):
        return "If it is good enough for Milton,\nit is good enough for me!"

    def get_widget(self):
        stanza_adj = gtk.Adjustment()
        stanza_b = gtk.Entry()
        #stanza_b.set_range(1, 10)
        #stanza_b.set_value(3)
        #stanza_b.set_increments(1, 1)

        lines_adj = gtk.Adjustment(4, 2, 15, 1)
        lines_b = gtk.SpinButton(lines_adj)

        box = gtk.HBox()
        box.pack_start(stanza_b)
        box.pack_start(gtk.Label("stanzas, each with"))
        box.pack_start(lines_b)
        box.pack_start(gtk.Label("lines."))

        return box
        

class PoemPicker(gtk.VBox):

    def __init__(self):
        gtk.VBox.__init__(self)
        self.__first = None
        self.__choices = []

    def add(self, pc):
        rb = gtk.RadioButton(self.__first)
        if self.__first is None:
            self.__first = rb

        box = gtk.VBox()

        w = gtk.Label()
        w.set_markup("<b>%s</b>" % pc.get_label())
        w.set_alignment(0, 0.5)
        box.pack_start(w, expand=True, fill=True)

        w = pc.get_widget()
        if not w:
            desc = pc.get_description()
            if desc:
                w = gtk.Label()
                w.set_markup(desc)

        if w:
            box.pack_start(w, expand=True, fill=True, padding=3)

        rb.add(box)

        if self.__choices:
            w = gtk.HSeparator()
            w.show_all()
            self.pack_start(w, expand=False, fill=True, padding=3)

        self.pack_start(rb)
        rb.show_all()

        self.__choices.append((pc, rb))


    def get_poem(self):
        for pc, rb in self.__choices:
            if rb.get_active():
                return pc.get_poem()
        return None




win = gtk.Window()
box = gtk.VBox()
win.add(box)

pp = PoemPicker()
pp.add(PoemChoice_Haiku())
pp.add(PoemChoice_Tanka())
pp.add(PoemChoice_BlankVerse())
box.pack_start(pp)

b = gtk.Button("Foo!")
box.pack_start(b)

def foo_cb(b, pp):
    print pp.get_poem()
b.connect("clicked", foo_cb, pp)

win.show_all()

gtk.main()

    
