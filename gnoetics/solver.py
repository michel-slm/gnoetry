# This is -*- Python -*-

import gnoetics


def find_leading_trailing(poem, i):

    assert 0 <= i < len(poem)

    u = poem[i]
    assert u.is_not_bound()

    leading_tokens = []
    trailing_tokens = []

    j = i-1
    while 0 <= j:
        if poem[j].is_not_bound():
            break
        tok = poem[j].get_binding()
        leading_tokens.insert(0, tok)
        if tok.is_break():
            break
        j -= 1

    j = i+1
    while j < len(poem):
        if poem[j].is_not_bound():
            break
        tok = poem[j].get_binding()
        trailing_tokens.append(tok)
        if tok.is_break():
            break
        j += 1

    return leading_tokens, trailing_tokens



def solve_unit(model, leading_tokens, unit, trailing_tokens,
               allow_left_queries=True,
               allow_right_queries=False):

    def token_list_is_valid(token_list, allow_break_at):
        if allow_break_at < 0:
            allow_break_at += len(token_list)
        for i, tok in enumerate(token_list):
            if tok.is_wildcard():
                return False # No wildcards!
            if tok.is_break() and i != allow_break_at:
                return False # Breaks only where we say it is OK!
        return True

    assert token_list_is_valid(leading_tokens, allow_break_at=0)
    assert token_list_is_valid(trailing_tokens, allow_break_at=-1)

    # Double leading/trailing breaks
    
    if leading_tokens and leading_tokens[0].is_break():
        leading_tokens.insert(0, gnoetics.token_lookup_break())

    if trailing_tokens and trailing_tokens[-1].is_break():
        trailing_tokens.append(gnoetics.token_lookup_break())

    assert len(leading_tokens) + len(trailing_tokens) >= 2

    assert unit.is_not_bound()

    syl = unit.get_syllables()
    assert syl > 0
    
    meter = unit.get_meter()

    word_count = 0
    punct_count = 0
    for tok in leading_tokens:
        if tok.is_punctuation():
            punct_count += 1
        elif not tok.is_break():
            word_count += 1
    for tok in trailing_tokens:
        if tok.is_punctuation():
            punct_count += 1
        elif not tok.is_break():
            word_count += 1

    discourage_punct = False
    if word_count > 2 and word_count < 3 * punct_count:
        discourage_punct = True

    def punctution_between(token_list, i0, i1):
        for i in range(i0, i1+1):
            if -len(token_list) <= i < len(token_list) \
                   and token_list[i].is_punctuation():
                return True
        return False

    forbid_punct = False
    if punctution_between(leading_tokens, -2, -1) \
           or punctution_between(trailing_tokens, 0, 1):
        forbid_punct = True


    have_both = False
    def single_both(s, b):
        if have_both:
            return b
        else:
            return s
        
    left_queries = []
    right_queries = []

    if leading_tokens and trailing_tokens:
        have_both = True

        if trailing_tokens[0].is_break():
            allow_left_queries = False
            allow_right_queries = True
            discourage_punct = False
            forbid_punct = False

        else:

            Q_filter = { "min_syllables": syl,
                         "max_syllables": syl,
                         "meter_left":    meter,
                         }
                
            Q = (leading_tokens[-1],
                 gnoetics.token_lookup_wildcard(),
                 trailing_tokens[0],
                 Q_filter)

            left_queries.append(Q)  # or right


    if leading_tokens and allow_left_queries:
        
        Q_filter = { "min_syllables": 0,
                     "max_syllables": single_both(syl, syl-1),
                     "meter_left":    meter,
                     }

        Q = (leading_tokens[-2],
             leading_tokens[-1],
             gnoetics.token_lookup_wildcard(),
             Q_filter)

        left_queries.append(Q)


    if trailing_tokens and allow_right_queries:

        Q_filter = { "min_syllables": 0,
                     "max_syllables": single_both(syl, syl-1),
                     "meter_right":   meter,
                     }

        Q = (gnoetics.token_lookup_wildcard(),
             trailing_tokens[0],
             trailing_tokens[1],
             Q_filter)

        right_queries.append(Q)


    if discourage_punct or forbid_punct:
        if discourage_punct:
            pref = 0
        else:
            pref = -1
        for Q in left_queries:
            Q[-1]["punctuation_preference"] = pref
        for Q in right_queries:
            Q[-1]["punctuation_preference"] = pref


    if left_queries:
        left_solns = model.query(*left_queries)
    else:
        left_solns = []

    if right_queries:
        right_solns = model.query(*right_queries)
    else:
        right_solns = []

    return left_solns, right_solns
          
