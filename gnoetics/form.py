import string

class Syllable:

    def __init__(self,
                 meter="*",
                 rhyme_key=None,
                 word_terminal=0,
                 sentence_terminal=0,
                 line_terminal=0,
                 stanza_terminal=0):
        self.__meter             = meter
        self.__rhyme_key         = rhyme_key
        self.__word_terminal     = word_terminal
        self.__sentence_terminal = sentence_terminal
        self.__line_terminal     = line_terminal
        self.__stanza_terminal   = stanza_terminal

    def get_meter(self):
        return self.__meter

    def get_rhyme_key(self):
        return self.__rhyme_key

    def is_word_terminal(self):
        return self.__word_terminal \
               or self.__line_terminal \
               or self.__stanza_terminal

    def is_sentence_terminal(self):
        return self.__sentence_terminal 

    def is_line_terminal(self):
        return self.__line_terminal \
               or self.__stanza_terminal

    def is_stanza_terminal(self):
        return self.__stanza_terminal

    def to_string(self):
        return "[%s%s]" % (self.get_meter(),
                           self.get_rhyme_key() or "")


class SyllableBasedForm:

    def __init__(self):
        self.__syls = []

    def add_syllable(self, c):
        self.__syls.append(c)

    def __len__(self):
        return len(self.__syls)

    def __getitem__(self, i):
        return self.__syls[i]

    def __getslice__(self, i, j):
        return self.__syls[i:j]

    def to_string(self):
        lines = []
        this_line = []

        for c in self.__syls:

            this_line.append(c.to_string())

            if c.is_line_terminal():
                lines.append(string.join(this_line, " "))
                this_line = []
            if c.is_stanza_terminal():
                lines.append("")

        return string.join(lines, "\n")

###############################################################################

class FixedMeterForm(SyllableBasedForm):

    def __init__(self, *meter_lines):
        SyllableBasedForm.__init__(self)

        while meter_lines:
            line = meter_lines[0]
            meter_lines = meter_lines[1:]
            if line:
                eostza = (not meter_lines) or (not meter_lines[0])
                for j in range(len(line)):
                    m = line[j]
                    eol = (j == len(line)-1)
                    s = Syllable(meter=m,
                                 line_terminal=eol,
                                 sentence_terminal=eol and eostza \
                                 and not meter_lines,
                                 stanza_terminal=eol and eostza)
                    self.add_syllable(s)


class FixedSyllableForm(FixedMeterForm):

    def __init__(self, *syllables):
        meter = map(lambda n: "*"*n, syllables)
        FixedMeterForm.__init__(self, *meter)


class Haiku(FixedSyllableForm):

    def __init__(self):
        FixedSyllableForm.__init__(self, 5, 7, 5)


class Tanka(FixedSyllableForm):

    def __init__(self):
        FixedSyllableForm.__init__(self, 5, 7, 5, 0, 7, 7)


class Renga(FixedSyllableForm):

    def __init__(self):
        FixedSyllableForm.__init__(self, 5, 7, 5, 0, 7, 7, 0, 5, 7, 5)


class Sapphic(FixedMeterForm):

    def __init__(self):

        stanza = ("-u-u-uu-u-u",) * 3 + ("-uu-u", "")
        lines = []
        for i in range(5):
            lines.extend(stanza)

        FixedMeterForm.__init__(self, *lines)


class Hendecasyllabic(FixedMeterForm):

    def __init__(self):
        lines = ("---uu-u-u--", ) * 13
        FixedMeterForm.__init__(self, *lines)
        

###############################################################################

def _build_iambic_pentameter(rhyme_tag=None, count=4, blank_verse=0):
    cells = []
    rhyme_keys = ["A", "B"] * (count/2+1)
    for i in range(count):
        meter = "u-" * 5
        while meter:
            m = meter[0]
            meter = meter[1:]
            
            key = None
            if meter == "" and not blank_verse:
                key = "%s%s" % (rhyme_keys.pop(0), rhyme_tag or "")
                
            c = Syllable(meter=m,
                         rhyme_key=key,
                         line_terminal=(meter == ""),
                         sentence_terminal=(i == count-1 and meter == ""),
                         stanza_terminal=(i == count-1 and meter == ""))
            cells.append(c)

    return cells

class SonnetStanza(SyllableBasedForm):

    def __init__(self):
        SyllableBasedForm.__init__(self)

        for c in _build_iambic_pentameter():
            self.add_syllable(c)


class BlankVerseStanza(SyllableBasedForm):
    def __init__(self):
        SyllableBasedForm.__init__(self)

        for c in _build_iambic_pentameter(blank_verse=1):
            self.add_syllable(c)


class HeroicCouplet(SyllableBasedForm):

    def __init__(self):
        SyllableBasedForm.__init__(self)

        for c in _build_iambic_pentameter(count=2):
            self.add_syllable(c)


class Sonnet(SyllableBasedForm):

    def __init__(self):
        SyllableBasedForm.__init__(self)

        for i in range(1, 4):
            for c in _build_iambic_pentameter(str(i)):
                self.add_syllable(c)

        # Heroic couplet
        for c in _build_iambic_pentameter("4", count=2):
            self.add_syllable(c)


