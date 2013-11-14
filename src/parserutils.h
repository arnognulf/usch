#ifndef PARSERUTILS_H
#define PARSERUTILS_H
#include <stdlib.h>

int get_identifiers(const char *p_line, int *p_count, char ***ppp_identifiers_out);
int iscmd(char *p_cmd);
int identifier_pos(char *p_line);
char* stripwhite(char *p_line);
#endif // PARSERUTILS_H
