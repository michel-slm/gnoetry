/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __TOKENFILTER_H__
#define __TOKENFILTER_H__

#include <Python.h>
#include <glib.h>

#include "rhyme.h"
#include "token.h"

typedef enum {
    FILTER_BLOCK = -1,
    FILTER_NEUTRAL = 0,
    FILTER_REQUIRE = 1,
} Filter;

#define filter_test(f, x) (((f)==FILTER_NEUTRAL)||((f)>0?(x):!(x)))

typedef struct _TokenFilter TokenFilter;
struct _TokenFilter {
    Filter in_dictionary;
    Filter is_punctuation;

    gboolean allow_break;

    int min_syllables;
    int max_syllables;

    Meter *meter_left;
    Meter *meter_right;
    double max_metric_error;

    Token *rhymes_with;
    RhymeType min_rhyme_type;
    
    gboolean impossible;
};

void token_filter_init              (TokenFilter *filter);
void token_filter_init_from_py_dict (TokenFilter *filter,
                                     PyObject    *py_dict);

void token_filter_optimize (TokenFilter *filter);

void token_filter_clear (TokenFilter *filter);

/* return FALSE if we need to filter out the token */
gboolean token_filter_test (TokenFilter *filter, Token *token);


#endif /* __TOKENFILTER_H__ */

