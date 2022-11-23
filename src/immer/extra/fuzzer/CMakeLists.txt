
add_custom_target(fuzzers
  COMMENT "Build all fuzzers.")

if (CHECK_FUZZERS)
  add_dependencies(tests fuzzers)
endif()

# LIB_FUZZING_ENGINE is set by the Google OSS-Fuzz
# infrastructure. Otherwise we use Clang's LibFuzzer
if (DEFINED ENV{LIB_FUZZING_ENGINE})
  set(immer_fuzzing_engine $ENV{LIB_FUZZING_ENGINE})
else()
  set(immer_fuzzing_engine "-fsanitize=fuzzer")
endif()

file(GLOB_RECURSE immer_fuzzers "*.cpp")
foreach(_file IN LISTS immer_fuzzers)
  immer_target_name_for(_target _output "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  target_compile_options(${_target} PUBLIC ${immer_fuzzing_engine})
  target_link_libraries(${_target} PUBLIC ${immer_fuzzing_engine}
    immer-dev)
  add_dependencies(fuzzers ${_target})
  if (CHECK_FUZZERS)
    add_test("fuzzer/${_output}" ${_output} -max_total_time=1)
  endif()
endforeach()
