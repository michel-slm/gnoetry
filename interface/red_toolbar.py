###
### Copyright (C) 2003 Ximian, Inc.
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

import gtk
import red_pixbuf

class Toolbar(gtk.Toolbar):

    def __init__(self):
        gtk.Toolbar.__init__(self)

        self.update_pending = 0

        self.items = []
        self.user_data = None


    def set_user_data(self, x):
        self.user_data = x


    def add(self,
            text,
            tooltip,
            stock=None,
            pixbuf=None,
            pixbuf_name=None,
            sensitive_fn=None,
            callback=None):

        if pixbuf_name:
            assert not pixbuf
            pixbuf = red_pixbuf.get_pixbuf(pixbuf_name)

        if stock or pixbuf:
            image = gtk.Image()

            if stock:
                assert not pixbuf
                image.set_from_stock(stock, gtk.ICON_SIZE_LARGE_TOOLBAR)

            if pixbuf:
                assert not stock
                image.set_from_pixbuf(pixbuf)
        else:
            image = None

        w = self.append_item(text, tooltip, None, image, lambda x: callback(self.user_data))
        w.set_sensitive(0)

        item = {"text":text,
                "tooltip":tooltip,
                "stock":stock,
                "pixbuf":pixbuf,
                "sensitive_fn":sensitive_fn,
                "callback":callback,
                "widget":w
                }

        self.items.append(item)

        return w

    def sensitize_toolbar_items(self):
        for i in self.items:
            s = 1
            if i["sensitive_fn"]:
                s = i["sensitive_fn"]()

            i["widget"].set_sensitive(s)
        self.update_pending = 0
        return 0

