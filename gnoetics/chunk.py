import gnoetics

import random

def _get_leading(L, n):
    return L[:n]

def _get_trailing(L, n):
    return L[-n:]

class Chunk(object):

    def get_word(self):
        return "<Chunk>"

    def __init__(self,
                 syllables=None,
                 meter=None):

        assert (syllables is not None) or (meter is not None)

        if syllables is None:
            syllables = len(meter)

        self.__syllables = syllables
        self.__meter = meter


    def __repr__(self):
        if self.__meter:
            return "<Chunk: meter=%s>" % self.__meter
        else:
            return "<Chunk: syls=%d>" % self.__syllables


    def drop_leading(self, N):
        if N >= self.__syllables:
            return None
        elif self.__meter:
            return Chunk(meter=self.__meter[N:])
        else:
            return Chunk(syllables=self.__syllables-N)

    def drop_trailing(self, N):
        if N >= self.__syllables:
            return None
        elif self.__meter:
            return Chunk(meter=self.__meter[:-N])
        else:
            return Chunk(syllables=self.__syllables-N)

    ###
    ### Notation:
    ### (x) = break
    ###  T  = token
    ###  c  = chunk

    # No before or after
    def __solve_free(self, model):

        kwargs = {}
        if self.__meter:
            kwargs["meter_left"] = self.__meter
        else:
            kwargs["max_syllables"] = self.__syllables

        return model.solve([], **kwargs) # empty tuple == filter only

        
    def __solve_leading(self, model, before):

        kwargs = {}
        if self.__meter:
            kwargs["meter_left"] = self.__meter
        else:
            kwargs["max_syllables"] = self.__syllables

        N = model.get_N()
        query = _get_trailing(before, N-1) + [gnoetics.token_lookup_wildcard()]
        print query
        return model.solve(query, **kwargs)


    def __solve_trailing(self, model, after):

        kwargs = {}
        if self.__meter:
            kwargs["meter_right"] = self.__meter
        else:
            kwargs["max_syllables"] = self.__syllables

        N = model.get_N()
        query = [gnoetics.token_lookup_wildcard()] + _get_leading(after, N-1)
        print query
        return model.solve(query, **kwargs)


    def __solve_pinch(self, model, before, after):

        soln_left = []
        soln_right = []

        N = model.get_N()
        wild = gnoetics.token_lookup_wildcard()

        # Find left solutions
        if self.__syllables > 1:

            kwargs = {}
            if self.__meter:
                kwargs["meter_left"] = self.__meter[:-1]
            else:
                kwargs["max_syllables"] = self.__syllables-1
            query = _get_trailing(before, N-1) + [wild]
            soln_left = model.solve(query, **kwargs)

            kwargs = {}
            if self.__meter:
                kwargs["meter_right"] = self.__meter[1:]
            else:
                kwargs["max_syllables"] = self.__syllables-1
            query = [wild] + _get_leading(after, N-1)
            soln_right = model.solve(query, **kwargs)


        kwargs = {}
        if self.__meter:
            kwargs["meter_left"] = self.__meter
        kwargs["min_syllables"] = self.__syllables
        kwargs["max_syllables"] = self.__syllables
        for i in range(1, N-1):
            query = _get_trailing(before, i) + [wild] + _get_leading(after, N-1-i)
            soln = model.solve(query, **kwargs)
            soln_left.extend(soln)

        return (soln_left, soln_right)

                
    def solve(self, model, before, after):

        if before and after:
            return self.__solve_pinch(model, before, after)
        elif before:
            return (self.__solve_leading(model, before), [])
        elif after:
            return ([], self.__solve_trailing(model, after))
        else:
            return (self.__solve_free(model), [])


    def evolve(self, model, before, after):

        soln_left, soln_right = self.solve(model, before, after)

        if not (soln_left or soln_right):
            return None

        def contains_break(L):
            for i in xrange(len(L)):
                if L[i].is_break():
                    return 1
            return 0

        i = random.randrange(len(soln_left) + len(soln_right))

        if i < len(soln_left):
            t = soln_left[i]
            c = self.__drop_leading(t.get_syllables())
            if c:
                return (t, c)
            else:
                return (t,)
        else:
            t = soln_right[i - len(soln_left)]
            c = self.__drop_trailing(t.get_syllables())
            if c:
                return (c, t)
            else:
                return (t,)

        
