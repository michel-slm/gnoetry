#!/usr/bin/python

import sys, string, os

def generate(filename, headers):
    sys.stderr.write("Processing %s\n" % filename)
    name, ext = os.path.splitext(filename)
    out = file("../texts-ts/%s.ts" % name, "w")
    for line in headers:
        out.write(line + "\n")
    for line in os.popen("./gxml2ts %s | ./tscleaner" % filename):
        out.write(line)
    sys.stderr.write("\n")

############################################################################

filename = None
headers = []

for line in file("../texts-gxml/TEXTS"):
    line = line.strip()

    if line and line[0] == "#":
        continue

    if line == "":

        if filename:
            generate("../texts-gxml/%s" % filename, headers)
            filename = None
            headers = []

        continue

    if filename:
        headers.append(line)
    else:
        filename = line
            

    
