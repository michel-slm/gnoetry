/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __TRIMODEL_H__
#define __TRIMODEL_H__

#include <Python.h>
#include <glib.h>

#include "refcount.h"
#include "pybind.h"

#include "token.h"
#include "text.h"

typedef struct _TrimodelElement TrimodelElement;
struct _TrimodelElement {
    int key_offset;
    int syl_offset;
    Token *t1;
    Token *t2;
    Token *soln;
};

int trimodel_element_cmp_pair (const TrimodelElement *, Token *t1, Token *t2);
int trimodel_element_cmp      (gconstpointer, gconstpointer);

typedef struct _Trimodel Trimodel;
struct _Trimodel {
    REFCOUNT_BODY;
    PYBIND_BODY;

    GArray *array_AB_C;
    GArray *array_AC_B;
    GArray *array_BC_A;

    gboolean is_sorted;

    GList *text_list;
};

REFCOUNT_HEADERS (Trimodel, trimodel);
PYBIND_HEADERS (Trimodel, trimodel);

Trimodel *trimodel_new      (void);

void      trimodel_add_text (Trimodel *tri,
                             Text     *txt);




#endif /* __TRIMODEL_H__ */

