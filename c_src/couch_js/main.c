// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <jsapi.h>

#include "utf8.h"

int gExitCode = 0;

#define MAX_GC_SIZE 1024 * 1024

#ifdef JS_THREADSAFE
#define SETUP_REQUEST(cx) \
    JS_SetContextThread(cx); \
    JS_BeginRequest(cx);
#define FINISH_REQUEST(cx) \
    JS_EndRequest(cx); \
    JS_ClearContextThread(cx);
#define FINISH_REQUEST_AND_DESTROY(cx) \
    JS_EndRequest(cx); \
    JS_DestroyContext(cx);
#else
#define SETUP_REQUEST(cx)
#define FINISH_REQUEST(cx)
#define FINISH_REQUEST_AND_DESTROY(cx) \
    JS_DestroyContext(cx);
#endif

static JSClass global_class = {
    "GlobalClass",
    JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
evalcx(JSContext *cx, uintN argc, jsval *vp)
{
    jsval* argv = JS_ARGV(cx, vp);
    JSString *str;
    JSObject *sandbox;
    JSContext *subcx;
    const jschar *src;
    size_t srclen;
    JSBool ret = JS_FALSE;
    JSCrossCompartmentCall *call = NULL;

    sandbox = NULL;
    if(!JS_ConvertArguments(cx, argc, argv, "S / o", &str, &sandbox))
    {
        return JS_FALSE;
    }

    subcx = JS_NewContext(JS_GetRuntime(cx), 8L * 1024L);
    if(!subcx)
    {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    SETUP_REQUEST(subcx);

    src = JS_GetStringCharsZ(subcx, str);
    if (src == NULL)
    	return JS_FALSE;

    srclen = JS_GetStringLength(str);

    /* Re-use the compartment associated with the main context,
     * rather than creating a new compartment */
    JSObject *global = JS_GetGlobalObject(cx);
    if(!global)
    {
        goto done;
    }
    call = JS_EnterCrossCompartmentCall(subcx, global);

    if(!sandbox)
    {
        sandbox = JS_NewGlobalObject(subcx, &global_class);
        if(!sandbox || !JS_InitStandardClasses(subcx, sandbox)) goto done;
    }

    if(srclen == 0)
    {
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(sandbox));
    }
    else
    {
        JS_EvaluateUCScript(subcx, sandbox, src, srclen, NULL, 0, &JS_RVAL(cx, vp));
    }
    
    ret = JS_TRUE;

done:
    if(call)
    {
        JS_LeaveCrossCompartmentCall(call);
    }
    /* Don't use FINISH_REQUEST before destroying a context
     * Destroying a context without a thread asserts on threadsafe
     * debug builds */
    FINISH_REQUEST_AND_DESTROY(subcx);
    return ret;
}

static JSBool
gc(JSContext *cx, uintN argc, jsval *vp)
{
    JS_GC(cx);
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool
print(JSContext *cx, uintN argc, jsval *vp)
{
    jsval* argv = JS_ARGV(cx, vp);
    uintN i;
    char *bytes;

    for(i = 0; i < argc; i++)
    {
        bytes = enc_string(cx, argv[i], NULL);
        if(!bytes) return JS_FALSE;

        fprintf(stdout, "%s%s", i ? " " : "", bytes);
        JS_free(cx, bytes);
    }

    fputc('\n', stdout);
    fflush(stdout);
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool
quit(JSContext *cx, uintN argc, jsval *vp)
{
    jsval* argv = JS_ARGV(cx, vp);
    JS_ConvertArguments(cx, argc, argv, "/ i", &gExitCode);
    return JS_FALSE;
}

#ifndef js_fgets
/* js_fgets is not linkable from C consumers with libmozjs >= 2.0,
 * so we reimplement it here */
static int
couchjs_fgets(char *buf, int size, FILE *file)
{
    int n, i, c;
    JSBool crflag;

    n = size - 1;
    if (n < 0)
        return -1;

    crflag = JS_FALSE;
    for (i = 0; i < n && (c = getc(file)) != EOF; i++) {
        buf[i] = c;
        if (c == '\n') {        /* any \n ends a line */
            i++;                /* keep the \n; we know there is room for \0 */
            break;
        }
        if (crflag) {           /* \r not followed by \n ends line at the \r */
            ungetc(c, file);
            break;              /* and overwrite c in buf with \0 */
        }
        crflag = (c == '\r');
    }

    buf[i] = '\0';
    return i;
}
#define js_fgets couchjs_fgets
#endif

static char*
readfp(JSContext* cx, FILE* fp, size_t* buflen)
{
    char* bytes = NULL;
    char* tmp = NULL;
    size_t used = 0;
    size_t byteslen = 256;
    size_t readlen = 0;

    bytes = JS_malloc(cx, byteslen);
    if(bytes == NULL) return NULL;
    
    while((readlen = js_fgets(bytes+used, byteslen-used, stdin)) > 0)
    {
        used += readlen;

        if(bytes[used-1] == '\n')
        {
            bytes[used-1] = '\0';
            break;
        }

        // Double our buffer and read more.
        byteslen *= 2;
        tmp = JS_realloc(cx, bytes, byteslen);
        if(!tmp)
        {
            JS_free(cx, bytes);
            return NULL;
        }
        bytes = tmp;
    }

    *buflen = used;
    return bytes;
}

static JSBool
readline(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;
    char* bytes;
    char* tmp;
    size_t byteslen;

    /* GC Occasionally */
    JS_MaybeGC(cx);

    bytes = readfp(cx, stdin, &byteslen);
    if(!bytes) return JS_FALSE;
    
    /* Treat the empty string specially */
    if(byteslen == 0)
    {
        JS_SET_RVAL(cx, vp, JS_GetEmptyStringValue(cx));
        JS_free(cx, bytes);
        return JS_TRUE;
    }

    /* Shrink the buffer to the real size */
    tmp = JS_realloc(cx, bytes, byteslen);
    if(!tmp)
    {
        JS_free(cx, bytes);
        return JS_FALSE;
    }
    bytes = tmp;
    
    str = dec_string(cx, bytes, byteslen);
    JS_free(cx, bytes);

    if(!str) return JS_FALSE;

    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(str));

    return JS_TRUE;
}

static JSBool
seal(JSContext *cx, uintN argc, jsval *vp) {
    jsval* argv = JS_ARGV(cx, vp);
    JSObject *target;
    JSBool deep = JS_FALSE;

    if (!JS_ConvertArguments(cx, argc, argv, "o/b", &target, &deep))
        return JS_FALSE;
    if (!target)
    {
        JS_SET_RVAL(cx, vp, JSVAL_VOID);
        return JS_TRUE;
    }
    JSBool res = deep ? JS_DeepFreezeObject(cx, target) : JS_FreezeObject(cx, target);
    if (res == JS_TRUE)
        JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return res;
}

static void
execute_script(JSContext *cx, JSObject *obj, const char *filename) {
    FILE *file;
    JSObject *script;
    jsval result;

    if(!filename || strcmp(filename, "-") == 0)
    {
        file = stdin;
    }
    else
    {
        file = fopen(filename, "r");
        if (!file)
        {
            fprintf(stderr, "could not open script file %s\n", filename);
            gExitCode = 1;
            return;
        }
    }

    script = JS_CompileFileHandle(cx, obj, filename, file);
    if(script)
    {
        JS_ExecuteScript(cx, obj, script, &result);
    }
}

static void
printerror(JSContext *cx, const char *mesg, JSErrorReport *report)
{
    if(!report || !JSREPORT_IS_WARNING(report->flags))
    {
        fprintf(stderr, "%s\n", mesg);
    }
}

static JSFunctionSpec global_functions[] = {
    JS_FS("evalcx", evalcx, 0, 0),
    JS_FS("gc", gc, 0, 0),
    JS_FS("print", print, 0, 0),
    JS_FS("quit", quit, 0, 0),
    JS_FS("readline", readline, 0, 0),
    JS_FS("seal", seal, 0, 0),
    JS_FS_END
};

int
usage()
{
    fprintf(stderr, "usage: couchjs [-H] [script_name]\n");
    return 1;
}

int
main(int argc, const char * argv[])
{
    JSRuntime* rt = NULL;
    JSContext* cx = NULL;
    JSObject* global = NULL;
    JSFunctionSpec* sp = NULL;
    char* script_name = NULL;

    if(argc > 3)
    {
        fprintf(stderr, "ERROR: Too many arguments.\n");
        return usage();
    }
    else if (argc == 3)
    {
        if(strcmp(argv[1], "-H"))
        {
            fprintf(stderr, "ERROR: Invalid option: %s\n", argv[1]);
            return usage();
        }
        return usage();
    }
    else if (argc == 2)
    {
        if (strcmp(argv[1], "-H") == 0)
        {
            return usage();
        }
        script_name = argv[1];
    }
    
    
    rt = JS_NewRuntime(64L * 1024L * 1024L);
    if (!rt) return 1;

    cx = JS_NewContext(rt, 8L * 1024L);
    if (!cx) return 1;

    JS_SetErrorReporter(cx, printerror);
    JS_ToggleOptions(cx, JSOPTION_XML);
    
    SETUP_REQUEST(cx);
    global = JS_NewCompartmentAndGlobalObject(cx, &global_class, NULL);
    if (!global) return 1;
    JSCrossCompartmentCall *call = JS_EnterCrossCompartmentCall(cx, global);

    if (!JS_InitStandardClasses(cx, global)) return 1;


    if (!JS_DefineFunctions(cx, global, global_functions))
       return JS_FALSE;
    
    for(sp = global_functions; sp->name != NULL; sp++)
    {
        if(!JS_DefineFunction(cx, global,
               sp->name, sp->call, sp->nargs, sp->flags))
        {
            fprintf(stderr, "Failed to create function: %s\n", sp->name);
            return 1;
        }
    }

    execute_script(cx, global, script_name);

    JS_LeaveCrossCompartmentCall(call);
    /* Don't use FINISH_REQUEST before destroying a context
     * Destroying a context without a thread asserts on threadsafe
     * debug builds */
    FINISH_REQUEST_AND_DESTROY(cx);

    JS_DestroyRuntime(rt);
    JS_ShutDown();

    return gExitCode;
}
