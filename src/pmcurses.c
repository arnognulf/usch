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
#include "pmcurses.h"

// poor-man's curses 

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


