/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "rhyme.h"

RhymeType
rhyme_get_type (Phoneme *decomp1, Phoneme *decomp2)
{
    int i1, i2;
    RhymeType type;
    Phoneme p1, p2;
    gboolean s1, s2;

    g_return_val_if_fail (decomp1 != NULL, RHYME_NONE);
    g_return_val_if_fail (decomp2 != NULL, RHYME_NONE);

    i1 = 0;
    i2 = 0;
    type = RHYME_TRUE;

    while (decomp1[i1] && decomp2[i2]) {

        p1 = decomp1[i1];
        p2 = decomp2[i2];

        if (type == RHYME_TRUE && ! PHONEME_EQ_MOD_STRESS (p1, p2))
            type = RHYME_SLANT;

        if (type < RHYME_TRUE && ! PHONEME_EQ_MOD_SLANT (p1, p2))
            return RHYME_NONE;

        s1 = PHONEME_IS_STRESSED (p1);
        s2 = PHONEME_IS_STRESSED (p2);

        if (PHONEME_IS_VOWEL (p1) && (s1 || s2)) {

            if (type == RHYME_SLANT)
                return type;

            if (s1 && s2) {
                ++i1;
                ++i2;
                p1 = decomp1[i1];
                p2 = decomp2[i2];
                /* In true rhyme the phonemes before the stressed vowel
                   must differ. */
                if (p1 && p2 && ! PHONEME_EQ_MOD_STRESS (p1, p2))
                    return RHYME_TRUE;

                return RHYME_FALSE;
            }

            /* If one phoneme isn't stressed, we report this to be no rhyme
               at all.  This might not be the right thing to do. */
            return RHYME_NONE;
        }
    
        ++i1;
        ++i2;
    }

    if (type == RHYME_TRUE) {
    
        /* a word never rhymes with itself */
        if (decomp1[i1] == 0 && decomp2[i2] == 0)
            type = RHYME_NONE;

        type = RHYME_FALSE;
    }

    return type;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct RhymeForeachInfo {
    Phoneme  *decomp;
    RhymeType minimum;
    RhymeFn   fn;
    gpointer  user_data;
};

static gboolean
rhyme_foreach_cb (DictionaryWord *dword, gpointer user_data)
{
    struct RhymeForeachInfo *info = user_data;
    RhymeType rt;

    rt = rhyme_get_type (dword->decomp, info->decomp);
    if (rt && rt >= info->minimum)
        return info->fn (info->decomp, dword, rt, info->user_data);

    return TRUE;
}

void
rhyme_foreach (Phoneme  *decomp,
               RhymeType minimum,
               RhymeFn   fn,
               gpointer  user_data)
{
    struct RhymeForeachInfo info;

    g_return_if_fail (decomp != NULL);
    g_return_if_fail (fn != NULL);

    info.decomp    = decomp;
    info.minimum   = minimum;
    info.fn        = fn;
    info.user_data = user_data;

    dictionary_foreach_by_tail (decomp, rhyme_foreach_cb, &info);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct RhymeExistsInfo {
    RhymeType minimum;
    gboolean flag;
};

gboolean
rhyme_exists_cb (Phoneme *decomp,
                 DictionaryWord *dword,
                 RhymeType type,
                 gpointer user_data)
{
    struct RhymeExistsInfo *info = user_data;
  
    if (!info->flag && type >= info->minimum) {
        info->flag = TRUE;
        return FALSE;
    }

    return TRUE;
}

gboolean
rhyme_exists (Phoneme *decomp, RhymeType minimum)
{
    struct RhymeExistsInfo info;

    info.minimum = minimum;
    info.flag    = FALSE;

    rhyme_foreach (decomp, minimum, rhyme_exists_cb, &info);

    return info.flag;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
py_rhyme_register (PyObject *dict)
{
    PyObject *x;

    x = PyInt_FromLong (RHYME_NONE);
    PyDict_SetItemString (dict, "RHYME_NONE", x);
    Py_DECREF (x);

    x = PyInt_FromLong (RHYME_SLANT);
    PyDict_SetItemString (dict, "RHYME_SLANT", x);
    Py_DECREF (x);

    x = PyInt_FromLong (RHYME_FALSE);
    PyDict_SetItemString (dict, "RHYME_FALSE", x);
    Py_DECREF (x);

    x = PyInt_FromLong (RHYME_TRUE);
    PyDict_SetItemString (dict, "RHYME_TRUE", x);
    Py_DECREF (x);
}

Phoneme *
maybe_lookup_decomp (PyObject *py_word, gboolean *own_decomp)
{
    if (PyString_Check (py_word)) {
        DictionaryWord *dword;
        *own_decomp = FALSE;
        dword = dictionary_get_word (PyString_AS_STRING (py_word));
        return dword ? dword->decomp : NULL;
    } else {
        *own_decomp = TRUE;
        return phoneme_decomp_from_py (py_word);
    }
}

PyObject *
py_rhyme_get_type (PyObject *self, PyObject *args)
{
    PyObject *py_word1;
    PyObject *py_word2;
    Phoneme *decomp1 = NULL;
    Phoneme *decomp2 = NULL;
    gboolean own_decomp1 = FALSE, own_decomp2 = FALSE;
    PyObject *retval = NULL;
    RhymeType rt;

    if (! PyArg_ParseTuple (args, "OO", &py_word1, &py_word2)) {
        return NULL;
    }

    decomp1 = maybe_lookup_decomp (py_word1, &own_decomp1);
    decomp2 = maybe_lookup_decomp (py_word2, &own_decomp2);

    if (decomp1 == NULL || decomp2 == NULL) {
        /* FIXME: set error */
        goto done;
    }

    rt = rhyme_get_type (decomp1, decomp2);

    retval = Py_BuildValue ("i", rt);

 done:
    if (own_decomp1)
        g_free (decomp1);
    if (own_decomp2)
        g_free (decomp2);

    return retval;
}

static gboolean
py_rhyme_get_all_cb (Phoneme        *decomp,
                     DictionaryWord *dword,
                     RhymeType       type,
                     gpointer        user_data)
{
    PyObject *py_list = user_data;
    PyObject *py_dword = dictionary_word_to_py (dword);
    PyList_Append (py_list, py_dword);

    return TRUE;
}

PyObject *
py_rhyme_get_all (PyObject *self, PyObject *args)
{
    PyObject *py_word;
    Phoneme *decomp;
    gboolean own_decomp;
    PyObject *py_list;
    RhymeType minimum = 0;

    if (! PyArg_ParseTuple (args, "O|i", &py_word, &minimum))
        return NULL;

    decomp = maybe_lookup_decomp (py_word, &own_decomp);
    py_list = PyList_New (0);

    if (decomp) 
        rhyme_foreach (decomp, minimum, py_rhyme_get_all_cb, py_list);

    if (own_decomp)
        g_free (decomp);

    return py_list;
}
