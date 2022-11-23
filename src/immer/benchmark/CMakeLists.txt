
#  Config
#  ======

option(CHECK_BENCHMARKS "Run benchmarks on check target" off)
option(BENCHMARK_DISABLE_GC "Disable gc during a measurement")

set(BENCHMARK_PARAM   "N:1000" CACHE STRING "Benchmark parameters")
set(BENCHMARK_SAMPLES "20"     CACHE STRING "Benchmark samples")

#  Dependencies
#  ============

find_package(RRB)

if (NOT RRB_FOUND)
  message(STATUS "Disabling benchmarks")
  return()
endif()

# These are expected to be in the include path, the nix-shell
# environment installs them:
#
#    https://github.com/marcusz/steady
#    https://github.com/deepsea-inria/chunkedseq.git
#    https://github.com/rsms/immutable-cpp.git

#  Targets
#  =======

add_custom_target(benchmarks
  COMMENT "Build all benchmarks.")

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE immer_git_commit_hash
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND git status --porcelain
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE immer_git_status
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if (NOT immer_git_status STREQUAL "")
  set(immer_git_commit_hash "${immer_git_commit_hash}+")
endif()

site_name(immer_hostname)

get_filename_component(immer_compiler_name "${CMAKE_CXX_COMPILER}" NAME)

set(immer_benchmark_report_base_dir "${CMAKE_SOURCE_DIR}/reports")
set(immer_benchmark_report_dir "${immer_benchmark_report_base_dir}/report_\
${immer_git_commit_hash}_\
${immer_hostname}_\
${immer_compiler_name}_\
${BENCHMARK_PARAM}_\
s${BENCHMARK_SAMPLES}")

if(DISABLE_FREE_LIST)
  set(immer_benchmark_report_dir "${immer_benchmark_report_dir}_nofl")
endif()

if(DISABLE_THREAD_SAFETY)
  set(immer_benchmark_report_dir "${immer_benchmark_report_dir}_nots")
endif()

if(BENCHMARK_DISABLE_GC)
  set(immer_benchmark_report_dir "${immer_benchmark_report_dir}_nogc")
endif()

if(CHECK_BENCHMARKS)
  add_dependencies(check benchmarks)
endif()

add_custom_target(benchmark-report-dir
  COMMAND ${CMAKE_COMMAND}
  -E make_directory ${immer_benchmark_report_dir})

file(GLOB_RECURSE immer_benchmarks "*.cpp")
foreach(_file IN LISTS immer_benchmarks)
  immer_target_name_for(_target _output "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  add_dependencies(benchmarks ${_target})
  add_dependencies(${_target} benchmark-report-dir)
  target_compile_options(${_target} PUBLIC -Wno-unused-function)
  target_compile_definitions(${_target} PUBLIC
    NONIUS_RUNNER
    IMMER_BENCHMARK_LIBRRB=1
    IMMER_BENCHMARK_STEADY=1
    IMMER_BENCHMARK_EXPERIMENTAL=0
    IMMER_BENCHMARK_DISABLE_GC=${BENCHMARK_DISABLE_GC}
    IMMER_BENCHMARK_BOOST_COROUTINE=${ENABLE_BOOST_COROUTINE})
  target_link_libraries(${_target} PUBLIC
    immer-dev
    ${RRB_LIBRARIES})
  target_include_directories(${_target} SYSTEM PUBLIC
    ${RRB_INCLUDE_DIR})
  if(CHECK_BENCHMARKS)
    add_test("benchmark/${_output}" "${CMAKE_SOURCE_DIR}/tools/with-tee.bash"
      ${immer_benchmark_report_dir}/${_target}.out
      "${CMAKE_CURRENT_BINARY_DIR}/${_output}" -v
      -t ${_target}
      -r html
      -s ${BENCHMARK_SAMPLES}
      -p ${BENCHMARK_PARAM}
      -o ${immer_benchmark_report_dir}/${_target}.html)
  endif()
endforeach()

set(immer_ssh_method
  ssh -p 5488
      -o StrictHostKeyChecking=no)

add_custom_target(upload-benchmark-reports
  COMMAND
  rsync -av -e \"${immer_ssh_method}\"
        ${immer_benchmark_report_base_dir}
        raskolnikov@sinusoid.es:public/misc/immer/)

add_custom_target(copy-benchmark-reports
  COMMAND
  rsync -av ${immer_benchmark_report_base_dir}
        ~/public/misc/immer/)
