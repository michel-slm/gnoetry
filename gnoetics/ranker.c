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
ranker_set_weight (Ranker *ranker,
                   Text   *txt,
                   double  wt)
{
    double *wt_ptr;

    g_return_if_fail (ranker != NULL);
    g_return_if_fail (txt != NULL);

    if (ranker->weights == NULL)
        ranker->weights = g_hash_table_new (NULL, NULL);

    wt = MAX (wt, 0.001);

    wt_ptr = g_hash_table_lookup (ranker->weights, txt);
    if (wt_ptr == NULL) {
        wt_ptr = g_new (double, 1);
        g_hash_table_insert (ranker->weights, txt, wt_ptr);
    }

    *wt_ptr = wt;
}

void
ranker_clear (Ranker *ranker)
{
    if (ranker->all_solns) {
        g_array_free (ranker->all_solns, TRUE);
        ranker->all_solns = NULL;
    }

    if (ranker->weights) {
        /* FIXME: leaking the values! */
        g_hash_table_destroy (ranker->weights);
        ranker->weights = NULL;
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

typedef struct _PerText PerText;
struct _PerText {
    Text      *text;
    ByResults *by_results;
    double     weight;

    /* some scratch variables */
    GPtrArray *array;
    int        i;
};

static void
per_text_collapse (GPtrArray    *per_text_array_original,
                   GHashTable   *uniq,
                   FilterResults results,
                   GPtrArray    *soln_array)
{
    GPtrArray *per_text_array;
    PerText *per_text;
    Token *token;
    double total_weight = 0.0;
    double wt;

    GPtrArray *array = NULL;
    int i;

    per_text_array = g_ptr_array_sized_new (per_text_array_original->len);

    /* First, we build up an array of the PerText items with solutions
       at this results level. */
    for (i = 0; i < per_text_array_original->len; ++i) {
        per_text = g_ptr_array_index (per_text_array_original, i);
        
        if (results == FILTER_RESULTS_FAVOR)
            array = per_text->by_results->favored;
        else if (results == FILTER_RESULTS_ACCEPT)
            array = per_text->by_results->accepted;
        else if (results == FILTER_RESULTS_TOLERATE)
            array = per_text->by_results->tolerated;
        else
            g_assert_not_reached ();

        if (array && array->len > 0 && per_text->weight > 0) {
            total_weight += per_text->weight;
            fate_shuffle_ptr_array (array);
            g_ptr_array_add (per_text_array, per_text);
            per_text->array = array;
            per_text->i = 0;
        }
    }

    while (per_text_array->len > 0) {

        per_text = NULL;

        /* First, pick a text to extract from */
        if (per_text_array->len > 1) {
            wt = fate_random_uniform (0, total_weight);
            i = -1;
            while (wt >= 0) {
                ++i;
                per_text = g_ptr_array_index (per_text_array, i);
                wt -= per_text->weight;
            }
        } else {
            /* If there is only one, there is no need for anything
               fancy. */
            i = 0;
            per_text = g_ptr_array_index (per_text_array, 0);
        }

        g_assert (per_text != NULL);

        /* Get the next solution, and add it to our solution array
           if we haven't seen it before. */
        
        token = g_ptr_array_index (per_text->array, per_text->i);
        if (g_hash_table_lookup (uniq, token) == NULL) {
            g_ptr_array_add (soln_array, token);
            g_hash_table_insert (uniq, token, token);
        }

        /* Increment the solution pointer.  If that was the last solution,
           remove this text's items from consideration. */
        ++per_text->i;
        if (per_text->i >= per_text->array->len) {
            total_weight -= per_text->weight;
            g_ptr_array_remove_index_fast (per_text_array, i);
        }
    }

    g_ptr_array_free (per_text_array, TRUE);
}



GPtrArray *
ranker_get_solutions (Ranker *ranker)
{
    GPtrArray *solns;
    RankerSolution *elt;
    int i;

    PerText    *per_text;
    GPtrArray  *per_text_array;
    GHashTable *per_text_hash;
    GHashTable *uniq;
    double *wt_ptr;
       
    g_return_val_if_fail (ranker != NULL, NULL);

    per_text_array = g_ptr_array_new ();
    per_text_hash = g_hash_table_new (NULL, NULL);

    if (ranker->all_solns) {
        for (i = 0; i < ranker->all_solns->len; ++i) {
            elt = & g_array_index (ranker->all_solns, RankerSolution, i);

            per_text = g_hash_table_lookup (per_text_hash, elt->text);
            if (per_text == NULL) {
                per_text = g_new0 (PerText, 1);
                per_text->text = elt->text;
                per_text->by_results = by_results_new ();

                wt_ptr = NULL;
                if (ranker->weights)
                    wt_ptr = g_hash_table_lookup (ranker->weights, elt->text);
                per_text->weight = wt_ptr ? *wt_ptr : 1.0;

                g_hash_table_insert (per_text_hash, elt->text, per_text);
                g_ptr_array_add (per_text_array, per_text);
            }

            by_results_add (per_text->by_results, elt->token, elt->results);
        }
    }

    solns = g_ptr_array_new ();
    uniq = g_hash_table_new (NULL, NULL);

    per_text_collapse (per_text_array, uniq, FILTER_RESULTS_FAVOR, solns);
    per_text_collapse (per_text_array, uniq, FILTER_RESULTS_ACCEPT, solns);
    per_text_collapse (per_text_array, uniq, FILTER_RESULTS_TOLERATE, solns);

    /* Clean up */
    g_hash_table_destroy (uniq);
    for (i = 0; i < per_text_array->len; ++i) {
        per_text = g_ptr_array_index (per_text_array, i);
        by_results_free (per_text->by_results);
        g_free (per_text);
    }
    g_ptr_array_free (per_text_array, TRUE);
    g_hash_table_destroy (per_text_hash);

    return solns;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

REFCOUNT_CODE (Ranker, ranker);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PYBIND_CODE (Ranker, ranker);

static PyObject *
py_ranker_set_weight (PyObject *self, PyObject *args)
{
    Ranker *ranker = ranker_from_py (self);
    PyObject *py_txt;
    Text *txt;
    double wt;

    if (! PyArg_ParseTuple (args, "Od", &py_txt, &wt))
        return NULL;

    txt = text_from_py (py_txt);
    ranker_set_weight (ranker, txt, wt);

    Py_INCREF (Py_None);
    return Py_None;
}

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
    { "set_weight",    py_ranker_set_weight,    METH_VARARGS },
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
