
import gobject, gtk, gnoetics
import sys, os, threading

def _text_sort_fn(a, b):

    a = a.get_title().lower()
    b = b.get_title().lower()

    if a[:4] == "the ":
        a = a[4:]
    if b[:4] == "the ":
        b = b[4:]

    return cmp(a.strip(), b.strip())


class TextPicker(gtk.Dialog):

    COLUMN_FLAG = 0
    COLUMN_TITLE = 1
    COLUMN_AUTHOR = 2
    COLUMN_TEXT = 3

    def __init__(self, lib, callback):
        gtk.Dialog.__init__(self, "Select Your Source Texts")

        self.__callback = callback

        self.__button_ok = self.add_button(gtk.STOCK_OK, gtk.RESPONSE_CLOSE)
        self.connect("response", TextPicker.__response_handler)

        self.__store = gtk.ListStore(gobject.TYPE_BOOLEAN,
                                     gobject.TYPE_STRING,
                                     gobject.TYPE_STRING,
                                     gobject.TYPE_PYOBJECT)

        self.__count = 0
        self.__count_label = gtk.Label("")
        
        self.__text_dict = {}

        all_texts = lib.get_all()
        all_texts.sort(_text_sort_fn)
        
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

        self.vbox.pack_start(swin, expand=1, fill=1)

        self.__update_count()
        self.__count_label.show()

        self.vbox.pack_start(self.__count_label, expand=0, fill=0)


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


    def __response_handler(self, id):
        if id == gtk.RESPONSE_CLOSE:
            texts = []
            for text, flag in self.__text_dict.items():
                if flag:
                    texts.append(text)
            self.__callback(texts)
        else:
            self.__callback(None)
        self.destroy()

            

            
                                 
            
    


