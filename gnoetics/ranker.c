/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "ranker.h"

#include "fate.h"
#include "pyutil.h"

typedef struct _RankerSolution RankerSolution;
struct _RankerSolution {
    Token        *token;
    Text         *text;
    int           pos;
    FilterResults results;
};

Ranker *
ranker_new (void)
{
    Ranker *ranker;

    ranker = g_new0 (Ranker, 1);
    REFCOUNT_INIT (ranker);
    PYBIND_INIT (ranker);

    return ranker;
}

static void
ranker_dealloc (Ranker *ranker)
{
    ranker_clear (ranker);
    g_free (ranker);
}

void
ranker_clear (Ranker *ranker)
{
    if (ranker->all_solns) {
        g_array_free (ranker->all_solns, TRUE);
        ranker->all_solns = NULL;
    }
}

void
ranker_add_solution (Ranker       *ranker,
                     Token        *token,
                     Text         *text,
                     int           pos,
                     FilterResults results)
{
    RankerSolution elt;

    g_return_if_fail (ranker != NULL);
    g_return_if_fail (token  != NULL);
    g_return_if_fail (text   != NULL);

    g_return_if_fail (pos >= 0);
    g_return_if_fail (pos < text_get_length (text));

    if (ranker->all_solns == NULL) {
        ranker->all_solns = g_array_new (FALSE, FALSE,
                                         sizeof (RankerSolution));
    }

    elt.token   = token;
    elt.text    = text;
    elt.pos     = pos;
    elt.results = results;
    g_array_append_val (ranker->all_solns, elt);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

typedef struct _ByResults ByResults;
struct _ByResults {
    GPtrArray *favored;
    GPtrArray *accepted;
    GPtrArray *tolerated;
};

static ByResults *
by_results_new (void)
{
    ByResults *byres = g_new0 (ByResults, 1);

    byres->favored   = NULL;
    byres->accepted  = NULL;
    byres->tolerated = NULL;

    return byres;
}

static void
by_results_free (ByResults *byres)
{
    if (byres != NULL) {
        if (byres->favored)
            g_ptr_array_free (byres->favored, TRUE);
        if (byres->accepted)
            g_ptr_array_free (byres->accepted, TRUE);
        if (byres->tolerated)
            g_ptr_array_free (byres->tolerated, TRUE);
        g_free (byres);
    }
}


#define DEFAULT_SIZE 100
static void
by_results_add (ByResults    *byres,
                Token        *token,
                FilterResults results)
{
    GPtrArray *array = NULL;

    g_return_if_fail (byres != NULL);

    switch (results) {

        case FILTER_RESULTS_FAVOR:
            if (byres->favored == NULL)
                byres->favored = g_ptr_array_sized_new (DEFAULT_SIZE);
            array = byres->favored;
            break;

        case FILTER_RESULTS_ACCEPT:
            if (byres->accepted == NULL)
                byres->accepted = g_ptr_array_sized_new (DEFAULT_SIZE);
            array = byres->accepted;
            break;

        case FILTER_RESULTS_TOLERATE:
            if (byres->tolerated == NULL)
                byres->tolerated = g_ptr_array_sized_new (DEFAULT_SIZE);
            array = byres->tolerated;
            break;

        case FILTER_RESULTS_REJECT:
            return;

        default:
            g_assert_not_reached ();
    }

    g_ptr_array_add (array, token);
}

static void
by_results_shuffle (ByResults *byres)
{
    g_return_if_fail (byres != NULL);

    if (byres->favored)
        fate_shuffle_ptr_array (byres->favored);
    if (byres->accepted)
        fate_shuffle_ptr_array (byres->accepted);
    if (byres->tolerated)
        fate_shuffle_ptr_array (byres->tolerated);
}

static void
merge_unique (GPtrArray  *src,
              GHashTable *uniq,
              GPtrArray  *dest)
{
    int i;

    g_return_if_fail (uniq != NULL);
    g_return_if_fail (dest != NULL);

    if (src == NULL)
        return;

    for (i = 0; i < src->len; ++i) {
        gpointer ptr = g_ptr_array_index (src, i);
        if (g_hash_table_lookup (uniq, ptr) == NULL) {
            g_ptr_array_add (dest, ptr);
            g_hash_table_insert (uniq, ptr, ptr);
        }
    }
}

static GPtrArray *
by_results_collapse (ByResults *byres)
{
    GPtrArray *array;
    GHashTable *uniq;

    g_return_val_if_fail (byres != NULL, NULL);

    uniq = g_hash_table_new (NULL, NULL);
    array = g_ptr_array_new ();

    by_results_shuffle (byres);

    merge_unique (byres->favored, uniq, array);
    merge_unique (byres->accepted, uniq, array);
    merge_unique (byres->tolerated, uniq, array);

    g_hash_table_destroy (uniq);

    return array;
}


GPtrArray *
ranker_get_solutions (Ranker *ranker)
{
    ByResults *byres;
    GPtrArray *solns;
    RankerSolution *elt;
    int i;
       
    g_return_val_if_fail (ranker != NULL, NULL);

    byres = by_results_new ();

    if (ranker->all_solns) {
        for (i = 0; i < ranker->all_solns->len; ++i) {
            elt = & g_array_index (ranker->all_solns, RankerSolution, i);
            by_results_add (byres, elt->token, elt->results);
        }
    }

    solns = by_results_collapse (byres);

    by_results_free (byres);

    return solns;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

REFCOUNT_CODE (Ranker, ranker);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PYBIND_CODE (Ranker, ranker);

static PyObject *
py_ranker_clear (PyObject *self, PyObject *args)
{
    Ranker *ranker = ranker_from_py (self);

    ranker_clear (ranker);

    Py_INCREF (Py_None);
    return Py_None;
}

static PyObject *
py_ranker_add_solution (PyObject *self, PyObject *args)
{
    Ranker *ranker = ranker_from_py (self);
    PyObject *py_soln, *py_text;
    Token *soln;
    Text *text;
    int pos;
    int results;

    if (! PyArg_ParseTuple (args, "OOii", &py_soln, &py_text, &pos, &results))
        return NULL;

    soln = token_from_py (py_soln);
    text = text_from_py (py_text);

    ranker_add_solution (ranker, soln, text, pos, 
                         (FilterResults)results);

    Py_INCREF (Py_None);
    return Py_None;
}

static PyObject *
py_ranker_get_solutions (PyObject *self, PyObject *args)
{
    Ranker *ranker = ranker_from_py (self);
    GPtrArray *array;
    PyObject *py_list;
    int i;

    array = ranker_get_solutions (ranker);

    if (array == NULL) {
        Py_INCREF (Py_None);
        return Py_None;
    }

    py_list = PyList_New (0);

    for (i = 0; i < array->len; ++i) {
        Token *token;
        PyObject *py_token;

        token = g_ptr_array_index (array, i);
        py_token = token_to_py (token);

        PyList_Append (py_list, py_token);
        Py_DECREF (py_token);
    }

    g_ptr_array_free (array, TRUE);

    return py_list;
}

static PyMethodDef py_ranker_methods[] = {
    { "clear",         py_ranker_clear,         METH_NOARGS },
    { "add_solution",  py_ranker_add_solution,  METH_VARARGS },
    { "get_solutions", py_ranker_get_solutions, METH_NOARGS },
    { NULL, NULL, 0 }
};

static Ranker *
py_ranker_assemble (PyObject *args, PyObject *kwdict)
{
    return ranker_new ();
}

void
py_ranker_register (PyObject *dict)
{
    PYBIND_REGISTER_CODE (Ranker, ranker, dict);
}
