/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __RANKER_H__
#define __RANKER_H__

#include <Python.h>
#include <glib.h>

#include "refcount.h"
#include "pybind.h"

#include "token.h"
#include "tokenfilter.h"
#include "text.h"

typedef struct _Ranker Ranker;
struct _Ranker {
    REFCOUNT_BODY;
    PYBIND_BODY;

    GArray *all_solns;
};

REFCOUNT_HEADERS (Ranker, ranker);
PYBIND_HEADERS (Ranker, ranker);

Ranker    *ranker_new (void);

void       ranker_clear (Ranker *);

void       ranker_add_solution (Ranker       *ranker,
                                Token        *soln,
                                Text         *text,
                                int           pos,
                                FilterResults results);

GPtrArray *ranker_get_solutions (Ranker *);
                                 
#endif /* __RANKER_H__ */

