# This is -*- Python -*-

import os, sys
from xxx_gnoetics import *

class Library:

    def __init__(self, dir=None):
        self.__texts = []
        if dir:
            self.add_from_directory(dir)

    def add(self, filename):
        txt = Text(filename)
        self.__texts.append(txt)

        def sort_fn(a, b):
            def canon(x):
                x = x.get_title().lower().strip()
                if x[:4] == "the ":
                    x = x[4:]
                if x[:2] == "a ":
                    x = x[2:]
                return x
            return cmp(canon(a), canon(b))
        self.__texts.sort(sort_fn)

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
        return list(self.__texts)

    def __iter__(self):
        return iter(self.__texts)
