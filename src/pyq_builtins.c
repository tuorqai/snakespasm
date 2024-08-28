/*
Copyright (C) 2024 tuorqai

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "quakedef.h"

//-------------------------------------------------------------------------------

/**
 * Utility function to emulate Python's print() behaviour.
 */
static int PyQ_PrintToBuffer(char *buffer, Py_ssize_t bufsize, PyObject *args, PyObject *kwargs)
{
    int i, num, pos = 0;

    char const *sepstr = " ", *endstr = "\n";
    int seplen = 1, endlen = 1;

    // Check if sep or end are set.
    if (kwargs) {
        PyObject *sep, *end;

        sep = PyDict_GetItemString(kwargs, "sep");

        if (sep) {
            if (PyUnicode_Check(sep)) {
                sepstr = PyUnicode_AsUTF8(sep);

                if (!sepstr) {
                    return -1;
                }

                seplen = strlen(sepstr);
            } else {
                PyErr_SetString(PyExc_TypeError, "sep must be a string");
                return -1;
            }
        }

        end = PyDict_GetItemString(kwargs, "end");

        if (end) {
            if (PyUnicode_Check(end)) {
                endstr = PyUnicode_AsUTF8(end);

                if (!endstr) {
                    return -1;
                }

                endlen = strlen(endstr);
            } else {
                PyErr_SetString(PyExc_TypeError, "end must be a string");
                return -1;
            }
        }
    }

    num = PyTuple_Size(args);

    for (i = 0; i < num; i++) {
        PyObject *item, *itemstr;
        char const *str;
        Py_ssize_t len;
        int truncated = 0;

        item = PyTuple_GetItem(args, i);
        itemstr = PyObject_Str(item); // new reference

        if (!itemstr) {
            return -1;
        }

        str = PyUnicode_AsUTF8AndSize(itemstr, &len);

        if (str) {
            if ((pos + len + seplen + endlen + 1) > bufsize) {
                // truncate
                Q_strncpy(&buffer[pos], str, bufsize - pos - 1);
                truncated = 1;
            } else {
                Q_strcpy(&buffer[pos], str);
                pos += len;

                if (i != num - 1) {
                    Q_strcpy(&buffer[pos], sepstr);
                    pos += seplen;
                }
            }
        }

        Py_DECREF(itemstr);

        if (truncated) {
            buffer[bufsize - 1] = '\0';
            return 1;
        }
    }

    Q_strcpy(&buffer[pos], endstr);
    return 0;
}

//-------------------------------------------------------------------------------
// quake.vec class

/**
 * quake.vec.__new__
 */
static PyObject *PyQ_vec_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyQ_vec *self = (PyQ_vec *) type->tp_alloc(type, 0);

    if (self) {
        self->v[0] = 0.f;
        self->v[1] = 0.f;
        self->v[2] = 0.f;
        self->p = &self->v;
        self->reprbuf[0] = '\0';
    }

    return (PyObject *) self;
}

/**
 * quake.vec.__init__
 */
static int PyQ_vec_init(PyQ_vec *self, PyObject *args, PyObject *kwds)
{
    Py_ssize_t p;
    float x, y, z;

    if (PyArg_ParseTuple(args, "n", &p)) {
        self->p = (vec3_t *) p;
        return 0;
    }

    PyErr_Clear();

    if (PyArg_ParseTuple(args, "fff", &x, &y, &z)) {
        self->v[0] = x;
        self->v[1] = y;
        self->v[2] = z;
        return 0;
    }

    PyErr_Clear();
    PyErr_SetString(PyExc_TypeError, "3 numbers or a pointer is required");

    return -1;
}

/**
 * quake.vec.__dealloc__
 */
static void PyQ_vec_dealloc(PyQ_vec *self)
{
    Py_TYPE(self)->tp_free((PyObject *) self);
}

/**
 * quake.vec.__repr__
 */
static PyObject *PyQ_vec_repr(PyQ_vec *self)
{
    snprintf(self->reprbuf, sizeof(self->reprbuf), "(%5.1f %5.1f %5.1f)",
             (*self->p)[0], (*self->p)[1], (*self->p)[2]);

    return PyUnicode_FromString(self->reprbuf);
}

/**
 * quake.vec.__richcmp__
 */
static PyObject *PyQ_vec_richcmp(PyQ_vec *a, PyQ_vec *b, int op)
{
    if (op == Py_EQ) {
        if (VectorCompare((*a->p), (*b->p))) {
            Py_RETURN_TRUE;
        } else {
            Py_RETURN_FALSE;
        }
    }

    Py_RETURN_NOTIMPLEMENTED;
}

/**
 * quake.vec.x: getter
 */
static PyObject *PyQ_vec_getx(PyQ_vec *self, void *closure)
{
    return Py_BuildValue("f", (*self->p)[0]);
}

/**
 * quake.vec.x: setter
 */
static int PyQ_vec_setx(PyQ_vec *self, PyObject *value, void *closure)
{
    double d = PyFloat_AsDouble(value);

    if (PyErr_Occurred()) {
        return -1;
    }

    (*self->p)[0] = (float) d;
    return 0;
}

/**
 * quake.vec.y: getter
 */
static PyObject *PyQ_vec_gety(PyQ_vec *self, void *closure)
{
    return Py_BuildValue("f", (*self->p)[1]);
}

/**
 * quake.vec.y: setter
 */
static int PyQ_vec_sety(PyQ_vec *self, PyObject *value, void *closure)
{
    double d = PyFloat_AsDouble(value);

    if (PyErr_Occurred()) {
        return -1;
    }

    (*self->p)[1] = (float) d;
    return 0;
}

/**
 * quake.vec.z: getter
 */
static PyObject *PyQ_vec_getz(PyQ_vec *self, void *closure)
{
    return Py_BuildValue("f", (*self->p)[2]);
}

/**
 * quake.vec.z: setter
 */
static int PyQ_vec_setz(PyQ_vec *self, PyObject *value, void *closure)
{
    double d = PyFloat_AsDouble(value);

    if (PyErr_Occurred()) {
        return -1;
    }

    (*self->p)[2] = (float) d;
    return 0;
}

/**
 * quake.vec.__add__
 */
static PyObject *PyQ_vec_add(PyQ_vec *a, PyQ_vec *b)
{
    vec3_t c;
    PyObject *args, *result;

    if (!PyObject_TypeCheck(b, &PyQ_vec_type)) {
        PyErr_SetString(PyExc_TypeError, "second operand is not a Vector");
        return NULL;
    }

    VectorAdd(*a->p, *b->p, c);

    if (!(args = Py_BuildValue("(fff)", c[0], c[1], c[2]))) {
        return NULL;
    }

    result = PyObject_CallObject((PyObject *) &PyQ_vec_type, args);
    Py_DECREF(args);

    return result;
}

/**
 * quake.vec.__sub__
 */
static PyObject *PyQ_vec_sub(PyQ_vec *a, PyQ_vec *b)
{
    vec3_t c;
    PyObject *args, *result;

    if (!PyObject_TypeCheck(b, &PyQ_vec_type)) {
        PyErr_SetString(PyExc_TypeError, "second operand is not a Vector");
        return NULL;
    }

    VectorSubtract(*a->p, *b->p, c);

    if (!(args = Py_BuildValue("(fff)", c[0], c[1], c[2]))) {
        return NULL;
    }

    result = PyObject_CallObject((PyObject *) &PyQ_vec_type, args);
    Py_DECREF(args);

    return result;
}

/**
 * quake.vec.__mul__
 */
static PyObject *PyQ_vec_mul(PyQ_vec *a, PyObject *b)
{
    vec3_t c;
    double s;
    PyObject *args, *result;

    s = PyFloat_AsDouble(b);

    if (PyErr_Occurred()) {
        return NULL;
    }

    VectorScale(*a->p, (vec_t) s, c);

    if (!(args = Py_BuildValue("(fff)", c[0], c[1], c[2]))) {
        return NULL;
    }

    result = PyObject_CallObject((PyObject *) &PyQ_vec_type, args);
    Py_DECREF(args);

    return result;
}

/**
 * quake.vec.__neg__
 */
static PyObject *PyQ_vec_neg(PyQ_vec *a)
{
    vec3_t c;
    PyObject *args, *result;

    VectorCopy(*a->p, c);
    VectorInverse(c);

    if (!(args = Py_BuildValue("(fff)", c[0], c[1], c[2]))) {
        return NULL;
    }

    result = PyObject_CallObject((PyObject *) &PyQ_vec_type, args);
    Py_DECREF(args);

    return result;
}

static PyGetSetDef PyQ_vec_getset[] = {
    { "x", (getter) PyQ_vec_getx, (setter) PyQ_vec_setx },
    { "y", (getter) PyQ_vec_gety, (setter) PyQ_vec_sety },
    { "z", (getter) PyQ_vec_getz, (setter) PyQ_vec_setz },
    { NULL },
};

static PyNumberMethods PyQ_vec_number_methods = {
    (binaryfunc) PyQ_vec_add,                   // nb_add
    (binaryfunc) PyQ_vec_sub,                   // nb_subtract
    (binaryfunc) PyQ_vec_mul,                   // nb_multiply
    NULL,                                       // nb_remainder
    NULL,                                       // nb_divmod
    NULL,                                       // nb_power
    (unaryfunc) PyQ_vec_neg,                    // nb_negative
    NULL,                                       // nb_positive
    NULL,                                       // nb_absolute
    NULL,                                       // nb_bool
    NULL,                                       // nb_invert
    NULL,                                       // nb_lshift
    NULL,                                       // nb_rshift
    NULL,                                       // nb_and
    NULL,                                       // nb_xor
    NULL,                                       // nb_or
    NULL,                                       // nb_int
    NULL,                                       // nb_reserved
    NULL,                                       // nb_float
    NULL,                                       // nb_inplace_add
    NULL,                                       // nb_inplace_subtract
    NULL,                                       // nb_inplace_multiply
    NULL,                                       // nb_inplace_remainder
    NULL,                                       // nb_inplace_power
    NULL,                                       // nb_inplace_lshift
    NULL,                                       // nb_inplace_rshift
    NULL,                                       // nb_inplace_and
    NULL,                                       // nb_inplace_xor
    NULL,                                       // nb_inplace_or
    NULL,                                       // nb_floor_divide
    NULL,                                       // nb_true_divide
    NULL,                                       // nb_inplace_floor_divide
    NULL,                                       // nb_inplace_true_divide
    NULL,                                       // nb_index
    NULL,                                       // nb_matrix_multiply
    NULL,                                       // nb_inplace_matrix_multiply
};

PyTypeObject PyQ_vec_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "quake.vec",                                // tp_name
    sizeof(PyQ_vec),                            // tp_basicsize
    0,                                          // tp_itemsize
    (destructor) PyQ_vec_dealloc,               // tp_dealloc
    0,                                          // tp_vectorcall_offset
    NULL,                                       // tp_getattr
    NULL,                                       // tp_setattr
    NULL,                                       // tp_as_async
    (reprfunc) PyQ_vec_repr,                    // tp_repr
    &PyQ_vec_number_methods,                    // tp_as_number
    NULL,                                       // tp_as_sequence
    NULL,                                       // tp_as_mapping
    NULL,                                       // tp_hash
    NULL,                                       // tp_call
    NULL,                                       // tp_str
    NULL,                                       // tp_getattro
    NULL,                                       // tp_setattro
    NULL,                                       // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   // tp_flags
    PyDoc_STR("Quake Vector"),                  // tp_doc
    NULL,                                       // tp_traverse
    NULL,                                       // tp_clear
    (richcmpfunc) PyQ_vec_richcmp,              // tp_richcompare
    0,                                          // tp_weaklistoffset
    NULL,                                       // tp_iter
    NULL,                                       // tp_iternext
    NULL,                                       // tp_methods
    NULL,                                       // tp_members
    PyQ_vec_getset,                             // tp_getset
    NULL,                                       // tp_base
    NULL,                                       // tp_dict
    NULL,                                       // tp_descr_get
    NULL,                                       // tp_descr_set
    0,                                          // tp_dictoffset
    (initproc) PyQ_vec_init,                    // tp_init
    NULL,                                       // tp_alloc
    PyQ_vec_new,                                // tp_new
    NULL,                                       // tp_free
    NULL,                                       // tp_is_gc
    NULL,                                       // tp_bases
    NULL,                                       // tp_mro
    NULL,                                       // tp_cache
    NULL,                                       // tp_subclasses
    NULL,                                       // tp_weaklist
    NULL,                                       // tp_del
    0,                                          // tp_version_tag
    NULL,                                       // tp_finalize
    NULL,                                       // tp_vectorcall
};

//-------------------------------------------------------------------------------
// quake._sv.edict class

/**
 * PyQ__sv_edict -> edict_t
 */
static edict_t *PyQ__sv_edict_get(PyQ__sv_edict *self)
{
    // check if server is inactive and not being spawned
    if (!sv.active && !PyQ_serverloading) {
        PyErr_SetString(PyExc_ReferenceError, "server is not running");
        return NULL;
    }

    // worldspawn (0 index) is valid between levels
    if (self->index == 0) {
        return sv.edicts;
    }

    // client entity is also valid
    if (self->index >= 1 && self->index <= svs.maxclients) {
        return EDICT_NUM(self->index);
    }

    // for anything else make basic checks

    if (self->servernumber != PyQ_servernumber) {
        PyErr_SetString(PyExc_ReferenceError, "edict was created in another server");
        return NULL;
    }

    if (self->index < 0 || self->index >= sv.num_edicts) {
        PyErr_SetString(PyExc_ReferenceError, "invalid edict");
        return NULL;
    }

    return EDICT_NUM(self->index);
}

/**
 * quake._sv.edict.__new__
 */
static PyQ__sv_edict *PyQ__sv_edict_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyQ__sv_edict *self = (PyQ__sv_edict *) type->tp_alloc(type, 0);

    if (!self) {
        return NULL;
    }

    self->servernumber = PyQ_servernumber;
    self->index = 0;

    return self;
}

/**
 * quake._sv.edict.__init__
 */
static int PyQ__sv_edict_init(PyQ__sv_edict *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_TypeError, "edict cannot be instantiated directly");
    return -1;
}

/**
 * quake._sv.edict.__dealloc__
 */
static void PyQ__sv_edict_dealloc(PyQ__sv_edict *self)
{
    Py_TYPE(self)->tp_free((PyObject *) self);
}

/**
 * quake._sv.edict.__repr__
 */
static PyObject *PyQ__sv_edict_repr(PyQ__sv_edict *self)
{
    edict_t *edict = PyQ__sv_edict_get(self);

    if (!edict) {
        PyErr_Clear();
        return PyUnicode_FromFormat("<invalid entity reference at %p>", self);
    }

    return PyUnicode_FromFormat("<edict #%d, classname \"%s\">",
                                self->index, PR_GetString(edict->v.classname));
}

/**
 * quake._sv.edict.__hash__
 */
static Py_hash_t PyQ__sv_edict_hash(PyQ__sv_edict *self)
{
    edict_t *edict = PyQ__sv_edict_get(self);

    if (!edict) {
        return -1;
    }

    return NUM_FOR_EDICT(edict);
}

/**
 * quake._sv.edict.__richcmp__
 */
static PyObject *PyQ__sv_edict_richcmp(PyQ__sv_edict *a, PyQ__sv_edict *b, int op)
{
    edict_t *_a, *_b;

    _a = PyQ__sv_edict_get(a);

    if (!_a) {
        return NULL;
    }

    _b = PyQ__sv_edict_get(b);

    if (!_b) {
        return NULL;
    }

    if (op == Py_EQ) {
        if (_a == _b) {
            Py_RETURN_TRUE;
        } else {
            Py_RETURN_FALSE;
        }
    }

    Py_RETURN_NOTIMPLEMENTED;
}

#define PyQ__sv_edict_NUMBER_GETTER(field) \
    static PyObject *PyQ__sv_edict_get##field(PyQ__sv_edict *self, void *closure) { \
        edict_t *edict = PyQ__sv_edict_get(self); \
        if (!edict) { \
            return NULL; \
        } \
        return PyFloat_FromDouble((double) edict->v.field); \
    }

#define PyQ__sv_edict_NUMBER_SETTER(field) \
    static int PyQ__sv_edict_set##field(PyQ__sv_edict *self, PyObject *value, void *closure) { \
        edict_t *edict; \
        double d; \
        edict = PyQ__sv_edict_get(self); \
        if (!edict) { \
            return -1; \
        } \
        d = PyFloat_AsDouble(value); \
        if (PyErr_Occurred()) { \
            return -1; \
        } \
        edict->v.field = (float) d; \
        return 0; \
    }

#define PyQ__sv_edict_BITSET_GETTER(field) \
    static PyObject *PyQ__sv_edict_get##field(PyQ__sv_edict *self, void *closure) { \
        edict_t *edict = PyQ__sv_edict_get(self); \
        if (!edict) { \
            return NULL; \
        } \
        return PyLong_FromDouble((double) edict->v.field); \
    }

#define PyQ__sv_edict_BITSET_SETTER(field) \
    static int PyQ__sv_edict_set##field(PyQ__sv_edict *self, PyObject *value, void *closure) { \
        edict_t *edict; \
        long n; \
        edict = PyQ__sv_edict_get(self); \
        if (!edict) { \
            return -1; \
        } \
        n = PyLong_AsLong(value); \
        if (PyErr_Occurred()) { \
            return -1; \
        } \
        edict->v.field = (float) n; \
        return 0; \
    }

#define PyQ__sv_edict_VECTOR_GETTER(field) \
    static PyObject *PyQ__sv_edict_get##field(PyQ__sv_edict *self, void *closure) { \
        edict_t *edict; \
        PyQ_vec *vec; \
        edict = PyQ__sv_edict_get(self); \
        if (!edict) { \
            return NULL; \
        } \
        vec = PyObject_New(PyQ_vec, &PyQ_vec_type); \
        if (!vec) { \
            return NULL; \
        } \
        vec->p = &edict->v.field; \
        return (PyObject *) vec; \
    }

#define PyQ__sv_edict_VECTOR_SETTER(field) \
    static int PyQ__sv_edict_set##field(PyQ__sv_edict *self, PyObject *value, void *closure) { \
        edict_t *edict; \
        PyQ_vec *vec; \
        edict = PyQ__sv_edict_get(self); \
        if (!edict) { \
            return -1; \
        } \
        if (!PyObject_TypeCheck(value, &PyQ_vec_type)) { \
            PyErr_SetString(PyExc_TypeError, "value must be vec"); \
            return -1; \
        } \
        vec = (PyQ_vec *) value; \
        edict->v.field[0] = *(vec->p)[0]; \
        edict->v.field[1] = *(vec->p)[1]; \
        edict->v.field[2] = *(vec->p)[2]; \
        return 0; \
    }

#define PyQ__sv_edict_STRING_GETTER(field) \
    static PyObject *PyQ__sv_edict_get##field(PyQ__sv_edict *self, void *closure) { \
        edict_t *edict = PyQ__sv_edict_get(self); \
        if (!edict) { \
            return NULL; \
        } \
        return PyUnicode_FromString(PR_GetString(edict->v.field)); \
    }

#define PyQ__sv_edict_STRING_SETTER(field) \
    static int PyQ__sv_edict_set##field(PyQ__sv_edict *self, PyObject *value, void *closure) { \
        edict_t *edict; \
        PyObject *bytes; \
        edict = PyQ__sv_edict_get(self); \
        if (!edict) { \
            return -1; \
        } \
        bytes = PyUnicode_AsUTF8String(value); \
        if (!bytes) { \
            return -1; \
        } \
        Q_strcpy(PyQ_string_storage[self->index].v.field, PyBytes_AsString(bytes)); \
        edict->v.field = PR_SetEngineString(PyQ_string_storage[self->index].v.field); \
        Py_DECREF(bytes); \
        return 0; \
    }

#define PyQ__sv_edict_ENTITY_GETTER(field) \
    static PyObject *PyQ__sv_edict_get##field(PyQ__sv_edict *self, void *closure) { \
        edict_t *edict; \
        PyQ__sv_edict *result; \
        edict = PyQ__sv_edict_get(self); \
        if (!edict) { \
            return NULL; \
        } \
        result = PyObject_New(PyQ__sv_edict, &PyQ__sv_edict_type); \
        if (!result) { \
            return NULL; \
        } \
        result->servernumber = PyQ_servernumber; \
        result->index = edict->v.field; \
        return (PyObject *) result; \
    }

#define PyQ__sv_edict_ENTITY_SETTER(field) \
    static int PyQ__sv_edict_set##field(PyQ__sv_edict *self, PyObject *value, void *closure) { \
        edict_t *selfedict, *valueedict; \
        selfedict = PyQ__sv_edict_get(self); \
        if (!selfedict) { \
            return -1; \
        } \
        if (!PyObject_TypeCheck(value, &PyQ__sv_edict_type)) { \
            PyErr_SetString(PyExc_TypeError, "value must be edict"); \
            return -1; \
        } \
        valueedict = PyQ__sv_edict_get((PyQ__sv_edict *) value); \
        if (!valueedict) { \
            return -1; \
        } \
        selfedict->v.field = NUM_FOR_EDICT(valueedict); \
        return 0; \
    }

PyQ__sv_edict_NUMBER_GETTER(modelindex)
PyQ__sv_edict_VECTOR_GETTER(absmin)
PyQ__sv_edict_VECTOR_GETTER(absmax)
PyQ__sv_edict_NUMBER_GETTER(ltime)
PyQ__sv_edict_NUMBER_GETTER(movetype)
PyQ__sv_edict_NUMBER_GETTER(solid)
PyQ__sv_edict_VECTOR_GETTER(origin)
PyQ__sv_edict_VECTOR_GETTER(oldorigin)
PyQ__sv_edict_VECTOR_GETTER(velocity)
PyQ__sv_edict_VECTOR_GETTER(angles)
PyQ__sv_edict_VECTOR_GETTER(avelocity)
PyQ__sv_edict_VECTOR_GETTER(punchangle)
PyQ__sv_edict_STRING_GETTER(classname)
PyQ__sv_edict_STRING_GETTER(model)
PyQ__sv_edict_NUMBER_GETTER(frame)
PyQ__sv_edict_NUMBER_GETTER(skin)
PyQ__sv_edict_BITSET_GETTER(effects)
PyQ__sv_edict_VECTOR_GETTER(mins)
PyQ__sv_edict_VECTOR_GETTER(maxs)
PyQ__sv_edict_VECTOR_GETTER(size)
PyQ__sv_edict_NUMBER_GETTER(touch)
PyQ__sv_edict_NUMBER_GETTER(use)
PyQ__sv_edict_NUMBER_GETTER(think)
PyQ__sv_edict_NUMBER_GETTER(blocked)
PyQ__sv_edict_NUMBER_GETTER(nextthink)
PyQ__sv_edict_ENTITY_GETTER(groundentity)
PyQ__sv_edict_NUMBER_GETTER(health)
PyQ__sv_edict_NUMBER_GETTER(frags)
PyQ__sv_edict_NUMBER_GETTER(weapon)
PyQ__sv_edict_STRING_GETTER(weaponmodel)
PyQ__sv_edict_NUMBER_GETTER(weaponframe)
PyQ__sv_edict_NUMBER_GETTER(currentammo)
PyQ__sv_edict_NUMBER_GETTER(ammo_shells)
PyQ__sv_edict_NUMBER_GETTER(ammo_nails)
PyQ__sv_edict_NUMBER_GETTER(ammo_rockets)
PyQ__sv_edict_NUMBER_GETTER(ammo_cells)
PyQ__sv_edict_BITSET_GETTER(items)
PyQ__sv_edict_NUMBER_GETTER(takedamage)
PyQ__sv_edict_ENTITY_GETTER(chain)
PyQ__sv_edict_NUMBER_GETTER(deadflag)
PyQ__sv_edict_VECTOR_GETTER(view_ofs)
PyQ__sv_edict_NUMBER_GETTER(button0)
PyQ__sv_edict_NUMBER_GETTER(button1)
PyQ__sv_edict_NUMBER_GETTER(button2)
PyQ__sv_edict_NUMBER_GETTER(impulse)
PyQ__sv_edict_NUMBER_GETTER(fixangle)
PyQ__sv_edict_VECTOR_GETTER(v_angle)
PyQ__sv_edict_NUMBER_GETTER(idealpitch)
PyQ__sv_edict_STRING_GETTER(netname)
PyQ__sv_edict_NUMBER_GETTER(enemy)
PyQ__sv_edict_BITSET_GETTER(flags)
PyQ__sv_edict_NUMBER_GETTER(colormap)
PyQ__sv_edict_NUMBER_GETTER(team)
PyQ__sv_edict_NUMBER_GETTER(max_health)
PyQ__sv_edict_NUMBER_GETTER(teleport_time)
PyQ__sv_edict_NUMBER_GETTER(armortype)
PyQ__sv_edict_NUMBER_GETTER(armorvalue)
PyQ__sv_edict_NUMBER_GETTER(waterlevel)
PyQ__sv_edict_NUMBER_GETTER(watertype)
PyQ__sv_edict_NUMBER_GETTER(ideal_yaw)
PyQ__sv_edict_NUMBER_GETTER(yaw_speed)
PyQ__sv_edict_ENTITY_GETTER(aiment)
PyQ__sv_edict_ENTITY_GETTER(goalentity)
PyQ__sv_edict_BITSET_GETTER(spawnflags)
PyQ__sv_edict_STRING_GETTER(target)
PyQ__sv_edict_STRING_GETTER(targetname)
PyQ__sv_edict_NUMBER_GETTER(dmg_take)
PyQ__sv_edict_NUMBER_GETTER(dmg_save)
PyQ__sv_edict_ENTITY_GETTER(dmg_inflictor)
PyQ__sv_edict_ENTITY_GETTER(owner)
PyQ__sv_edict_VECTOR_GETTER(movedir)
PyQ__sv_edict_STRING_GETTER(message)
PyQ__sv_edict_NUMBER_GETTER(sounds)
PyQ__sv_edict_STRING_GETTER(noise)
PyQ__sv_edict_STRING_GETTER(noise1)
PyQ__sv_edict_STRING_GETTER(noise2)
PyQ__sv_edict_STRING_GETTER(noise3)

PyQ__sv_edict_NUMBER_SETTER(ltime)
PyQ__sv_edict_NUMBER_SETTER(movetype)
PyQ__sv_edict_NUMBER_SETTER(solid)
PyQ__sv_edict_VECTOR_SETTER(velocity)
PyQ__sv_edict_VECTOR_SETTER(angles)
PyQ__sv_edict_VECTOR_SETTER(avelocity)
PyQ__sv_edict_VECTOR_SETTER(punchangle)
PyQ__sv_edict_STRING_SETTER(classname)
PyQ__sv_edict_NUMBER_SETTER(frame)
PyQ__sv_edict_NUMBER_SETTER(skin)
PyQ__sv_edict_BITSET_SETTER(effects)
PyQ__sv_edict_NUMBER_SETTER(nextthink)
PyQ__sv_edict_ENTITY_SETTER(groundentity)
PyQ__sv_edict_NUMBER_SETTER(health)
PyQ__sv_edict_NUMBER_SETTER(frags)
PyQ__sv_edict_NUMBER_SETTER(weapon)
PyQ__sv_edict_STRING_SETTER(weaponmodel)
PyQ__sv_edict_NUMBER_SETTER(weaponframe)
PyQ__sv_edict_NUMBER_SETTER(currentammo)
PyQ__sv_edict_NUMBER_SETTER(ammo_shells)
PyQ__sv_edict_NUMBER_SETTER(ammo_nails)
PyQ__sv_edict_NUMBER_SETTER(ammo_rockets)
PyQ__sv_edict_NUMBER_SETTER(ammo_cells)
PyQ__sv_edict_BITSET_SETTER(items)
PyQ__sv_edict_NUMBER_SETTER(takedamage)
PyQ__sv_edict_ENTITY_SETTER(chain)
PyQ__sv_edict_NUMBER_SETTER(deadflag)
PyQ__sv_edict_VECTOR_SETTER(view_ofs)
PyQ__sv_edict_NUMBER_SETTER(button0)
PyQ__sv_edict_NUMBER_SETTER(button1)
PyQ__sv_edict_NUMBER_SETTER(button2)
PyQ__sv_edict_NUMBER_SETTER(impulse)
PyQ__sv_edict_NUMBER_SETTER(fixangle)
PyQ__sv_edict_VECTOR_SETTER(v_angle)
PyQ__sv_edict_NUMBER_SETTER(idealpitch)
PyQ__sv_edict_STRING_SETTER(netname)
PyQ__sv_edict_NUMBER_SETTER(enemy)
PyQ__sv_edict_BITSET_SETTER(flags)
PyQ__sv_edict_NUMBER_SETTER(colormap)
PyQ__sv_edict_NUMBER_SETTER(team)
PyQ__sv_edict_NUMBER_SETTER(max_health)
PyQ__sv_edict_NUMBER_SETTER(teleport_time)
PyQ__sv_edict_NUMBER_SETTER(armortype)
PyQ__sv_edict_NUMBER_SETTER(armorvalue)
PyQ__sv_edict_NUMBER_SETTER(waterlevel)
PyQ__sv_edict_NUMBER_SETTER(watertype)
PyQ__sv_edict_NUMBER_SETTER(ideal_yaw)
PyQ__sv_edict_NUMBER_SETTER(yaw_speed)
PyQ__sv_edict_ENTITY_SETTER(aiment)
PyQ__sv_edict_ENTITY_SETTER(goalentity)
PyQ__sv_edict_BITSET_SETTER(spawnflags)
PyQ__sv_edict_STRING_SETTER(target)
PyQ__sv_edict_STRING_SETTER(targetname)
PyQ__sv_edict_NUMBER_SETTER(dmg_take)
PyQ__sv_edict_NUMBER_SETTER(dmg_save)
PyQ__sv_edict_ENTITY_SETTER(dmg_inflictor)
PyQ__sv_edict_ENTITY_SETTER(owner)
PyQ__sv_edict_VECTOR_SETTER(movedir)
PyQ__sv_edict_STRING_SETTER(message)
PyQ__sv_edict_NUMBER_SETTER(sounds)
PyQ__sv_edict_STRING_SETTER(noise)
PyQ__sv_edict_STRING_SETTER(noise1)
PyQ__sv_edict_STRING_SETTER(noise2)
PyQ__sv_edict_STRING_SETTER(noise3)

static PyMethodDef PyQ__sv_edict_methods[] = {
    { NULL },
};

static PyGetSetDef PyQ__sv_edict_getset[] = {
    { "modelindex",     (getter) PyQ__sv_edict_getmodelindex },
    { "absmin",         (getter) PyQ__sv_edict_getabsmin },
    { "absmax",         (getter) PyQ__sv_edict_getabsmax },
    { "ltime",          (getter) PyQ__sv_edict_getltime,            (setter) PyQ__sv_edict_setltime },
    { "movetype",       (getter) PyQ__sv_edict_getmovetype,         (setter) PyQ__sv_edict_setmovetype },
    { "solid",          (getter) PyQ__sv_edict_getsolid,            (setter) PyQ__sv_edict_setsolid },
    { "origin",         (getter) PyQ__sv_edict_getorigin },
    { "oldorigin",      (getter) PyQ__sv_edict_getoldorigin },
    { "velocity",       (getter) PyQ__sv_edict_getvelocity,         (setter) PyQ__sv_edict_setvelocity },
    { "angles",         (getter) PyQ__sv_edict_getangles,           (setter) PyQ__sv_edict_setangles },
    { "avelocity",      (getter) PyQ__sv_edict_getavelocity,        (setter) PyQ__sv_edict_setavelocity },
    { "punchangle",     (getter) PyQ__sv_edict_getpunchangle,       (setter) PyQ__sv_edict_setpunchangle },
    { "classname",      (getter) PyQ__sv_edict_getclassname,        (setter) PyQ__sv_edict_setclassname },
    { "model",          (getter) PyQ__sv_edict_getmodel },
    { "frame",          (getter) PyQ__sv_edict_getframe,            (setter) PyQ__sv_edict_setframe },
    { "skin",           (getter) PyQ__sv_edict_getskin,             (setter) PyQ__sv_edict_setskin },
    { "effects",        (getter) PyQ__sv_edict_geteffects,          (setter) PyQ__sv_edict_seteffects },
    { "mins",           (getter) PyQ__sv_edict_getmins },
    { "maxs",           (getter) PyQ__sv_edict_getmaxs },
    { "size",           (getter) PyQ__sv_edict_getsize },
    { "touch",          (getter) PyQ__sv_edict_gettouch },
    { "use",            (getter) PyQ__sv_edict_getuse },
    { "think",          (getter) PyQ__sv_edict_getthink },
    { "blocked",        (getter) PyQ__sv_edict_getblocked },
    { "nextthink",      (getter) PyQ__sv_edict_getnextthink,        (setter) PyQ__sv_edict_setnextthink },
    { "groundentity",   (getter) PyQ__sv_edict_getgroundentity,     (setter) PyQ__sv_edict_setgroundentity },
    { "health",         (getter) PyQ__sv_edict_gethealth,           (setter) PyQ__sv_edict_sethealth },
    { "frags",          (getter) PyQ__sv_edict_getfrags,            (setter) PyQ__sv_edict_setfrags },
    { "weapon",         (getter) PyQ__sv_edict_getweapon,           (setter) PyQ__sv_edict_setweapon },
    { "weaponmodel",    (getter) PyQ__sv_edict_getweaponmodel,      (setter) PyQ__sv_edict_setweaponmodel },
    { "weaponframe",    (getter) PyQ__sv_edict_getweaponframe,      (setter) PyQ__sv_edict_setweaponframe },
    { "currentammo",    (getter) PyQ__sv_edict_getcurrentammo,      (setter) PyQ__sv_edict_setcurrentammo },
    { "ammo_shells",    (getter) PyQ__sv_edict_getammo_shells,      (setter) PyQ__sv_edict_setammo_shells },
    { "ammo_nails",     (getter) PyQ__sv_edict_getammo_nails,       (setter) PyQ__sv_edict_setammo_nails },
    { "ammo_rockets",   (getter) PyQ__sv_edict_getammo_rockets,     (setter) PyQ__sv_edict_setammo_rockets },
    { "ammo_cells",     (getter) PyQ__sv_edict_getammo_cells,       (setter) PyQ__sv_edict_setammo_cells },
    { "items",          (getter) PyQ__sv_edict_getitems,            (setter) PyQ__sv_edict_setitems },
    { "takedamage",     (getter) PyQ__sv_edict_gettakedamage,       (setter) PyQ__sv_edict_settakedamage },
    { "chain",          (getter) PyQ__sv_edict_getchain,            (setter) PyQ__sv_edict_setchain },
    { "deadflag",       (getter) PyQ__sv_edict_getdeadflag,         (setter) PyQ__sv_edict_setdeadflag },
    { "view_ofs",       (getter) PyQ__sv_edict_getview_ofs,         (setter) PyQ__sv_edict_setview_ofs },
    { "button0",        (getter) PyQ__sv_edict_getbutton0,          (setter) PyQ__sv_edict_setbutton0 },
    { "button1",        (getter) PyQ__sv_edict_getbutton1,          (setter) PyQ__sv_edict_setbutton1 },
    { "button2",        (getter) PyQ__sv_edict_getbutton2,          (setter) PyQ__sv_edict_setbutton2 },
    { "impulse",        (getter) PyQ__sv_edict_getimpulse,          (setter) PyQ__sv_edict_setimpulse },
    { "fixangle",       (getter) PyQ__sv_edict_getfixangle,         (setter) PyQ__sv_edict_setfixangle },
    { "v_angle",        (getter) PyQ__sv_edict_getv_angle,          (setter) PyQ__sv_edict_setv_angle },
    { "idealpitch",     (getter) PyQ__sv_edict_getidealpitch,       (setter) PyQ__sv_edict_setidealpitch },
    { "netname",        (getter) PyQ__sv_edict_getnetname,          (setter) PyQ__sv_edict_setnetname },
    { "enemy",          (getter) PyQ__sv_edict_getenemy,            (setter) PyQ__sv_edict_setenemy },
    { "flags",          (getter) PyQ__sv_edict_getflags,            (setter) PyQ__sv_edict_setflags },
    { "colormap",       (getter) PyQ__sv_edict_getcolormap,         (setter) PyQ__sv_edict_setcolormap },
    { "team",           (getter) PyQ__sv_edict_getteam,             (setter) PyQ__sv_edict_setteam },
    { "max_health",     (getter) PyQ__sv_edict_getmax_health,       (setter) PyQ__sv_edict_setmax_health },
    { "teleport_time",  (getter) PyQ__sv_edict_getteleport_time,    (setter) PyQ__sv_edict_setteleport_time },
    { "armortype",      (getter) PyQ__sv_edict_getarmortype,        (setter) PyQ__sv_edict_setarmortype },
    { "armorvalue",     (getter) PyQ__sv_edict_getarmorvalue,       (setter) PyQ__sv_edict_setarmorvalue },
    { "waterlevel",     (getter) PyQ__sv_edict_getwaterlevel,       (setter) PyQ__sv_edict_setwaterlevel },
    { "watertype",      (getter) PyQ__sv_edict_getwatertype,        (setter) PyQ__sv_edict_setwatertype },
    { "ideal_yaw",      (getter) PyQ__sv_edict_getideal_yaw,        (setter) PyQ__sv_edict_setideal_yaw },
    { "yaw_speed",      (getter) PyQ__sv_edict_getyaw_speed,        (setter) PyQ__sv_edict_setyaw_speed },
    { "aiment",         (getter) PyQ__sv_edict_getaiment,           (setter) PyQ__sv_edict_setaiment },
    { "goalentity",     (getter) PyQ__sv_edict_getgoalentity,       (setter) PyQ__sv_edict_setgoalentity },
    { "spawnflags",     (getter) PyQ__sv_edict_getspawnflags,       (setter) PyQ__sv_edict_setspawnflags },
    { "target",         (getter) PyQ__sv_edict_gettarget,           (setter) PyQ__sv_edict_settarget },
    { "targetname",     (getter) PyQ__sv_edict_gettargetname,       (setter) PyQ__sv_edict_settargetname },
    { "dmg_take",       (getter) PyQ__sv_edict_getdmg_take,         (setter) PyQ__sv_edict_setdmg_take },
    { "dmg_save",       (getter) PyQ__sv_edict_getdmg_save,         (setter) PyQ__sv_edict_setdmg_save },
    { "dmg_inflictor",  (getter) PyQ__sv_edict_getdmg_inflictor,    (setter) PyQ__sv_edict_setdmg_inflictor },
    { "owner",          (getter) PyQ__sv_edict_getowner,            (setter) PyQ__sv_edict_setowner },
    { "movedir",        (getter) PyQ__sv_edict_getmovedir,          (setter) PyQ__sv_edict_setmovedir },
    { "message",        (getter) PyQ__sv_edict_getmessage,          (setter) PyQ__sv_edict_setmessage },
    { "sounds",         (getter) PyQ__sv_edict_getsounds,           (setter) PyQ__sv_edict_setsounds },
    { "noise",          (getter) PyQ__sv_edict_getnoise,            (setter) PyQ__sv_edict_setnoise },
    { "noise1",         (getter) PyQ__sv_edict_getnoise1,           (setter) PyQ__sv_edict_setnoise1 },
    { "noise2",         (getter) PyQ__sv_edict_getnoise2,           (setter) PyQ__sv_edict_setnoise2 },
    { "noise3",         (getter) PyQ__sv_edict_getnoise3,           (setter) PyQ__sv_edict_setnoise3 },
    { NULL },
};

PyTypeObject PyQ__sv_edict_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "quake._sv.edict",                          // tp_name
    sizeof(PyQ__sv_edict),                      // tp_basicsize
    0,                                          // tp_itemsize
    (destructor) PyQ__sv_edict_dealloc,         // tp_dealloc
    0,                                          // tp_vectorcall_offset
    NULL,                                       // tp_getattr
    NULL,                                       // tp_setattr
    NULL,                                       // tp_as_async
    (reprfunc) PyQ__sv_edict_repr,              // tp_repr
    NULL,                                       // tp_as_number
    NULL,                                       // tp_as_sequence
    NULL,                                       // tp_as_mapping
    (hashfunc) PyQ__sv_edict_hash,              // tp_hash
    NULL,                                       // tp_call
    NULL,                                       // tp_str
    NULL,                                       // tp_getattro
    NULL,                                       // tp_setattro
    NULL,                                       // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   // tp_flags
    NULL,                                       // tp_doc
    NULL,                                       // tp_traverse
    NULL,                                       // tp_clear
    (richcmpfunc) PyQ__sv_edict_richcmp,        // tp_richcompare
    0,                                          // tp_weaklistoffset
    NULL,                                       // tp_iter
    NULL,                                       // tp_iternext
    PyQ__sv_edict_methods,                      // tp_methods
    NULL,                                       // tp_members
    PyQ__sv_edict_getset,                       // tp_getset
    NULL,                                       // tp_base
    NULL,                                       // tp_dict
    NULL,                                       // tp_descr_get
    NULL,                                       // tp_descr_set
    0,                                          // tp_dictoffset
    (initproc) PyQ__sv_edict_init,              // tp_init
    NULL,                                       // tp_alloc
    (newfunc) PyQ__sv_edict_new,                // tp_new
    NULL,                                       // tp_free
    NULL,                                       // tp_is_gc
    NULL,                                       // tp_bases
    NULL,                                       // tp_mro
    NULL,                                       // tp_cache
    NULL,                                       // tp_subclasses
    NULL,                                       // tp_weaklist
    NULL,                                       // tp_del
    0,                                          // tp_version_tag
    NULL,                                       // tp_finalize
    NULL,                                       // tp_vectorcall
};

//-------------------------------------------------------------------------------
// quake._sv class

typedef struct {
    PyObject_HEAD
} PyQ__sv;

/**
 * quake._sv.__dealloc__
 */
static void PyQ__sv_dealloc(PyObject *self)
{
    Py_TYPE(self)->tp_free(self);
}

/**
 * quake._sv.setorigin
 */
static PyObject *PyQ__sv_setorigin(PyObject *self, PyObject *args)
{
    PyQ__sv_edict *e;
    PyQ_vec *v;
    edict_t *edict;

    if (!PyArg_ParseTuple(args, "O!O!", &PyQ__sv_edict_type, &e, &PyQ_vec_type, &v)) {
        return NULL;
    }

    edict = PyQ__sv_edict_get(e);

    if (!edict) {
        return NULL;
    }

    VectorCopy(*v->p, edict->v.origin);
    SV_LinkEdict(edict, false);

    Py_RETURN_NONE;
}

/**
 * quake._sv.setmodel
 */
static PyObject *PyQ__sv_setmodel(PyObject *self, PyObject *args)
{
    PyQ__sv_edict *e;
    edict_t *edict;
    int index;
    char const *model, **cache;

    if (!PyArg_ParseTuple(args, "O!s", &PyQ__sv_edict_type, &e, &model)) {
        return NULL;
    }

    edict = PyQ__sv_edict_get(e);

    if (!edict) {
        return NULL;
    }

    index = 0;

    for (cache = sv.model_precache; *cache; cache++) {
        if (!Q_strcmp(*cache, model)) {
            break;
        }
        index++;
    }

    if (!*cache) {
        PyErr_SetString(PyExc_ValueError, "model not precached");
        return NULL;
    }

    edict->v.model = PR_SetEngineString(*cache);
    edict->v.modelindex = index;

    Py_RETURN_NONE;
}

/**
 * quake._sv.setsize
 */
static PyObject *PyQ__sv_setsize(PyObject *self, PyObject *args)
{
    PyQ__sv_edict *edict;
    PyQ_vec *mins, *maxs;
    edict_t *qedict;

    if (!PyArg_ParseTuple(args, "O!O!O!", &PyQ__sv_edict_type, &edict,
                          &PyQ_vec_type, &mins, &PyQ_vec_type, &maxs)) {
        return NULL;
    }

    qedict = PyQ__sv_edict_get(edict);

    if (!qedict) {
        return NULL;
    }

    VectorCopy(*mins->p, qedict->v.mins);
    VectorCopy(*maxs->p, qedict->v.maxs);
    VectorSubtract(*maxs->p, *mins->p, qedict->v.size);
    SV_LinkEdict(qedict, false);

    Py_RETURN_NONE;
}

/**
 * quake._sv.sound(edict, sample, chan=0, vol=1.0, attn=0)
 */
static PyObject *PyQ__sv_sound(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "edict", "sample", "chan", "vol", "attn", NULL };

    PyQ__sv_edict *e;
    char const *sample;
    int chan = 0;
    float vol = 1.0;
    int attn = 0;

    edict_t *edict;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!s|ifi", kwlist,
                                     &PyQ__sv_edict_type, &e, &sample,
                                     &chan, &vol, &attn)) {
        return NULL;
    }

    edict = PyQ__sv_edict_get(e);

    if (!edict) {
        return NULL;
    }

    if (sv.active) {
        SV_StartSound(edict, chan, sample, vol, attn);
    }

    Py_RETURN_NONE;
}

/**
 * quake._sv.spawn()
 */
static PyObject *PyQ__sv_spawn(PyObject *self, PyObject *args)
{
    PyQ__sv_edict *edict = PyObject_New(PyQ__sv_edict, &PyQ__sv_edict_type);

    if (!edict) {
        return NULL;
    }

    edict->servernumber = PyQ_servernumber;

    // if ED_Alloc() fails, it will shut down the entire server
    edict->index = NUM_FOR_EDICT(ED_Alloc());

    return (PyObject *) edict;
}

/**
 * quake.__sv.remove(ent)
 */
static PyObject *PyQ__sv_remove(PyObject *self, PyObject *args)
{
    PyQ__sv_edict *edict;
    edict_t *qedict;

    if (!PyArg_ParseTuple(args, "O!", &PyQ__sv_edict_type, &edict)) {
        return NULL;
    }

    qedict = PyQ__sv_edict_get(edict);

    if (!qedict) {
        return NULL;
    }

    ED_Free(qedict);
    Py_RETURN_NONE;
}

/**
 * quake._sv.precache_sound(name)
 */
static PyObject *PyQ__sv_precache_sound(PyObject *self, PyObject *args)
{
    int i;
    char const *name;

    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    for (i = 0; i < MAX_SOUNDS; i++) {
        if (!sv.sound_precache[i]) {
            sv.sound_precache[i] = name;
            Py_RETURN_NONE;
        }

        if (!strcmp(sv.sound_precache[i], name)) {
            Py_RETURN_NONE;
        }
    }

    PyErr_SetString(PyExc_RuntimeError, "precache_sound: overflow");
    return NULL;
}

/**
 * quake._sv.precache_sound(name)
 */
static PyObject *PyQ__sv_precache_model(PyObject *self, PyObject *args)
{
    int i;
    char const *name;

    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    for (i = 0; i < MAX_MODELS; i++) {
        if (!sv.model_precache[i]) {
            sv.model_precache[i] = name;
            sv.models[i] = Mod_ForName(name, true);
            Py_RETURN_NONE;
        }

        if (!strcmp(sv.model_precache[i], name)) {
            Py_RETURN_NONE;
        }
    }

    PyErr_SetString(PyExc_RuntimeError, "precache_model: overflow");
    return NULL;
}

/**
 * quake._sv.bprint(*args, sep=' ', end='\n')
 */
static PyObject *PyQ__sv_bprint(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char buffer[1024];

    if (!sv.active) {
        PyErr_SetString(PyExc_RuntimeError, "server is not running");
        return NULL;
    }

    if (PyQ_PrintToBuffer(buffer, sizeof(buffer), args, kwargs) == -1) {
        return NULL;
    }

    SV_BroadcastPrintf("%s", buffer);
    Py_RETURN_NONE;
}

/**
 * quake._sv.sprint(ent, *args, sep=' ', end='\n')
 */
static PyObject *PyQ__sv_sprint(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char buffer[1024];
    Py_ssize_t i, argslen;
    PyObject *newargs;
    PyQ__sv_edict *edict;
    int status;

    edict = (PyQ__sv_edict *) PyTuple_GetItem(args, 0);

    if (!edict || !PyObject_TypeCheck((PyObject *) edict, &PyQ__sv_edict_type)) {
        PyErr_SetString(PyExc_ValueError, "first parameter should be edict");
        return NULL;
    }

    if (edict->index < 1 || edict->index > svs.maxclients) {
        PyErr_SetString(PyExc_ValueError, "edict must be client");
        return NULL;
    }

    if (!sv.active) {
        PyErr_SetString(PyExc_RuntimeError, "server is not running");
        return NULL;
    }

    argslen = PyTuple_Size(args);
    newargs = PyTuple_New(argslen - 1);

    if (!newargs) {
        return NULL;
    }

    for (i = 1; i < argslen; i++) {
        PyObject *item = PyTuple_GetItem(args, i);
        Py_INCREF(item);
        PyTuple_SetItem(newargs, i - 1, item);
    }

    status = PyQ_PrintToBuffer(buffer, sizeof(buffer), newargs, kwargs);
    Py_DECREF(newargs);

    if (status == -1) {
        return NULL;
    }

    MSG_WriteChar(&svs.clients[edict->index - 1].message, svc_print);
    MSG_WriteString(&svs.clients[edict->index - 1].message, buffer);

    Py_RETURN_NONE;
}

/**
 * quake._sv.particle(org, dir=(0.0, 0.0, 0.0), color=0, count=1)
 */
static PyObject *PyQ__sv_particle(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "org", "dir", "color", "count", NULL };

    PyQ_vec *org, *dir = NULL;
    int color = 0;
    int count = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|O!ii", kwlist,
                                     &PyQ_vec_type, &org, &PyQ_vec_type, &dir,
                                     &color, &count)) {
        return NULL;
    }

    if (!dir) {
        dir = PyObject_New(PyQ_vec, &PyQ_vec_type);

        if (!dir) {
            return NULL;
        }

        dir->p = &dir->v;
        dir->v[0] = dir->v[1] = dir->v[2] = 0.f;
    }

    if (sv.active) {
        SV_StartParticle(*org->p, *dir->p, color, count);
    }

    Py_RETURN_NONE;
}

/**
 * quake._sv.centerprint(ent, *args, sep=' ', end='\n')
 */
static PyObject *PyQ__sv_centerprint(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char buffer[1024];
    Py_ssize_t i, argslen;
    PyObject *newargs;
    PyQ__sv_edict *edict;
    int status;

    edict = (PyQ__sv_edict *) PyTuple_GetItem(args, 0);

    if (!edict || !PyObject_TypeCheck((PyObject *) edict, &PyQ__sv_edict_type)) {
        PyErr_SetString(PyExc_ValueError, "first parameter should be edict");
        return NULL;
    }

    if (edict->index < 1 || edict->index > svs.maxclients) {
        PyErr_SetString(PyExc_ValueError, "edict must be client");
        return NULL;
    }

    if (!sv.active) {
        PyErr_SetString(PyExc_RuntimeError, "server is not running");
        return NULL;
    }

    argslen = PyTuple_Size(args);
    newargs = PyTuple_New(argslen - 1);

    if (!newargs) {
        return NULL;
    }

    for (i = 1; i < argslen; i++) {
        PyObject *item = PyTuple_GetItem(args, i);
        Py_INCREF(item);
        PyTuple_SetItem(newargs, i - 1, item);
    }

    status = PyQ_PrintToBuffer(buffer, sizeof(buffer), newargs, kwargs);
    Py_DECREF(newargs);

    if (status == -1) {
        return NULL;
    }

    MSG_WriteChar(&svs.clients[edict->index - 1].message, svc_centerprint);
    MSG_WriteString(&svs.clients[edict->index - 1].message, buffer);

    Py_RETURN_NONE;
}

/**
 * quake._sv.edict getter
 */
static PyObject *PyQ__sv_getedict(PyObject *self, void *closure)
{
    // I'm not really sure if this is a correct way to put
    // a class inside another class.
    return (PyObject *) &PyQ__sv_edict_type;
}

/**
 * quake._sv.edicts getter
 */
static PyObject *PyQ__sv_getedicts(PyObject *self, void *closure)
{
    int i;
    PyObject *list;

    if (!sv.active && !PyQ_serverloading) {
        PyErr_SetString(PyExc_RuntimeError, "server is not running");
        return NULL;
    }

    list = PyList_New(0);

    if (!list) {
        return NULL;
    }

    for (i = 0; i < sv.num_edicts; i++) {
        PyQ__sv_edict *edict;

        // ignore free ents
        if (EDICT_NUM(i)->free) {
            continue;
        }

        edict = PyObject_New(PyQ__sv_edict, &PyQ__sv_edict_type);

        if (!edict) {
            goto error;
        }

        if (i >= 1 && i <= svs.maxclients) {
            edict->servernumber = -1;
        } else {
            edict->servernumber = PyQ_servernumber;
        }

        edict->index = i;

        PyList_Append(list, (PyObject *) edict);
        Py_DECREF(edict);
    }

    return list;

error:
    Py_DECREF(list);
    return NULL;
}

/**
 * quake._sv.world getter
 */
static PyObject *PyQ__sv_getworld(PyObject *self, void *closure)
{
    PyQ__sv_edict *world;

    if (!sv.active && !PyQ_serverloading) {
        PyErr_SetString(PyExc_RuntimeError, "server is not running");
        return NULL;
    }

    world = PyObject_New(PyQ__sv_edict, &PyQ__sv_edict_type);

    if (!world) {
        return NULL;
    }

    world->servernumber = -1;
    world->index = 0;

    return (PyObject *) world;
}

/**
 * quake._sv.time getter
 */
static PyObject *PyQ__sv_gettime(PyObject *self, void *closure)
{
    return PyFloat_FromDouble(sv.time);
}

static PyMethodDef PyQ__sv_methods[] = {
    { "setorigin",          PyQ__sv_setorigin,                      METH_VARARGS },
    { "setmodel",           PyQ__sv_setmodel,                       METH_VARARGS },
    { "setsize",            PyQ__sv_setsize,                        METH_VARARGS },
    { "sound",              (PyCFunction) PyQ__sv_sound,            METH_VARARGS | METH_KEYWORDS },
    { "spawn",              PyQ__sv_spawn,                          METH_VARARGS },
    { "remove",             PyQ__sv_remove,                         METH_VARARGS },
    { "precache_sound",     PyQ__sv_precache_sound,                 METH_VARARGS },
    { "precache_model",     PyQ__sv_precache_model,                 METH_VARARGS },
    { "bprint",             (PyCFunction) PyQ__sv_bprint,           METH_VARARGS | METH_KEYWORDS },
    { "sprint",             (PyCFunction) PyQ__sv_sprint,           METH_VARARGS | METH_KEYWORDS },
    { "particle",           (PyCFunction) PyQ__sv_particle,         METH_VARARGS | METH_KEYWORDS },
    { "centerprint",        (PyCFunction) PyQ__sv_centerprint,      METH_VARARGS | METH_KEYWORDS },
    { NULL },
};

static PyGetSetDef PyQ__sv_getset[] = {
    { "edict",              PyQ__sv_getedict },
    { "edicts",             PyQ__sv_getedicts },
    { "world",              PyQ__sv_getworld },
    { "time",               PyQ__sv_gettime },
    { NULL },
};

static PyTypeObject PyQ__sv_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "quake._sv",                                // tp_name
    sizeof(PyQ__sv),                            // tp_basicsize
    0,                                          // tp_itemsize
    PyQ__sv_dealloc,                            // tp_dealloc
    0,                                          // tp_vectorcall_offset
    NULL,                                       // tp_getattr
    NULL,                                       // tp_setattr
    NULL,                                       // tp_as_async
    NULL,                                       // tp_repr
    NULL,                                       // tp_as_number
    NULL,                                       // tp_as_sequence
    NULL,                                       // tp_as_mapping
    NULL,                                       // tp_hash
    NULL,                                       // tp_call
    NULL,                                       // tp_str
    NULL,                                       // tp_getattro
    NULL,                                       // tp_setattro
    NULL,                                       // tp_as_buffer
    Py_TPFLAGS_DEFAULT,                         // tp_flags
    NULL,                                       // tp_doc
    NULL,                                       // tp_traverse
    NULL,                                       // tp_clear
    NULL,                                       // tp_richcompare
    0,                                          // tp_weaklistoffset
    NULL,                                       // tp_iter
    NULL,                                       // tp_iternext
    PyQ__sv_methods,                            // tp_methods
    NULL,                                       // tp_members
    PyQ__sv_getset,                             // tp_getset
    NULL,                                       // tp_base
    NULL,                                       // tp_dict
    NULL,                                       // tp_descr_get
    NULL,                                       // tp_descr_set
    0,                                          // tp_dictoffset
    NULL,                                       // tp_init
    NULL,                                       // tp_alloc
    NULL,                                       // tp_new
    NULL,                                       // tp_free
    NULL,                                       // tp_is_gc
    NULL,                                       // tp_bases
    NULL,                                       // tp_mro
    NULL,                                       // tp_cache
    NULL,                                       // tp_subclasses
    NULL,                                       // tp_weaklist
    NULL,                                       // tp_del
    0,                                          // tp_version_tag
    NULL,                                       // tp_finalize
    NULL,                                       // tp_vectorcall
};

//-------------------------------------------------------------------------------
// quake._cl class

typedef struct {
    PyObject_HEAD
} PyQ__cl;

/**
 * quake._cl.__dealloc__
 */
static void PyQ__cl_dealloc(PyObject *self)
{
    Py_TYPE(self)->tp_free(self);
}

/**
 * quake._cl.print
 */
static PyObject *PyQ__cl_print(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char buffer[4096]; // Con_Printf() has a 4k buffer
    int status;

    status = PyQ_PrintToBuffer(buffer, sizeof(buffer), args, kwargs);

    if (status == -1) {
        // Something went horribly wrong
        return NULL;
    }

    Con_Printf("%s", buffer);

    if (status == 1) {
        // Output got truncated, warn user
        Con_Printf("%c\nconsole output from the last Python command got truncated.\n", 2);
    }

    Py_RETURN_NONE;
}

static PyMethodDef PyQ__cl_methods[] = {
    { "print",              (PyCFunction) PyQ__cl_print,            METH_VARARGS | METH_KEYWORDS },
    { NULL },
};

static PyTypeObject PyQ__cl_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "quake._cl",                                // tp_name
    sizeof(PyQ__cl),                            // tp_basicsize
    0,                                          // tp_itemsize
    PyQ__cl_dealloc,                            // tp_dealloc
    0,                                          // tp_vectorcall_offset
    NULL,                                       // tp_getattr
    NULL,                                       // tp_setattr
    NULL,                                       // tp_as_async
    NULL,                                       // tp_repr
    NULL,                                       // tp_as_number
    NULL,                                       // tp_as_sequence
    NULL,                                       // tp_as_mapping
    NULL,                                       // tp_hash
    NULL,                                       // tp_call
    NULL,                                       // tp_str
    NULL,                                       // tp_getattro
    NULL,                                       // tp_setattro
    NULL,                                       // tp_as_buffer
    Py_TPFLAGS_DEFAULT,                         // tp_flags
    NULL,                                       // tp_doc
    NULL,                                       // tp_traverse
    NULL,                                       // tp_clear
    NULL,                                       // tp_richcompare
    0,                                          // tp_weaklistoffset
    NULL,                                       // tp_iter
    NULL,                                       // tp_iternext
    PyQ__cl_methods,                            // tp_methods
    NULL,                                       // tp_members
    NULL,                                       // tp_getset
    NULL,                                       // tp_base
    NULL,                                       // tp_dict
    NULL,                                       // tp_descr_get
    NULL,                                       // tp_descr_set
    0,                                          // tp_dictoffset
    NULL,                                       // tp_init
    NULL,                                       // tp_alloc
    NULL,                                       // tp_new
    NULL,                                       // tp_free
    NULL,                                       // tp_is_gc
    NULL,                                       // tp_bases
    NULL,                                       // tp_mro
    NULL,                                       // tp_cache
    NULL,                                       // tp_subclasses
    NULL,                                       // tp_weaklist
    NULL,                                       // tp_del
    0,                                          // tp_version_tag
    NULL,                                       // tp_finalize
    NULL,                                       // tp_vectorcall
};

//------------------------------------------------------------------------------
// quake module

/**
 * makevectors(v: vec) -> (vec, vec, vec)
 */
static PyObject *PyQ_makevectors(PyObject *self, PyObject *args)
{
    PyQ_vec *vec;

    PyQ_vec *forward = NULL;
    PyQ_vec *right = NULL;
    PyQ_vec *up = NULL;

    PyObject *result = NULL;

    if (!PyArg_ParseTuple(args, "O!", &PyQ_vec_type, &vec)) {
        return NULL;
    }

    if (!(forward = PyObject_New(PyQ_vec, &PyQ_vec_type))) {
        goto end;
    }

    if (!(right = PyObject_New(PyQ_vec, &PyQ_vec_type))) {
        goto end;
    }

    if (!(up = PyObject_New(PyQ_vec, &PyQ_vec_type))) {
        goto end;
    }

    forward->p = &forward->v;
    right->p = &right->v;
    up->p = &up->v;

    AngleVectors(*vec->p, *forward->p, *right->p, *up->p);

    result = PyTuple_Pack(3, forward, right, up);

end:
    Py_XDECREF(forward);
    Py_XDECREF(right);
    Py_XDECREF(up);

    return result;
}

/**
 * normalize(v: vec) -> vec
 */
static PyObject *PyQ_normalize(PyObject *self, PyObject *args)
{
    PyQ_vec *vec, *result;

    float x, y, z;
    double len;

    if (!PyArg_ParseTuple(args, "O!", &PyQ_vec_type, &vec)) {
        return NULL;
    }

    if (!(result = PyObject_New(PyQ_vec, &PyQ_vec_type))) {
        return NULL;
    }

    x = (*vec->p)[0];
    y = (*vec->p)[1];
    z = (*vec->p)[2];

    len = sqrt(x * x + y * y + z * z);

    result->p = &result->v;

    if (len == 0.0) {
        result->v[0] = 0.f;
        result->v[1] = 0.f;
        result->v[2] = 0.f;
    } else {
        result->v[0] = x / len;
        result->v[1] = y / len;
        result->v[2] = z / len;
    }

    return (PyObject *) result;
}

/**
 * vlen(v: vec) -> float
 */
static PyObject *PyQ_vlen(PyObject *self, PyObject *args)
{
    PyQ_vec *vec;

    float x, y, z;
    double len;

    if (!PyArg_ParseTuple(args, "O!", &PyQ_vec_type, &vec)) {
        return NULL;
    }

    x = (*vec->p)[0];
    y = (*vec->p)[1];
    z = (*vec->p)[2];

    len = sqrt(x * x + y * y + z * z);

    return PyFloat_FromDouble(len);
}

/**
 * vectoyaw(v: vec) -> float
 */
static PyObject *PyQ_vectoyaw(PyObject *self, PyObject *args)
{
    PyQ_vec *vec;
    float x, y;
    double yaw;

    if (!PyArg_ParseTuple(args, "O!", &PyQ_vec_type, &vec)) {
        return NULL;
    }

    x = (*vec->p)[0];
    y = (*vec->p)[1];

    if (y == 0.f && x == 0.f) {
        return PyFloat_FromDouble(0.0);
    }

    yaw = floor(atan2(y, x) * 180 / M_PI);

    if (yaw < 0.0) {
        yaw += 360.0;
    }

    return PyFloat_FromDouble(yaw);
}

/**
 * vectoangles(v: vec) -> vec
 */
static PyObject *PyQ_vectoangles(PyObject *self, PyObject *args)
{
    PyQ_vec *vec, *result;
    float x, y, z;
    float yaw, pitch, forward;

    if (!PyArg_ParseTuple(args, "O!", &PyQ_vec_type, &vec)) {
        return NULL;
    }

    if (!(result = PyObject_New(PyQ_vec, &PyQ_vec_type))) {
        return NULL;
    }

    x = (*vec->p)[0];
    y = (*vec->p)[1];
    z = (*vec->p)[2];

    if (y == 0.f && x == 0.f) {
        yaw = 0.f;

        if (z > 0.f) {
            pitch = 90.f;
        } else {
            pitch = 270.f;
        }
    } else {
        yaw = floorf(atan2f(y, x) * 180.f / M_PI);

        if (yaw < 0.f) {
            yaw += 360.f;
        }

        forward = sqrtf(x * x + y * y);
        pitch = floorf(atan2f(z, forward) * 180.f / M_PI);

        if (pitch < 0.f) {
            pitch += 360.f;
        }
    }

    result->p = &result->v;

    result->v[0] = pitch;
    result->v[1] = yaw;
    result->v[2] = 0.f;

    return (PyObject *) result;
}

/**
 * dprint(*args, sep=' ', end='\n')
 */
static PyObject *PyQ_dprint(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char buffer[1024];

    if (PyQ_PrintToBuffer(buffer, sizeof(buffer), args, kwargs) == -1) {
        return NULL;
    }

    Con_DPrintf("%s", buffer);

    Py_RETURN_NONE;
}

/**
 * cvar(name: str) -> float
 */
static PyObject *PyQ_cvar(PyObject *self, PyObject *args)
{
    char const *name;

    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    return PyFloat_FromDouble((double) Cvar_VariableValue(name));
}

/**
 * localcmd(line: str)
 */
static PyObject *PyQ_localcmd(PyObject *self, PyObject *args)
{
    char const *line;

    if (!PyArg_ParseTuple(args, "s", &line)) {
        return NULL;
    }

    Cbuf_AddText(line);
    Cbuf_AddText("\n");

    Py_RETURN_NONE;
}

static PyMethodDef quake_methods[] = {
    { "makevectors",    PyQ_makevectors,                METH_VARARGS },
    { "normalize",      PyQ_normalize,                  METH_VARARGS },
    { "vlen",           PyQ_vlen,                       METH_VARARGS },
    { "vectoyaw",       PyQ_vectoyaw,                   METH_VARARGS },
    { "vectoangles",    PyQ_vectoangles,                METH_VARARGS },
    { "dprint",         (PyCFunction) PyQ_dprint,       METH_VARARGS | METH_KEYWORDS },
    { "cvar",           PyQ_cvar,                       METH_VARARGS },
    { "localcmd",       PyQ_localcmd,                   METH_VARARGS },
    { NULL },
};

static PyModuleDef quake_module = {
    PyModuleDef_HEAD_INIT,
    "quake",                        // m_name
    NULL,                           // m_doc
    -1,                             // m_size
    quake_methods,                  // m_methods
    NULL,                           // m_slots
    NULL,                           // m_traverse
    NULL,                           // m_clear
    NULL,                           // m_free
};

struct PyQ_namevalue
{
    char const *name;
    int value;
};

PyObject *PyQ_quake_init(void)
{
    PyObject *module;

    PyQ__sv *sv = NULL;
    PyQ__cl *cl = NULL;

    if (PyType_Ready(&PyQ_vec_type) == -1) {
        return NULL;
    }

    if (PyType_Ready(&PyQ__sv_edict_type) == -1) {
        return NULL;
    }

    if (PyType_Ready(&PyQ__sv_type) == -1) {
        return NULL;
    }

    if (PyType_Ready(&PyQ__cl_type) == -1) {
        return NULL;
    }

    module = PyModule_Create(&quake_module);

    if (!module) {
        return NULL;
    }

    Py_INCREF(&PyQ_vec_type);
    Py_INCREF(&PyQ__sv_edict_type);
    Py_INCREF(&PyQ__sv_type);
    Py_INCREF(&PyQ__cl_type);

    struct PyQ_namevalue constant_list[] = {
        { "IT_AXE", IT_AXE },
        { "IT_SHOTGUN", IT_SHOTGUN },
        { "IT_SUPER_SHOTGUN", IT_SUPER_SHOTGUN },
        { "IT_NAILGUN", IT_NAILGUN },
        { "IT_SUPER_NAILGUN", IT_SUPER_NAILGUN },
        { "IT_GRENADE_LAUNCHER", IT_GRENADE_LAUNCHER },
        { "IT_ROCKET_LAUNCHER", IT_ROCKET_LAUNCHER },
        { "IT_LIGHTNING", IT_LIGHTNING },
        { "IT_SUPER_LIGHTNING", IT_SUPER_LIGHTNING },
        { "IT_SHELLS", IT_SHELLS },
        { "IT_NAILS", IT_NAILS },
        { "IT_ROCKETS", IT_ROCKETS },
        { "IT_CELLS", IT_CELLS },
        { "IT_AXE", IT_AXE },
        { "IT_ARMOR1", IT_ARMOR1 },
        { "IT_ARMOR2", IT_ARMOR2 },
        { "IT_ARMOR3", IT_ARMOR3 },
        { "IT_SUPERHEALTH", IT_SUPERHEALTH },
        { "IT_KEY1", IT_KEY1 },
        { "IT_KEY2", IT_KEY2 },
        { "IT_INVISIBILITY", IT_INVISIBILITY },
        { "IT_INVULNERABILITY", IT_INVULNERABILITY },
        { "IT_SUIT", IT_SUIT },
        { "IT_QUAD", IT_QUAD },
        { "IT_SIGIL1", IT_SIGIL1 },
        { "IT_SIGIL2", IT_SIGIL2 },
        { "IT_SIGIL3", IT_SIGIL3 },
        { "IT_SIGIL4", IT_SIGIL4 },
        { "MOVETYPE_NONE", MOVETYPE_NONE },
        { "MOVETYPE_ANGLENOCLIP", MOVETYPE_ANGLENOCLIP },
        { "MOVETYPE_ANGLECLIP", MOVETYPE_ANGLECLIP },
        { "MOVETYPE_WALK", MOVETYPE_WALK },
        { "MOVETYPE_STEP", MOVETYPE_STEP },
        { "MOVETYPE_FLY", MOVETYPE_FLY },
        { "MOVETYPE_TOSS", MOVETYPE_TOSS },
        { "MOVETYPE_PUSH", MOVETYPE_PUSH },
        { "MOVETYPE_NOCLIP", MOVETYPE_NOCLIP },
        { "MOVETYPE_FLYMISSILE", MOVETYPE_FLYMISSILE },
        { "MOVETYPE_BOUNCE", MOVETYPE_BOUNCE },
        { "MOVETYPE_GIB", MOVETYPE_GIB },
        { "SOLID_NOT", SOLID_NOT },
        { "SOLID_TRIGGER", SOLID_TRIGGER },
        { "SOLID_BBOX", SOLID_BBOX },
        { "SOLID_SLIDEBOX", SOLID_SLIDEBOX },
        { "SOLID_BSP", SOLID_BSP },
        { "DEAD_NO", DEAD_NO },
        { "DEAD_DYING", DEAD_DYING },
        { "DEAD_DEAD", DEAD_DEAD },
        { "FL_FLY", FL_FLY },
        { "FL_SWIM", FL_SWIM },
        { "FL_CONVEYOR", FL_CONVEYOR },
        { "FL_CLIENT", FL_CLIENT },
        { "FL_INWATER", FL_INWATER },
        { "FL_MONSTER", FL_MONSTER },
        { "FL_GODMODE", FL_GODMODE },
        { "FL_NOTARGET", FL_NOTARGET },
        { "FL_ITEM", FL_ITEM },
        { "FL_ONGROUND", FL_ONGROUND },
        { "FL_PARTIALGROUND", FL_PARTIALGROUND },
        { "FL_WATERJUMP", FL_WATERJUMP },
        { "FL_JUMPRELEASED", FL_JUMPRELEASED },
        { NULL },
    };

    for (struct PyQ_namevalue *c = constant_list; c->name != NULL; c++) {
        if (PyModule_AddIntConstant(module, c->name, c->value) == -1) {
            goto error;
        }
    }

    if (PyModule_AddObject(module, "vec", (PyObject *) &PyQ_vec_type) == -1) {
        goto error;
    }

    sv = PyObject_New(PyQ__sv, &PyQ__sv_type);

    if (!sv || PyModule_AddObject(module, "sv", (PyObject *) sv) == -1) {
        goto error;
    }

    cl = PyObject_New(PyQ__cl, &PyQ__cl_type);

    if (!cl || PyModule_AddObject(module, "cl", (PyObject *) cl) == -1) {
        goto error;
    }

    return module;

error:
    Py_XDECREF(sv);
    Py_DECREF(&PyQ__cl_type);
    Py_DECREF(&PyQ__sv_type);
    Py_DECREF(&PyQ__sv_edict_type);
    Py_DECREF(&PyQ_vec_type);
    Py_DECREF(module);

    return NULL;
}

//------------------------------------------------------------------------------
// 'quakecl' module

static PyObject *quakecl__console_print(PyObject *self, PyObject *args)
{
    char const *msg;

    if (!PyArg_ParseTuple(args, "s", &msg)) {
        return NULL;
    }

    Con_Printf("%s", msg);
    Py_RETURN_NONE;
}

static PyObject *quakecl__console_error(PyObject *self, PyObject *args)
{
    char const *msg;

    if (!PyArg_ParseTuple(args, "s", &msg)) {
        return NULL;
    }

    Con_Printf("%c%s", 2, msg);
    Py_RETURN_NONE;
}

static PyMethodDef quakecl_methods[] = {
    { "console_print", quakecl__console_print, METH_VARARGS },
    { "console_error", quakecl__console_error, METH_VARARGS },
    { NULL },
};

static PyModuleDef quakecl_module = {
    PyModuleDef_HEAD_INIT,
    "quakecl",                      // m_name
    NULL,                           // m_doc
    -1,                             // m_size
    quakecl_methods,                // m_methods
    NULL,                           // m_slots
    NULL,                           // m_traverse
    NULL,                           // m_clear
    NULL,                           // m_free
};

PyObject *PyQ_quakecl_init(void)
{
    return PyModule_Create(&quakecl_module);
}
