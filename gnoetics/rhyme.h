/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __RHYME_H__
#define __RHYME_H__

#include <Python.h>
#include "phoneme.h"
#include "dictionary.h"

typedef enum {
    RHYME_NONE    = 0,
    RHYME_SLANT   = 1,
    RHYME_FALSE   = 2,
    RHYME_TRUE    = 3,
} RhymeType;

typedef gboolean (*RhymeFn) (Phoneme        *decomp,
                             DictionaryWord *word,
                             RhymeType       type,
                             gpointer        user_data);

RhymeType rhyme_get_type (Phoneme *decomp1, Phoneme *decomp2);

void      rhyme_foreach (Phoneme  *decomp,
                         RhymeType minimum,
                         RhymeFn   fn,
                         gpointer  user_data);

gboolean   rhyme_exists (Phoneme *decomp, RhymeType minimum);


/* Python Extenions */

void py_rhyme_register (PyObject *);

PyObject *py_rhyme_get_type (PyObject *self, PyObject *args);
PyObject *py_rhyme_get_all  (PyObject *self, PyObject *args);


#endif /* __RHYME_H__ */

