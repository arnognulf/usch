#ifndef PARSERUTILS_H
#define PARSERUTILS_H
#include <stdlib.h>

int iscmd(char *p_cmd);
int identifier_pos(char *p_line);
char* stripwhite(char *p_line);
#endif // PARSERUTILS_H
