###
### Copyright (C) 2002-2003 Ximian, Inc.
###
### This program is free software; you can redistribute it and/or modify
### it under the terms of the GNU General Public License, version 2,
### as published by the Free Software Foundation.
###
### This program is distributed in the hope that it will be useful,
### but WITHOUT ANY WARRANTY; without even the implied warranty of
### MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
### GNU General Public License for more details.
###
### You should have received a copy of the GNU General Public License
### along with this program; if not, write to the Free Software
### Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
###

import os, string, types, re, gobject, gtk
import red_pixbuf

## The format looks like "<Control>a" or "<Shift><Alt>F1.
## The parser is fairly liberal and allows lower or upper case, and also
## abbreviations such as "<Ctl>" and "<Ctrl>".

class AcceleratorParser:

    def __init__(self, s=None):
        self.__key = None
        self.__mods = 0

        self.pattern = re.compile("<([a-z]+)+>", re.IGNORECASE)
        self.parse(s)

    def parse(self, s=None):
        self.__key = None
        self.__mods = 0

        if not s:
             return

        mods = self.pattern.findall(s)
        self.parse_mods(mods)

        key = self.pattern.sub("", s)
        self.parse_key(key)

        # No key, no joy!
        if not self.key():
            self.mods = 0

    def key(self):
        return self.__key
    def mods(self):
        return self.__mods

    ## End of public methods

    def parse_mods(self, mods=None):
        if not mods:
            return

        for m in mods:
            m = m[0].lower()
            if m == 's':
                self.__mods |= gtk.gdk.SHIFT_MASK
            elif m == 'c':
                self.__mods |= gtk.gdk.CONTROL_MASK
            elif m == 'a':
                self.__mods |= gtk.gdk.MOD1_MASK

    def parse_key(self, key):
        if key:
            self.__key = gtk.gdk.keyval_from_name(key)
        else:
            sel.__key = None

class MenuBar(gtk.MenuBar):

    def __init__(self, accel_group=None):
        gobject.GObject.__init__(self)

        #self.accel_group = accel_group
        #if accel_group:
        #    accel_group.connect("accel-activate",
        #                        lambda g,o,x,y,this:this.refresh_items(),
        #                        self)
        #self.accel_parser = AcceleratorParser()

        self.constructed = 0
        self.pending_items = []
        self.pending_items_hash = {}
        self.user_data = None
        self.statusbar = None

        # Automatically construct our menu items, and refresh the items,
        # when we are realized.
        def on_realize(x):
            x.construct()
            x.refresh_items()
            
        self.connect("realize",
                     on_realize)

    def set_statusbar(self, statusbar):
        self.statusbar = statusbar

    def set_user_data(self, x):
        self.user_data = x
        
    def refresh_items(self):
        self.emit("refresh_items")


    def add(self, path,
            description=None,
            callback=None,
            with_dropdown_arrow=0,
            is_separator=0,
            visible_fn=None,
            sensitive_fn=None,
            stock=None,
            image=None,
            pixbuf=None,
            pixbuf_name=None,
            checked_get=None, checked_set=None,
            radiogroup=None,
            radiotag=None,
            radio_get=None,
            radio_set=None,
            accelerator=None):

        if self.constructed:
            print "Can't add '%s' to an already-constructed menu bar." \
                  % path
            assert 0

        prefix, name = os.path.split(path)
        path = string.replace(path, "_", "")

        if self.pending_items_hash.has_key(path):
            print "Collision: there is already a menu item with path '%s'" \
                  % path
            assert 0

        if pixbuf_name:
            assert not pixbuf and not image
            image = red_pixbuf.get_widget(pixbuf_name)

        if pixbuf:
            assert not pixbuf_name and not image
            image = gtk.Image()
            image.set_from_pixbuf(pixbuf)

        item = {"path":path,
                "name":name,
                "description":description,
                "callback":callback,
                "with_dropdown_arrow":with_dropdown_arrow,
                "is_separator":is_separator,
                "visible_fn":visible_fn,
                "sensitive_fn":sensitive_fn,
                "stock":stock,
                "image":image,
                "checked_get":checked_get,
                "checked_set":checked_set,
                "radiogroup":radiogroup,
                "radiotag":radiotag,
                "radio_get":radio_get,
                "radio_set":radio_set,
                "accelerator":accelerator,
                }

        self.pending_items.append(item)
        self.pending_items_hash[path] = item

    def exercise_menubar(self):
        for item in self.pending_items:
            if item["path"][:7] != "/Debug/" \
               and item["path"] != "/File/Quit" \
               and item["callback"]:
                print item["path"]
                item["callback"](self.user_data)


    def construct(self):

        # We can only be constructed once.
        if self.constructed:
            return
        self.constructed = 1

        tree_structure = {}
        radiogroups = {}
        
        for item in self.pending_items:
            prefix, base = os.path.split(item["path"])
            if tree_structure.has_key(prefix):
                tree_structure[prefix].append(base)
            else:
                tree_structure[prefix] = [base]

        def walk_tree(prefix, parent_menu):

            for name in tree_structure[prefix]:

                path = os.path.join(prefix, name)
                item = self.pending_items_hash[path]

                needs_refresh = item["visible_fn"] or \
                                item["sensitive_fn"]

                is_leaf = not tree_structure.has_key(path)

                item_name = item["name"] or ""

                ### Flag items that aren't hooked up to callbacks.
                if is_leaf and not item["callback"]:
                    item_name = item_name + " (inactive)"

                if item["is_separator"]:
                    menu_item = gtk.SeparatorMenuItem()
                    
                elif item["stock"]:
                    #menu_item = gtk.ImageMenuItem(item["stock"],
                    #                              self.accel_group)
                    menu_item = gtk.ImageMenuItem(item["stock"])
                elif item["image"]:
                    menu_item = gtk.ImageMenuItem(item["name"])
                    menu_item.set_image(item["image"])
                elif item["radiogroup"] and item["radiotag"]:

                    grp = radiogroups.get(item["radiogroup"])
                    grp_widget = None
                    if grp:
                        grp_widget, grp_item = grp
                        item["radio_get"] = grp_item["radio_get"]
                        item["radio_set"] = grp_item["radio_set"]
                        
                    
                    menu_item = gtk.RadioMenuItem(grp_widget, item["name"])
                    if not grp:
                        #assert item["radio_get"] and item["radio_set"]
                        radiogroups[item["radiogroup"]] = (menu_item,
                                                           item)

                    def radio_activate(mi, get_fn, set_fn, tag):
                        if get_fn() != tag:
                            set_fn(tag)

                    menu_item.connect_after("activate",
                                            radio_activate,
                                            item["radio_get"],
                                            item["radio_set"],
                                            item["radiotag"])

                    needs_refresh = 1
                    
                elif item["checked_get"] and item["checked_set"]:
                    menu_item = gtk.CheckMenuItem(item["name"])
                    menu_item.set_active(item["checked_get"]())
                    needs_refresh = 1
                    
                    def check_activate(mi, get_fn, set_fn):
                        state = mi.get_active()
                        x = (get_fn() and 1) or 0
                        if x ^ state:
                            set_fn(state)
                            
                    menu_item.connect_after("activate",
                                            check_activate,
                                            item["checked_get"],
                                            item["checked_set"])
                else:
                    if item["with_dropdown_arrow"]:
                        menu_item = gtk.MenuItem()
                        hbox = gtk.HBox(0, 0)
                        hbox.pack_start(gtk.Label(item_name), 0, 0, 0)
                        hbox.pack_start(gtk.Arrow(gtk.ARROW_DOWN,
                                                  gtk.SHADOW_OUT), 0, 0, 0)
                        menu_item.add(hbox)
                    else:
                        menu_item = gtk.MenuItem(item_name)

                if self.statusbar and item["description"]:
                    def select_cb(mi, sb, i):
                        sb.push(hash(mi), i["description"])

                    def deselect_cb(mi, sb):
                        sb.pop(hash(mi))

                    menu_item.connect("select", select_cb,
                                      self.statusbar, item)
                    menu_item.connect("deselect", deselect_cb, self.statusbar)

                parent_menu.append(menu_item)
                menu_item.show_all()

                ### If this item is a leaf in our tree,
                ### hook up it's callback

                if is_leaf and item["callback"]:
                    menu_item.connect_after(
                        "activate",
                        lambda x, i:i["callback"](self.user_data),
                        item)

                if item["accelerator"]:
                    self.accel_parser.parse(item["accelerator"])
                    key = self.accel_parser.key()
                    if key:
                        mods = self.accel_parser.mods()
                        menu_item.add_accelerator("activate",
                                                  self.accel_group,
                                                  key, mods,
                                                  gtk.ACCEL_VISIBLE)

                ###
                ### If this item has special visibility, sensitivity or checked
                ### functions, hook them up to listen for our refresh_items
                ### signals.
                ###

                def refresh_items(widget, item):
                    visible_fn = item["visible_fn"]
                    if (not visible_fn) or visible_fn():
                        widget.show()
                    else:
                        widget.hide()

                    def eval_fn_or_tuple(fn):
                        if not fn:
                            return 1
                        elif callable(fn):
                            return (fn() and 1) or 0
                        elif type(fn) == types.TupleType \
                             or type(fn) == types.ListType:
                            assert(len(fn) > 0)
                            assert(callable(fn[0]))
                            return (apply(fn[0], fn[1:]) and 1) or 0
                        print "Couldn't eval", fn
                        return 0

                    is_sensitive = eval_fn_or_tuple(item["sensitive_fn"])
                    widget.set_sensitive(is_sensitive)

                    if item["checked_get"]:
                        is_checked = eval_fn_or_tuple(item["checked_get"])
                        widget.set_active(is_checked)

                    radiogroup = item["radiogroup"]
                    radiotag = item["radiotag"]
                    radio_get = item["radio_get"]
                    radio_set = item["radio_set"]
                    if radiogroup and radiotag and radio_get and radio_set:
                        active_tag = radio_get()
                        widget.set_active(radiotag == active_tag)

                if needs_refresh:
                    self.connect("refresh_items",
                                 lambda menu, x, y: refresh_items(x, y),
                                 menu_item, item)

                ###
                ### If this item has subitems, construct the submenu
                ### and continue walking down the tree.
                ###

                if not is_leaf:

                    # Refresh the menu bar every time a top-level
                    # menu item is opened.
                    if prefix == "/":
                        menu_item.connect("activate",
                                          lambda x:self.refresh_items())
                    submenu = gtk.Menu()
                    menu_item.set_submenu(submenu)
                    submenu.show()
                    walk_tree(path, submenu)

        walk_tree("/", self)

gobject.type_register(MenuBar)


gobject.signal_new("refresh_items",
                   MenuBar,
                   gobject.SIGNAL_RUN_LAST,
                   gobject.TYPE_NONE,
                   ())
