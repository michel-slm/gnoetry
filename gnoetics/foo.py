#!/usr/bin/python

import random, sys, string, time
import gnoetics

tri = gnoetics.Trimodel()

time1 = time.time()
count = 0
for txt in gnoetics.Library(dir="../texts-ts"):
    tri.add_text(txt)
    count += 1
    if count > 55:
        break
time2 = time.time()

print "load time:", time2-time1


time1 = time.time()
tri.prepare()
time2 = time.time()
print "prep time:", time2-time1

query_times = []

def babble():

    global query_times
    
    t1 = gnoetics.token_lookup_break()
    t2 = gnoetics.token_lookup_break()
    t3 = gnoetics.token_lookup_wildcard()
    tokfilt = { }

    syl = 0
    syl_since_punct = 0

    token_list = []

    while 1:
        time1 = time.time()

        if syl > 18:
            tokfilt["break_preference"] = 1
        elif syl > 10:
            tokfilt["break_preference"] = 0
        else:
            tokfilt["break_preference"] = -1
            

        if syl_since_punct < 7:
            tokfilt["punctuation_preference"] = 0
        else:
            tokfilt["punctuation_preference"] = 1

        if syl == 0:
            tokfilt["rhymes_with"] = "mouse"
            tokfilt["punctuation_preference"] = -1
        else:
            tokfilt["rhymes_with"] = None



        if syl % 2 == 0:
            tokfilt["meter_left"] = "u-" * 20
        else:
            tokfilt["meter_left"] = "-u" * 20
        results = tri.query((t1, t2, t3, tokfilt))
        time2 = time.time()
        query_times.append(time2-time1)
        if not results:
            return 0
        t = results[0]
        if t.is_punctuation():
            syl_since_punct = 0
        if t.is_break():
            break
        token_list.append(t)

        syl += t.get_syllables()
        syl_since_punct += t.get_syllables()

        t1 = t2
        t2 = t

    print gnoetics.token_list_to_str(token_list)

    return 1


for i in range(5):
    while not babble(): pass
    print

query_times.sort()
N = len(query_times)
sum = 0
for t in query_times:
    sum += t
print "query times"
print " total: %f" % sum
print "   min: %f" % query_times[0]
print "   med: %f" % query_times[N/2]
print "  mean: %f" % (sum/N)
print "   max: %f" % query_times[-1]

