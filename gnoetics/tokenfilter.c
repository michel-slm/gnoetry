/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "tokenfilter.h"

void
token_filter_init (TokenFilter *filter)
{
    g_return_if_fail (filter != NULL);

    filter->is_impossible = FALSE;
    filter->is_optimized  = FALSE;

    filter->break_preference       = FILTER_RESULTS_ACCEPT;
    filter->punctuation_preference = FILTER_RESULTS_ACCEPT;

    filter->min_syllables = -1;
    filter->max_syllables = -1;

    filter->meter_left = NULL;
    filter->meter_right = NULL;
    filter->metric_error_lower_threshold = 0;
    filter->metric_error_upper_threshold = 0;

    filter->rhymes_with = NULL;
    filter->rhyme_type_lower_threshold = RHYME_TRUE;
    filter->rhyme_type_upper_threshold = RHYME_FALSE;

    filter->leading_preference = FILTER_RESULTS_ACCEPT;
    filter->trailing_preference = FILTER_RESULTS_ACCEPT;
}

static gboolean
py_dict_get_int (PyObject   *py_dict,
                 const char *key,
                 int        *val)
{
    PyObject *py_val;
    long x;

    g_return_val_if_fail (py_dict != NULL, FALSE);
    g_return_val_if_fail (key && *key, FALSE);
    g_return_val_if_fail (val != NULL, FALSE);

    if (! PyMapping_HasKeyString (py_dict, (char *) key))
        return FALSE;

    py_val = PyMapping_GetItemString (py_dict, (char *) key);
    if (py_val == NULL || py_val == Py_None)
        return FALSE;

    if (! PyInt_Check (py_val)) {
        Py_DECREF (py_val);
        g_warning ("'%s' exists but is not an integer!", key);
        return FALSE;
    }

    x = PyInt_AsLong (py_val);
    *val = x;

    Py_DECREF (py_val);
    
    return TRUE;
}

static gboolean
py_dict_get_string (PyObject   *py_dict,
                    const char *key,
                    char      **val)
{
    PyObject *py_val;
    char *x;

    g_return_val_if_fail (py_dict != NULL, FALSE);
    g_return_val_if_fail (key && *key, FALSE);
    g_return_val_if_fail (val != NULL, FALSE);

    if (! PyMapping_HasKeyString (py_dict, (char *) key))
        return FALSE;

    py_val = PyMapping_GetItemString (py_dict, (char *) key);
    if (py_val == NULL || py_val == Py_None)
        return FALSE;

    if (! PyString_Check (py_val)) {
        Py_DECREF (py_val);
        g_warning ("'%s' exists but is not a string!", key);
        return FALSE;
    }

    x = PyString_AsString (py_val);
    *val = g_strdup (x);

    Py_DECREF (py_val);
    
    return TRUE;
}

static gboolean
py_dict_get_double (PyObject   *py_dict,
                    const char *key,
                    double     *val)
{
    PyObject *py_val;
    double x;

    g_return_val_if_fail (py_dict != NULL, FALSE);
    g_return_val_if_fail (key && *key, FALSE);
    g_return_val_if_fail (val != NULL, FALSE);

    if (! PyMapping_HasKeyString (py_dict, (char *) key))
        return FALSE;

    py_val = PyMapping_GetItemString (py_dict, (char *) key);
    if (py_val == NULL || py_val == Py_None)
        return FALSE;

    if (! PyFloat_Check (py_val)) {
        Py_DECREF (py_val);
        g_warning ("'%s' exists but is not a float!", key);
        return FALSE;
    }

    x = PyFloat_AsDouble (py_val);
    *val = x;

    Py_DECREF (py_val);
    
    return TRUE;
}

void
token_filter_init_from_py_dict (TokenFilter *filter,
                                PyObject    *py_dict)
{
    int x;

    g_return_if_fail (filter != NULL);
    
    token_filter_init (filter);

    if (py_dict == NULL)
        return;

    if (py_dict_get_int (py_dict, "break_preference", &x))
        filter->break_preference = (FilterResults) x;
    
    if (py_dict_get_int (py_dict, "punctuation_preference", &x))
        filter->punctuation_preference = (FilterResults) x;

    py_dict_get_int (py_dict, "min_syllables", &filter->min_syllables);

    py_dict_get_int (py_dict, "max_syllables", &filter->max_syllables);

    py_dict_get_string (py_dict, "meter_left", &filter->meter_left);

    py_dict_get_string (py_dict, "meter_right", &filter->meter_right);

    py_dict_get_double (py_dict, "metric_error_lower_threshold",
                        &filter->metric_error_lower_threshold);

    py_dict_get_double (py_dict, "metric_error_upper_threshold",
                        &filter->metric_error_upper_threshold);

    if (PyMapping_HasKeyString (py_dict, "rhymes_with")) {
        PyObject *py_val = PyMapping_GetItemString (py_dict, "rhymes_with");
        if (py_val != Py_None) {
            if (py_token_check (py_val)) {
                filter->rhymes_with = token_from_py (py_val);
            } else if (PyString_Check (py_val)) {
                filter->rhymes_with = token_lookup (PyString_AsString (py_val));
            } else {
                g_warning ("Unknown type for 'rhymes_with'");
            }
        }
        Py_DECREF (py_val);
    }

    if (py_dict_get_int (py_dict, "rhyme_type_lower_threshold", &x))
        filter->rhyme_type_lower_threshold = (RhymeType) x;

    if (py_dict_get_int (py_dict, "rhyme_type_upper_threshold", &x))
        filter->rhyme_type_upper_threshold = (RhymeType) x;

    if (py_dict_get_int (py_dict, "leading_preference", &x))
        filter->leading_preference = (FilterResults) x;

    if (py_dict_get_int (py_dict, "trailing_preference", &x))
        filter->trailing_preference = (FilterResults) x;

    token_filter_optimize (filter);
}

void
token_filter_optimize (TokenFilter *filter)
{
    g_return_if_fail (filter != NULL);

    if (filter->is_optimized)
        return;

    filter->is_optimized = TRUE;

    if (filter->meter_left != NULL) {
        int n = strlen (filter->meter_left);
        if (filter->max_syllables > n)
            filter->max_syllables = n;
    }

    if (filter->meter_right != NULL) {
        int n = strlen (filter->meter_right);
        if (filter->max_syllables > n)
            filter->max_syllables = n;
    }

    /* Some sanity checking */

    if (filter->rhymes_with != NULL
        && token_get_decomp (filter->rhymes_with) == NULL) {
        filter->is_impossible = TRUE;
        return;
    }

    if (filter->max_syllables >= 0
        && filter->min_syllables > filter->max_syllables) {
        filter->is_impossible = TRUE;
    }
}

void
token_filter_clear (TokenFilter *filter)
{
    if (filter != NULL) {
        g_free (filter->meter_left);
        g_free (filter->meter_right);
    }
}

FilterResults
token_filter_test (TokenFilter *filter,
                   Token       *token,
                   TokenFn      leading_test_cb,
                   TokenFn      trailing_test_cb,
                   gpointer     user_data)
{
    int syl;
    double metric_error;
    RhymeType rhyme_type;
    FilterResults results = FILTER_RESULTS_ACCEPT;

    g_return_val_if_fail (token != NULL, FILTER_RESULTS_REJECT);

    if (filter == NULL)
        return FILTER_RESULTS_ACCEPT;

    if (! filter->is_optimized)
        token_filter_optimize (filter);

    if (filter->is_impossible)
        return FILTER_RESULTS_REJECT;

    if (token_is_break (token))
        return filter->break_preference;

    if (token_is_punctuation (token))
        return filter->punctuation_preference;

    syl = token_get_syllables (token);

    /* Zero-syllable tokens that are not break or punctuation
       tend to be garbled crap that the fallback syllable guessing
       routine chokes on.  We just reject it. */
    if (syl == 0)
        return FILTER_RESULTS_REJECT;

    if (filter->min_syllables >= 0 && syl < filter->min_syllables)
        return FILTER_RESULTS_REJECT;

    if (filter->max_syllables >= 0 && syl > filter->max_syllables)
        return FILTER_RESULTS_REJECT;

    if (filter->meter_left || filter->meter_right) {
        
        Meter *meter = token_get_meter (token);

        if (meter == NULL) {
            metric_error = metric_error_unknown (syl);
        } else if (filter->meter_left)
            metric_error = metric_error_left (filter->meter_left, meter);
        else /* if (filter->meter_right) */
            metric_error = metric_error_right (filter->meter_right, meter);

        if (metric_error > filter->metric_error_upper_threshold)
            return FILTER_RESULTS_REJECT;
        else if (metric_error > filter->metric_error_lower_threshold)
            results = FILTER_RESULTS_TOLERATE;
    }

    if (filter->rhymes_with) {
        Phoneme *p1 = token_get_decomp (filter->rhymes_with);
        Phoneme *p2 = token_get_decomp (token);

        if (p1 == NULL || p2 == NULL)
            return FILTER_RESULTS_REJECT;

        rhyme_type = rhyme_get_type (p1, p2);

        if (rhyme_type < filter->rhyme_type_upper_threshold)
            return FILTER_RESULTS_REJECT;

        if (rhyme_type < filter->rhyme_type_lower_threshold)
            return FILTER_RESULTS_TOLERATE;
    }

    if (leading_test_cb 
        && results > filter->leading_preference
        && leading_test_cb (token, user_data))
        results = filter->leading_preference;

    if (trailing_test_cb 
        && results > filter->trailing_preference
        && trailing_test_cb (token, user_data))
        results = filter->trailing_preference;

    return results;
}
