
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

#for x in range(40):
#    t = model.pick(min_syllables=5, is_punctuation=-1)
#    print t, t.get_syllables()

#sys.exit(0)

def tokenize(s):
    return map(gnoetics.token_lookup, s.split())

def spew():
    sentence = []
    vec = tokenize("*break* "*(N-1) + "*wild*")
    while 1:
        #L = model.solve(vec, rhymes_with="green")
        L = model.solve(vec)
        if not L:
            return
        x = random.choice(L)
        if x.is_break():
            break
        sentence.append(x)
        vec.pop(0)
        vec.pop(-1)
        vec.append(x)
        vec.append(gnoetics.token_lookup("*wild*"))

    print gnoetics.token_list_to_str(sentence)


while 1:
    spew()




