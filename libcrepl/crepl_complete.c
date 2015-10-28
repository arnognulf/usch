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

#include <crepl.h>
#include <clang-c/Index.h>

E_CREPL crepl_complete(struct crepl_t *p_crepl,
                   const char *p_input,
                   char **pp_results,
                   int *p_num_results)
{
    if (p_crepl == NULL ||
        pp_results == NULL ||
        p_num_results == NULL)
        return E_CREPL_PARAM;

    CXTranslationUnit p_tu = p_crepl->p_tu;

    return E_CREPL_OK;
}

int crepl_reload_tu(struct crepl_t *p_crepl)
{
    int status = 0;
    CXTranslationUnit p_tu = NULL;
    unsigned int visitorstatus = 0;

    if (p_crepl == NULL)
        return E_CREPL_PARAM;
    
    p_new_tu = clang_parseTranslationUnit(p_crepl->p_idx,
                                      p_crepl->p_includefile,
                                      NULL,
                                      0,
                                      NULL,
                                      0,
                                      0);
    FAIL_IF(p_tu == NULL);
    (void)clang_disposeTranslationUnit(p_crepl->p_tu);
    p_crepl->p_tu = p_new_tu;
end:
    return E_CREPL_OK;
}

