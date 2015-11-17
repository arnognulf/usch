/*
 * USCH - The (permutated) tcsh successor
 * Copyright (c) 2013 Thomas Eriksson 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#if 0
// http://clang.llvm.org/doxygen/group__CINDEX__CODE__COMPLET.html#ga50fedfa85d8d1517363952f2e10aa3bf
CINDEX_LINKAGE CXCodeCompleteResults* clang_codeCompleteAt  (   CXTranslationUnit   TU,
        const char *    complete_filename,
        unsigned    complete_line,
        unsigned    complete_column,
        struct CXUnsavedFile *  unsaved_files,
        unsigned    num_unsaved_files,
        unsigned    options 

// http://clang.llvm.org/doxygen/structCXUnsavedFile.html
// 
// CINDEX_LINKAGE CXTranslationUnit clang_Cursor_getTranslationUnit (   CXCursor        )   
// how do we get a CXCursor (which can give us an CXTranslationUnit)??? Traverse the tree?
#endif // 0

#include "crepl_types.h"
#include "crepl_debug.h"
#include <crepl.h>
#include <clang-c/Index.h>

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
    if (p_crepl == NULL ||
        p_input == NULL ||
        pp_results == NULL ||
        p_num_results == NULL)
        return E_CREPL_PARAM;
    E_CREPL estatus = E_CREPL_OK;
    
    ustash s = {0};

    CXTranslationUnit p_tu = p_crepl->p_tu;
    CXCodeCompleteResults* results = NULL;
    struct CXUnsavedFile unsaved_files;

    unsaved_files.Filename = p_crepl->p_stmt_c;
    unsaved_files.Contents = ustrjoin(&s, p_crepl->p_stmt_header, \
                                          "int ", CREPL_DYN_FUNCNAME, "(struct crepl *p_crepl)\n", \
                                          "{\n", \
                                          CREPL_INDENT, p_input, ";\n", \
                                          CREPL_INDENT, "return 0;\n", \
                                          "}\n");

    unsaved_files.Length = strlen(unsaved_files.Contents);

    results = clang_codeCompleteAt(
            p_tu,
            p_crepl->p_stmt_c,
            count_lines(p_crepl->p_stmt_header), // line
            sizeof(CREPL_INDENT) - 1 + strlen(p_input), // col
            &unsaved_files,
            1, // num unsaved files
            clang_defaultCodeCompleteOptions()
            );
    E_FAIL_IF(results == NULL);
    p_crepl->p_completion_results = results;

end:
    uclear(&s);
    return estatus;
}

void crepl_complete_dispose(struct crepl *p_crepl)
{
    if (p_crepl == NULL)
        return;
    (void)clang_disposeCodeCompleteResults(p_crepl->p_completion_results);
}

E_CREPL crepl_reload_tu(struct crepl *p_crepl)
{
    E_CREPL estatus = 0;
    CXTranslationUnit p_new_tu = NULL;

    if (p_crepl == NULL)
        return E_CREPL_PARAM;

    if (!p_crepl->p_tu)
    {
        int cxerr = clang_reparseTranslationUnit(p_crepl->p_tu,
        0,
        NULL,
        clang_defaultEditingTranslationUnitOptions()
        );
        if (cxerr != 0)
        {
            clang_disposeTranslationUnit(p_crepl->p_tu);
            p_crepl->p_tu = NULL;
        }
    }

    if (!p_crepl->p_tu)
    {
    p_new_tu = clang_parseTranslationUnit(
                             p_crepl->p_idx,
                             p_crepl->p_stmt_c,
                             NULL,
                             0,
                             NULL,
                             0,
                             clang_defaultEditingTranslationUnitOptions());
    E_FAIL_IF(p_new_tu == NULL);
    }
    p_crepl->p_tu = p_new_tu;
end:
    return estatus;
}

