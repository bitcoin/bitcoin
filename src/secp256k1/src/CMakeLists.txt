# Must be included before CMAKE_INSTALL_INCLUDEDIR is used.
include(GNUInstallDirs)
set(${PROJECT_NAME}_installables "")

if(SECP256K1_ASM STREQUAL "arm")
  add_library(common OBJECT
    asm/field_10x26_arm.s
  )
  set(common_obj "$<TARGET_OBJECTS:common>")
else()
  set(common_obj "")
endif()

add_library(precomputed OBJECT
  precomputed_ecmult.c
  precomputed_ecmult_gen.c
)
set(internal_obj "$<TARGET_OBJECTS:precomputed>" "${common_obj}")

add_library(secp256k1 SHARED EXCLUDE_FROM_ALL
  secp256k1.c
  ${internal_obj}
)
target_include_directories(secp256k1 INTERFACE
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_definitions(secp256k1 PRIVATE
  $<$<PLATFORM_ID:Windows>:DLL_EXPORT>
)
set_target_properties(secp256k1 PROPERTIES
  VERSION "${${PROJECT_NAME}_LIB_VERSION_CURRENT}.${${PROJECT_NAME}_LIB_VERSION_AGE}.${${PROJECT_NAME}_LIB_VERSION_REVISION}"
  SOVERSION "${${PROJECT_NAME}_LIB_VERSION_CURRENT}"
)
if(SECP256K1_BUILD_SHARED)
  get_target_property(use_pic secp256k1 POSITION_INDEPENDENT_CODE)
  set_target_properties(precomputed PROPERTIES POSITION_INDEPENDENT_CODE ${use_pic})
  set_target_properties(secp256k1 PROPERTIES EXCLUDE_FROM_ALL FALSE)
  list(APPEND ${PROJECT_NAME}_installables secp256k1)
endif()

add_library(secp256k1_static STATIC EXCLUDE_FROM_ALL
  secp256k1.c
  ${internal_obj}
)
target_include_directories(secp256k1_static INTERFACE
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
if(NOT MSVC)
  set_target_properties(secp256k1_static PROPERTIES
    OUTPUT_NAME secp256k1
  )
endif()
if(SECP256K1_BUILD_STATIC)
  set_target_properties(secp256k1_static PROPERTIES EXCLUDE_FROM_ALL FALSE)
  list(APPEND ${PROJECT_NAME}_installables secp256k1_static)
endif()

add_library(binary_interface INTERFACE)
target_compile_definitions(binary_interface INTERFACE
  $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

add_library(link_library INTERFACE)
if(SECP256K1_BUILD_SHARED)
  target_link_libraries(link_library INTERFACE secp256k1)
elseif(SECP256K1_BUILD_STATIC)
  target_link_libraries(link_library INTERFACE secp256k1_static)
endif()

if(SECP256K1_BUILD_BENCHMARK)
  add_executable(bench bench.c)
  target_link_libraries(bench binary_interface link_library)
  add_executable(bench_internal bench_internal.c ${internal_obj})
  target_link_libraries(bench_internal binary_interface)
  add_executable(bench_ecmult bench_ecmult.c ${internal_obj})
  target_link_libraries(bench_ecmult binary_interface)
endif()

if(SECP256K1_BUILD_TESTS)
  add_executable(noverify_tests tests.c ${internal_obj})
  target_link_libraries(noverify_tests binary_interface)
  add_test(noverify_tests noverify_tests)
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Coverage")
    add_executable(tests tests.c ${internal_obj})
    target_compile_definitions(tests PRIVATE VERIFY)
    target_link_libraries(tests binary_interface)
    add_test(tests tests)
  endif()
endif()

if(SECP256K1_BUILD_EXHAUSTIVE_TESTS)
  # Note: do not include $<TARGET_OBJECTS:precomputed> in exhaustive_tests (it uses runtime-generated tables).
  add_executable(exhaustive_tests tests_exhaustive.c ${common_obj})
  target_compile_definitions(exhaustive_tests PRIVATE $<$<NOT:$<CONFIG:Coverage>>:VERIFY>)
  target_link_libraries(exhaustive_tests binary_interface)
  add_test(exhaustive_tests exhaustive_tests)
endif()

if(SECP256K1_BUILD_CTIME_TESTS)
  add_executable(ctime_tests ctime_tests.c)
  target_link_libraries(ctime_tests binary_interface link_library)
endif()

install(TARGETS ${${PROJECT_NAME}_installables}
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
  COMPATIBILITY SameMajorVersion
)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
