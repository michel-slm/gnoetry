# This is -*- Python -*-

import os, sys
from xxx_gnoetics import *

class Library:

    def __init__(self, dir=None):
        self.__texts = []
        self.__sorted = True
        if dir:
            self.add_from_directory(dir)

    def add(self, filename):
        txt = Text(filename)
        self.__texts.append(txt)
        self.__sorted = False

    def add_from_directory(self, dirname):
        for fn in os.listdir(dirname):
            if os.path.splitext(fn)[1] == ".ts":
                self.add(os.path.join(dirname, fn))

    def get_by_title(self, title):
        for txt in self.__texts:
            if txt.get_title().lower() == title.lower().strip():
                return txt
        return None

    def get_all(self):
        if not self.__sorted:
            self.__texts.sort(lambda x, y: cmp(x.get_sort_title(),
                                               y.get_sort_title()))
            self.__sorted = True
        return list(self.__texts)

    def __iter__(self):
        return iter(self.__texts)
