
import random
import gtk, gnoetics
import tilemodel, tileview, tiletextview
import about

class AppWindow(gtk.Window):

    __total_app_window_count = 0

    def __init__(self):
        gtk.Window.__init__(self)
        self.set_title("Gnoetry")

        AppWindow.__total_app_window_count += 1
        self.connect("delete_event", lambda aw, ev: aw.__close_window())

        self.__token_model = gnoetics.TokenModel(3)
        self.__tile_model = tilemodel.TileModel(5)
        for i in range(self.__tile_model.get_line_count()):
            if i == 3:
                self.__tile_model.append_tile(i, "stanza break")
                continue
            self.__tile_model.append_tile(i, "foo")
            for j in range(random.randrange(3, 5)):
                t = gnoetics.token_lookup("line %d" % i)
                self.__tile_model.append_tile(i, t)
                t = gnoetics.token_lookup("*punct* ,")
                self.__tile_model.append_tile(i, t)

            self.__tile_model.append_tile(i, "u-u-u-")
            self.__tile_model.append_tile(i, gnoetics.token_lookup("bar"))
            self.__tile_model.append_tile(i, gnoetics.token_lookup("*punct* !"))
        
        vbox = gtk.VBox()
        self.add(vbox)

        menubar = self.__assemble_menubar()
        vbox.pack_start(menubar, expand=0, fill=0)


        self.__tile_view = tileview.TileView(self.__tile_model)

        self.__tile_text_view = tiletextview.TileTextView(self.__tile_model)

        vpaned = gtk.VPaned()

        sw = gtk.ScrolledWindow()
        sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        sw.add_with_viewport(self.__tile_view)

        vpaned.pack1(sw, 1, 1)
       
        sw = gtk.ScrolledWindow()
        sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        sw.add_with_viewport(self.__tile_text_view)

        vpaned.pack2(sw, 0, 1)

        vbox.pack_start(vpaned, expand=1, fill=1)

        vbox.show_all()


        
        

    def __assemble_menubar(self):

        menubar = gtk.MenuBar()

        ##
        ## File Menu
        ##

        file_item = gtk.MenuItem("File")
        file_subm = gtk.Menu()
        file_item.set_submenu(file_subm)

        new_item = gtk.ImageMenuItem(gtk.STOCK_NEW)
        file_subm.append(new_item)
        new_item.connect("activate",
                         lambda mi, aw: aw.__new_window(),
                         self)

        open_item = gtk.ImageMenuItem(gtk.STOCK_OPEN)
        file_subm.append(open_item)
        open_item.set_sensitive(0)

        file_subm.append(gtk.SeparatorMenuItem())

        save_item = gtk.ImageMenuItem(gtk.STOCK_SAVE)
        file_subm.append(save_item)
        save_item.set_sensitive(0)

        save_as_item = gtk.ImageMenuItem(gtk.STOCK_SAVE_AS)
        file_subm.append(save_as_item)
        save_as_item.set_sensitive(0)

        file_subm.append(gtk.SeparatorMenuItem())

        quit_item = gtk.ImageMenuItem(gtk.STOCK_QUIT)
        file_subm.append(quit_item)
        quit_item.connect("activate",
                          lambda mi, aw: aw.__close_window(),
                          self)

        menubar.append(file_item)
        file_item.show_all()


        ##
        ## Edit Menu
        ##

        edit_item = gtk.MenuItem("Edit")
        edit_subm = gtk.Menu()
        edit_item.set_submenu(edit_subm)

        undo_item = gtk.ImageMenuItem(gtk.STOCK_UNDO)
        edit_subm.append(undo_item)
        undo_item.set_sensitive(0) # FIXME

        redo_item = gtk.ImageMenuItem(gtk.STOCK_REDO)
        edit_subm.append(redo_item)
        redo_item.set_sensitive(0) # FIXME

        menubar.append(edit_item)
        edit_item.show_all()
        

        ##
        ## Help Menu
        ##
        
        help_item = gtk.MenuItem("Help")
        help_subm = gtk.Menu()
        help_item.set_submenu(help_subm)

        about_item = gtk.MenuItem("About Gnoetry")
        help_subm.append(about_item)
        about_item.connect("activate",
                           lambda mi: about.show_about_gnoetry())

        about_bob_item = gtk.MenuItem("About Beard of Bees")
        help_subm.append(about_bob_item)
        about_bob_item.connect("activate",
                               lambda mi: about.show_about_beard_of_bees())

        about_ubu_item = gtk.MenuItem("About Ubu Roi")
        help_subm.append(about_ubu_item)
        about_ubu_item.connect("activate",
                               lambda mi: about.show_about_ubu_roi())

        menubar.append(help_item)
        help_item.show_all()


        return menubar


    def __new_window(self):
        new_win = AppWindow()
        new_win.show_all()
        

    def __close_window(self):

        self.destroy()
        AppWindow.__total_app_window_count -= 1
        if AppWindow.__total_app_window_count == 0:
            gtk.mainquit()
        return gtk.TRUE
            
