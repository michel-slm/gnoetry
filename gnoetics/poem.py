# This is -*- Python -*-

import string, time
import gobject

import gnoetics
from poeticunit import *

class Poem(gobject.GObject):

    def __sanity_check(self):
        assert len(self.__units) > 0
        assert self.__units[0].is_head()
        assert self.__units[0].is_beginning_of_line()
        assert self.__units[0].is_beginning_of_stanza()
        assert self.__units[-1].is_tail()
        assert self.__units[-1].is_end_of_line()
        assert self.__units[-1].is_end_of_stanza()
        

    def __init__(self, form_name, units=None, magic=None):
        gobject.GObject.__init__(self)
        if magic:
            assert units is None
            units = _unit_magic(magic)
        else:
            units = list(units)
        assert units
        self.__form_name = form_name
        self.__units = units

        self.__seqno = 0

        self.__freeze_changed_count = 0
        self.__freeze_changed_pending = False
        
        self.__sanity_check()

        
    def copy(self, clear=False):
        p = Poem(self.get_form_name(),
                 map(lambda x: x.copy(drop_binding=clear),
                     self.__units))
        if clear:
            p.combine_units()

        p.__seqno = self.__seqno

        return p

    def get_seqno(self):
        return self.__seqno

    def get_form_name(self):
        return self.__form_name

    def freeze_changed(self):
        assert self.__freeze_changed_count >= 0
        self.__freeze_changed_count += 1

    def thaw_changed(self):
        assert self.__freeze_changed_count > 0
        self.__freeze_changed_count -= 1
        if self.__freeze_changed_count == 0 and self.__freeze_changed_pending:
            self.__freeze_changed_pending = False
            self.emit_changed()

    def emit_changed(self):
        if self.__freeze_changed_count == 0:
            self.emit("changed")
        else:
            self.__freeze_changed_pending = True

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

    def is_not_fully_bound(self):
        return not self.is_fully_bound()


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
        if did_work:
            self.__seqno += 1

        self.__sanity_check()
        
        return did_work


    def bind(self, i, tok, is_left=None):
        i = self.__fix_index(i)
        assert self.__good_index(i)
        assert is_left is not None
        assert not tok.is_wildcard()

        orig = self.__units[i]
        assert orig.is_not_bound()

        if not (0 <= tok.get_syllables() <= orig.get_syllables()):
            print i, (is_left and "left") or "right"
            print tok
            print tok.get_syllables()
            print orig
            print orig.get_syllables()
            print
            for i, u in enumerate(self.__units):
                print i, u
                
                                    
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

        self.__seqno += 1

        self.__sanity_check()
            
        self.emit_changed()
        return bind_at


    def bind_left(self, i, tok):
        self.bind(i, tok, is_left=True)


    def bind_right(self, i, tok):
        self.bind(i, tok, is_left=False)

    
    def bind_mandatory_breaks(self):
        i = 0
        while i < len(self.__units):
            u = self.__units[i]
            if u.is_not_bound() and u.is_head():
                self.bind_left(i, gnoetics.token_lookup_break())
                i += 1
            elif u.is_not_bound() and u.is_tail():
                self.bind_right(i, gnoetics.token_lookup_break())
            else:
                i += 1
                

    def unbind(self, i):
        i = self.__fix_index(i)
        assert self.__good_index(i)

        u = self.__units[i]
        assert u.is_bound()

        u.unbind()
        self.combine_units() # FIXME: could be optimized

        self.__seqno += 1

        self.__sanity_check()

        self.emit_changed()


    def clear(self):
        did_work = False
        for u in self.__units:
            if u.is_bound():
                u.unbind()
                did_work = True
        if did_work:
            self.combine_units()
            self.__seqno += 1
            self.emit_changed()


    def get_flag(self, i):
        i = self.__fix_index(i)
        assert self.__good_index(i)

        u = self.__units[i]
        return u.get_flag()
    

    def set_flag(self, i, x):
        i = self.__fix_index(i)
        assert self.__good_index(i)

        u = self.__units[i]

        if x != u.get_flag():
            u.set_flag(x)
            self.emit("changed_flag", i)


    def contains_flagged(self):
        for u in self.__units:
            if u.get_flag():
                return True
        return False


    def unbind_flagged(self):
        self.freeze_changed()

        # First, walk through and unflag zero-syllable "singletons"
        # (i.e. punctuation w/o flagged adjacent units and breaks)
        for i, u in enumerate(self.__units):
            if not u.get_flag():
                continue
            
            if u.is_break():
                u.set_flag(False)
            elif u.get_syllables() == 0:
                prev_flag = i > 0 and self.__units[i-1].get_flag() and self.__units[i-1].get_syllables() > 0
                next_flag = i < len(self.__units)-1 and self.__units[i+1].get_flag() and self.__units[i+1].get_syllables() > 0
                if not (next_flag or prev_flag):
                    u.set_flag(False)
                    self.emit("changed_flag", i)

        # Now actually unbind the flagged units.  If a bit of punctuation is
        # flagged, also unbind any breaks that follow.
        i = 0
        while i < len(self.__units):
            u = self.__units[i]
            if u.get_flag() and u.is_bound():
                if u.is_punctuation() and i+1 < len(self.__units):
                    u2 = self.__units[i+1]
                    if u2.is_break():
                        self.unbind(i+1)
                self.unbind(i)
            else:
                i += 1

        # Next, walk through and pop off any remaining unbound
        # zero-length units (since we know they are next to non-zero units,
        # so everything will work out OK in the end.
        i = 0
        while i < len(self.__units):
            u = self.__units[i]
            if u.get_flag() and u.is_not_bound() and u.get_syllables() == 0:
                self.__units.pop(i)
                self.emit_changed()
            else:
                i += 1

        self.thaw_changed()


    def get_bound_rhyme(self, key):
        if key is None:
            return None
        for u in self.__units:
            if u.get_rhyme() == key and u.is_bound():
                return u.get_binding()
        return None


    def extract_surrounding_tokens(self, i):

        assert 0 <= i < len(self.__units)

        u = self.__units[i]
        assert u.is_not_bound()

        leading_tokens = []
        trailing_tokens = []

        j = i-1
        while 0 <= j:
            u = self.__units[j]
            if u.is_not_bound():
                break
            tok = u.get_binding()
            leading_tokens.insert(0, tok)
            if tok.is_break():
                break
            j -= 1

        j = i+1
        while j < len(self.__units):
            u = self.__units[j]
            if u.is_not_bound():
                break
            tok = u.get_binding()
            trailing_tokens.append(tok)
            if tok.is_break():
                break
            j += 1

        return leading_tokens, trailing_tokens


    def to_list_with_breaks(self):
        L = []
        for u in self.__units:
            if u.is_bound():
                L.append(u.get_binding())
            else:
                L.append(u)
            if u.is_end_of_stanza():
                L.append("end of stanza")
            elif u.is_end_of_line():
                L.append("end of line")

        i = 0
        while i < len(L)-1:
            a = L[i]
            b = L[i+1]
            swap = False

            if a in ("end of stanza", "end of line"):
                if type(b) == gnoetics.Token and \
                   (b.is_punctuation() or b.is_break()):
                    swap = True

            if swap:
                L[i:i+2] = L[i+1], L[i]
                i -= 1
            else:
                i += 1

        return L


    def to_string(self,
                  add_timestamp=False,
                  add_latex_markup=False,
                  add_latex_wrapper=False):
        lines = []
        current_line = ""

        stanza_break = ""
        if add_latex_markup:
            stanza_break = "\\vskip 1ex plus 0.4ex minus 0.4ex"

        if add_latex_wrapper:
            lines.append("\\documentclass[12pt]{article}")
            lines.append("\\begin{document}")
            lines.append("\\pagestyle{empty}")

        if add_latex_markup:
            lines.append("{\\obeylines\\parindent=0in")

        if add_timestamp:
            time_str = time.asctime(time.localtime())
            if add_latex_markup:
                time_str = "{\\it " + time_str + "}"
            lines.append(time_str)
            lines.append(stanza_break)

        last_was_break = False

        for x in self.to_list_with_breaks():
            if x == "end of line":
                lines.append(current_line)
                current_line = ""
            elif x == "end of stanza":
                lines.append(current_line)
                lines.append(stanza_break)
                current_line = ""
            elif type(x) == gnoetics.Token:
                if x.is_break():
                    last_was_break = True
                else:
                    word = x.get_word()

                    if last_was_break:
                        word = word[0].upper() + word[1:]

                    if word == "i":
                        word = "I"

                    # extra space at beginning of sentence
                    if current_line and last_was_break:
                        current_line += " "
                        
                    if current_line and not x.has_left_glue():
                        current_line += " "

                    current_line += word

                    if not x.is_punctuation():
                        last_was_break = False

            else: # is a unit
                if current_line:
                    current_line += " "
                current_line += "_" * x.get_syllables()

        if current_line:
            lines.append(current_line)

        if add_latex_markup:
            lines.append("}")

        if add_latex_wrapper:
            lines.append("\\end{document}")

        def quote_latex_special_chars(line):
            line = line.replace("%", "\%")
            line = line.replace("&", "\&")
            line = line.replace("$", "\$")
            line = line.replace("#", "\#")
            return line

        if add_latex_markup:
            lines = map(quote_latex_special_chars, lines)

        return string.join(lines, "\n")



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

gobject.type_register(Poem)

gobject.signal_new("changed",
                   Poem,
                   gobject.SIGNAL_RUN_LAST,
                   gobject.TYPE_NONE,
                   ())

gobject.signal_new("changed_flag",
                   Poem,
                   gobject.SIGNAL_RUN_LAST,
                   gobject.TYPE_NONE,
                   (gobject.TYPE_INT,))

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

class BlankVerse(Poem):
    def __init__(self, stanzas, lines_per_stanza):
        magic = string.join(("*"*lines_per_stanza,)*stanzas, " ")
        Poem.__init__(self,
                      "Blank Verse (%d/%d)" % (stanzas, lines_per_stanza),
                      magic=magic)



