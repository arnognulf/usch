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
#include <stdint.h>
#define KEY_TAB 9
#define KEY_BACKSPACE 127
#define KEY_NEWLINE '\n'
#define KEY_SPACE ' '
#define ASCII_ESC '\033'
#define PMCURSES_GETCH_BUFSZ 5
typedef struct pmcurses_t pmcurses_t;
void pmcurses_move(int col, int row);
char pmcurses_getch(char *p_buf);
void pmcurses_backspace(int num);

int pmcurses_create(pmcurses_t **pp_pmcurses);
void pmcurses_destroy(pmcurses_t *p_pmcurses);
int pmcurses_insert(pmcurses_t *p_pmcurses, const char *p_token);
int pmcurses_erase(pmcurses_t *p_pmcurses);
int pmcurses_back(pmcurses_t *p_pmcurses);
int pmcurses_forward(pmcurses_t *p_pmcurses);
int pmcurses_home(pmcurses_t *p_pmcurses);
int pmcurses_end(pmcurses_t *p_pmcurses);
int pmcurses_draw(pmcurses_t *p_pmcurses, int height, int width);
char *pmcurses_gettok(pmcurses_t *p_pmcurses, size_t index);
size_t pmcurses_linelen(pmcurses_t *p_pmcurses);
int pmcurses_writeline(pmcurses_t *p_pmcurses, char *p_line);

