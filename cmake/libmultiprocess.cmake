# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

function(add_libmultiprocess subdir)
  # Unconditionally set BUILD_TESTING to ON. BUILD_TESTING is a standard cmake
  # variable that controls whether CTest is used. In the bitcoin build, CTest is
  # always used and we do not want an option to turn it off. But in
  # libmultiprocess, CTest is optional and BUILD_TESTING needs to be set in
  # order to add libmultiprocess unit tests to the CTest configuration.
  set(BUILD_TESTING ON)
  add_subdirectory(${subdir} EXCLUDE_FROM_ALL)
  # Apply core_interface compile options to libmultiprocess runtime library.
  target_link_libraries(multiprocess PUBLIC $<BUILD_INTERFACE:core_interface>)
  target_link_libraries(mputil PUBLIC $<BUILD_INTERFACE:core_interface>)
  target_link_libraries(mpgen PUBLIC $<BUILD_INTERFACE:core_interface>)
  # Mark capproto options as advanced to hide by default from cmake UI
  mark_as_advanced(CapnProto_DIR)
  mark_as_advanced(CapnProto_capnpc_IMPORTED_LOCATION)
  mark_as_advanced(CapnProto_capnp_IMPORTED_LOCATION)
  mark_as_advanced(CapnProto_capnp-json_IMPORTED_LOCATION)
  mark_as_advanced(CapnProto_capnp-rpc_IMPORTED_LOCATION)
  mark_as_advanced(CapnProto_capnp-websocket_IMPORTED_LOCATION)
  mark_as_advanced(CapnProto_kj-async_IMPORTED_LOCATION)
  mark_as_advanced(CapnProto_kj-gzip_IMPORTED_LOCATION)
  mark_as_advanced(CapnProto_kj-http_IMPORTED_LOCATION)
  mark_as_advanced(CapnProto_kj_IMPORTED_LOCATION)
  mark_as_advanced(CapnProto_kj-test_IMPORTED_LOCATION)
  mark_as_advanced(CapnProto_kj-tls_IMPORTED_LOCATION)
  # Share MP_INCLUDE_DIR variable with parent scope so target_capnp_sources
  # build rules can find the libmultiprocess include directory and avoid
  # "error: Import failed: /mp/proxy.capnp" errors from capnp.
  set(MP_INCLUDE_DIR "${MP_INCLUDE_DIR}" PARENT_SCOPE)
  # Add tests to "all" target so ctest can run them
  set_target_properties(mptests PROPERTIES EXCLUDE_FROM_ALL OFF)
  # Exclude examples from compilation database, because the examples are not
  # built by default, and they contain generated c++ code. Without this
  # exclusion, tools like clang-tidy and IWYU that make use of compilation
  # database would complain that the generated c++ source files do not exist. An
  # alternate fix could build "mpexamples" by default like "mptests" above.
  set_target_properties(mpcalculator mpprinter mpexample PROPERTIES EXPORT_COMPILE_COMMANDS OFF)
endfunction()
