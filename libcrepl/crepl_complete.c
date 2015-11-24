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
                       void(add_completion_callback(void *, char *)),
                       void *p_callback_data)
{
    E_CREPL estatus = E_CREPL_OK;
    ustash s = {0};

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
    // must point to beginning of identifier/after '>'/after '.' to be completed
    int column = (int)sizeof(CREPL_INDENT) - 1 + (int)strlen(p_input);

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
        int match = -1;

        for (unsigned j = 0; j < clang_getNumCompletionChunks(str); j++) {
            const char *p_str = clang_getCString(clang_getCompletionChunkText(str, j));
            if (clang_getCompletionChunkKind(str, j) \
                    == CXCompletionChunk_TypedText && 
                    ustrneq(p_str, p_input, strlen(p_input)))
            {
                if (match == -1)
                {
                    match = j;
                }
            }
        }

#if 0
str: crepl_create 1
str: ( 6 
str: struct crepl **pp_crepl 3
str: ,  14
str: crepl_options options 3
str: ) 7 
#endif // 0

        for (unsigned j = match; j < clang_getNumCompletionChunks(str); j++) {

            {
                if (clang_getCompletionChunkKind(str, j+1) == CXCompletionChunk_LeftParen && clang_getCompletionChunkKind(str, j+2) == CXCompletionChunk_Placeholder)
                {
#if 0
                    printf("complete: \"%s%s/* %s */",
                      clang_getCString(clang_getCompletionChunkText(str, j+0)),
                      clang_getCString(clang_getCompletionChunkText(str, j+1)),
                      clang_getCString(clang_getCompletionChunkText(str, j+2))
                          );
#endif // 0
                    ustash s = {0};
                    char *p_completed_string = ustrjoin(&s, 
                      clang_getCString(clang_getCompletionChunkText(str, j+0)),
                      clang_getCString(clang_getCompletionChunkText(str, j+1)),
                      "/*",
                      clang_getCString(clang_getCompletionChunkText(str, j+2)),
                      "*/",
                     );
 
                    add_completion_callback(p_callback_data, p_completed_string);
                }
            }
            match = -1;
        }
    }
    clang_disposeCodeCompleteResults(res);
end:
    return estatus;
}


