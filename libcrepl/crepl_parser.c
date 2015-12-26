/*
 * USCH - The (permutated) tcsh successor
 * Copyright (c) 2013 Thomas Eriksson 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "crepl_parser.h"
#include <assert.h>                     // for assert
#include <ctype.h>                      // for isalnum, isalpha, isdigit, etc
#include <limits.h>                     // for PATH_MAX
#include <stddef.h>                     // for size_t
#include <stdio.h>                      // for printf, fclose, putchar, etc
#include <stdlib.h>                     // for NULL, free, calloc, getenv
#include <string.h>                     // for strlen, strcpy, strncmp, etc
#include <sys/stat.h>                   // for stat
#include "crepl.h"                      // for ::CREPL_STATE_CPARSER, etc
#include "crepl_debug.h"                // for FAIL_IF
#include "crepl_types.h"                // for preparse_userdata_t, etc
#include "strutils.h"                   // for fwrite_ok
#include "clang-c/Index.h"
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
    // TODO: handle invalid identifiers starting with nondigits!
    while (p_input[i] != '\0' && (isalnum(p_input[i]) || p_input[i] == '_'))
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

    while (i >= 0)
    {
        if (p_line[i] != ')')
        {
            if (p_line[i] == '(')
            {
                unclosed = 1;
                break;
            }
            i--;
        }
        else
        {
            break;
        }
    }
    return unclosed;
}
static int has_trailing_closed_parenthesis(char *p_line)
{
    int len = (int)strlen(p_line);
    int i = len - 1;
    int closed = 0;


    while (i >= 0)
    {
        if (p_line[i] != ')')
        {
            i--;
        }
        else
        {
            break;
        }
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
   while (p_t > p_s && (isspace(*p_t) || *p_t == '\n' || *p_t == '\r' || *p_t == '\t'))
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
    char *p_std = NULL;

    int status = 0;
    p_std = strdup(getenv("PATH"));
    FAIL_IF(p_std == NULL);
    int found = 0;

    for (p_item = p_std;; *p_item++ = ':')
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
end:
    free(p_std);
    if (status != 0)
        found = 0;
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

static int has_trailing_identifier(char *p_line, char **pp_identifiers)
{
    int i;
    int fetch_id;
    int trailing_identifier;
    size_t len = strlen(p_line);
    size_t identifier_len;
    if (pp_identifiers[0] == NULL)
        return 0;

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

static enum CXChildVisitResult clang_preparseVisitor(
        CXCursor cursor, 
        CXCursor parent, 
        CXClientData p_client_data)
{
    (void)parent;
    int status = 0;
    char *p_fnstr = NULL;
    crepl_dyfn_t *p_dyfn = NULL;
    enum CXChildVisitResult res = CXChildVisit_Recurse;
    CXString cxstr;
    CXString cxparstr;
    preparse_userdata_t *p_userdata = NULL;
    struct crepl *p_crepl = NULL;

    cxstr = clang_getCursorSpelling(cursor);
    cxparstr = clang_getCursorSpelling(parent);
    p_userdata = (preparse_userdata_t*)p_client_data;
    FAIL_IF(p_userdata == NULL);
    FAIL_IF(p_userdata->p_context == NULL);
    p_crepl = p_userdata->p_context ;

    switch (cursor.kind) 
    {
        case CXCursor_FunctionDecl:
            {
                if (crepl_getoptions(p_crepl).verbosity >= 11)
                    fprintf(stderr, "clang_preparseVisitor: clang_getCString(cxstr): \"%s\"\n", clang_getCString(cxstr));

                if ((strncmp(clang_getCString(cxstr), p_userdata->p_cur_id, strlen(p_userdata->p_cur_id)) == 0) && strlen(clang_getCString(cxstr)) == strlen(p_userdata->p_cur_id))
                {
                    p_userdata->found_cur_id = 1;
                }
                res = CXChildVisit_Recurse;

                break;
            }
        default:
            {
                res = CXChildVisit_Recurse;
                break;
            }
    }
    free(p_fnstr);
    p_fnstr = NULL;
end:
    clang_disposeString(cxstr);
    clang_disposeString(cxparstr);
    (void)status;
    free(p_fnstr);
    free(p_dyfn);
    return res;
}

static const char* set_preparsefile_content(ustash *p_s, char* p_line, char **pp_identifiers)
{
    int i;
    const char *p_content = ustrjoin(p_s, "#ifndef CREPL_PARSER\n",
                                   "#define CREPL_PARSER\n",
                                   "#endif // CREPL_PARSER\n",
                                   "struct crepl;\n",
                                   "#include <usch.h>\n",
                                   "#include \"includes.h\"\n",
                                   "#include \"definitions.h\"\n",
                                   "#include \"trampolines.h\"\n",
				   "#ifndef cd\n",
				   "#define cd(...) ucmd(\"cd\", ##__VA_ARGS__)\n",
				   "#endif // cd\n")
;

    for (i = 0; pp_identifiers[i] != NULL; i++)
    {
        // clang doesn't parse preprocessor #defines
        // define a function instead
        p_content = ustrjoin(p_s, p_content,
                                 "void ",
                                 pp_identifiers[i],
                                 "(...) {};\n");
    }
    p_content = ustrjoin(p_s, p_content,
                             "int ",
                             CREPL_DYN_FUNCNAME,
                             "()\n",
                             "{\n",
                             "\t",
                             p_line);
    if (has_trailing_identifier(p_line, pp_identifiers))
    {
        p_content = ustrjoin(p_s, p_content, "()");
    }
    else if (has_trailing_open_parenthesis(p_line))
    {
        p_content = ustrjoin(p_s, p_content, ")");
    }
    p_content = ustrjoin(p_s, p_content,
                             ";\n\treturn 0;\n",
                             "}\n");
    return p_content;
}

static int resolve_identifier(struct crepl *p_crepl,
                 char *p_parsefile_fullname,
                 char *p_line,
                 preparse_userdata_t *p_userdata,
                 char **pp_definitions)
{
    int status = 0;
    CXTranslationUnit p_tu = NULL;
    CXIndex p_idx = NULL;
    unsigned int visitorstatus = 0;
    ustash s = {0};
    const char *p_filecontent = "";

    FILE *p_parsefile = NULL;

    if (pp_definitions != NULL)
    {
        p_filecontent = set_preparsefile_content(&s, p_line, pp_definitions);
    }

    p_parsefile = fopen(p_parsefile_fullname, "w");
    FAIL_IF(p_parsefile == NULL);
    if (crepl_getoptions(p_crepl).verbosity >= 11)
        fprintf(stderr, "resolve_identifier: p_filecontent: \"%s\"\n", p_filecontent);

    FAIL_IF(!fwrite_ok(p_filecontent, p_parsefile));
    fclose(p_parsefile);
    p_parsefile = NULL;
    p_idx = clang_createIndex(0, 0);
    FAIL_IF(p_idx == NULL);
    p_tu = clang_parseTranslationUnit(p_idx, p_parsefile_fullname, NULL, 0, NULL, 0, 0);
    FAIL_IF(p_tu == NULL);

    p_userdata->p_context = p_crepl;

    visitorstatus = clang_visitChildren(
            clang_getTranslationUnitCursor(p_tu),
            clang_preparseVisitor,
            (void*)p_userdata);
    FAIL_IF(visitorstatus != 0);
    clang_disposeTranslationUnit(p_tu);
    clang_disposeIndex(p_idx);

    p_tu = NULL;
    p_idx = NULL;

    if (crepl_getoptions(p_crepl).verbosity >= 11)
        fprintf(stderr, "resolve_identifier: p_parsefile_fullname: \"%s\" found: %d\n", p_userdata->p_cur_id, p_userdata->found_cur_id);


end:
    uclear(&s);
    if (p_parsefile)
        fclose(p_parsefile);
    return status;
}
static USCH_BOOL is_builtin_cmd(char *p_str)
{
    if (ustreq(p_str, "cd")      ||
        ustreq(p_str, "ucmd")    ||
        ustreq(p_str, "ucmdout"))
    {
        return USCH_TRUE;
    }
    else
    {
        return USCH_FALSE;
    }
}

static void clear_range(char *p_line, int start, int end)
{
    int i;

    for (i = start; i <= end; i++)
    {
        p_line[i] = ' ';
    }
}
static void store_and_clear_definition(char *p_line, char *p_defs, char **pp_defstart, int *p_num_defs, int start_in, int end)
{
    (void)p_defs;
    (void)pp_defstart;
    int start = start_in;
    int i = 0;
    int last_clear = 0;
    int def_end = 0;
    int firstid = -1;
    int identifiers = 0;

    if (start == end)
    {
        return;
    }
    if (p_line[start] == '\0')
        return;
    while (p_line[start] == '\t' || p_line[start] == ' ')
        start++;
    if (strncmp(&p_line[start], "sizeof ", strlen("sizeof ")) == 0)
        return;
    i = start;
    while (i < end)
    {
        if (p_line[i] == '\0' || p_line[i] == '=')
            break;
        while (p_line[i] == '_' || isalpha(p_line[i]))
        {
            if (firstid == -1)
            {
                firstid = i;
            }
            last_clear = i - 1;
            i++;
            while (p_line[i] == '_' || isalnum(p_line[i]))
                i++;
            identifiers++;
        }
        if (p_line[i] == ';')
            break;

        i++;
    }
    def_end = i;
    *p_num_defs = *p_num_defs + 1;
    if (*pp_defstart == NULL)
    {
        *pp_defstart = &p_defs[firstid];
    }
#if 0
    for (i = last_clear; p_line[i] == '_' || isalnum(p_line[i]); i++)
    {
        putchar(p_line[i]);
    }
#endif // 0
    if (identifiers > 1)
    {
        for (i = start; i < def_end; i++)
        {
            p_defs[i] = p_line[i];
        }
        clear_range(p_line, firstid, last_clear);
        p_defs[def_end] = ';';
        //putchar('\n');
    }
}

int crepl_parsedefs(struct crepl *p_crepl, char *p_line_c)
{
    (void)p_crepl;
    int status = 0;
    int i = 0;
    int start = 0;
    int identifiers = 0;
    char *p_defs = NULL;
    char *p_defstart = NULL;
    int num_defs = 0;
    char *p_line = NULL;

    FAIL_IF(p_crepl == NULL || p_line_c == NULL);
    free(p_crepl->p_nodef_line);
    p_crepl->p_nodef_line = NULL;
    free(p_crepl->p_defs_line);
    p_crepl->p_defs_line = NULL;

    p_line = strdup(p_line_c);
    FAIL_IF(p_line == NULL);
    p_defs = calloc(strlen(p_line) + 2, 1);
    FAIL_IF(p_defs == NULL);
    memset(p_defs, ' ', strlen(p_line)+1);
    p_defs[strlen(p_line)+0] = ';';
    p_defs[strlen(p_line)+1] = '\0';

    while (p_line[i] != '\0')
    {
        if (p_line[i] == ';' && identifiers > 1)
        {
            store_and_clear_definition(p_line, p_defs, &p_defstart, &num_defs, start, i);
        }
        if (p_line[i] == ';')
        {
            identifiers = 0;
            start = i + 1;
        }
        while (p_line[i] == ' ' || p_line[i] == '*')
        {
            i++;
        }

        if (p_line[i] == '_' || isalpha(p_line[i]))
        {
            int skipahead;
            int valid_identifier = 1;
            i++;
            while (isalnum(p_line[i]) || p_line[i] == '_')
            {
                i++;
            }
            skipahead = i;

            while (p_line[skipahead] != '\0' && p_line[skipahead] != ';')
            {
                if (p_line[skipahead] == '=')
                {
                    // ok
                }
                else
                {
                    if (p_line[skipahead] == '(' && p_line[i] != '\0')
                    {
                        while(p_line[i] != ';' && p_line[i] != '\0')
                        {
                            i++;
                        }
                        valid_identifier = 0;
                    }

                }
                skipahead++;
            }
            if (valid_identifier)
                identifiers++;
        }
        else
        {
            i++;
        }
    }
    if (identifiers > 1)
    {
        store_and_clear_definition(p_line, p_defs, &p_defstart, &num_defs, start, i);
    }
    i = 0;
    while(i < (int)strlen(p_line))
    {
        if (p_defs[i] == ';')
        {
            i++;
            while(p_defs[i] == ' ' || p_defs[i] == ';')
            {
                p_defs[i] = ' ';
                i++;
            }
        }
        i++;
    }
    if (crepl_getoptions(p_crepl).verbosity >= 11)
        fprintf(stderr, "crepl_parsedefs(): p_defs: \"%s\n\"", p_defs);
    p_crepl->p_nodef_line = p_line;
    p_line = NULL;
    p_crepl->p_defs_line = p_defs;
    p_defs = NULL;
end:
    free(p_defs);
    free(p_line);
    return status;
}

int crepl_preparse(struct crepl *p_crepl, const char *p_input, crepl_state_t *p_state)
{
    int i;
    crepl_state_t state = CREPL_STATE_CPARSER;
    preparse_userdata_t userdata;
    int status = 0;
    char preparse_filename[] = "preparse.c";
    char *p_parsefile_fullname = NULL;
    char *p_line_copy = NULL;
    char *p_line = NULL;
    int num_identifiers = 0;
    char **pp_identifiers = NULL;
    char **pp_cmds = NULL;
    char *p_cmds = NULL;
    int cmdidx = 0;

    for (i = 0; i < (int)strlen(p_input);i++)
    {
        if (p_input[i] == ' ' || p_input[i] == '\t')
            continue;
        if (p_input[i] == '#')
        {
            *p_state = CREPL_STATE_PREPROCESSOR;
            ENDOK_IF(1);
        }
    }
    p_line_copy = strdup(p_input);
    FAIL_IF(p_line_copy == NULL);
    p_line = stripwhite(p_line_copy);
    if (p_crepl->options.verbosity >= 11)
        fprintf(stderr, "crepl_preparse(): p_line=%s\n", p_line);
    
    status = get_identifiers(p_line, &num_identifiers, &pp_identifiers);
    FAIL_IF(status != 0);
    if (num_identifiers)
    {

        pp_cmds = calloc((num_identifiers + 1)* sizeof(char*) + (strlen(p_line) + 1) * sizeof(char), 1);
        FAIL_IF(pp_cmds == NULL);
        memcpy((char*)&pp_cmds[num_identifiers + 1], p_line, strlen(p_line));
        // NULL terminate vector before it's content
        pp_cmds[num_identifiers] = NULL;

        p_parsefile_fullname = calloc(strlen(p_crepl->p_tmpdir) + 1 + strlen(preparse_filename) + 1, 1);
        FAIL_IF(p_parsefile_fullname == NULL);
        strcpy(p_parsefile_fullname, p_crepl->p_tmpdir);
        p_parsefile_fullname[strlen(p_crepl->p_tmpdir)] = '/';
        strcpy(&p_parsefile_fullname[strlen(p_crepl->p_tmpdir) + 1], preparse_filename);

        for (i = 0; pp_identifiers[i] != NULL; i++)
        {
            char *p_null_identifier[1] = {NULL};
            //state = CREPL_STATE_CPARSER;

            userdata.p_cur_id = pp_identifiers[i];
            userdata.found_cur_id = 0;

            FAIL_IF(resolve_identifier(p_crepl, p_parsefile_fullname, p_line, &userdata, p_null_identifier));

            // identifier is not defined
            if (userdata.found_cur_id == 0)
            {
                if (is_builtin_cmd(userdata.p_cur_id))
                {
                    if (pp_cmds[0] == NULL)
                    {
                        p_cmds = (char*)&pp_cmds[num_identifiers + 1];
                        FAIL_IF(p_cmds == NULL);
                    }
                    FAIL_IF(pp_identifiers[i] == NULL);
                    FAIL_IF(p_cmds == NULL);
                    strcpy(p_cmds, pp_identifiers[i]);
                    pp_cmds[cmdidx] = p_cmds;
                    p_cmds += strlen(pp_identifiers[i]) + 1;
                    cmdidx++;

                    if (has_trailing_closed_parenthesis(p_line))
                    {
                        state = CREPL_STATE_CPARSER;
                    }
                    else if (has_trailing_open_parenthesis(p_line))
                    {
                        state = CREPL_STATE_CMDARG;
                    } 
                    else
                    {
                        state = CREPL_STATE_CMDSTART;
                    }
                    break;
                }
                else if (is_system_cmd(userdata.p_cur_id))
                {
                    // the identifier is available as a system command
                    // try to define the identifier as a function

                    userdata.found_cur_id = 0;
                    FAIL_IF(resolve_identifier(p_crepl, p_parsefile_fullname, p_line, &userdata, pp_identifiers));
                    

                    // unknown error
                    if (userdata.found_cur_id == 0)
                    {
                        state = CREPL_STATE_ERROR;
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
                        FAIL_IF(p_cmds == NULL);
                        FAIL_IF(pp_identifiers[i] == NULL);
                        strcpy(p_cmds, pp_identifiers[i]);
                        pp_cmds[cmdidx] = p_cmds;
                        p_cmds += strlen(pp_identifiers[i]) + 1;
                        cmdidx++;

                        if (has_trailing_closed_parenthesis(p_line))
                        {
                            state = CREPL_STATE_CPARSER;
                        }
                        else if (has_trailing_open_parenthesis(p_line))
                        {
                            state = CREPL_STATE_CMDARG;
                        } 
                        else
                        {
                            state = CREPL_STATE_CMDSTART;
                        }
                        break;
                    }
                }
                else
                {
                    // the identifier could not be resolved, nor is it a system command
                    state = CREPL_STATE_ERROR;
                    break;
                }
            }
            state = CREPL_STATE_CPARSER;
        }
    }
    *p_state = state;
    if (p_crepl->options.verbosity >= 11)
        fprintf(stderr, "crepl_preparse(): state=%d\n", (unsigned int)state);
    free(p_crepl->pp_cmds);
    p_crepl->pp_cmds = pp_cmds;
    pp_cmds = NULL;
end:
    p_cmds = NULL;
    free(pp_identifiers);
    p_line = NULL;
    free(p_parsefile_fullname);
    free(p_line_copy);
    free(pp_cmds);

    return status;
}
static size_t find_matching(struct crepl *p_crepl, char end, char *p_incomplete)
{
    // TODO: why 1?
    size_t i = 1;
    int found = 0;
    int escaped = 0;

    if (p_incomplete[0] == '\0')
	return 0;

    if (p_crepl->options.verbosity >= 11)
        fprintf(stderr, "find_matching(): p_incomplete=%s\n", p_incomplete);

    while (p_incomplete[i] != '\0')
    {
        if (p_incomplete[i] == '\\'
            && p_incomplete[i+1] != 'u'
            && p_incomplete[i+1] != 'r'
            && p_incomplete[i+1] != 'n')
        {
            escaped = !escaped;
            i++;
            continue;
        }
        if (p_incomplete[i] == '\n')
        {
            i++;
            continue;
        }

        if (escaped)
        {
            i++;
            continue;
        }

        if (p_incomplete[i] == end)
        {
            found = 1;
            break;
        }

        switch (p_incomplete[i])
        {
            case '{':
                {
                    i += find_matching(p_crepl, '}', &p_incomplete[i]);
                    break;
                }
            case '(':
                {
                    i += find_matching(p_crepl, ')', &p_incomplete[i]);
                    break;
                }
            case '[':
                {
                    i += find_matching(p_crepl, ']', &p_incomplete[i]);
                    break;
                }
            case '"':
                {
                    i += find_matching(p_crepl, '"', &p_incomplete[i]);
                    break;
                }
            case '\'':
                {
                    i += find_matching(p_crepl, '\'', &p_incomplete[i]);
                    break;
                }
            default:
                {
                    break;
                }
        }
        i++;
    }
    if (!found)
    {
        p_incomplete[i] = end;
        p_incomplete[i+1] = '\0';
    }
    return i;
}
int crepl_finalize(struct crepl *p_crepl, char *p_unfinalized, char **pp_finalized)
{
    int status = 0;
    char *p_finalized = NULL;

    if (p_crepl->options.verbosity >= 11)
        fprintf(stderr, "crepl_finalize(): p_unfinalized = %s\n", p_unfinalized);


    p_finalized = calloc(strlen(p_unfinalized) * 2 + 1, 1);
    FAIL_IF(p_finalized == NULL);
    memcpy(p_finalized, p_unfinalized, strlen(p_unfinalized));

    find_matching(p_crepl, '\0', p_finalized);

    if (p_crepl->options.verbosity >= 11)
        fprintf(stderr, "crepl_finalize(): p_finalized = %s\n", p_finalized);

    *pp_finalized = p_finalized;
    p_finalized = NULL;
end:
    free(p_finalized);
    return status;
}

#if 0
E_CREPL crepl_argnum(struct crepl *p_crepl, char *p_input, int *p_fnidx_out, int *p_argnum_out)
{

}

#endif // 0
