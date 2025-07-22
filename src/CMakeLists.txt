add_library(secp256k1)

set_property(TARGET secp256k1 PROPERTY PUBLIC_HEADER
  ${PROJECT_SOURCE_DIR}/include/secp256k1.h
  ${PROJECT_SOURCE_DIR}/include/secp256k1_preallocated.h
)

# Processing must be done in a topological sorting of the dependency graph
# (dependent module first).
if(SECP256K1_ENABLE_MODULE_ELLSWIFT)
  add_compile_definitions(ENABLE_MODULE_ELLSWIFT=1)
  set_property(TARGET secp256k1 APPEND PROPERTY PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/include/secp256k1_ellswift.h)
endif()

if(SECP256K1_ENABLE_MODULE_MUSIG)
  if(DEFINED SECP256K1_ENABLE_MODULE_SCHNORRSIG AND NOT SECP256K1_ENABLE_MODULE_SCHNORRSIG)
    message(FATAL_ERROR "Module dependency error: You have disabled the schnorrsig module explicitly, but it is required by the musig module.")
  endif()
  set(SECP256K1_ENABLE_MODULE_SCHNORRSIG ON)
  add_compile_definitions(ENABLE_MODULE_MUSIG=1)
  set_property(TARGET secp256k1 APPEND PROPERTY PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/include/secp256k1_musig.h)
endif()

if(SECP256K1_ENABLE_MODULE_SCHNORRSIG)
  if(DEFINED SECP256K1_ENABLE_MODULE_EXTRAKEYS AND NOT SECP256K1_ENABLE_MODULE_EXTRAKEYS)
    message(FATAL_ERROR "Module dependency error: You have disabled the extrakeys module explicitly, but it is required by the schnorrsig module.")
  endif()
  set(SECP256K1_ENABLE_MODULE_EXTRAKEYS ON)
  add_compile_definitions(ENABLE_MODULE_SCHNORRSIG=1)
  set_property(TARGET secp256k1 APPEND PROPERTY PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/include/secp256k1_schnorrsig.h)
endif()

if(SECP256K1_ENABLE_MODULE_EXTRAKEYS)
  add_compile_definitions(ENABLE_MODULE_EXTRAKEYS=1)
  set_property(TARGET secp256k1 APPEND PROPERTY PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/include/secp256k1_extrakeys.h)
endif()

if(SECP256K1_ENABLE_MODULE_RECOVERY)
  add_compile_definitions(ENABLE_MODULE_RECOVERY=1)
  set_property(TARGET secp256k1 APPEND PROPERTY PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/include/secp256k1_recovery.h)
endif()

if(SECP256K1_ENABLE_MODULE_ECDH)
  add_compile_definitions(ENABLE_MODULE_ECDH=1)
  set_property(TARGET secp256k1 APPEND PROPERTY PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/include/secp256k1_ecdh.h)
endif()

add_library(secp256k1_precomputed OBJECT EXCLUDE_FROM_ALL
  precomputed_ecmult.c
  precomputed_ecmult_gen.c
)

# Add objects explicitly rather than linking to the object libs to keep them
# from being exported.
target_sources(secp256k1 PRIVATE secp256k1.c $<TARGET_OBJECTS:secp256k1_precomputed>)

if(NOT SECP256K1_ENABLE_API_VISIBILITY_ATTRIBUTES)
  target_compile_definitions(secp256k1 PRIVATE SECP256K1_NO_API_VISIBILITY_ATTRIBUTES)
endif()

# Create a helper lib that parent projects can use to link secp256k1 into a
# static lib.
add_library(secp256k1_objs INTERFACE)
target_sources(secp256k1_objs INTERFACE $<TARGET_OBJECTS:secp256k1> $<TARGET_OBJECTS:secp256k1_precomputed>)

add_library(secp256k1_asm INTERFACE)
if(SECP256K1_ASM STREQUAL "arm32")
  add_library(secp256k1_asm_arm OBJECT EXCLUDE_FROM_ALL)
  target_sources(secp256k1_asm_arm PUBLIC
    asm/field_10x26_arm.s
  )
  target_sources(secp256k1 PRIVATE $<TARGET_OBJECTS:secp256k1_asm_arm>)
  target_sources(secp256k1_objs INTERFACE $<TARGET_OBJECTS:secp256k1_asm_arm>)
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

# Add the include path for parent projects so that they don't have to manually add it.
target_include_directories(secp256k1 INTERFACE
  $<BUILD_INTERFACE:$<$<NOT:$<BOOL:${PROJECT_IS_TOP_LEVEL}>>:${PROJECT_SOURCE_DIR}/include>>
)
set_target_properties(secp256k1_objs PROPERTIES
  INTERFACE_COMPILE_DEFINITIONS "$<TARGET_PROPERTY:secp256k1,INTERFACE_COMPILE_DEFINITIONS>"
  INTERFACE_INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:secp256k1,INTERFACE_INCLUDE_DIRECTORIES>"
)

# This emulates Libtool to make sure Libtool and CMake agree on the ABI version,
# see below "Calculate the version variables" in build-aux/ltmain.sh.
math(EXPR ${PROJECT_NAME}_soversion "${${PROJECT_NAME}_LIB_VERSION_CURRENT} - ${${PROJECT_NAME}_LIB_VERSION_AGE}")
set_target_properties(secp256k1 PROPERTIES
  SOVERSION ${${PROJECT_NAME}_soversion}
)
if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|FreeBSD)$")
  set_target_properties(secp256k1 PROPERTIES
    VERSION ${${PROJECT_NAME}_soversion}.${${PROJECT_NAME}_LIB_VERSION_AGE}.${${PROJECT_NAME}_LIB_VERSION_REVISION}
  )
elseif(APPLE)
  math(EXPR ${PROJECT_NAME}_compatibility_version "${${PROJECT_NAME}_LIB_VERSION_CURRENT} + 1")
  set_target_properties(secp256k1 PROPERTIES
    MACHO_COMPATIBILITY_VERSION ${${PROJECT_NAME}_compatibility_version}
    MACHO_CURRENT_VERSION ${${PROJECT_NAME}_compatibility_version}.${${PROJECT_NAME}_LIB_VERSION_REVISION}
  )
  unset(${PROJECT_NAME}_compatibility_version)
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
  include(GNUInstallDirs)
  target_include_directories(secp256k1 INTERFACE
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  install(TARGETS secp256k1
    EXPORT ${PROJECT_NAME}-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
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
