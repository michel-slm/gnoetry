/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "pyutil.h"

void
py_default_dealloc (PyObject *self)
{
	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

void
py_default_free (void *self)
{
	PyObject_Del (self);
}

PyObject *
py_default_alloc (PyTypeObject *type, int nitems)
{
	return PyObject_New (PyObject, type);
}

void
pyutil_register_type (PyObject *dict,
					  PyTypeObject *type)
{

	/* Set some useful default values in the type */

	if (type->tp_getattro == NULL)
		type->tp_getattro = PyObject_GenericGetAttr;

	if (type->tp_setattro == NULL)
		type->tp_setattro = PyObject_GenericSetAttr;

	if (type->tp_dealloc == NULL)
		type->tp_dealloc = py_default_dealloc;

	if (type->tp_free == NULL)
		type->tp_free = py_default_free;

	if (type->tp_alloc == NULL)
		type->tp_alloc = py_default_alloc;


	if (type->tp_flags == 0)
		type->tp_flags = Py_TPFLAGS_DEFAULT;

	PyType_Ready (type);
	PyDict_SetItemString (dict, type->tp_name, (PyObject *) type);
}
