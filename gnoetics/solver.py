# This is -*- Python -*-

import time, os

import gnoetics

class SolveFailed:
    pass


def find_leading_trailing(poem, i):

    assert 0 <= i < len(poem)

    u = poem[i]
    assert u.is_not_bound()

    leading_tokens = []
    trailing_tokens = []

    j = i-1
    while 0 <= j:
        if poem[j].is_not_bound():
            break
        tok = poem[j].get_binding()
        leading_tokens.insert(0, tok)
        if tok.is_break():
            break
        j -= 1

    j = i+1
    while j < len(poem):
        if poem[j].is_not_bound():
            break
        tok = poem[j].get_binding()
        trailing_tokens.append(tok)
        if tok.is_break():
            break
        j += 1

    return leading_tokens, trailing_tokens



def solve_unit(model, leading_tokens, unit, trailing_tokens,
               allow_left_queries=True,
               allow_right_queries=False,
               verbose=None,
               extra_filters=True):

    # Copy list of leading and trailing tokens, so we can
    # manipulate them freely
    leading_tokens = list(leading_tokens)
    trailing_tokens = list(trailing_tokens)

    if verbose is None:
        verbose = os.getenv("GNOETRY_DEBUG") is not None

    def token_list_is_valid(token_list, allow_break_at):
        if allow_break_at < 0:
            allow_break_at += len(token_list)
        for i, tok in enumerate(token_list):
            if tok.is_wildcard():
                return False # No wildcards!
            if tok.is_break() and i != allow_break_at:
                return False # Breaks only where we say it is OK!
        return True

    lead_ok = token_list_is_valid(leading_tokens, allow_break_at=0)
    trail_ok = token_list_is_valid(trailing_tokens, allow_break_at=-1)

    if (not lead_ok) and verbose:
        print "Bad leading tokens!"
        print leading_tokens
        print

    if (not trail_ok) and verbose:
        print "Bad trailing tokens!"
        print trailing_tokens
        print

    assert lead_ok
    assert trail_ok

    # Double leading/trailing breaks
    
    if leading_tokens and leading_tokens[0].is_break():
        leading_tokens.insert(0, gnoetics.token_lookup_break())

    if trailing_tokens and trailing_tokens[-1].is_break():
        trailing_tokens.append(gnoetics.token_lookup_break())

    assert len(leading_tokens) + len(trailing_tokens) >= 2

    assert unit.is_not_bound()

    syl = unit.get_syllables()
    assert syl > 0
    
    meter = unit.get_meter()

    word_count = 0
    punct_count = 0
    for tok in leading_tokens:
        if tok.is_punctuation():
            punct_count += 1
        elif not tok.is_break():
            word_count += 1
    for tok in trailing_tokens:
        if tok.is_punctuation():
            punct_count += 1
        elif not tok.is_break():
            word_count += 1

    discourage_punct = False
    if word_count > 2 and word_count < 3 * punct_count:
        discourage_punct = True

    def punctution_between(token_list, i0, i1):
        for i in range(i0, i1+1):
            if -len(token_list) <= i < len(token_list) \
                   and token_list[i].is_punctuation():
                return True
        return False

    forbid_punct = False
    if extra_filters and \
       (punctution_between(leading_tokens, -2, -1) \
        or punctution_between(trailing_tokens, 0, 1)):
        forbid_punct = True


    have_both = False
    def single_both(s, b):
        if have_both:
            return b
        else:
            return s
        
    left_queries = []
    right_queries = []

    if leading_tokens and trailing_tokens:
        have_both = True

        if trailing_tokens[0].is_break():
            allow_left_queries = False
            allow_right_queries = True
            discourage_punct = False
            forbid_punct = False

        else:

            Q_filter = { "min_syllables": syl,
                         "max_syllables": syl,
                         "meter_left":    meter,
                         }
                
            Q = (leading_tokens[-1],
                 gnoetics.token_lookup_wildcard(),
                 trailing_tokens[0],
                 Q_filter)

            left_queries.append(Q)  # or right


    if leading_tokens and allow_left_queries:
        
        Q_filter = { "min_syllables": 0,
                     "max_syllables": single_both(syl, syl-1),
                     "meter_left":    meter,
                     }

        Q = (leading_tokens[-2],
             leading_tokens[-1],
             gnoetics.token_lookup_wildcard(),
             Q_filter)

        left_queries.append(Q)


    if trailing_tokens and allow_right_queries:

        Q_filter = { "min_syllables": 0,
                     "max_syllables": single_both(syl, syl-1),
                     "meter_right":   meter,
                     }

        Q = (gnoetics.token_lookup_wildcard(),
             trailing_tokens[0],
             trailing_tokens[1],
             Q_filter)

        right_queries.append(Q)


    if discourage_punct or forbid_punct:
        if discourage_punct:
            pref = 0
        else:
            pref = -1
        for Q in left_queries:
            Q[-1]["punctuation_preference"] = pref
        for Q in right_queries:
            Q[-1]["punctuation_preference"] = pref


    if left_queries:
        if verbose:
            print "Left queries:", left_queries
        left_solns = model.query(*left_queries)
        if verbose:
            uniq = {}
            for x in left_solns:
                uniq[x.get_word()] = 1
            print "Left solns:", uniq.keys()
    else:
        left_solns = []

    if right_queries:
        if verbose:
            print "Right queries:", right_queries
        right_solns = model.query(*right_queries)
        if verbose:
            uniq = {}
            for x in right_solns:
                uniq[x.get_word()] = 1
            print "Right solns:", uniq.keys()
    else:
        right_solns = []

    return left_solns, right_solns

#############################################################################

class Solver:

    def __init__(self, model):
        self.__model = model
        self.__actions = []
        self.__poem = None


    def set_poem(self, p):
        self.__poem = p
        self.__actions = []


    def get_poem(self):
        return self.__poem


    def __next_position(self):
        assert self.__poem
        assert not self.__poem.is_fully_bound()
        return self.__poem.find_first_unbound()


    def __solve_at(self, i, verbose=None):

        if verbose is None:
            verbose = os.getenv("GNOETRY_DEBUG") is not None

        assert self.__poem
        assert 0 <= i < len(self.__poem)

        u = self.__poem[i]
        assert u.is_not_bound()

        # If the stuff in our action queue isn't immediately
        # adjacent to where we are now, obviously rolling back
        # won't help us.
        # FIXME: Obviously this will have to be more subtle once
        # we have cross-line constraints.
        if self.__actions and abs(self.__actions[-1][0] - i) > 1:
            self.__actions = []

        if u.get_syllables() == 0 and verbose:
            print "Hit 0-length unit at %d" % i
            for j in range(len(self.__poem)):
                if i == j:
                    print "*****",
                print j, self.__poem[j]
            print

        leading_tokens, trailing_tokens = self.__poem.extract_surrounding_tokens(i)

        left_solns, right_solns = gnoetics.solve_unit(self.__model,
                                                      leading_tokens,
                                                      u,
                                                      trailing_tokens)

        # If we found no solutions, try again w/o the extra filters.
        if not left_solns and not right_solns:
            left_solns, right_solns = gnoetics.solve_unit(self.__model,
                                                          leading_tokens,
                                                          u,
                                                          trailing_tokens,
                                                          extra_filters=False)


        if verbose:
            print "pos=%d left_solns=%d right_solns=%d" % (i,
                                                           len(left_solns),
                                                           len(right_solns))

        if left_solns:
            self.__actions.append([i, "left", left_solns, right_solns])
        elif right_solns:
            self.__actions.append([i, "right", left_solns, right_solns])
        else:
            while 1:
                if not self.__actions:
                    raise SolveFailed
                act = self.__actions[-1]
                i = act[0]
                mode = act[1]
                if mode == "left":
                    self.__poem.unbind(i)
                    if verbose:
                        print "left-unbind at %d" % i
                else:
                    self.__poem.unbind(i+1)
                    if verbose:
                        print "right-unbind at %d" % i
                left_solns = act[2]
                right_solns = act[3]
                if right_solns and not left_solns:
                    act[1] = "right"
                if left_solns or right_solns:
                    break
                self.__actions.pop(-1)

        if left_solns:
            tok = left_solns.pop(0)
            if verbose:
                print "left-bind at %d" % i
            self.__poem.bind_left(i, tok)
        else:
            tok = right_solns.pop(0)
            if verbose:
                print "right-bind at %d" % i
            self.__poem.bind_right(i, tok)


    def step(self):
        if not self.__poem:
            return

        if self.__poem.is_fully_bound():
            return

        self.__poem.bind_mandatory_breaks()

        verbose = os.getenv("GNOETRY_DEBUG") is not None

        i = self.__next_position()

        if verbose:
            print "Initial state (solving at %d):" % i
            for j in range(len(self.__poem)):
                if j == i:
                    print "*****",
                print j, self.__poem[j]
            print
    
        self.__solve_at(i)


    def multistep(self, count=10, timeout=0.2):
        if not self.__poem:
            return

        t1 = time.time()
        N = 0
        
        self.__poem.freeze_changed()

        while self.__poem.is_not_fully_bound():

            if count is not None and count > 0 and N > count:
                break

            if timeout is not None:
                t2 = time.time()
                if t2 - t1 > timeout:
                    break

            self.step()

            N += 1

        self.__poem.thaw_changed()


    def full_solution(self):
        verbose = os.getenv("GNOETRY_DEBUG") is not None
        if verbose:
            print
            print "-" * 50
            print
            
        self.__actions = []
        self.multistep(count=None, timeout=None)
