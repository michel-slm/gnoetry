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

##############################################################################

lines_per_stanza = 4
stanzas = 6

lines = lines_per_stanza * stanzas

line_meter = (gnoetics.METER_UNSTRESSED+gnoetics.METER_STRESSED)*5
line_len = len(line_meter)

brk  = gnoetics.token_lookup_break()
wild = gnoetics.token_lookup_wildcard()



syl = 0
poem = [ [brk] ]

while syl < lines * line_len:

    assert poem

    t2 = poem[-1][0]
    if t2.is_break():
        t1 = t2
    else:
        t1 = poem[-2][0]

    i = -1
    sentence_len = 0
    punct_count = 0
    while 1:
        x = poem[i][0]
        if x.is_break():
            break
        i -= 1
        if x.is_punctuation():
            punct_count += 1
        else:
            sentence_len += 1

    punct_ratio = 1
    if sentence_len > 0:
        punct_ratio = punct_ratio / float(sentence_len)

    tokfilt = {}
    tokfilt["max_syllables"] = line_len - (syl % line_len)
    tokfilt["meter_left"] = line_meter[syl % line_len:]

    if sentence_len < 3:
        tokfilt["trailing_preference"] = -1
    elif sentence_len < 6:
        tokfilt["trailing_preference"] = 0
    elif sentence_len > 10:
        tokfilt["trailing_preference"] = 2

    soln = tri.query((t1, t2, wild, tokfilt))

    if soln:
        for x in soln:
            assert x.get_syllables() <= tokfilt["max_syllables"]
        poem.append(soln)
        syl += soln[0].get_syllables()
    else:
        while 1:
            syl -= poem[-1][0].get_syllables()
            poem[-1].pop(0)
            if poem[-1]:
                syl += poem[-1][0].get_syllables()
                break
            else:
                poem.pop(-1)


###
### Print the poem
###

poem_str = ""

syl = 0
capitalize_next = 1
for L in poem:
    t = L[0]
    if syl > 0 and t.get_syllables() > 0 and syl % line_len == 0:
        poem_str += "\n"
        if syl % ((line_len*lines)/stanzas) == 0:
            poem_str += "\n"
            
    if t.is_break():
        capitalize_next = 1
    else:
        if not t.has_left_glue():
            poem_str += " "
        w = t.get_word()
        if capitalize_next:
            w = w[0].upper() + w[1:]
        poem_str += w
        capitalize_next = 0

    syl += t.get_syllables()

print poem_str


    









