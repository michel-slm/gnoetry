# This is -*- Python -*-

import gnoetics

class PoeticUnit:

    __next_id = 1


    def __init__(self, **args):
        if args.has_key("xml"):
            self.__from_xml(args["xml"])
        else:
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


                   id=None,
                   flag=False,

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

        if id is None:
            id = PoeticUnit.__next_id
            PoeticUnit.__next_id += 1
        self.__id = id

        # Initialize binding information

        self.__token = None

        # Initialize flagging

        self.__flag = flag
        

    def copy(self, drop_binding=False):
        u = PoeticUnit(syllables=self.get_syllables(),
                       meter=self.get_meter(),
                       rhyme=self.get_rhyme(),
                       is_beginning_of_line=self.is_beginning_of_line(),
                       is_beginning_of_stanza=self.is_beginning_of_stanza(),
                       is_end_of_line=self.is_end_of_line(),
                       is_end_of_stanza=self.is_end_of_stanza(),
                       is_head=self.is_head(),
                       is_tail=self.is_tail(),
                       id=self.get_id(),
                       flag=self.get_flag())

        if self.is_bound() and not drop_binding:
            u.bind(self.get_binding())

        return u
    
    
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


    def get_id(self):
        return self.__id


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
    ### Flagging
    ###

    def get_flag(self):
        return self.__flag

    def set_flag(self, x):
        self.__flag = x


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

        L["id"] = self.get_id()
        R["id"] = self.get_id()

        L["flag"] = self.get_flag()
        R["flag"] = self.get_flag()

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

        assert self.get_id() == right.get_id()

        J = {}

        J["syllables"]     = self.get_syllables() + right.get_syllables()
        J["meter"]         = self.get_meter() + right.get_meter()

        if right.get_syllables() > 0:
            J["rhyme"] = right.get_rhyme()
        else:
            J["rhyme"] = self.get_rhyme()

        J["is_beginning_of_line"]   = self.is_beginning_of_line()
        J["is_beginning_of_stanza"] = self.is_beginning_of_stanza()

        J["is_end_of_line"]   = right.is_end_of_line()
        J["is_end_of_stanza"] = right.is_end_of_stanza()

        J["is_head"]  = self.is_head()
        J["is_tail"]  = right.is_tail()

        J["id"] = self.get_id()
        J["flag"] = self.get_flag() or right.get_flag()

        return J


    def append(self, right):
        J = self.__join(right)
        self.__populate(**J)


    def prepend(self, left):
        J = left.__join(self)
        self.__populate(**J)


    def to_xml(self, doc):
        node = doc.createElement("unit")

        node.setAttribute("syllables", "%d" % self.get_syllables())
        node.setAttribute("meter", self.get_meter())

        if self.is_rhymed():
            node.setAttribute("rhyme", str(self.get_rhyme()))

        if self.is_beginning_of_line():
            node.setAttribute("is_beginning_of_line", "1")

        if self.is_beginning_of_stanza():
            node.setAttribute("is_beginning_of_stanza", "1")

        if self.is_end_of_line():
            node.setAttribute("is_end_of_line", "1")

        if self.is_end_of_stanza():
            node.setAttribute("is_end_of_stanza", "1")

        node.setAttribute("id", str(self.get_id()))

        if self.is_head():
            node.setAttribute("is_head", "1")

        if self.is_tail():
            node.setAttribute("is_tail", "1")

        if self.is_bound():

            node.setAttribute("is_bound", "1")
            
            if self.is_break():
                node.setAttribute("binding_is_break", "1")
            else:
                node.setAttribute("binding", self.get_binding().get_word())

            if self.is_punctuation():
                node.setAttribute("binding_is_punctuation", "1")


            if self.has_left_glue():
                node.setAttribute("has_left_glue", "1")

            if self.has_right_glue():
                node.setAttribute("has_right_glue", "1")

        return node


    def __from_xml(self, node):

        assert node.localName == "unit"

        P = {}

        P["syllables"] = int(node.getAttribute("syllables"))
        P["meter"]     = node.getAttribute("meter")
        
        x = node.getAttribute("rhyme")
        if x:
            P["rhyme"] = x

        P["is_beginning_of_line"]   = bool(node.getAttribute("is_beginning_of_line"))
        P["is_beginning_of_stanza"] = bool(node.getAttribute("is_beginning_of_stanza"))
        P["is_end_of_line"]         = bool(node.getAttribute("is_end_of_line"))
        P["is_end_of_stanza"]       = bool(node.getAttribute("is_end_of_stanza"))

        P["is_head"] = bool(node.getAttribute("is_head"))
        P["is_tail"] = bool(node.getAttribute("is_tail"))

        P["id"] = int(node.getAttribute("id"))

        self.__populate(**P)

        if node.getAttribute("is_bound"):
            tok = None
            if node.getAttribute("binding_is_break"):
                tok = gnoetics.token_lookup_break()
            else:
                x = node.getAttribute("binding")
                if node.getAttribute("binding_is_punctuation"):
                    x = "*punct* %s" % x
                tok = gnoetics.token_lookup(x)

            assert tok.has_left_glue() == bool(node.getAttribute("has_left_glue"))
            assert tok.has_right_glue() == bool(node.getAttribute("has_right_glue"))

            if tok:
                self.bind(tok)
            
