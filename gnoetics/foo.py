#!/usr/bin/python

import random, sys, string, time
import gnoetics

p = gnoetics.Sonnet()

p.bind_left(0, gnoetics.token_lookup_break())
p.bind_left(1, gnoetics.token_lookup("the"))
p.bind_left(2, gnoetics.token_lookup("insane"))
p.bind_left(3, gnoetics.token_lookup("butchershop"))
p.bind_left(4, gnoetics.token_lookup("was"))
p.bind_left(5, gnoetics.token_lookup("undertow"))
p.bind_left(6, gnoetics.token_lookup("before"))
p.bind_left(7, gnoetics.token_lookup("*punct* ."))
p.bind_left(8, gnoetics.token_lookup_break())
p.bind_left(9, gnoetics.token_lookup("who"))
p.bind_left(10, gnoetics.token_lookup("soever"))
p.bind_right(13, gnoetics.token_lookup("monkey"))

print p.to_string()
