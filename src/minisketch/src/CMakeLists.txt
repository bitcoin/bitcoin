add_library(minisketch_field_sources INTERFACE)
target_sources(minisketch_field_sources
  INTERFACE
    fields/generic_1byte.cpp
    fields/generic_2bytes.cpp
    fields/generic_3bytes.cpp
    fields/generic_4bytes.cpp
    fields/generic_5bytes.cpp
    fields/generic_6bytes.cpp
    fields/generic_7bytes.cpp
    fields/generic_8bytes.cpp
)
if(HAVE_CLMUL)
  set(clmul_sources
    fields/clmul_1byte.cpp
    fields/clmul_2bytes.cpp
    fields/clmul_3bytes.cpp
    fields/clmul_4bytes.cpp
    fields/clmul_5bytes.cpp
    fields/clmul_6bytes.cpp
    fields/clmul_7bytes.cpp
    fields/clmul_8bytes.cpp
  )
  set_property(SOURCE minisketch.cpp
    PROPERTY COMPILE_DEFINITIONS HAVE_CLMUL
  )
  set_property(SOURCE ${clmul_sources}
    PROPERTY COMPILE_OPTIONS ${CLMUL_CXXFLAGS}
  )
  target_sources(minisketch_field_sources INTERFACE ${clmul_sources})
endif()

add_compile_definitions($<$<BOOL:${HAVE_CLZ}>:HAVE_CLZ>)

add_library(minisketch minisketch.cpp)
set_property(TARGET minisketch PROPERTY PUBLIC_HEADER
  ${PROJECT_SOURCE_DIR}/include/minisketch.h
)
target_include_directories(minisketch
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)
target_link_libraries(minisketch PRIVATE minisketch_field_sources)

add_library(minisketch_verify EXCLUDE_FROM_ALL minisketch.cpp)
target_compile_definitions(minisketch_verify
  PUBLIC
    MINISKETCH_VERIFY
)
target_link_libraries(minisketch_verify PRIVATE minisketch_field_sources)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(minisketch_verify
    PRIVATE
      /wd4702
  )
endif()

if(MINISKETCH_BUILD_TESTS)
  add_executable(test-noverify test.cpp)
  target_link_libraries(test-noverify PRIVATE minisketch)
  add_test(NAME ${PROJECT_NAME}_test_noverify COMMAND test-noverify)

  add_executable(test-verify test.cpp)
  target_link_libraries(test-verify PRIVATE minisketch_verify)
  add_test(NAME ${PROJECT_NAME}_test_verify COMMAND test-verify)
endif()

if(MINISKETCH_BUILD_BENCHMARK)
  add_executable(bench bench.cpp)
  target_link_libraries(bench PRIVATE minisketch)
endif()

if(MINISKETCH_INSTALL)
  include(GNUInstallDirs)
  install(TARGETS minisketch)
endif()
