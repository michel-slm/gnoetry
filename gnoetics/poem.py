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
        assert self.__units[0].is_head()
        assert self.__units[0].is_beginning_of_line()
        assert self.__units[0].is_beginning_of_stanza()
        assert self.__units[-1].is_tail()
        assert self.__units[-1].is_end_of_line()
        assert self.__units[-1].is_end_of_stanza()


    def form_name(self):
        return self.__form_name


    def __good_index(self, i):
        return 0 <= i < len(self.__units)


    def __fix_index(self, i):
        if i < 0:
            i += len(self.__units)
        return i


    def __len__(self):
        return len(self.__units)


    def __getitem__(self, i):
        i = self.__fix_index(i)
        assert self.__good_index(i)
        return self.__units[i]


    def is_fully_bound(self):
        for x in self.__units:
            if x.is_not_bound():
                return False
        return True


    def find_first_unbound(self):
        for i, x in enumerate(self.__units):
            if x.is_not_bound():
                return i
        return None


    def combine_units(self):
        i = 0
        did_work = False
        while i < len(self.__units)-1:
            a = self.__units[i]
            b = self.__units[i+1]
            if a.can_combine(b):
                a.append(b)
                self.__units.pop(i+1)
                did_work = True
            else:
                i += 1
        return did_work


    def bind(self, i, tok, is_left=None):
        i = self.__fix_index(i)
        assert self.__good_index(i)
        assert is_left is not None
        assert not tok.is_wildcard()

        orig = self.__units[i]
        assert orig.is_not_bound()
        assert 0 <= tok.get_syllables() <= orig.get_syllables()

        if tok.get_syllables() == orig.get_syllables():
            site = orig
            repl = None
            bind_at = i
        elif is_left:
            site = orig.pop_left(tok.get_syllables())
            repl = (site, orig)
            bind_at = i
        else:
            site = orig.pop_right(tok.get_syllables())
            repl = (orig, site)
            bind_at = i+1

        site.bind(tok)
        if repl:
            self.__units[i:i+1] = repl
        return bind_at


    def bind_left(self, i, tok):
        self.bind(i, tok, is_left=True)


    def bind_right(self, i, tok):
        self.bind(i, tok, is_left=False)

    def unbind(self, i):
        i = self.__fix_index(i)
        assert self.__good_index(i)

        u = self.__units[i]
        assert u.is_bound()

        u.unbind()
        self.combine_units() # FIXME: could be optimized


    def to_string(self, highlight=None, show_line_break_info=False):
        out_str = ""
        if highlight is not None:
            highlight = self.__fix_index(highlight)
        last_was_break = False
        suppress_space = False
        for i, u in enumerate(self.__units):

            if not u.is_beginning_of_line() \
               and not u.has_left_glue() \
               and not suppress_space:
                out_str += " "

            if u.is_bound():
                if not u.is_break():
                    s = u.get_binding().get_word()
                    if last_was_break:
                        s = s[0].upper() + s[1:]
                    out_str += s
            else:
                out_str += "<%s>" % u
            
            if u.is_end_of_line():
                out_str += "\n"
            if u.is_end_of_stanza():
                out_str += "\n"

            suppress_space = u.is_beginning_of_line() and u.is_break()
            last_was_break = u.is_break()

        return out_str


##############################################################################

def _unit_magic(scheme):
    scheme = scheme.strip()
    units = []
    for i, x in enumerate(scheme):

        if x == " ":
            continue
        
        is_first = (i == 0)
        is_last = (i == len(scheme)-1)

        args = {}

        if x in "123456789":
            args["syllables"] = int(x)
        else:
            args["iambs"] = 5
            if x != "*":
                args["rhyme"] = x

        args["is_beginning_of_line"] = True
        args["is_end_of_line"] = True
        if is_first:
            args["is_head"] = True
        if is_last:
            args["is_tail"] = True
        if is_first or scheme[i-1] == " ":
            args["is_beginning_of_stanza"] = True
        if is_last or scheme[i+1] == " ":
            args["is_end_of_stanza"] = True

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
