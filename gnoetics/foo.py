#!/usr/bin/python

import random, sys, string, time
import gnoetics

tri = gnoetics.Trimodel()

time1 = time.time()
for n in ("emma", "moby-dick", "oliver-twist", "origin-of-species",
          "tale-of-two-cities", "red-badge-of-courage"):
    txt = gnoetics.Text("../texts-ts/%s.ts" % n)
    tri.add_text(txt)
time2 = time.time()

print "load time:", time2-time1


while 1:
    time1 = time.time()
    tri.prepare()
    time2 = time.time()
    print "prep time:", time2-time1

sys.exit(0)

query_times = []

def babble():

    global query_times
    
    t1 = gnoetics.token_lookup_break()
    t2 = gnoetics.token_lookup_break()
    t3 = gnoetics.token_lookup_wildcard()
    tokfilt = { }

    while 1:
        time1 = time.time()
        results = tri.query((t1, t2, t3, tokfilt))
        time2 = time.time()
        query_times.append(time2-time1)
        if not results:
            print "!!!!!!"
            break
        t = results[0]
        if t.is_break():
            break
        print t.get_word(),

        t1 = t2
        t2 = t

    print


for i in range(100):
    babble()
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

