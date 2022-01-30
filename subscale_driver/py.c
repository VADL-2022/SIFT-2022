#include "py.h"
#include "lib/pf_string.h"

#include <python3.7m/osdefs.h>
#include <python3.7m/frameobject.h>
#include <stdbool.h>

#define PyString_FromString PyUnicode_FromString

static bool s_sys_path_add_dir(const char *filename)
{
    if(strlen(filename) >= 512)
        return false;

    char copy[512];
    strcpy(copy, filename);

    char *end = copy + (strlen(copy) - 1);
    while(end > copy && *end != '/')
        end--;

    if(end == copy)
        return false;
    *end = '\0';

    PyObject *sys_path = PySys_GetObject("path");
    assert(sys_path);

    if(0 != PyList_Append(sys_path, Py_BuildValue("s", copy)) )
        return false;

    return true;
}

static void s_print_err_text(int offset, const char *text, size_t maxout, char out[maxout]);
bool s_print_source_line(const char *filename, int lineno, int indent,
                         size_t maxout, char out[static maxout]);

struct py_err_ctx{
    bool           occurred;
    PyObject      *type;
    PyObject      *value;
    PyObject      *traceback;
};

static void s_err_clear(struct py_err_ctx* s_err_ctx)
{
    Py_CLEAR(s_err_ctx->type);
    Py_CLEAR(s_err_ctx->value);
    Py_CLEAR(s_err_ctx->traceback);
    s_err_ctx->occurred = false;
}

int s_parse_syntax_error(PyObject *err, PyObject **message, const char **filename, 
                                 int *lineno, int *offset, const char **text)
{
    long hold;
    PyObject *v;

    /* old style errors */
    if (PyTuple_Check(err))
        return PyArg_ParseTuple(err, "O(ziiz)", message, filename,
                                lineno, offset, text);

    *message = NULL;

    /* new style errors.  `err' is an instance */
    *message = PyObject_GetAttrString(err, "msg");
    if (!*message)
        goto finally;

    v = PyObject_GetAttrString(err, "filename");
    if (!v)
        goto finally;
    if (v == Py_None) {
        Py_DECREF(v);
        *filename = NULL;
    }
    else {
#define PyString_AsString PyUnicode_AsUTF8
        *filename = PyString_AsString(v);
        Py_DECREF(v);
        if (!*filename)
            goto finally;
    }

    v = PyObject_GetAttrString(err, "lineno");
    if (!v)
        goto finally;
    #define PyInt_AsLong PyLong_AsLong
    hold = PyInt_AsLong(v);
    Py_DECREF(v);
    if (hold < 0 && PyErr_Occurred())
        goto finally;
    *lineno = (int)hold;

    v = PyObject_GetAttrString(err, "offset");
    if (!v)
        goto finally;
    if (v == Py_None) {
        *offset = -1;
        Py_DECREF(v);
    } else {
        hold = PyInt_AsLong(v);
        Py_DECREF(v);
        if (hold < 0 && PyErr_Occurred())
            goto finally;
        *offset = (int)hold;
    }

    v = PyObject_GetAttrString(err, "text");
    if (!v)
        goto finally;
    if (v == Py_None) {
        Py_DECREF(v);
        *text = NULL;
    }
    else {
        *text = PyString_AsString(v);
        Py_DECREF(v);
        if (!*text)
            goto finally;
    }
    return 1;

finally:
    Py_XDECREF(*message);
    return 0;
}

void S_Error_Update(struct py_err_ctx *err_ctx) {
    // Handle with command-line too: printf's below //

    const char* msg = "Unhandled Python Exception";
    printf("%s\n", msg);
    if (true) {
        char buff[256] = "";
        PyObject *repr;

        assert(err_ctx->type);
        if(PyExceptionClass_Check(err_ctx->type) && PyExceptionClass_Name(err_ctx->type)) {
            const char *clsname = PyExceptionClass_Name(err_ctx->type);
            char *dot = strrchr(clsname, '.');
            if(dot)
                clsname = dot+1;
            pf_strlcpy(buff, clsname, sizeof(buff));
        }else{
            repr = PyObject_Str(err_ctx->type);
            if(repr) {
                #define PyString_AS_STRING PyUnicode_AsUTF8
                pf_strlcpy(buff, PyString_AS_STRING(repr), sizeof(buff));
            }
            Py_XDECREF(repr);
        }

        PyObject *message = NULL;
        const char *filename, *text;
        int lineno, offset;

        if(err_ctx->value) {

            if(s_parse_syntax_error(err_ctx->value, &message, &filename, &lineno, &offset, &text)) {
                repr = PyObject_Str(message);
                Py_DECREF(message);
            }else{
                PyErr_Clear();
                repr = PyObject_Str(err_ctx->value);
            }

            if(repr && strlen(PyString_AS_STRING(repr))) {
                pf_strlcat(buff, ": ", sizeof(buff));
                pf_strlcat(buff, PyString_AS_STRING(repr), sizeof(buff));
            }
            Py_XDECREF(repr);
        }

	printf("%s\n", buff);

        if(message) {

            pf_snprintf(buff, sizeof(buff), "    File: \"%s\"", filename ? filename : "<string>", lineno);
	    printf("%s\n", buff);

            pf_snprintf(buff, sizeof(buff), "    Line: %d", lineno);
	    printf("%s\n", buff);

            if(text) {
            
                s_print_err_text(offset, text, sizeof(buff), buff);

                int idx = 0;
                char *saveptr;
                char *curr = pf_strtok_r(buff, "\n", &saveptr);
                while(curr) {
		    printf("%s\n", curr);
                    curr = pf_strtok_r(NULL, "\n", &saveptr);
                    idx++;
                }
            }
        }
        Py_XDECREF(message);

        if(err_ctx->traceback) {
        
	    const char* msg = "Traceback:";
	    printf("%s\n", msg);

            long depth = 0;
            PyTracebackObject *tb = (PyTracebackObject*)err_ctx->traceback, *tbLast;
	    PyTracebackObject *lastTB = tb;

            while (tb != NULL) {
                depth++;
                tb = tb->tb_next;
		lastTB = tb;
            }
            tb = (PyTracebackObject*)err_ctx->traceback;
	    lastTB = tb;

            while (tb != NULL) {
                if (depth <= 128) {

                    char filebuff[512] = "";
                    pf_snprintf(filebuff, sizeof(filebuff), "  [%02ld] %s: %d", depth,
                        PyString_AsString(tb->tb_frame->f_code->co_filename), tb->tb_lineno);
		    printf("%s\n", filebuff);

                    char linebuff[512] = "";
                    s_print_source_line(PyString_AsString(tb->tb_frame->f_code->co_filename), tb->tb_lineno, 4, 
                        sizeof(linebuff), linebuff);
                    size_t len = strlen(linebuff);
                    linebuff[len > 0 ? len-1 : 0] = '\0'; /* trim newline */

		    printf("%s\n", linebuff);
                }
                depth--;
		//lastTB = tb;
                tb = tb->tb_next;
            }

	    // // Grab locals on last frame
	    // if (lastTB != NULL) {
	    //   PyObject* locals = lastTB->tb_frame->f_locals;//f_frame->f_locals;
	    //   S_RunString("import code; code.interact(local=locals())", locals);
	    //   // If we leave the above, consider it "continue"'ed:
	    //   err_ctx->occurred = false;
	    //   Py_CLEAR(err_ctx->type);
	    //   Py_CLEAR(err_ctx->value);
	    //   Py_CLEAR(err_ctx->traceback);
	    //   G_SetSimState(err_ctx->prev_state);
	    // }
        }

        // if(nk_button_label(ctx, "Continue")) {

        //     err_ctx->occurred = false;
        //     Py_CLEAR(err_ctx->type);
        //     Py_CLEAR(err_ctx->value);
        //     Py_CLEAR(err_ctx->traceback);
        //     G_SetSimState(err_ctx->prev_state);
        // }

    }
}

void S_ShowLastError(void)
{
    static struct py_err_ctx  s_err_ctx = {false,};
    s_err_ctx.occurred = PyErr_Occurred();
    PyErr_Fetch(&s_err_ctx.type, &s_err_ctx.value, &s_err_ctx.traceback);
    PyErr_NormalizeException(&s_err_ctx.type, &s_err_ctx.value, &s_err_ctx.traceback);

    if(s_err_ctx.occurred) {
        /* PyObject *ptype, *pvalue, *ptraceback; */
        /* PyErr_Fetch(&ptype, &pvalue, &s_err_ctx.traceback); */
        //pvalue contains error message
        //ptraceback contains stack snapshot and many other information
        //(see python traceback structure)

        //Get error message
        /* char *pStrErrorMessage = PyString_AsString(s_err_ctx.value); */
        /* puts(pStrErrorMessage); */
        
        S_Error_Update(&s_err_ctx); // Doesn't seem to work right
        s_err_clear(&s_err_ctx);
    }
}

static void s_print_err_text(int offset, const char *text, size_t maxout, char out[maxout])
{
    if(!maxout)
        return;
    out[0] = '\0';

    char *nl;
    if (offset >= 0) {
        if (offset > 0 && offset == strlen(text) && text[offset - 1] == '\n')
            offset--;
        for (;;) {
            nl = strchr(text, '\n');
            if (nl == NULL || nl-text >= offset)
                break;
            offset -= (int)(nl+1-text);
            text = nl+1;
        }
        while (*text == ' ' || *text == '\t') {
            text++;
            offset--;
        }
    }
    pf_strlcat(out, "    ", maxout);
    pf_strlcat(out, text, maxout);
    if (*text == '\0' || text[strlen(text)-1] != '\n')
        pf_strlcat(out, "\n", maxout);
    if (offset == -1)
        return;
    pf_strlcat(out, "    ", maxout);
    offset--;
    while (offset > 0) {
        pf_strlcat(out, " ", maxout);
        offset--;
    }
    pf_strlcat(out, "^\n", maxout);
}

bool s_print_source_line(const char *filename, int lineno, int indent,
                        size_t maxout, char out[static maxout])
{
    if(!maxout)
        return true;
    out[0] = '\0';

    FILE *xfp = NULL;
    char linebuf[2000];
    int i;
    char namebuf[MAXPATHLEN+1];

    if (filename == NULL)
        return false;
    xfp = fopen(filename, "r" PY_STDIOTEXTMODE);
    if (xfp == NULL) {
        /* Search tail of filename in sys.path before giving up */
        PyObject *path;
        const char *tail = strrchr(filename, SEP);
        if (tail == NULL)
            tail = filename;
        else
            tail++;
        path = PySys_GetObject("path");
        if (path != NULL && PyList_Check(path)) {
            Py_ssize_t _npath = PyList_Size(path);
            int npath = Py_SAFE_DOWNCAST(_npath, Py_ssize_t, int);
            size_t taillen = strlen(tail);
            for (i = 0; i < npath; i++) {
                PyObject *v = PyList_GetItem(path, i);
                if (v == NULL) {
                    PyErr_Clear();
                    break;
                }
                #define PyString_Check PyUnicode_Check
                if (PyString_Check(v)) {
                    size_t len;
                    #define PyString_GET_SIZE PyUnicode_GET_SIZE
                    len = PyString_GET_SIZE(v);
                    if (len + 1 + taillen >= MAXPATHLEN)
                        continue; /* Too long */
                    strcpy(namebuf, PyString_AsString(v));
                    if (strlen(namebuf) != len)
                        continue; /* v contains '\0' */
                    if (len > 0 && namebuf[len-1] != SEP)
                        namebuf[len++] = SEP;
                    strcpy(namebuf+len, tail);
                    xfp = fopen(namebuf, "r" PY_STDIOTEXTMODE);
                    if (xfp != NULL) {
                        break;
                    }
                }
            }
        }
    }

    if (xfp == NULL)
        return false;

    for (i = 0; i < lineno; i++) {
        char* pLastChar = &linebuf[sizeof(linebuf)-2];
        do {
            *pLastChar = '\0';
            if (Py_UniversalNewlineFgets(linebuf, sizeof linebuf, xfp, NULL) == NULL)
                break;
            /* fgets read *something*; if it didn't get as
               far as pLastChar, it must have found a newline
               or hit the end of the file;              if pLastChar is \n,
               it obviously found a newline; else we haven't
               yet seen a newline, so must continue */
        } while (*pLastChar != '\0' && *pLastChar != '\n');
    }
    if (i == lineno) {
        char buf[11];
        char *p = linebuf;
        while (*p == ' ' || *p == '\t' || *p == '\014')
            p++;

        /* Write some spaces before the line */
        strcpy(buf, "          ");
        assert (strlen(buf) == 10);
        while (indent > 0) {
            if(indent < 10)
                buf[indent] = '\0';
            pf_strlcat(out, buf, maxout);
            indent -= 10;
        }

        pf_strlcat(out, p, maxout);
        if (strchr(p, '\n') == NULL)
            pf_strlcat(out, "\n", maxout);
    }

    fclose(xfp);
    return true;
}

// Returns true on success
bool S_RunString(const char *str) {
    printf("Running Python: %s\n", str);
    PyObject *main_module = PyImport_AddModule("__main__"); /* borrowed */
    if(!main_module)
      return false;
    
    PyObject *global_dict = PyModule_GetDict(main_module); /* borrowed */
    
    PyObject *result = PyRun_StringFlags(str, Py_file_input /* Py_single_input for a single statement, or Py_file_input for more than a statement */, global_dict, global_dict, NULL);
    Py_XDECREF(result);

    if(PyErr_Occurred()) {
        S_ShowLastError();
	return false;
    }

    return true;
}

// Returns true on success
bool S_RunFile(const char *path, int argc, char **argv)
{
    printf("Running Python file: %s%s", path, argc>1 ? " with args:\n" : "\n");
    for (int i = 1; i < argc; i++) {
        printf("\t%s\n", argv[i]);
    }

    bool ret = false;
    FILE *script = fopen(path, "r");
    if(!script)
        return false;

    char *cargv[argc + 1];
    cargv[0] = (char*)path;
    if(argc) {
        memcpy(cargv + 1, argv, sizeof(*argv) * argc);
    }

    wchar_t *argv_w[argc + 1];

    /* The directory of the script file won't be automatically added by 'PyRun_SimpleFile'.
     * We add it manually to sys.path ourselves. */
    if(!s_sys_path_add_dir(path))
        goto done;

    PyObject *main_module = PyImport_AddModule("__main__"); /* borrowed */
    if(!main_module)
        goto done;
    
    PyObject *global_dict = PyModule_GetDict(main_module); /* borrowed */

    if(PyDict_GetItemString(global_dict, "__file__") == NULL) {
        PyObject *f = PyString_FromString(path);
        if(f == NULL)
            goto done;
        if(PyDict_SetItemString(global_dict, "__file__", f) < 0) {
            Py_DECREF(f);
            goto done;
        }
        Py_DECREF(f);
    }

    // https://docs.python.org/3.7/c-api/sys.html#c.Py_DecodeLocale
    for (size_t i = 0; i < argc + 1; i++) {
      argv_w[i] = Py_DecodeLocale(cargv[i], NULL);
    }
    PySys_SetArgvEx(argc + 1, argv_w, 1);
    PyObject *result = PyRun_File(script, path, Py_file_input, global_dict, global_dict);
    ret = (result != NULL);
    Py_XDECREF(result);
    
    for (size_t i = 0; i < argc; i++) {
      PyMem_RawFree(argv_w[i]);
    }

    if(PyErr_Occurred()) {
        S_ShowLastError();
    }

done:
    fclose(script);
    return ret;
}
