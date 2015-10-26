include_directories(${CMAKE_SOURCE_DIR}/libcrepl)
# include_directories(${GTEST_INCLUDE_DIRS})
add_executable(usch_tests tests.c)
target_link_libraries(usch_tests crepl clang dl)
add_test(usch_tests usch_tests)
add_dependencies(build_and_test usch_tests)

