# This is -*- Python -*-

import gnoetics
import random


class GenericSolver:


    def __init__(self):
        self.__action_list = []


    # Returns the index of the next unit to solve, along w/ the
    # left/right binding type.
    def next_pos(self, poem):
        assert False


    # Given an index and left/right, return an Action item,
    # or None if no solutions are found.
    def find_action(self, poem, i, is_left=True):
        assert False


    def step(self, poem):
        if poem.is_fully_bound():
            return False
        i, is_left = self.next_pos(poem)
        assert 0 <= i < len(poem)
        assert poem[i].is_not_bound()
        act = self.find_action(poem, i, is_left=is_left)
        if act and act.has_solution():
            act.apply()
            self.__action_list.append(act)
            return +1 # found solution by stepping forward
        found_soln = False
        while self.__action_list and not found_soln:
            act = self.__action_list[-1]
            act.revert()
            if act.has_solution():
                act.apply()
                found_soln = True
            else:
                self.__action_list.pop(-1)
        if found_soln:
            return -1 # found solution by stepping back
        else:
            return 0  # No solutions found
        

    def solve(self, poem):
        while not poem.is_fully_bound() and self.step(poem) != 0:
            pass


############################################################################


# FIXME: needs to be changed to reflect changes to GenericSolver,
# PoeticUnit, Poem, etc.
class SimpleSolver(GenericSolver):

    def __init__(self, model):
        assert model.get_N() >= 3
        self.__model = model

    def solve_at(self, poem, i, right=0):
        u = poem[i]
        assert u.is_not_bound()

        print "i=%d, right=%d" % (i, right)
        
        t_break = gnoetics.token_lookup_break()
        t_wild  = gnoetics.token_lookup_wildcard()

        query1 = []
        filter1 = {}

        query2 = []
        filter2 = {}

        t_prev = None
        if i == 0:
            t_prev = t_break
        elif poem[i-1].is_bound():
            t_prev = poem[i-1].get_binding()

        t_next = None
        if i == len(poem)-1:
            t_next = t_break
        elif poem[i+1].is_bound():
            t_next = poem[i+1].get_binding()

        print "prev:", t_prev
        print "next:", t_next

        if t_prev is None and t_next is None:
            query1 = [t_wild]
            filter1["min_syllables"] = 0
            filter1["max_syllables"] = u.get_syllables()
        elif right:
            query1 = [t_wild, t_next]
            filter1["min_syllables"] = 0
            filter1["max_syllables"] = u.get_syllables()
            if t_next and t_next.get_syllables() == 0:
                filter1["min_syllables"] = 1
            if t_prev:
                filter1["max_syllables"] -= 1
                query2 = [t_prev, t_wild, t_next]
                filter2["min_syllables"] = u.get_syllables()
                filter2["max_syllables"] = u.get_syllables()
        else: # left
            query1 = [t_prev, t_wild]
            filter1["min_syllables"] = 0
            filter1["max_syllables"] = u.get_syllables()
            if t_prev and t_prev.get_syllables() == 0:
                filter1["min_syllables"] = 1
            if t_next:
                filter1["max_syllables"] -= 1
                query2 = [t_prev, t_wild, t_next]
                filter2["min_syllables"] = u.get_syllables()
                filter2["max_syllables"] = u.get_syllables()

        query1 = filter(lambda x: x is not None, query1)
        query2 = filter(lambda x: x is not None, query2)

        soln_raw = []
        if query1:
            print "Q1:", query1, filter1
            soln_raw.extend(self.__model.solve(query1, **filter1))
        if query2:
            print "Q2:", query2, filter2
            soln_raw.extend(self.__model.solve(query2, **filter2))

        act = None
        if soln_raw:
            random.shuffle(soln_raw)
            uniq = {}
            soln = []
            for x in soln_raw:
                if not uniq.has_key(x.get_word()):
                    soln.append(x)
                    uniq[x.get_word()] = 1
            act = gnoetics.Action(poem, i, soln, right=right)

        return act

        
                
        
