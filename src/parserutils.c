#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "parserutils.h"
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <ctype.h>
#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "usch_debug.h"

static size_t count_stars(char *p_input)
{
    size_t i = 0;
    size_t len = strlen(p_input);
    size_t num_stars = 0;

    while (i < len)
    {
        if (p_input[i] == '*')
        {
            num_stars++;
            i++;
        }
        i+=count_spaces(&p_input[i]);
        i++;
    }
    return num_stars;
}

static size_t count_identifier(char *p_input)
{
    size_t i = 0;
    if (isdigit(p_input[i]))
        return 0;
    while (p_input[i] != '\0' && isalnum(p_input[i]))
        i++;
    return i;
}


size_t get_type_len(char *p_defname)
{
    size_t i;
    size_t last_type_pos = 0;
    size_t deflen = strlen(p_defname);

    for (i = 0; i < deflen; i++)
    {
        if (p_defname[i] == ' ')
            last_type_pos = i;
        if (p_defname[i] == '*')
            last_type_pos = i;
    }
    return last_type_pos;
}


int count_spaces(char *p_input)
{
    size_t i = 0;
    while (p_input[i] != '\0' && (p_input[i] == ' '  || p_input[i] == '\t'))
    {
        i++;
    }
    return i;
}
int has_trailing_open_parenthesis(char *p_line)
{
    int len = (int)strlen(p_line);
    int i = len - 1;
    int unclosed = 0;

    while (p_line[i] != ')' && i >= 0)
    {
        if (p_line[i] == '(')
        {
            unclosed = 1;
            break;
        }
        i--;
    }
    return unclosed;
}
int has_trailing_closed_parenthesis(char *p_line)
{
    int len = (int)strlen(p_line);
    int i = len - 1;
    int closed = 0;

    while (p_line[i] != ')' && i >= 0)
    {
        i--;
    }
    while (i >= 0)
    {
        if (p_line[i] == '(')
        {
            closed = 1;
            break;
        }
        i--;
    }
    return closed;
}

int is_definition(char *p_input)
{
    size_t i = 0;
    size_t id1 = 0;
    size_t stars = 0;
    size_t id2 = 0;

    i += count_spaces(p_input); 

    id1 = count_identifier(&p_input[i]);
    i += id1;
    i += count_spaces(&p_input[i]); 
    stars = count_stars(&p_input[i]);
    i += stars;
    i += count_spaces(&p_input[i]); 
    id2 = count_identifier(&p_input[i]);

    if (id1 > 0 && id2 > 0)
        return 1;

    return 0;
}

int get_identifiers(const char *p_line_in, int *p_count, char ***ppp_identifiers_out)
{
    int status = 0;
    char **pp_identifiers = NULL;
    int num_identifiers = 0;
    int i = 0;
    char *p_line = NULL;
    int len = (int)strlen(p_line_in);
    char last = '\0';
    int parsed_identifier = 0;
    int has_looped = 0;

    p_line = strdup(p_line_in);
    FAIL_IF(p_line == NULL);

    while (pp_identifiers == NULL)
    {
        if (has_looped)
        {
            if (num_identifiers == 0)
            {
                break;
            }
            pp_identifiers = calloc((num_identifiers + 1)* sizeof(char*) + len * sizeof(char) + 1, 1);
            FAIL_IF(pp_identifiers == NULL);
            strcpy((char*)&pp_identifiers[num_identifiers+1], p_line_in);
            // simplify array iteration by null termination
            pp_identifiers[num_identifiers] = NULL;
            free(p_line);
            p_line = (char*)&pp_identifiers[num_identifiers+1];
            i = 0;
            last = '\0';
            num_identifiers = 0;
        }
        while (i < len)
        {
            if (i > 0)
                last = p_line[i-1];

            if (isalpha(p_line[i]) && !isdigit(last))
            {
                if (pp_identifiers != NULL)
                {
                    pp_identifiers[num_identifiers] = &p_line[i];
                }
                num_identifiers++;
                i++;
                while (isalnum(p_line[i])|| p_line[i] == '_')
                {
                    i++;
                }
                parsed_identifier = 1;
            }
            else if (isdigit(p_line[i]))
            {
                while (isalnum(p_line[i]) || p_line[i] == '_')
                {
                    i++;
                }
                parsed_identifier = 0;
            }
            else if (p_line[i] == '-' && p_line[i+1] == '>' && parsed_identifier)
            {
                i+=2;
                while(isalnum(p_line[i]))
                {
                    i++;
                }
                parsed_identifier = 0;
            }
            else if (p_line[i] == '.' && parsed_identifier)
            {
                i+=2;
                while(isalnum(p_line[i]))
                {
                    i++;
                }
                parsed_identifier = 0;
            }
            else if (p_line[i] == '\'')
            {
                i++;
                while (p_line[i] != '\'' && p_line[i] != '\0')
                {
                    i++;
                }
            }
            else if (p_line[i] == '\"')
            {
                i++;
                while (p_line[i] != '\"' && p_line[i] != '\0')
                {
                    i++;
                }
            }
            else
            {
                i++;
            }
        }

        has_looped = 1;
    }
    for (i = 0; i < len; i++)
    {
        if (!isalnum(p_line[i]) && p_line[i] != '_')
            p_line[i] = '\0';
    }
    *ppp_identifiers_out = pp_identifiers;
    p_line = NULL;
    *p_count = num_identifiers;
end:
    free(p_line);
    return status;
}
int identifier_pos(char *p_line)
{
    char prevchar = '\0';
    int i = 0;
    size_t len = 0;
    len = strlen(p_line);
    if (len == 0)
        return 0;
    i = (int)len - 1;

    do
    {
        if(!isalnum(p_line[i]))
        {
            break;
        }
        prevchar = p_line[i];
        i--;
    } while(i != -1);

    if (isalpha(prevchar))
    {
        return (int)i + 1;
    }
    else
    {
        return len;
    }
}

char* stripwhite(char *string)
{
   char *p_s, *p_t;

   for (p_s = string; isspace(*p_s); p_s++)
   {
      *p_s = '\0';
   }

   if (*p_s == 0)
      return p_s;

   p_t = p_s + strlen(p_s) - 1;
   while (p_t > p_s && isspace(*p_t))
   {
       *p_t = '\0';
       p_t--;
   }

   return p_s;
}

//static 
int iscmd(char *p_cmd)
{
    struct stat sb;
    char *p_item = NULL;
    char *p_tmp = NULL;
    char path[PATH_MAX] = {0};
    char *std = NULL;
    std = getenv("PATH");
    int found = 0;

    for (p_item = std;; *p_item++ = ':')
    {
        p_tmp = p_item;
        if ((p_item = strchr(p_item, ':')) != NULL) {
            *p_item = '\0';
            if (p_tmp == p_item)
            {
                p_tmp = ".";
            }
        }
        else
            if (strlen(p_tmp) == 0)
            {
                p_tmp = ".";
            }

        (void)snprintf(path, sizeof(path), "%s/%s", p_tmp, p_cmd);
        if (!stat(path, &sb))
        {
            found = 1;
            break;
        }
        if (p_item == NULL)
        {
            break;
        }
    }
    return found;
}


char *get_symname(char *p_defname)
{
    char *p_tmp = p_defname;
    char *p_symname = p_defname;
    while (*p_tmp != '\0')
    {
        if (*p_tmp == ' ' || *p_tmp == '*')
        {
            p_symname = p_tmp;
        }
        p_tmp++;
    }
    return p_symname + 1;
}

int parse_line(char *p_input, usch_def_t *p_definition)
{
    //usch_def_t definition;
    int status = 1;
    size_t i = 0;
    size_t length;
    char *p_symname = NULL;

    length = strlen(p_input);
    p_symname = calloc(length, 1);

    FAIL_IF(p_symname == NULL);

    while (p_input[i] == ' ' || p_input[i] == '\t')
        i++;



    for (; i < length; i++)
    {
        char c = p_input[i];
        switch (p_input[i])
        {
            case '\t':
            case ' ':
                {
                    break;
                }
            case '(':
                {

                    break;
                }
            default:
                {
                    if (isalnum(c) != c)
                    {
                        p_symname[i] = c;
                    }
                    else
                    {
                        assert(0);
                    }
                    break;
                }
        }
    }
    p_definition->p_symname = p_symname;
end:
    return status;
}

int has_trailing_identifier(char *p_line, char **pp_identifiers)
{
    int i;
    int fetch_id;
    int trailing_identifier;
    size_t len = strlen(p_line);
    size_t identifier_len;

    for (i = 0; pp_identifiers[i] != NULL; i++)
        ;
    fetch_id = i - 1;
    identifier_len = strlen(pp_identifiers[fetch_id]);
    if (strcmp(pp_identifiers[fetch_id], &p_line[len - identifier_len]) == 0)
    {
        trailing_identifier = 1;
    }
    else
    {
        trailing_identifier = 0;
    }

    return trailing_identifier;
}

