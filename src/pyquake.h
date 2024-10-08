
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

PyObject *PyQ_quake_init(void);

// Called from ED_LoadFromFile() in pr_edict.c
// prevents QuakeC spawn function lookup and execution if returns true
qboolean PyQ_OverrideSpawn(edict_t *edict);

// Called from ED_LoadFromFile() in pr_edict.c after QuakeC spawn function executed
void PyQ_SupplementSpawn(edict_t *edict);

// Called from PR_ExecuteProgram() in pr_exec.c before QuakeC function execution
// prevents QuakeC function execution if returns true
qboolean PyQ_OverrideProgram(func_t function_index);

// Called from PR_ExecuteProgram() in pr_exec.c after QuakeC function execute
void PyQ_SupplementProgram(func_t function_index);

enum
{
    em_touch,
    em_think,
    em_blocked,
};

qboolean PyQ_OverrideEntityMethod(int em);
void PyQ_SupplementEntityMethod(int em);

//------------------------------------------------------------------------------

#endif // QUAKE_PQ_H
