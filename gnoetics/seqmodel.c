/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "seqmodel.h"

#include "pyutil.h"
#include "token.h"

SeqModel *
seq_model_new (unsigned     N,
               SeqAtom      padding_atom,
               SeqAtom      wildcard_atom,
               SeqAtomEq    atom_eq,
               SeqAtomHash  atom_hash,
               SeqAtomRef   atom_ref,
               SeqAtomUnref atom_unref)
{
    SeqModel *model = g_new0 (SeqModel, 1);

    model->refs          = 1;
    model->N             = N;
    model->padding_atom  = padding_atom;
    model->wildcard_atom = wildcard_atom;
    model->atom_eq       = atom_eq;
    model->atom_hash     = atom_hash;
    model->atom_ref      = atom_ref;
    model->atom_unref    = atom_unref;

    return model;
}

SeqModel *
seq_model_ref (SeqModel *model)
{
    g_return_val_if_fail (model != NULL, NULL);
    g_return_val_if_fail (model->refs > 0, NULL);
  
    ++model->refs;
    return model;
}

void
seq_model_unref (SeqModel *model)
{
    if (model != NULL) {
        g_return_if_fail (model->refs > 0);
        --model->refs;

        if (model->refs == 0) {
            /* no-op for now */
        }
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static GHashTable *
seq_model_make_atom_hash (SeqModel *model)
{
    return g_hash_table_new ((GHashFunc) model->atom_hash,
                             (GEqualFunc) model->atom_eq);
}

static GPtrArray *
seq_model_walk_tuple (SeqModel *model,
                      SeqAtom  *tuple,
                      gboolean  create_array)
{
    GHashTable *walk = model->tree;
    GPtrArray *array;
    unsigned i;

    if (walk == NULL) {
        model->tree = walk = seq_model_make_atom_hash (model);
    }

    for (i = 0; i < model->N-1; ++i) {
        GHashTable *step = g_hash_table_lookup (walk, tuple[i]);
        if (step == NULL) {
            if (! create_array)
                return NULL;
            step = seq_model_make_atom_hash (model);
            if (model->atom_ref)
                model->atom_ref (tuple[i]);
            g_hash_table_insert (walk, tuple[i], step); 
        }
        walk = step;
    }

    array = g_hash_table_lookup (walk, tuple[model->N-1]);
    if (array == NULL && create_array) {
        array = g_ptr_array_new ();
        if (model->atom_ref)
            model->atom_ref (tuple[model->N-1]);
        g_hash_table_insert (walk, tuple[model->N-1], array);
    }

    return array;
}

static void
seq_model_add_tuple (SeqModel *model,
                     SeqAtom  *tuple)
{
    GPtrArray *array;
    SeqAtom saved;
    unsigned i;

    for (i = 0; i < model->N; ++i) {
        saved = tuple[i];
        tuple[i] = model->wildcard_atom;

        array = seq_model_walk_tuple (model, tuple, TRUE);
        if (model->atom_ref)
            model->atom_ref (saved);
        g_ptr_array_add (array, saved);

        tuple[i] = saved;
    }
}

void
seq_model_add_sentence (SeqModel *model,
                        SeqAtom  *sentence,
                        unsigned  len)
{
    SeqAtom *tuple;
    unsigned i, j;

    /* build a fully-padded tuple */
    tuple = g_new0 (SeqAtom, model->N);
    for (i = 0; i < model->N; ++i)
        tuple[i] = model->padding_atom;

    /* slide our sentence through the tuple */
    for (j = 0; j < len; ++j) {
        /* shift tuple */
        for (i = 1; i < model->N; ++i)
            tuple[i-1] = tuple[i];
        tuple[model->N-1] = sentence[j];
        seq_model_add_tuple (model, tuple);
    }

    /* generate our right-padded tuples */
    for (j = 0; j < model->N-1; ++j) {
        /* shift tuple */
        for (i = 1; i < model->N; ++i)
            tuple[i-1] = tuple[i];
        tuple[model->N-1] = model->padding_atom;
        seq_model_add_tuple (model, tuple);
    }
  
    g_free (tuple);
}

int
seq_model_solve (SeqModel *model,
                 SeqAtom  *tuple,
                 SeqAtomFn callback,
                 gpointer  user_data)
{
    GPtrArray *array;
    int i;

    g_return_val_if_fail (model != NULL, -1);
    g_return_val_if_fail (tuple != NULL, -1);
    g_return_val_if_fail (callback != NULL, -1);

    array = seq_model_walk_tuple (model, tuple, FALSE);
    if (array == NULL)
        return 0;

    for (i = 0; i < array->len; ++i)
        if (! callback((SeqAtom) g_ptr_array_index (array, i), user_data))
            break;

    return array->len;
}

