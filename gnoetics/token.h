/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __TOKEN_H__
#define __TOKEN_H__

#include <Python.h>
#include <glib.h>

#include "dictionary.h"
#include "meter.h"

typedef struct _Token Token;
typedef gboolean (*TokenFn) (Token *, gpointer user_data);

struct _Token {
    char *raw;
    char *word;
    DictionaryWord *dict;

    int syllables;
    int word_count;
    Meter *meter;

    unsigned is_break       : 1;
    unsigned is_punctuation : 1;
    unsigned is_wildcard    : 1;
    unsigned left_glue      : 1;
    unsigned right_glue     : 1;
};

const char *token_get_raw        (Token *);
const char *token_get_word       (Token *);
gboolean    token_is_break       (Token *);
gboolean    token_is_punctuation (Token *);
gboolean    token_is_wildcard    (Token *);
gboolean    token_in_dictionary  (Token *);
gboolean    token_has_left_glue  (Token *);
gboolean    token_has_right_glue (Token *);
Phoneme    *token_get_decomp     (Token *);
int         token_get_syllables  (Token *);
int         token_get_word_count (Token *);
Meter      *token_get_meter      (Token *);

Token *token_lookup (const char *raw);

Token *token_lookup_break    (void);
Token *token_lookup_wildcard (void);


void py_token_register (PyObject *dict);

PyObject *token_to_py    (Token *token);
Token    *token_from_py  (PyObject *py_token);
int       py_token_check (PyObject *obj);

PyObject *py_token_lookup (PyObject *self, PyObject *args);

#endif /* __TOKEN_H__ */

