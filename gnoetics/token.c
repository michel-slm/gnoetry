/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "token.h"

#include "pyutil.h"
#include "syllable.h"

const char *
token_get_raw (Token *t)
{
    g_return_val_if_fail (t != NULL, NULL);
    return t->raw;
}

const char *
token_get_word (Token *t)
{
    g_return_val_if_fail (t != NULL, NULL);
    if (token_is_break (t))
        return "\n";
    return t->word;
}

gboolean
token_is_break (Token *t)
{
    g_return_val_if_fail (t != NULL, FALSE);
    return t->is_break;
}

gboolean
token_is_punctuation (Token *t)
{
    g_return_val_if_fail (t != NULL, FALSE);
    return t->is_punctuation;
}

gboolean
token_is_wildcard (Token *t)
{
    g_return_val_if_fail (t != NULL, FALSE);
    return t->is_wildcard;
}

gboolean
token_in_dictionary (Token *t)
{
    g_return_val_if_fail (t != NULL, FALSE);
    return t->dict != NULL;
}

gboolean
token_has_left_glue (Token *t)
{
    g_return_val_if_fail (t != NULL, FALSE);
    return t->left_glue;
}

gboolean
token_has_right_glue (Token *t)
{
    g_return_val_if_fail (t != NULL, FALSE);
    return t->right_glue;
}

Phoneme *
token_get_decomp (Token *t)
{
    g_return_val_if_fail (t != NULL, NULL);
    return t->dict ? t->dict->decomp : NULL;
}

int
token_get_syllables (Token *t)
{
    g_return_val_if_fail (t != NULL, -1);
    return t->syllables;
}

int
token_get_word_count (Token *t)
{
    g_return_val_if_fail (t != NULL, -1);
    return t->word_count;
}

Meter *
token_get_meter (Token *t)
{
    g_return_val_if_fail (t != NULL, NULL);
    return t->meter;
}


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
str_word_count (const char *str)
{
  gboolean is_in_word = FALSE;
  int count = 0;

  while (*str) {
    if (isspace (*str)) {
      is_in_word = FALSE;
    } else if (! is_in_word) {
      is_in_word = TRUE;
      ++count;
    }
    ++str;
  }

  return count;
}

static GHashTable  *token_table = NULL;
static GStaticMutex token_mutex = G_STATIC_MUTEX_INIT;

Token *
token_lookup (const char *raw)
{
    Token *tok;

    g_static_mutex_lock (&token_mutex);

    if (token_table == NULL)
        token_table = g_hash_table_new (g_str_hash, g_str_equal);

    tok = g_hash_table_lookup (token_table, raw);
    if (tok == NULL) {

        static GMemChunk *token_chunks = NULL;

        if (token_chunks == NULL)
            token_chunks = g_mem_chunk_create (Token, 1000, G_ALLOC_ONLY);


        tok = g_chunk_new0 (Token, token_chunks);
        tok->raw = g_strdup (raw);
        tok->syllables = -1;
        tok->word_count = -1;

        if (! strcmp (raw, "*break*")) {
            tok->is_break = TRUE;
        } else if (! strncmp (raw, "*punct*", 7)) {
            tok->word = g_strdup (raw+8);
            tok->is_punctuation = TRUE;
            tok->left_glue = TRUE; /* FIXME */
            tok->syllables = 0;
            tok->word_count = 0;
        } else if (! strcmp (raw, "*wildcard*")) {
            tok->word = g_strdup (raw);
            tok->is_wildcard = TRUE;
        } else {
            DictionaryWord *dword;
            tok->word = g_strdup (raw);
            dword = dictionary_get_word (tok->word);
            if (dword != NULL) {
                tok->dict = dword;
                tok->syllables = syllable_count_from_decomp (dword->decomp);
                tok->meter = meter_from_phoneme_decomp (dword->decomp);
            } else {
                tok->syllables = syllable_count_approximate (tok->word);
            }
            tok->word_count = str_word_count (tok->word);
        }

        g_hash_table_insert (token_table, tok->raw, tok);
    }

    g_static_mutex_unlock (&token_mutex);
    
    return tok;
}

Token *
token_lookup_break (void)
{
    static Token *brk = NULL;
    if (brk == NULL)
        brk = token_lookup ("*break*");
    return brk;
}

Token *
token_lookup_wildcard (void)
{
    static Token *wild = NULL;
    if (wild == NULL)
        wild = token_lookup ("*wildcard*");
    return wild;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/* Python Type Magic */

typedef struct _PyToken PyToken;
struct _PyToken {
    PyObject_HEAD;
    Token *token;
};

static PyObject *
py_token_get_raw (PyObject *self, PyObject *args)
{
    Token *token = token_from_py (self);
    return PyString_FromString (token_get_raw (token));
}

static PyObject *
py_token_get_word (PyObject *self, PyObject *args)
{
    Token *token = token_from_py (self);
    return PyString_FromString (token_get_word (token));
}

static PyObject *
py_token_is_break (PyObject *self, PyObject *args)
{
    Token *token = token_from_py (self);
    return Py_BuildValue ("i", token_is_break (token));
}

static PyObject *
py_token_is_punctuation (PyObject *self, PyObject *args)
{
    Token *token = token_from_py (self);
    return Py_BuildValue ("i", token_is_punctuation (token));
}

static PyObject *
py_token_is_wildcard (PyObject *self, PyObject *args)
{
    Token *token = token_from_py (self);
    return Py_BuildValue ("i", token_is_wildcard (token));
}

static PyObject *
py_token_in_dictionary (PyObject *self, PyObject *args)
{
    Token *token = token_from_py (self);
    return Py_BuildValue ("i", token_in_dictionary (token));
}

static PyObject *
py_token_has_left_glue (PyObject *self, PyObject *args)
{
    Token *token = token_from_py (self);
    return Py_BuildValue ("i", token_has_left_glue (token));
}

static PyObject *
py_token_has_right_glue (PyObject *self, PyObject *args)
{
    Token *token = token_from_py (self);
    return Py_BuildValue ("i", token_has_right_glue (token));
}

static PyObject *
py_token_get_decomp (PyObject *self, PyObject *args)
{
    Token *token = token_from_py (self);
    return phoneme_decomp_to_py (token_get_decomp (token));
}

static PyObject *
py_token_get_syllables (PyObject *self, PyObject *args)
{
    Token *token = token_from_py (self);
    return Py_BuildValue ("i", token_get_syllables (token));
}

static PyObject *
py_token_get_word_count (PyObject *self, PyObject *args)
{
    Token *token = token_from_py (self);
    return Py_BuildValue ("i", token_get_word_count (token));
}

static PyObject *
py_token_get_meter (PyObject *self, PyObject *args)
{
    Token *token = token_from_py (self);
    Meter *m = token_get_meter (token);
    if (m == NULL) {
        Py_INCREF (Py_None);
        return Py_None;
    }
    return PyString_FromString (m);
}


static PyMethodDef py_token_methods[] = {
    { "get_raw",        py_token_get_raw,        METH_NOARGS },
    { "get_word",       py_token_get_word,       METH_NOARGS },
    { "is_break",       py_token_is_break,       METH_NOARGS },
    { "is_punctuation", py_token_is_punctuation, METH_NOARGS },
    { "is_wildcard",    py_token_is_wildcard,    METH_NOARGS },
    { "in_dictionary",  py_token_in_dictionary,  METH_NOARGS },
    { "has_left_glue",  py_token_has_left_glue,  METH_NOARGS },
    { "has_right_glue", py_token_has_right_glue, METH_NOARGS },
    { "get_decomp",     py_token_get_decomp,     METH_NOARGS },
    { "get_syllables",  py_token_get_syllables,  METH_NOARGS },
    { "get_word_count", py_token_get_word_count, METH_NOARGS },
    { "get_meter",      py_token_get_meter,      METH_NOARGS },

    { NULL, NULL, 0 }
};

static PyObject *
py_token_str (PyObject *self)
{
    Token *token = token_from_py (self);
    return PyString_FromFormat ("<Token '%s'>", token->raw);
}

static PyTypeObject py_token_type_info = {
    PyObject_HEAD_INIT(NULL)
    0,
    "Token",
    sizeof (PyToken),
    0
};

void
py_token_register (PyObject *dict)
{
    py_token_type_info.tp_str  = py_token_str;
    py_token_type_info.tp_repr = py_token_str;
    py_token_type_info.tp_methods = py_token_methods;

    pyutil_register_type (dict, &py_token_type_info);
}

PyObject *
token_to_py (Token *token)
{
    PyToken *py_token;

    if (token == NULL) {
        Py_INCREF (Py_None);
        return Py_None;
    }

    py_token = PyObject_New (PyToken, &py_token_type_info);
    py_token->token = token;

    return (PyObject *) py_token;
}

Token *
token_from_py (PyObject *obj)
{
    return ((PyToken *) obj)->token;
}

int
py_token_check (PyObject *obj)
{
    return PyObject_TypeCheck (obj, &py_token_type_info);
}


PyObject *
py_token_lookup (PyObject *self, PyObject *args)
{
    Token *token;
    char *raw;
    if (! PyArg_ParseTuple (args, "s", &raw))
        return NULL;
    token = token_lookup (raw);
    return token_to_py (token);
}

PyObject *
py_token_lookup_break (PyObject *self, PyObject *args)
{
    return token_to_py (token_lookup_break ());
}

PyObject *
py_token_lookup_wildcard (PyObject *self, PyObject *args)
{
    return token_to_py (token_lookup_wildcard ());
}
