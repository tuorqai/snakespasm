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

static PyObject     *PyQ_globals;

static char         PyQ_autocomplete_buffer[1024];

//------------------------------------------------------------------------------
// engineglue: basic glue module

static PyObject *PyQ_engineglue_module;
static PyObject *PyQ_engineglue_compile_f;
static PyObject *PyQ_engineglue_complete_f;

/**
 * engineglue._con_write(str, colored=False)
 * Outputs a string to the Quake console.
 * The string should not exceed 1024 chars.
 */
static PyObject *PyQ_engineglue__con_write(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char const *str;
    int colored = 0;

    char *kwlist[] = { "str", "colored", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|p", kwlist, &str, &colored)) {
        return NULL;
    }

    if (colored) {
        Con_Printf("%c%s", 2, str);
    } else {
        Con_Printf("%s", str);
    }

    Py_RETURN_NONE;
}

static PyObject *PyQ_engineglue__chdir(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_RuntimeError, "changing directories is not allowed");
    return NULL;
}

static PyObject *PyQ_engineglue__popen(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_RuntimeError, "no pipes save lives");
    return NULL;
}

static PyObject *PyQ_engineglue__system(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_RuntimeError, "calling external processes is not allowed");
    return NULL;
}

static PyMethodDef PyQ_engineglue_methods[] = {
    { "_con_write",     (PyCFunction) PyQ_engineglue__con_write,    METH_VARARGS | METH_KEYWORDS },
    { "_chdir",         PyQ_engineglue__chdir,                      METH_VARARGS },
    { "_popen",         PyQ_engineglue__popen,                      METH_VARARGS },
    { "_system",        PyQ_engineglue__system,                     METH_VARARGS },
    { NULL },
};

static PyModuleDef PyQ_engineglue_moddef = {
    PyModuleDef_HEAD_INIT,
    "engineglue",                   // m_name
    NULL,                           // m_doc
    -1,                             // m_size
    PyQ_engineglue_methods,         // m_methods
    NULL,                           // m_slots
    NULL,                           // m_traverse
    NULL,                           // m_clear
    NULL,                           // m_free
};

/**
 * Portion of engineglue written in Python
 */
static char const *PyQ_engineglue_pycode =
    "import io, codeop, os, sys\n"
    "from rlcompleter import Completer\n"
    "\n"
    "class ConsoleOutput(io.TextIOBase):\n"
    "    def __init__(self, colored=False, chunk_size=1024):\n"
    "        self.colored = colored\n"
    "        self.chunk_size = chunk_size\n"
    "    def write(self, str):\n"
    "        for i in range(0, len(str), self.chunk_size):\n"
    "            _con_write(str[i:i + self.chunk_size], self.colored)\n"
    "    def isatty(self):\n"
    "        return False\n"
    "\n"
    "def _setup_glue(basedir, gamedir):\n"
    "    os.chdir = _chdir\n"
    "    os.popen = _popen\n"
    "    os.system = _system\n"
    "    sys.stdin = None\n"
    "    sys.stdout = ConsoleOutput(colored=False)\n"
    "    sys.stderr = ConsoleOutput(colored=True)\n"
    "    sys.path.insert(0, f'{basedir}/scripts')\n"
    "    sys.path.insert(0, f'{gamedir}/scripts')\n"
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
    "        print(line, ':', sep='')\n"
    "        for c in completions:\n"
    "            print('\\x02', c, sep='  ')\n"
    "\n";

PyObject *PyQ_engineglue_init(void)
{
    PyObject *module;
    PyObject *dict;
    PyObject *pycode_result;
    qboolean is_pycode_compiled;
    PyObject *setup_glue_f;
    PyObject *setup_glue_args;
    PyObject *setup_glue_result;
    qboolean is_setup_glue_failed;

    module = PyModule_Create(&PyQ_engineglue_moddef);

    if (!module) {
        return NULL;
    }

    dict = PyModule_GetDict(module);

    if (!dict) {
        goto error;
    }

    pycode_result = PyRun_String(PyQ_engineglue_pycode, Py_file_input, dict, dict);
    is_pycode_compiled = pycode_result ? true : false;

    Py_XDECREF(pycode_result);

    if (!is_pycode_compiled) {
        goto error;
    }

    setup_glue_f = PyObject_GetAttrString(module, "_setup_glue");
    setup_glue_args = Py_BuildValue("(ss)", com_basedir, com_gamedir);
    setup_glue_result = PyObject_CallObject(setup_glue_f, setup_glue_args);
    is_setup_glue_failed = (setup_glue_result == NULL);

    Py_XDECREF(setup_glue_args);
    Py_XDECREF(setup_glue_result);

    if (is_setup_glue_failed) {
        goto error;
    }

    Py_DECREF(setup_glue_result);

    PyQ_engineglue_compile_f = PyObject_GetAttrString(module, "compile");
    PyQ_engineglue_complete_f = PyObject_GetAttrString(module, "complete");

    if (!PyQ_engineglue_compile_f || !PyQ_engineglue_complete_f) {
        goto error;
    }

    return module;

error:
    Py_XDECREF(dict);
    Py_DECREF(module);

    return NULL;
}

//------------------------------------------------------------------------------
// quake: this module exposes the engine data to Python

static PyObject *PyQ_quake_module;
static PyObject *PyQ_quake_call_hook_f;

static PyMethodDef PyQ_quake_methods[] = {
    { NULL },
};

static PyModuleDef PyQ_quake_moddef = {
    PyModuleDef_HEAD_INIT,
    "quake",                        // m_name
    NULL,                           // m_doc
    -1,                             // m_size
    PyQ_quake_methods,              // m_methods
    NULL,                           // m_slots
    NULL,                           // m_traverse
    NULL,                           // m_clear
    NULL,                           // m_free
};

static char const *PyQ_quake_pycode =
    "hooks = {\n"
    "    'serverspawn': [],\n"
    "    'entityspawn': [],\n"
    "    'entitytouch': [],\n"
    "    'entitythink': [],\n"
    "    'entityblocked': [],\n"
    "    'startframe': [],\n"
    "    'playerprethink': [],\n"
    "    'playerpostthink': [],\n"
    "    'clientkill': [],\n"
    "    'clientconnect': [],\n"
    "    'putclientinserver': [],\n"
    "    'setnewparms': [],\n"
    "    'setchangeparms': [],\n"
    "}\n"
    "\n"
    "def call_hook(name, *args):\n"
    "    for h in hooks[name]:\n"
    "        try:\n"
    "            h(*args)\n"
    "        except Exception:\n"
    "            print(f'The hook {h} failed to run and was removed.')\n"
    "            hooks[name].remove(h)\n"
    "\n";

static PyObject *PyQ_quake_init(void)
{
    PyObject *module;
    PyObject *dict;
    PyObject *pycode_result;
    qboolean is_pycode_compiled;

    module = PyModule_Create(&PyQ_quake_moddef);

    if (!module) {
        return NULL;
    }

    dict = PyModule_GetDict(module);

    if (!dict) {
        goto error;
    }

    pycode_result = PyRun_String(PyQ_quake_pycode, Py_file_input, dict, dict);
    is_pycode_compiled = pycode_result ? true : false;

    Py_XDECREF(pycode_result);

    if (!is_pycode_compiled) {
        goto error;
    }

    PyQ_hooks = PyObject_GetAttrString(module, "hooks");
    PyQ_quake_call_hook_f = PyObject_GetAttrString(module, "call_hook");

    if (!PyQ_hooks || !PyQ_quake_call_hook_f) {
        goto error;
    }

    return module;

error:
    Py_XDECREF(PyQ_hooks);
    Py_DECREF(module);

    return NULL;
}

//------------------------------------------------------------------------------

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
    PyObject *args;
    PyObject *result;
    qboolean is_failed;

#if 0
    if (PyQ_HookArgs(&args, qedict1, qedict2) == -1) {
        return -1;
    }
#endif

    args = Py_BuildValue("(s)", name);
    result = PyObject_CallObject(PyQ_quake_call_hook_f, args);
    is_failed = (result == NULL);

    Py_XDECREF(result);
    Py_XDECREF(args);

    if (is_failed) {
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------

static qboolean PyQ_LoadSingleScript(PyObject *mScript)
{
    qboolean isSucceeded = false;
    PyObject *fOnLoad = PyObject_GetAttrString(mScript, "on_load");

    if (fOnLoad && PyCallable_Check(fOnLoad)) {
        PyObject *result = PyObject_CallNoArgs(fOnLoad);
        isSucceeded = (result != NULL);
        Py_XDECREF(result);
    }

    Py_XDECREF(fOnLoad);
    return isSucceeded;
}

static qboolean PyQ_LoadScripts(void)
{
    PyObject *mConfig = PyImport_ImportModule("config");

    if (!mConfig) {
        PyErr_Print();
        return false;
    }

    PyObject *aScripts = PyObject_GetAttrString(mConfig, "scripts");

    if (aScripts && PyList_Check(aScripts)) {
        Py_ssize_t size = PyList_Size(aScripts);

        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject *sScriptName = PyList_GetItem(aScripts, i);
            char const *scriptName = PyUnicode_AsUTF8(sScriptName);
            PyObject *mScript = PyImport_ImportModule(scriptName);

            if (mScript && PyQ_LoadSingleScript(mScript)) {
                Con_Printf("[Python] Loaded script: %s\n", scriptName);
            } else {
                Con_Printf("[Python] Failed to load script: %s\n", scriptName);
                PyErr_Print();
            }
        }
    }

    Py_XDECREF(aScripts);
    Py_DECREF(mConfig);

    return true;
}

static qboolean PyQ_ClearGlobals(void)
{
    if (PyQ_globals) {
        Py_DECREF(PyQ_globals);
    }

    PyQ_globals = Py_BuildValue("{}");

    if (!PyQ_globals) {
        return false;
    }

    PyDict_SetItemString(PyQ_globals, "__name__", PyUnicode_FromString("__main__"));
    PyDict_SetItemString(PyQ_globals, "__builtins__", PyEval_GetBuiltins());
    PyDict_SetItemString(PyQ_globals, "quake", PyQ_quake_module);

    return true;
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
        PyObject *code = PyObject_Call(PyQ_engineglue_compile_f, args, NULL);
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

    if (!PyQ_engineglue_complete_f) {
        return NULL;
    }

    str = PyUnicode_FromString(line);
    result = NULL;

    if (str) {
        Py_INCREF(PyQ_globals);
        PyObject *args = PyTuple_Pack(2, str, PyQ_globals);

        if (args) {
            result = PyObject_CallObject(PyQ_engineglue_complete_f, args);
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
    PyObject *code = PyQ_Compile(buffer);
    PyObject *result;
    qboolean is_success;

    if (!code) {
        return -1;
    }

    if (Py_IsNone(code)) {
        return 1;
    }

    result = PyEval_EvalCode(code, PyQ_globals, PyQ_globals);
    is_success = result ? true : false;

    Py_XDECREF(result);

    if (PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
            PyErr_Clear();
            Con_Printf("%cSorry, exit is disabled. Use Ctrl-D to exit from REPL and the console command 'quit' to exit properly.\n", 2);
        } else {
            PyErr_Print();
        }
    }

    return is_success ? 0 : -1;
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
 * "py_clear" console command.
 */
static void PyQ_PyClear_f(void)
{
    PyQ_ClearGlobals();
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

    PyImport_AppendInittab("engineglue", &PyQ_engineglue_init);
    PyImport_AppendInittab("quake", &PyQ_quake_init);

    status = Py_InitializeFromConfig(&config);

    if (PyStatus_Exception(status)) {
        if (PyStatus_IsError(status)) {
            Sys_Error("Python error: %s\n", status.err_msg);
        } else if (PyStatus_IsExit(status)) {
            Sys_Error("Python critical error: %s (%d)\n", status.err_msg, status.exitcode);
        }
    }

    PyQ_engineglue_module = PyImport_ImportModule("engineglue");
    PyQ_quake_module = PyImport_ImportModule("quake");

    if (!PyQ_LoadScripts()) {
        Sys_Error("Python error: can't load scripts");
    }

    // This is for REPL only
    if (!PyQ_ClearGlobals()) {
        Sys_Error("Python error: can't create globals dict");
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

