
/* -*- Mode: C; tab-width: 4; indent-tab-modes: nil; c-basic-offset: 4 -*- */

/*
 * Confidential Proprietary Material Exclusively Owned by EMC Capital
 * Management.  (C) EMC Capital Management 2003.  All Rights Reserved.
 */

#ifndef __PYBIND_H__
#define __PYBIND_H__

#include <glib.h>
#include <Python.h>
#include "pyutil.h"

#define PYBIND_BODY             \
/* struct { ... */              \
    gboolean  _enable_wrapping; \
    PyObject *_py_wrapper;      \
/* };           */

#define PYBIND_HEADERS(type, prefix)             \
void       py_##prefix##_register (PyObject *);  \
PyObject * prefix##_to_py         (type *);      \
type *     prefix##_from_py       (PyObject *);  \
int        py_##prefix##_check    (PyObject *);

#define PYBIND_INIT(x) {           \
    (x)->_enable_wrapping = FALSE; \
}

#define PYBIND_INIT_WITH_WRAPPING(x) { \
    (x)->_enable_wrapping = TRUE;      \
    (x)->_py_wrapper = NULL;           \
}
 
#define PYBIND_CODE(type, prefix)                           \
                                                            \
typedef struct {                                            \
	PyObject_HEAD;                                          \
	type *_##prefix;                                        \
} Py##type;                                                 \
                                                            \
static PyTypeObject py_##prefix##_type_info = {             \
	PyObject_HEAD_INIT (&PyType_Type)                       \
	0,                                                      \
	#type,                                                  \
	sizeof (Py##type),                                      \
	0                                                       \
};                                                          \
                                                            \
static void                                                 \
py_##prefix##_ref_hook (gpointer p, int new_ref_count)      \
{                                                           \
	if (new_ref_count == 2) {                               \
		type *x = (type *) p;                               \
		PyObject *py_x = x->_py_wrapper;                    \
		Py_INCREF (py_x);                                   \
	}                                                       \
}                                                           \
                                                            \
static void                                                 \
py_##prefix##_unref_hook (gpointer p, int new_ref_count)    \
{                                                           \
	if (new_ref_count == 1) {                               \
		type *x = (type *) p;                               \
		PyObject *py_x = x->_py_wrapper;                    \
		Py_DECREF (py_x);                                   \
	}                                                       \
}                                                           \
                                                            \
static void                                                 \
py_##prefix##_wrap(type *x, PyObject *py_x)                 \
{                                                           \
    if (! x->_enable_wrapping)                              \
        return;                                             \
    g_assert (x->_py_wrapper == NULL);                      \
     x->_py_wrapper = py_x;                                 \
    if (x->_refs > 1)                                       \
       Py_INCREF (py_x);                                    \
	x->_ref_hook = py_##prefix##_ref_hook;                  \
	x->_unref_hook = py_##prefix##_unref_hook;              \
}                                                           \
                                                            \
PyObject *                                                  \
prefix##_to_py (type *x)                                    \
{                                                           \
	Py##type *py_x;                                         \
                                                            \
	if (x == NULL) {                                        \
		Py_INCREF (Py_None);                                \
		return Py_None;                                     \
	}                                                       \
                                                            \
	py_x = PyObject_New (Py##type,                          \
						 &py_##prefix##_type_info);         \
	py_x->_##prefix = prefix##_ref(x);                      \
    py_##prefix##_wrap (x, (PyObject *) py_x);              \
                                                            \
	return (PyObject *) py_x;                               \
}                                                           \
                                                            \
type *                                                      \
prefix##_from_py (PyObject *obj)                            \
{                                                           \
	return ((Py##type *) obj)->_##prefix;                   \
}                                                           \
                                                            \
int                                                         \
py_##prefix##_check (PyObject *obj)                         \
{                                                           \
	return PyObject_TypeCheck (obj,                         \
							   &py_##prefix##_type_info);   \
}                                                           \
                                                            \
static void                                                 \
py_##prefix##_dealloc (PyObject *py_x)                      \
{                                                           \
	type *x = prefix##_from_py (py_x);                      \
	prefix##_unref (x);                                     \
	py_default_dealloc (py_x);                              \
}                                                           \
                                                            \
static type *py_##prefix##_assemble (PyObject*, PyObject*); \
                                                            \
static PyObject *                                           \
py_##prefix##_new (PyTypeObject *pto,                       \
				   PyObject *args, PyObject *kwds)          \
{                                                           \
	type *x;                                                \
	Py##type *py_x;                                         \
                                                            \
	x = py_##prefix##_assemble (args, kwds);                \
	if (x == NULL)                                          \
		return NULL;                                        \
                                                            \
	py_x = (Py##type *) pto->tp_alloc (pto, 0);             \
	py_x->_##prefix = x;                                    \
    py_##prefix##_wrap (x, (PyObject *) py_x);              \
                                                            \
	return (PyObject *) py_x;                               \
}

#define PYBIND_REGISTER_CODE(type, prefix, dict)             \
py_##prefix##_type_info.tp_methods = py_##prefix##_methods;  \
py_##prefix##_type_info.tp_dealloc = py_##prefix##_dealloc;  \
py_##prefix##_type_info.tp_new     = py_##prefix##_new;      \
pyutil_register_type (dict, &py_##prefix##_type_info);




#endif /* __PYBIND_H__ */

