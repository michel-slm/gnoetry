/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __TRIMODEL_H__
#define __TRIMODEL_H__

#include <Python.h>
#include <glib.h>

#include "refcount.h"
#include "pybind.h"

#include "token.h"
#include "tokenfilter.h"
#include "text.h"
#include "ranker.h"

typedef struct _Trimodel Trimodel;
struct _Trimodel {
    REFCOUNT_BODY;
    PYBIND_BODY;

    GMutex *lock;

    GPtrArray *all_tokens;

    GArray *array_AB_C;
    GArray *array_AC_B;
    GArray *array_BC_A;

    GHashTable *is_leading;
    GHashTable *is_trailing;

    gboolean is_prepped;

    GList *text_list;
};

REFCOUNT_HEADERS (Trimodel, trimodel);
PYBIND_HEADERS (Trimodel, trimodel);

Trimodel *trimodel_new      (void);

void      trimodel_add_text (Trimodel *tri,
                             Text     *txt);

void      trimodel_prepare (Trimodel *tri);

gboolean  trimodel_is_ready (Trimodel *tri);

gint      trimodel_query (Trimodel       *tri,
                          Token          *token_a,
                          Token          *token_b,
                          Token          *token_c,
                          TokenFilter    *filter,
                          Ranker         *ranker);

gboolean  trimodel_token_is_leading  (Trimodel *tri, Token *tok);

gboolean  trimodel_token_is_trailing (Trimodel *tri, Token *tok);




#endif /* __TRIMODEL_H__ */

