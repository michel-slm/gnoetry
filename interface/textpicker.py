
import gobject, gtk, gnoetics
import sys, os, threading

text_dirs = [ "../texts-ts" ]
text_cache = []

def _text_sort_fn(a, b):

    a = a.get_title().lower()
    b = b.get_title().lower()

    if a[:4] == "the ":
        a = a[4:]
    if b[:4] == "the ":
        b = b[4:]

    return cmp(a.strip(), b.strip())


def get_all_texts():
    global text_cache
    if len(text_cache) == 0:
        for dir in text_dirs:
            for file in os.listdir(dir):
                path = os.path.join(dir, file)
                ext = os.path.splitext(file)[1]
                if ext == ".ts":
                    text = gnoetics.Text(path)
                    text_cache.append(text)
        text_cache.sort(_text_sort_fn)

    return text_cache

                                              
class TextPicker(gtk.Dialog):

    COLUMN_FLAG = 0
    COLUMN_TITLE = 1
    COLUMN_AUTHOR = 2
    COLUMN_TEXT = 3

    def __init__(self, model):
        gtk.Dialog.__init__(self, "Select Your Source Texts")

        b = self.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)

        # If no texts are initially set in the model, don't allow the user
        # to cancel out of the dialog.
        if len(model.get_texts()) == 0:
            b.set_sensitive(0)
        
        self.__button_ok = self.add_button(gtk.STOCK_OK, gtk.RESPONSE_CLOSE)
        self.connect("response", TextPicker.__response_handler)

        self.__model = model

        self.__store = gtk.ListStore(gobject.TYPE_BOOLEAN,
                                     gobject.TYPE_STRING,
                                     gobject.TYPE_STRING,
                                     gobject.TYPE_PYOBJECT)

        self.__count = 0
        self.__count_label = gtk.Label("")
        
        self.__text_dict = {}
        
        for txt in get_all_texts():
            flag = self.__text_in_model(txt)
            if flag:
                self.__flag_text(txt, 1)
            iter = self.__store.append()
            self.__store.set(iter,
                             self.COLUMN_FLAG, flag,
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

        self.vbox.pack_start(swin, expand=1, fill=1)

        self.__update_count()
        self.__count_label.show()

        self.vbox.pack_start(self.__count_label, expand=0, fill=0)


    def __flag_text(self, txt, flag):
        old_flag = self.__text_dict.get(txt)
        flag = (flag and 1) or 0
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


    def __text_in_model(self, txt):
        for t in self.__model.get_texts():
            if t.get_title() == txt.get_title() \
               and t.get_author() == txt.get_author():
                return 1
        return 0


    def __response_handler(self, id):
        if id == gtk.RESPONSE_CLOSE:
            self.__model.clear()
            for txt, flag in self.__text_dict.items():
                if flag:
                    print "Adding", txt
                    self.__model.add_text(txt)
        self.destroy()

            

            
                                 
            
    


