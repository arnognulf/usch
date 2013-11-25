#include <dlfcn.h>
#include <stdio.h>
#include "uschshell.h"
#include "uschshell_types.h"
#include "usch_debug.h"
#include "clang-c/Index.h"
#include "bufstr.h"
#include "strutils.h"

static int has_symbol(struct uschshell_t *p_context, const char* p_sym);

static enum CXChildVisitResult clang_visitor(
        CXCursor cursor, 
        CXCursor parent, 
        CXClientData p_client_data)
{
    (void)parent;
    uschshell_dyfn_t *p_dyfns = NULL;
    bufstr_t bufstr = {NULL, 0};
    int status = 0;
    char *p_fnstr = NULL;
    uschshell_dyfn_t *p_dyfn = NULL;
    CXString cxretkindstr = {NULL, 0};
    enum CXChildVisitResult res = CXChildVisit_Recurse;
    CXString cxstr = {NULL, 0};
    CXString cxid = {NULL, 0}; 
    uschshell_t *p_context = NULL;

    cxstr = clang_getCursorSpelling(cursor);
    p_context = (uschshell_t*)p_client_data;
    FAIL_IF(p_context == NULL);
    p_dyfns = p_context->p_dyfns;
    (void)p_dyfns;
    switch (cursor.kind) 
    {
        case CXCursor_FunctionDecl:
            {
                int num_args = -1;
                int i;
                CXType return_type;

                cxstr = clang_getCursorSpelling(cursor);
                ENDOK_IF(strncmp(clang_getCString(cxstr), "__builtin_", strlen("__builtin_")) == 0);
                ENDOK_IF(!has_symbol(p_context, clang_getCString(cxstr)));
                bufstr.p_str = calloc(1024, 1);
                bufstr.len = 1024;
                FAIL_IF(bufstr.p_str == NULL);
                cxid = clang_getCursorDisplayName(cursor);

                return_type = clang_getCursorResultType(cursor);
                cxretkindstr = clang_getTypeSpelling(return_type);

                bufstradd(&bufstr, clang_getCString(cxretkindstr));
                bufstradd(&bufstr, " ");
                bufstradd(&bufstr, clang_getCString(cxstr));

                bufstradd(&bufstr, "(");
                num_args = clang_Cursor_getNumArguments(cursor);
                for (i = 0; i < num_args; i++)
                {
                    CXString cxkindstr = {NULL, 0};
                    CXString cxargstr = {NULL, 0};
                    CXCursor argCursor;
                    CXType argType;

                    argCursor = clang_Cursor_getArgument(cursor, i);
                    argType = clang_getCursorType(argCursor);
                    cxkindstr = clang_getTypeSpelling(argType);
                    cxargstr = clang_getCursorSpelling(argCursor);
                    bufstradd(&bufstr, clang_getCString(cxkindstr)); 
                    bufstradd(&bufstr, " "); 
                    bufstradd(&bufstr, clang_getCString(cxargstr)); 
                    if (i != (num_args - 1))
                    {
                        bufstradd(&bufstr, ", "); 
                    }
                    clang_disposeString(cxargstr);
                }
                bufstradd(&bufstr, ")\n{\n");

                bufstradd(&bufstr, "\t");
                bufstradd(&bufstr, clang_getCString(cxretkindstr));
                bufstradd(&bufstr, " (*handle)(");
                for (i = 0; i < num_args; i++)
                {
                    CXString cxkindstr = {NULL, 0};
                    CXString cxargstr = {NULL, 0};
                    CXCursor argCursor;
                    CXType argType;

                    argCursor = clang_Cursor_getArgument(cursor, i);
                    argType = clang_getCursorType(argCursor);
                    cxkindstr = clang_getTypeSpelling(argType);
                    cxargstr = clang_getCursorSpelling(argCursor);
                    bufstradd(&bufstr, clang_getCString(cxkindstr)); 

                    if (i != (num_args - 1))
                    {
                        bufstradd(&bufstr, ", "); 
                    }
                    clang_disposeString(cxargstr);
                }
                bufstradd(&bufstr, ");\n");
                bufstradd(&bufstr, "\thandle = uschshell_getdyfnhandle(p_uschshell_context, \"");
                bufstradd(&bufstr, clang_getCString(cxid));
                bufstradd(&bufstr, "\");\n");
                bufstradd(&bufstr, "\treturn handle(");
                for (i = 0; i < num_args; i++)
                {
                    CXString cxargstr = {NULL, 0};
                    CXCursor argCursor;

                    argCursor = clang_Cursor_getArgument(cursor, i);
                    cxargstr = clang_getCursorSpelling(argCursor);
                    bufstradd(&bufstr, clang_getCString(cxargstr));
                    if (i != (num_args - 1))
                    {
                        bufstradd(&bufstr, ","); 
                    }
                    clang_disposeString(cxargstr);
                }
                bufstradd(&bufstr, ");\n}\n");
                p_dyfn = calloc(strlen(clang_getCString(cxid)) + 1 + sizeof(uschshell_dyfn_t), 1);
                FAIL_IF(p_dyfn == NULL);
                strcpy(p_dyfn->dyfnname, clang_getCString(cxid));
                p_dyfn->p_dyfndef = bufstr.p_str;
                bufstr.p_str = NULL;

                break;
            }
        default:
            {
                res = CXChildVisit_Continue;
                break;
            }
    }
    p_dyfn = NULL;
    free(p_fnstr);
    p_fnstr = NULL;
end:
    clang_disposeString(cxstr);
    clang_disposeString(cxid);
    clang_disposeString(cxretkindstr);
    (void)status;
    free(bufstr.p_str);
    free(p_fnstr);
    free(p_dyfn);
    return res;
}

static int has_symbol(struct uschshell_t *p_context, const char* p_sym) 
{
    int status = 0;
    int symbol_found = 0;
    void *p_handle = NULL;
    char *p_error = NULL;
    int (*dyn_func)();
    uschshell_lib_t *p_lib = NULL;

    p_lib = p_context->p_libs;
    
    FAIL_IF(p_lib == NULL);

    do 
    {
        p_handle = p_lib->p_handle;

        FAIL_IF(p_handle == NULL);

        *(void **) (&dyn_func) = dlsym(p_handle, p_sym);
        if ((p_error = dlerror()) != NULL)
            symbol_found = 1;
        p_lib = p_lib->p_next;
    } while (p_lib != NULL);

end:
    (void)status;
    return symbol_found;
}

int uschshell_lib(struct uschshell_t *p_context, char *p_libname)
{
    int status = 0;
    uschshell_lib_t *p_lib = NULL;
    uschshell_lib_t *p_current_lib = NULL;

    FAIL_IF(p_context == NULL || p_libname == NULL);
    
    p_lib = calloc(sizeof(uschshell_lib_t) + strlen(p_libname), 1);
    FAIL_IF(p_lib == NULL);

    strcpy(p_lib->libname, p_libname);

    p_lib->p_handle = dlopen(p_libname, RTLD_LAZY);

    p_current_lib = p_context->p_libs;

    if (p_current_lib != NULL)
    {
        while (p_current_lib->p_next != NULL)
        {
            p_current_lib = p_current_lib->p_next;
        }
        p_current_lib->p_next = p_lib;
    }
    else
    {
        p_context->p_libs = p_lib;
    }
    p_lib = NULL;
end:
    free(p_lib);
    return status;
}

static int loadsyms_from_header_ok(uschshell_t *p_context, char *p_includefile)
{
    int status = 0;
    CXTranslationUnit p_tu = NULL;
    CXIndex p_idx = NULL;
    unsigned int visitorstatus = 0;

    p_idx = clang_createIndex(0, 0);
    FAIL_IF(p_idx == NULL);

    p_tu = clang_parseTranslationUnit(p_idx, p_includefile, NULL, 0, NULL, 0, 0);
    FAIL_IF(p_tu == NULL);

    visitorstatus = clang_visitChildren(
            clang_getTranslationUnitCursor(p_tu),
            clang_visitor,
            (void*)p_context);
    FAIL_IF(visitorstatus != 0);
end:
    clang_disposeTranslationUnit(p_tu);
    clang_disposeIndex(p_idx);
    return status;
}

int uschshell_include(struct uschshell_t *p_context, char *p_header)
{
    int status = 0;
    char *p_tmpheader = NULL;
    char tmp_h[] = "tmp.h";
    FILE *p_includefile = NULL;
    char *p_tmpdir = NULL;
    uschshell_inc_t *p_inc = NULL;
    uschshell_inc_t *p_incs = NULL;

    FAIL_IF(p_context == NULL || p_header == NULL);

    p_incs = p_context->p_incs;
    p_tmpdir = p_context->tmpdir;
    p_tmpheader = calloc(strlen(p_tmpdir) + 1 + strlen(tmp_h) + 1, 1);
    strcpy(p_tmpheader, p_tmpdir);
    p_tmpheader[strlen(p_tmpheader)] = '/';
    strcpy(&p_tmpheader[strlen(p_tmpheader)], tmp_h);
    printf("header: %s\n", p_tmpheader);

    p_includefile = fopen(p_tmpheader, "w");
    FAIL_IF(p_includefile == NULL);
    if (p_header[0] == '<')
    {
        FAIL_IF(!fwrite_ok("#include ", p_includefile));
        FAIL_IF(!fwrite_ok(p_header, p_includefile));
        FAIL_IF(!fwrite_ok("\n", p_includefile));
    }
    else
    {
        FAIL_IF(!fwrite_ok("#include \"", p_includefile));
        FAIL_IF(!fwrite_ok(p_header, p_includefile));
        FAIL_IF(!fwrite_ok("\"\n", p_includefile));
    }
    if (p_includefile)
        fclose(p_includefile);
    p_includefile = NULL;
    FAIL_IF(loadsyms_from_header_ok(p_context, p_tmpheader) != 0);

    p_inc = calloc(strlen(p_header) + 1 + sizeof(uschshell_inc_t), 1);
    FAIL_IF(p_inc == NULL);
    strcpy(p_inc->incname, p_header);
    HASH_ADD_STR(p_incs, incname, p_inc);
end:
    free(p_tmpheader);
    if (p_includefile)
        fclose(p_includefile);
    return status;
}
