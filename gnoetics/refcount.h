/* -*- Mode: C; tab-width: 4; indent-tab-modes: nil; c-basic-offset: 4 -*- */

/*
 * Confidential Proprietary Material Exclusively Owned by EMC Capital
 * Management.  (C) EMC Capital Management 2003.  All Rights Reserved.
 */

#ifndef __REFCOUNT_H__
#define __REFCOUNT_H__

#include <glib.h>

/*
  Macro-fu for generating boilerplate code for refcounted structs.
*/
 

#define REFCOUNT_BODY \
/* struct { ... */    \
     int _refs;       \
     void (*_ref_hook) (gpointer, int new_ref_count); \
     void (*_unref_hook) (gpointer, int new_ref_count); \
/* };           */    

#define REFCOUNT_INIT(x) { \
  (x)->_refs = 1;          \
  (x)->_ref_hook = NULL;   \
  (x)->_unref_hook = NULL; \
}

#define REFCOUNT_HEADERS(type, prefix) \
type *prefix##_ref   (type *);         \
void  prefix##_unref (type *);

#define REFCOUNT_CODE(type, prefix)                \
static void prefix##_dealloc (type *);             \
                                                   \
type *                                             \
prefix##_ref (type *x)                             \
{                                                  \
	if (x != NULL) {                               \
		g_return_val_if_fail (x->_refs > 0, NULL); \
		++x->_refs;                                \
        if (x->_ref_hook != NULL)                  \
            x->_ref_hook (x, x->_refs);            \
	}                                              \
	return x;                                      \
}                                                  \
                                                   \
void                                               \
prefix##_unref (type *x)                           \
{                                                  \
	if (x != NULL) {                               \
		g_return_if_fail (x->_refs > 0);           \
		--x->_refs;                                \
        if (x->_unref_hook != NULL)                \
            x->_unref_hook (x, x->_refs);          \
		if (x->_refs == 0) {                       \
			prefix##_dealloc (x);                  \
		}                                          \
	}                                              \
}

#define REFCOUNT_CODE_WITH_DEALLOC(type, prefix) \
REFCOUNT_CODE(type, prefix);                     \
static void                                      \
prefix##_dealloc (type *x)                       \
{                                                \
	g_free (x);                                  \
}

#endif /* __REFCOUNT_H__ */

