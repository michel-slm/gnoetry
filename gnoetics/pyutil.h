/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __PYUTIL_H__
#define __PYUTIL_H__

#include <Python.h>

/* default object magic */
void      py_default_dealloc (PyObject *);
void      py_default_free    (void     *);
PyObject *py_default_alloc   (PyTypeObject *, int);

void pyutil_register_type (PyObject *dict,
                           PyTypeObject *type);
		      

#endif /* __PYUTIL_H__ */

