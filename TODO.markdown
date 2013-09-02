* globbing
    fnmatch
    fstat
    search PATH
* tab completion
* foreach
    strdup
    [usch_]strsplit
    [usch_]strexp: *.h

    return char** and param to enable later free'ing
    void buf[2];
    char** strexp(instr, buf[2], &outstr)
    // if no expansion possible, outstr == buf, buf -> instr
    // or NULL
    glob

    ptr1 ptr2 ptr3 NULL data1 data2 data3
    if(strsplit(instr, &outstr) == NULL)
        goto err;
    for (i = 0; str[i] != NULL; i++)
    {
    }

    foreach(char *p, "*.h")
    {
    }
// http://stackoverflow.com/questions/400951/does-c-have-a-foreach-loop-construct
Iteration over an array is also possible:

    #define foreach(item, array) \
    for(int keep = 1, \
            count = 0,\
            size = sizeof (array) / sizeof *(array); \
            keep && count != size; \
            keep = !keep, count++) \
                for(item = (array) + count; keep; keep = !keep)
    And can be used like

    int values[] = { 1, 2, 3 };
    foreach(int *v, values) {
        printf("value: %d\n", *v);
    }

    forusch?


* prompt

