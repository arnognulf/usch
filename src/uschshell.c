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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <ctype.h>
#include <assert.h>
#include <dirent.h>

#include "clang-c/Index.h"
#include "usch.h"
#include "usch_debug.h"
#define MAX(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
typedef struct
{
    char *p_symname;
} usch_def_t;

#include "uschshell.h"
#include "../external/uthash/src/uthash.h"

static int count_spaces(char *p_input)
{
    size_t i = 0;
    while (p_input[i] != '\0' && (p_input[i] == ' '  || p_input[i] == '\t'))
    {
        i++;
    }
    return i;
}
static size_t get_type_len(char *p_defname)
{
    size_t i;
    size_t last_type_pos = 0;
    size_t deflen = strlen(p_defname);

    for (i = 0; i < deflen; i++)
    {
        if (p_defname[i] == ' ')
            last_type_pos = i;
        if (p_defname[i] == '*')
            last_type_pos = i;
    }
    return last_type_pos;
}

static char *get_symname(char *p_defname)
{
    char *p_tmp = p_defname;
    char *p_symname = p_defname;
    while (*p_tmp != '\0')
    {
        if (*p_tmp == ' ' || *p_tmp == '*')
        {
            p_symname = p_tmp;
        }
        p_tmp++;
    }
    return p_symname + 1;
}

static int parse_line(char *p_input, usch_def_t *p_definition)
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

#define usch_shell_cc(...) usch_cmd("gcc", ##__VA_ARGS__)

#define USCHSHELL_DEFINE_SIZE 8
typedef struct uschshell_def_t
{
    UT_hash_handle hh;
    size_t size;
    void *p_body_data;
    void *p_alloc_data;
    uint8_t data[USCHSHELL_DEFINE_SIZE];
    char defname[];
} uschshell_def_t;

typedef struct uschshell_cmd_t
{
    UT_hash_handle hh;
    char cmdname[];
} uschshell_cmd_t;

typedef struct uschshell_lib_t
{
    struct uschshell_lib_t *p_next;
    void *p_handle;
    char libname[];
} uschshell_lib_t;

typedef struct uschshell_sym_t
{
    UT_hash_handle hh;
    void *p_handle;
    char symname[];
} uschshell_sym_t;


typedef struct uschshell_t 
{
    uschshell_def_t *p_defs;
    uschshell_cmd_t *p_cmds;
    uschshell_lib_t *p_libs;
    uschshell_sym_t *p_syms;
    char tmpdir[];
} uschshell_t;

int uschshell_define(uschshell_t *p_context, size_t var_size, char *p_defname)
{
    int status = 0;
    if (p_context == NULL || p_defname == NULL)
        return -1;
    uschshell_def_t *p_defs = p_context->p_defs;
    uschshell_def_t *p_def = NULL;
    void *p_alloc_data = NULL;
    HASH_FIND_STR(p_defs, p_defname, p_def);

    FAIL_IF(p_def != NULL);

    p_def = calloc(sizeof(uschshell_def_t) + strlen(p_defname) + 1, 1);
    FAIL_IF(p_def == NULL);

    if (var_size > USCHSHELL_DEFINE_SIZE)
    {
        p_alloc_data = calloc(var_size, 1);
        FAIL_IF(p_alloc_data == NULL);
        p_def->p_alloc_data = p_alloc_data;
    }
    p_def->size = var_size;
    strcpy(p_def->defname, p_defname);

    HASH_ADD_STR(p_context->p_defs, defname, p_def);
    p_def = NULL;
    p_alloc_data = NULL;
end:
    free(p_alloc_data);
    free(p_def);
    return status;
}
void uschshell_undef(uschshell_t *p_context, char *p_defname)
{
    if (p_context == NULL || p_defname == NULL)
        return;
    uschshell_def_t *p_def = NULL;
    uschshell_def_t *p_defs = p_context->p_defs;
    HASH_FIND_STR(p_defs, p_defname, p_def);
    if (p_def != NULL)
    {
        free(p_def->p_alloc_data);
        p_def->p_alloc_data = NULL;
        free(p_def->p_body_data);
        p_def->p_body_data = NULL;
        HASH_DEL(p_defs, p_def);
    }
    return;
}
int uschshell_load(uschshell_t *p_context, char *p_defname, void *p_data)
{
    int status = 0;
    if (p_context == NULL || p_defname == NULL)
        return -1;
    uschshell_def_t *p_def = NULL;
    uschshell_def_t *p_defs = p_context->p_defs;
    HASH_FIND_STR(p_defs, p_defname, p_def);
    if (p_def != NULL)
    {
        memcpy(p_data, p_def->data, p_def->size);
    }
    else
    {
        status = -1;
    }
    return status;
}

static void print_updated_variables(char *p_defname, void *p_data)
{
    size_t type_len;
    type_len = get_type_len(p_defname);

    if (strncmp(p_defname, "unsigned int", type_len) == 0)
        printf("%%%s = %d\n", get_symname(p_defname), *(unsigned int*)p_data);
    if (strncmp(p_defname, "char", type_len) == 0)
        printf("%%%s = %c\n", get_symname(p_defname), *(char*)p_data);
    else if (strncmp(p_defname, "int", type_len) == 0)
        printf("%%%s = %d\n", get_symname(p_defname), *(int*)p_data);
    else if (strncmp(p_defname, "double", type_len) == 0)
        printf("%%%s = %f\n", get_symname(p_defname), *(double*)p_data);
    else if (strncmp(p_defname, "float", type_len) == 0)
        printf("%%%s = %f\n", get_symname(p_defname), *(float*)p_data);
    else
        printf("%%%s = 0x%lx\n", get_symname(p_defname), *(long unsigned int*)p_data);

}

int uschshell_store(uschshell_t *p_context, char *p_defname, void *p_data)
{
    int status = 0;
    uint8_t tmp[USCHSHELL_DEFINE_SIZE] = {0};
    int is_updated = 0;
    if (p_context == NULL || p_defname == NULL)
        return -1;
    uschshell_def_t *p_def = NULL;
    uschshell_def_t *p_defs = p_context->p_defs;
    HASH_FIND_STR(p_defs, p_defname, p_def);
    
    if (p_def != NULL)
    {
            memcpy(tmp, p_data, p_def->size);

            size_t i;
            for (i = 0; i < p_def->size; i++)
            {
                if (((uint8_t*)tmp)[i] != ((uint8_t*)p_def->data)[i])
                {
                    is_updated = 1;
                }
            }

            memcpy(p_def->data, p_data, p_def->size);
            if (is_updated)
                print_updated_variables(p_defname, p_data);
        //}
    }
    else
    {
        status = -1;
    }
    return status;
}

int uschshell_define_fn(struct uschshell_t *p_context, char *p_fndefname, char *p_body)
{
    int status = 0;
    if (p_context == NULL || p_fndefname == NULL || p_body == NULL)
        return -1;
    uschshell_def_t *p_defs = p_context->p_defs;
    uschshell_def_t *p_def = NULL;
    void *p_body_data = NULL;
    HASH_FIND_STR(p_defs, p_fndefname, p_def);

    FAIL_IF(p_def != NULL);

    p_def = calloc(sizeof(uschshell_def_t) + strlen(p_fndefname) + 1, 1);
    FAIL_IF(p_def == NULL);

    p_body_data = calloc(strlen(p_body) + 1, 1);

    FAIL_IF(p_body_data == NULL);

    p_def->p_body_data = p_body_data;
    strcpy(p_def->defname, p_fndefname);

    HASH_ADD_STR(p_context->p_defs, defname, p_def);
    p_def = NULL;
    p_body_data = NULL;
end:
    free(p_body_data);
    free(p_def);
    return status;
}

int uschshell_create(uschshell_t **pp_context)
{
    int status = 0;
    uschshell_t *p_context = NULL;
    uschshell_def_t *p_def = NULL;
    uschshell_cmd_t *p_cmd = NULL;
    uschshell_sym_t *p_sym = NULL;
    char dir_template[] = "/tmp/usch-XXXXXX";
    char *p_tempdir = NULL;

    p_tempdir = mkdtemp(dir_template);
    FAIL_IF(p_tempdir == NULL);

    p_context = calloc(sizeof(uschshell_t) + strlen(dir_template) + 1, 1);
    FAIL_IF(p_context == NULL);

    p_def = calloc(sizeof(uschshell_def_t) + 1, 1);
    FAIL_IF(p_def == NULL);

    p_cmd = calloc(sizeof(uschshell_cmd_t) + 1, 1);
    FAIL_IF(p_cmd == NULL);

    p_sym = calloc(sizeof(uschshell_sym_t) + 1, 1);
    FAIL_IF(p_sym == NULL);

    HASH_ADD_STR(p_context->p_defs, defname, p_def);
    HASH_ADD_STR(p_context->p_cmds, cmdname, p_cmd);
    HASH_ADD_STR(p_context->p_syms, symname, p_sym);
    strcpy(p_context->tmpdir, p_tempdir);

    *pp_context = p_context;
    p_context = NULL;
    p_cmd = NULL;
    p_def = NULL;
    p_sym = NULL;
end:
    free(p_sym);
    free(p_cmd);
    free(p_def);
    free(p_context);
    return status;
}
void uschshell_destroy(uschshell_t *p_context)
{

    if (p_context)
    {
        uschshell_cmd_t *p_tmpcmd = NULL;
        uschshell_cmd_t *p_cmd = NULL;
        uschshell_cmd_t *p_cmds = p_context->p_cmds;

        HASH_ITER(hh, p_cmds, p_cmd, p_tmpcmd) {
            HASH_DEL(p_cmds, p_cmd);
        }

        uschshell_sym_t *p_tmpsym = NULL;
        uschshell_sym_t *p_sym = NULL;
        uschshell_sym_t *p_syms = p_context->p_syms;

        HASH_ITER(hh, p_syms, p_sym, p_tmpsym) {
            HASH_DEL(p_syms, p_sym);
        }

        uschshell_def_t *p_tmpdef = NULL;
        uschshell_def_t *p_def = NULL;
        uschshell_def_t *p_defs = p_context->p_defs;

        HASH_ITER(hh, p_defs, p_def, p_tmpdef) {
            free(p_def->p_body_data);
            free(p_def->p_alloc_data);

            HASH_DEL(p_defs, p_def);
        }


        // TODO: free uschshell_lib_t

    }
    free(p_context);
    return;
}
static int fwrite_ok(char* p_str, FILE *p_file)
{
    size_t bytes_written;
    size_t bytes_to_write;
    bytes_to_write = strlen(p_str);
    bytes_written = fwrite(p_str, sizeof(char), bytes_to_write, p_file);
    if (bytes_to_write != bytes_written)
    {
        return 0;
    }
    else
    {
        return 1;
    }

}
static int write_definitions_h(uschshell_t *p_context, char *p_tempdir)
{
    int status = 0;
    char definitions_h_filename[] = "definitions.h";
    size_t filename_length;
    size_t tempdir_len;
    char *p_definitionsfile = NULL;
    FILE *p_definitions_h = NULL;
    uschshell_def_t *p_defs = NULL;
    uschshell_def_t *p_def = NULL;
    uschshell_def_t *p_tmp = NULL;

    p_defs = p_context->p_defs;
    tempdir_len = strlen(p_tempdir);
    filename_length = tempdir_len + 1 + strlen(definitions_h_filename) + 1;
    p_definitionsfile = calloc(filename_length, 1);
    FAIL_IF(p_definitionsfile == NULL);

    strcpy(p_definitionsfile, p_tempdir);

    p_definitionsfile[tempdir_len] = '/';
    strcpy(&p_definitionsfile[tempdir_len + 1], definitions_h_filename);
    p_definitionsfile[filename_length-1] = '\0';
    p_definitions_h = fopen(p_definitionsfile, "w+");
    FAIL_IF(p_definitions_h == NULL);
    FAIL_IF(!fwrite_ok("struct uschshell_t;\n", p_definitions_h));
    HASH_ITER(hh, p_defs, p_def, p_tmp)
    {
        if (strcmp(p_def->defname, "") != 0)
        {
            FAIL_IF(!fwrite_ok(p_def->defname, p_definitions_h));
            FAIL_IF(!fwrite_ok(";\n", p_definitions_h));
        }
    }
    FAIL_IF(!fwrite_ok("\nvoid uschshell_load_vars(struct uschshell_t *p_context)\n{\n", p_definitions_h));
    HASH_ITER(hh, p_defs, p_def, p_tmp)
    {
        if (strcmp(p_def->defname, "") != 0)
        {
            FAIL_IF(!fwrite_ok("\tuschshell_load(p_context, \"", p_definitions_h));
            FAIL_IF(!fwrite_ok(p_def->defname, p_definitions_h));
            FAIL_IF(!fwrite_ok("\", (void*)&", p_definitions_h));
            FAIL_IF(!fwrite_ok(get_symname(p_def->defname), p_definitions_h));
            FAIL_IF(!fwrite_ok(");\n", p_definitions_h));
        }
    }

    FAIL_IF(!fwrite_ok("return;\n}\n", p_definitions_h));

    FAIL_IF(!fwrite_ok("\nvoid uschshell_store_vars(struct uschshell_t *p_context)\n{\n", p_definitions_h));
    HASH_ITER(hh, p_defs, p_def, p_tmp)
    {
        if (strcmp(p_def->defname, "") != 0)
        {
            FAIL_IF(!fwrite_ok("\tuschshell_store(p_context, \"", p_definitions_h));
            FAIL_IF(!fwrite_ok(p_def->defname, p_definitions_h));
            FAIL_IF(!fwrite_ok("\", (void*)&", p_definitions_h));
            FAIL_IF(!fwrite_ok(get_symname(p_def->defname), p_definitions_h));
            FAIL_IF(!fwrite_ok(");\n", p_definitions_h));
        }
    }

    FAIL_IF(!fwrite_ok("return;\n}\n", p_definitions_h));


end:
    if(p_definitions_h)
        fclose(p_definitions_h);
    free(p_definitionsfile);
    return status;
}
int uschshell_pathhash(uschshell_t *p_context)
{
    int status = 0;
    char **pp_path = NULL;
    int num_paths = 0;
    int i;
    DIR *p_dir = NULL;
    uschshell_cmd_t *p_cmds = NULL;
    uschshell_cmd_t *p_cmd = NULL;

    if (p_context == NULL)
        return -1;

    p_cmds = p_context->p_cmds;
    num_paths = usch_strsplit(getenv("PATH"), ":", &pp_path);
    FAIL_IF(num_paths < 1);

    for (i = 0; i < num_paths; i++)
    {
        struct dirent *p_ent;
        if ((p_dir = opendir(pp_path[i])) != NULL)
        {
            while ((p_ent = readdir(p_dir)) != NULL)
            {
                uschshell_cmd_t *p_found_cmd = NULL;

                if (strcmp(p_ent->d_name, "..") == 0)
                    continue;
                if (strcmp(p_ent->d_name, ".") == 0)
                    continue;

                HASH_FIND_STR(p_cmds, p_ent->d_name, p_found_cmd);
                if (p_found_cmd)
                    continue;
                p_cmd = calloc(sizeof(uschshell_cmd_t) + strlen(p_ent->d_name) + 1, 1);
                FAIL_IF(p_cmd == NULL);
                strcpy(p_cmd->cmdname, p_ent->d_name);

                HASH_ADD_STR(p_cmds, cmdname, p_cmd);
            }
        }
    }
end:
    if (p_dir)
        closedir(p_dir);
    free(pp_path);
    free(p_cmd);
    return status;
}
static int is_reserved_word(char *p_item)
{
    size_t i;
    char *p_keywords[] = { \
            "printf", \
            "open"};

    for (i = 0; i < sizeof(p_keywords)/sizeof(p_keywords[0]); i++)
    {
        if (strncmp(p_item, p_keywords[i], strlen(p_keywords[i])) == 0)
            return 1;
    }
    return 0;
}
static int is_c_keyword(char *p_item)
{
    size_t i;
    char *p_keywords[] = { \
            "auto", \
            "break", \
            "case", \
            "char", \
            "const", \
            "continue", \
            "default", \
            "do", \
            "double", \
            "else" \
            "enum", \
            "extern", \
            "float", \
            "for", \
            "goto", \
            "if", \
            "int", \
            "long", \
            "register", \
            "return", \
            "short", \
            "signed", \
            "sizeof", \
            "static", \
            "struct", \
            "switch", \
            "typedef", \
            "union", \
            "unsigned", \
            "void", \
            "volatile", \
            "while", \
            /* common compiler prefixes, but not actual c keywords: */ \
            "_", \
            "inline", \
            "asm", \
            "typeof"};

    for (i = 0; i < sizeof(p_keywords)/sizeof(p_keywords[0]); i++)
    {
        if (strncmp(p_item, p_keywords[i], strlen(p_keywords[i])) == 0)
            return 1;
    }
    return 0;
}

static void trim_end_space(char *p_input)
{
    size_t i;
    size_t len = strlen(p_input);

    for (i = len - 1; i > 0; i--)
    {
        if (p_input[i] == ' ' || p_input[i] == '\t')
            p_input[i] = '\0';
        else
             break;
    }
}

static int pre_assign(char *p_input, char **pp_pre_assign)
{
    int status = 0;
    char *p_pre_assign = NULL;
    int i = 0;

    p_pre_assign = strdup(p_input);

    FAIL_IF(p_pre_assign == NULL);

    i += count_spaces(p_pre_assign);
    while (p_pre_assign[i] != '\0')
    {
        if (p_input[i] == '=')
            p_pre_assign[i] = '\0';
        i++;
    }
    trim_end_space(p_pre_assign);
    *pp_pre_assign = p_pre_assign;
    p_pre_assign = NULL;
end:
    free(p_pre_assign);
    return status;
}

static int post_assign(char *p_input, char **pp_post_assign)
{
    int status = 0;
    char *p_post_assign = NULL;
    int i = 0;

    p_post_assign = calloc(strlen(p_input) + 1, 1);

    FAIL_IF(p_post_assign == NULL);

    while (p_input[i] != '\0')
    {
        if (p_input[i] == '=')
            strcpy(p_post_assign, &p_input[i+1]);
        i++;
    }
    *pp_post_assign = p_post_assign;
    p_post_assign = NULL;
end:
    free(p_post_assign);
    return status;
}

int uschshell_is_cmd(uschshell_t *p_context, char *p_item)
{
    uschshell_cmd_t *p_cmds = NULL;
    uschshell_cmd_t *p_found_cmd = NULL;
    int i = 0;

    if (p_context == NULL || p_item == NULL)
        return -1;
    if (is_c_keyword(p_item))
        return 0;
    if (is_reserved_word(p_item))
        return 0;
    if (strncmp(p_item, "cd", 2) == 0)
        return 1;

    while (p_item[i] != '\0')
    {
        if (p_item[i] == ' ')
            return 0;
        if (p_item[i] == '(')
            return 1;
        i++;
    }
    p_cmds = p_context->p_cmds;
    HASH_FIND_STR(p_cmds, p_item, p_found_cmd);
    if (p_found_cmd)
        return 1;
    else
        return 0;
}
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
// fun excercise: 
// guess ptr or mult op without type info!
//
// does the string have a pointer definition?
// nope.
// a *= b
// probably yes
// a * b = c...
// definitely not
// a = b...
// empty assignment or pointer decl?
// a * b...[not:=]
#if 0
static int guess_ptr_def(char *p_input)
{
    size_t i = 0;
    size_t len = strlen(p_input);
    size_t num_nonidentifiers = 0;
    int has_star = 0;
    int is_ptr_def = 0;
    // *a ..
    if (count_stars(p_input) > 1)
    {
        return 0;
    }
    // erronous strings that end with * or = will be 
    // caugt by the compiler 
    while (i < (len - 1))
    {
        // a *= b
        if (strncmp(&p_input[i], "*=", 2) == 0)
        {
            is_ptr_def = 0;
            break;
        }

        i += count_identifier(&p_input[i]);
        num_nonidentifiers = count_nonidentifier(&p_input[i]);

        if (num_nonidentifiers > 1)
        {
            printf("num_nonidentifiers\n");
            is_ptr_def = 0;
            break;
        }
        // a * b
        // a ** b
        // a *** b
        if (p_input[i] == '*')
        {
            size_t counted_stars;
            counted_stars += count_stars(&p_input[i]);
            if (counted_stars > 1)
            {
                is_ptr_def = 1;
                break;
            }
            else
            {
                has_star++;

                // a * b * c
                if (has_star > 1)
                {
                    is_ptr_def = 1;
                    break;
                }

            }
        }
         
        if (p_input[i] == '=' && has_star)
        {
            is_ptr_def = 1;
            break;
        }
 
        i++;
    }
    return is_ptr_def;
}
#endif // 0
static int is_definition(char *p_input)
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
int uschshell_eval(uschshell_t *p_context, char *p_input)
{
    int status = 0;
    usch_def_t definition = {0};
    FILE *p_stmt_c = NULL;
    void *p_handle = NULL;
    int (*dyn_func)(uschshell_t*);
    int (*uschshell_store_vars)(uschshell_t*);
    int (*uschshell_load_vars)(uschshell_t*);
    char *p_error = NULL;
    char **pp_path = NULL;
    size_t filename_length;
    size_t dylib_length;
    // TODO: we need to determine wether stmt need to be usch-defined or not
    // declare dummy function to get overridden errors
    // use macro to call the real function
    struct stat sb;
    char *p_fullpath_uschrc_h = NULL;
    char uschrc_h[] = "/.uschrc.h";

    char expr_c_filename[] = "expr.c";
    char dylib_filename[] = "dyn_stmt";
    char *p_tempdir = NULL;
    char *p_tempfile = NULL;
    char *p_tempdylib = NULL;
    size_t tempdir_len = 0;
    char *p_pre_assign = NULL;
    char *p_post_assign = NULL;

    if (p_context == NULL || p_input == NULL)
        return -1;

    FAIL_IF(usch_strsplit(getenv("PATH"), ":", &pp_path) < 0);
    p_tempdir = p_context->tmpdir;
    FAIL_IF(p_tempdir == NULL);
    tempdir_len = strlen(p_tempdir);
    filename_length = tempdir_len + 1 + strlen(expr_c_filename) + 1;
    p_tempfile = malloc(filename_length);
    FAIL_IF(p_tempfile == NULL);
    strcpy(p_tempfile, p_tempdir);
    p_tempfile[tempdir_len] = '/';
    strcpy(&p_tempfile[tempdir_len + 1], expr_c_filename);
    p_tempfile[filename_length-1] = '\0';
    p_stmt_c = fopen(p_tempfile, "w+");
    FAIL_IF(p_stmt_c == NULL);

    FAIL_IF(write_definitions_h(p_context, p_tempdir) != 0);
    
    FAIL_IF(parse_line(p_input, &definition) < 1);
    
    FAIL_IF(!fwrite_ok("#include <usch.h>\n", p_stmt_c));
    FAIL_IF(!fwrite_ok("#include \"definitions.h\"\n", p_stmt_c));

    p_fullpath_uschrc_h = calloc(sizeof(getenv("HOME")) + sizeof(uschrc_h) + 2, 1);
    FAIL_IF(p_fullpath_uschrc_h == NULL);
    strcpy(p_fullpath_uschrc_h, getenv("HOME"));
    strcpy(p_fullpath_uschrc_h + sizeof(getenv("HOME")) + 2, uschrc_h);

    if (stat(p_fullpath_uschrc_h, &sb) != -1)
    {
        FAIL_IF(!fwrite_ok("#include \"", p_stmt_c));
        FAIL_IF(!fwrite_ok(getenv("HOME"), p_stmt_c));
        FAIL_IF(!fwrite_ok("/.uschrc.h\"\n", p_stmt_c));
    }

    if (uschshell_is_cmd(p_context, p_input))
    {
        if (strcmp(definition.p_symname, "cd") != 0)
        {
            FAIL_IF(!fwrite_ok("#define ", p_stmt_c));
            FAIL_IF(!fwrite_ok(definition.p_symname, p_stmt_c));
            FAIL_IF(!fwrite_ok("(...) usch_cmd(\"", p_stmt_c));
            FAIL_IF(!fwrite_ok(definition.p_symname, p_stmt_c));
            FAIL_IF(!fwrite_ok("\", ##__VA_ARGS__)\n", p_stmt_c));
        }
        FAIL_IF(!fwrite_ok("int dyn_func(struct uschshell_t *p_context)\n{\n\t", p_stmt_c));
        FAIL_IF(!fwrite_ok(p_input, p_stmt_c));

        FAIL_IF(!fwrite_ok("\n\treturn 0;\n}\n", p_stmt_c));
    }
    else if(is_definition(p_input))
    {
        FAIL_IF(pre_assign(p_input, &p_pre_assign) != 0);
        FAIL_IF(!fwrite_ok(p_pre_assign, p_stmt_c));
        FAIL_IF(!fwrite_ok(";\n", p_stmt_c));

        FAIL_IF(!fwrite_ok("int dyn_func(struct uschshell_t *p_context)\n{\n", p_stmt_c));
        FAIL_IF(!fwrite_ok("\tuschshell_define(p_context, sizeof(", p_stmt_c));
        FAIL_IF(!fwrite_ok(get_symname(p_pre_assign), p_stmt_c));
        FAIL_IF(!fwrite_ok("), \"", p_stmt_c));
        FAIL_IF(!fwrite_ok(p_pre_assign, p_stmt_c));
        FAIL_IF(!fwrite_ok("\");\n", p_stmt_c));
        
        FAIL_IF(post_assign(p_input, &p_post_assign));
        if (strlen(p_post_assign) > 0)
        {
            FAIL_IF(!fwrite_ok("\t", p_stmt_c));
            FAIL_IF(!fwrite_ok(get_symname(p_pre_assign), p_stmt_c));
            FAIL_IF(!fwrite_ok(" = ", p_stmt_c));
            FAIL_IF(!fwrite_ok(p_post_assign, p_stmt_c));
            FAIL_IF(!fwrite_ok(";\n", p_stmt_c));

            FAIL_IF(!fwrite_ok("\tuschshell_store(p_context, \"", p_stmt_c));
            FAIL_IF(!fwrite_ok(p_pre_assign, p_stmt_c));
            FAIL_IF(!fwrite_ok("\", (void*)&", p_stmt_c));
            FAIL_IF(!fwrite_ok(get_symname(p_pre_assign), p_stmt_c));
            FAIL_IF(!fwrite_ok(");\n", p_stmt_c));

        }

        FAIL_IF(!fwrite_ok("\treturn 0;\n}\n", p_stmt_c));
    }
    else /* as is */   
    {
        FAIL_IF(!fwrite_ok("int dyn_func(struct uschshell_t *p_context)\n{\n\t", p_stmt_c));
        FAIL_IF(!fwrite_ok(p_input, p_stmt_c));

        FAIL_IF(!fwrite_ok(";\n\treturn 0;\n}\n", p_stmt_c));
    }

    fclose(p_stmt_c);
    p_stmt_c = NULL;
    usch_cmd("cat", p_tempfile);
    dylib_length = tempdir_len + 1 + strlen(dylib_filename) + 1;
    p_tempdylib = malloc(dylib_length);
    FAIL_IF(p_tempdylib == NULL);
    strcpy(p_tempdylib, p_tempdir);
    p_tempdylib[tempdir_len] = '/';
    strcpy(&p_tempdylib[tempdir_len + 1], dylib_filename);
    p_tempdylib[dylib_length-1] = '\0';
    if (usch_shell_cc("-rdynamic", "-shared", "-fPIC", "-o", p_tempdylib, p_tempfile) != 0) 
    {
        fprintf(stderr, "usch: compile error\n");
        ENDOK_IF(1);
    }

    p_handle = dlopen(p_tempdylib, RTLD_LAZY);
    FAIL_IF(!p_handle);

    dlerror();

    *(void **) (&uschshell_load_vars) = dlsym(p_handle, "uschshell_load_vars");

    FAIL_IF((p_error = dlerror()) != NULL);
    (*uschshell_load_vars)(p_context);

    *(void **) (&dyn_func) = dlsym(p_handle, "dyn_func");

    FAIL_IF((p_error = dlerror()) != NULL);
    (*dyn_func)(p_context);

    *(void **) (&uschshell_store_vars) = dlsym(p_handle, "uschshell_store_vars");

    FAIL_IF((p_error = dlerror()) != NULL);
    (*uschshell_store_vars)(p_context);


end:
    free(definition.p_symname);
    if (p_handle)
        dlclose(p_handle);
    free(pp_path);
    free(p_tempfile);
    free(p_pre_assign);
    free(p_post_assign);
    free(p_fullpath_uschrc_h);

    if (p_stmt_c != NULL)
        fclose(p_stmt_c);

    return status;
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

#if 0
enum CXChildVisitResult test_clang_visitor(CXCursor cursor, 
        CXCursor parent, 
        CXClientData client_data)
{
    (void)parent;
    (void)client_data;
    enum CXChildVisitResult res = CXChildVisit_Recurse;
    CXString cxstr = {0};

    switch (cursor.kind) 
    {
    case CXCursor_FunctionDecl:
    {
        int num_args = -1;
        int i;
        CXType return_type = {0};
        cxstr = clang_getCursorSpelling(cursor);
#if 0
        if (test_has_symbol("/usr/lib/i386-linux-gnu/libm.so", clang_getCString(cxstr)))
#else
        if (test_has_symbol("/usr/lib/x86_64-linux-gnu/libm.so", clang_getCString(cxstr)))

#endif // 0
        {
            CXString cxretkindstr = {0};
            return_type = clang_getCursorResultType(cursor);
            cxretkindstr = clang_getTypeSpelling(return_type);

            printf("%s %s(", clang_getCString(cxretkindstr), clang_getCString(cxstr));
            num_args = clang_Cursor_getNumArguments(cursor);
            for (i = 0; i < num_args; i++)
            {
                CXString cxkindstr;
                CXString cxargstr;
                CXCursor argCursor = {0};
                CXType argType = {0};

                argCursor = clang_Cursor_getArgument(cursor, i);
                argType = clang_getCursorType(argCursor);
                cxkindstr = clang_getTypeSpelling(argType);
                cxargstr = clang_getCursorSpelling(argCursor);
                printf("%s %s", clang_getCString(cxkindstr), clang_getCString(cxargstr));
                if (i != (num_args - 1))
                    printf(",");
                clang_disposeString(cxargstr);
            }
            printf(") {\n\treturn %s(", clang_getCString(cxstr));
            for (i = 0; i < num_args; i++)
            {
                CXString cxargstr;
                CXCursor argCursor = {0};

                argCursor = clang_Cursor_getArgument(cursor, i);
                cxargstr = clang_getCursorSpelling(argCursor);
                printf("%s", clang_getCString(cxargstr));
                if (i != (num_args - 1))
                    printf(",");
                clang_disposeString(cxargstr);
            }
            printf(");\n}\n");
            clang_disposeString(cxretkindstr);
        }
        else
        {
            res = CXChildVisit_Continue;
        }
        break;
    }
    default:
    {
        res = CXChildVisit_Continue;
        break;
    }
    }
    clang_disposeString(cxstr);
    return res;
}
static char *test_clang_parser()
{
    char *p_message = NULL;
    CXTranslationUnit p_tu = NULL;
    CXIndex p_idx = NULL;
    unsigned int visitorstatus = 0;

    p_idx = clang_createIndex(0, 0);
    mu_assert("error: clang_createIndex()", p_idx != NULL);

    p_tu = clang_parseTranslationUnit(p_idx, "/usr/include/math.h", NULL, 0, NULL, 0, 0);
    mu_assert("error: clang_parseTranslationUnit()", p_tu != NULL);

    visitorstatus = clang_visitChildren(clang_getTranslationUnitCursor(p_tu),
                                        test_clang_visitor,
                                        NULL);
    mu_assert("error: clang_visitChildren()", visitorstatus == 0);

cleanup:
    clang_disposeTranslationUnit(p_tu);
    clang_disposeIndex(p_idx);

    return p_message;
}
#endif // 0
static int has_symbol(uschshell_t *p_context, const char* p_sym) 
{
    int status = 0;
    int symbol_found = 0;
    void *p_handle = NULL;
    char *p_error = NULL;
    int (*dyn_func)();
    uschshell_lib_t *p_lib = NULL;

    p_lib = p_context->p_libs;
    
    FAIL_IF(p_lib == NULL);

    while (p_lib->p_next != NULL)
    {
        p_lib = p_lib->p_next;

        FAIL_IF(p_handle == NULL);

        *(void **) (&dyn_func) = dlsym(p_handle, p_sym);
        if ((p_error = dlerror()) != NULL)
            symbol_found = 1;
    }
end:
    (void)status;
    return symbol_found;
}
static enum CXChildVisitResult clang_visitor(
        CXCursor cursor, 
        CXCursor parent, 
        CXClientData p_client_data)
{
    (void)parent;
    enum CXChildVisitResult res = CXChildVisit_Recurse;
    CXString cxstr = {0};
    
    cxstr = clang_getCursorSpelling(cursor);
    uschshell_t *p_context = NULL;

    p_context = (uschshell_t*)p_client_data;

    switch (cursor.kind) 
    {
        case CXCursor_FunctionDecl:
            {
                int num_args = -1;
                int i;
                CXType return_type;
                cxstr = clang_getCursorSpelling(cursor);
#if 0
                if (has_symbol("/usr/lib/i386-linux-gnu/libm.so", clang_getCString(cxstr)))
#else
                    if (has_symbol(p_context, clang_getCString(cxstr)))

#endif // 0
                    {
                        CXString cxretkindstr;
                        return_type = clang_getCursorResultType(cursor);
                        cxretkindstr = clang_getTypeSpelling(return_type);

                        printf("%s %s(", clang_getCString(cxretkindstr), clang_getCString(cxstr));
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
                            printf("%s %s", clang_getCString(cxkindstr), clang_getCString(cxargstr));
                            if (i != (num_args - 1))
                                printf(",");
                            clang_disposeString(cxargstr);
                        }
                        printf(") {\n\treturn %s(", clang_getCString(cxstr));
                        for (i = 0; i < num_args; i++)
                        {
                            CXString cxargstr;
                            CXCursor argCursor;

                            argCursor = clang_Cursor_getArgument(cursor, i);
                            cxargstr = clang_getCursorSpelling(argCursor);
                            printf("%s", clang_getCString(cxargstr));
                            if (i != (num_args - 1))
                                printf(",");
                            clang_disposeString(cxargstr);
                        }
                        printf(");\n}\n");
                        clang_disposeString(cxretkindstr);
                    }
                    else
                    {
                        res = CXChildVisit_Continue;
                    }
                break;
            }
        default:
            {
                res = CXChildVisit_Continue;
                break;
            }
    }
    clang_disposeString(cxstr);
//end:
    return res;
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
    //FAIL_IF(visitorstatus == 0);
    (void)visitorstatus;
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
    FAIL_IF(p_context == NULL || p_header == NULL);

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
    FAIL_IF(!loadsyms_from_header_ok(p_context, p_tmpheader));
    // clang_getCursorDisplayName(CXCursor)

#if 0

    calloc(sizeof(p_lib) + strlen(p_header

                HASH_ADD_STR(p_libs, libname, p_lib);

end:
#else
status = -1;
#endif // 0
end:
    if (p_includefile)
        fclose(p_includefile);
    return status;
}


