#!/usr/bin/python

import random, sys, string, time
import gnoetics

tri = gnoetics.Trimodel()

sys.stderr.write("Loading... ")
t1 = time.time()
count = 0
for txt in gnoetics.Library(dir="../texts-ts"):
    tri.add_text(txt)
    count += 1
    if count > 995:
        break
t2 = time.time()
sys.stderr.write("done. (%.1fs)\n" % (t2-t1))

sys.stderr.write("Preparing model... ")
t1 = time.time()
tri.prepare()
t2 = time.time()
sys.stderr.write("done. (%.1fs)\n" % (t2-t1))

sys.stderr.write("\n")

##############################################################################

for i in range(5):

    t1 = time.time()

    verse = gnoetics.BlankVerse(4, 4)

    i = 0
    while i < len(verse):
        u = verse[i]
        if u.is_not_bound() and u.is_head():
            verse.bind_left(i, gnoetics.token_lookup_break())
            i += 1
        elif u.is_not_bound() and u.is_tail():
            verse.bind_right(i, gnoetics.token_lookup_break())
        else:
            i += 1


    # Actions are three-tuples of the form:
    #  (unit index, left_solns, right_solns)
    actions = []

    while verse.is_not_fully_bound():

        i = verse.find_first_unbound()
        u = verse[i]

        leading_tokens, trailing_tokens = \
                        gnoetics.find_leading_trailing(verse, i)

        left_solns, right_solns = gnoetics.solve_unit(tri,
                                                      leading_tokens,
                                                      u,
                                                      trailing_tokens)

        if left_solns:
            actions.append([i, "left", left_solns, right_solns])
        elif right_solns:
            actions.append([i, "right", left_solns, right_solns])
        else:
            while 1:
                assert actions
                act = actions[-1]
                i = act[0]
                mode = act[1]
                if mode == "left":
                    verse.unbind(i)
                else:
                    verse.unbind(i+1)
                left_solns = act[2]
                right_solns = act[3]
                if right_solns and not left_solns:
                    act[1] = "right"
                if left_solns or right_solns:
                    break
                actions.pop(-1)

        if left_solns:
            tok = left_solns.pop(0)
            verse.bind_left(i, tok)
        else:
            tok = right_solns.pop(0)
            verse.bind_right(i, tok)


    t2 = time.time()

    print verse.to_string()

    print "(Generated poem in %.2fs)\n\n" % (t2-t1)












