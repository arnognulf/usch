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
size_t get_type_len(char *p_defname);
int parse_line(char *p_input, usch_def_t *p_definition);
int has_trailing_identifier(char *p_line, char **pp_identifiers);
int has_unclosed_parenthesis(char *p_line);
int is_definition(char *p_input);
int has_trailing_closed_parenthesis(char *p_line);
int has_trailing_open_parenthesis(char *p_line);
int count_spaces(char *p_input);

#endif // PARSERUTILS_H
