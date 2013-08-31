#define BUFSIZE 8192
#define STATUS_OK 0
#define STATUS_FILE_FAILED 0
#define USCH_TRUE 0
#define USCH_FALSE 1

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <editline/readline.h>
#include <editline/history.h>

int is_declaration(char* pStr)
{
    int i = 0;
    int value = USCH_TRUE;
    int has_space = USCH_FALSE;

    // remove leading whitespace
    while (pStr[i] != '\t' && pStr[i] != ' ' && pStr[i] != 0)
    {
        i++;
    }

    while (pStr[i] != 0)
    {
        printf("%c\n", pStr[i]);
        switch (pStr[i])
        {
            case ' ':
            case '\t': // type name;
                has_space = USCH_TRUE;
                break;
            case '=': // a = b
            case '+': // a++
            case '-': // b--
            case '%': // a % b
                value = USCH_FALSE;
                break;
            case '*': // int a* 
                      // a * b
                while(pStr[i] != ' ' || pStr[i] != '*' || pStr[i] != '\t')
                    i++;
                break;
            case '(':
            default:
                break;
        }

        i++;
    }
    return value;
}
int has_semicolon(char* pStr)
{
    int i;
    size_t len = strlen(pStr);

    for(i = 0; i < len; i++)
    {
        if(pStr[i] == ';')
        {
            return USCH_TRUE;
        }
    }

    return USCH_FALSE;
}
int evaluate(char *pBuf)
{
    // 1. try to put in global namespace
    // 2. try to put in function
    // 3. ???
    // 4. profit
    //
    int status = STATUS_OK;
    FILE *pStmt = NULL;
    int global_declaration = is_declaration(pBuf) & !has_semicolon(pBuf);
    char* stmt_header = "void f() { ";
    char* stmt_footer = "}";
    char* decl_header = "";
    char* decl_footer = ";";
    char* header = NULL;
    char* footer = NULL;

    pStmt = fopen("temp.c", "w");
    if(pStmt == NULL)
    {
        status = STATUS_FILE_FAILED;
        goto end;
    }
    if(is_declaration(pBuf))
    {
        header = decl_header;
        footer = decl_footer;
    }
    else
    {
        header = stmt_header;
        footer = stmt_footer;
    }
    fwrite(header, strlen(header), sizeof(char), pStmt);
    fwrite(pBuf, strlen(pBuf), sizeof(char), pStmt);
    fwrite(footer, strlen(footer), sizeof(char), pStmt);
    fclose(pStmt);
    pStmt = NULL;
    system("cc -shared temp.c -o foo.so");
end:
    return status;
}
#define USCH_TEST_EXPECT(a, b) assert((a) == (b))
int main(int argc, char **argv)
{
    char buf[BUFSIZE];
    char c = '0';
    char *pStr = NULL;
  USCH_TEST_EXPECT(has_semicolon("char* b;"), USCH_TRUE);
  USCH_TEST_EXPECT(has_semicolon("char* b"), USCH_FALSE);
  USCH_TEST_EXPECT(has_semicolon("cha;r* b"), USCH_TRUE);
  USCH_TEST_EXPECT(is_declaration("int a"), USCH_TRUE);
  USCH_TEST_EXPECT(is_declaration("a * b"), USCH_TRUE);
  USCH_TEST_EXPECT(is_declaration("char *= b"), USCH_FALSE);
  USCH_TEST_EXPECT(is_declaration("char  b = 3 * 2"), USCH_TRUE);
  USCH_TEST_EXPECT(is_declaration("a + b"), USCH_TRUE);
    pStr = readline("/* usch */ ");
    printf("%s\n", pStr);
    evaluate(pStr);
    free(pStr);
    pStr = NULL;
    return 0;
}

