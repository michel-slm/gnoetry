/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "tokenfilter.h"

void
token_filter_init (TokenFilter *filter)
{
    g_return_if_fail (filter != NULL);

    filter->in_dictionary = FILTER_NEUTRAL;
    filter->is_punctuation = FILTER_NEUTRAL;

    filter->allow_break = TRUE;

    filter->min_syllables = -1;
    filter->max_syllables = -1;

    filter->meter_left = NULL;
    filter->meter_right = NULL;
    filter->max_metric_error = 0;

    filter->rhymes_with = NULL;
    filter->min_rhyme_type = RHYME_FALSE;

    filter->impossible = FALSE;
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
    if (py_val == NULL)
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
    if (py_val == NULL)
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
    if (py_val == NULL)
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
    g_return_if_fail (filter != NULL);
    
    token_filter_init (filter);

    if (py_dict == NULL)
        return;

    py_dict_get_int (py_dict, "in_dictionary", &filter->in_dictionary);
    
    py_dict_get_int (py_dict, "is_punctuation", &filter->is_punctuation);

    py_dict_get_int (py_dict, "allow_break", &filter->allow_break);

    py_dict_get_int (py_dict, "min_syllables", &filter->min_syllables);

    py_dict_get_int (py_dict, "max_syllables", &filter->max_syllables);

    py_dict_get_string (py_dict, "meter_left", &filter->meter_left);

    py_dict_get_string (py_dict, "meter_right", &filter->meter_right);

    py_dict_get_double (py_dict, "max_metric_error",
                        &filter->max_metric_error);

    if (PyMapping_HasKeyString (py_dict, "rhymes_with")) {
        PyObject *py_val = PyMapping_GetItemString (py_dict, "rhymes_with");
        if (py_token_check (py_val)) {
            filter->rhymes_with = token_from_py (py_val);
        } else if (PyString_Check (py_val)) {
            filter->rhymes_with = token_lookup (PyString_AsString (py_val));
        } else {
            g_warning ("Unknown type for 'rhymes_with'");
        }
        Py_DECREF (py_val);
    }
}

void
token_filter_optimize (TokenFilter *filter)
{
    g_return_if_fail (filter != NULL);

    if (filter->meter_left) {
        int len = strlen (filter->meter_left);
        if (len > filter->min_syllables)
            filter->min_syllables = len;
    }

    if (filter->meter_right) {
        int len = strlen (filter->meter_right);
        if (len > filter->min_syllables)
            filter->min_syllables = len;
    }

    if (filter->rhymes_with != NULL
        && token_get_decomp (filter->rhymes_with) == NULL) {
        filter->impossible = TRUE;
        return;
    }

    if (filter->min_rhyme_type == RHYME_NONE)
        filter->rhymes_with = NULL;

    if (filter->max_syllables > 0
        && filter->min_syllables > filter->max_syllables) {
        filter->impossible = TRUE;
        return;
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

gboolean
token_filter_test (TokenFilter *filter,
                   Token       *token)
{
    g_return_val_if_fail (token != NULL, FALSE);
    if (filter == NULL)
        return TRUE;

    if (filter->impossible)
        return FALSE; 

    if (! filter_test (filter->in_dictionary, token_in_dictionary (token)))
        return FALSE;

    if (! filter_test (filter->is_punctuation, token_is_punctuation (token)))
        return FALSE;

    if (token_is_break (token) && !filter->allow_break)
        return FALSE;

    /* Breaks and punctuation are exempted from syllable limits. */
    if (! (token_is_break (token) || token_is_punctuation (token))) {
        if (filter->min_syllables >= 0
            && token_get_syllables (token) < filter->min_syllables)
            return FALSE;

        if (filter->max_syllables >= 0
            && token_get_syllables (token) > filter->max_syllables)
            return FALSE;
    }

    if ((filter->meter_left || filter->meter_right)
        && ! token_is_break (token)
        && ! token_is_punctuation (token)) {
        Meter *meter;

        meter = token_get_meter (token);
        if (meter == NULL)
            return FALSE;

        if (filter->max_metric_error > 0) {
            if (filter->meter_left
                && metric_error_left (filter->meter_left,
                                      meter) > filter->max_metric_error)
                return FALSE;
            if (filter->meter_right
                && metric_error_right (filter->meter_right,
                                       meter) > filter->max_metric_error)
                return FALSE;
        } else {
            if (filter->meter_left
                && ! metric_match_left (filter->meter_left, meter))
                return FALSE;
            if (filter->meter_right
                && ! metric_match_right (filter->meter_right, meter))
                return FALSE;
        }
    }

    if (filter->rhymes_with != NULL) {

        Phoneme *p1 = token_get_decomp (filter->rhymes_with);
        Phoneme *p2 = token_get_decomp (token);
        RhymeType type;

        if (p1 == NULL || p2 == NULL)
            return FALSE;

        type = rhyme_get_type (p1, p2);

        if (type < filter->min_rhyme_type)
            return FALSE;
    }

    return TRUE;
}
