# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found. Any number of sources could be listed here.
add_library (crepl SHARED bufstr.c crepl.c crepl_eval.c crepl_linker.c crepl_complete.c crepl_parser.c crepl_vars.c strutils.c crepl.h ../usch_h/usch.h)

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
target_include_directories (crepl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS crepl 
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION lib)
install(FILES usch.h
        DESTINATION "include")
install(FILES crepl.h
        DESTINATION "include")
install(FILES uschrc.h
        DESTINATION "etc")

