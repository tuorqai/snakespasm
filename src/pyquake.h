
#ifndef QUAKE_PQ_H
#define QUAKE_PQ_H

//------------------------------------------------------------------------------

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h> // PyMemberDef (not required in 3.12+)

//------------------------------------------------------------------------------

extern int PyQ_servernumber;

typedef struct {
    PyObject_HEAD
    int servernumber;
    edict_t *edict;
} PyQ_Entity;

extern PyTypeObject PyQ_Entity_type;

void PyQ_Init(void);
void PyQ_Shutdown(void);
void PyQ_LoadScripts(void);
int PyQ_RunBuffer(const char *buffer);

PyObject *PyQ_InitBuiltins(void);

// void PY_Server_ClientConnect(edict_t *client);
// void PY_Server_PutClientInServer(edict_t *client);
// void PY_Server_OnCommandExecuted(int argc, char **argv);

// void PY_Client_OnCommandExecuted(int argc, char **argv);

//------------------------------------------------------------------------------

#endif // QUAKE_PQ_H
