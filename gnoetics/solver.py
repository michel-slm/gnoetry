# This is -*- Python -*-

import gnoetics
import random


class GenericSolver:


    def __init__(self):
        self.__action_list = []


    # Return the left/right score of an unbound unit.  High-score units get
    # bound first.
    def unit_scores(self, poem, u, i):
        assert False


    # Returns the index of the next unit to solve, along w/ the
    # left/right binding type.
    def next_pos(self, poem):
        L = []
        for i, u in enumerate(poem):
            if u.is_not_bound():
                left_score, right_score = self.unit_scores(poem, u, i)
                print i, left_score, right_score
                # Since this is a stable sort, ordering the appends
                # this way favors right-binding.
                L.append((i, False, right_score))
                L.append((i, True, left_score))
        if not L:
            return None, None
        L.sort(lambda x, y: cmp(y[-1], x[-1]))
        return L[0][:2]
            

    # Given an index and left/right, return an Action item,
    # or None if no solutions are found.
    def find_action(self, poem, u, i, is_left=True):
        assert False


    def step(self, poem):
        if poem.is_fully_bound():
            return False
        i, is_left = self.next_pos(poem)
        print "next:", i, ((is_left and "left") or "right")
        assert 0 <= i < len(poem)
        u = poem[i]
        assert u.is_not_bound()
        act = self.find_action(poem, u, i, is_left=is_left)
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
            poem.dump()
        print "*****"
        poem.dump()


############################################################################


# FIXME: needs to be changed to reflect changes to GenericSolver,
# PoeticUnit, Poem, etc.

class SimpleSolver(GenericSolver):

    def __init__(self, model):
        GenericSolver.__init__(self)
        assert model.get_N() >= 3
        self.__model = model


    def unit_scores(self, poem, u, i):
        left_score = 0
        right_score = 0

        if u.is_right_logic_constrained():
            right_score += 100
        if u.is_rhymed():
            right_score += 100
        if i == len(poem)-1 or (i < len(poem)-1 and poem[i+1].is_bound()):
            right_score += 100
                        
        if u.is_left_logic_constrained():
            left_score += 100
        if i == 0 or (i > 0 and poem[i-1].is_bound()):
            left_score += 100
        if u.is_rhymed():
            left_score += 50

        return left_score, right_score


    def find_action(self, poem, u, i, is_left=True):
        assert u.is_not_bound()

        n = u.get_syllables()
        
        t_break = gnoetics.token_lookup_break()
        t_wild  = gnoetics.token_lookup_wildcard()

        u_prev = poem.get(i-1) # or None if out-of-bounds
        u_next = poem.get(i+1) # ditto

        t_prev = None
        t_next = None
 
        if u.is_head() or (u_prev and u_prev.is_tail()):
            t_prev = t_break
        elif u_prev and u_prev.is_bound():
            t_prev = u_prev.get_binding()

        if u.is_tail() or (u_next and u_next.is_head()):
            t_next = t_break
        elif u_next and u_next.is_bound():
            t_next = u_next.get_binding()

            
        query1 = [t_prev, t_wild, t_next]
        filter1 = {}
        filter1["min_syllables"] = n
        filter1["max_syllables"] = n

        query2 = []
        filter2 = {}

        if n > 1:
            filter2["min_syllables"] = 0
            filter2["max_syllables"] = n-1
            if is_left:
                query2 = [t_prev, t_wild]
            else: # is right
                query2 = [t_wild, t_next]


        ## Process the queries
        
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
            act = gnoetics.Action(poem, i, soln, is_left=is_left)

        return act

        
                
        
