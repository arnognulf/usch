#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "uschshell_parser.h"
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <ctype.h>
#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "usch_debug.h"
#include "bufstr.h"
#include "uschshell_types.h"
#include "clang-c/Index.h"
#include "strutils.h"
#include "uschshell.h"

static size_t count_stars(char *p_input)
{
    size_t i = 0;
    size_t len = strlen(p_input);
    size_t num_stars = 0;

    while (i < len)
    {
        if (p_input[i] == '*')
        {
            num_stars++;
            i++;
        }
        i+=count_spaces(&p_input[i]);
        i++;
    }
    return num_stars;
}

static size_t count_identifier(char *p_input)
{
    size_t i = 0;
    if (isdigit(p_input[i]))
        return 0;
    while (p_input[i] != '\0' && isalnum(p_input[i]))
        i++;
    return i;
}



int count_spaces(char *p_input)
{
    size_t i = 0;
    while (p_input[i] != '\0' && (p_input[i] == ' '  || p_input[i] == '\t'))
    {
        i++;
    }
    return i;
}
int has_trailing_open_parenthesis(char *p_line)
{
    int len = (int)strlen(p_line);
    int i = len - 1;
    int unclosed = 0;

    while (p_line[i] != ')' && i >= 0)
    {
        if (p_line[i] == '(')
        {
            unclosed = 1;
            break;
        }
        i--;
    }
    return unclosed;
}
int has_trailing_closed_parenthesis(char *p_line)
{
    int len = (int)strlen(p_line);
    int i = len - 1;
    int closed = 0;

    while (p_line[i] != ')' && i >= 0)
    {
        i--;
    }
    while (i >= 0)
    {
        if (p_line[i] == '(')
        {
            closed = 1;
            break;
        }
        i--;
    }
    return closed;
}

int is_definition(char *p_input)
{
    size_t i = 0;
    size_t id1 = 0;
    size_t stars = 0;
    size_t id2 = 0;

    i += count_spaces(p_input); 

    id1 = count_identifier(&p_input[i]);
    i += id1;
    i += count_spaces(&p_input[i]); 
    stars = count_stars(&p_input[i]);
    i += stars;
    i += count_spaces(&p_input[i]); 
    id2 = count_identifier(&p_input[i]);

    if (id1 > 0 && id2 > 0)
        return 1;

    return 0;
}

int get_identifiers(char *p_line_in, int *p_count, char ***ppp_identifiers_out)
{
    int status = 0;
    char **pp_identifiers = NULL;
    int num_identifiers = 0;
    int i = 0;
    int len = (int)strlen(p_line_in);
    char prevchar = '\0';
    int parsed_identifier = 0;
    int has_looped = 0;

    char *p_line = p_line_in;

    while (pp_identifiers == NULL)
    {
        if (has_looped)
        {
            if (num_identifiers == 0)
            {
                break;
            }
            pp_identifiers = calloc((num_identifiers + 1)* sizeof(char*) + len * sizeof(char) + 1, 1);
            FAIL_IF(pp_identifiers == NULL);
            strcpy((char*)&pp_identifiers[num_identifiers+1], p_line_in);
            // simplify array iteration by null termination
            pp_identifiers[num_identifiers] = NULL;
            p_line = (char*)&pp_identifiers[num_identifiers+1];
            i = 0;
            prevchar = '\0';
            num_identifiers = 0;
        }
        while (i < len)
        {
            if (i > 0)
                prevchar = p_line[i-1];

            if (isalpha(p_line[i]) && !isdigit(prevchar))
            {
                if (pp_identifiers != NULL)
                {
                    pp_identifiers[num_identifiers] = &p_line[i];
                }
                num_identifiers++;
                i++;
                while (isalnum(p_line[i])|| p_line[i] == '_')
                {
                    i++;
                }
                parsed_identifier = 1;
            }
            else if (isdigit(p_line[i]))
            {
                while (isalnum(p_line[i]) || p_line[i] == '_')
                {
                    i++;
                }
                parsed_identifier = 0;
            }
            else if (p_line[i] == '-' && p_line[i+1] == '>' && parsed_identifier)
            {
                i+=2;
                while(isalnum(p_line[i]))
                {
                    i++;
                }
                parsed_identifier = 0;
            }
            else if (p_line[i] == '.' && parsed_identifier)
            {
                i+=2;
                while(isalnum(p_line[i]))
                {
                    i++;
                }
                parsed_identifier = 0;
            }
            else if (p_line[i] == '\'')
            {
                i++;
                while (p_line[i] != '\'' && p_line[i] != '\0')
                {
                    i++;
                }
            }
            else if (p_line[i] == '\"')
            {
                i++;
                while (p_line[i] != '\"' && p_line[i] != '\0')
                {
                    i++;
                }
            }
            else
            {
                i++;
            }
        }

        has_looped = 1;
    }
    for (i = 0; i < len; i++)
    {
        if (!isalnum(p_line[i]) && p_line[i] != '_')
            p_line[i] = '\0';
    }
    *ppp_identifiers_out = pp_identifiers;
    pp_identifiers = NULL;
    *p_count = num_identifiers;
end:
    free(pp_identifiers);
    return status;
}
int identifier_pos(char *p_line)
{
    char prevchar = '\0';
    int i = 0;
    size_t len = 0;
    len = strlen(p_line);
    if (len == 0)
        return 0;
    i = (int)len - 1;

    do
    {
        if(!isalnum(p_line[i]))
        {
            break;
        }
        prevchar = p_line[i];
        i--;
    } while(i != -1);

    if (isalpha(prevchar))
    {
        return (int)i + 1;
    }
    else
    {
        return len;
    }
}

char* stripwhite(char *string)
{
   char *p_s, *p_t;

   for (p_s = string; isspace(*p_s); p_s++)
   {
      *p_s = '\0';
   }

   if (*p_s == 0)
      return p_s;

   p_t = p_s + strlen(p_s) - 1;
   while (p_t > p_s && isspace(*p_t))
   {
       *p_t = '\0';
       p_t--;
   }

   return p_s;
}

 
static int is_system_cmd(char *p_cmd)
{
    struct stat sb;
    char *p_item = NULL;
    char *p_tmp = NULL;
    char path[PATH_MAX] = {0};
    char *std = NULL;
    std = getenv("PATH");
    int found = 0;

    for (p_item = std;; *p_item++ = ':')
    {
        p_tmp = p_item;
        if ((p_item = strchr(p_item, ':')) != NULL) {
            *p_item = '\0';
            if (p_tmp == p_item)
            {
                p_tmp = ".";
            }
        }
        else
            if (strlen(p_tmp) == 0)
            {
                p_tmp = ".";
            }

        (void)snprintf(path, sizeof(path), "%s/%s", p_tmp, p_cmd);
        if (!stat(path, &sb))
        {
            found = 1;
            break;
        }
        if (p_item == NULL)
        {
            break;
        }
    }
    return found;
}


int parse_line(char *p_input, usch_def_t *p_definition)
{
    //usch_def_t definition;
    int status = 1;
    size_t i = 0;
    size_t length;
    char *p_symname = NULL;

    length = strlen(p_input);
    p_symname = calloc(length, 1);

    FAIL_IF(p_symname == NULL);

    while (p_input[i] == ' ' || p_input[i] == '\t')
        i++;



    for (; i < length; i++)
    {
        char c = p_input[i];
        switch (p_input[i])
        {
            case '\t':
            case ' ':
                {
                    break;
                }
            case '(':
                {

                    break;
                }
            default:
                {
                    if (isalnum(c) != c)
                    {
                        p_symname[i] = c;
                    }
                    else
                    {
                        assert(0);
                    }
                    break;
                }
        }
    }
    p_definition->p_symname = p_symname;
end:
    return status;
}

int has_trailing_identifier(char *p_line, char **pp_identifiers)
{
    int i;
    int fetch_id;
    int trailing_identifier;
    size_t len = strlen(p_line);
    size_t identifier_len;

    for (i = 0; pp_identifiers[i] != NULL; i++)
        ;
    fetch_id = i - 1;
    identifier_len = strlen(pp_identifiers[fetch_id]);
    if (strcmp(pp_identifiers[fetch_id], &p_line[len - identifier_len]) == 0)
    {
        trailing_identifier = 1;
    }
    else
    {
        trailing_identifier = 0;
    }

    return trailing_identifier;
}

//static 
enum CXChildVisitResult clang_preparseVisitor(
        CXCursor cursor, 
        CXCursor parent, 
        CXClientData p_client_data)
{
    (void)parent;
    bufstr_t bufstr = {NULL, 0};
    int status = 0;
    char *p_fnstr = NULL;
    uschshell_dyfn_t *p_dyfn = NULL;
    enum CXChildVisitResult res = CXChildVisit_Recurse;
    CXString cxstr = {NULL, 0};
    preparse_userdata_t *p_userdata = NULL;

    cxstr = clang_getCursorSpelling(cursor);
    p_userdata = (preparse_userdata_t*)p_client_data;
    FAIL_IF(p_userdata == NULL);
    switch (cursor.kind) 
    {
        case CXCursor_FunctionDecl:
            {
                if ((strncmp(clang_getCString(cxstr), p_userdata->p_cur_id, strlen(p_userdata->p_cur_id)) == 0) && strlen(clang_getCString(cxstr)) == strlen(p_userdata->p_cur_id))
                {
                    //printf("found_cur_id = 1\n");
                    p_userdata->found_cur_id = 1;
                }
                res = CXChildVisit_Recurse;

                break;
            }
        default:
            {
                //printf("\nfound::: %s\n",clang_getCString(cxstr));
                res = CXChildVisit_Recurse;
                break;
            }
    }
    free(p_fnstr);
    p_fnstr = NULL;
end:
    clang_disposeString(cxstr);
    (void)status;
    free(bufstr.p_str);
    free(p_fnstr);
    free(p_dyfn);
    return res;
}

//static
int ends_with_identifier(char *p_line)
{
    size_t i = 0;
    size_t len = 0;
    int has_identifier = 0;
    len = strlen(p_line);
    if (len == 0)
        return 0;
    i = len - 1;
    do 
    {
        if (isdigit(p_line[i]) == 0)
        {
            if (isalpha(p_line[i]) == 0)
            {
                break;
            }
            else
            {
                has_identifier = 1;
                break;
            }
        }
        i--;
    } while(i != 0);
    return has_identifier;
}

//static
void set_preparsefile_content(bufstr_t *p_bufstr, char* p_line, char **pp_identifiers)
{
    int i;
    p_bufstr->p_str[0] = '\0';
    bufstradd(p_bufstr, "struct uschshell_t;\n");
    bufstradd(p_bufstr, "#include <usch.h>\n");
    bufstradd(p_bufstr, "#include \"includes.h\"\n");
    bufstradd(p_bufstr, "#include \"definitions.h\"\n");
    bufstradd(p_bufstr, "#include \"trampolines.h\"\n");

    for (i = 0; pp_identifiers[i] != NULL; i++)
    {
        // clang doesn't parse preprocessor #defines
        // define a function instead
        bufstradd(p_bufstr, "void ");
        bufstradd(p_bufstr, pp_identifiers[i]);
        bufstradd(p_bufstr, "(...) {};\n");
    }
    bufstradd(p_bufstr, "int ");
    bufstradd(p_bufstr, USCHSHELL_DYN_FUNCNAME);
    bufstradd(p_bufstr, "()\n");
    bufstradd(p_bufstr, "{\n");
    bufstradd(p_bufstr, "\t");
    bufstradd(p_bufstr, p_line);
    if (has_trailing_identifier(p_line, pp_identifiers))
    {
        bufstradd(p_bufstr, "()");
    }
    else if (has_trailing_open_parenthesis(p_line))
    {
        bufstradd(p_bufstr, ")");
    }
    bufstradd(p_bufstr, ";\n\treturn 0;\n");
    bufstradd(p_bufstr, "}\n");
}

// static
int resolve_identifier(char *p_parsefile_fullname,
                 bufstr_t *p_filecontent,
                 char *p_line,
                 preparse_userdata_t *p_userdata,
                 char **pp_definitions)
{
    int status = 0;
    CXTranslationUnit p_tu = NULL;
    CXIndex p_idx = NULL;
    unsigned int visitorstatus = 0;

    FILE *p_parsefile = NULL;
    if (pp_definitions != NULL)
    {
        set_preparsefile_content(p_filecontent, p_line, pp_definitions);
    }

    p_parsefile = fopen(p_parsefile_fullname, "w");
    FAIL_IF(p_parsefile == NULL);

    FAIL_IF(!fwrite_ok(p_filecontent->p_str, p_parsefile));
    fclose(p_parsefile);
    p_parsefile = NULL;
    p_idx = clang_createIndex(0, 0);
    FAIL_IF(p_idx == NULL);
    p_tu = clang_parseTranslationUnit(p_idx, p_parsefile_fullname, NULL, 0, NULL, 0, 0);
    FAIL_IF(p_tu == NULL);

    visitorstatus = clang_visitChildren(
            clang_getTranslationUnitCursor(p_tu),
            clang_preparseVisitor,
            (void*)p_userdata);
    FAIL_IF(visitorstatus != 0);
    clang_disposeTranslationUnit(p_tu);
    clang_disposeIndex(p_idx);

    p_tu = NULL;
    p_idx = NULL;

end:
    if (p_parsefile)
        fclose(p_parsefile);
    return status;
}
static int is_builtin_cmd(char *p_str)
{
    int is_builtin = 0;
    if (strncmp(p_str, "lib", strlen(p_str)) == 0)
    {
        is_builtin = 1;
    }
    if (strncmp(p_str, "define", strlen(p_str)) == 0)
    {
        is_builtin = 1;
    }
    return is_builtin;
}
int uschshell_preparse(struct uschshell_t *p_context, char *p_input, uschshell_state_t *p_state, char ***ppp_cmds)
{
    int i;
    preparse_userdata_t userdata = {0};
    uschshell_state_t state;
    int status = 0;
    bufstr_t filecontent = {0};
    char preparse_filename[] = "preparse.c";
    char *p_parsefile_fullname = NULL;
    char *p_line_copy = NULL;
    char *p_line = NULL;
    int num_identifiers = 0;
    char **pp_identifiers = NULL;
    char **pp_cmds = NULL;
    char *p_cmds = NULL;
    int cmdidx = 0;

    p_line_copy = strdup(p_input);
    FAIL_IF(p_line_copy == NULL);
    p_line = stripwhite(p_line_copy);
    
    status = get_identifiers(p_line, &num_identifiers, &pp_identifiers);
    FAIL_IF(status != 0 || num_identifiers == 0);

    pp_cmds = calloc((num_identifiers + 1)* sizeof(char*) + (strlen(p_line) + 1) * sizeof(char), 1);
    FAIL_IF(pp_cmds == NULL);
    memcpy((char*)&pp_cmds[num_identifiers + 1], p_line, strlen(p_line));
    // NULL terminate vector before it's content
    pp_cmds[num_identifiers + 1] = NULL;

    filecontent.p_str = calloc(1, 1024);
    FAIL_IF(filecontent.p_str == NULL);
    filecontent.len = 1024;

    p_parsefile_fullname = calloc(strlen(p_context->tmpdir) + 1 + strlen(preparse_filename), 1);
    FAIL_IF(p_parsefile_fullname == NULL);
    strcpy(p_parsefile_fullname, p_context->tmpdir);
    p_parsefile_fullname[strlen(p_context->tmpdir)] = '/';
    strcpy(&p_parsefile_fullname[strlen(p_context->tmpdir) + 1], preparse_filename);
    
    for (i = 0; pp_identifiers[i] != NULL; i++)
    {
        state = USCHSHELL_STATE_CPARSER;

        userdata.p_cur_id = pp_identifiers[i];
        userdata.found_cur_id = 0;

        FAIL_IF(resolve_identifier(p_parsefile_fullname, &filecontent, p_line, &userdata, NULL));

        // identifier is not defined
        if (userdata.found_cur_id == 0)
        {
            if (is_system_cmd(userdata.p_cur_id) || is_builtin_cmd(userdata.p_cur_id))
            {
                // the identifier is available as a system command
                // try to define the identifier as a function

                userdata.found_cur_id = 0;
                FAIL_IF(resolve_identifier(p_parsefile_fullname, &filecontent, p_line, &userdata, pp_identifiers));

                // unknown error
                if (userdata.found_cur_id == 0)
                {
                        state = USCHSHELL_STATE_ERROR;
                        break;
                }
                // TODO: if prevchar identifer is a system command, we probably are in some parameter unless...
                // 
                // ... there is a nested command (ARGH!)
                else if (pp_identifiers[i+1] == NULL)
                {
                    if (pp_cmds[0] == NULL)
                    {
                        p_cmds = (char*)&pp_cmds[num_identifiers + 1];
                    }
                    strcpy(p_cmds, pp_identifiers[i]);
                    pp_cmds[cmdidx] = p_cmds;
                    p_cmds += strlen(pp_identifiers[i]) + 1;
                    cmdidx++;

                    if (has_trailing_closed_parenthesis(p_line))
                    {
                        state = USCHSHELL_STATE_CPARSER;
                    }
                    else if (has_trailing_open_parenthesis(p_line))
                    {
                        state = USCHSHELL_STATE_CMDARG;
                    } 
                    else
                    {
                        state = USCHSHELL_STATE_CMDSTART;
                    }
                    break;
                }
            }
            else
            {
                // the identifier could not be resolved, nor is it a system command
                state = USCHSHELL_STATE_ERROR;
                break;
            }
        }
        state = USCHSHELL_STATE_CPARSER;
    }
    *p_state = state;
    *ppp_cmds = pp_cmds;
    pp_cmds = NULL;
end:
    p_cmds = NULL;
    free(pp_identifiers);
    p_line = NULL;
    free(p_parsefile_fullname);
    free(p_line_copy);
    free(pp_cmds);

    free(filecontent.p_str);
    return status;
}

