#include <ctype.h>
#include "../external/uthash/src/uthash.h"
#include "usch.h"
#include "crepl.h"
#define CREPL_DYN_FUNCNAME "usch_dyn_func"
#define CREPL_DEFINE_SIZE 8
#define CREPL_CC_FALLBACK "gcc"
#define CREPL_CXX_FALLBACK "g++"

typedef struct crepl_def_t
{
    UT_hash_handle hh;
    size_t size;
    void *p_body_data;
    void *p_alloc_data;
    uint8_t data[CREPL_DEFINE_SIZE];
    char defname[];
} crepl_def_t;

typedef struct crepl_cmd_t
{
    UT_hash_handle hh;
    char cmdname[];
} crepl_cmd_t;

typedef struct crepl_lib_t
{
    struct crepl_lib_t *p_next;
    void *p_handle;
    char libname[];
} crepl_lib_t;

typedef struct crepl_sym_t
{
    UT_hash_handle hh;
    void *p_handle;
    char symname[];
} crepl_sym_t;

typedef struct crepl_dyfn_t
{
    UT_hash_handle hh;
    void *p_handle;
    char *p_dyfndef;
    char dyfnname[];
} crepl_dyfn_t;

typedef struct crepl_inc_t
{
    UT_hash_handle hh;
    char incname[];
} crepl_inc_t;

typedef struct crepl_t 
{
    char **pp_ldpath;
    char **pp_cmds;
    char *p_nodef_line;
    char *p_defs_line;
    crepl_def_t *p_defs;
    crepl_lib_t *p_libs;
    crepl_sym_t *p_syms;
    crepl_dyfn_t *p_dyfns;
    crepl_inc_t *p_incs;
    char *p_prompt;
    ustash prompt_stash;
    char is_initialized;
    char source_ext[4]; // c || cpp
    char header_ext[4]; // h || hpp
    crepl_options options;
    char tmpdir[];
} crepl_t;

struct usch_ids
{
    struct usch_ids *p_usch_ids;
    char name[];
};

typedef struct 
{
    char *p_cur_id;
    int found_cur_id;
    struct crepl_t *p_context;
} preparse_userdata_t;


