# This is -*- Python -*-

import gnoetics

class PoeticUnit:


    def __init__(self, **args):
        self.__populate(**args)


    def __populate(self,
                   syllables=None,
                   meter=None,
                   rhyme=None,
                   
                   end_of_line=False,      # Is this the end of a line?
                   end_of_stanza=False,    # ...or a stanza?
                   
                   must_be_head=False,
                   must_be_tail=False,
                   
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
            end_of_line = True

        # Populate variables
            
        self.__syllables = syllables
        self.__meter = meter
        self.__rhyme = rhyme

        self.__eol = end_of_line
        self.__eostz = end_of_stanza

        self.__musthead = must_be_head
        self.__musttail = must_be_tail

        # Initialize binding information

        self.__token = None
        self.__ishead = False
        self.__istail = False

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


    def is_end_of_line(self):
        return self.__eol


    def is_end_of_stanza(self):
        return self.__eostz


    def must_be_head(self):
        return self.__musthead


    def must_be_tail(self):
        return self.__musttail


    def is_head(self):
        return self.__musthead or self.__ishead


    def is_tail(self):
        return self.__musttail or self.__istail


    def is_left_constrained(self):
        return self.is_head()
    

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


    def bind(self, token):
        assert self.__token is None
        if token.is_break():
            assert self.get_syllables() == 0
        else:
            assert token.get_syllables() == self.get_syllables()
        self.__token = token


    def set_head(self):
        assert self.is_bound()
        self.__ishead = True


    def set_tail(self):
        assert self.is_bound()
        self.__istail = True


    def unbind(self):
        assert self.__token is not None
        self.__token = None
        self.__ishead = False
        self.__istail = False

    ###
    ### Tokenification
    ###

    def get_tokens(self):
        T = []
        if self.is_head():
            T.append(gnoetics.token_lookup_break())
        if self.is_bound():
            T.append(s.get_binding())
        else:
            T.append(gnoetics.token_lookup_wildcard())
        if self.is_tail():
            T.append(gnoetics.token_lookup_break())

    ###
    ### Stringification
    ###

    def to_string(self,
                  show_line_break_info=True,
                  highlight=False):
        if self.is_bound():
            s = self.get_binding().get_word()
        else:
            s = self.get_meter()

        if self.is_rhymed():
            s += "(%s)" % self.get_rhyme()
        if show_line_break_info:
            if self.is_end_of_stanza():
                s += "<stz>"
            elif self.is_end_of_line():
                s += "<ln>"

        if self.must_be_head():
            s = "[[[" + s
        elif self.is_head():
            s = "[[" + s
        if self.must_be_tail():
            s = s + "]]]"
        elif self.is_tail():
            s = s + "]]"

        if highlight:
            s = " ((( %s ))) " % s
        return s


    def __str__(self):
        return self.to_string()

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

        R["rhyme"] = self.get_rhyme()

        R["end_of_line"] = self.is_end_of_line()
        R["end_of_stanza"] = self.is_end_of_stanza()

        L["must_be_head"] = self.must_be_head()
        R["must_be_tail"] = self.must_be_tail()

        return L, R


    def pop_left(self, n):
        L, R = self.__split(n)
        self.__populate(**R)
        return PoeticUnit(**L)


    def pop_right(self, n):
        L, R = self.__split(n)
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

        J["end_of_line"]   = right.is_end_of_line()
        J["end_of_stanza"] = right.is_end_of_stanza()

        J["must_be_head"]  = self.must_be_head()
        J["must_be_tail"]  = right.must_be_tail()

        return J


    def append(self, right):
        J = self.__join(right)
        self.__populate(J)


    def prepend(self, left):
        J = left.__join(self)
        self.__populate(J)

