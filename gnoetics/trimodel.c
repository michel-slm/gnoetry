/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifdef CONFIG_H
#include <config.h>
#endif
#include "trimodel.h"

#include "fate.h"

#define CMP(x, y) (((x) > (y)) - ((x) < (y)))

typedef struct _TrimodelElement TrimodelElement;
struct _TrimodelElement {
    int key_offset;
    int syl_offset;
    Token *t1;
    Token *t2;
    Token *soln;

    Text  *text; /* source text */
    int    pos;  /* position of soln in that text */
};

/* 
   Note: We don't hold a reference to the text in the TrimodelElement
   structure, since the Trimodel itself will hold a reference in the
   text_list.
*/

static int
trimodel_element_cmp_pair (const TrimodelElement *elt,
                           const Token           *t1,
                           const Token           *t2)
{
    guint x_id, y_id;

    x_id = GPOINTER_TO_UINT (elt->t1);
    y_id = GPOINTER_TO_UINT (t1);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    x_id = GPOINTER_TO_UINT (elt->t2);
    y_id = GPOINTER_TO_UINT (t2);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    return 0;
}

static int
trimodel_element_cmp (gconstpointer x_ptr,
                      gconstpointer y_ptr)
{
    const TrimodelElement *x = x_ptr;
    const TrimodelElement *y = y_ptr;
    guint x_id, y_id;

    x_id = GPOINTER_TO_UINT (x->t1);
    y_id = GPOINTER_TO_UINT (y->t1);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    x_id = GPOINTER_TO_UINT (x->t2);
    y_id = GPOINTER_TO_UINT (y->t2);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    x_id = token_get_syllables (x->soln);
    y_id = token_get_syllables (y->soln);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    x_id = GPOINTER_TO_UINT (x->soln);
    y_id = GPOINTER_TO_UINT (y->soln);
    return CMP (x_id, y_id);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

Trimodel *
trimodel_new (void)
{
    Trimodel *tri;

    tri = g_new0 (Trimodel, 1);
    REFCOUNT_INIT (tri);
    PYBIND_INIT (tri);

    tri->array_AB_C = g_array_new (FALSE, FALSE, sizeof (TrimodelElement));
    tri->array_AC_B = g_array_new (FALSE, FALSE, sizeof (TrimodelElement));
    tri->array_BC_A = g_array_new (FALSE, FALSE, sizeof (TrimodelElement));

    tri->is_prepped = FALSE;

    return tri;
}

static void
trimodel_dealloc (Trimodel *tri)
{
    g_return_if_fail (tri != NULL);

    g_array_free (tri->array_AB_C, TRUE);
    g_array_free (tri->array_AC_B, TRUE);
    g_array_free (tri->array_BC_A, TRUE);

    if (tri->all_tokens != NULL)
        g_ptr_array_free (tri->all_tokens, TRUE);

    if (tri->is_leading != NULL)
        g_hash_table_destroy (tri->is_leading);
    if (tri->is_trailing != NULL)
        g_hash_table_destroy (tri->is_trailing);
    
    g_list_foreach (tri->text_list, (GFunc) text_unref, NULL);
    g_list_free (tri->text_list);
    
    g_free (tri);
}

static void
trimodel_sort_single_array (Trimodel *tri,
                            GArray   *array)
{
    int i, first_i, firstsyl_i, syl_n;
    TrimodelElement *elt, *first, *firstsyl;

    if (array->len < 2)
        return;

    g_array_sort (array, trimodel_element_cmp);

    /* Attach offset information. */
    first_i = firstsyl_i = 0;
    first = firstsyl = &g_array_index (array, TrimodelElement, 0);
    syl_n = -1;

    for (i = 1; i < array->len; ++i) {
        elt = &g_array_index (array, TrimodelElement, i);

        if (first->t1 == elt->t1 && first->t2 == elt->t2) {
            first->key_offset = i - first_i;
            elt->key_offset = first_i - i;

            if (syl_n == token_get_syllables (elt->soln)) {
                firstsyl->syl_offset = i - firstsyl_i;
                elt->syl_offset = firstsyl_i - i;
            } else {
                firstsyl_i = i;
                firstsyl = elt;
                firstsyl->syl_offset = 0;
                syl_n = token_get_syllables (firstsyl->soln);
            }

        } else {
            first_i = firstsyl_i = i;
            first = firstsyl = elt;
            first->key_offset = 0;
            firstsyl->syl_offset = 0;
        }
    }
}

static void
trimodel_add_triple (Trimodel *tri,
                     Text     *text,
                     int       pos_of_a,
                     Token    *a,
                     Token    *b,
                     Token    *c)
{
    TrimodelElement elt;

    g_return_if_fail (tri != NULL);
    g_return_if_fail (a != NULL);
    g_return_if_fail (b != NULL);
    g_return_if_fail (c != NULL);

    elt.key_offset = 0;
    elt.syl_offset = 0;
    
    elt.t1 = a;
    elt.t2 = b;
    elt.soln = c;
    elt.text = text;
    elt.pos  = pos_of_a + 2;
    g_array_append_val (tri->array_AB_C, elt);

    elt.t1 = a;
    elt.t2 = c;
    elt.soln = b;
    elt.text = text;
    elt.pos  = pos_of_a + 1;
    g_array_append_val (tri->array_AC_B, elt);

    elt.t1 = b;
    elt.t2 = c;
    elt.soln = a;
    elt.text = text;
    elt.pos = pos_of_a;
    g_array_append_val (tri->array_BC_A, elt);

    tri->is_prepped = FALSE;
}

void
trimodel_add_text (Trimodel *tri,
                   Text     *txt)
{
    Token *brk, *tok, *window[3];
    int i, j, N;

    g_return_if_fail (tri != NULL);
    g_return_if_fail (txt != NULL);

    tri->text_list = g_list_append (tri->text_list, text_ref (txt));

    brk = token_lookup_break ();
    g_assert (brk != NULL);

    window[0] = brk;
    window[1] = brk;
    window[2] = brk;
    
    N = text_get_length (txt);
    for (i = 0; i < N; ++i) {
        tok = text_get_token (txt, i);

        j = 0;
        do {
            window[0] = window[1];
            window[1] = window[2];
            window[2] = tok;
            trimodel_add_triple (tri, txt, i-2+j, 
                                 window[0], window[1], window[2]);
            ++j;
        } while (j < 2 && token_is_break (tok));
    }
}

static void
trimodel_build_all_tokens_array (Trimodel *tri)
{
    GHashTable *uniq;
    TrimodelElement *elt;
    int i;

    g_return_if_fail (tri != NULL);

    if (tri->all_tokens != NULL)
        g_ptr_array_free (tri->all_tokens, TRUE);

    tri->all_tokens = g_ptr_array_new ();

    uniq = g_hash_table_new (NULL, NULL);

    elt = & g_array_index (tri->array_AB_C, TrimodelElement, 0);

    for (i = 0; i < tri->array_AB_C->len; ++i) {

        elt = & g_array_index (tri->array_AB_C, TrimodelElement, i);

        /* We do a lot more work here than we need to... */

        if (g_hash_table_lookup (uniq, elt->t1) == NULL) {
            g_ptr_array_add (tri->all_tokens, elt->t1);
            g_hash_table_insert (uniq, elt->t1, elt->t1);
        }

        if (g_hash_table_lookup (uniq, elt->t2) == NULL) {
            g_ptr_array_add (tri->all_tokens, elt->t2);
            g_hash_table_insert (uniq, elt->t2, elt->t2);
        }
        
        if (g_hash_table_lookup (uniq, elt->soln) == NULL) {
            g_ptr_array_add (tri->all_tokens, elt->soln);
            g_hash_table_insert (uniq, elt->soln, elt->soln);
        }
    }
}

static void
trimodel_build_leading_trailing (Trimodel *tri)
{
    GHashTable *not_head, *not_tail;
    TrimodelElement *elt;
    int i;

    g_return_if_fail (tri != NULL);

    if (tri->is_leading != NULL)
        g_hash_table_destroy (tri->is_leading);

    if (tri->is_trailing != NULL)
        g_hash_table_destroy (tri->is_trailing);

    tri->is_leading = g_hash_table_new (NULL, NULL);
    tri->is_trailing = g_hash_table_new (NULL, NULL);

    not_head = g_hash_table_new (NULL, NULL);
    not_tail = g_hash_table_new (NULL, NULL);

    for (i = 0; i < tri->array_AB_C->len; ++i) {
        elt = &g_array_index (tri->array_AB_C, TrimodelElement, i);

        if (!token_is_break (elt->t1) && !token_is_break (elt->t2)) {
            if (g_hash_table_lookup (not_tail, elt->t1) == NULL)
                g_hash_table_insert (not_tail, elt->t1, elt->t1);
            if (g_hash_table_lookup (not_head, elt->t2) == NULL)
                g_hash_table_insert (not_head, elt->t2, elt->t2);
        }

        if (!token_is_break (elt->t2) && !token_is_break (elt->soln)) {
            if (g_hash_table_lookup (not_tail, elt->t2) == NULL)
                g_hash_table_insert (not_tail, elt->t2, elt->t2);
            if (g_hash_table_lookup (not_head, elt->soln) == NULL)
                g_hash_table_insert (not_head, elt->soln, elt->soln);
        }
    }

    for (i = 0; i < tri->all_tokens->len; ++i) {
        Token *t = g_ptr_array_index (tri->all_tokens, i);

        if (token_is_break (t))
            continue;
        
        if (g_hash_table_lookup (not_head, t) == NULL) 
            g_hash_table_insert (tri->is_leading, t, t);

        if (g_hash_table_lookup (not_tail, t) == NULL)
            g_hash_table_insert (tri->is_trailing, t, t);

    }

    g_hash_table_destroy (not_head);
    g_hash_table_destroy (not_tail);
}

void
trimodel_prepare (Trimodel *tri)
{
    g_return_if_fail (tri != NULL);

    if (tri->is_prepped)
        return;

    trimodel_sort_single_array (tri, tri->array_AB_C);
    trimodel_sort_single_array (tri, tri->array_AC_B);
    trimodel_sort_single_array (tri, tri->array_BC_A);

    trimodel_build_all_tokens_array (tri);

    trimodel_build_leading_trailing (tri);

    tri->is_prepped = TRUE;
}

static gboolean
trimodel_query_array (Trimodel *tri,
                      GArray   *array,
                      Token    *t1,
                      Token    *t2,
                      int       min_syllables,
                      int       max_syllables,
                      int      *out_i0,
                      int      *out_i1)
{
    int a, b, i, i0, i1, cmp;
    const TrimodelElement *elt;

    g_return_val_if_fail (tri != NULL, FALSE);
    g_return_val_if_fail (t1 != NULL, FALSE);
    g_return_val_if_fail (t2 != NULL, FALSE);

    if (min_syllables >= 0 
        && max_syllables >= 0 
        && min_syllables > max_syllables)
        return FALSE;

    if (! tri->is_prepped)
        trimodel_prepare (tri);

    /* First, find a possible hit. */

    a = 0;
    b = array->len-1;
    i = -1;

    while (b-a > 1) {
        i = (a+b)/2;

        elt = &g_array_index (array, TrimodelElement, i);
        cmp = trimodel_element_cmp_pair (elt, t1, t2);

        if (cmp == 0)
            break;
        else if (cmp < 0)
            a = i;
        else /* cmp > 0 */
            b = i;

        i = -1;
    }

    if (b-a <= 1 && i != -1) {
        elt = &g_array_index (array, TrimodelElement, a);
        cmp = trimodel_element_cmp_pair (elt, t1, t2);
        if (cmp == 0)
            i = a;
        else if (a != b) {
            elt = &g_array_index (array, TrimodelElement, b);
            cmp = trimodel_element_cmp_pair (elt, t1, t2);
            if (cmp == 0)
                i = b;
        }
    }

    if (i == -1)
        return FALSE;


    /* Next, use the hit to find the beginning and end of our
       solution range. */
    g_assert (0 <= i && i < array->len);
    elt = &g_array_index (array, TrimodelElement, i);
    g_assert (elt->t1 == t1);
    g_assert (elt->t2 == t2);

    i0 = i;
    if (elt->key_offset < 0) /* move back to start */
        i0 += elt->key_offset;

    g_assert (0 <= i && i < array->len);
    elt = &g_array_index (array, TrimodelElement, i0);
    g_assert (elt->key_offset >= 0);
    g_assert (elt->t1 == t1);
    g_assert (elt->t2 == t2);

    i1 = i0 + elt->key_offset;

    /* Adjust bounds to find solutions w/ right numbers of syllables. */
    if (min_syllables >= 0) {
        while (i0 <= i1) {
            elt = &g_array_index (array, TrimodelElement, i0);
            if (min_syllables <= token_get_syllables (elt->soln)) {
                /* If min_syllables == max_syllables, we have
                   enough information to skip the separate max_syllables
                   check. */
                if (min_syllables == max_syllables) {
                    i1 = i0 + elt->syl_offset;
                    max_syllables = -1;
                }
                break;
            }
            i0 += elt->syl_offset+1;
        }
    }

    if (max_syllables >= 0) {
        while (i0 <= i1) {
            elt = &g_array_index (array, TrimodelElement, i0);
            if (token_get_syllables (elt->soln) <= max_syllables)
                break;
            i1 -= elt->syl_offset+1;
        }
    }

    if (i0 > i1)
        return FALSE;

    if (out_i0)
        *out_i0 = i0;

    if (out_i1)
        *out_i1 = i1;
    
    return TRUE;
}

static gboolean
leading_test_cb (Token   *t,
                 gpointer user_data)
{
    Trimodel *tri = user_data;
    return trimodel_token_is_leading (tri, t);
}

static gboolean
trailing_test_cb (Token   *t,
                  gpointer user_data)
{
    Trimodel *tri = user_data;
    return trimodel_token_is_trailing (tri, t);
}

gint
trimodel_query (Trimodel       *tri,
                Token          *token_a,
                Token          *token_b,
                Token          *token_c,
                TokenFilter    *filter,
                Ranker         *ranker)
{

    int i0, i1, i, count;
    int wild_count;
    Token *t1 = NULL, *t2 = NULL;
    GArray *array = NULL;
    const TrimodelElement *elt;
    Token *prev_soln;
    FilterResults results;

    g_return_val_if_fail (tri != NULL, -1);
    g_return_val_if_fail (token_a != NULL, -1);
    g_return_val_if_fail (token_b != NULL, -1);
    g_return_val_if_fail (token_c != NULL, -1);

    wild_count = 0;
    if (token_is_wildcard (token_a))
        ++wild_count;
    if (token_is_wildcard (token_b))
        ++wild_count;
    if (token_is_wildcard (token_c))
        ++wild_count;
    if (wild_count != 1) {
        g_warning ("ignoring ill-formed query w/ %d wildcards", wild_count);
        return -1;
    }

    if (token_is_wildcard (token_a)) {
        t1 = token_b;
        t2 = token_c;
        array = tri->array_BC_A;
    } else if (token_is_wildcard (token_b)) {
        t1 = token_a;
        t2 = token_c;
        array = tri->array_AC_B;
    } else if (token_is_wildcard (token_c)) {
        t1 = token_a;
        t2 = token_b;
        array = tri->array_AB_C;
    } else {
        g_assert_not_reached ();
    }

    g_assert (t1 != NULL);
    g_assert (t2 != NULL);
    g_assert (array != NULL);

    if (! trimodel_query_array (tri,
                                array,
                                t1, t2,
                                filter ? filter->min_syllables : -1,
                                filter ? filter->max_syllables : -1,
                                &i0, &i1)) {
        return 0;
    }

    g_assert (i0 <= i1);

    prev_soln = NULL;
    results = FILTER_RESULTS_REJECT;
    count = 0;
    for (i = i0; i <= i1; ++i) {
        elt = &g_array_index (array, TrimodelElement, i);
        if (filter == NULL) {
            results = FILTER_RESULTS_ACCEPT;
        } else if (elt->soln == prev_soln) {
            /* retain previous value of results */
        } else {
            results = token_filter_test (filter,
                                         elt->soln,
                                         leading_test_cb,
                                         trailing_test_cb,
                                         tri);


            prev_soln = elt->soln;
        }


        if (results != FILTER_RESULTS_REJECT) {
            if (ranker != NULL) {
                ranker_add_solution (ranker,
                                     elt->soln,
                                     elt->text,
                                     elt->pos,
                                     results);
            }
            ++count;
        }
    }

    return count;
}

gboolean
trimodel_token_is_leading (Trimodel *tri,
                           Token    *tok)
{
    g_return_val_if_fail (tri != NULL, FALSE);
    g_return_val_if_fail (tok != NULL, FALSE);

    return g_hash_table_lookup (tri->is_leading, tok) != NULL;
}

gboolean
trimodel_token_is_trailing (Trimodel *tri,
                            Token    *tok)
{
    g_return_val_if_fail (tri != NULL, FALSE);
    g_return_val_if_fail (tok != NULL, FALSE);

    return g_hash_table_lookup (tri->is_trailing, tok) != NULL;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

REFCOUNT_CODE (Trimodel, trimodel);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PYBIND_CODE (Trimodel, trimodel);

static PyObject *
py_trimodel_add_text (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    PyObject *py_txt;
    Text *txt;
    
    if (! (PyArg_ParseTuple (args, "O", &py_txt) && py_text_check (py_txt)))
        return NULL;

    txt = text_from_py (py_txt);
    trimodel_add_text (tri, txt);

    Py_INCREF (Py_None);
    return Py_None;
}

static PyObject *
py_trimodel_get_texts (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    GList *iter;
    PyObject *py_list;

    py_list = PyList_New (0);

    for (iter = tri->text_list; iter != NULL; iter = iter->next) {
        Text *txt = iter->data;
        PyObject *py_txt = text_to_py (txt);
        PyList_Append (py_list, py_txt);
        Py_DECREF (py_txt);
    }

    return py_list;
}

static PyObject *
py_trimodel_prepare (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    trimodel_prepare (tri);

    Py_INCREF (Py_None);
    return Py_None;
}

static PyObject *
py_trimodel_query (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);

    PyObject *py_t1, *py_t2, *py_t3, *py_filter, *py_ranker;

    Token *t1, *t2, *t3;
    TokenFilter filter;
    Ranker *ranker;

    int count;

    if (! PyArg_ParseTuple (args, "OOOOO",
                            &py_t1, &py_t2, &py_t3,
                            &py_filter,
                            &py_ranker))
        return NULL;

    if (! (py_token_check (py_t1)
           && py_token_check (py_t2)
           && py_token_check (py_t3)
           && PyDict_Check (py_filter)
           && py_ranker_check (py_ranker))) {
        PyErr_SetString (PyExc_ValueError, "Badly-formed query");
        return NULL;
    }

    t1 = token_from_py (py_t1);
    t2 = token_from_py (py_t2);
    t3 = token_from_py (py_t3);

    token_filter_init_from_py_dict (&filter, py_filter);

    ranker = ranker_from_py (py_ranker);

    count = trimodel_query (tri, t1, t2, t3, &filter, ranker);

    return PyInt_FromLong (count);
}

static PyMethodDef py_trimodel_methods[] = {
    { "add_text",  py_trimodel_add_text, METH_VARARGS },
    { "get_texts", py_trimodel_get_texts, METH_NOARGS },
    { "prepare",   py_trimodel_prepare,  METH_NOARGS  },
    { "query",     py_trimodel_query,    METH_VARARGS },

    { NULL, NULL, 0 }

};

static Trimodel *
py_trimodel_assemble (PyObject *args, PyObject *kwdict)
{
    return trimodel_new ();
}

void
py_trimodel_register (PyObject *dict)
{
    PYBIND_REGISTER_CODE (Trimodel, trimodel, dict);
}
