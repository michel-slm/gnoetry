
import gobject, gtk, gnoetics
import sys, os, threading

class TextPicker(gobject.GObject):

    COLUMN_FLAG = 0
    COLUMN_TITLE = 1
    COLUMN_AUTHOR = 2
    COLUMN_TEXT = 3

    def __init__(self, lib):
        gobject.GObject.__init__(self)

        self.__d = gtk.Dialog("Select Your Source Texts")

        self.__button_ok = self.__d.add_button(gtk.STOCK_OK,
                                               gtk.RESPONSE_CLOSE)

        def response_handler_cb(dialog, id):
            texts = None
            if id == gtk.RESPONSE_CLOSE:
                texts = []
                for text, flag in self.__text_dict.items():
                    if flag:
                        texts.append(text)
            self.emit("finished", texts)
            self.destroy()
        self.__d.connect("response", response_handler_cb)

        self.__store = gtk.ListStore(gobject.TYPE_BOOLEAN,
                                     gobject.TYPE_STRING,
                                     gobject.TYPE_STRING,
                                     gobject.TYPE_PYOBJECT)

        self.__count = 0
        self.__count_label = gtk.Label("")
        
        self.__text_dict = {}

        all_texts = lib.get_all()
        all_texts.sort(lambda x, y: cmp(x.get_sort_title(),
                                        y.get_sort_title()))
        
        for txt in all_texts:
            iter = self.__store.append()
            self.__store.set(iter,
                             self.COLUMN_FLAG, False,
                             self.COLUMN_TITLE, txt.get_title(),
                             self.COLUMN_AUTHOR, txt.get_author(),
                             self.COLUMN_TEXT, txt)


        treeview = gtk.TreeView(self.__store)

        def text_toggle(cell, path, picker):
            iter = picker.__store.get_iter((int(path),))
            flag = picker.__store.get_value(iter, self.COLUMN_FLAG)
            txt = picker.__store.get_value(iter, self.COLUMN_TEXT)
            flag = not flag
            self.__flag_text(txt, flag)
            picker.__store.set(iter, self.COLUMN_FLAG, flag)
            picker.__update_count()

        renderer = gtk.CellRendererToggle()
        col = gtk.TreeViewColumn("Active",
                                 renderer,
                                 active=self.COLUMN_FLAG)
        renderer.connect("toggled", text_toggle, self)
        col.set_clickable(1)
        treeview.append_column(col)

        col = gtk.TreeViewColumn("Title",
                                 gtk.CellRendererText(),
                                 text=self.COLUMN_TITLE)
        treeview.append_column(col)

        col = gtk.TreeViewColumn("Author",
                                 gtk.CellRendererText(),
                                 text=self.COLUMN_AUTHOR)
        treeview.append_column(col)

        swin = gtk.ScrolledWindow()
        swin.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
        swin.add_with_viewport(treeview)
        swin.show_all()

        swin.set_size_request(-1, 500)

        self.__d.vbox.pack_start(swin, expand=1, fill=1)

        self.__update_count()
        self.__count_label.show()

        self.__d.vbox.pack_start(self.__count_label, expand=0, fill=0)


    def __flag_text(self, txt, flag):
        old_flag = self.__text_dict.get(txt, False)
        if flag != old_flag:
            self.__text_dict[txt] = flag
            if flag:
                txt.preload()
                self.__count += 1
            else:
                self.__count -= 1
            self.__update_count()


    def __update_count(self):
        if self.__count == 0:
            msg = "No texts selected"
        elif self.__count == 1:
            msg = "1 text selected"
        else:
            msg = "%d texts selected" % self.__count

        self.__count_label.set_text(msg)

        self.__button_ok.set_sensitive(self.__count > 0)


    def show_all(self):
        self.__d.show_all()

        
    def destroy(self):
        self.__d.destroy()
            

gobject.type_register(TextPicker)

gobject.signal_new("finished",
                   TextPicker,
                   gobject.SIGNAL_RUN_LAST,
                   gobject.TYPE_NONE,
                   (gobject.TYPE_PYOBJECT,))
