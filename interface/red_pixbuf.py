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

import os, gobject, gtk, sys

pixbuf_path = [ "../art" ]

pixbuf_cache = {}

def get_pixbuf(name, fail_gracefully=0, width=0, height=0):

    if not name:
        assert fail_gracefully
        return None

    if width > 0 and height > 0:
        key = "%s %d %d" % (name, width, height)
    else:
        key = name

    # If this isn't a string, maybe it is a GdkPixbuf.  If so, just
    # pass it through.  Otherwise something is very wrong, so we
    # assert.
    # FIXME: How do I check this type w/o the "try"?
    if type(name) != type(""):
        try:
            if gobject.type_is_a(name.__gtype__,
                                 gtk.gdk.Pixbuf.__gtype__):
                return name
        except:
            print "Unknown type passed to get_pixbuf"
            assert 0
        

    if pixbuf_cache.has_key(key):
        return pixbuf_cache[key]

    pixbuf = None
    
    if width > 0 and height > 0:

        original = get_pixbuf(name)
        pixbuf = original.scale_simple(width, height, gtk.gdk.INTERP_BILINEAR)

    else:

        for dir in pixbuf_path:
            filename = os.path.join(dir, name + ".png")
            if os.path.isfile(filename):
                pixbuf = gtk.gdk.pixbuf_new_from_file(filename)
                if not pixbuf:
                    print "Couldn't load pixbuf from '%s'" % filename
                    assert 0

    if pixbuf:
        pixbuf_cache[key] = pixbuf
        return pixbuf

    if not fail_gracefully:
        print "Couldn't find pixbuf '%s'" % name
        assert 0

    return None



def get_widget(name, fail_gracefully=0, width=0, height=0):

    pixbuf = get_pixbuf(name, fail_gracefully, width, height)

    img = None
    if pixbuf:
        img = gtk.Image()
        img.set_from_pixbuf(pixbuf)

    return img

