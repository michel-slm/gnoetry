#!/usr/bin/python

import random, sys, string, time
import gnoetics

tri = gnoetics.Trimodel()
txt = gnoetics.Text("../texts-ts/emma.ts")
tri.add_text(txt)

def lookup(tri, t1, t2,
           min_syllables=0, max_syllables=-1):

    N = tri.get_raw_len()

    ###
    ### First, find a possible hit and save it in 'i'.
    ###

    a = 0
    b = N-1
    i = None

    while b - a > 1:
        i = (a+b)/2

        cmp = tri.get_raw_cmp(i, t1, t2)

        if cmp == 0:
            break
        elif cmp < 0:
            a = i
        else: # cmp > 0
            b = i

    if b - a <= 1:
        if tri.get_raw_cmp(a, t1, t2) == 0:
            i = a
        elif tri.get_raw_cmp(b, t1, t2) == 0:
            i = b
        else:
            return None, None

    ###
    ### Next, use our hit to find the beginning and end of the
    ### range of solutions.
    ###

    assert 0 <= i < N
    delta, delta_syl, hit_t1, hit_t2, hit_soln = tri.get_raw(i)
    assert hit_t1.get_word() == t1.get_word()
    assert hit_t2.get_word() == t2.get_word()

    ## Move back to the first hit
    i0 = i
    if delta < 0:
        i0 += delta

    assert 0 <= i0 < N
    delta, delta_syl, hit_t1, hit_t2, hit_soln = tri.get_raw(i0)
    assert delta >= 0
    assert hit_t1.get_word() == t1.get_word()
    assert hit_t2.get_word() == t2.get_word()

    ## Move forward to the last hit
    i1 = i0 + delta


    ###
    ### Finally, use the fact that solutions are syllable-sorted
    ### to find a subrange w/ syllables within certain bounds.
    ###

    ## Move forward to the first soln that meets the min number
    ## of syllables.
    while i0 <= i1:
        delta, delta_syl, foo1, foo2, soln = tri.get_raw(i0)
        if min_syllables <= soln.get_syllables():
            if min_syllables == max_syllables:
                i1 = i0 + delta_syl
                max_syllables = -1
            break
        i0 += delta_syl+1

    ## Move backward to the last solution that meets the max number
    ## of syllables.
    if max_syllables >= 0:
        while i0 <= i1:
            delta, delta_syl, foo1, foo2, soln = tri.get_raw(i1)
            if soln.get_syllables() <= max_syllables:
                break
            i1 -= delta_syl+1
        

    if i0 > i1:
        return None, None
    
    return (i0, i1)


t1 = gnoetics.token_lookup("and")
t2 = gnoetics.token_lookup("then")

i0, i1 = lookup(tri, t1, t2, min_syllables=1, max_syllables=3)
if i0 is None and i1 is None:
    print "none"
    sys.exit(0)
    
for i in range(i0-5, i1+6):
    delta, delta_syl, t1, t2, soln = tri.get_raw(i)
    if i == i0:
        print
    print i, delta, delta_syl, t1, t2, soln
    if i == i1:
        print
    

    
    

