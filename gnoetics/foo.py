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


time1 = time.time()
tri.prepare()
time2 = time.time()
print "prep time:", time2-time1

def babble():
    t1 = gnoetics.token_lookup_break()
    t2 = gnoetics.token_lookup_break()
    t3 = gnoetics.token_lookup_wildcard()
    tokfilt = { }

    while 1:
        results = tri.query(t1, t2, t3, tokfilt)
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


while 1:
    babble()
    print

