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

static qboolean CheckEdict(PyQ_Entity *self)
{
    if (!sv.active) {
        PyErr_SetString(PyExc_ReferenceError, "Server is not running");
        return false;
    }

    if (self->servernumber != PyQ_servernumber) {
        PyErr_SetString(PyExc_ReferenceError, "Entity was created in another server");
        return false;
    }

    if (!self->edict) {
        PyErr_SetString(PyExc_ReferenceError, "edict is NULL");
        return false;
    }

    return true;
}

static PyObject *E_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyQ_Entity *self = (PyQ_Entity *) type->tp_alloc(type, 0);

    if (self) {
        self->servernumber = PyQ_servernumber;
        self->edict = NULL;
    }

    return (PyObject *) self;
}

static int E_init(PyQ_Entity *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static void E_dealloc(PyQ_Entity *self)
{
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *E_repr(PyQ_Entity *self)
{
    if (!CheckEdict(self)) {
        PyErr_Clear();
        return PyUnicode_FromFormat("<invalid entity reference at %p>", self);
    }

    return PyUnicode_FromFormat("<edict #%d, classname \"%s\">",
        NUM_FOR_EDICT(self->edict), PR_GetString(self->edict->v.classname));
}

static PyObject *E_richcmp(PyQ_Entity *a, PyQ_Entity *b, int op)
{
    if (!CheckEdict(a)) {
        return NULL;
    }

    if (!CheckEdict(b)) {
        return NULL;
    }

    if (op == Py_EQ) {
        if (a->edict == b->edict) {
            Py_RETURN_TRUE;
        } else {
            Py_RETURN_FALSE;
        }
    }

    Py_RETURN_NOTIMPLEMENTED;
}

#define GET_FLOAT_VALUE(self, field) \
    do { \
        if (!CheckEdict(self)) { \
            return NULL; \
        } \
        return Py_BuildValue("f", self->edict->field); \
    } while (0)

#define SET_FLOAT_VALUE(self, field, value) \
    do { \
        double d; \
        if (!CheckEdict(self)) { \
            return -1; \
        } \
        d = PyFloat_AsDouble(value); \
        self->edict->field = (float) d; \
        return 0; \
    } while (0)

#define GET_VEC3_VALUE(self, field) \
    do { \
        if (!CheckEdict(self)) { \
            return NULL; \
        } \
        return Py_BuildValue("(fff)", \
            self->edict->field[0], \
            self->edict->field[1], \
            self->edict->field[2]); \
    } while (0)

#define SET_VEC3_VALUE(self, field, value) \
    do { \
        float x, y, z; \
        if (!CheckEdict(self)) { \
            return -1; \
        } \
        if (!PyArg_ParseTuple(value, "fff", &x, &y, &z)) { \
            return -1; \
        } \
        self->edict->field[0] = (float) x; \
        self->edict->field[1] = (float) y; \
        self->edict->field[2] = (float) z; \
        return 0; \
    } while (0)

#define GET_STRING_VALUE(self, field) \
    do { \
        if (!CheckEdict(self)) { \
            return NULL; \
        } \
        return PyUnicode_FromString(PR_GetString(self->edict->field)); \
    } while (0)

#define SET_STRING_VALUE(self, field, value) \
    do { \
        if (!CheckEdict(self)) { \
            return -1; \
        } \
        PyErr_SetString(PyExc_NotImplementedError, "setting string values is not supported yet"); \
        return -1; \
    } while (0)

#define GET_ENTITY_VALUE(self, field) \
    do { \
        PyQ_Entity *entity; \
        if (!CheckEdict(self)) { \
            return NULL; \
        } \
        entity = (PyQ_Entity *) PyObject_CallNoArgs((PyObject *) &PyQ_Entity_type); \
        if (entity) { \
            entity->edict = PROG_TO_EDICT(self->edict->field); \
        } \
        return (PyObject *) entity; \
    } while (0)

#define SET_ENTITY_VALUE(self, field, value) \
    do { \
        if (!CheckEdict(self)) { \
            return -1; \
        } \
        PyErr_SetString(PyExc_NotImplementedError, "setting entity values is not supported yet"); \
        return -1; \
    } while (0)

#define GET_FUNC_VALUE(self, field) \
    do { \
        if (!CheckEdict(self)) { \
            return NULL; \
        } \
        return PyUnicode_FromFormat("QuakeC function %s (#%d)", \
            PR_GetString(pr_functions[self->edict->field].s_name), \
            self->edict->field); \
    } while (0)

#define SET_FUNC_VALUE(self, field, value) \
    do { \
        if (!CheckEdict(self)) { \
            return -1; \
        } \
        PyErr_SetString(PyExc_NotImplementedError, "modifying func values is not supported"); \
        return -1; \
    } while (0)

static PyObject *E_getmodelindex(PyQ_Entity *self, void *closure)
{
    GET_FLOAT_VALUE(self, v.modelindex);
}

static int E_setmodelindex(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.modelindex, value);
}

static PyObject *E_getabsmin(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.absmin);
}

static int E_setabsmin(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.absmin, value);
}

static PyObject *E_getabsmax(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.absmax);
}

static int E_setabsmax(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.absmax, value);
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

static int E_setorigin(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.origin, value);
}

static PyObject *E_getoldorigin(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.oldorigin);
}

static int E_setoldorigin(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.oldorigin, value);
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

static int E_setmodel(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_STRING_VALUE(self, v.model, value);
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
    GET_FLOAT_VALUE(self, v.effects);
}

static int E_seteffects(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.effects, value);
}

static PyObject *E_getmins(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.mins);
}

static int E_setmins(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.mins, value);
}

static PyObject *E_getmaxs(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.maxs);
}

static int E_setmaxs(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.maxs, value);
}

static PyObject *E_getsize(PyQ_Entity *self, void *closure)
{
    GET_VEC3_VALUE(self, v.size);
}

static int E_setsize(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_VEC3_VALUE(self, v.size, value);
}

static PyObject *E_gettouch(PyQ_Entity *self, void *closure)
{
    GET_FUNC_VALUE(self, v.touch);
}

static int E_settouch(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FUNC_VALUE(self, v.touch, value);
}

static PyObject *E_getuse(PyQ_Entity *self, void *closure)
{
    GET_FUNC_VALUE(self, v.use);
}

static int E_setuse(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FUNC_VALUE(self, v.use, value);
}

static PyObject *E_getthink(PyQ_Entity *self, void *closure)
{
    GET_FUNC_VALUE(self, v.think);
}

static int E_setthink(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FUNC_VALUE(self, v.think, value);
}

static PyObject *E_getblocked(PyQ_Entity *self, void *closure)
{
    GET_FUNC_VALUE(self, v.blocked);
}

static int E_setblocked(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FUNC_VALUE(self, v.blocked, value);
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
    GET_FLOAT_VALUE(self, v.items);
}

static int E_setitems(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.items, value);
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
    GET_FLOAT_VALUE(self, v.flags);
}

static int E_setflags(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.flags, value);
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
    GET_FLOAT_VALUE(self, v.spawnflags);
}

static int E_setspawnflags(PyQ_Entity *self, PyObject *value, void *closure)
{
    SET_FLOAT_VALUE(self, v.spawnflags, value);
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
    { NULL },
};

static PyMemberDef E_members[] = {
    { NULL },
};

static PyGetSetDef E_getset[] = {
    { "modelindex",     (getter) E_getmodelindex,       (setter) E_setmodelindex },
    { "absmin",         (getter) E_getabsmin,           (setter) E_setabsmin },
    { "absmax",         (getter) E_getabsmax,           (setter) E_setabsmax },
    { "ltime",          (getter) E_getltime,            (setter) E_setltime },
    { "movetype",       (getter) E_getmovetype,         (setter) E_setmovetype },
    { "solid",          (getter) E_getsolid,            (setter) E_setsolid },
    { "origin",         (getter) E_getorigin,           (setter) E_setorigin },
    { "oldorigin",      (getter) E_getoldorigin,        (setter) E_setoldorigin },
    { "velocity",       (getter) E_getvelocity,         (setter) E_setvelocity },
    { "angles",         (getter) E_getangles,           (setter) E_setangles },
    { "avelocity",      (getter) E_getavelocity,        (setter) E_setavelocity },
    { "punchangle",     (getter) E_getpunchangle,       (setter) E_setpunchangle },
    { "classname",      (getter) E_getclassname,        (setter) E_setclassname },
    { "model",          (getter) E_getmodel,            (setter) E_setmodel },
    { "frame",          (getter) E_getframe,            (setter) E_setframe },
    { "skin",           (getter) E_getskin,             (setter) E_setskin },
    { "effects",        (getter) E_geteffects,          (setter) E_seteffects },
    { "mins",           (getter) E_getmins,             (setter) E_setmins },
    { "maxs",           (getter) E_getmaxs,             (setter) E_setmaxs },
    { "size",           (getter) E_getsize,             (setter) E_setsize },
    { "touch",          (getter) E_gettouch,            (setter) E_settouch },
    { "use",            (getter) E_getuse,              (setter) E_setuse },
    { "think",          (getter) E_getthink,            (setter) E_setthink },
    { "blocked",        (getter) E_getblocked,          (setter) E_setblocked },
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
    NULL,                                       // tp_hash
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

static PyObject *M_console_print(PyObject *self, PyObject *args)
{
    char const *msg;

    if (!PyArg_ParseTuple(args, "s", &msg)) {
        return NULL;
    }

    Con_Printf("%s", msg);
    Py_RETURN_NONE;
}

static PyObject *M_console_error(PyObject *self, PyObject *args)
{
    char const *msg;

    if (!PyArg_ParseTuple(args, "s", &msg)) {
        return NULL;
    }

    Con_Printf("\x02%s", msg);
    Py_RETURN_NONE;
}

static PyObject *M_get_entities(PyObject *self, PyObject *args)
{
    PyObject *list;
    
    if (!sv.active) {
        PyErr_SetString(PyExc_RuntimeError, "server is not running");
        return NULL;
    }

    list = PyList_New(sv.num_edicts);

    if (list) {
        qboolean failed = false;
        int i;

        for (i = 0; i < PyList_Size(list); i++) {
            PyQ_Entity *entity = (PyQ_Entity *) PyObject_CallNoArgs((PyObject *) &PyQ_Entity_type);

            if (entity) {
                entity->edict = EDICT_NUM(i);
                PyList_SET_ITEM(list, i, (PyObject *) entity);
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

static PyMethodDef M_methods[] = {
    { "console_print",      M_console_print,    METH_VARARGS },
    { "console_error",      M_console_error,    METH_VARARGS },
    { "get_entities",       M_get_entities,     METH_NOARGS },
    { NULL },
};

static PyModuleDef M_module = {
    PyModuleDef_HEAD_INIT,
    "quake",                        // m_name
    NULL,                           // m_doc
    -1,                             // m_size
    M_methods,                      // m_methods
    NULL,                           // m_slots
    NULL,                           // m_traverse
    NULL,                           // m_clear
    NULL,                           // m_free
};

PyObject *PyQ_InitBuiltins(void)
{
    PyObject *module;

    if (PyType_Ready(&PyQ_Entity_type) == -1) {
        return NULL;
    }

    module = PyModule_Create(&M_module);

    if (module) {
        Py_INCREF(&PyQ_Entity_type);

        PyModule_AddIntConstant(module, "IT_AXE", IT_AXE);
        PyModule_AddIntConstant(module, "IT_SHOTGUN", IT_SHOTGUN);
        PyModule_AddIntConstant(module, "IT_SUPER_SHOTGUN", IT_SUPER_SHOTGUN);
        PyModule_AddIntConstant(module, "IT_NAILGUN", IT_NAILGUN);
        PyModule_AddIntConstant(module, "IT_SUPER_NAILGUN", IT_SUPER_NAILGUN);
        PyModule_AddIntConstant(module, "IT_GRENADE_LAUNCHER", IT_GRENADE_LAUNCHER);
        PyModule_AddIntConstant(module, "IT_ROCKET_LAUNCHER", IT_ROCKET_LAUNCHER);
        PyModule_AddIntConstant(module, "IT_LIGHTNING", IT_LIGHTNING);
        PyModule_AddIntConstant(module, "IT_SUPER_LIGHTNING", IT_SUPER_LIGHTNING);
        PyModule_AddIntConstant(module, "IT_SHELLS", IT_SHELLS);
        PyModule_AddIntConstant(module, "IT_NAILS", IT_NAILS);
        PyModule_AddIntConstant(module, "IT_ROCKETS", IT_ROCKETS);
        PyModule_AddIntConstant(module, "IT_CELLS", IT_CELLS);
        PyModule_AddIntConstant(module, "IT_AXE", IT_AXE);
        PyModule_AddIntConstant(module, "IT_ARMOR1", IT_ARMOR1);
        PyModule_AddIntConstant(module, "IT_ARMOR2", IT_ARMOR2);
        PyModule_AddIntConstant(module, "IT_ARMOR3", IT_ARMOR3);
        PyModule_AddIntConstant(module, "IT_SUPERHEALTH", IT_SUPERHEALTH);
        PyModule_AddIntConstant(module, "IT_KEY1", IT_KEY1);
        PyModule_AddIntConstant(module, "IT_KEY2", IT_KEY2);
        PyModule_AddIntConstant(module, "IT_INVISIBILITY", IT_INVISIBILITY);
        PyModule_AddIntConstant(module, "IT_INVULNERABILITY", IT_INVULNERABILITY);
        PyModule_AddIntConstant(module, "IT_SUIT", IT_SUIT);
        PyModule_AddIntConstant(module, "IT_QUAD", IT_QUAD);
        PyModule_AddIntConstant(module, "IT_SIGIL1", IT_SIGIL1);
        PyModule_AddIntConstant(module, "IT_SIGIL2", IT_SIGIL2);
        PyModule_AddIntConstant(module, "IT_SIGIL3", IT_SIGIL3);
        PyModule_AddIntConstant(module, "IT_SIGIL4", IT_SIGIL4);

        PyModule_AddIntConstant(module, "MOVETYPE_NONE", MOVETYPE_NONE);
        PyModule_AddIntConstant(module, "MOVETYPE_ANGLENOCLIP", MOVETYPE_ANGLENOCLIP);
        PyModule_AddIntConstant(module, "MOVETYPE_ANGLECLIP", MOVETYPE_ANGLECLIP);
        PyModule_AddIntConstant(module, "MOVETYPE_WALK", MOVETYPE_WALK);
        PyModule_AddIntConstant(module, "MOVETYPE_STEP", MOVETYPE_STEP);
        PyModule_AddIntConstant(module, "MOVETYPE_FLY", MOVETYPE_FLY);
        PyModule_AddIntConstant(module, "MOVETYPE_TOSS", MOVETYPE_TOSS);
        PyModule_AddIntConstant(module, "MOVETYPE_PUSH", MOVETYPE_PUSH);
        PyModule_AddIntConstant(module, "MOVETYPE_NOCLIP", MOVETYPE_NOCLIP);
        PyModule_AddIntConstant(module, "MOVETYPE_FLYMISSILE", MOVETYPE_FLYMISSILE);
        PyModule_AddIntConstant(module, "MOVETYPE_BOUNCE", MOVETYPE_BOUNCE);
        PyModule_AddIntConstant(module, "MOVETYPE_GIB", MOVETYPE_GIB);

        PyModule_AddIntConstant(module, "SOLID_NOT", SOLID_NOT);
        PyModule_AddIntConstant(module, "SOLID_TRIGGER", SOLID_TRIGGER);
        PyModule_AddIntConstant(module, "SOLID_BBOX", SOLID_BBOX);
        PyModule_AddIntConstant(module, "SOLID_SLIDEBOX", SOLID_SLIDEBOX);
        PyModule_AddIntConstant(module, "SOLID_BSP", SOLID_BSP);

        PyModule_AddIntConstant(module, "DEAD_NO", DEAD_NO);
        PyModule_AddIntConstant(module, "DEAD_DYING", DEAD_DYING);
        PyModule_AddIntConstant(module, "DEAD_DEAD", DEAD_DEAD);

        PyModule_AddIntConstant(module, "FL_FLY", FL_FLY);
        PyModule_AddIntConstant(module, "FL_SWIM", FL_SWIM);
        PyModule_AddIntConstant(module, "FL_CONVEYOR", FL_CONVEYOR);
        PyModule_AddIntConstant(module, "FL_CLIENT", FL_CLIENT);
        PyModule_AddIntConstant(module, "FL_INWATER", FL_INWATER);
        PyModule_AddIntConstant(module, "FL_MONSTER", FL_MONSTER);
        PyModule_AddIntConstant(module, "FL_GODMODE", FL_GODMODE);
        PyModule_AddIntConstant(module, "FL_NOTARGET", FL_NOTARGET);
        PyModule_AddIntConstant(module, "FL_ITEM", FL_ITEM);
        PyModule_AddIntConstant(module, "FL_ONGROUND", FL_ONGROUND);
        PyModule_AddIntConstant(module, "FL_PARTIALGROUND", FL_PARTIALGROUND);
        PyModule_AddIntConstant(module, "FL_WATERJUMP", FL_WATERJUMP);
        PyModule_AddIntConstant(module, "FL_JUMPRELEASED", FL_JUMPRELEASED);

        if (PyModule_AddObject(module, "Entity", (PyObject *) &PyQ_Entity_type) == 0) {
            return module;
        }

        Py_DECREF(&PyQ_Entity_type);
        Py_DECREF(module);
    }

    return NULL;
}
