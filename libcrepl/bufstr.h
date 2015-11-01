#include <unistd.h>

typedef struct {
    char *p_str;
    size_t len;
} bufstr_t;

// deprecated function, use ustrjoin instead
int bufstradd(bufstr_t *p_bufstr, const char *p_addstr);

