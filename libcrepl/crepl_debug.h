#ifndef CREPL_DEBUG_H
#define CREPL_DEBUG_H
#define FAIL_IF(x) if(x) { status=-1;fprintf(stderr, "FAIL at %s:%d\n", __FILE__, __LINE__); goto end;}
#define QUIET_FAIL_IF(x) if(x) { status=-1; goto end;}
#define ENDOK_IF(x) { if(x) { goto end;}}
#endif // CREPL_DEBUG_H
