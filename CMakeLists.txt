cmake_minimum_required (VERSION 2.8.11)
project (USCH)

#
# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Debug prior to calling PROJECT()
#
IF(DEFINED CMAKE_BUILD_TYPE)
       SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
       SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

IF(CMAKE_BUILD_TYPE MATCHES Debug)
      message("Debug build.")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb -D_DEFAULT_SOURCE -std=gnu99 -pedantic -Wall -Wextra -Werror -Wno-gnu-zero-variadic-macro-arguments -g3 -DUSCH_INSTALL_PREFIX=\"$(PREFIX)\"")
else()
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -D_DEFAULT_SOURCE -std=gnu99 -pedantic -Wall -Wextra -Werror -g3 -DUSCH_INSTALL_PREFIX=\"$(PREFIX)\"")
endif()

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
        "Use -DUSCH_LLVM_ROOT_PATH=/usr/lib/llvm-3.6 or both "
        "-DUSCH_LLVM_INCLUDE_PATH=/usr/lib/llvm-3.6/include and -DUSCH_LLVM_LIB_PATH=/usr/lib/llvm-3.6/lib")
endif()

include_directories(${USCH_LLVM_INCLUDE_PATH})
link_directories(${USCH_LLVM_LIB_PATH})

add_subdirectory (external)
add_subdirectory (libcrepl)
add_subdirectory (interpreter)

# find_package(GTest)
# if (GTEST_FOUND)
    enable_testing()
    add_custom_target(build_and_test ${CMAKE_CTEST_COMMAND} -V)
    add_subdirectory(test)
# endif()

