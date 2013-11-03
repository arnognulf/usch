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


