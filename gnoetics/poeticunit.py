# This is -*- Python -*-

import gnoetics

class PoeticUnit:


    def __init__(self, **args):
        self.__populate(**args)


    def __populate(self,
                   syllables=None,
                   meter=None,
                   rhyme=None,

                   is_beginning_of_line=False,   # Is this a line's beginning?
                   is_beginning_of_stanza=False, # ...or a stanza's?
                   
                   is_end_of_line=False,    # Is this the end of a line?
                   is_end_of_stanza=False,  # ...or a stanza?
                   
                   is_head=False,
                   is_tail=False,

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

        # beginning of stanza implies beginning of line
        if is_beginning_of_stanza:
            is_beginning_of_line = True

        # end of stanza implies end of line
        if is_end_of_stanza:
            is_end_of_line = True

        # Populate variables
            
        self.__syllables = syllables
        self.__meter = meter
        self.__rhyme = rhyme

        self.__bol = is_beginning_of_line
        self.__bostz = is_beginning_of_stanza

        self.__eol = is_end_of_line
        self.__eostz = is_end_of_stanza

        self.__ishead = is_head
        self.__istail = is_tail

        # Initialize binding information

        self.__token = None

    ###
    ### Accessors
    ###

    def get_syllables(self):
        return self.__syllables


    def get_meter(self):
        return self.__meter


    def is_rhymed(self):
        return self.__rhyme is not None


    def get_rhyme(self):
        return self.__rhyme


    def is_beginning_of_line(self):
        return self.__bol


    def is_beginning_of_stanza(self):
        return self.__bostz


    def is_end_of_line(self):
        return self.__eol


    def is_end_of_stanza(self):
        return self.__eostz


    def is_head(self):
        return self.__ishead


    def is_tail(self):
        return self.__istail


    def is_left_constrained(self):
        return self.is_beginning_of_line() \
               or self.is_beginning_of_stanza() \
               or self.is_head()
    

    def is_right_constrained(self):
        return self.is_rhymed() \
               or self.is_end_of_line() \
               or self.is_end_of_stanza() \
               or self.is_tail()

    ###
    ### Binding
    ###


    def is_bound(self):
        return self.__token is not None


    def is_not_bound(self):
        return self.__token is None


    def get_binding(self):
        return self.__token


    def is_break(self):
        return self.is_bound() and self.get_binding().is_break()


    def is_punctuation(self):
        return self.is_bound() and self.get_binding().is_punctuation()


    def has_left_glue(self):
        return self.is_bound() and self.get_binding().has_left_glue()


    def has_right_glue(self):
        return self.is_bound() and self.get_binding().has_right_glue()


    def bind(self, token):
        assert self.__token is None
        assert token.get_syllables() == self.get_syllables()
        if self.is_head() or self.is_tail():
            assert token.is_break()
        self.__token = token


    def unbind(self):
        assert self.__token is not None
        self.__token = None


    ###
    ### Stringification
    ###

    def to_string(self,
                  show_line_break_info=False,
                  highlight=False):
        if self.is_break():
            s = "<x>"
        elif self.is_bound():
            s = self.get_binding().get_word()
        else:
            s = self.get_meter()

        if self.is_rhymed():
            s += "(%s)" % self.get_rhyme()

        if self.is_head():
            s = "[[" + s
        if self.is_tail():
            s = s + "]]"
            
        if show_line_break_info:
            if self.is_beginning_of_line():
                s = "<line>" + s
            if self.is_beginning_of_stanza():
                s = "<stanza>" + s
            if self.is_end_of_line():
                s += "</line>"
            if self.is_end_of_stanza():
                s += "</stanza>"

        if highlight:
            s = " ((( %s ))) " % s
        return s


    def __str__(self):
        return self.to_string()

    def __repr__(self):
        return "<PoeticUnit: %s>" % self.to_string()

    ###
    ### Subdivision
    ###

    def __split(self, n):
        assert self.is_not_bound()
        assert 0 <= n <= self.get_syllables()

        L = {}
        R = {}

        L["syllables"] = n
        R["syllables"] = self.get_syllables() - n
        
        L["meter"] = self.get_meter()[:n]
        R["meter"] = self.get_meter()[n:]

        # The rhyme should never end up attached to a
        # zero-syllable unit.
        if n < self.get_syllables():
            R["rhyme"] = self.get_rhyme()
        else:
            L["rhyme"] = self.get_rhyme()

        L["is_beginning_of_line"] = self.is_beginning_of_line()
        L["is_beginning_of_stanza"] = self.is_beginning_of_stanza()

        R["is_end_of_line"] = self.is_end_of_line()
        R["is_end_of_stanza"] = self.is_end_of_stanza()

        L["is_head"] = self.is_head()
        R["is_tail"] = self.is_tail()

        return L, R


    def pop_left(self, n):
        assert n < self.get_syllables()
        L, R = self.__split(n)
        self.__populate(**R)
        return PoeticUnit(**L)


    def pop_right(self, n):
        assert n < self.get_syllables()
        L, R = self.__split(self.get_syllables() - n)
        self.__populate(**L)
        return PoeticUnit(**R)

    ###
    ### Recombination
    ###

    def can_combine(self, right): # self = left
        return self.is_not_bound() \
               and right.is_not_bound() \
               and not self.is_right_constrained() \
               and not right.is_left_constrained()


    def __join(self, right): # self = left
        assert self.is_not_bound()
        assert right.is_not_bound()
        assert not self.is_right_constrained()
        assert not right.is_left_constrained()
        # For debugging purposes, we do the above checks separately.
        assert self.can_combine(right)

        J = {}

        J["syllables"]     = self.get_syllables() + right.get_syllables()
        J["meter"]         = self.get_meter() + right.get_meter()

        J["rhyme"]         = right.get_rhyme()

        J["is_beginning_of_line"]   = self.is_beginning_of_line()
        J["is_beginning_of_stanza"] = self.is_beginning_of_stanza()

        J["is_end_of_line"]   = right.is_end_of_line()
        J["is_end_of_stanza"] = right.is_end_of_stanza()

        J["is_head"]  = self.is_head()
        J["is_tail"]  = right.is_tail()

        return J


    def append(self, right):
        J = self.__join(right)
        self.__populate(**J)


    def prepend(self, left):
        J = left.__join(self)
        self.__populate(**J)

