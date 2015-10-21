#ifndef PARSERUTILS_H
#define PARSERUTILS_H
#include <stdlib.h>
typedef struct
{
    char *p_symname;
} usch_def_t;

int get_identifiers(char *p_line, int *p_count, char ***ppp_identifiers_out);
int identifier_pos(char *p_line);
char* stripwhite(char *p_line);
int parse_line(char *p_input, usch_def_t *p_definition);
char* crepl_parent_identifier(char *p_str);
int is_definition(char *p_input);
int count_spaces(char *p_input);

#endif // PARSERUTILS_H
