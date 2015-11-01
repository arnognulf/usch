#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <fnmatch.h>

#include "crepl_types.h"
#include "crepl_debug.h"
#include <clang-c/Index.h>
#include "bufstr.h"
#include "strutils.h"
#include "usch.h"

static int validate_symbol(struct crepl *p_crepl, const char* p_sym);

char **crepl_getldpath()
{
    ustash stash = {0};
    int i = 0;
    int j = 0;
    int ld_idx = 0;
    int status = 0;
    int num_paths = 0;
    int count = 0;
    char **pp_ldpath = NULL;
    char *p_ldscript = NULL;
    char *p_ldpaths = NULL;
    
    p_ldscript = ustrout(&stash, "ld", "--verbose");
    FAIL_IF(p_ldscript[0] == '\0');

    while (p_ldscript[i] != '\0')
    {
        if (strncmp("SEARCH_DIR(\"", &p_ldscript[i], strlen("SEARCH_DIR(\"")) == 0)
        {

            i+=strlen("SEARCH_DIR(\"");
            if (p_ldscript[i] == '=')
            {
                i++;
            }
            while(p_ldscript[i] != '"')
            {
                count++;
                i++;
            }
            // leave space for NUL 
            count++;

            num_paths++;
        }
        i++;
    }
    pp_ldpath = calloc((num_paths + 1)*sizeof(char*) + count * sizeof(char), 1);
    FAIL_IF(pp_ldpath == NULL);
    
    p_ldpaths = (char*)&pp_ldpath[num_paths+1];

    i = 0;
    while (p_ldscript[i] != '\0')
    {
        if (strncmp("SEARCH_DIR(\"", &p_ldscript[i], strlen("SEARCH_DIR(\"")) == 0)
        {

            i+=strlen("SEARCH_DIR(\"");
            if (p_ldscript[i] == '=')
            {
                i++;
            }
            pp_ldpath[ld_idx] = &p_ldpaths[j];
            while(p_ldscript[i] != '"')
            {
                p_ldpaths[j] = p_ldscript[i];
                j++;
                i++;
            }
            p_ldpaths[j] = '\0';
            j++;
            ld_idx++;
        }
        i++;
    }
end:
    uclear(&stash);
    if (status != 0)
    {
        free(pp_ldpath);
    }

    return pp_ldpath;
}

static enum CXChildVisitResult visitorImpl(
        CXCursor cursor, 
        CXCursor parent, 
        CXClientData p_client_data)
{
    (void)parent;
    crepl_dyfn_t *p_dyfns = NULL;
    bufstr_t bufstr = {NULL, 0};
    int status = 0;
    char *p_fnstr = NULL;
    CXString cxretkindstr = {NULL, 0};
    enum CXChildVisitResult res = CXChildVisit_Recurse;
    CXString cxstr;
    CXString cxid = {NULL, 0}; 
    crepl *p_crepl = NULL;

    cxstr = clang_getCursorSpelling(cursor);
    p_crepl = (crepl*)p_client_data;
    FAIL_IF(p_crepl == NULL);
    p_dyfns = p_crepl->p_dyfns;
    (void)p_dyfns;
    switch (cursor.kind) 
    {
        case CXCursor_FunctionDecl:
            {
                int num_args = -1;
                int i;
                CXType return_type;

                ENDOK_IF(strncmp(clang_getCString(cxstr), "__builtin_", strlen("__builtin_")) == 0);
                QUIET_FAIL_IF(validate_symbol(p_crepl, clang_getCString(cxstr)) == 1);
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
                    CXString cxkindstr;
                    CXString cxargstr;
                    CXCursor argCursor;
                    CXType argType;

                    argCursor = clang_Cursor_getArgument(cursor, i);
                    argType = clang_getCursorType(argCursor);
                    cxkindstr = clang_getTypeSpelling(argType);
                    cxargstr = clang_getCursorSpelling(argCursor);
                    bufstradd(&bufstr, clang_getCString(cxkindstr)); 
                    clang_disposeString(cxkindstr);
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
                    CXString cxkindstr;
                    CXString cxargstr;
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
                    clang_disposeString(cxkindstr);
                    clang_disposeString(cxargstr);
                }
                bufstradd(&bufstr, ");\n");
                bufstradd(&bufstr, "\thandle = crepl_getdyfnhandle(p_crepl_context, \"");
                bufstradd(&bufstr, clang_getCString(cxid));
                bufstradd(&bufstr, "\");\n");
                bufstradd(&bufstr, "\treturn handle(");
                for (i = 0; i < num_args; i++)
                {
                    CXString cxargstr;
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
                free(bufstr.p_str);
                bufstr.p_str = NULL;

                break;
            }
        default:
            {
                res = CXChildVisit_Continue;
                break;
            }
    }
    free(p_fnstr);
    p_fnstr = NULL;
end:
    if (status != 0)
    {
        res = CXChildVisit_Break;
    }
    clang_disposeString(cxstr);
    clang_disposeString(cxid);
    clang_disposeString(cxretkindstr);
    (void)status;
    free(bufstr.p_str);
    free(p_fnstr);
    return res;
}

static int validate_symbol(struct crepl *p_crepl, const char* p_sym) 
{
    int status = 0;
    int symbol_found = 0;
    void *p_handle = NULL;
    char *p_error = NULL;
    int (*dyn_func)();
    crepl_lib_t *p_lib = NULL;

    p_lib = p_crepl->p_libs;
    
    if (p_lib == NULL)
        fprintf(stderr, "usch: error: cannot resolve header symbols without at least one library added.\n");
    QUIET_FAIL_IF(p_lib == NULL);

    // check if intrinsic
    ENDOK_IF(strlen(p_sym) > 1 &&
             p_sym[0] == '_' &&
             p_sym[1] == '_');
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
    if (symbol_found == 1)
        fprintf(stderr, "usch: error: could not resolve: %s, did you forget to add the corresponding library?\n", p_sym);
    if (status != 0)
        symbol_found = 1;
    return symbol_found;
}
static char *get_fullname(struct crepl *p_crepl, char *p_libname_in)
{
    bufstr_t namecand;
    struct stat sb;
    char **pp_ldpath = NULL;
    int i = 0;
    int status = 0;
    char *p_foundlib = NULL;
    char apistr[(sizeof(int)*8+1)] = {0};

    namecand.len = 256;
    namecand.p_str = calloc(namecand.len, 1);

    pp_ldpath = p_crepl->pp_ldpath;
    FAIL_IF(pp_ldpath == NULL);

    if (stat(p_libname_in, &sb) == 0)
    {
        p_foundlib = strdup(p_libname_in);
        ENDOK_IF(1);
    }


    for (i = 0; pp_ldpath[i] != NULL; i++)
    {
        namecand.p_str[0] = '\0';
        bufstradd(&namecand, pp_ldpath[i]);
        bufstradd(&namecand, "/");
        bufstradd(&namecand, p_libname_in);
        if (stat(namecand.p_str, &sb) == 0)
        {
            p_foundlib = namecand.p_str;
            namecand.p_str = NULL;
            ENDOK_IF(1);
        }
        namecand.p_str[0] = '\0';
        bufstradd(&namecand, pp_ldpath[i]);
        bufstradd(&namecand, "/lib");
        bufstradd(&namecand, p_libname_in);

        bufstradd(&namecand, ".so");
        if (stat(namecand.p_str, &sb) == 0)
        {
            p_foundlib = namecand.p_str;
            namecand.p_str = NULL;
            ENDOK_IF(1);
        }
        namecand.p_str[0] = '\0';
        bufstradd(&namecand, pp_ldpath[i]);
        bufstradd(&namecand, "/");
        bufstradd(&namecand, "lib");
        bufstradd(&namecand, p_libname_in);
        bufstradd(&namecand, ".so.*");
        int apiver;
        // TODO: there is probably a better way to do this
        // also the hardcoded limit at 99 may be exceeded by some crazy library, at least 22 is out in the wild.
        for (apiver = 99; apiver >= 0; apiver--)
        {
            namecand.p_str[0] = '\0';
            bufstradd(&namecand, pp_ldpath[i]);
            bufstradd(&namecand, "/");
            bufstradd(&namecand, "lib");
            bufstradd(&namecand, p_libname_in);
            bufstradd(&namecand, ".so.");
            sprintf(apistr, "%d", apiver);
            bufstradd(&namecand, apistr);
            apistr[0] = '\0';

            if (stat(namecand.p_str, &sb) == 0)
            {
                p_foundlib = namecand.p_str;
                namecand.p_str = NULL;
                ENDOK_IF(1);
            }
            namecand.p_str[0] = '\0';
        }
    }
    if (p_foundlib == NULL)
    {
        fprintf(stderr, "usch: error: could not find library\n");
    }

end:
    (void)status;
    free(namecand.p_str);
    return p_foundlib;

}

int crepl_lib(struct crepl *p_crepl, char *p_libname_in)
{
    int status = 0;
    crepl_lib_t *p_lib = NULL;
    crepl_lib_t *p_current_lib = NULL;
    void *p_handle = NULL;
    char *p_libname = NULL;

    FAIL_IF(p_crepl == NULL || p_libname_in == NULL);
   
    p_libname = get_fullname(p_crepl, p_libname_in);
    QUIET_FAIL_IF(p_libname == NULL);

    p_handle = dlopen(p_libname, RTLD_LAZY);
    FAIL_IF(p_handle == NULL);

    p_lib = calloc(sizeof(crepl_lib_t) + strlen(p_libname) + 1, 1);
    FAIL_IF(p_lib == NULL);

    p_lib->p_handle = p_handle;
    strcpy(p_lib->libname, p_libname);

    p_current_lib = p_crepl->p_libs;

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
        p_crepl->p_libs = p_lib;
    }
    p_lib = NULL;
end:
    free(p_libname);
    free(p_lib);
    return status;
}

static int loadsyms_from_header_ok(crepl *p_crepl, char *p_includefile)
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
            visitorImpl,
            (void*)p_crepl);
    QUIET_FAIL_IF(visitorstatus != 0);
end:
    clang_disposeTranslationUnit(p_tu);
    clang_disposeIndex(p_idx);
    return status;
}

static int compile_header_ok(crepl *p_crepl, char *p_includefile)
{
    (void)p_crepl;
    int status = 0;
    ustash s = {0};
    char *p_tmp_s = ustrjoin(&s, p_crepl->p_tmpdir, "/tmp.s");

    QUIET_FAIL_IF(ucmd("cc", "-S", "-pipe", p_includefile, "-o", p_tmp_s) != 0);
    ucmd("rm", "-f", p_tmp_s);
end:
    uclear(&s);
    return status;
}


int crepl_include(struct crepl *p_crepl, char *p_header)
{
    int status = 0;
    char *p_tmpheader = NULL;
    char tmp_c[] = "tmp.c";
    FILE *p_includefile = NULL;
    char *p_tmpdir = NULL;
    crepl_inc_t *p_inc = NULL;
    crepl_inc_t *p_incs = NULL;

    FAIL_IF(p_crepl == NULL || p_header == NULL);

    p_incs = p_crepl->p_incs;
    p_tmpdir = p_crepl->p_tmpdir;
    p_tmpheader = calloc(strlen(p_tmpdir) + 1 + strlen(tmp_c) + 1, 1);
    HASH_FIND_STR(p_incs, p_header, p_inc);
    FAIL_IF(p_inc != NULL);

    strcpy(p_tmpheader, p_tmpdir);
    p_tmpheader[strlen(p_tmpheader)] = '/';
    strcpy(&p_tmpheader[strlen(p_tmpheader)], tmp_c);

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
    QUIET_FAIL_IF(compile_header_ok(p_crepl, p_tmpheader) != 0);
    QUIET_FAIL_IF(loadsyms_from_header_ok(p_crepl, p_tmpheader) != 0);

    p_inc = calloc(strlen(p_header) + 1 + sizeof(crepl_inc_t), 1);
    FAIL_IF(p_inc == NULL);
    strcpy(p_inc->incname, p_header);
    HASH_ADD_STR(p_incs, incname, p_inc);
    p_inc = NULL;
end:
    free(p_inc);
    free(p_tmpheader);
    if (p_includefile)
        fclose(p_includefile);
    return status;
}
void* crepl_getdyfnhandle(crepl *p_crepl, const char *p_id)
{
    int status = 0;
    crepl_dyfn_t *p_dyfns = NULL;
    crepl_dyfn_t *p_dyfn = NULL;
    void *p_handle = NULL;

    if (p_crepl == NULL || p_id == NULL)
        return NULL;

    FAIL_IF(p_crepl->p_dyfns == NULL);
    p_dyfns = p_crepl->p_dyfns;

    HASH_FIND_STR(p_dyfns, p_id, p_dyfn);
    FAIL_IF(p_dyfn == NULL);

    p_handle = p_dyfn->p_handle;
end:
    (void)status;
    return p_handle;
}




