/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __TEXT_H__
#define __TEXT_H__

#include <Python.h>
#include <glib.h>

#include "refcount.h"
#include "pybind.h"

#include "token.h"

typedef struct _Text Text;
struct _Text {
    REFCOUNT_BODY;
    PYBIND_BODY;

    GMutex *lock;
    gboolean preloading;

    char *filename;
    char *title;
    char *author;

    char *sort_title;
    char *sort_author;

    int length;
    Token **token_stream;
};

REFCOUNT_HEADERS (Text, text);
PYBIND_HEADERS (Text, text);

Text *text_new (const char *filename);

const char *text_get_filename    (Text *txt);
const char *text_get_title       (Text *txt);
const char *text_get_author      (Text *txt);
const char *text_get_sort_title  (Text *txt);
const char *text_get_sort_author (Text *txt);

int    text_get_length    (Text *txt);
Token *text_get_token     (Text *txt, int i);

void text_preload (Text *txt);

#endif /* __TEXT_H__ */

