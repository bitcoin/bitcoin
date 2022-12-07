
#  Targets
#  =======

add_custom_target(tests
  COMMENT "Build all the unit tests.")
add_dependencies(check tests)

include(CTest)

file(GLOB_RECURSE immer_unit_tests "*.cpp")
foreach(_file IN LISTS immer_unit_tests)
  immer_target_name_for(_target _output "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  add_dependencies(tests ${_target})
  target_compile_definitions(${_target} PUBLIC
    -DIMMER_OSS_FUZZ_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/oss-fuzz/data"
    DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
    CATCH_CONFIG_MAIN)
  target_link_libraries(${_target} PUBLIC immer-dev)
  add_test("test/${_output}" ${_output})
endforeach()
