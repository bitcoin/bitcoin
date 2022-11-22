include(FetchContent)

FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.9.1
)
FetchContent_MakeAvailable(pybind11)

include_directories(
        ${INCLUDE_DIRECTORIES}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

pybind11_add_module(blspy ${CMAKE_CURRENT_SOURCE_DIR}/pythonbindings.cpp)
target_link_libraries(blspy PRIVATE dashbls)
