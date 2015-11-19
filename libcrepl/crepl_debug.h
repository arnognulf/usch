#ifndef CREPL_DEBUG_H
#define CREPL_DEBUG_H
#include "crepl.h"
#include <assert.h>

#define FAIL_IF(x) if(x) { status=-1;fprintf(stderr, "FAIL at %s:%d\n", __FILE__, __LINE__); goto end;}
#define QUIET_FAIL_IF(x) if(x) { status=-1; goto end;}
#define ENDOK_IF(x) { if(x) { goto end;}}

#define E_FAIL_IF(x) if(x) { assert(0); estatus=E_CREPL_WAT;fprintf(stderr, "E_FAIL at %s:%d\n", __FILE__, __LINE__); goto end;}
#define E_QUIET_FAIL_IF(x) if(x) { estatus=E_CREPL_WAT; goto end;}



#endif // CREPL_DEBUG_H

