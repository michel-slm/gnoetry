/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#include <Python.h>
#include <glib.h>

#include "phoneme.h"
#include "meter.h"

typedef struct _DictionaryWord DictionaryWord;
struct _DictionaryWord {
    char    *word;
    Phoneme *decomp;
    long     hash;
};

typedef void (*DictionaryFn) (DictionaryWord *, gpointer);

DictionaryWord *dictionary_add_word   (const char      *word,
                                       Phoneme         *decomp);

DictionaryWord *dictionary_get_word   (const char *word);

Phoneme        *dictionary_get_decomp (const char *phrase);

void            dictionary_load_pronunciation  (const char *filename);

void            dictionary_foreach_by_tail (Phoneme *decomp,
                                            DictionaryFn fn,
                                            gpointer user_data);

void            dictionary_init (void);

PyObject       *dictionary_word_to_py   (DictionaryWord *dword);
DictionaryWord *dictionary_word_from_py (PyObject *py_dword);

/* Python Extensions */

PyObject *py_dictionary_load   (PyObject *self, PyObject *args);

PyObject *py_dictionary_lookup (PyObject *self, PyObject *args);

#endif /* __DICTIONARY_H__ */

