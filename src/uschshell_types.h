#include <ctype.h>
#include "../external/uthash/src/uthash.h"

#define USCHSHELL_DYN_FUNCNAME "usch_dyn_func"
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

typedef struct uschshell_dyfn_t
{
    UT_hash_handle hh;
    void *p_handle;
    char *p_dyfndef;
    char dyfnname[];
} uschshell_dyfn_t;

typedef struct uschshell_inc_t
{
    UT_hash_handle hh;
    char incname[];
} uschshell_inc_t;

typedef struct uschshell_t 
{
    uschshell_def_t *p_defs;
    uschshell_lib_t *p_libs;
    uschshell_sym_t *p_syms;
    uschshell_dyfn_t *p_dyfns;
    uschshell_inc_t *p_incs;
    char tmpdir[];
} uschshell_t;

struct usch_ids
{
    struct usch_ids *p_usch_ids;
    char name[];
};

typedef struct 
{
char *p_cur_id;
int found_cur_id;
} preparse_userdata_t;


