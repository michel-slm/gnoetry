#!/usr/bin/python

import sys, string, os

def generate_from_xml(filename, headers):
    sys.stderr.write("Processing %s\n" % filename)
    name, ext = os.path.splitext(filename)

    tokens = []
    for line in os.popen("./gxml2txt %s | ./txt2ts" % filename):
        tokens.append(line)

    if len(tokens) < 10:
        return
    
    out = file("../texts-ts/%s.ts" % os.path.basename(name), "w")
    for line in headers:
        out.write(line + "\n")
    for line in tokens:
        out.write(line)
    sys.stderr.write("\n")


def generate_from_txt(filename):
    sys.stderr.write("Processing %s\n" % filename)
    name, ext = os.path.splitext(filename)

    out_name = "../texts-ts/%s.ts" % os.path.basename(name)
    os.system("./txt2ts %s > %s" % (filename, out_name))
    
    

############################################################################

###
### First, do the XML files
###

filename = None
headers = []

for line in file("../texts-gxml/TEXTS"):
    line = line.strip()

    if line and line[0] == "#":
        continue

    if line == "":

        if filename:
            generate_from_xml("../texts-gxml/%s" % filename, headers)
            filename = None
            headers = []

        continue

    if filename:
        headers.append(line)
    else:
        filename = line


###
### Next, do the text files
###


dir = "../texts-txt"
for name in os.listdir(dir):
    if name[-4:] != ".txt":
        continue
    name = os.path.join(dir, name)
    generate_from_txt(name)

            

    
