# This is -*- Python -*-

import gnoetics
from poeticunit import *

class Poem:

    def __init__(self, form_name, units=None, magic=None):
        if magic:
            assert units is None
            units = _unit_magic(magic)
        else:
            units = list(units)
        assert units
        self.__form_name = form_name
        self.__units = units

        ### Sanity-check
        assert len(self.__units) > 0
        assert self.__units[-1].is_end_of_sentence()

    def form_name(self):
        return self.__form_name

    def __len__(self):
        return len(self.__units)

    def __getitem__(self, i):
        assert 0 <= i < len(self.__units)
        return self.__units[i]

    def replace(self, i, units):
        assert 0 <= i < len(self.__units)
        self.__units = self.__units[:i] + list(units) + self.__units[i+1:]

    def delete(self, i):
        self.replace(i, [])

    def is_fully_bound(self):
        for x in self.__units:
            if not x.is_bound():
                return 0
        return 1

    def find_first_unbound(self):
        for i in range(len(self.__units)):
            if not self.__units[i].is_bound():
                return i
        return None

    def combine_units(self):
        i = 0
        did_work = 0
        while i < len(self.__units)-1:
            x = self.__units[i]
            if x.get_syllables() == 0 and not x.is_bound():
                self.__units.pop(i)
                did_work = 1
            else:
                c = combine_two_units(self.__units[i],
                                      self.__units[i+1])
                if c:
                    self.__units[i:i+2] = [c]
                    did_work = 1
                else:
                    i += 1
        if did_work:
            pass # FIXME: emit a signal, or something

    def dump(self):
        for x in self.__units:
            s = x.to_string(show_line_break_info=0)
            print s,
            if x.is_end_of_line():
                print
            if x.is_end_of_stanza():
                print


##############################################################################

def _unit_magic(scheme):
    scheme = scheme.strip()
    units = []
    for i in range(len(scheme)):
        is_last = (i == len(scheme)-1)
        x = scheme[i]
        if x == " ":
            continue

        args = {}

        if x in "123456789":
            args["syllables"] = int(x)
        else:
            args["iambs"] = 5
            if x != "*":
                args["rhyme"] = x

        args["end_of_line"] = 1
        if is_last:
            args["end_of_sentence"] = 1
        if is_last or scheme[i+1] == " ":
            args["end_of_stanza"] = 1

        units.append(PoeticUnit(**args))

    return units
        
##############################################################################

class SonnetStanza(Poem):
    def __init__(self):
        Poem.__init__(self, "Sonnet Stanza", magic="ABAB")

class Sonnet(Poem):
    def __init__(self):
        Poem.__init__(self, "Sonnet", magic="ABAB CDCD EFEF GG")

class Haiku(Poem):
    def __init__(self):
        Poem.__init__(self, "Haiku", magic="575")

class Tanka(Poem):
    def __init__(self):
        Poem.__init__(self, "Tanka", magic="575 77")

class Renga(Poem):
    def __init__(self):
        Poem.__init__(self, "Renga", magic="575 77 575")
