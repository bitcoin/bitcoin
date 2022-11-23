
find_package(PkgConfig)

pkg_check_modules(Guile guile-2.2)

if (NOT Guile_FOUND)
  message(STATUS "Disabling Guile modules")
  return()
endif()

set(GUILE_EXTENSION_DIR ${CMAKE_CURRENT_BINARY_DIR})
configure_file(immer.scm.in immer.scm)

add_library(guile-immer SHARED EXCLUDE_FROM_ALL
  src/immer.cpp)
target_include_directories(guile-immer PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CALLABLE_TRAITS_INCLUDE_DIR}
  ${Guile_INCLUDE_DIRS})
target_link_libraries(guile-immer PUBLIC
  immer
  ${Guile_LIBRARIES})

add_custom_target(guile DEPENDS guile-immer)
