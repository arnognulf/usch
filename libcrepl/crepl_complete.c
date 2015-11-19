#include <clang-c/Index.h>
//#include <cstdlib>
//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "crepl.h"
#include "crepl_types.h"
#include "crepl_debug.h"
/*
 * Compile with:
 * g++ complete.cc -o complete -lclang -L/usr/lib/llvm
 * Run with:
 * LIBCLANG_TIMING=1 ./complete file.cc line column [clang args...]
 */

static int count_lines(const char *p_string)
{
    int num_lines = 1;
    while (*p_string != '\0')
    {
        if (*p_string == '\n')
            num_lines++;
        p_string++;
    }
    return num_lines;
}

E_CREPL crepl_complete(struct crepl *p_crepl,
                   const char *p_input,
                   char **pp_results,
                   int *p_num_results)
{
    (void)pp_results;
    E_CREPL estatus = E_CREPL_OK;
    ustash s = {0};
    int num_results = 0;

    CXIndex idx = clang_createIndex(1, 0);
    E_FAIL_IF(!idx);

    struct CXUnsavedFile unsaved_files;
    unsaved_files.Filename = p_crepl->p_stmt_c;
    unsaved_files.Contents = ustrjoin(&s, p_crepl->p_stmt_header, \
                                          "int ", CREPL_DYN_FUNCNAME, "(struct crepl *p_crepl)\n", \
                                          "{\n", \
                                          CREPL_INDENT, p_input);
    unsaved_files.Length = strlen(unsaved_files.Contents);

    CXTranslationUnit u = clang_parseTranslationUnit(idx,
                          p_crepl->p_stmt_c,
                          NULL, //argv + 4,
                          0, // argc - 4,
                          &unsaved_files,
                          1,
                          CXTranslationUnit_SkipFunctionBodies |
                          CXTranslationUnit_PrecompiledPreamble|
                          CXTranslationUnit_DetailedPreprocessingRecord |
                          CXTranslationUnit_Incomplete);

    E_FAIL_IF(!u);

    int line = count_lines(unsaved_files.Contents);
    // must point to beginning of identifier to be completed
    int column = (int)sizeof(CREPL_INDENT) - 1 + (int)strlen(p_input);

    //int line = strtol(argv[2], 0, 10);
    //int column = strtol(argv[3], 0, 10);
    CXCodeCompleteResults* res = clang_codeCompleteAt(
                                  u,
                                  p_crepl->p_stmt_c,
                                  line, column,
                                  0, 0,
                                  CXCodeComplete_IncludeMacros |
                                  CXCodeComplete_IncludeCodePatterns);
    E_FAIL_IF(!res);

    for (unsigned i = 0; i < res->NumResults; i++) {
        const CXCompletionString str = res->Results[i].CompletionString;

        for (unsigned j = 0; j < clang_getNumCompletionChunks(str); j++) {
            if (clang_getCompletionChunkKind(str, j) != CXCompletionChunk_TypedText)
                continue;

            const CXString out = clang_getCompletionChunkText(str, j);
            const char *p_str = clang_getCString(out);
            if (ustrneq(p_str, p_input, strlen(p_input)) == USCH_TRUE)
            {
                printf("%s %s\n", p_str, p_input); 
                num_results++;
            }
        }
    }
    clang_disposeCodeCompleteResults(res);
    *p_num_results = num_results;
end:
    return estatus;
}


