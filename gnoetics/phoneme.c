/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "phoneme.h"

#include <Python.h>
#include <string.h>
#include <ctype.h>

const char *
phoneme_code_to_string (PhonemeCode code)
{
    code = PHONEME_TO_CODE (code);
    switch (code) {
    case PHONEME_AA: return "AA";
    case PHONEME_AE: return "AE";
    case PHONEME_AH: return "AH";
    case PHONEME_AO: return "AO";
    case PHONEME_AW: return "AW";
    case PHONEME_AY: return "AY";
    case PHONEME_EH: return "EH";
    case PHONEME_ER: return "ER";
    case PHONEME_EY: return "EY";
    case PHONEME_IH: return "IH";
    case PHONEME_IY: return "IY";
    case PHONEME_OW: return "OW";
    case PHONEME_OY: return "OY";
    case PHONEME_UH: return "UH";
    case PHONEME_UW: return "UW";

    case PHONEME_B : return "B";
    case PHONEME_CH: return "CH";
    case PHONEME_D : return "D";
    case PHONEME_DH: return "DH";
    case PHONEME_F : return "F";
    case PHONEME_G : return "G";
    case PHONEME_HH: return "HH";
    case PHONEME_JH: return "JH";
    case PHONEME_K : return "K";
    case PHONEME_L : return "L";
    case PHONEME_M : return "M";
    case PHONEME_N : return "N";
    case PHONEME_NG: return "NG";
    case PHONEME_P : return "P";
    case PHONEME_R : return "R";
    case PHONEME_S : return "S";
    case PHONEME_SH: return "SH";
    case PHONEME_T : return "T";
    case PHONEME_TH: return "TH";
    case PHONEME_V : return "V";
    case PHONEME_W : return "W";
    case PHONEME_Y : return "Y";
    case PHONEME_Z : return "Z";
    case PHONEME_ZH: return "ZH";

    default:
        return "??";
    }
}

PhonemeCode
phoneme_code_from_string (const char *str, int *len)
{
    if (! (str && *str)) {
        if (len) *len = 0;
        return PHONEME_INVALID;
    }

    switch (str[0]) {
    case 'A':
        if (str[1] == 'A') {
            if (len) *len = 2;
            return PHONEME_AA;
        } else if (str[1] == 'E') {
            if (len) *len = 2;
            return PHONEME_AE;
        } else if (str[1] == 'H') {
            if (len) *len = 2;
            return PHONEME_AH;
        } else if (str[1] == 'O') {
            if (len) *len = 2;
            return PHONEME_AO;
        } else if (str[1] == 'W') {
            if (len) *len = 2;
            return PHONEME_AW;
        } else if (str[1] == 'Y') {
            if (len) *len = 2;
            return PHONEME_AY;
        }
        break;

    case 'E':
        if (str[1] == 'H') {
            if (len) *len = 2;
            return PHONEME_EH;
        } else if (str[1] == 'R') {
            if (len) *len = 2;
            return PHONEME_ER;
        } else if (str[1] == 'Y') {
            if (len) *len = 2;
            return PHONEME_EY;
        }
        break;

    case 'I':
        if (str[1] == 'H') {
            if (len) *len = 2;
            return PHONEME_IH;
        } else if (str[1] == 'Y') {
            if (len) *len = 2;
            return PHONEME_IY;
        }
        break;

    case 'O':
        if (str[1] == 'W') {
            if (len) *len = 2;
            return PHONEME_OW;
        } else if (str[1] == 'Y') {
            if (len) *len = 2;
            return PHONEME_OY;
        }
        break;

    case 'U':
        if (str[1] == 'H') {
            if (len) *len = 2;
            return PHONEME_UH;
        } else if (str[1] == 'W') {
            if (len) *len = 2;
            return PHONEME_UW;
        }
        break;

    case 'B':
        if (len) *len = 1;
        return PHONEME_B;

    case 'C':
        if (str[1] == 'H') {
            if (len) *len = 2;
            return PHONEME_CH;
        }
        break;

    case 'D':
        if (str[1] == 'H') {
            if (len) *len = 2;
            return PHONEME_DH;
        }
        if (len) *len = 1;
        return PHONEME_D;

    case 'F':
        if (len) *len = 1;
        return PHONEME_F;

    case 'G':
        if (len) *len = 1;
        return PHONEME_G;

    case 'H':
        if (str[1] == 'H') {
            if (len) *len = 2;
            return PHONEME_HH;
        }
        break;

    case 'J':
        if (str[1] == 'H') {
            if (len) *len = 2;
            return PHONEME_JH;
        }
        break;

    case 'K':
        if (len) *len = 1;
        return PHONEME_K;

    case 'L':
        if (len) *len = 1;
        return PHONEME_L;

    case 'M':
        if (len) *len = 1;
        return PHONEME_M;

    case 'N':
        if (str[1] == 'G') {
            if (len) *len = 2;
            return PHONEME_NG;
        }
        if (len) *len = 1;
        return PHONEME_N;

    case 'P':
        if (len) *len = 1;
        return PHONEME_P;

    case 'R':
        if (len) *len = 1;
        return PHONEME_R;

    case 'S':
        if (str[1] == 'H') {
            if (len) *len = 2;
            return PHONEME_SH;
        }
        if (len) *len = 1;
        return PHONEME_S;

    case 'T':
        if (str[1] == 'H') {
            if (len) *len = 2;
            return PHONEME_TH;
        }
        if (len) *len = 1;
        return PHONEME_T;

    case 'V':
        if (len) *len = 1;
        return PHONEME_V;

    case 'W':
        if (len) *len = 1;
        return PHONEME_W;

    case 'Y':
        if (len) *len = 1;
        return PHONEME_P;

    case 'Z':
        if (str[1] == 'H') {
            if (len) *len = 2;
            return PHONEME_ZH;
        }
        if (len) *len = 1;
        return PHONEME_Z;
    }

    if (len) *len = 0;
    return PHONEME_INVALID;
}

const char *
phoneme_stress_to_string (PhonemeStress stress)
{
    stress = PHONEME_TO_STRESS (stress);

    switch (stress) {

    case PHONEME_EMPTY_STRESS:     return "";
    case PHONEME_NO_STRESS:        return "0";
    case PHONEME_PRIMARY_STRESS:   return "1";
    case PHONEME_SECONDARY_STRESS: return "2";

    default:
        return "?";
    }
}

PhonemeStress
phoneme_stress_from_string (const char *str, int *len)
{
    if ( !(str && *str)) {
        if (len) *len = 0;
        return PHONEME_EMPTY_STRESS;
    }

    switch (str[0]) {
    case '0':
        if (len) *len = 1;
        return PHONEME_NO_STRESS;
    
    case '1':
        if (len) *len = 1;
        return PHONEME_PRIMARY_STRESS;
    
    case '2':
        if (len) *len = 1;
        return PHONEME_SECONDARY_STRESS;
    }

    if (len) *len = 0;
    return PHONEME_EMPTY_STRESS;
}

char *
phoneme_to_string (Phoneme p)
{
    return g_strdup_printf("%s%s",
                           phoneme_code_to_string (p),
                           phoneme_stress_to_string (p));
}

Phoneme
phoneme_from_string (const char *str, int *len)
{
    int code, stress=0;
    int L;

    if (len) *len = 0;
    code = phoneme_code_from_string (str, &L);
    if (code) {
        if (len) *len += L;
        stress = phoneme_stress_from_string (str+L, &L);
        if (len) *len += L;
    }
    return PHONEME_JOIN (code, stress);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

int
phoneme_decomp_length (Phoneme *decomp)
{
    int count = 0;
    if (decomp) {
        while (decomp[count])
            ++count;
    }
    return count;
}

char *
phoneme_decomp_to_string (Phoneme *decomp)
{
    int i, j, len;
    const char *tmp;
    char *buffer;

    g_return_val_if_fail (decomp != NULL, NULL);

    len = 0;
    for (i = 0; decomp[i]; ++i) {
        len += strlen (phoneme_code_to_string (decomp[i]));
        len += strlen (phoneme_stress_to_string (decomp[i]));
        ++len;
    }

    buffer = g_new (char, len);
    --i;
    j = 0;
    while (i >= 0) {
        tmp = phoneme_code_to_string (decomp[i]);
        strcpy (buffer+j, tmp);
        j += strlen (tmp);
        tmp = phoneme_stress_to_string (decomp[i]);
        strcpy (buffer+j, tmp);
        j += strlen (tmp);
        if (i > 0) {
            strcpy (buffer+j, " ");
            ++j;
        }
        --i;
    }
    buffer[j] = '\0';

    return buffer;
}

Phoneme *
phoneme_decomp_from_string (const char *str)
{
    GSList *plist = NULL, *iter;
    int i, len;
    Phoneme phon;
    Phoneme *decomp;

    i = 0;
    while (str[i]) {
        phon = phoneme_from_string (str+i, &len);
        i += len;
        if (len == 0) {
            phon = PHONEME_INVALID;
        }
        if (str[i] && ! isspace (str[i])) {
            phon = PHONEME_INVALID;
            while (str[i] && ! isspace (str[i]))
                ++i;
        }
        while (str[i] && isspace (str[i]))
            ++i;

        plist = g_slist_prepend (plist, GINT_TO_POINTER ((int) phon));
    }

    len = g_slist_length (plist);
    decomp = g_new (Phoneme, len+1);
    decomp[len] = 0;

    i = 0;
    for (iter = plist; iter != NULL; iter = iter->next) {
        phon = GPOINTER_TO_INT (iter->data);
        decomp[i] = phon;
        ++i;
    }

    g_slist_free (plist);

    return decomp;
}

PyObject *
phoneme_decomp_to_py (Phoneme *decomp)
{
    int i, len;
    PyObject *py_decomp, *py_phon;

    if (decomp == NULL) {
        Py_INCREF (Py_None);
        return Py_None;
    }

    for (len = 0; decomp[len]; ++len);
    py_decomp = PyTuple_New (len);

    for (i = 0; i < len; ++i) {
        py_phon = Py_BuildValue ("i", decomp[i]);
        PyTuple_SET_ITEM (py_decomp, len-1-i, py_phon);
    }

    return py_decomp;
}

Phoneme *
phoneme_decomp_from_py (PyObject *py_decomp)
{
    int i, len;
    Phoneme phon;
    Phoneme *decomp;

    g_return_val_if_fail (py_decomp != NULL, NULL);

    len = PySequence_Size (py_decomp);
    decomp = g_new (Phoneme, len+1);

    for (i = 0; i < len; ++i) {
        PyObject *py_phon = PySequence_GetItem (py_decomp, i);
        if (PyInt_Check (py_phon))
            phon = PyInt_AS_LONG (py_phon);
        else
            phon = PHONEME_INVALID;
        decomp[len-1-i] = phon;
    }
    decomp[len] = 0;
    return decomp;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PyObject *
py_phoneme_code (PyObject *self, PyObject *args)
{
    int phon;

    if (! PyArg_ParseTuple (args, "i", &phon))
        return NULL;

    return Py_BuildValue ("i", PHONEME_TO_CODE (phon));
}

PyObject *
py_phoneme_code_to_string (PyObject *self, PyObject *args)
{
    int phon;

    if (! PyArg_ParseTuple (args, "i", &phon))
        return NULL;

    return Py_BuildValue ("s", phoneme_code_to_string(PHONEME_TO_CODE (phon)));
}

PyObject *
py_phoneme_stress (PyObject *self, PyObject *args)
{
    int phon;

    if (! PyArg_ParseTuple (args, "i", &phon))
        return NULL;

    return Py_BuildValue ("i", PHONEME_TO_STRESS (phon));
}

PyObject *
py_phoneme_stress_to_string (PyObject *self, PyObject *args)
{
    int phon;

    if (! PyArg_ParseTuple (args, "i", &phon))
        return NULL;

    return Py_BuildValue ("s",
                          phoneme_stress_to_string(PHONEME_TO_STRESS (phon)));
}

PyObject *
py_phoneme_split (PyObject *self, PyObject *args)
{
    int phon;

    if (! PyArg_ParseTuple (args, "i", &phon))
        return NULL;

    return Py_BuildValue ("(ii)",
                          PHONEME_TO_CODE (phon),
                          PHONEME_TO_STRESS (phon));
}

PyObject *
py_phoneme_split_to_string (PyObject *self, PyObject *args)
{
    int phon;

    if (! PyArg_ParseTuple (args, "i", &phon))
        return NULL;

    return Py_BuildValue ("(ss)",
                          phoneme_code_to_string (PHONEME_TO_CODE (phon)),
                          phoneme_stress_to_string (PHONEME_TO_STRESS (phon)));
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PyObject *
py_phoneme_is_vowel (PyObject *self, PyObject *args)
{
    int phon;

    if (! PyArg_ParseTuple (args, "i", &phon))
        return NULL;

    return Py_BuildValue ("i", PHONEME_IS_VOWEL (phon));
}

PyObject *
py_phoneme_is_consonant (PyObject *self, PyObject *args)
{
    int phon;

    if (! PyArg_ParseTuple (args, "i", &phon))
        return NULL;

    return Py_BuildValue ("i", PHONEME_IS_CONSONANT (phon));
}

PyObject *
py_phoneme_is_stressed (PyObject *self, PyObject *args)
{
    int phon;

    if (! PyArg_ParseTuple (args, "i", &phon))
        return NULL;

    return Py_BuildValue ("i", PHONEME_IS_STRESSED (phon));
}

PyObject *
py_phoneme_is_xstressed (PyObject *self, PyObject *args)
{
    int phon;

    if (! PyArg_ParseTuple (args, "i", &phon))
        return NULL;

    return Py_BuildValue ("i", PHONEME_IS_EXPLICITLY_STRESSED (phon));
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PyObject *
py_phoneme_to_string (PyObject *self, PyObject *args)
{
    int phon, i;
    const char *tmp;
    char buffer[4];

    if (! PyArg_ParseTuple (args, "i", &phon))
        return NULL;

    i = 0;
    tmp = phoneme_code_to_string (PHONEME_TO_CODE (phon));
    strcpy (buffer, tmp);
    i += strlen (tmp);

    tmp = phoneme_stress_to_string (PHONEME_TO_STRESS (phon));
    strcpy (buffer+i, tmp);
    i += strlen (tmp);

    buffer[i] = '\0';
  
    return Py_BuildValue ("s", buffer);
}

PyObject *
py_phoneme_from_string (PyObject *self, PyObject *args)
{
    char *str;
    int phon;
  
    if (! PyArg_ParseTuple (args, "s", &str))
        return NULL;

    phon = phoneme_from_string (str, NULL);

    return Py_BuildValue ("i", phon);
}

PyObject *
py_phoneme_decomp_to_string (PyObject *self, PyObject *args)
{
    PyObject *py_decomp, *py_str;
    Phoneme *decomp;
    char *str;

    if (! PyArg_ParseTuple (args, "O", &py_decomp))
        return NULL;

    decomp = phoneme_decomp_from_py (py_decomp);
    str = phoneme_decomp_to_string (decomp);
    py_str = PyString_FromString (str);

    g_free (decomp);
    g_free (str);

    return py_str;
}

PyObject *
py_phoneme_decomp_from_string (PyObject *self, PyObject *args)
{
    char *str;
    Phoneme *decomp;
    PyObject *py_decomp;

    if (! PyArg_ParseTuple (args, "s", &str))
        return NULL;

    decomp = phoneme_decomp_from_string (str);
    py_decomp = phoneme_decomp_to_py (decomp);

    g_free (decomp);

    return py_decomp;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PyObject *
py_phoneme_equal_mod_stress (PyObject *self, PyObject *args)
{
    int p1, p2;

    if (! PyArg_ParseTuple (args, "ii", &p1, &p2))
        return NULL;

    return Py_BuildValue ("i", PHONEME_EQ_MOD_STRESS (p1, p2));
}

PyObject *
py_phoneme_equal_mod_slant (PyObject *self, PyObject *args)
{
    int p1, p2;

    if (! PyArg_ParseTuple (args, "ii", &p1, &p2))
        return NULL;

    return Py_BuildValue ("i", PHONEME_EQ_MOD_SLANT (p1, p2));
}

