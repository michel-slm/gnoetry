#!/usr/bin/python

import random, sys, string, time
import gnoetics


N = 3
model = gnoetics.TokenModel(N)

all_texts = ("fu-manchu.ts",
             "great-expectations.ts",
             "time-machine.ts",
             "wuthering-heights.ts",
             "moby-dick.ts",
             "red-badge-of-courage.ts",
             "call-of-the-wild.ts",
             "tale-of-two-cities.ts",
             "oliver-twist.ts",
             "origin-of-species.ts",
             "emma.ts")

some_texts = ("fu-manchu.ts", "emma.ts")

for name in some_texts:
    sys.stderr.write("%s...\n" % name)
    name = "../texts-ts/" + name
    txt = gnoetics.Text(name)
    model.add_text(txt)

poem = gnoetics.Haiku()

sol = gnoetics.SimpleSolver(model)

sol.solve(poem)
poem.dump()
        
