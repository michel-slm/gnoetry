/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __SYLLABLE_H__
#define __SYLLABLE_H__

#include <Python.h>
#include "phoneme.h"

int syllable_count_approximate (const char *str);
int syllable_count_from_decomp (Phoneme *decomp);

/* Python Extensions */

PyObject *py_syllable_count_approximate (PyObject *self, PyObject *args);
PyObject *py_syllable_count_from_decomp (PyObject *self, PyObject *args);

#endif /* __SYLLABLE_H__ */

