/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __METER_H__
#define __METER_H__

#include <Python.h>
#include <glib.h>

#include "phoneme.h"

typedef char Meter;
typedef enum {
    METER_STRESSED = '-',
    METER_UNSTRESSED = 'u',
    METER_UNKNOWN = '?',
    METER_ANY = '*',
} MeterType;

#define METER_UNKNOWN_PENALTY 0.6

#define METER_ERROR(a, b) \
  (((a) == METER_UNKNOWN || (b) == METER_UNKNOWN) ? METER_UNKNOWN_PENALTY : \
    (((a) != (b) && (a) != METER_ANY && (b) != METER_ANY) ? 1.0 : 0.0))

#define METER_EQ(a, b) \
( ((a) == METER_ANY) || ((b) == METER_ANY) \
  || (((a) == (b)) && ((a) != METER_UNKNOWN) && ((b) != METER_UNKNOWN)))

Meter *meter_from_phoneme_decomp (Phoneme *);

gboolean meter_is_valid (const Meter *a);

gboolean metric_match_left  (const Meter *a, const Meter *b);
gboolean metric_match_right (const Meter *a, const Meter *b);

double metric_error_left    (const Meter *a, const Meter *b);
double metric_error_right   (const Meter *a, const Meter *b);
double metric_error_unknown (int num_syllables);

/* Python Extensions */

void py_meter_register (PyObject *);

PyObject *py_meter_from_phoneme_decomp (PyObject *self, PyObject *args);
PyObject *py_meter_is_valid (PyObject *self, PyObject *args);
PyObject *py_metric_match_left (PyObject *self, PyObject *args);
PyObject *py_metric_match_right (PyObject *self, PyObject *args);
PyObject *py_metric_error_left (PyObject *self, PyObject *args);
PyObject *py_metric_error_right (PyObject *self, PyObject *args);


#endif /* __METER_H__ */

