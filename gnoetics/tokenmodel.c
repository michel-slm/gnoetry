/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "tokenmodel.h"
#include "pyutil.h"

static gboolean
token_seq_atom_eq (SeqAtom a, SeqAtom b)
{
    return a == b;
}

static guint
token_seq_atom_hash (SeqAtom a)
{
    return GPOINTER_TO_UINT (a);
}

static void
token_model_fill_in (TokenModel *model)
{
    model->all_tokens = g_ptr_array_new ();
    
    model->seq_model = seq_model_new (model->N,
                                      token_lookup_break (),
                                      token_lookup_wildcard (),
                                      token_seq_atom_eq,
                                      token_seq_atom_hash,
                                      NULL, NULL);

    model->texts = NULL;
}

static void
token_model_hollow_out (TokenModel *model)
{
    g_ptr_array_free (model->all_tokens, TRUE);
    model->all_tokens = NULL;

    seq_model_unref (model->seq_model);
    model->seq_model = NULL;

    g_list_foreach (model->texts, (GFunc) text_unref, NULL);
    model->texts = NULL;
}

static void
token_model_dealloc (TokenModel *model)
{
    token_model_hollow_out (model);
    g_free (model);
}

TokenModel *
token_model_new (unsigned N)
{
    TokenModel *model;

    g_return_val_if_fail (N > 1, NULL);

    model = g_new0 (TokenModel, 1);
    REFCOUNT_INIT (model);
    PYBIND_INIT (model);

    model->N = N;
    
    token_model_fill_in (model);

    return model;
}

void
token_model_clear (TokenModel *model)
{
    g_return_if_fail (model != NULL);

    token_model_hollow_out (model);
    token_model_fill_in (model);
}

void
token_model_add_text (TokenModel *model,
                      Text       *txt)
{
    Token *buf[1024];
    int i, j, N;

    g_return_if_fail (model != NULL);
    g_return_if_fail (txt != NULL);

    text_ref (txt);
    model->texts = g_list_prepend (model->texts, txt);

    i = 0;
    N = text_get_length (txt);

    while (i < N) {
        j = i;
        while (j < N) {
            Token *token = text_get_token (txt, j);
            if (! token_is_break (token)) {
                g_assert (j-i < 1024);
                buf[j-i] = token;
                g_ptr_array_add (model->all_tokens, token);
                ++j;
            } else {
                break;
            }
        }

        if (j < N) {

            seq_model_add_sentence (model->seq_model, (SeqAtom *)buf, j-i);
         
            ++j; /* skip past the break */

            /* skip past any adjacent breaks */
            while (j < N && token_is_break (text_get_token (txt, j)))
                ++j;
        }

        i = j;
    }
}

Token *
token_model_pick (TokenModel  *model,
                  TokenFilter *filter)
{
    int i, j, N;

    g_return_val_if_fail (model != NULL, NULL);
    
    N = model->all_tokens->len;

    if (N == 0)
        return NULL;

    /* Case #1: No filtering.  This is easy. */
    if (filter == NULL) {
        i = random() % N;
        return (Token *) g_ptr_array_index (model->all_tokens, i);
    }

    /* Case #1.5: If the filter is impossible, we don't need to do
       any work. */
    if (filter->impossible)
        return NULL;
    
    /* Case #2: Permute the whole array, then walk across it and return
       the first item than matches. */
    for (i = 0; i < N-1; ++i) {
        j = i + (random() % (N-i));
        if (i != j) {
            gpointer tmp = model->all_tokens->pdata[i];
            model->all_tokens->pdata[i] = model->all_tokens->pdata[j];
            model->all_tokens->pdata[j] = tmp;
        }
    }

    for (i = 0; i < N; ++i) {
        Token *token = g_ptr_array_index (model->all_tokens, i);
        if (token_filter_test (filter, token))
            return token;
    }

    /* No matches.  Too bad. */
    return NULL;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct SolveInfo {
    TokenFilter *filter;
    TokenFn      callback;
    gpointer     user_data;
    int          soln_count;
};

static gboolean
solve_cb (SeqAtom atom, gpointer user_data)
{
    struct SolveInfo *info = user_data;
    Token *token = atom;
    
    if (info->filter == NULL || token_filter_test (info->filter, token)) {
        info->callback (token, info->user_data);
        ++info->soln_count;
    }

    return TRUE;
}

int
token_model_solve (TokenModel  *model,
                   Token      **tuple,
                   TokenFilter *filter,
                   TokenFn      callback,
                   gpointer     user_data)
{
    struct SolveInfo info;
    int N;

    g_return_val_if_fail (model != NULL, -1);
    g_return_val_if_fail (tuple != NULL, -1);
    g_return_val_if_fail (callback != NULL, -1);

    info.filter     = filter;
    info.callback   = callback;
    info.user_data  = user_data;
    info.soln_count = 0;

    N = seq_model_solve (model->seq_model, (SeqAtom *) tuple, solve_cb, &info);

    return N;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

REFCOUNT_CODE (TokenModel, token_model);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PYBIND_CODE (TokenModel, token_model);

static TokenModel *
py_token_model_assemble (PyObject *args, PyObject *kwdict)
{
    int N;

    if (! PyArg_ParseTuple (args, "i", &N))
        return NULL;

    g_assert (N > 1); /* FIXME */

    return token_model_new (N);
}

static PyObject *
py_token_model_clear (PyObject *self, PyObject *args)
{
    TokenModel *model = token_model_from_py (self);
    token_model_clear (model);
    
    Py_INCREF (Py_None);
    return Py_None;
}

static PyObject *
py_token_model_add_text (PyObject *self, PyObject *args)
{
    TokenModel *model = token_model_from_py (self);
    PyObject *py_txt;
    Text *txt;

    if (! (PyArg_ParseTuple (args, "O", &py_txt) && py_text_check (py_txt)))
        return NULL;

    txt = text_from_py (py_txt);
    token_model_add_text (model, txt);

    Py_INCREF (Py_None);
    return Py_None;
}

static PyObject *
py_token_model_get_texts (PyObject *self, PyObject *args)
{
    TokenModel *model = token_model_from_py (self);
    PyObject *py_list = PyList_New (0);
    GList *iter;

    for (iter = model->texts; iter != NULL; iter = iter->next) {
        PyObject *py_txt = text_to_py (iter->data);
        PyList_Append (py_list, py_txt);
    }

    return py_list;
}

static PyObject *
py_token_model_pick (PyObject *self, PyObject *args, PyObject *kwdict)
{
    TokenModel *model = token_model_from_py (self);
    TokenFilter filter;
    Token *token;

    token_filter_init_from_py_dict (&filter, kwdict);
    token_filter_optimize (&filter);

    token = token_model_pick (model, &filter);
    
    token_filter_clear (&filter);

    return token_to_py (token);
}

static gboolean
py_solve_cb (Token *token, gpointer user_data)
{
    PyObject *py_list = user_data;
    PyObject *py_token = token_to_py (token);
    PyList_Append (py_list, py_token);
    Py_DECREF (py_token);
    return TRUE;
}

static PyObject *
py_token_model_solve (PyObject *self, PyObject *args, PyObject *kwdict)
{
    TokenModel *model = token_model_from_py (self);
    TokenFilter filter;
    Token **tuple;
    PyObject *py_seq, *soln;
    int i, len;

    if (! PyArg_ParseTuple (args, "O", &py_seq))
        return NULL;

    if (! PySequence_Check (py_seq)) {
        PyErr_SetString (PyExc_ValueError,
                         "Argument must be a sequence of tokens");
        return NULL;
    }

    len = PySequence_Size (py_seq);
    if (len != model->seq_model->N) {
        PyErr_Format (PyExc_ValueError,
                      "Token tuple must have length %d",
                      model->seq_model->N);
        return NULL;
    }

    for (i = 0; i < len; ++i) {
        PyObject *py_token = PySequence_Fast_GET_ITEM (py_seq, i);
        if (! py_token_check (py_token)) {
            PyErr_Format (PyExc_ValueError,
                          "Non-Token found in position %d of sequence", i);
            return NULL;
        }
    }

    token_filter_init_from_py_dict (&filter, kwdict);
    token_filter_optimize (&filter);

    tuple = g_new (Token *, len);
    for (i = 0; i < len; ++i) {
        PyObject *py_token = PySequence_Fast_GET_ITEM (py_seq, i);
        tuple[i] = token_from_py (py_token);
    }

    soln = PyList_New (0);
    
    token_model_solve (model, tuple, &filter,
                       py_solve_cb, soln);
    g_free (tuple);

    token_filter_clear (&filter);

    return soln;
}

static PyMethodDef py_token_model_methods[] = {
    { "clear",        py_token_model_clear,     METH_NOARGS  },
    { "add_text",     py_token_model_add_text,  METH_VARARGS },
    { "get_texts",    py_token_model_get_texts, METH_NOARGS },
    { "pick",         py_token_model_pick,      METH_VARARGS | METH_KEYWORDS },
    { "solve",        py_token_model_solve,     METH_VARARGS | METH_KEYWORDS },
    { NULL, NULL, 0 }
};

void
py_token_model_register (PyObject *dict)
{
    PYBIND_REGISTER_CODE (TokenModel, token_model, dict);
}

