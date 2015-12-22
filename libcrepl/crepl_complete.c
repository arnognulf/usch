#include <clang-c/Index.h>
//#include <cstdlib>
//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "crepl.h"
#include "crepl_types.h"
#include "crepl_debug.h"

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
static E_CREPL write_contents(const char *p_filename, const char *p_contents, size_t contents_length)
{
   E_CREPL estatus = E_CREPL_OK;
   FILE *p_file = NULL;

   p_file = fopen(p_filename, "w+");
   size_t written_bytes = fwrite(p_contents, 1, contents_length, p_file);
   if (written_bytes != contents_length)
       estatus = E_CREPL_WAT;
   fclose(p_file);
   return estatus;
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
    estatus = write_contents(p_crepl->p_stmt_c, unsaved_files.Contents, unsaved_files.Length);
    if (estatus != E_CREPL_OK)
        goto end;

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

        for (unsigned j = match; j < clang_getNumCompletionChunks(str); j++) {

            {
                if (clang_getCompletionChunkKind(str, j+1) == CXCompletionChunk_LeftParen && clang_getCompletionChunkKind(str, j+2) == CXCompletionChunk_Placeholder)
                {
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
end:
    clang_disposeCodeCompleteResults(res);
    return estatus;
}


