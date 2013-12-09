#include <stdio.h>

#include "crepl_types.h"
#include "crepl_debug.h"

int crepl_define(crepl_t *p_context, size_t var_size, char *p_defname)
{
    int status = 0;
    if (p_context == NULL || p_defname == NULL)
        return -1;
    crepl_def_t *p_defs = p_context->p_defs;
    crepl_def_t *p_def = NULL;
    void *p_alloc_data = NULL;
    HASH_FIND_STR(p_defs, p_defname, p_def);

    FAIL_IF(p_def != NULL);

    p_def = calloc(sizeof(crepl_def_t) + strlen(p_defname) + 1, 1);
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
void crepl_undef(crepl_t *p_context, char *p_defname)
{
    if (p_context == NULL || p_defname == NULL)
        return;
    crepl_def_t *p_def = NULL;
    crepl_def_t *p_defs = p_context->p_defs;
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
int crepl_load(crepl_t *p_context, char *p_defname, void *p_data)
{
    int status = 0;
    if (p_context == NULL || p_defname == NULL)
        return -1;
    crepl_def_t *p_def = NULL;
    crepl_def_t *p_defs = p_context->p_defs;
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
size_t get_type_len(char *p_defname)
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

int crepl_store(crepl_t *p_context, char *p_defname, void *p_data)
{
    int status = 0;
    uint8_t tmp[USCHSHELL_DEFINE_SIZE] = {0};
    int is_updated = 0;
    if (p_context == NULL || p_defname == NULL)
        return -1;
    crepl_def_t *p_def = NULL;
    crepl_def_t *p_defs = p_context->p_defs;
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

int crepl_define_fn(struct crepl_t *p_context, char *p_fndefname, char *p_body)
{
    int status = 0;
    if (p_context == NULL || p_fndefname == NULL || p_body == NULL)
        return -1;
    crepl_def_t *p_defs = p_context->p_defs;
    crepl_def_t *p_def = NULL;
    void *p_body_data = NULL;
    HASH_FIND_STR(p_defs, p_fndefname, p_def);

    FAIL_IF(p_def != NULL);

    p_def = calloc(sizeof(crepl_def_t) + strlen(p_fndefname) + 1, 1);
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


