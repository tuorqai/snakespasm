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

int PyQ_servernumber;

static PyObject    *PyQ_main;
static PyObject    *PyQ_globals;
static PyObject    *PyQ_locals;
static char         PyQ_scriptdir[4096];
static PyObject    *PyQ_quakeutil_module;
static PyObject    *PyQ_start_module;
static PyObject    *PyQ_QuakeConsoleOut_type;
static PyObject    *PyQ_QuakeConsoleErr_type;
static PyObject    *PyQ_compile_func;
static qboolean     PyQ_console_output_set;

//------------------------------------------------------------------------------
// quakeutil.py

static char const *PyQ_quakeutil_source =
    "import io, codeop, quake\n"
    "\n"
    "class QuakeConsoleOut(io.TextIOBase):\n"
    "    def write(self, str):\n"
    "        quake.console_print(str)\n"
    "\n"
    "class QuakeConsoleErr(io.TextIOBase):\n"
    "    def write(self, str):\n"
    "        quake.console_error(str)\n"
    "\n"
    "def compile(source, filename='<input>', symbol='single'):\n"
    "    return codeop.compile_command(source, filename, symbol)\n";

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
 * Import single script or reload it if it already loaded.
 */
static void PyQ_LoadSingleScript(PyObject *unicode)
{
    PyObject *dict = PyImport_GetModuleDict();

    if (dict) {
        PyObject *module = PyDict_GetItem(dict, unicode);

        if (module) {
            module = PyImport_ReloadModule(module);
        } else {
            module = PyImport_Import(unicode);
        }
    }

    PyQ_CheckError();
}

/**
 * Imports starts.py and loads scripts.
 */
static void PyQ_ReloadScripts(void)
{
    PyObject *module = PyQ_start_module ? PyQ_start_module : PyQ_ImportModule("start");

    if (module) {
        PyObject *list = PyObject_GetAttrString(module, "scripts");

        if (list && PyList_Check(list)) {
            int i;
    
            for (i = 0; i < PyList_Size(list); i++) {
                PyObject *unicode = PyList_GET_ITEM(list, i);

                if (unicode && PyUnicode_Check(unicode)) {
                    PyQ_LoadSingleScript(unicode);
                }

                Py_XDECREF(unicode);
            }
        }

        Py_XDECREF(list);
    }

    PyQ_start_module = module;

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

    PyQ_ReloadScripts();
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

    PyImport_AppendInittab("quake", &PyQ_InitBuiltins);

    status = Py_InitializeFromConfig(&config);

    if (PyStatus_Exception(status)) {
        if (PyStatus_IsError(status)) {
            Con_Printf("Python error: %s\n", status.err_msg);
        } else if (PyStatus_IsExit(status)) {
            Con_Printf("Python critical error: %s (%d)\n", status.err_msg, status.exitcode);
        }
        return;
    }

    PyQ_InsertModulePath(com_basedir);
    PyQ_InsertModulePath(com_gamedir);

    q_snprintf(PyQ_scriptdir, sizeof(PyQ_scriptdir), "%s/scripts", com_gamedir);
    PyQ_InsertModulePath(PyQ_scriptdir);

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

    PyQ_ReloadScripts();

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
 * Called when server starts.
 */
void PyQ_LoadScripts(void)
{
    PyQ_servernumber++;
}
