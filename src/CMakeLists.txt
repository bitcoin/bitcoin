# Must be included before CMAKE_INSTALL_INCLUDEDIR is used.
include(GNUInstallDirs)

add_library(secp256k1_precomputed OBJECT EXCLUDE_FROM_ALL
  precomputed_ecmult.c
  precomputed_ecmult_gen.c
)

# Add objects explicitly rather than linking to the object libs to keep them
# from being exported.
add_library(secp256k1 secp256k1.c $<TARGET_OBJECTS:secp256k1_precomputed>)

add_library(secp256k1_asm INTERFACE)
if(SECP256K1_ASM STREQUAL "arm32")
  add_library(secp256k1_asm_arm OBJECT EXCLUDE_FROM_ALL)
  target_sources(secp256k1_asm_arm PUBLIC
    asm/field_10x26_arm.s
  )
  target_sources(secp256k1 PRIVATE $<TARGET_OBJECTS:secp256k1_asm_arm>)
  target_link_libraries(secp256k1_asm INTERFACE secp256k1_asm_arm)
endif()

if(WIN32)
  # Define our export symbol only for shared libs.
  set_target_properties(secp256k1 PROPERTIES DEFINE_SYMBOL SECP256K1_DLL_EXPORT)
  target_compile_definitions(secp256k1 INTERFACE $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:SECP256K1_STATIC>)
endif()

# Object libs don't know if they're being built for a shared or static lib.
# Grab the PIC property from secp256k1 which knows.
get_target_property(use_pic secp256k1 POSITION_INDEPENDENT_CODE)
set_target_properties(secp256k1_precomputed PROPERTIES POSITION_INDEPENDENT_CODE ${use_pic})

target_include_directories(secp256k1 INTERFACE
  # Add the include path for parent projects so that they don't have to manually add it.
  $<BUILD_INTERFACE:$<$<NOT:$<BOOL:${PROJECT_IS_TOP_LEVEL}>>:${PROJECT_SOURCE_DIR}/include>>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# This emulates Libtool to make sure Libtool and CMake agree on the ABI version,
# see below "Calculate the version variables" in build-aux/ltmain.sh.
math(EXPR ${PROJECT_NAME}_soversion "${${PROJECT_NAME}_LIB_VERSION_CURRENT} - ${${PROJECT_NAME}_LIB_VERSION_AGE}")
set_target_properties(secp256k1 PROPERTIES
  SOVERSION ${${PROJECT_NAME}_soversion}
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set_target_properties(secp256k1 PROPERTIES
    VERSION ${${PROJECT_NAME}_soversion}.${${PROJECT_NAME}_LIB_VERSION_AGE}.${${PROJECT_NAME}_LIB_VERSION_REVISION}
  )
elseif(APPLE)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.17)
    math(EXPR ${PROJECT_NAME}_compatibility_version "${${PROJECT_NAME}_LIB_VERSION_CURRENT} + 1")
    set_target_properties(secp256k1 PROPERTIES
      MACHO_COMPATIBILITY_VERSION ${${PROJECT_NAME}_compatibility_version}
      MACHO_CURRENT_VERSION ${${PROJECT_NAME}_compatibility_version}.${${PROJECT_NAME}_LIB_VERSION_REVISION}
    )
    unset(${PROJECT_NAME}_compatibility_version)
  elseif(BUILD_SHARED_LIBS)
    message(WARNING
      "The 'compatibility version' and 'current version' values of the DYLIB "
      "will diverge from the values set by the GNU Libtool. To ensure "
      "compatibility, it is recommended to upgrade CMake to at least version 3.17."
    )
  endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(${PROJECT_NAME}_windows "secp256k1")
  if(MSVC)
    set(${PROJECT_NAME}_windows "${PROJECT_NAME}")
  endif()
  set_target_properties(secp256k1 PROPERTIES
    ARCHIVE_OUTPUT_NAME "${${PROJECT_NAME}_windows}"
    RUNTIME_OUTPUT_NAME "${${PROJECT_NAME}_windows}-${${PROJECT_NAME}_soversion}"
  )
  unset(${PROJECT_NAME}_windows)
endif()
unset(${PROJECT_NAME}_soversion)

if(SECP256K1_BUILD_BENCHMARK)
  add_executable(bench bench.c)
  target_link_libraries(bench secp256k1)
  add_executable(bench_internal bench_internal.c)
  target_link_libraries(bench_internal secp256k1_precomputed secp256k1_asm)
  add_executable(bench_ecmult bench_ecmult.c)
  target_link_libraries(bench_ecmult secp256k1_precomputed secp256k1_asm)
endif()

if(SECP256K1_BUILD_TESTS)
  add_executable(noverify_tests tests.c)
  target_link_libraries(noverify_tests secp256k1_precomputed secp256k1_asm)
  add_test(NAME secp256k1_noverify_tests COMMAND noverify_tests)
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Coverage")
    add_executable(tests tests.c)
    target_compile_definitions(tests PRIVATE VERIFY)
    target_link_libraries(tests secp256k1_precomputed secp256k1_asm)
    add_test(NAME secp256k1_tests COMMAND tests)
  endif()
endif()

if(SECP256K1_BUILD_EXHAUSTIVE_TESTS)
  # Note: do not include secp256k1_precomputed in exhaustive_tests (it uses runtime-generated tables).
  add_executable(exhaustive_tests tests_exhaustive.c)
  target_link_libraries(exhaustive_tests secp256k1_asm)
  target_compile_definitions(exhaustive_tests PRIVATE $<$<NOT:$<CONFIG:Coverage>>:VERIFY>)
  add_test(NAME secp256k1_exhaustive_tests COMMAND exhaustive_tests)
endif()

if(SECP256K1_BUILD_CTIME_TESTS)
  add_executable(ctime_tests ctime_tests.c)
  target_link_libraries(ctime_tests secp256k1)
endif()

if(SECP256K1_INSTALL)
  install(TARGETS secp256k1
    EXPORT ${PROJECT_NAME}-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  set(${PROJECT_NAME}_headers
    "${PROJECT_SOURCE_DIR}/include/secp256k1.h"
    "${PROJECT_SOURCE_DIR}/include/secp256k1_preallocated.h"
  )
  if(SECP256K1_ENABLE_MODULE_ECDH)
    list(APPEND ${PROJECT_NAME}_headers "${PROJECT_SOURCE_DIR}/include/secp256k1_ecdh.h")
  endif()
  if(SECP256K1_ENABLE_MODULE_RECOVERY)
    list(APPEND ${PROJECT_NAME}_headers "${PROJECT_SOURCE_DIR}/include/secp256k1_recovery.h")
  endif()
  if(SECP256K1_ENABLE_MODULE_EXTRAKEYS)
    list(APPEND ${PROJECT_NAME}_headers "${PROJECT_SOURCE_DIR}/include/secp256k1_extrakeys.h")
  endif()
  if(SECP256K1_ENABLE_MODULE_SCHNORRSIG)
    list(APPEND ${PROJECT_NAME}_headers "${PROJECT_SOURCE_DIR}/include/secp256k1_schnorrsig.h")
  endif()
  if(SECP256K1_ENABLE_MODULE_MUSIG)
    list(APPEND ${PROJECT_NAME}_headers "${PROJECT_SOURCE_DIR}/include/secp256k1_musig.h")
  endif()
  if(SECP256K1_ENABLE_MODULE_ELLSWIFT)
    list(APPEND ${PROJECT_NAME}_headers "${PROJECT_SOURCE_DIR}/include/secp256k1_ellswift.h")
  endif()
  install(FILES ${${PROJECT_NAME}_headers}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(EXPORT ${PROJECT_NAME}-targets
    FILE ${PROJECT_NAME}-targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )

  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/config.cmake.in
    ${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    NO_SET_AND_CHECK_MACRO
  )
  write_basic_package_version_file(${PROJECT_NAME}-config-version.cmake
    COMPATIBILITY SameMinorVersion
  )

  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )

  include(GeneratePkgConfigFile)
  generate_pkg_config_file(${PROJECT_SOURCE_DIR}/libsecp256k1.pc.in)
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )
endif()
