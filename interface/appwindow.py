# This is -*- Python -*-

import os, tempfile, sys, threading, time

import gnoetics

import gtk
import red_menubar, red_toolbar
import about

from poempicker   import *
from textpicker   import *
from weightpicker import *
from poemtileview import *
from poemtextview import *

###
### Launcher
###

def launch(is_first=False):

    picker = PoemPicker()
    picker.connect("finished", poem_picker_finished_cb, is_first)
    picker.show_all()

master_lib = None
def poem_picker_finished_cb(pp, poem, is_first):
    if poem is None:
        if is_first:
            sys.exit(0)
        return
    
    global master_lib
    if master_lib is None:
        master_lib = gnoetics.Library("../texts-ts")

    picker = TextPicker(master_lib)
    picker.connect("finished", text_picker_finished_cb, poem, is_first)
    picker.show_all()
    

def text_picker_finished_cb(picker, text_list, poem, is_first):
    if text_list is None:
        if is_first:
            sys.exit(0)
        return

    # Create a model, add the texts and start preparing it (which will
    # happen in another thread).
    tri = gnoetics.Trimodel()
    for txt in text_list:
        tri.add_text(txt)
    tri.prepare()

    # Create initial dict of weights
    weights = {}
    for txt in text_list:
        weights[txt] = 1.0

    if len(text_list) > 1:
        wp = WeightPicker(weights)
        wp.show_all()
        wp.connect("finished", weight_picker_finished_cb, poem, tri, weights)
    else:
        weight_picker_finished_cb(None, poem, tri, weights)

def weight_picker_finished_cb(picker, poem, model, weights):

    if picker:
        picker.destroy()
        
    if model.is_ready():
        launch_appwindow(poem, model, weights, None)
    else:
        win = gtk.MessageDialog(None, 0,
                                gtk.MESSAGE_INFO,
                                gtk.BUTTONS_NONE,
                                " Building statistical model")
        win.show_all()
        gobject.timeout_add(100, wait_for_model_cb, poem, model, weights, win)
    

def wait_for_model_cb(poem, model, weights, win):
    if model.is_ready():
        launch_appwindow(poem, model, weights, win)
        return False
    return True


def launch_appwindow(poem, model, weights, win):
    if win:
        win.destroy()
    appwin = AppWindow(model=model,
                       weights=weights)
    appwin.set_poem(poem)
    num_lines = poem.num_lines()
    # Boost for stanzas
    num_lines += poem.num_stanzas() - 1
    # Sketchy hardwired sizes.  Very gross!
    appwin.set_size_request(-1, min(1000, 250 + int(36 * num_lines)))
    appwin.show_all()
        

###
### Callbacks
###

def new_callback(app):
    new = AppWindow(model=app.get_model())
    new.set_poem(app.get_poem().copy(clear=True))
    new.show_all()


def copy_callback(app):
    new = app.copy()
    new.show_all()


def save_callback(app):
    app.save(None)


def save_as_callback(app):
    app.save(None, use_previous=False)


_TEX_DOCUMENT = """
\\documentclass[12pt]{article}
\\usepackage{fancyhdr}
\\pagestyle{fancy}
\\headheight 18pt
\\lhead{\\large\\tt Beard of Bees Press}
\\rhead{\\tt www.beardofbees.com}
\\cfoot{\\relax}
\\begin{document}
\\null\\vskip 6ex minus 4ex

%(markup)s
\\tiny
\\vskip 2ex
\\parindent=0in
Texts:
\par
%(texts)s
\\end{document}
"""

def print_callback(app):
    # Save anything we print to /home/gnoetry/printed.
    app.save("/home/gnoetry/printed/%s" % app.get_default_filename())
    markup = app.get_poem().to_string(add_timestamp=True,
                                      add_latex_markup=True)

    texts_list = []
    for txt in app.get_model().get_texts():
        texts_list.append("%s, {\\it %s}" % (
                txt.get_author(), txt.get_title()))
    texts = '\\par\n'.join(texts_list)

    fh = tempfile.NamedTemporaryFile()
    fh.write(_TEX_DOCUMENT % {'markup': markup,
                              'texts': texts})
    fh.flush()

    dvi_name = fh.name + ".dvi"

    os.chdir("/tmp")
    os.system("latex %s" % fh.name)

    lpr_cmd = os.getenv("GNOETRY_LPR") or "lpr"
    os.system("dvips -f %s | %s" % (dvi_name, lpr_cmd))


def close_callback(app):
    app.close_window()


def undo_callback(app):
    app.undo()


def redo_callback(app):
    app.redo()


def clear_selection_callback(app):
    p = app.get_poem()
    for i in range(len(p)):
        p.set_flag(i, False)


def modify_weights_callback(app):
    app.modify_weights()


def regenerate_selected_callback(app):
    app.save_for_undo()
    p = app.get_poem()
    p_copy = p.copy()
    p.freeze_changed()
    p.unbind_flagged()

    try:
        app.solve()
    except gnoetics.SolveFailed:
        sys.stderr.write("Oops!\n")
        app.set_poem(p_copy)
        
    p.thaw_changed()
    app.fix_position()


###
### AppWindow class
###

class AppWindow(gtk.Window,
                gnoetics.PoemListener):

    __total_app_window_count = 0

    def __init__(self, model,
                 weights=None):
        
        gtk.Window.__init__(self)
        
        self.set_title("Gnoetry 0.2")

        AppWindow.__total_app_window_count += 1
        self.connect("delete_event", lambda aw, ev: aw.close_window())

        self.__model = model
        self.__solver = gnoetics.Solver(self.__model)

        self.__weights = weights or {}
        self.__weight_picker = None

        self.__save_filename = None
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

        self.fix_position()

        # Yes, we are doing this last on purpose.
        gnoetics.PoemListener.__init__(self)


    def fix_position(self):
        self.set_position(gtk.WIN_POS_CENTER_ALWAYS)


    def copy(self):
        cpy = AppWindow(model=self.get_model())

        # We explicitly *don't* copy the save filename
        #cpy.__save_filename = self.__save_filename
        
        cpy.__save_seqno = self.__save_seqno
        cpy.__undo_history = map(lambda x: x.copy(), self.__undo_history)
        cpy.__redo_history = map(lambda x: x.copy(), self.__redo_history)
        cpy.set_poem(self.get_poem().copy())

        return cpy


    def get_model(self):
        return self.__model


    def get_solver(self):
        return self.__solver

    def solve(self):
        self.__solver.full_solution(weights=self.__weights)


    def set_poem(self, p):
        gnoetics.PoemListener.set_poem(self, p)
        self.__save_seqno = -1
        
        self.__tileview.set_poem(p)
        self.__textview.set_poem(p)
        self.__solver.set_poem(p)

        self.solve()


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
        bar.add("/_File/sep1", is_separator=True)
        bar.add("/_File/_Save",
                stock=gtk.STOCK_SAVE,
                description="Save the current poem",
                callback=save_callback)
        bar.add("/_File/_Save AS",
                stock=gtk.STOCK_SAVE_AS,
                description="Save the current poem with a different name",
                callback=save_as_callback)
        bar.add("/_File/sep2", is_separator=True)
        bar.add("/_File/_Print",
                stock=gtk.STOCK_PRINT,
                description="Print the current poem",
                callback=print_callback)
        bar.add("/_File/sep3", is_separator=True)
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
        bar.add("/_Edit/sep1", is_separator=True)
        bar.add("/_Edit/Deselect",
                description="Deselect all currently selected words",
                pixbuf_name="deselect_16",
                sensitive_fn=contains_flagged_check,
                callback=clear_selection_callback)
        bar.add("/_Edit/sep2", is_separator=True)
        bar.add("/_Edit/Modify Text Weights",
                description="Adjust the weighting of the source texts",
                sensitive_fn=lambda: len(self.__weights) > 1,
                callback=modify_weights_callback)
        bar.add("/_Edit/sep3", is_separator=True)
        bar.add("/_Edit/Regenerate",
                description="Remove the selected words and make new choices",
                pixbuf_name="regenerate_16",
                sensitive_fn=contains_flagged_check,
                callback=regenerate_selected_callback)

        bar.add("/_Help/About Gnoetry",
                description="Learn more about Gnoetry",
                callback=lambda x: about.show_about_gnoetry())
        ###bar.add("/_Help/About Beard of Bees",
        ###        description="Learn more about Beard of Bees",
        ###        callback=lambda x: about.show_about_beard_of_bees())
        bar.add("/_Help/About Ubu Roi",
                description="Learn more about Ubu Roi",
                callback=lambda x: about.show_about_ubu_roi())


    def __assemble_toolbar(self, bar):

        def contains_flagged_check():
            p = self.get_poem()
            return p and p.contains_flagged()


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

        bar.add("Deselect",
                "Clear current selection",
                pixbuf_name="deselect_24",
                sensitive_fn=contains_flagged_check,
                callback=clear_selection_callback)

        bar.add("Regenerate",
                "Remove the selected words and make new choices",
                pixbuf_name="regenerate_24",
                sensitive_fn=contains_flagged_check,
                callback=regenerate_selected_callback)


    def poem_changed(self, p):
        self.__toolbar.sensitize_toolbar_items()

    def poem_changed_flag(self, p, i):
        self.__toolbar.sensitize_toolbar_items()


    def save_for_undo(self, clear_redo=True):
        p = self.get_poem()
        if p and p.is_fully_bound():
            self.__undo_history.append(p.copy())
            if len(self.__undo_history) > 100:
                self.__undo_history.pop(0)
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

    def modify_weights(self):
        if not self.__weights:
            return

        if self.__weight_picker:
            self.__weight_picker.present()
            return

        def finished_cb(picker, app):
            app.__weight_picker = None
            picker.destroy()

        wp = WeightPicker(self.__weights)
        wp.connect("finished", finished_cb, self)
        wp.show_all()

        self.__weight_picker = wp
        

    def new_window(self):
        new_win = AppWindow()
        new_win.show_all()
        new_win.fix_position()
        

    def close_window(self, require_confirmation=True):
        if require_confirmation and self.has_unsaved_changes():
            warn = gtk.MessageDialog(self,
                                     gtk.DIALOG_MODAL,
                                     gtk.MESSAGE_WARNING,
                                     gtk.BUTTONS_NONE,
                                     "You have unsaved changes.\n"
                                     "Are you sure you want to quit?")
            warn.add_button("Don't Save", 0)
            warn.add_button(gtk.STOCK_CANCEL, 1)
            warn.add_button(gtk.STOCK_SAVE, 2)
            warn.show_all()
            x = warn.run()
            warn.destroy()

            if x == 1:
                return True
            elif x == 2:
                self.save(None)

        self.destroy()
        AppWindow.__total_app_window_count -= 1
        if AppWindow.__total_app_window_count == 0:
            gtk.main_quit()
        return True


    def has_unsaved_changes(self):
        return self.__save_seqno != self.get_poem().get_seqno()


    def get_default_filename(self):
        words = []
        i = 0
        p = self.get_poem()
        while i < len(p):
            u = p[i]
            if u.is_bound() \
               and not u.is_punctuation() \
               and not u.is_break():
                words.append(u.get_binding().get_word())
            if u.is_end_of_line():
                break
            i += 1
            

        if not words:
            words = ["poem"]

        name = string.join(words, "-") + ".txt"

        dir = os.getenv("GNOETRY_SAVE_DIR")
        if dir:
            name = os.path.join(dir, name)

        return name


    def save(self, target_filename, use_previous=True):
        p = self.get_poem()

        if target_filename is None and use_previous:
            target_filename = self.__save_filename

        if target_filename is None:
            filesel = gtk.FileSelection("Save Your Timeless Work of Art")
            guess = self.__save_filename or self.get_default_filename()
            filesel.set_filename(guess)
            x = filesel.run()
            target_filename = filesel.get_filename()
            filesel.destroy()
            if x == gtk.RESPONSE_CANCEL:
                return

        self.__save_filename = target_filename

        fh = file(target_filename, "w")
        fh.write(p.to_string(add_timestamp=True))
        fh.write("\n\n\nTexts:\n")
        for txt in self.get_model().get_texts():
            fh.write("%s, %s\n" % (txt.get_author(), txt.get_title()))
        fh.close()

        self.__save_seqno = p.get_seqno()
        
            
