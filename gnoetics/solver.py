
import random
import gnoetics

POS_TAG_BREAK = gnoetics.pos_from_string("(x)")
POS_TAG_WILDCARD = gnoetics.pos_from_string("(*)")

def clean_options(L):
    random.shuffle(L)
    seen = {}
    i = 0
    while i < len(L):
        x = L[i]
        if seen.has_key(x):
            L.pop(i)
        else:
            seen[x] = 1
            i += 1

###
### Case: --n--
###
def solve_free(model, bag, our_poem, chunk_num, info):
    assert 0
    meter = info["meter"]
    all_pos = words_by_pos.keys()
    random.shuffle(all_pos)
    for pos in all_pos:
        word = words_by_pos[pos].pick_by_meter_left(meter)
        if word:
            our_poem.bind(chunk_num, pos=pos, word=word)
            return 1
    return 0
    
###
### Cases: (x) --n--
###        (x) A B C --n--
###        A B C --n--
###
def solve_leading(model, bag, our_poem, chunk_num, info):
    N = model.get_N()
    vec = [POS_TAG_BREAK]*N
    vec.extend(info["predecessors"])
    vec.append(POS_TAG_WILDCARD)
    vec = vec[-N:]
    options = model.solve(vec)
    if not options:
        return 0
    clean_options(options)
    for pos in options:
        if pos == POS_TAG_BREAK:
            our_poem.bind(chunk_num, insert_break=1)
            return 1
        else:
            min_syl = 0
            if info["pred_zero"]:
                min_syl = 1
            word = bag.pick_by_meter_left(pos, min_syl, info["meter"])
            if word:
                our_poem.bind(chunk_num, pos=pos, word=word)
                return 1
    return 0

###
### Cases: --n-- (x)
###        (x) --n-- (x)
###        (x) A B C --n-- (x)
###        A B C --n-- (x)
###
def solve_trailing_break(model, bag, our_poem, chunk_num, info):
    vec = [POS_TAG_WILDCARD] + [POS_TAG_BREAK]*(model.get_N()-1)
    options = model.solve(vec)
    if not options:
        return 0
    clean_options(options)
    for pos in options:
        if pos == POS_TAG_BREAK: # we should never get adjacent breaks
            assert 0
        else:
            word = bag.pick_by_meter_right(pos, 0, info["meter"])
            if word:
                our_poem.bind(chunk_num, pos=pos, word=word, right_bind=1)
                return 1
    return 0

###
### Case: --n-- A B C (x)
###
def solve_trailing(model, bag, chunk_num, info):
    assert 0

###
### Cases: (x) --n-- A B C (x)
###        (x) A B C --n-- D E F (x)
###        (x) A B C --n-- D E F
###        A B C --n-- D E F (x)
###
def solve_pinch(model, bag, our_poem, chunk_num, info):
    N = model.get_N()

    vec1 = []
    if info["pred_break"]:
        vec1 = [POS_TAG_BREAK] * N
    vec1.extend(info["predecessors"])

    vec2 = info["successors"]
    if info["succ_break"]:
        vec2.extend([POS_TAG_BREAK] * N)

    leading_options = []
    if len(vec1) >= N-1:
        leading_vec = vec1[-(N-1):] + [POS_TAG_WILDCARD]
        leading_options = model.solve(leading_vec)

    pinched_options = []
    for i in range(1, N-1):
        pinch_vec = vec1[-i:] + [POS_TAG_WILDCARD] + vec2[:(N-1-i)]
        if len(pinch_vec) == N:
            pinched_options.extend(model.solve(pinch_vec))

    all_options = []
    all_options.extend(map(lambda x:(x, 0), leading_options))
    all_options.extend(map(lambda x:(x, 1), pinched_options))
    clean_options(all_options)

    for pos, is_pinched in all_options:

        if is_pinched:

            # Don't allow breaks on the "full pinch".
            if pos != POS_TAG_BREAK:
                word = bag.pick_by_meter_left(pos, len(info["meter"]),
                                              info["meter"])
                if word:
                    our_poem.bind(chunk_num, pos=pos, word=word)
                    return 1
            
        else:
            if pos == POS_TAG_BREAK:
                our_poem.bind(chunk_num, insert_break=1)
                return 1

            min_syl = 0
            if info["pred_zero"]:
                min_syl = 1
            word = bag.pick_by_meter_left(pos, min_syl, info["meter"][:-1])
            if word:
                our_poem.bind(chunk_num, pos=pos, word=word)
                return 1

    return 0

    

def solve(model, bag, our_poem, chunk_num):

    info = our_poem._study_free_chunk(chunk_num)

    args = (model, bag, our_poem, chunk_num, info)

    pred_break   = info["pred_break"]
    succ_break   = info["succ_break"]
    predecessors = info["predecessors"]
    successors   = info["successors"]

    loud = 0

    if not predecessors and not successors \
       and not pred_break and not succ_break:
        if loud:
            print "solve_free"
        return solve_free(*args)

    if not successors and succ_break:
        # in this case we don't care about any predecessors
        if loud:
            print "solve_trailing_break"
        return solve_trailing_break(*args)

    if not successors and not succ_break:
        assert predecessors or pred_break
        if loud:
            print "solve_leading"
        return solve_leading(*args)

    if not predecessors and not pred_break:
        assert successors or succ_break
        if loud:
            print "solve_trailing"
        return solve_trailing(*args)

    if (predecessors or pred_break) and successors:
        if loud:
            print "solve_pinch"
        return solve_pinch(*args)

    print "We Fell through!"
    print "predecessors =", predecessors
    print "  pred_break =", pred_break
    print "  successors =", successors
    print "  succ_break =", succ_break
    print

    return 0
    
          
          
