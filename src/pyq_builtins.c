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

typedef struct {
    PyObject_HEAD
    int servernumber;
    int index;
} PyQ__sv_edict;

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
        PyErr_SetString(PyExc_NotImplementedError, "under construction"); \
        return NULL; \
    }

#define PyQ__sv_edict_VECTOR_SETTER(field) \
    static int PyQ__sv_edict_set##field(PyQ__sv_edict *self, PyObject *value, void *closure) { \
        PyErr_SetString(PyExc_NotImplementedError, "under construction"); \
        return -1; \
    }

#define PyQ__sv_edict_STRING_GETTER(field) \
    static PyObject *PyQ__sv_edict_get##field(PyQ__sv_edict *self, void *closure) { \
        PyErr_SetString(PyExc_NotImplementedError, "under construction"); \
        return NULL; \
    }

#define PyQ__sv_edict_STRING_SETTER(field) \
    static int PyQ__sv_edict_set##field(PyQ__sv_edict *self, PyObject *value, void *closure) { \
        PyErr_SetString(PyExc_NotImplementedError, "under construction"); \
        return -1; \
    }

#define PyQ__sv_edict_ENTITY_GETTER(field) \
    static PyObject *PyQ__sv_edict_get##field(PyQ__sv_edict *self, void *closure) { \
        PyErr_SetString(PyExc_NotImplementedError, "under construction"); \
        return NULL; \
    }

#define PyQ__sv_edict_ENTITY_SETTER(field) \
    static int PyQ__sv_edict_set##field(PyQ__sv_edict *self, PyObject *value, void *closure) { \
        PyErr_SetString(PyExc_NotImplementedError, "under construction"); \
        return -1; \
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

static PyTypeObject PyQ__sv_edict_type = {
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
    Py_RETURN_NONE;
}

/**
 * quake._sv.setmodel
 */
static PyObject *PyQ__sv_setmodel(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

/**
 * quake._sv.setsize
 */
static PyObject *PyQ__sv_setsize(PyObject *self, PyObject *args)
{
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

    return world;
}

/**
 * quake._sv.time getter
 */
static PyObject *PyQ__sv_gettime(PyObject *self, void *closure)
{
    return PyFloat_FromDouble(sv.time);
}

static PyMethodDef PyQ__sv_methods[] = {
    { "setorigin", PyQ__sv_setorigin, METH_VARARGS },
    { "setmodel", PyQ__sv_setmodel, METH_VARARGS },
    { "setsize", PyQ__sv_setsize, METH_VARARGS },
    { NULL },
};

static PyGetSetDef PyQ__sv_getset[] = {
    { "edict", PyQ__sv_getedict },
    { "edicts", PyQ__sv_getedicts },
    { "world", PyQ__sv_getworld },
    { "time", PyQ__sv_gettime },
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
// quake.Entity class

/**
 * Checks if Entity object is valid and returns edict pointer.
 */
static edict_t *GetEdict(PyQ_Entity *self)
{
    // sv.active is set to true a bit too late
    // if (!sv.active) {
    //     PyErr_SetString(PyExc_ReferenceError, "Server is not running");
    //     return false;
    // }

    // worldspawn is valid between levels
    if (self->index == 0) {
        return sv.edicts;
    }

    // client entity is also valid
    if (self->index >= 1 && self->index <= svs.maxclients) {
        return EDICT_NUM(self->index);
    }

    // for anything else make basic checks

    if (self->servernumber != PyQ_servernumber) {
        PyErr_SetString(PyExc_ReferenceError, "Entity was created in another server");
        return NULL;
    }

    if (self->index < 0 || self->index >= sv.num_edicts) {
        PyErr_SetString(PyExc_ReferenceError, "invalid entity index");
        return NULL;
    }

    return EDICT_NUM(self->index);
}

/**
 * quake.Entity.__new__
 */
static PyObject *E_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyQ_Entity *self = (PyQ_Entity *) type->tp_alloc(type, 0);

    if (self) {
        self->servernumber = PyQ_servernumber;
        self->index = -1;
    }

    return (PyObject *) self;
}

/**
 * quake.Entity.__init__
 */
static int E_init(PyQ_Entity *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

/**
 * quake.Entity.__dealloc__
 */
static void E_dealloc(PyQ_Entity *self)
{
    Py_TYPE(self)->tp_free((PyObject *) self);
}

/**
 * quake.Entity.__repr__
 */
static PyObject *E_repr(PyQ_Entity *self)
{
    edict_t *edict = GetEdict(self);

    if (!edict) {
        PyErr_Clear();
        return PyUnicode_FromFormat("<invalid entity reference at %p>", self);
    }

    return PyUnicode_FromFormat("<edict #%d, classname \"%s\">",
        self->index, PR_GetString(edict->v.classname));
}

/**
 * quake.Entity.__hash__
 */
static Py_hash_t E_hash(PyQ_Entity *self)
{
    edict_t *edict;

    if (!(edict = GetEdict(self))) {
        return -1;
    }

    return NUM_FOR_EDICT(edict);
}

/**
 * quake.Entity.__richcmp__
 */
static PyObject *E_richcmp(PyQ_Entity *a, PyQ_Entity *b, int op)
{
    edict_t *aedict, *bedict;

    if (!(aedict = GetEdict(a))) {
        return NULL;
    }

    if (!(bedict = GetEdict(b))) {
        return NULL;
    }

    if (op == Py_EQ) {
        if (aedict == bedict) {
            Py_RETURN_TRUE;
        } else {
            Py_RETURN_FALSE;
        }
    }

    Py_RETURN_NOTIMPLEMENTED;
}

/**
 * quake.Entity.setorigin(origin)
 */
static PyObject *E_setorigin(PyQ_Entity *self, PyObject *args)
{
    edict_t *edict;
    PyQ_vec *vec;

    if (!(edict = GetEdict(self))) {
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O!", &PyQ_vec_type, &vec)) {
        return NULL;
    }

    VectorCopy(*vec->p, edict->v.origin);
    SV_LinkEdict(edict, false);

    Py_RETURN_NONE;
}

/**
 * quake.Entity.setmodel(model)
 */
static PyObject *E_setmodel(PyQ_Entity *self, PyObject *args)
{
    edict_t *edict;
    int index;
    char const *model, **cache;

    if (!(edict = GetEdict(self))) {
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "s", &model)) {
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
 * quake.Entity.setsize(mins, maxs)
 */
static PyObject *E_setsize(PyQ_Entity *self, PyObject *args)
{
    edict_t *edict;
    PyQ_vec *mins;
    PyQ_vec *maxs;

    if (!(edict = GetEdict(self))) {
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O!O!", &PyQ_vec_type, &mins, &PyQ_vec_type, &maxs)) {
        return NULL;
    }

    VectorCopy(*mins->p, edict->v.mins);
    VectorCopy(*maxs->p, edict->v.maxs);
    VectorSubtract(*maxs->p, *mins->p, edict->v.size);
    SV_LinkEdict(edict, false);

    Py_RETURN_NONE;
}

/**
 * Macro to get edict's float field.
 */
#define GET_FLOAT_VALUE(self, field) \
    do { \
        edict_t *edict; \
        if (!(edict = GetEdict(self))) { \
            return NULL; \
        } \
        return Py_BuildValue("f", edict->field); \
    } while (0)

/**
 * Macro to set edict's float field.
 */
#define SET_FLOAT_VALUE(self, field, value) \
    do { \
        edict_t *edict; \
        double d; \
        if (!(edict = GetEdict(self))) { \
            return -1; \
        } \
        d = PyFloat_AsDouble(value); \
        edict->field = (float) d; \
        return 0; \
    } while (0)

/**
 * Macro to get edict's int field.
 * Used only for fields that are supposed to hold flags.
 */
#define GET_INT_VALUE(self, field) \
    do { \
        edict_t *edict; \
        if (!(edict = GetEdict(self))) { \
            return NULL; \
        } \
        return Py_BuildValue("i", (int) edict->field); \
    } while (0)

/**
 * Macro to set edict's int field.
 * Used only for fields that are supposed to hold flags.
 */
#define SET_INT_VALUE(self, field, value) \
    do { \
        edict_t *edict; \
        long n; \
        if (!(edict = GetEdict(self))) { \
            return -1; \
        } \
        n = PyLong_AsLong(value); \
        edict->field = (float) n; \
        return 0; \
    } while (0)

/**
 * Macro to get edict's vec3_t field.
 */
#define GET_VEC3_VALUE(self, field) \
    do { \
        edict_t *edict; \
        PyObject *args; \
        PyObject *result = NULL; \
        if (!(edict = GetEdict(self))) { \
            return NULL; \
        } \
        args = Py_BuildValue("(n)", &edict->field); \
        if (args) { \
            result = PyObject_CallObject((PyObject *) &PyQ_vec_type, args); \
        } \
        Py_XDECREF(args); \
        return result; \
    } while (0)

/**
 * Macro to get edict's vec3_t field.
 */
#define SET_VEC3_VALUE(self, field, value) \
    do { \
        edict_t *edict; \
        if (!(edict = GetEdict(self))) { \
            return -1; \
        } \
        if (!PyObject_TypeCheck(value, &PyQ_vec_type)) { \
            return -1; \
        } \
        edict->field[0] = (*((PyQ_vec *) value)->p)[0]; \
        edict->field[1] = (*((PyQ_vec *) value)->p)[1]; \
        edict->field[2] = (*((PyQ_vec *) value)->p)[2]; \
        return 0; \
    } while (0)

/**
 * Macro to get edict's string field.
 */
#define GET_STRING_VALUE(self, field) \
    do { \
        edict_t *edict; \
        if (!(edict = GetEdict(self))) { \
            return NULL; \
        } \
        return PyUnicode_FromString(PR_GetString(edict->field)); \
    } while (0)

/**
 * Macro to get edict's string field.
 * (later comment) I don't believe I was sober while
 * writing this code.
 */
#define SET_STRING_VALUE(self, field, value) \
    do { \
        edict_t *edict; \
        PyObject *bytes; \
        char const *str; \
        char *stored; \
        if (!(edict = GetEdict(self))) { \
            return -1; \
        } \
        bytes = PyUnicode_AsASCIIString(value); \
        if (!bytes) { \
            return -1; \
        } \
        str = PyBytes_AsString(bytes); \
        if (!str) { \
            Py_DECREF(bytes); \
            return -1; \
        } \
        stored = PyQ_string_storage[self->index].field; \
        Q_strcpy(stored, str); \
        edict->field = PR_SetEngineString(stored); \
        Py_DECREF(bytes); \
        return 0; \
    } while (0)

/**
 * Macro to get edict's entity field.
 */
#define GET_ENTITY_VALUE(self, field) \
    do { \
        edict_t *edict; \
        PyQ_Entity *entity; \
        if (!(edict = GetEdict(self))) { \
            return NULL; \
        } \
        entity = (PyQ_Entity *) PyObject_CallNoArgs((PyObject *) &PyQ_Entity_type); \
        if (entity) { \
            entity->index = edict->field; \
        } \
        return (PyObject *) entity; \
    } while (0)

/**
 * Macro to set edict's entity field.
 */
#define SET_ENTITY_VALUE(self, field, value) \
    do { \
        edict_t *edict; \
        if (!(edict = GetEdict(self))) { \
            return -1; \
        } \
        PyErr_SetString(PyExc_NotImplementedError, "setting entity values is not supported yet"); \
        return -1; \
    } while (0)

/**
 * Macro to get edict's func_t field.
 * Doesn't really return any function, just shows
 * a brief description.
 */
#define GET_FUNC_VALUE(self, field) \
    do { \
        edict_t *edict; \
        if (!(edict = GetEdict(self))) { \
            return NULL; \
        } \
        return PyUnicode_FromFormat("QuakeC function %s (#%d)", \
            PR_GetString(pr_functions[edict->field].s_name), \
            edict->field); \
    } while (0)

static PyObject *E_getmodelindex(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.modelindex);
}

static PyObject *E_getabsmin(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.absmin);
}

static PyObject *E_getabsmax(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.absmax);
}

static PyObject *E_getltime(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.ltime);
}

static int E_setltime(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.ltime, value);
}

static PyObject *E_getmovetype(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.movetype);
}

static int E_setmovetype(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.movetype, value);
}

static PyObject *E_getsolid(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.solid);
}

static int E_setsolid(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.solid, value);
}

static PyObject *E_getorigin(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.origin);
}

static PyObject *E_getoldorigin(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.oldorigin);
}

static PyObject *E_getvelocity(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.velocity);
}

static int E_setvelocity(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.velocity, value);
}

static PyObject *E_getangles(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.angles);
}

static int E_setangles(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.angles, value);
}

static PyObject *E_getavelocity(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.avelocity);
}

static int E_setavelocity(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.avelocity, value);
}

static PyObject *E_getpunchangle(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.punchangle);
}

static int E_setpunchangle(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.punchangle, value);
}

static PyObject *E_getclassname(PyQ_Entity *self, void *closure)
{
    GET_STRING_VALUE(self, v.classname);
}

static int E_setclassname(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_STRING_VALUE(self, v.classname, value);
}

static PyObject *E_getmodel(PyQ_Entity *self, void *closure)
{
    GET_STRING_VALUE(self, v.model);
}

static PyObject *E_getframe(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.frame);
}

static int E_setframe(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.frame, value);
}

static PyObject *E_getskin(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.skin);
}

static int E_setskin(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.skin, value);
}

static PyObject *E_geteffects(PyQ_Entity *self, void *closure)
{
    GET_INT_VALUE(self, v.effects);
}

static int E_seteffects(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_INT_VALUE(self, v.effects, value);
}

static PyObject *E_getmins(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.mins);
}

static PyObject *E_getmaxs(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.maxs);
}

static PyObject *E_getsize(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.size);
}

static PyObject *E_gettouch(PyQ_Entity *self, void *closure)
{
    GET_FUNC_VALUE(self, v.touch);
}

static PyObject *E_getuse(PyQ_Entity *self, void *closure)
{
    GET_FUNC_VALUE(self, v.use);
}

static PyObject *E_getthink(PyQ_Entity *self, void *closure)
{
    GET_FUNC_VALUE(self, v.think);
}

static PyObject *E_getblocked(PyQ_Entity *self, void *closure)
{
    GET_FUNC_VALUE(self, v.blocked);
}

static PyObject *E_getnextthink(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.nextthink);
}

static int E_setnextthink(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.nextthink, value);
}

static PyObject *E_getgroundentity(PyQ_Entity *self, void *closure)
{
    GET_ENTITY_VALUE(self, v.groundentity);
}

static int E_setgroundentity(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_ENTITY_VALUE(self, v.groundentity, value);
}

static PyObject *E_gethealth(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.health);
}

static int E_sethealth(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.health, value);
}

static PyObject *E_getfrags(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.frags);
}

static int E_setfrags(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.frags, value);
}

static PyObject *E_getweapon(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.weapon);
}

static int E_setweapon(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.weapon, value);
}

static PyObject *E_getweaponmodel(PyQ_Entity *self, void *closure)
{
    GET_STRING_VALUE(self, v.weaponmodel);
}

static int E_setweaponmodel(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_STRING_VALUE(self, v.weaponmodel, value);
}

static PyObject *E_getweaponframe(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.weaponframe);
}

static int E_setweaponframe(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.weaponframe, value);
}

static PyObject *E_getcurrentammo(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.currentammo);
}

static int E_setcurrentammo(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.currentammo, value);
}

static PyObject *E_getammo_shells(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.ammo_shells);
}

static int E_setammo_shells(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.ammo_shells, value);
}

static PyObject *E_getammo_nails(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.ammo_nails);
}

static int E_setammo_nails(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.ammo_nails, value);
}

static PyObject *E_getammo_rockets(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.ammo_rockets);
}

static int E_setammo_rockets(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.ammo_rockets, value);
}

static PyObject *E_getammo_cells(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.ammo_cells);
}

static int E_setammo_cells(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.ammo_cells, value);
}

static PyObject *E_getitems(PyQ_Entity *self, void *closure)
{
    GET_INT_VALUE(self, v.items);
}

static int E_setitems(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_INT_VALUE(self, v.items, value);
}

static PyObject *E_gettakedamage(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.takedamage);
}

static int E_settakedamage(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.takedamage, value);
}

static PyObject *E_getchain(PyQ_Entity *self, void *closure)
{
    GET_ENTITY_VALUE(self, v.chain);
}

static int E_setchain(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_ENTITY_VALUE(self, v.chain, value);
}

static PyObject *E_getdeadflag(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.deadflag);
}

static int E_setdeadflag(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.deadflag, value);
}

static PyObject *E_getview_ofs(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.view_ofs);
}

static int E_setview_ofs(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.view_ofs, value);
}

static PyObject *E_getbutton0(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.button0);
}

static int E_setbutton0(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.button0, value);
}

static PyObject *E_getbutton1(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.button1);
}

static int E_setbutton1(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.button1, value);
}

static PyObject *E_getbutton2(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.button2);
}

static int E_setbutton2(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.button2, value);
}

static PyObject *E_getimpulse(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.impulse);
}

static int E_setimpulse(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.impulse, value);
}

static PyObject *E_getfixangle(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.fixangle);
}

static int E_setfixangle(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.fixangle, value);
}

static PyObject *E_getv_angle(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.v_angle);
}

static int E_setv_angle(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.v_angle, value);
}

static PyObject *E_getidealpitch(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.idealpitch);
}

static int E_setidealpitch(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.idealpitch, value);
}

static PyObject *E_getnetname(PyQ_Entity *self, void *closure)
{
    GET_STRING_VALUE(self, v.netname);
}

static int E_setnetname(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_STRING_VALUE(self, v.netname, value);
}

static PyObject *E_getenemy(PyQ_Entity *self, void *closure)
{
    GET_ENTITY_VALUE(self, v.enemy);
}

static int E_setenemy(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_ENTITY_VALUE(self, v.enemy, value);
}

static PyObject *E_getflags(PyQ_Entity *self, void *closure)
{
    GET_INT_VALUE(self, v.flags);
}

static int E_setflags(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_INT_VALUE(self, v.flags, value);
}

static PyObject *E_getcolormap(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.colormap);
}

static int E_setcolormap(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.colormap, value);
}

static PyObject *E_getteam(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.team);
}

static int E_setteam(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.team, value);
}

static PyObject *E_getmax_health(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.max_health);
}

static int E_setmax_health(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.max_health, value);
}

static PyObject *E_getteleport_time(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.teleport_time);
}

static int E_setteleport_time(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.teleport_time, value);
}

static PyObject *E_getarmortype(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.armortype);
}

static int E_setarmortype(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.armortype, value);
}

static PyObject *E_getarmorvalue(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.armorvalue);
}

static int E_setarmorvalue(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.armorvalue, value);
}

static PyObject *E_getwaterlevel(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.waterlevel);
}

static int E_setwaterlevel(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.waterlevel, value);
}

static PyObject *E_getwatertype(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.watertype);
}

static int E_setwatertype(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.watertype, value);
}

static PyObject *E_getideal_yaw(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.ideal_yaw);
}

static int E_setideal_yaw(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.ideal_yaw, value);
}

static PyObject *E_getyaw_speed(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.yaw_speed);
}

static int E_setyaw_speed(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.yaw_speed, value);
}

static PyObject *E_getaiment(PyQ_Entity *self, void *closure)
{
    GET_ENTITY_VALUE(self, v.aiment);
}

static int E_setaiment(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_ENTITY_VALUE(self, v.aiment, value);
}

static PyObject *E_getgoalentity(PyQ_Entity *self, void *closure)
{
    GET_ENTITY_VALUE(self, v.goalentity);
}

static int E_setgoalentity(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_ENTITY_VALUE(self, v.goalentity, value);
}

static PyObject *E_getspawnflags(PyQ_Entity *self, void *closure)
{
    GET_INT_VALUE(self, v.spawnflags);
}

static int E_setspawnflags(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_INT_VALUE(self, v.spawnflags, value);
}

static PyObject *E_gettarget(PyQ_Entity *self, void *closure)
{
    GET_STRING_VALUE(self, v.target);
}

static int E_settarget(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_STRING_VALUE(self, v.target, value);
}

static PyObject *E_gettargetname(PyQ_Entity *self, void *closure)
{
    GET_STRING_VALUE(self, v.targetname);
}

static int E_settargetname(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_STRING_VALUE(self, v.targetname, value);
}

static PyObject *E_getdmg_take(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.dmg_take);
}

static int E_setdmg_take(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.dmg_take, value);
}

static PyObject *E_getdmg_save(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.dmg_save);
}

static int E_setdmg_save(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.dmg_save, value);
}

static PyObject *E_getdmg_inflictor(PyQ_Entity *self, void *closure)
{
    GET_ENTITY_VALUE(self, v.dmg_inflictor);
}

static int E_setdmg_inflictor(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_ENTITY_VALUE(self, v.dmg_inflictor, value);
}

static PyObject *E_getowner(PyQ_Entity *self, void *closure)
{
    GET_ENTITY_VALUE(self, v.owner);
}

static int E_setowner(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_ENTITY_VALUE(self, v.owner, value);
}

static PyObject *E_getmovedir(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.movedir);
}

static int E_setmovedir(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.movedir, value);
}

static PyObject *E_getmessage(PyQ_Entity *self, void *closure)
{
    GET_STRING_VALUE(self, v.message);
}

static int E_setmessage(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_STRING_VALUE(self, v.message, value);
}

static PyObject *E_getsounds(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.sounds);
}

static int E_setsounds(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.sounds, value);
}

static PyObject *E_getnoise(PyQ_Entity *self, void *closure)
{
    GET_STRING_VALUE(self, v.noise);
}

static int E_setnoise(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_STRING_VALUE(self, v.noise, value);
}

static PyObject *E_getnoise1(PyQ_Entity *self, void *closure)
{
    GET_STRING_VALUE(self, v.noise1);
}

static int E_setnoise1(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_STRING_VALUE(self, v.noise1, value);
}

static PyObject *E_getnoise2(PyQ_Entity *self, void *closure)
{
    GET_STRING_VALUE(self, v.noise2);
}

static int E_setnoise2(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_STRING_VALUE(self, v.noise2, value);
}

static PyObject *E_getnoise3(PyQ_Entity *self, void *closure)
{
    GET_STRING_VALUE(self, v.noise3);
}

static int E_setnoise3(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_STRING_VALUE(self, v.noise3, value);
}

static PyMethodDef E_methods[] = {
    { "setorigin",      (PyCFunction) E_setorigin,      METH_VARARGS },
    { "setmodel",       (PyCFunction) E_setmodel,       METH_VARARGS },
    { "setsize",        (PyCFunction) E_setsize,        METH_VARARGS },
    { NULL },
};

static PyMemberDef E_members[] = {
    { NULL },
};

static PyGetSetDef E_getset[] = {
    { "modelindex",     (getter) E_getmodelindex,       (setter) NULL },
    { "absmin",         (getter) E_getabsmin,           (setter) NULL },
    { "absmax",         (getter) E_getabsmax,           (setter) NULL },
    { "ltime",          (getter) E_getltime,            (setter) E_setltime },
    { "movetype",       (getter) E_getmovetype,         (setter) E_setmovetype },
    { "solid",          (getter) E_getsolid,            (setter) E_setsolid },
    { "origin",         (getter) E_getorigin,           (setter) NULL },
    { "oldorigin",      (getter) E_getoldorigin,        (setter) NULL },
    { "velocity",       (getter) E_getvelocity,         (setter) E_setvelocity },
    { "angles",         (getter) E_getangles,           (setter) E_setangles },
    { "avelocity",      (getter) E_getavelocity,        (setter) E_setavelocity },
    { "punchangle",     (getter) E_getpunchangle,       (setter) E_setpunchangle },
    { "classname",      (getter) E_getclassname,        (setter) E_setclassname },
    { "model",          (getter) E_getmodel,            (setter) NULL },
    { "frame",          (getter) E_getframe,            (setter) E_setframe },
    { "skin",           (getter) E_getskin,             (setter) E_setskin },
    { "effects",        (getter) E_geteffects,          (setter) E_seteffects },
    { "mins",           (getter) E_getmins,             (setter) NULL },
    { "maxs",           (getter) E_getmaxs,             (setter) NULL },
    { "size",           (getter) E_getsize,             (setter) NULL },
    { "touch",          (getter) E_gettouch,            (setter) NULL },
    { "use",            (getter) E_getuse,              (setter) NULL },
    { "think",          (getter) E_getthink,            (setter) NULL },
    { "blocked",        (getter) E_getblocked,          (setter) NULL },
    { "nextthink",      (getter) E_getnextthink,        (setter) E_setnextthink },
    { "groundentity",   (getter) E_getgroundentity,     (setter) E_setgroundentity },
    { "health",         (getter) E_gethealth,           (setter) E_sethealth },
    { "frags",          (getter) E_getfrags,            (setter) E_setfrags },
    { "weapon",         (getter) E_getweapon,           (setter) E_setweapon },
    { "weaponmodel",    (getter) E_getweaponmodel,      (setter) E_setweaponmodel },
    { "weaponframe",    (getter) E_getweaponframe,      (setter) E_setweaponframe },
    { "currentammo",    (getter) E_getcurrentammo,      (setter) E_setcurrentammo },
    { "ammo_shells",    (getter) E_getammo_shells,      (setter) E_setammo_shells },
    { "ammo_nails",     (getter) E_getammo_nails,       (setter) E_setammo_nails },
    { "ammo_rockets",   (getter) E_getammo_rockets,     (setter) E_setammo_rockets },
    { "ammo_cells",     (getter) E_getammo_cells,       (setter) E_setammo_cells },
    { "items",          (getter) E_getitems,            (setter) E_setitems },
    { "takedamage",     (getter) E_gettakedamage,       (setter) E_settakedamage },
    { "chain",          (getter) E_getchain,            (setter) E_setchain },
    { "deadflag",       (getter) E_getdeadflag,         (setter) E_setdeadflag },
    { "view_ofs",       (getter) E_getview_ofs,         (setter) E_setview_ofs },
    { "button0",        (getter) E_getbutton0,          (setter) E_setbutton0 },
    { "button1",        (getter) E_getbutton1,          (setter) E_setbutton1 },
    { "button2",        (getter) E_getbutton2,          (setter) E_setbutton2 },
    { "impulse",        (getter) E_getimpulse,          (setter) E_setimpulse },
    { "fixangle",       (getter) E_getfixangle,         (setter) E_setfixangle },
    { "v_angle",        (getter) E_getv_angle,          (setter) E_setv_angle },
    { "idealpitch",     (getter) E_getidealpitch,       (setter) E_setidealpitch },
    { "netname",        (getter) E_getnetname,          (setter) E_setnetname },
    { "enemy",          (getter) E_getenemy,            (setter) E_setenemy },
    { "flags",          (getter) E_getflags,            (setter) E_setflags },
    { "colormap",       (getter) E_getcolormap,         (setter) E_setcolormap },
    { "team",           (getter) E_getteam,             (setter) E_setteam },
    { "max_health",     (getter) E_getmax_health,       (setter) E_setmax_health },
    { "teleport_time",  (getter) E_getteleport_time,    (setter) E_setteleport_time },
    { "armortype",      (getter) E_getarmortype,        (setter) E_setarmortype },
    { "armorvalue",     (getter) E_getarmorvalue,       (setter) E_setarmorvalue },
    { "waterlevel",     (getter) E_getwaterlevel,       (setter) E_setwaterlevel },
    { "watertype",      (getter) E_getwatertype,        (setter) E_setwatertype },
    { "ideal_yaw",      (getter) E_getideal_yaw,        (setter) E_setideal_yaw },
    { "yaw_speed",      (getter) E_getyaw_speed,        (setter) E_setyaw_speed },
    { "aiment",         (getter) E_getaiment,           (setter) E_setaiment },
    { "goalentity",     (getter) E_getgoalentity,       (setter) E_setgoalentity },
    { "spawnflags",     (getter) E_getspawnflags,       (setter) E_setspawnflags },
    { "target",         (getter) E_gettarget,           (setter) E_settarget },
    { "targetname",     (getter) E_gettargetname,       (setter) E_settargetname },
    { "dmg_take",       (getter) E_getdmg_take,         (setter) E_setdmg_take },
    { "dmg_save",       (getter) E_getdmg_save,         (setter) E_setdmg_save },
    { "dmg_inflictor",  (getter) E_getdmg_inflictor,    (setter) E_setdmg_inflictor },
    { "owner",          (getter) E_getowner,            (setter) E_setowner },
    { "movedir",        (getter) E_getmovedir,          (setter) E_setmovedir },
    { "message",        (getter) E_getmessage,          (setter) E_setmessage },
    { "sounds",         (getter) E_getsounds,           (setter) E_setsounds },
    { "noise",          (getter) E_getnoise,            (setter) E_setnoise },
    { "noise1",         (getter) E_getnoise1,           (setter) E_setnoise1 },
    { "noise2",         (getter) E_getnoise2,           (setter) E_setnoise2 },
    { "noise3",         (getter) E_getnoise3,           (setter) E_setnoise3 },
    { NULL },
};

PyTypeObject PyQ_Entity_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "quake.Entity",                             // tp_name
    sizeof(PyQ_Entity),                         // tp_basicsize
    0,                                          // tp_itemsize
    (destructor) E_dealloc,                     // tp_dealloc
    0,                                          // tp_vectorcall_offset
    NULL,                                       // tp_getattr
    NULL,                                       // tp_setattr
    NULL,                                       // tp_as_async
    (reprfunc) E_repr,                          // tp_repr
    NULL,                                       // tp_as_number
    NULL,                                       // tp_as_sequence
    NULL,                                       // tp_as_mapping
    (hashfunc) E_hash,                          // tp_hash
    NULL,                                       // tp_call
    NULL,                                       // tp_str
    NULL,                                       // tp_getattro
    NULL,                                       // tp_setattro
    NULL,                                       // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   // tp_flags
    PyDoc_STR("Quake Entity"),                  // tp_doc
    NULL,                                       // tp_traverse
    NULL,                                       // tp_clear
    (richcmpfunc) E_richcmp,                    // tp_richcompare
    0,                                          // tp_weaklistoffset
    NULL,                                       // tp_iter
    NULL,                                       // tp_iternext
    E_methods,                                  // tp_methods
    E_members,                                  // tp_members
    E_getset,                                   // tp_getset
    NULL,                                       // tp_base
    NULL,                                       // tp_dict
    NULL,                                       // tp_descr_get
    NULL,                                       // tp_descr_set
    0,                                          // tp_dictoffset
    (initproc) E_init,                          // tp_init
    NULL,                                       // tp_alloc
    E_new,                                      // tp_new
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
    int i, num;
    
    char const *sepstr = " ", *endstr = "\n";

    if (kwargs) {
        PyObject *sep, *end;

        sep = PyDict_GetItemString(kwargs, "sep");

        if (sep) {
            if (PyUnicode_Check(sep)) {
                sepstr = PyUnicode_AsUTF8(sep);

                if (!sepstr) {
                    return NULL;
                }
            } else {
                PyErr_SetString(PyExc_TypeError, "sep must be a string");
                return NULL;
            }
        }

        end = PyDict_GetItemString(kwargs, "end");

        if (end) {
            if (PyUnicode_Check(end)) {
                endstr = PyUnicode_AsUTF8(end);

                if (!endstr) {
                    return NULL;
                }
            } else {
                PyErr_SetString(PyExc_TypeError, "end must be a string");
                return NULL;
            }
        }
    }

    num = PyTuple_Size(args);

    for (i = 0; i < num; i++) {
        PyObject *item, *str;

        item = PyTuple_GetItem(args, i);
        str = PyObject_Str(item); // new reference
        
        if (str) {
            char const *c_str = PyUnicode_AsUTF8(str);

            if (c_str) {
                Con_DPrintf("%s", c_str);

                if (i != num - 1) {
                    Con_DPrintf("%s", sepstr);
                }
            }
        }

        Py_DECREF(item);

        if (PyErr_Occurred()) {
            return NULL;
        }
    }

    Con_DPrintf("%s", endstr);
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

/**
 * quake.spawn() [#14 in Quakec]
 */
static PyObject *quake__spawn(PyObject *self, PyObject *args)
{
    PyQ_Entity *entity;

    entity = (PyQ_Entity *) PyObject_CallNoArgs((PyObject *) &PyQ_Entity_type);

    if (!entity) {
        return NULL;
    }

    entity->index = NUM_FOR_EDICT(ED_Alloc());

    return (PyObject *) entity;
}

/**
 * quake.remove(ent) [#15 in Quakec]
 */
static PyObject *quake__remove(PyObject *self, PyObject *args)
{
    PyQ_Entity *entity;
    edict_t *edict;

    if (!PyArg_ParseTuple(args, "O", &entity)) {
        return NULL;
    }

    if (!(edict = GetEdict(entity))) {
        return NULL;
    }

    ED_Free(edict);
    Py_RETURN_NONE;
}

/**
 * quake.bprint(message) [#23 in Quakec]
 */
static PyObject *quake__bprint(PyObject *self, PyObject *args)
{
    char const *msg;

    if (!sv.active) {
        PyErr_SetString(PyExc_RuntimeError, "server is not running");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "s", &msg)) {
        return NULL;
    }

    SV_BroadcastPrintf("%s\n", msg);
    Py_RETURN_NONE;
}

/**
 * quake.sprint(ent, msg) [#24 in Quakec]
 */
static PyObject *quake__sprint(PyObject *self, PyObject *args)
{
    PyQ_Entity *entity;
    char const *msg;

    if (!sv.active) {
        PyErr_SetString(PyExc_RuntimeError, "server is not running");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "Os", &entity, &msg)) {
        return NULL;
    }

    if (entity->index < 1 || entity->index > svs.maxclients) {
        PyErr_SetString(PyExc_ValueError, "entity is non-client");
        return NULL;
    }

    // FIXME: add \n at the end

    MSG_WriteChar(&svs.clients[entity->index - 1].message, svc_print);
    MSG_WriteString(&svs.clients[entity->index - 1].message, msg);

    Py_RETURN_NONE;
}

/**
 * quake.centerprint(ent, msg) [#73 in Quakec]
 */
static PyObject *quake__centerprint(PyObject *self, PyObject *args)
{
    PyQ_Entity *entity;
    char const *msg;

    if (!sv.active) {
        PyErr_SetString(PyExc_RuntimeError, "server is not running");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "Os", &entity, &msg)) {
        return NULL;
    }

    if (entity->index < 1 || entity->index > svs.maxclients) {
        PyErr_SetString(PyExc_ValueError, "entity is non-client");
        return NULL;
    }

    MSG_WriteChar(&svs.clients[entity->index - 1].message, svc_centerprint);
    MSG_WriteString(&svs.clients[entity->index - 1].message, msg);

    Py_RETURN_NONE;
}

/**
 * quake.entities()
 */
static PyObject *quake__entities(PyObject *self, PyObject *args)
{
    PyObject *list;

    list = PyList_New(0);

    if (list) {
        qboolean failed = false;
        int i;

        for (i = 0; i < sv.num_edicts; i++) {
            // ignore free ents
            if (EDICT_NUM(i)->free) {
                continue;
            }

            PyQ_Entity *entity = (PyQ_Entity *) PyObject_CallNoArgs((PyObject *) &PyQ_Entity_type);

            if (entity) {
                entity->index = i;
                PyList_Append(list, (PyObject *) entity);
                Py_DECREF(entity);
            } else {
                failed = true;
                break;
            }
        }

        if (failed) {
            Py_DECREF(list);
            return NULL;
        }
    }

    return list;
}

/**
 * quake.precache_sound(name)
 */
static PyObject *quake__precache_sound(PyObject *self, PyObject *args)
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
 * quake.precache_sound(name)
 */
static PyObject *quake__precache_model(PyObject *self, PyObject *args)
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
 * quake.particle(org, dir=(0.0, 0.0, 0.0), color=0, count=1)
 */
static PyObject *quake__particle(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "org", "dir", "color", "count", NULL };

    vec3_t org;
    vec3_t dir = { 0, 0, 0 };
    int color = 0;
    int count = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "(fff)|(fff)ii", kwlist,
        &org[0], &org[1], &org[2],
        &dir[0], &dir[1], &dir[2],
        &color, &count)) {
        return NULL;
    }

    if (sv.active) {
        SV_StartParticle(org, dir, color, count);
    }

    Py_RETURN_NONE;
}

/**
 * quake.sound(entity, sample, chan=0, vol=1.0, attn=0)
 */
static PyObject *quake__sound(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "entity", "sample", "chan", "vol", "attn", NULL };

    PyQ_Entity *entity;
    char const *sample;
    int chan = 0;
    float vol = 1.0;
    int attn = 0;

    edict_t *edict;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "Os|ifi", kwlist,
        &entity, &sample,
        &chan, &vol, &attn)) {
        return NULL;
    }

    if (Py_IsNone((PyObject *) entity)) {
        edict = sv.edicts;
    } else {
        edict = GetEdict(entity);

        if (!edict) {
            return NULL;
        }
    }

    if (sv.active) {
        SV_StartSound(edict, chan, sample, vol, attn);
    }

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
    { "bprint",         quake__bprint,                  METH_VARARGS },
    { "sprint",         quake__sprint,                  METH_VARARGS },
    { "centerprint",    quake__centerprint,             METH_VARARGS },
    { "entities",       quake__entities,                METH_NOARGS },
    { "spawn",          quake__spawn,                   METH_VARARGS },
    { "remove",         quake__remove,                  METH_VARARGS },
    { "precache_sound", quake__precache_sound,          METH_VARARGS },
    { "precache_model", quake__precache_model,          METH_VARARGS },
    { "particle",       (PyCFunction) quake__particle,  METH_VARARGS },
    { "sound",          (PyCFunction) quake__sound,     METH_VARARGS | METH_KEYWORDS },
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

    if (PyType_Ready(&PyQ_vec_type) == -1) {
        return NULL;
    }

    if (PyType_Ready(&PyQ__sv_edict_type) == -1) {
        return NULL;
    }

    if (PyType_Ready(&PyQ__sv_type) == -1) {
        return NULL;
    }

    if (PyType_Ready(&PyQ_Entity_type) == -1) {
        return NULL;
    }

    module = PyModule_Create(&quake_module);

    if (!module) {
        return NULL;
    }

    Py_INCREF(&PyQ_vec_type);
    Py_INCREF(&PyQ__sv_edict_type);
    Py_INCREF(&PyQ__sv_type);
    Py_INCREF(&PyQ_Entity_type);

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

    if (PyModule_AddObject(module, "Entity", (PyObject *) &PyQ_Entity_type) == -1) {
        goto error;
    }

    sv = PyObject_New(PyQ__sv, &PyQ__sv_type);

    if (!sv || PyModule_AddObject(module, "sv", (PyObject *) sv) == -1) {
        goto error;
    }

    return module;

error:
    Py_XDECREF(sv);
    Py_DECREF(&PyQ_Entity_type);
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
