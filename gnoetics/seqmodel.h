/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __SEQMODEL_H__
#define __SEQMODEL_H__

#include <glib.h>
#include <Python.h>

#include "text.h"

typedef gpointer  SeqAtom;

typedef gboolean (*SeqAtomFn)    (SeqAtom a, gpointer user_data);

typedef gboolean (*SeqAtomEq)    (SeqAtom a, SeqAtom b);
typedef guint    (*SeqAtomHash)  (SeqAtom a);
typedef void     (*SeqAtomRef)   (SeqAtom a);
typedef void     (*SeqAtomUnref) (SeqAtom a);

typedef struct _SeqModel SeqModel;
struct _SeqModel {
    int refs;

    unsigned N;

    SeqAtom      padding_atom;
    SeqAtom      wildcard_atom;
    SeqAtomEq    atom_eq;
    SeqAtomHash  atom_hash;
    SeqAtomRef   atom_ref;
    SeqAtomUnref atom_unref;

    GHashTable  *tree;

};

SeqModel *seq_model_new (unsigned     N,
                         SeqAtom      padding_atom,
                         SeqAtom      wildcard_atom,
                         SeqAtomEq    atom_eq,
                         SeqAtomHash  atom_hash,
                         SeqAtomRef   atom_ref,
                         SeqAtomUnref atom_unref);

SeqModel *seq_model_ref   (SeqModel *model);
void      seq_model_unref (SeqModel *model);

void seq_model_add_sentence (SeqModel *model,
                             SeqAtom  *sentence,
                             unsigned  len);

int seq_model_solve (SeqModel *model,
                     SeqAtom  *tuple,
                     SeqAtomFn callback,
                     gpointer  user_data);
                      


SeqModel *token_seq_model_new (unsigned N);
 
void token_seq_model_add_text (SeqModel *model, Text *txt);

/* Python Extensions */

PyObject *seq_model_to_py   (SeqModel *model);
SeqModel *seq_model_from_py (PyObject *py_model);

void py_seq_model_register (PyObject *dict);

#endif /* __SEQMODEL_H__ */

