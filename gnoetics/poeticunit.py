# This is -*- Python -*-

import gnoetics

class PoeticUnit:

    def __init__(self,
                 syllables=None,
                 meter=None,
                 rhyme=None,
                 
                 end_of_line=0,      # Is this the end of a line?
                 end_of_stanza=0,    # ...or a stanza?
                 end_of_sentence=0,  # ...or a sentence?
                 
                 iambs=None):       # Convenience: how many iambs?

        ### Sanity check arguments

        if syllables is not None:
            assert syllables >= 0

        if meter is not None:
            assert gnoetics.meter_is_valid(meter)

        if iambs is not None:
            assert meter is None
            assert iambs > 0
            meter = (gnoetics.METER_UNSTRESSED+gnoetics.METER_STRESSED)*iambs

        assert (syllables is not None) or (meter is not None)


        ### Rationalize arguments

        if syllables is not None and meter is not None:
            assert len(meter) == syllables
        elif syllables is None and meter is not None:
            syllables = len(meter)
        elif syllables is not None and meter is None:
            meter = gnoetics.METER_ANY * syllables

        # end of stanza implies end of line
        if end_of_stanza:
            end_of_line = 1
            
            
        self.__syllables = syllables
        self.__meter = meter
        self.__rhyme = rhyme

        self.__eol = end_of_line
        self.__eostz = end_of_stanza
        self.__eos = end_of_sentence
        
        self.__token = None


    def is_bound(self):
        return self.__token is not None

    def get_binding(self):
        return self.__token

    def bind(self, token):
        assert self.__token is None
        assert token.get_syllables() == self.get_syllables()
        self.__token = token

    def unbind(self):
        assert self.__token is not None
        self.__token = None

    def get_syllables(self):
        return self.__syllables

    def get_meter(self):
        return self.__meter

    def is_rhymed(self):
        return self.__rhyme is not None

    def get_rhyme(self):
        return self.__rhyme

    def is_end_of_line(self):
        return self.__eol

    def is_end_of_stanza(self):
        return self.__eostz

    def is_end_of_sentence(self):
        return self.__eos

    def is_left_constrained(self):
        return 0
    
    def is_right_constrained(self):
        return self.is_rhymed() \
               or self.is_end_of_line() \
               or self.is_end_of_stanza() \
               or self.is_end_of_sentence()

    def to_string(self, show_line_break_info=1):
        s = self.get_meter()
        if self.is_end_of_sentence():
            s += "."
        if self.is_rhymed():
            s += "[%s]" % self.get_rhyme()
        if show_line_break_info:
            if self.is_end_of_stanza():
                s += "<stz>"
            elif self.is_end_of_line():
                s += "<ln>"
        return s

    def __str__(self):
        return self.to_string()

    def split(self, n, right=0):
        assert n >= 0
        assert n <= self.get_syllables()
        if right:
             n = self.get_syllables() - n

        mtr = self.get_meter()
        x = PoeticUnit(meter=mtr[:n])
        y = PoeticUnit(meter=mtr[n:],
                       rhyme=self.get_rhyme(),
                       end_of_line=self.is_end_of_line(),
                       end_of_stanza=self.is_end_of_stanza(),
                       end_of_sentence=self.is_end_of_sentence())

        return (x, y)
            
        
def combine_two_units(x, y):
    if x.is_right_constrained() or x.is_bound() \
       or y.is_left_constrained() or y.is_bound():
        return None

    xmtr = x.get_meter()
    ymtr = y.get_meter()
    mtr = xmtr + ymtr
    
    return PoeticUnit(meter=mtr,
                      rhyme=y.get_rhyme(),
                      end_of_line=y.is_end_of_line(),
                      end_of_stanza=y.is_end_of_stanza(),
                      end_of_sentence=y.is_end_of_sentence())

    
