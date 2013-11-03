#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "parserutils.h"
#include <stdio.h>
int identifier_pos(char *p_line)
{
    char prevchar = '\0';
    size_t i = 0;
    size_t len = 0;
    len = strlen(p_line);
    if (len == 0)
        return 0;
    i = len - 1;

    do
    {
        if(!isalnum(p_line[i]))
        {
            break;
        }
        prevchar = p_line[i];
        i--;
    } while(i != 0);

    if (isalpha(prevchar))
    {
        return (int)i+1;
    }
    else
    {
        return -1;
    }
}

