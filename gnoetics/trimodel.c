/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifdef CONFIG_H
#include <config.h>
#endif
#include "trimodel.h"

#define CMP(x, y) (((x) > (y)) - ((x) < (y)))

int
trimodel_element_cmp_pair (const TrimodelElement *elt,
                           Token                 *t1,
                           Token                 *t2)
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

int
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

    tri->is_sorted = FALSE;

    return tri;
}

static void
trimodel_dealloc (Trimodel *tri)
{
    g_return_if_fail (tri != NULL);

    g_array_free (tri->array_AB_C, TRUE);
    g_array_free (tri->array_AC_B, TRUE);
    g_array_free (tri->array_BC_A, TRUE);
    
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
trimodel_do_sort (Trimodel *tri)
{
    g_return_if_fail (tri != NULL);

    if (! tri->is_sorted) {

        trimodel_sort_single_array (tri, tri->array_AB_C);
        trimodel_sort_single_array (tri, tri->array_AC_B);
        trimodel_sort_single_array (tri, tri->array_BC_A);

        tri->is_sorted = TRUE;
    }
}

static void
trimodel_add_triple (Trimodel *tri,
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
    g_array_append_val (tri->array_AB_C, elt);

    elt.t1 = a;
    elt.t2 = c;
    elt.soln = b;
    g_array_append_val (tri->array_AC_B, elt);

    elt.t1 = b;
    elt.t2 = c;
    elt.soln = a;
    g_array_append_val (tri->array_BC_A, elt);

    tri->is_sorted = FALSE;
}

void
trimodel_add_text (Trimodel *tri,
                   Text     *txt)
{
    Token *brk, *tok, *window[3];
    int i, j, N;

    g_return_if_fail (tri != NULL);
    g_return_if_fail (txt != NULL);

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
            trimodel_add_triple (tri, window[0], window[1], window[2]);
            ++j;
        } while (j < 2 && token_is_break (tok));
    }

    tri->text_list = g_list_append (tri->text_list, text_ref (txt));

    trimodel_do_sort (tri);
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
py_trimodel_get_raw_len (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    return PyInt_FromLong (tri->array_AB_C->len);
}

static PyObject *
py_trimodel_get_raw (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    TrimodelElement *elt;
    int i;
    
    if (! PyArg_ParseTuple (args, "i", &i))
        return NULL;
    g_assert (0 <= i < tri->array_AB_C->len);

    elt = &g_array_index (tri->array_AB_C, TrimodelElement, i);

    return Py_BuildValue ("(iiNNN)",
                          elt->key_offset,
                          elt->syl_offset,
                          token_to_py (elt->t1),
                          token_to_py (elt->t2),
                          token_to_py (elt->soln));
}

static PyObject *
py_trimodel_get_raw_cmp (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    PyObject *py_t1, *py_t2;
    Token *t1, *t2;
    TrimodelElement *elt;
    int i;
    
    if (! PyArg_ParseTuple (args, "iOO", &i, &py_t1, &py_t2))
        return NULL;
    g_assert (0 <= i < tri->array_AB_C->len);

    t1 = token_from_py (py_t1);
    t2 = token_from_py (py_t2);

    elt = &g_array_index (tri->array_AB_C, TrimodelElement, i);

    return PyInt_FromLong (trimodel_element_cmp_pair (elt, t1, t2));
}

static PyMethodDef py_trimodel_methods[] = {
    { "add_text", py_trimodel_add_text, METH_VARARGS },

    { "get_raw_len", py_trimodel_get_raw_len, METH_NOARGS },
    { "get_raw",     py_trimodel_get_raw,     METH_VARARGS },
    { "get_raw_cmp", py_trimodel_get_raw_cmp, METH_VARARGS },

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
