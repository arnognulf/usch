#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <ctype.h>
#include "../external/uthash/src/uthash.h"
#include "../usch_h/usch.h"
#include "crepl.h"
#include <clang-c/Index.h>

#define CREPL_STMT_C_FN       "stmt.c"
#define CREPL_DEFS_H_FN       "defs.h"
#define CREPL_INCS_H_FN       "incs.h"
#define CREPL_TRAMPOLINES_H_FN "trampolines.h"
#define CREPL_DYN_FUNCNAME "crepl_eval_stmt"
#define CREPL_DEFINE_SIZE 8
#define CREPL_INDENT "    "


#ifndef CMAKE_INSTALL_PREFIX
#define CMAKE_INSTALL_PREFIX "/usr/local"
//#warning hardcoding CMAKE_INSTALL_PREFIX
#endif


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

typedef struct crepl
{
    ustash stash;
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
    crepl_options options;
    char *p_tmpdir;
    char *p_stmt_c;
    char *p_defs_h;
    char *p_incs_h;
    char *p_trampolines_h;
    char *p_stmt_header;
    // clang data
    CXIndex p_idx;
    CXTranslationUnit p_tu;
    CXCodeCompleteResults *p_completion_results;
} crepl;

struct usch_ids
{
    struct usch_ids *p_usch_ids;
    char name[];
};

typedef struct 
{
    char *p_cur_id;
    int found_cur_id;
    struct crepl *p_context;
} preparse_userdata_t;

#ifdef __cplusplus
}
#endif // __cplusplus

