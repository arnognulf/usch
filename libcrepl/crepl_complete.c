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


