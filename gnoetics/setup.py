#!/usr/bin/python

import os, string
from distutils.core import setup, Extension

pkgs = "glib-2.0 gthread-2.0"

pc = os.popen("pkg-config --cflags-only-I %s" % pkgs, "r")
glib_includes = map(lambda x:x[2:], string.split(pc.readline()))
pc.close()

pc = os.popen("pkg-config --libs-only-l %s" % pkgs, "r")
glib_libs = map(lambda x:x[2:], string.split(pc.readline()))
pc.close()

pc = os.popen("pkg-config --libs-only-L %s" % pkgs, "r")
glib_libdirs = map(lambda x:x[2:], string.split(pc.readline()))
pc.close()

module_gnoetics = Extension("xxx_gnoetics",
                            sources=["pyutil.c",
                                     "phoneme.c",
                                     "meter.c",
                                     "dictionary.c",
                                     "syllable.c",
                                     "rhyme.c",
                                     "token.c",
                                     "tokenfilter.c",
                                     "text.c",
                                     "seqmodel.c",
                                     "tokenmodel.c",
                                     "gnoetics.c",
                                     ],
                            extra_compile_args=["-Wall"],
                            include_dirs=glib_includes,
                            libraries=glib_libs,
                            library_dirs=glib_libdirs)

setup (name="engine",
       version="0.1",
       description="Gnoetics Engine",
       ext_modules=[module_gnoetics])
