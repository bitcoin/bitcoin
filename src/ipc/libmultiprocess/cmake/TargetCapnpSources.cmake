# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

#[=[

target_capnp_sources
--------------------

This function adds build steps to generate C++ files from Cap'n Proto files
and build them as part of a specified target.

Arguments:

  target: The name of the CMake target (e.g., a library or executable) to
    which the generated source files will be added. This target must already
    be defined elsewhere in the CMake scripts.

  include_prefix: Absolute path indicating what portion of capnp source paths
    should be used in relative #include statements in the generated C++
    files. For example, if the .capnp path is /home/src/lib/schema.capnp
    and include_prefix is /home/src, generated includes look like:

      #include <lib/schema.capnp.h>

    And if include_prefix is /home/src/lib, generated includes look like:

      #include <schema.capnp.h>

    The specified include_prefix should be ${CMAKE_SOURCE_DIR} or a
    subdirectory of it to include files relative to the project root. It can
    be ${CMAKE_CURRENT_SOURCE_DIR} to include files relative to the current
    source directory.

Additional Unnamed Arguments:

  After `target` and `include_prefix`, all unnamed arguments are treated as
  paths to `.capnp` schema files. These should be paths relative to
  ${CMAKE_CURRENT_SOURCE_DIR}.

Optional Keyword Arguments:

  IMPORT_PATHS: Specifies additional directories to search for imported
    `.capnp` files.

Example:
  # Assuming `my_library` is a target and `lib/` contains `.capnp` schema
  # files with imports from `include/`.
  target_capnp_sources(my_library "${CMAKE_SOURCE_DIR}"
                       lib/schema1.capnp lib/schema2.capnp
                       IMPORT_PATHS ${CMAKE_SOURCE_DIR}/include)

#]=]

function(target_capnp_sources target include_prefix)
  cmake_parse_arguments(PARSE_ARGV 2
    "TCS"           # prefix
    ""              # options
    ""              # one_value_keywords
    "IMPORT_PATHS"  # multi_value_keywords
  )

  set(MPGEN_BINARY "")
  if(MPGEN_EXECUTABLE)
    set(MPGEN_BINARY "${MPGEN_EXECUTABLE}")
    if(NOT EXISTS "${MPGEN_BINARY}")
      message(FATAL_ERROR "MPGEN_EXECUTABLE: \"${MPGEN_BINARY}\" does not exist.")
    endif()
  elseif(TARGET Libmultiprocess::multiprocess)
    set(MPGEN_BINARY Libmultiprocess::mpgen)
  else()
    message(FATAL_ERROR "No usable mpgen. Set MPGEN_EXECUTABLE or enable the internal target.")
  endif()

  get_property(mp_include_dir GLOBAL PROPERTY MP_INCLUDE_DIR)
  set(generated_headers "")
  foreach(capnp_file IN LISTS TCS_UNPARSED_ARGUMENTS)
    add_custom_command(
      OUTPUT ${capnp_file}.c++ ${capnp_file}.h ${capnp_file}.proxy-client.c++ ${capnp_file}.proxy-types.h ${capnp_file}.proxy-server.c++ ${capnp_file}.proxy-types.c++ ${capnp_file}.proxy.h
      COMMAND ${MPGEN_BINARY} ${CMAKE_CURRENT_SOURCE_DIR} ${include_prefix} ${CMAKE_CURRENT_SOURCE_DIR}/${capnp_file} ${TCS_IMPORT_PATHS} ${mp_include_dir}
      DEPENDS ${capnp_file}
      VERBATIM
    )
    # Skip linting for capnp-generated files but keep it for mpgen-generated ones
    set_source_files_properties(${capnp_file}.c++ PROPERTIES SKIP_LINTING TRUE) # Ignored before cmake 3.27
    target_sources(${target} PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}/${capnp_file}.c++
      ${CMAKE_CURRENT_BINARY_DIR}/${capnp_file}.proxy-client.c++
      ${CMAKE_CURRENT_BINARY_DIR}/${capnp_file}.proxy-server.c++
      ${CMAKE_CURRENT_BINARY_DIR}/${capnp_file}.proxy-types.c++
    )

    list(APPEND generated_headers ${capnp_file}.h)
  endforeach()

  # Translate include_prefix from a source path to a binary path and add it as a
  # target include directory.
  set(build_include_prefix ${CMAKE_BINARY_DIR})
  file(RELATIVE_PATH relative_path ${CMAKE_SOURCE_DIR} ${include_prefix})
  if(relative_path)
    string(APPEND build_include_prefix "/" "${relative_path}")
  endif()
  target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${build_include_prefix}> ${mp_include_dir})

  if(TARGET Libmultiprocess::multiprocess)
    target_link_libraries(${target} PRIVATE Libmultiprocess::multiprocess)
  endif()

  # Add a custom target that can be specified as a dependency of c++ targets
  # that include generated headers. It can be necessary to specify these
  # dependencies explicitly because while cmake detect dependencies of non
  # generated files on generated headers, it does not reliably detect
  # dependencies of generated headers on other generated headers.
  add_custom_target("${target}_headers" DEPENDS ${generated_headers})
endfunction()
