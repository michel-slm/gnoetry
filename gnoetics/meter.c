/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "meter.h"

Meter *
meter_from_phoneme_decomp (Phoneme *p)
{
    Meter *m;
    int i, count = 0;

    g_return_val_if_fail (p != NULL, NULL);

    for (i = 0; p[i]; ++i) {
        if (PHONEME_IS_EXPLICITLY_STRESSED (p[i]))
            count += 1;
    }
   
    m = g_new (Meter, count+1);
    m[count] = '\0';

    for (i = 0; p[i]; ++i) {
        if (PHONEME_IS_EXPLICITLY_STRESSED (p[i])) {
            --count;
            m[count] = PHONEME_IS_STRESSED (p[i]) ? 
                METER_STRESSED : METER_UNSTRESSED;
        }
    }

    return m;
}

gboolean
meter_is_valid (const Meter *a)
{
    g_return_val_if_fail (a != NULL, FALSE);

    while (*a) {
        if (*a != METER_STRESSED && *a != METER_UNSTRESSED
            && *a != METER_UNKNOWN && *a != METER_ANY)
            return FALSE;
        ++a;
    }

    return TRUE;
}

gboolean
metric_match_left (const Meter *a, const Meter *b)
{
    g_return_val_if_fail (a != NULL, FALSE);
    g_return_val_if_fail (b != NULL ,FALSE);

    while (*a && *b) {
        if (! METER_EQ (*a, *b))
            return FALSE;
        ++a;
        ++b;
    }
    return TRUE;
}

gboolean
metric_match_right (const Meter *a, const Meter *b)
{
    int ia, ib;
    ia = strlen (a)-1;
    ib = strlen (b)-1;

    while (ia >= 0 && ib >= 0) {
        if (! METER_EQ (a[ia], b[ib]))
            return FALSE;
        --ia;
        --ib;
    }
    return TRUE;
}

double
metric_error_left (const Meter *a, const Meter *b)
{
    int i;
    double tally = 0;
  
    g_return_val_if_fail (a != NULL, 1000);
    g_return_val_if_fail (b != NULL, 2000);

    for (i = 0; a[i] && b[i]; ++i)
        tally += METER_ERROR (a[i], b[i]);

    return tally;
}

double
metric_error_right (const Meter *a, const Meter *b)
{
    int ia, ib;
    double tally = 0;

    g_return_val_if_fail (a != NULL, 3000);
    g_return_val_if_fail (b != NULL, 4000);

    ia = strlen (a)-1;
    ib = strlen (b)-1;

    while (ia >= 0 && ib >= 0 && a[ia] && b[ia]) {
        tally += METER_ERROR (a[ia], b[ib]);
        --ia;
        --ib;
    }

    return tally;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
py_meter_register (PyObject *dict)
{
    PyObject *x;

    x = PyString_FromFormat ("%c", METER_STRESSED);
    PyDict_SetItemString (dict, "METER_STRESSED", x);
    Py_DECREF (x);

    x = PyString_FromFormat ("%c", METER_UNSTRESSED);
    PyDict_SetItemString (dict, "METER_UNSTRESSED", x);
    Py_DECREF (x);

    x = PyString_FromFormat ("%c", METER_UNKNOWN);
    PyDict_SetItemString (dict, "METER_UNKNOWN", x);
    Py_DECREF (x);

    x = PyString_FromFormat ("%c", METER_ANY);
    PyDict_SetItemString (dict, "METER_ANY", x);
    Py_DECREF (x);
}

PyObject *
py_meter_from_phoneme_decomp (PyObject *self, PyObject *args)
{
    PyObject *py_decomp;
    Phoneme *decomp;
    Meter *m;

    if (! PyArg_ParseTuple (args, "O", &py_decomp))
        return NULL;

    decomp = phoneme_decomp_from_py (py_decomp);
    m = meter_from_phoneme_decomp (decomp);
    g_free (decomp);

    return Py_BuildValue ("s", m);
}

PyObject *
py_meter_is_valid (PyObject *self, PyObject *args)
{
    Meter *a;
    
    if (! PyArg_ParseTuple (args, "s", &a))
        return NULL;

    return PyBool_FromLong (meter_is_valid (a));
}

PyObject *
py_metric_match_left (PyObject *self, PyObject *args)
{
    Meter *a, *b;

    if (! PyArg_ParseTuple (args, "ss", &a, &b))
        return NULL;

    return PyBool_FromLong (metric_match_left (a, b));
}

PyObject *
py_metric_match_right (PyObject *self, PyObject *args)
{
    Meter *a, *b;

    if (! PyArg_ParseTuple (args, "ss", &a, &b))
        return NULL;

    return PyBool_FromLong (metric_match_right (a, b));
}

PyObject *
py_metric_error_left (PyObject *self, PyObject *args)
{
    Meter *a, *b;

    if (! PyArg_ParseTuple (args, "ss", &a, &b))
        return NULL;

    return Py_BuildValue ("f", metric_error_left (a, b));
}

PyObject *
py_metric_error_right (PyObject *self, PyObject *args)
{
    Meter *a, *b;

    if (! PyArg_ParseTuple (args, "ss", &a, &b))
        return NULL;

    return Py_BuildValue ("f", metric_error_right (a, b));
}
