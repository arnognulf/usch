cmake_minimum_required (VERSION 2.8.11)
project (USCH)

if( DEFINED USCH_LLVM_INCLUDE_PATH AND DEFINED USCH_LLVM_LIB_PATH )
    # User provided include/lib paths, fall through
elseif ( DEFINED USCH_LLVM_ROOT_PATH )
    # Synthesize include/lib relative to a root.
    set(USCH_LLVM_INCLUDE_PATH ${USCH_LLVM_ROOT_PATH}/include)
    set(USCH_LLVM_LIB_PATH ${USCH_LLVM_ROOT_PATH}/lib)
else()
    # If none provided, fail.
    message(FATAL_ERROR
        "Don't know how to find LLVM headers/libraries. "
        "Use -DUSCH_LLVM_ROOT_PATH=/xyz or both "
        "-DUSCH_LLVM_INCLUDE_PATH=/xyz/include and -DUSCH_LLVM_LIB_PATH=/xyz/lib")
endif()

include_directories(${USCH_LLVM_INCLUDE_PATH})
link_directories(${USCH_LLVM_LIB_PATH})

add_subdirectory (external)
add_subdirectory (libcrepl)
add_subdirectory (interpreter)

