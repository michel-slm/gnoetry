/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __TOKENMODEL_H__
#define __TOKENMODEL_H__

#include <Python.h>
#include <glib.h>

#include "refcount.h"
#include "pybind.h"

#include "seqmodel.h"
#include "token.h"
#include "tokenfilter.h"
#include "text.h"

typedef struct _TokenModel TokenModel;
struct _TokenModel {
    REFCOUNT_BODY;
    PYBIND_BODY;

    unsigned N;
    GPtrArray *all_tokens;
    SeqModel **seq_model_array;
    GList *texts;
};

REFCOUNT_HEADERS (TokenModel, token_model);
PYBIND_HEADERS   (TokenModel, token_model);

TokenModel *token_model_new   (unsigned N);

#define token_model_get_N(tm) ((tm)->N)

void        token_model_clear (TokenModel *);

void        token_model_add_text (TokenModel *model, Text *txt);

Token      *token_model_pick (TokenModel  *model,
                              TokenFilter *filter);

int         token_model_solve (TokenModel       *model,
                               unsigned          tuple_len,
                               Token           **tuple,
                               TokenFilter      *filter,
                               TokenFn           callback,
                               gpointer          user_data);

#endif /* __TOKENMODEL_H__ */

