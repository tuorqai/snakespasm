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

//------------------------------------------------------------------------------

enum
{
    PyQ_callback_entity_spawn,
    PyQ_callback_entity_touch,
    PyQ_callback_entity_think,
    PyQ_callback_entity_blocked,
    PyQ_callback_start_frame,
    PyQ_callback_player_pre_think,
    PyQ_callback_player_post_think,
    PyQ_callback_client_kill,
    PyQ_callback_client_connect,
    PyQ_callback_put_client_in_server,
    PyQ_callback_set_new_parms,
    PyQ_callback_set_change_parms,
    PyQ_total_callbacks,
};

struct PyQ_callback
{
    char const *name;
    char const *aftername;
    PyObject *function;
    PyObject *afterfunction;
};

//------------------------------------------------------------------------------

int                 PyQ_servernumber;
qboolean            PyQ_serverloading;
PyQ_StringStorage  *PyQ_string_storage;
int                 PyQ_string_storage_size;

static PyObject    *PyQ_main;
static PyObject    *PyQ_progs;
static PyObject    *PyQ_globals;
static PyObject    *PyQ_locals;
static PyObject    *PyQ_quakeutil_module;
static PyObject    *PyQ_QuakeConsoleOut_type;
static PyObject    *PyQ_QuakeConsoleErr_type;
static PyObject    *PyQ_compile_func;
static qboolean     PyQ_console_output_set;

//------------------------------------------------------------------------------
// quakeutil.py

static char const *PyQ_quakeutil_source =
    "import io, codeop, quakecl\n"
    "\n"
    "class QuakeConsoleOut(io.TextIOBase):\n"
    "    def write(self, str):\n"
    "        quakecl.console_print(str)\n"
    "\n"
    "class QuakeConsoleErr(io.TextIOBase):\n"
    "    def write(self, str):\n"
    "        quakecl.console_error(str)\n"
    "\n"
    "def compile(source, filename='<input>', symbol='single'):\n"
    "    return codeop.compile_command(source, filename, symbol)\n";

static struct PyQ_callback PyQ_callbacks[] = {
    { "entityspawn",        "afterentityspawn",         NULL, NULL },
    { "entitytouch",        "afterentitytouch",         NULL, NULL },
    { "entitythink",        "afterentitythink",         NULL, NULL },
    { "entityblocked",      "afterentityblocked",       NULL, NULL },
    { "startframe",         "afterstartframe",          NULL, NULL },
    { "playerprethink",     "afterplayerprethink",      NULL, NULL },
    { "playerpostthink",    "afterplayerpostthink",     NULL, NULL },
    { "clientkill",         "afterclientkill",          NULL, NULL },
    { "clientconnect",      "afterclientconnect",       NULL, NULL },
    { "putclientinserver",  "afterputclientinserver",   NULL, NULL },
    { "setnewparms",        "aftersetnewparms",         NULL, NULL },
    { "setchangeparms",     "aftersetchangeparms",      NULL, NULL },
};

//------------------------------------------------------------------------------

static void PyQ_CheckError(void)
{
    PyObject *type = PyErr_Occurred();

    if (type) {
        if (!PyQ_console_output_set) {
            Con_Printf("Python error occurred, but console output is not captured.\n");
        }

        PyErr_Print();
    }
}

//------------------------------------------------------------------------------

static PyObject *PyQ_ImportModule(char const *name)
{
    PyObject *unicode = PyUnicode_DecodeFSDefault(name); /* new reference */

    if (unicode) {
        PyObject *module = PyImport_Import(unicode); /* new reference */
        Py_DECREF(unicode);

        if (module) {
            Con_Printf("PyQ_ImportModule: imported module \"%s\"\n", name);
            return module;
        }
    }

    PyQ_CheckError();
    return NULL;
}

/**
 * Print Python Unicode string to console.
 */
static void PyQ_Con_Print(PyObject *unicode)
{
    PyObject *bytes = PyUnicode_AsUTF8String(unicode); /* new reference to bytes() object*/

    if (bytes) {
        const char *str = PyBytes_AsString(bytes); /* get pointer to contents */

        if (str) {
            Con_Printf("%s\n", str);
        }

        Py_DECREF(bytes);
    }

    PyQ_CheckError();
}

/**
 * Print dir(object) to the console.
 */
static void PyQ_Con_PyObject_Dir(PyObject *object)
{
    PyObject *list = PyObject_Dir(object); /* new reference to list() object */

    if (list) {
        PyObject *unicode = PyObject_Str(list); /* new reference to str() object */

        if (unicode) {
            PyQ_Con_Print(unicode);
            Py_DECREF(unicode);
        }

        Py_DECREF(list);
    }

    PyQ_CheckError();
}

/**
 * Print repr(object) to the console.
 */
static void PyQ_Con_PyObject_Repr(PyObject *object)
{
    PyObject *unicode = PyObject_Repr(object); /* new reference to str() object */

    if (unicode) {
        PyQ_Con_Print(unicode);
        Py_DECREF(unicode);
    }

    PyQ_CheckError();
}

static qboolean PyQ_IsBufferMultiline(const char *buffer)
{
    char const *s = buffer;
    int lines = 0;
    qboolean line_is_not_empty = false;

    for (; *s; s++) {
        if (*s == '\n') {
            if (line_is_not_empty) {
                lines++;
                line_is_not_empty = false;
            }
        } else {
            if (*s != '\t' && *s != ' ') {
                line_is_not_empty = true;
            }
        }
    }

    return lines > 1 ? true : false;
}

static PyObject *PyQ_Compile(const char *str)
{
    // Why is this mess? Can't we just compile Python code using C API
    // functions? Well, we can, but...
    // there is no way to tell if the piece of code is invalid or incomplete.
    // Say, user prompts 'def f():' <-- what should happen?
    // The code is correct, but incomplete.
    // Since Python 3.9, the only way to emulate REPL is to use 'interactive
    // loop' which requires separate thread, otherwise the game will hang.
    // The only way left to do the trick in single thread is Pythonic 'codeop'
    // module.

    PyObject *args = Py_BuildValue("(s)", str);

    if (args) {
        PyObject *code = PyObject_Call(PyQ_compile_func, args, NULL);
        Py_DECREF(args);

        if (code) {
            return code;
        }
    }

    return NULL;
}

int PyQ_RunBuffer(const char *buffer)
{
    qboolean error = false;
    PyObject *code = PyQ_Compile(buffer);

    if (code) {
        PyObject *object;

        if (Py_IsNone(code)) {
            return 1;
        }

        object = PyEval_EvalCode(code, PyQ_globals, PyQ_locals);

        if (object) {
            Py_DECREF(object);
        } else {
            error = true;
        }
    }

    PyQ_CheckError();

    return error ? -1 : 0;
}

/**
 * "py" console command.
 */
static void PyQ_Py_f(void)
{
    if (Cmd_Argc() == 1) {
        if (key_dest == key_console) {
            Con_EnterRawMode();
        } else {
            Con_Printf("py: this command is console-only\n");
        }
    } else {
        if (PyQ_RunBuffer(Cmd_Args()) == 1) {
            Con_Printf("py: incomplete input\n");
        }
    }
}

/**
 * Get pyprogs callbacks from dictionary.
 */
static void PyQ_GetCallbacks(PyObject *dict)
{
    int callback;

    for (callback = 0; callback < PyQ_total_callbacks; callback++) {
        struct PyQ_callback *c = &PyQ_callbacks[callback];

        c->function = PyDict_GetItemString(dict, c->name);
        c->afterfunction = PyDict_GetItemString(dict, c->aftername);
    }
}

/**
 * Imports 'pyprogs' module/package.
 */
static void PyQ_LoadProgs(void)
{
    PyQ_progs = PyQ_ImportModule("pyprogs");

    if (PyQ_progs) {
        PyObject *dict = PyObject_GetAttrString(PyQ_progs, "callbacks");

        if (dict && PyDict_Check(dict)) {
            PyQ_GetCallbacks(dict);
        }

        Py_XDECREF(dict);
    }

    PyQ_CheckError();
}

/**
 * "py_clear" console command.
 */
static void PyQ_PyClear_f(void)
{
    int i;

    Py_XDECREF(PyQ_globals);

    PyQ_globals = Py_BuildValue("{}");
    PyQ_locals = PyQ_globals;
}

/**
 * Insert path to the beginning of the module path list.
 */
static void PyQ_InsertModulePath(char const *path)
{
    PyObject *list = PySys_GetObject("path");
    PyObject *unicode = PyUnicode_DecodeFSDefault(path);

    if (list && unicode) {
        if (PyList_Insert(list, 0, unicode) == 0) {
            Con_Printf("PyQ_InsertModulePath: inserted \"%s\" to sys.path\n", path);
        }
    }

    Py_XDECREF(unicode);
    PyQ_CheckError();
}

static int PyQ_RedirectOutput(char const *name, PyObject *type)
{
    PyObject *instance;

    if (type) {
        instance = PyObject_CallNoArgs(type);
        Py_DECREF(type);

        if (instance) {
            if (PySys_SetObject(name, instance) == 0) {
                return 0;
            }

            Py_DECREF(instance);
        }
    }

    return -1;
}

static int PyQ_SetupConsoleOutput(void)
{
    int status = -1;

    PyObject *out = PyObject_CallNoArgs(PyQ_QuakeConsoleOut_type);
    PyObject *err = PyObject_CallNoArgs(PyQ_QuakeConsoleErr_type);

    if (out && err) {
        status = 0;

        if (PySys_SetObject("stdout", out) == -1) {
            status = -1;
        }

        if (PySys_SetObject("stderr", err) == -1) {
            status = -1;
        }
    }

    Py_XDECREF(err);
    Py_XDECREF(out);

    PyQ_CheckError();
    return status;
}

static int PyQ_InitQuakeUtil(void)
{
    PyObject *code = Py_CompileString(PyQ_quakeutil_source, "quakeutil.py", Py_file_input);

    if (code) {
        PyObject *module = PyImport_ExecCodeModule("quakeutil", code);

        if (module) {
            PyQ_quakeutil_module = module;

            PyQ_QuakeConsoleOut_type = PyObject_GetAttrString(PyQ_quakeutil_module, "QuakeConsoleOut");
            PyQ_QuakeConsoleErr_type = PyObject_GetAttrString(PyQ_quakeutil_module, "QuakeConsoleErr");
            PyQ_compile_func = PyObject_GetAttrString(PyQ_quakeutil_module, "compile");

            if (PyQ_QuakeConsoleOut_type && PyQ_QuakeConsoleErr_type && PyQ_compile_func) {
                return 0;
            }

            Py_XDECREF(PyQ_compile_func);
            Py_XDECREF(PyQ_QuakeConsoleErr_type);
            Py_XDECREF(PyQ_QuakeConsoleOut_type);
        }

        Py_DECREF(code);
    }

    PyQ_CheckError();
    return -1;
}

/**
 * Initialize Python subsystem.
 */
void PyQ_Init(void)
{
    PyConfig config;
    PyStatus status;

    PyConfig_InitIsolatedConfig(&config);
    PyConfig_SetBytesString(&config, &config.program_name, host_parms->argv[0]);

    PyImport_AppendInittab("quake", &PyQ_quake_init);
    PyImport_AppendInittab("quakecl", &PyQ_quakecl_init);

    status = Py_InitializeFromConfig(&config);

    if (PyStatus_Exception(status)) {
        if (PyStatus_IsError(status)) {
            Sys_Error("Python error: %s\n", status.err_msg);
        } else if (PyStatus_IsExit(status)) {
            Sys_Error("Python critical error: %s (%d)\n", status.err_msg, status.exitcode);
        }
    }

    PyQ_InsertModulePath(com_basedir);
    PyQ_InsertModulePath(com_gamedir);

    PyQ_main = PyImport_AddModule("__main__");

    PyQ_globals = Py_BuildValue("{}"); // was PyModule_GetDict(PyQ_main)
    PyQ_locals = PyQ_globals;

    if (PyQ_InitQuakeUtil() == 0) {
        if (PyQ_SetupConsoleOutput() == 0) {
            PyQ_console_output_set = true;
        } else {
            Con_Printf("PyQ_Init: output from Python is not captured\n");
        }
    } else {
        Con_Printf("PyQ_InitQuakeUtil() failed");
    }

    PyQ_LoadProgs();

    Cmd_AddCommand("py", PyQ_Py_f);
    Cmd_AddCommand("py_clear", PyQ_PyClear_f);

    Con_Printf("PyQ_Init: initialized Python successfully\n");
}

/**
 * Shutdown Python subsystem.
 */
void PyQ_Shutdown(void)
{
    Py_FinalizeEx();
}

/**
 * Server is spawning.
 */
void PyQ_PreServerSpawn(void)
{
    PyQ_StringStorage *new_string_storage;

    PyQ_servernumber++;
    PyQ_serverloading = true;

    if (PyQ_string_storage_size < sv.max_edicts) {
        new_string_storage = realloc(PyQ_string_storage, sizeof(*PyQ_string_storage) * sv.max_edicts);

        if (!new_string_storage) {
            Sys_Error("Out of memory."); // never going to happen
        }

        PyQ_string_storage = new_string_storage;
        PyQ_string_storage_size = sv.max_edicts;
    }
}

/**
* Server is spawned.
*/
void PyQ_PostServerSpawn(void)
{
    PyQ_serverloading = false;
}

//------------------------------------------------------------------------------
// QuakeC-style callbacks

static PyObject *GetCallbackObject(int callback, qboolean after)
{
    PyObject *function;

    if (after) {
        function = PyDict_GetItemString(PyQ_globals, PyQ_callbacks[callback].aftername);
        
        if (!function) {
            function = PyQ_callbacks[callback].afterfunction;
        }
    } else {
        function = PyDict_GetItemString(PyQ_globals, PyQ_callbacks[callback].name);
        
        if (!function) {
            function = PyQ_callbacks[callback].function;
        }
    }

    return function;
}

static qboolean CallSimpleCallback(int callback, qboolean after)
{
    qboolean override = false;
    PyObject *function, *result;

    function = GetCallbackObject(callback, after);

    if (!function) {
        return false;
    }

    result = PyObject_CallNoArgs(function);

    if (result && Py_IsTrue(result)) {
        override = true;
    }

    Py_XDECREF(result);
    PyQ_CheckError();
    return override;
}

static qboolean CallEntityCallback(int callback, edict_t *edict, qboolean after)
{
    PyObject *function, *result;
    PyQ_Entity *entity;

    qboolean override = false;

    function = GetCallbackObject(callback, after);

    if (!function) {
        return false;
    }

    entity = (PyQ_Entity *) PyObject_CallNoArgs((PyObject *) &PyQ_Entity_type);

    if (entity) {
        entity->index = NUM_FOR_EDICT(edict);
        result = PyObject_CallOneArg(function, (PyObject *) entity);

        if (result && Py_IsTrue(result)) {
            override = true;
        }

        Py_XDECREF(result);
        Py_DECREF((PyObject *) entity);
    }

    PyQ_CheckError();
    return override;
}

static qboolean CallTwoEntityCallback(int callback, edict_t *edict1, edict_t *edict2, qboolean after)
{
    PyObject *function, *args, *result;
    PyQ_Entity *entity1, *entity2;

    qboolean override = false;

    function = GetCallbackObject(callback, after);

    if (!function) {
        return false;
    }

    entity1 = (PyQ_Entity *) PyObject_CallNoArgs((PyObject *) &PyQ_Entity_type);
    entity2 = (PyQ_Entity *) PyObject_CallNoArgs((PyObject *) &PyQ_Entity_type);

    if (entity1 && entity2) {
        entity1->index = NUM_FOR_EDICT(edict1);
        entity2->index = NUM_FOR_EDICT(edict2);

        args = Py_BuildValue("(OO)", entity1, entity2);

        if (args) {
            result = PyObject_Call(function, args, NULL);

            if (result && Py_IsTrue(result)) {
                override = true;
            }

            Py_XDECREF(result);
            Py_DECREF(args);
        }
    }

    Py_XDECREF((PyObject *) entity1);
    Py_XDECREF((PyObject *) entity2);

    PyQ_CheckError();
    return override;
}

/**
 * Called when entity is about to be spawned.
 */
static qboolean PyQ_EntitySpawn(edict_t *edict, qboolean after)
{
    return CallEntityCallback(PyQ_callback_entity_spawn, edict, after);
}

/**
 * entity.touch()
 */
static qboolean PyQ_EntityTouch(edict_t *edict1, edict_t *edict2, qboolean after)
{
    return CallTwoEntityCallback(PyQ_callback_entity_touch, edict1, edict2, after);
}

/**
 * entity.think()
 */
static qboolean PyQ_EntityThink(edict_t *edict, qboolean after)
{
    return CallEntityCallback(PyQ_callback_entity_think, edict, after);
}

/**
 * entity.blocked()
 */
static qboolean PyQ_EntityBlocked(edict_t *edict1, edict_t *edict2, qboolean after)
{
    return CallTwoEntityCallback(PyQ_callback_entity_blocked, edict1, edict2, after);
}

/**
 * StartFrame()
 */
static qboolean PyQ_StartFrame(qboolean after)
{
    return CallSimpleCallback(PyQ_callback_start_frame, after);
}

/**
 * PlayerPreThink()
 */
static qboolean PyQ_PlayerPreThink(edict_t *edict, qboolean after)
{
    return CallEntityCallback(PyQ_callback_player_pre_think, edict, after);
}

/**
 * PlayerPostThink()
 */
static qboolean PyQ_PlayerPostThink(edict_t *edict, qboolean after)
{
    return CallEntityCallback(PyQ_callback_player_post_think, edict, after);
}

/**
 * ClientKill()
 */
static qboolean PyQ_ClientKill(edict_t *edict, qboolean after)
{
    return CallEntityCallback(PyQ_callback_client_kill, edict, after);
}

/**
 * ClientConnect()
 */
static qboolean PyQ_ClientConnect(edict_t *edict, qboolean after)
{
    return CallEntityCallback(PyQ_callback_client_connect, edict, after);
}

/**
 * PutClientInServer()
 */
static qboolean PyQ_PutClientInServer(edict_t *edict, qboolean after)
{
    return CallEntityCallback(PyQ_callback_put_client_in_server, edict, after);
}

/**
 * SetNewParms()
 */
static qboolean PyQ_SetNewParms(qboolean after)
{
    return CallSimpleCallback(PyQ_callback_set_new_parms, after);
}

/**
 * SetChangeParms()
 */
static qboolean PyQ_SetChangeParms(edict_t *edict, qboolean after)
{
    return CallEntityCallback(PyQ_callback_set_change_parms, edict, after);
}

//------------------------------------------------------------------------------
// Python/QuakeC Adapter

qboolean PyQ_OverrideSpawn(edict_t *edict)
{
    return PyQ_EntitySpawn(edict, false);
}

void PyQ_SupplementSpawn(edict_t *edict)
{
    PyQ_EntitySpawn(edict, true);
}

qboolean PyQ_OverrideProgram(func_t function_index)
{
    edict_t *self = PROG_TO_EDICT(pr_global_struct->self);
    edict_t *other = PROG_TO_EDICT(pr_global_struct->other);

    if (function_index == pr_global_struct->StartFrame) {
        return PyQ_StartFrame(false);
    }
    
    if (function_index == pr_global_struct->PlayerPreThink) {
        return PyQ_PlayerPreThink(self, false);
    }
    
    if (function_index == pr_global_struct->PlayerPostThink) {
        return PyQ_PlayerPostThink(self, false);
    }
    
    if (function_index == pr_global_struct->ClientKill) {
        return PyQ_ClientKill(self, false);
    }
    
    if (function_index == pr_global_struct->ClientConnect) {
        return PyQ_ClientConnect(self, false);
    }
    
    if (function_index == pr_global_struct->PutClientInServer) {
        return PyQ_PutClientInServer(self, false);
    }
    
    if (function_index == pr_global_struct->SetNewParms) {
        return PyQ_SetNewParms(false);
    }

    if (function_index == pr_global_struct->SetChangeParms) {
        return PyQ_SetChangeParms(self, false);
    }

    return false;
}

void PyQ_SupplementProgram(func_t function_index)
{
    edict_t *self = PROG_TO_EDICT(pr_global_struct->self);
    edict_t *other = PROG_TO_EDICT(pr_global_struct->other);

    if (function_index == pr_global_struct->StartFrame) {
        PyQ_StartFrame(true);
        return;
    }
    
    if (function_index == pr_global_struct->PlayerPreThink) {
        PyQ_PlayerPreThink(self, true);
        return;
    }
    
    if (function_index == pr_global_struct->PlayerPostThink) {
        PyQ_PlayerPostThink(self, true);
        return;
    }
    
    if (function_index == pr_global_struct->ClientKill) {
        PyQ_ClientKill(self, true);
        return;
    }
    
    if (function_index == pr_global_struct->ClientConnect) {
        PyQ_ClientConnect(self, true);
        return;
    }
    
    if (function_index == pr_global_struct->PutClientInServer) {
        PyQ_PutClientInServer(self, true);
        return;
    }
    
    if (function_index == pr_global_struct->SetNewParms) {
        PyQ_SetNewParms(true);
        return;
    }

    if (function_index == pr_global_struct->SetChangeParms) {
        PyQ_SetChangeParms(self, true);
        return;
    }
}

qboolean PyQ_OverrideEntityMethod(int em)
{
    edict_t *self = PROG_TO_EDICT(pr_global_struct->self);
    edict_t *other = PROG_TO_EDICT(pr_global_struct->other);

    if (em == em_touch) {
        return PyQ_EntityTouch(self, other, false);
    }
    
    if (em == em_think) {
        return PyQ_EntityThink(self, false);
    }
    
    if (em == em_blocked) {
        return PyQ_EntityBlocked(self, other, false);
    }

    return false;
}

void PyQ_SupplementEntityMethod(int em)
{
    edict_t *self = PROG_TO_EDICT(pr_global_struct->self);
    edict_t *other = PROG_TO_EDICT(pr_global_struct->other);

    if (em == em_touch) {
        PyQ_EntityTouch(self, other, true);
        return;
    }
    
    if (em == em_think) {
        PyQ_EntityThink(self, true);
        return;
    }
    
    if (em == em_blocked) {
        PyQ_EntityBlocked(self, other, true);
        return;
    }
}
