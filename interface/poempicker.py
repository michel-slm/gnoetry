# This is -*- Python -*-

import gnoetics
import gobject, gtk
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
        return "5-7-5"

    def build_poem(self):
        return gnoetics.Haiku()


class PoemChoice_Tanka(PoemChoice):

    def __init__(self):
        PoemChoice.__init__(self, "Tanka")

    def get_description(self):
        return "5-7-5\n7-7"

    def build_poem(self):
        return gnoetics.Tanka()


class PoemChoice_Renga(PoemChoice):

    def __init__(self):
        PoemChoice.__init__(self, "Renga")

    def get_description(self):
        return "5-7-5\n7-7\n5-7-5"

    def build_poem(self):
        return gnoetics.Renga()


class PoemChoice_Sonnet(PoemChoice):

    def __init__(self):
        PoemChoice.__init__(self, "Sonnet")

    def get_description(self):
        return "Sonnet!"

    def build_poem(self):
        return gnoetics.Sonnet()


class PoemChoice_BlankVerse(PoemChoice):

    def __init__(self):
        PoemChoice.__init__(self, "Blank Verse")

    def get_widget(self):
        self.__stanza_adj = gtk.Adjustment(3, 1, 10, 1)
        stanza_b = gtk.SpinButton(self.__stanza_adj)

        self.__lines_adj = gtk.Adjustment(4, 2, 15, 1)
        lines_b = gtk.SpinButton(self.__lines_adj)

        box1 = gtk.HBox()
        box1.pack_start(stanza_b, False, False, 3)
        box1.pack_start(gtk.Label("stanzas, each with"), False, False, 3)

        box2 = gtk.HBox()
        box2.pack_start(lines_b, False, False, 3)
        box2.pack_start(gtk.Label("lines."), False, False, 3)

        box = gtk.VBox()
        box.pack_start(box1, 0, 0, 3)
        box.pack_start(box2, 0, 0, 3)

        return box

    def build_poem(self):
        num_stanzas = int(self.__stanza_adj.get_value())
        num_lines = int(self.__lines_adj.get_value())
        return gnoetics.BlankVerse(num_stanzas, num_lines)


class PoemChoice_RandomSyllabic(PoemChoice):

    def __init__(self):
        PoemChoice.__init__(self, "Syllabic")

    def get_widget(self):
        self.__stanza_adj = gtk.Adjustment(3, 1, 10, 1)
        stanza_b = gtk.SpinButton(self.__stanza_adj)

        self.__lines_adj = gtk.Adjustment(4, 2, 15, 1)
        lines_b = gtk.SpinButton(self.__lines_adj)

        self.__min_adj = gtk.Adjustment(5, 1, 10, 1)
        min_b = gtk.SpinButton(self.__min_adj)

        self.__max_adj = gtk.Adjustment(10, 1, 20, 1)
        max_b = gtk.SpinButton(self.__max_adj)

        self.__same_b = gtk.CheckButton("Identical stanzas")

        box1 = gtk.HBox()
        box1.pack_start(stanza_b, False, False, 3)
        box1.pack_start(gtk.Label("stanzas, each with"), False, False, 3)

        box2 = gtk.HBox()
        box2.pack_start(lines_b, False, False, 3)
        box2.pack_start(gtk.Label("lines, each with between"), False, False, 3)

        box3 = gtk.HBox()
        box3.pack_start(min_b, False, False, 3)
        box3.pack_start(gtk.Label("and"))
        box3.pack_start(max_b, False, False, 3)
        box3.pack_start(gtk.Label("syllables."))

        box = gtk.VBox()
        box.pack_start(box1, 0, 0, 3)
        box.pack_start(box2, 0, 0, 3)
        box.pack_start(box3, 0, 0, 3)
        box.pack_start(self.__same_b, 0, 0, 3)

        return box

    def build_poem(self):
        num_stanzas = int(self.__stanza_adj.get_value())
        num_lines = int(self.__lines_adj.get_value())
        min_syllables = int(self.__min_adj.get_value())
        max_syllables = int(self.__max_adj.get_value())
        return gnoetics.RandomSyllabic(num_stanzas, num_lines,
                                       min_syllables, max_syllables,
                                       self.__same_b.get_active())


class PoemPicker(gobject.GObject):

    def __init__(self):
        gobject.GObject.__init__(self)

        self.__d = gtk.Dialog("Choose a Base Form")
        self.__first = None
        self.__choices = []

        self.__table = gtk.Table(0, 2)
        self.__d.vbox.add(self.__table)

        self.__d.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
        self.__d.add_button(gtk.STOCK_OK, gtk.RESPONSE_CLOSE)

        def response_handler_cb(dialog, id):
            choice = None
            if id == gtk.RESPONSE_CLOSE:
                for pc, rb, w in self.__choices:
                    if rb.get_active():
                        choice = pc.build_poem()
                        break
            self.emit("finished", choice)
            self.destroy()
        self.__d.connect("response", response_handler_cb)

        self.__assemble()


    def add(self, pc):

        rb = gtk.RadioButton(self.__first)
        w = pc.get_widget()

        if self.__first is None:
            self.__first = rb
        elif w:
            w.set_sensitive(False)

        self.__choices.append((pc, rb, w))
        
        if not w:
            desc = pc.get_description()
            if desc:
                w = gtk.Label()
                w.set_markup(desc)
                w.set_alignment(0, 0.5)

        n = len(self.__choices)

        box = gtk.VBox()

        lab = gtk.Label()
        lab.set_markup("<b>%s</b>" % pc.get_label())
        lab.set_alignment(0, 0.5)


        def sensitize_cb(b, pp):
            for pc, rb, w in pp.__choices:
                if w:
                    w.set_sensitive(rb == b)
            
        rb.connect_after("toggled", sensitize_cb, self)

        self.__table.resize(2*n-1, 3)

        if n > 1:
            sep = gtk.HSeparator()
            self.__table.attach(sep,
                                0, 3, 2*n-3, 2*n-2,
                                gtk.EXPAND | gtk.FILL, 0,
                                1, 3)
            sep.show_all()
            
        self.__table.attach(rb,
                            0, 1, 2*n-2, 2*n-1,
                            0, 0,
                            3, 3)

        self.__table.attach(lab,
                            1, 2, 2*n-2, 2*n-1,
                            0, 0,
                            3, 3)

        self.__table.attach(w,
                            2, 3, 2*n-2, 2*n-1,
                            gtk.EXPAND | gtk.FILL, 0,
                            3, 3)

        rb.show_all()
        lab.show_all()
        w.show_all()


    def __assemble(self):
        self.add(PoemChoice_Haiku())
        self.add(PoemChoice_Tanka())
        self.add(PoemChoice_Renga())
        #self.add(PoemChoice_Sonnet())
        self.add(PoemChoice_BlankVerse())
        self.add(PoemChoice_RandomSyllabic())


    def show_all(self):
        self.__d.show_all()

    def destroy(self):
        self.__d.destroy()


gobject.type_register(PoemPicker)

gobject.signal_new("finished",
                   PoemPicker,
                   gobject.SIGNAL_RUN_LAST,
                   gobject.TYPE_NONE,
                   (gobject.TYPE_PYOBJECT,))



    
