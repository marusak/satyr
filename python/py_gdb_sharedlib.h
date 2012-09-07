/*
    py_gdb_sharedlib.h

    Copyright (C) 2010, 2011, 2012  Red Hat, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef BTPARSER_PY_GDB_SHAREDLIB_H
#define BTPARSER_PY_GDB_SHAREDLIB_H

/**
 * @file
 * @brief Python bindings for GDB sharedlib.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <Python.h>
#include <structmember.h>

extern PyTypeObject btp_py_gdb_sharedlib_type;

struct btp_py_gdb_sharedlib
{
    PyObject_HEAD
    struct btp_gdb_sharedlib *sharedlib;
    int syms_ok;
    int syms_wrong;
    int syms_not_found;
};

/* constructor */
PyObject *btp_py_gdb_sharedlib_new(PyTypeObject *object,
                                   PyObject *args,
                                   PyObject *kwds);

/* destructor */
void btp_py_gdb_sharedlib_free(PyObject *object);

/* str */
PyObject *btp_py_gdb_sharedlib_str(PyObject *self);

/* getters & setters */
PyObject *btp_py_gdb_sharedlib_get_from(PyObject *self, PyObject *args);
PyObject *btp_py_gdb_sharedlib_set_from(PyObject *self, PyObject *args);
PyObject *btp_py_gdb_sharedlib_get_to(PyObject *self, PyObject *args);
PyObject *btp_py_gdb_sharedlib_set_to(PyObject *self, PyObject *args);
PyObject *btp_py_gdb_sharedlib_get_soname(PyObject *self, PyObject *args);
PyObject *btp_py_gdb_sharedlib_set_soname(PyObject *self, PyObject *args);
PyObject *btp_py_gdb_sharedlib_get_symbols(PyObject *self, PyObject *args);
PyObject *btp_py_gdb_sharedlib_set_symbols(PyObject *self, PyObject *args);

#ifdef __cplusplus
}
#endif

#endif
