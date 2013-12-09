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
#include "bufstr.h"
#include <string.h>
#include <unistd.h>
#include "crepl_debug.h"
#include "malloc.h"
#define MAX(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

int bufstradd(bufstr_t *p_bufstr, const char *p_addstr)
{
    int status = 0;
    char *p_newstr = NULL;
    size_t cur_len = 0;
    size_t add_len = 0;
    
    if (p_bufstr == NULL || p_addstr == NULL)
        return -1;
    if (p_bufstr->len == 0)
        return -1;
    add_len = strlen(p_addstr);
    cur_len = strlen(p_bufstr->p_str);
    if ((add_len + cur_len) > (p_bufstr->len - 1))
    {
        size_t new_size = MAX(p_bufstr->len * 2, (add_len + cur_len + 1) * 2);
        p_newstr = calloc(new_size, 1);
        FAIL_IF(p_newstr == NULL);
        strcpy(p_newstr, p_bufstr->p_str);
        free(p_bufstr->p_str);

        p_bufstr->len = new_size;
        p_bufstr->p_str = p_newstr;
        p_newstr = NULL;
    }
    strcpy(&p_bufstr->p_str[cur_len], p_addstr);
end:
    free(p_newstr);
    return status;
}

