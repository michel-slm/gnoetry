import os, tempfile

import gnoetics

import gtk
import red_menubar, red_toolbar
import about

from poemtileview import *
from poemtextview import *

###
### Callbacks
###

def new_callback(app):
    new = AppWindow(model=app.get_model())
    new.set_poem(app.get_poem().copy(clear=True))
    new.show_all()


def copy_callback(app):
    new = AppWindow(model=app.get_model())
    new.set_poem(app.get_poem().copy())
    new.show_all()


def print_callback(app):
    markup = app.get_poem().to_string(add_timestamp=True,
                                      add_latex_markup=True,
                                      add_latex_wrapper=True)
    fh = tempfile.NamedTemporaryFile()
    fh.write(markup)
    fh.flush()

    dvi_name = fh.name + ".dvi"

    os.chdir("/tmp")
    os.system("latex %s" % fh.name)
    os.system("dvips %s" % dvi_name)
    

def close_callback(app):
    # FIXME: check seqno
    app.close_window()


def undo_callback(app):
    app.undo()


def redo_callback(app):
    app.redo()


def clear_selected_callback(app):
    app.save_for_undo()
    app.get_poem().freeze_changed()
    app.get_poem().unbind_flagged()
    app.get_solver().full_solution()
    app.get_poem().thaw_changed()


###
### AppWindow class
###

class AppWindow(gtk.Window,
                gnoetics.PoemListener):

    __total_app_window_count = 0

    def __init__(self, model):
        gtk.Window.__init__(self)
        
        self.set_title("Gnoetry 0.2")

        AppWindow.__total_app_window_count += 1
        self.connect("delete_event", lambda aw, ev: aw.close_window())

        self.__model = model
        self.__solver = gnoetics.Solver(self.__model)

        self.__save_seqno = -1

        self.__undo_history = []
        self.__redo_history = []

        self.__vbox = gtk.VBox(0, 0)
        self.add(self.__vbox)

        self.__accel_group = gtk.AccelGroup()
        self.add_accel_group(self.__accel_group)

        ### Menubar
        self.__menubar = red_menubar.MenuBar(self.__accel_group)
        self.__menubar.set_user_data(self)
        self.__assemble_menubar(self.__menubar)
        self.__vbox.pack_start(self.__menubar, expand=0, fill=1)

        ### Toolbar
        self.__toolbar = red_toolbar.Toolbar()
        self.__toolbar.set_user_data(self)
        self.__assemble_toolbar(self.__toolbar)
        self.__vbox.pack_start(self.__toolbar, expand=0, fill=1)


        ### The poem views
        self.__tileview = PoemTileView()
        self.__textview = PoemTextView()


        view_container = gtk.HBox(0, 10)
        view_container.set_size_request(-1, 300)

        sw = gtk.ScrolledWindow()
        sw.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
        sw.add_with_viewport(self.__tileview)

        view_container.pack_start(sw, expand=1, fill=1)
       
        sw = gtk.ScrolledWindow()
        sw.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
        sw.add_with_viewport(self.__textview)

        view_container.pack_start(sw, expand=1, fill=1)

        self.__vbox.pack_start(view_container, expand=1, fill=1)

        ### Statusbar
        self.__statusbar = gtk.Statusbar()
        self.__statusbar.show()
        self.__vbox.pack_start(self.__statusbar, expand=0, fill=1)
        self.__menubar.set_statusbar(self.__statusbar)

        self.__vbox.show_all()

        # Yes, we are doing this last on purpose.
        gnoetics.PoemListener.__init__(self)


    def get_model(self):
        return self.__model


    def get_solver(self):
        return self.__solver


    def set_poem(self, p):
        gnoetics.PoemListener.set_poem(self, p)
        self.__save_seqno = -1
        
        self.__tileview.set_poem(p)
        self.__textview.set_poem(p)
        self.__solver.set_poem(p)

        self.__solver.full_solution()


    def __assemble_menubar(self, bar):

        def contains_flagged_check():
            p = self.get_poem()
            return p and p.contains_flagged()
        
        bar.add("/_File")
        bar.add("/_Edit")
        bar.add("/_Help")

        bar.add("/_File/_New Window",
                stock=gtk.STOCK_NEW,
                description="Open a new Gnoetry window",
                callback=new_callback)
        bar.add("/_File/_Copy Window",
                stock=gtk.STOCK_COPY,
                description="Open a new Gnoetry window with the same poem",
                callback=copy_callback)
        bar.add("/_File/_Save",
                stock=gtk.STOCK_SAVE,
                description="Save the current poem to a text file")
        bar.add("/_File/_Print",
                stock=gtk.STOCK_PRINT,
                description="Print the current poem",
                callback=print_callback)
        bar.add("/_File/sep", is_separator=True)
        bar.add("/_File/_Close",
                stock=gtk.STOCK_CLOSE,
                description="Close this window",
                callback=close_callback)

        bar.add("/_Edit/Undo",
                stock=gtk.STOCK_UNDO,
                description="Undo the previous changes",
                sensitive_fn=lambda: self.can_undo(),
                callback=undo_callback)
        bar.add("/_Edit/Redo",
                stock=gtk.STOCK_REDO,
                description="Redo the previously undone changes",
                sensitive_fn=lambda: self.can_redo(),
                callback=redo_callback)
        bar.add("/_Edit/sep", is_separator=True)
        bar.add("/_Edit/Regenerate Selected Text",
                description="Remove the selected words and make new choices",
                sensitive_fn=contains_flagged_check,
                callback=clear_selected_callback)

        bar.add("/_Help/About Gnoetry",
                description="Learn more about Gnoetry",
                callback=lambda x: about.show_about_gnoetry())
        bar.add("/_Help/About Beard of Bees",
                description="Learn more about Beard of Bees",
                callback=lambda x: about.show_about_beard_of_bees())
        bar.add("/_Help/About Ubu Roi",
                description="Learn more about Ubu Roi",
                callback=lambda x: about.show_about_ubu_roi())


    def __assemble_toolbar(self, bar):

        bar.add("Undo",
                "Undo the previous changes",
                stock=gtk.STOCK_UNDO,
                sensitive_fn=lambda: self.can_undo(),
                callback=undo_callback)

        bar.add("Redo",
                "Redo the previously undone changes",
                stock=gtk.STOCK_REDO,
                sensitive_fn=lambda: self.can_redo(),
                callback=redo_callback)


    def poem_changed(self, p):
        self.__toolbar.sensitize_toolbar_items()


    def save_for_undo(self, clear_redo=True):
        p = self.get_poem()
        if p and p.is_fully_bound():
            self.__undo_history.append(p.copy())
            if clear_redo:
                self.__redo_history = []
        self.__toolbar.sensitize_toolbar_items()
            
    def can_undo(self):
        return len(self.__undo_history) > 0

    def undo(self):
        if self.can_undo():
            self.__redo_history.append(self.get_poem().copy())
            p = self.__undo_history.pop(-1)
            self.set_poem(p)
            

    def can_redo(self):
        return len(self.__redo_history) > 0

    def redo(self):
        if self.can_redo():
            p = self.__redo_history.pop(-1)
            self.save_for_undo(clear_redo=False)
            self.set_poem(p)
            

    def new_window(self):
        new_win = AppWindow()
        new_win.show_all()
        

    def close_window(self):
        self.destroy()
        AppWindow.__total_app_window_count -= 1
        if AppWindow.__total_app_window_count == 0:
            gtk.mainquit()
        return gtk.TRUE
            
