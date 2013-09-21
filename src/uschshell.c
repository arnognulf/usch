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
    size_t i, last_type_pos;
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

typedef struct uschshell_t 
{
    uschshell_def_t *p_defs;
    uschshell_cmd_t *p_cmds;
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
        printf("$%s = %d\n", get_symname(p_defname), *(unsigned int*)p_data);
    if (strncmp(p_defname, "char", type_len) == 0)
        printf("$%s = %c\n", get_symname(p_defname), *(char*)p_data);
    else if (strncmp(p_defname, "int", type_len) == 0)
        printf("$%s = %d\n", get_symname(p_defname), *(int*)p_data);
    else if (strncmp(p_defname, "double", type_len) == 0)
        printf("$%s = %f\n", get_symname(p_defname), *(double*)p_data);
    else if (strncmp(p_defname, "float", type_len) == 0)
        printf("$%s = %f\n", get_symname(p_defname), *(float*)p_data);
    else
        printf("$%s = 0x%lx\n", get_symname(p_defname), *(uint64_t*)p_data);
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

    p_context = calloc(sizeof(uschshell_t), 1);
    FAIL_IF(p_context == NULL);

    p_def = calloc(sizeof(uschshell_def_t) + 1, 1);
    FAIL_IF(p_def == NULL);

    p_cmd = calloc(sizeof(uschshell_cmd_t) + 1, 1);
    FAIL_IF(p_cmd == NULL);

    HASH_ADD_STR(p_context->p_defs, defname, p_def);
    HASH_ADD_STR(p_context->p_cmds, cmdname, p_cmd);

    *pp_context = p_context;
    p_context = NULL;
    p_cmd = NULL;
    p_def = NULL;
end:
    free(p_cmd);
    free(p_def);
    free(p_context);
    return status;
}
void uschshell_destroy(uschshell_t *p_context)
{
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
#if 0 
static int is_end_of_pre_assign(char *p_input)
{
    size_t i = 0;
    printf("\npre_assign: XX%sYY\n", p_input);
    while (p_input[i] != '\0')
    {

        if (p_input[i] == '=')
            return 1;
        if (p_input[i] != ' ' || p_input[i] != '\t')
            return 0;
        i++;
    }
    return 0;
}
#endif // 0
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
    int i;

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
    int i;

    if (p_context == NULL || p_item == NULL)
        return -1;
    if (is_c_keyword(p_item))
    {
        return 0;
    }
    if (is_reserved_word(p_item))
    {
        return 0;
    }
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

static int is_definition(char *p_input)
{
    size_t i = 0;
    size_t words = 0;
    size_t unfinished_words = 0;

    i += count_spaces(p_input); 

    while (p_input[i] != '\0')
    {
        if (p_input[i] == '=')
            break;
        if (p_input[i] == '(')
            return 0;
        if (p_input[i] == ' ' || p_input[i] == '\t')
        {
            unfinished_words = 0;
            words++;
            i += count_spaces(&p_input[i]);
        }
        else
        {
            unfinished_words = 1;
            i++;
        }
    }
    words = words + unfinished_words;
    if (words > 1)
        return 1;
    else
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
    char dir_template[] = "/tmp/usch-XXXXXX";
    char *p_pre_assign = NULL;
    char *p_post_assign = NULL;

    if (p_context == NULL || p_input == NULL)
        return -1;

    FAIL_IF(usch_strsplit(getenv("PATH"), ":", &pp_path) < 0);
    p_tempdir = mkdtemp(dir_template);
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

    if (p_stmt_c != NULL)
        fclose(p_stmt_c);

    return status;
}

