
#ifndef QUAKE_PQ_H
#define QUAKE_PQ_H

//------------------------------------------------------------------------------

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h> // PyMemberDef (not required in 3.12+)

//------------------------------------------------------------------------------

typedef struct {
    PyObject_HEAD
    vec3_t v;
    vec3_t *p;
    char reprbuf[32];
} PyQ_vec;

typedef struct {
    PyObject_HEAD
    int servernumber;
    int index;
} PyQ__sv_edict;

extern PyTypeObject PyQ_vec_type;
extern PyTypeObject PyQ__sv_edict_type;

//------------------------------------------------------------------------------

#define PyQ_ENTITY_STRLEN           64

extern int PyQ_servernumber;
extern qboolean PyQ_serverloading; // check only if sv.active == false

typedef struct {
    struct {
        char classname[PyQ_ENTITY_STRLEN];
        char weaponmodel[PyQ_ENTITY_STRLEN];
        char netname[PyQ_ENTITY_STRLEN];
        char target[PyQ_ENTITY_STRLEN];
        char targetname[PyQ_ENTITY_STRLEN];
        char message[PyQ_ENTITY_STRLEN];
        char noise[PyQ_ENTITY_STRLEN];
        char noise1[PyQ_ENTITY_STRLEN];
        char noise2[PyQ_ENTITY_STRLEN];
        char noise3[PyQ_ENTITY_STRLEN];
    } v;
} PyQ_StringStorage;

extern PyQ_StringStorage *PyQ_string_storage;
extern int PyQ_string_storage_size;

//------------------------------------------------------------------------------

extern PyObject *PyQ_hooks;

//------------------------------------------------------------------------------

void PyQ_Init(void);
void PyQ_Shutdown(void);
void PyQ_PreServerSpawn(void);
void PyQ_PostServerSpawn(void);
char const *PyQ_AutoComplete(char const *line);
int PyQ_RunBuffer(const char *buffer);

void PyQ_OnProgramCall(func_t function_index);
void PyQ_PostProgramCall(func_t function_index);

void PyQ_OnEntitySpawn(edict_t *edict);
void PyQ_PostEntitySpawn(edict_t *edict);
void PyQ_OnEntityThink(edict_t *edict);
void PyQ_PostEntityThink(edict_t *edict);
void PyQ_OnEntityTouch(edict_t *edict, edict_t *other);
void PyQ_PostEntityTouch(edict_t *edict, edict_t *other);
void PyQ_OnEntityBlocked(edict_t *edict, edict_t *other);
void PyQ_PostEntityBlocked(edict_t *edict, edict_t *other);


PyObject *PyQ_quake_init(void);

//------------------------------------------------------------------------------

#endif // QUAKE_PQ_H
