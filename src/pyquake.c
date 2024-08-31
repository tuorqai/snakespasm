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

int                 PyQ_servernumber;
qboolean            PyQ_serverloading;
PyQ_StringStorage  *PyQ_string_storage;
int                 PyQ_string_storage_size;

cvar_t              py_strict = { "py_strict", "1", CVAR_ARCHIVE };
cvar_t              py_override_progs = { "py_override_progs", "0", CVAR_ARCHIVE };

static PyObject    *PyQ_main;
static PyObject    *PyQ_progs;
static PyObject    *PyQ_globals;
static PyObject    *PyQ_locals;
static PyObject    *PyQ_quakeutil_module;
static PyObject    *PyQ_QuakeConsoleOut_type;
static PyObject    *PyQ_QuakeConsoleErr_type;
static PyObject    *PyQ_compile_func;
static qboolean     PyQ_console_output_set;

static PyObject    *PyQ_quakeutil_complete;
static char         PyQ_autocomplete_buffer[1024];

//------------------------------------------------------------------------------
// quakeutil.py

static char const *PyQ_quakeutil_source =
    "import io, codeop, quake\n"
    "from rlcompleter import Completer\n"
    "\n"
    "class QuakeConsoleOut(io.TextIOBase):\n"
    "    def write(self, str):\n"
    "        quake.cl.print(str, end='')\n"
    "\n"
    "class QuakeConsoleErr(io.TextIOBase):\n"
    "    def write(self, str):\n"
    "        quake.cl.print('\\x02', str, sep='', end='')\n"
    "\n"
    "def compile(source, filename='<input>', symbol='single'):\n"
    "    return codeop.compile_command(source, filename, symbol)\n"
    "\n"
    "def complete(line, context):\n"
    "    completer = Completer(context)\n"
    "    lastword = line.split()[-1]\n"
    "    completions = [completer.complete(lastword, 0)]\n"
    "    while completions[-1] != None:\n"
    "        completions.append(completer.complete(lastword, len(completions)))\n"
    "    completions = completions[:-1]\n"
    "    if len(completions) == 1:\n"
    "        s = line.split(' ')\n"
    "        s[-1] = completions[0]\n"
    "        return ' '.join(s)\n"
    "    elif len(completions) > 1:\n"
    "        quake.cl.print(line, ':', sep='')\n"
    "        for c in completions:\n"
    "            quake.cl.print('\\x02', c, sep='  ')\n"
    "\n";

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

/**
* Utility function to copy from PyUnicode object to char buffer.
*/
static int PyQ_strncpy(char *dst, PyObject *src, size_t dstlen)
{
    int result = -1;
    PyObject *bytes = PyUnicode_AsUTF8String(src);

    if (bytes) {
        char const *str = PyBytes_AsString(bytes);

        if (str) {
            strncpy(dst, str, dstlen);
            result = 0;
        }

        Py_DECREF(bytes);
    }

    return result;
}

//------------------------------------------------------------------------------
// Hooks

static int PyQ_InitHooks(void)
{
    int i;

    char const *hooknames[] = {
        "serverspawn", "entityspawn", "entitytouch", "entitythink",
        "entityblocked", "startframe", "playerprethink", "playerpostthink",
        "clientkill", "clientconnect", "putclientinserver", "setnewparms",
        "setchangeparms",
    };

    // PyQ_hooks is a dictionary object which is initialized during the 'quake'
    // module initialization.

    if (!PyQ_hooks) {
        // an exceptional case
        return -1;
    }

    for (i = 0; i < sizeof(hooknames) / sizeof(*hooknames); i++) {
        PyObject *emptylist = PyList_New(0);

        if (!emptylist) {
            // another exceptional case
            return -1;
        }

        PyDict_SetItemString(PyQ_hooks, hooknames[i], emptylist);
    }

    return 0;
}

static int PyQ_HookArgs(PyObject **pargs, edict_t *qedict1, edict_t *qedict2)
{
    *pargs = NULL;

    // No-entity callback
    if (!qedict1) {
        return 0;
    }

    // Single-entity callback
    if (!qedict2) {
        PyQ__sv_edict *edict = PyObject_New(PyQ__sv_edict, &PyQ__sv_edict_type);

        if (edict) {
            PyObject *args = PyTuple_Pack(1, edict);

            if (args) {
                edict->servernumber = PyQ_servernumber;
                edict->index = NUM_FOR_EDICT(qedict1);

                *pargs = args;
                return 0;
            }

            Py_DECREF(edict);
        }
    }
    
    // Two-entity callback
    if (qedict1 && qedict2) {
        PyQ__sv_edict *edict1 = PyObject_New(PyQ__sv_edict, &PyQ__sv_edict_type);

        if (edict1) {
            PyQ__sv_edict *edict2 = PyObject_New(PyQ__sv_edict, &PyQ__sv_edict_type);

            if (edict2) {
                PyObject *args = PyTuple_Pack(2, edict1, edict2);

                if (args) {
                    edict1->servernumber = edict2->servernumber = PyQ_servernumber;
                    edict1->index = NUM_FOR_EDICT(qedict1);
                    edict2->index = NUM_FOR_EDICT(qedict2);

                    *pargs = args;
                    return 0;
                }

                Py_DECREF(edict2);
            }

            Py_DECREF(edict1);
        }
    }

    return -1;
}

static int PyQ_CallHook(char const *name, edict_t *qedict1, edict_t *qedict2)
{
    PyObject *item, *args;

    item = PyDict_GetItemString(PyQ_hooks, name);

    if (!item) {
        return -1;
    }

    if (PyQ_HookArgs(&args, qedict1, qedict2) == -1) {
        return -1;
    }

    if (PyList_Check(item)) {
        // Is this a list? (normal situation)
        Py_ssize_t i, len = PyList_GET_SIZE(item);

        for (i = 0; i < len; i++) {
            PyObject *listitem = PyList_GET_ITEM(item, i);

            if (!PyObject_CallObject(listitem, args)) {
                Py_XDECREF(args);
                return -1;
            }
        }
    } else if (PyCallable_Check(item)) {
        // If it's not a list... is it callable?
        if (!PyObject_CallObject(item, args)) {
            Py_XDECREF(args);
            return -1;
        }
    } else {
        // User is an idiot and/or fucked up, tell them.
        PyErr_SetString(PyExc_RuntimeError, "hook is neither a list nor callable\n");
        Py_XDECREF(args);
        return -1;
    }

    Py_XDECREF(args);

    return 0;
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

char const *PyQ_AutoComplete(char const *line)
{
    PyObject *str, *result;
    int copied;

    if (!PyQ_quakeutil_complete) {
        PyQ_quakeutil_complete = PyObject_GetAttrString(PyQ_quakeutil_module, "complete");

        if (!PyQ_quakeutil_complete) {
            PyErr_Print();
            return NULL;
        }
    }

    str = PyUnicode_FromString(line);
    result = NULL;

    if (str) {
        Py_INCREF(PyQ_globals);
        PyObject *args = PyTuple_Pack(2, str, PyQ_globals);

        if (args) {
            result = PyObject_CallObject(PyQ_quakeutil_complete, args);
            Py_DECREF(args);
        } else {
            Py_DECREF(PyQ_globals);
        }
    } else {
        Py_XDECREF(str);
    }

    if (!result) {
        PyErr_Print();
        return NULL;
    }

    if (Py_IsNone(result)) {
        return NULL;
    }

    copied = PyQ_strncpy(PyQ_autocomplete_buffer, result, sizeof(PyQ_autocomplete_buffer));
    Py_DECREF(result);

    if (copied == -1) {
        PyErr_Print();
        return NULL;
    }

    return PyQ_autocomplete_buffer;
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
 * Imports 'pyprogs' module/package.
 */
static void PyQ_LoadProgs(void)
{
    PyObject *progs = PyQ_progs
        ? PyImport_ReloadModule(PyQ_progs)
        : PyImport_ImportModule("pyprogs");

    if (!progs) {
        PyErr_Print();
        Con_Warning("PyQ_LoadProgs: failed to load 'pyprogs' module\n");
        return;
    }

    PyQ_progs = progs;
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

    if (PyQ_InitHooks() == -1) {
        Sys_Error("Python error");
    }

    Cvar_RegisterVariable(&py_strict);
    Cvar_RegisterVariable(&py_override_progs);
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

    PyQ_LoadProgs();
}

/**
* Server is spawned.
*/
void PyQ_PostServerSpawn(void)
{
    PyQ_serverloading = false;
    PyQ_CallHook("serverspawn", NULL, NULL);
}

//------------------------------------------------------------------------------
// Python/QuakeC Adapter
//
// This was reworked heavily. In the past, every callback had two versions,
// which were called before or after some event. Callback which was called
// before event, could return True value to signal that QuakeC function
// should not be called (overridden). Now callbacks (or hooks) called after
// QuakeC function was executed, but they still could be suppressed by
// "py_override_progs" cvar.

qboolean PyQ_OverrideSpawn(edict_t *edict)
{
    return py_override_progs.value;
}

void PyQ_SupplementSpawn(edict_t *edict)
{
    if (PyQ_CallHook("entityspawn", edict, NULL) == -1) {
        PyErr_Print();

        if (py_strict.value) {
            Host_Error("Python error");
        }
    }
}

qboolean PyQ_OverrideProgram(func_t function_index)
{
    return py_override_progs.value;
}

void PyQ_SupplementProgram(func_t function_index)
{
    int result = -1;
    edict_t *self = PROG_TO_EDICT(pr_global_struct->self);
    edict_t *other = PROG_TO_EDICT(pr_global_struct->other);

    if (function_index == pr_global_struct->StartFrame) {
        result = PyQ_CallHook("startframe", NULL, NULL);
    } else if (function_index == pr_global_struct->PlayerPreThink) {
        result = PyQ_CallHook("playerprethink", self, NULL);
    } else if (function_index == pr_global_struct->PlayerPostThink) {
        result = PyQ_CallHook("playerpostthink", self, NULL);
    } else if (function_index == pr_global_struct->ClientKill) {
        result = PyQ_CallHook("clientkill", self, NULL);
    } else if (function_index == pr_global_struct->ClientConnect) {
        result = PyQ_CallHook("clientconnect", self, NULL);
    } else if (function_index == pr_global_struct->PutClientInServer) {
        result = PyQ_CallHook("putclientinserver", self, NULL);
    } else if (function_index == pr_global_struct->SetNewParms) {
        result = PyQ_CallHook("setnewparms", NULL, NULL);
    } else if (function_index == pr_global_struct->SetChangeParms) {
        result = PyQ_CallHook("setchangeparms", self, NULL);
    } else {
        result = 0;
    }

    if (result == -1) {
        PyErr_Print();

        if (py_strict.value) {
            Host_Error("PyQ_SupplementProgram: Python error occurred");
        }
    }
}

qboolean PyQ_OverrideEntityMethod(int em)
{
    return py_override_progs.value;
}

void PyQ_SupplementEntityMethod(int em)
{
    int result = -1;
    edict_t *self = PROG_TO_EDICT(pr_global_struct->self);
    edict_t *other = PROG_TO_EDICT(pr_global_struct->other);

    if (em == em_touch) {
        result = PyQ_CallHook("entitytouch", self, other);
    } else if (em == em_think) {
        result = PyQ_CallHook("entitythink", self, NULL);
    } else if (em == em_blocked) {
        result = PyQ_CallHook("entityblocked", self, other);
    } else {
        Host_Error("PyQ_SupplementEntityMethod: unknown method");
    }

    if (result == -1) {
        PyErr_Print();

        if (py_strict.value) {
            Host_Error("PyQ_SupplementEntityMethod: Python error occurred");
        }
    }
}
