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
        //printf("ZZZ: trying: %s\n", path);
        if (!stat(path, &sb))
        {
            //printf("XXX: found: %s\n", path);
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


