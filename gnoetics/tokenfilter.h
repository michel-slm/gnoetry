/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __TOKENFILTER_H__
#define __TOKENFILTER_H__

#include <Python.h>
#include <glib.h>

#include "rhyme.h"
#include "token.h"

typedef enum {
    FILTER_RESULTS_REJECT   = -1,
    FILTER_RESULTS_TOLERATE =  0,
    FILTER_RESULTS_ACCEPT   =  1,
    FILTER_RESULTS_FAVOR    =  2,
} FilterResults;

typedef gboolean (*FilterResultsFn) (Token *, FilterResults, gpointer);

typedef struct _TokenFilter TokenFilter;
struct _TokenFilter {

    gboolean is_impossible;
    gboolean is_optimized;

    /* Local criteria */

    FilterResults break_preference;
    FilterResults punctuation_preference;

    int min_syllables;
    int max_syllables;

    Meter *meter_left;
    Meter *meter_right;
    double metric_error_lower_threshold;
    double metric_error_upper_threshold;

    Token *rhymes_with;
    RhymeType rhyme_type_lower_threshold;
    RhymeType rhyme_type_upper_threshold;

    /* Non-local or model-sensitive criteria */

    FilterResults leading_preference;
    FilterResults trailing_preference;
};

void token_filter_init              (TokenFilter *filter);
void token_filter_init_from_py_dict (TokenFilter *filter,
                                     PyObject    *py_dict);

void token_filter_optimize (TokenFilter *filter);

void token_filter_clear    (TokenFilter *filter);

/* return FALSE if we need to filter out the token */
FilterResults token_filter_test (TokenFilter *filter,
                                 Token *token,
                                 TokenFn leading_test_cb,
                                 TokenFn trailing_test_cb,
                                 gpointer user_data);


#endif /* __TOKENFILTER_H__ */

