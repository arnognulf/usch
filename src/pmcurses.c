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
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "pmcurses.h"
#include "malloc.h"
#include "usch_debug.h"
// poor-man's curses 
#define MAX(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
#define MIN(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })


struct  tokstr {
    char *p_str;
    size_t len;
};

static size_t toklen(const char *p_str, const size_t maxlen)
{
    size_t i;
    size_t len = 0;

    for (i = 0; i < (maxlen - 1); i++)
    {
        if (p_str[i] == '\0' && p_str[i + 1] == '\0')
        {
            break;
        }

        len++;
    }
    return len;
}
static size_t declen(const char *p_str)
{
    size_t i = 0;
    size_t pos = 0;
    if (p_str[pos] == '\033')
    {
        pos++;
        if (p_str[pos] == '[')
        {
            pos++;
            while (p_str[i] != '\0')
            {
                if(isdigit(p_str[pos]) || p_str[pos] == ';')
                {
                    if (i == 0)
                        i = 2;
                    i++;
                    pos++;
                }
                else
                {
                    if (p_str[pos] == 'm')
                    {
                        i++;
                        break;
                    }
                    else
                    {
                        i = 0;
                        break;
                    }
                }

            }
        }
    }
    return i;
}
static size_t utflen(const char *p_str)
{
    // http://zaemis.blogspot.se/2011/06/reading-unicode-utf-8-by-hand-in-c.html
    /* mask values for bit pattern of first byte in 
     * multi-byte UTF-8 sequences: 
     *   192 - 110xxxxx - for U+0080 to U+07FF 
     *   224 - 1110xxxx - for U+0800 to U+FFFF 
     *   240 - 11110xxx - for U+010000 to U+1FFFFF */
    static unsigned short mask[] = {192, 224, 240};
    
    size_t i = 1;
    if ((p_str[0] & mask[0]) == mask[0]) i++;
    if ((p_str[0] & mask[1]) == mask[1]) i++;
    if ((p_str[0] & mask[2]) == mask[2]) i++;
    return i;
}
size_t termprintlen(const char *p_str, const size_t maxlen)
{
    size_t i;
    size_t len = 0;

    for (i = 0; i < (maxlen - 1); i++)
    {
        if (p_str[i] == '\0' && p_str[i + 1] == '\0')
        {
            break;
        }

        if (p_str[i] == '\0')
            continue;
        len += utflen(&p_str[i]);
        len += declen(&p_str[i]);
    }
    return len;
}
static size_t restlen(const char *p_str, size_t pos, size_t len)
{
    size_t rest_len = 0;
    while (pos < len - 1)
    {
        if (p_str[pos] == '\0' && p_str[pos+1] == '\0')
            break;
        rest_len++;
        pos++;
    }
    return rest_len;
}

static int tokstrinsert(struct tokstr *p_tokstr, size_t pos, const char *p_addstr)
{
    int status = 0;
    char *p_newstr = NULL;
    size_t cur_len = 0;
    size_t add_len = 0;
    
    if (p_tokstr == NULL || p_tokstr == NULL)
        return -1;
    if (p_tokstr->len == 0)
        return -1;
    add_len = strlen(p_addstr);
    cur_len = toklen(p_tokstr->p_str, p_tokstr->len);
    if (p_tokstr->p_str[0] != '\0')
        cur_len++;
    if ((add_len + cur_len) > (p_tokstr->len - 2))
    {
        size_t new_size = MAX(p_tokstr->len * 2, (add_len + cur_len) * 2);
        p_newstr = calloc(new_size, 1);
        FAIL_IF(p_newstr == NULL);
        memcpy(p_newstr, p_tokstr->p_str, p_tokstr->len);
        free(p_tokstr->p_str);

        p_tokstr->len = new_size;
        p_tokstr->p_str = p_newstr;
        p_newstr = NULL;
    }
    if (p_tokstr->p_str[pos] == '\0' && p_tokstr->p_str[pos+1] == '\0')
    {
        memcpy(&p_tokstr->p_str[cur_len], p_addstr, add_len);
        p_tokstr->p_str[cur_len+add_len+1] = '\0';
        p_tokstr->p_str[cur_len+add_len+2] = '\0';
    }
    else
    {
        size_t rest_len = restlen(p_tokstr->p_str, pos, p_tokstr->len);
        memmove(&p_tokstr->p_str[pos + add_len + 1], &p_tokstr->p_str[pos+1], rest_len);
        memcpy(&p_tokstr->p_str[pos], p_addstr, add_len);
    }
#if 0
    fputc(p_tokstr->p_str[0], stdout);
    for(i = 1; i < p_tokstr->len; i++)
    {
        if (p_tokstr->p_str[i] == '\0')
        {
            if (p_tokstr->p_str[i-1] != '\0')
                printf(" NUL ");
        }
        else
        {
            fputc(p_tokstr->p_str[i], stdout);
        }
    }
#endif // 0

end:
    free(p_newstr);
    return status;
}

void pmcurses_backspace(int num)
{
    int i;
    printf("\033[%dD", num);
    for (i = 0; i < num; i++)
    {
        printf(" ");
    }
    printf("\033[%dD", num);
    fflush(stdout);
}
enum pmcurses_key pmcurses_parsekey(char *p_key)
{
    enum pmcurses_key key = PMCURSES_PRINTABLE;
    if (p_key[0] == '\033')
    {
        if(p_key[1] == '[')
            switch (p_key[2])
            {
                case 'A':
                    key = PMCURSES_UP;
                    break;
                case 'D':
                    key = PMCURSES_BACK;
                    break;
                case 'C':
                    key = PMCURSES_FORWARD;
                    break;
                case 'B':
                    key = PMCURSES_DOWN;
                    break;
                default:
                    break;
            }
        if(p_key[1] == 'O')
            switch (p_key[2])
            {
                case 'H':
                    key = PMCURSES_HOME;
                    break;
                case 'F':
                    key = PMCURSES_HOME;
                    break;
                default:
                    break;
            }
        // emacs
        if(p_key[1] == 'A')
            key = PMCURSES_HOME;
        if(p_key[1] == 'E')
            key = PMCURSES_END;
        if (p_key[1] == 'H')
            key = PMCURSES_BACKSPACE;
        if (p_key[1] == 'I')
            key = PMCURSES_TAB;
    }
    if (p_key[0] == 127)
        key = PMCURSES_BACKSPACE;
    return key;

}

char pmcurses_getch(char *p_buf) {
    if (p_buf == 0)
        return EOF;
    // http://zaemis.blogspot.se/2011/06/reading-unicode-utf-8-by-hand-in-c.html
    /* mask values for bit pattern of first byte in 
     * multi-byte UTF-8 sequences: 
     *   192 - 110xxxxx - for U+0080 to U+07FF 
     *   224 - 1110xxxx - for U+0800 to U+FFFF 
     *   240 - 11110xxx - for U+010000 to U+1FFFFF */
    static unsigned short mask[] = {192, 224, 240};
    struct termios old;
    char return_char = '?';
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, p_buf, 1) < 0)
        perror ("read()");
    return_char = p_buf[0];
    if (p_buf[0] == ASCII_ESC)
    {
        if (read(0, &p_buf[1], 1) < 0)
        {
            perror ("read()");
        }
        if (p_buf[1] == '[')
        {
            if (read(0, &p_buf[2], 1) < 0)
            {
                perror ("read()");
            }
            // HOME = ASCII_ESC [OH
            // END  = ASCII_ESC [OF
            if (p_buf[2] == 'O')
            {
                if (read(0, &p_buf[3], 1) < 0)
                {
                    perror ("read()");
                }
            }
        }
    }
    else
    {
        int i = 0;
        if ((p_buf[0] & mask[0]) == mask[0]) i++;
        if ((p_buf[0] & mask[1]) == mask[1]) i++;
        if ((p_buf[0] & mask[2]) == mask[2]) i++;
        if (i)
        {
            if (read(0, &p_buf[1], i) < 0)
            {
                perror ("read()");
            }
            return_char = '?';
        }
    }

    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror ("tcsetattr ~ICANON");
    return return_char;
}
struct pmcurses_t
{
    size_t bufpos;
    size_t tokpos;
    size_t oldwidth;
    size_t oldheight;
    size_t curhpos;
    size_t curwpos;

    struct tokstr stritems;
};

int pmcurses_create(pmcurses_t **pp_pmcurses)
{
    int status = 0;
    char *p_buf = NULL;
    pmcurses_t *p_pmcurses = NULL;

    p_buf = calloc(1024, 1);
    FAIL_IF(p_buf == NULL);

    p_pmcurses = calloc(sizeof(pmcurses_t), 1);
    FAIL_IF(p_pmcurses == NULL);
    p_pmcurses->bufpos = 0;
    p_pmcurses->tokpos = 0;
    p_pmcurses->stritems.p_str = p_buf;
    p_pmcurses->stritems.len = 1024;
    p_buf = NULL;
    *pp_pmcurses = p_pmcurses;
    p_pmcurses = NULL;

end:
    free(p_buf);
    free(p_pmcurses);
    return status;
}

void pmcurses_destroy(pmcurses_t *p_pmcurses)
{
    free(p_pmcurses);
}
int pmcurses_insert(pmcurses_t *p_pmcurses, const char *p_token)
{
    int status = 0;

    // one NUL separates tokens, two NUL end array
    FAIL_IF(tokstrinsert(&p_pmcurses->stritems, p_pmcurses->bufpos, p_token));
    p_pmcurses->bufpos += (strlen(p_token) + 1);
end:
    return status;
}

int pmcurses_erase(pmcurses_t *p_pmcurses)
{
    (void)p_pmcurses;
    return -1;
}
int pmcurses_back(pmcurses_t *p_pmcurses)
{
    size_t bufpos;
    bufpos = p_pmcurses->bufpos;

    printf("%s\n", &p_pmcurses->stritems.p_str[bufpos]);
    if (p_pmcurses->stritems.p_str[bufpos] == '\0' && bufpos > 0)
    {
        bufpos--;
        printf("%s\n", &p_pmcurses->stritems.p_str[bufpos]);
    }
    while (p_pmcurses->stritems.p_str[bufpos] != '\0' && bufpos > 0)
    {
        printf("%s\n", &p_pmcurses->stritems.p_str[bufpos]);
        bufpos--;
    }

    p_pmcurses->bufpos = bufpos;
    p_pmcurses->tokpos--;

    return 1;
}
int pmcurses_forward(pmcurses_t *p_pmcurses)
{
    (void)p_pmcurses;
    return -1;
}
int pmcurses_home(pmcurses_t *p_pmcurses)
{
    (void)p_pmcurses;
    return -1;
}
int pmcurses_end(pmcurses_t *p_pmcurses)
{
    (void)p_pmcurses;
    return -1;
}
char *pmcurses_gettok(pmcurses_t *p_pmcurses, size_t index)
{
    size_t i;
    size_t pos = 0;
    char *p_str = NULL;
    for (i = 0; i < (p_pmcurses->stritems.len - 1); i++)
    {
        if (p_pmcurses->stritems.p_str[i] == '\0' && p_pmcurses->stritems.p_str[i+1] == '\0')
        {
            p_str = NULL;
            break;
        }
        if (p_pmcurses->stritems.p_str[i] == '\0')
        {
            pos++;
            continue;
        }
        if (pos == index)
        {
            p_str = &p_pmcurses->stritems.p_str[i];
            break;
        }
    }

    return p_str;
}
char *pmcurses_getcurtok(pmcurses_t *p_pmcurses)
{
    char *p_str = &p_pmcurses->stritems.p_str[p_pmcurses->bufpos];

    return p_str;
}
size_t pmcurses_linelen(pmcurses_t *p_pmcurses)
{
    return toklen(p_pmcurses->stritems.p_str, p_pmcurses->stritems.len);
}

int pmcurses_writeline(pmcurses_t *p_pmcurses, char *p_line)
{
    int status = 0;
    size_t i;
    size_t pos = 0;

    for (i = 0; i < (p_pmcurses->stritems.len - 1); i++)
    {
        if (p_pmcurses->stritems.p_str[i] == '\0' && p_pmcurses->stritems.p_str[i+1] == '\0')
        {
            break;
        }
        if (p_pmcurses->stritems.p_str[i] == '\0')
        {
            continue;
        }
        strcpy(&p_line[pos], &p_pmcurses->stritems.p_str[i]);
        pos++;
    }

    return status;
}
static int print_char(const char *p_str, size_t *p_readlen, size_t *p_displaylen)
{
    size_t i = 0;
    size_t readlen = 0;
    readlen = utflen(p_str);
    for (i = 0; i < readlen; i++)
    {
        fputc(p_str[i], stdout);
    }
    *p_displaylen = 1; // TODO: handle double-width here!
    *p_readlen = readlen;
    return 1;
}

int pmcurses_draw(pmcurses_t *p_pmcurses, int width, int height)
{
    // position cursorpos while drawpos < bufpos
    // get lastpos by subtract cursorpos from drawpos
    // position cursor! WIN
    int status = 0;
    size_t drawpos = 0;
    size_t displaylen = 0;
    size_t readlen = 0;
    size_t bufsize;
    size_t bufpos;
    size_t hpos = 0;
    size_t wpos = 0;
    size_t curhpos = 0;
    size_t curwpos = 0;

    bufpos = p_pmcurses->bufpos;
    if (p_pmcurses->curwpos > 0)
        printf("\033[%dD", (int)p_pmcurses->curwpos); // BACK
    if (p_pmcurses->curhpos > 0)
        printf("\033[%dA", (int)p_pmcurses->curhpos); // UP 


    if (p_pmcurses->oldheight != (size_t)height || p_pmcurses->oldwidth != (size_t)width)
    {
        p_pmcurses->oldheight = (size_t)height;
        p_pmcurses->oldwidth = (size_t)width;
    }
    bufsize = p_pmcurses->stritems.len;
    while (drawpos < (bufsize - 2))
    {
        if (p_pmcurses->stritems.p_str[drawpos] == '\0' && p_pmcurses->stritems.p_str[drawpos+1] == '\0')
            break;
        if (p_pmcurses->stritems.p_str[drawpos] == '\0')
        {
            drawpos++;
            continue;
        }
        FAIL_IF(!print_char(&p_pmcurses->stritems.p_str[drawpos], &readlen, &displaylen));
        drawpos += readlen;
        if (drawpos < bufpos)
        {
            curhpos = hpos;
            curwpos = wpos;
        }
        wpos += displaylen;
        if (wpos == (size_t)width)
        {
            wpos = 0;
            hpos++;
            curhpos++;
            curwpos = 0;
            printf("\n");
        }
        // note that double-width chars are not split up but instead put on the new line
    }
    
    if (((int)wpos - (int)curwpos - 1) > 0)
        printf("\033[%dD", (int)wpos - (int)curwpos - 1); // BACK
    if ((hpos - curhpos) > 0)
        printf("\033[%dA", (int)hpos - (int)curhpos); // UP 
    p_pmcurses->curwpos = MAX(0, (int)wpos);
        p_pmcurses->curhpos = MAX(0, (int)hpos);

    fflush(stdout);
end: 

    return status;
}

int pmcurses_delete(pmcurses_t *p_pmcurses)
{
    char *p_str = NULL;
    size_t pos;

    p_str = p_pmcurses->stritems.p_str;
    pos = p_pmcurses->bufpos;
    printf("wwooo: %s\n", &p_str[pos]);
    if (p_str[pos] != '\0')
    {
        size_t rest_len = restlen(p_str, pos, p_pmcurses->stritems.len);
        while (p_str[pos + 1] != '\0')
        {
            pos++;
        }
        printf("wwooo2: %s\n", &p_str[pos]);
        if (p_str[pos + 2] == '\0')
        {
            p_str[p_pmcurses->bufpos+1] = '\0';
            p_str[p_pmcurses->bufpos+2] = '\0';
        }
        else
        {
            memmove(&p_str[p_pmcurses->bufpos], &p_str[pos+2], rest_len-2);
        }
    }
    return 1;
}
int pmcurses_backspaceex(pmcurses_t *p_pmcurses)
{
    if (p_pmcurses->bufpos > 0)
    {
        pmcurses_back(p_pmcurses);
        pmcurses_delete(p_pmcurses);
    }
    return 1;
}

