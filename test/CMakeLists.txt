include_directories(${CMAKE_SOURCE_DIR}/libcrepl)
# include_directories(${GTEST_INCLUDE_DIRS})
add_executable(usch_tests tests.c)
add_executable(test_num_args test_num_args.c)
target_link_libraries(usch_tests crepl clang dl tcc git2)
add_test(usch_tests usch_tests)
add_dependencies(build_and_test usch_tests)

