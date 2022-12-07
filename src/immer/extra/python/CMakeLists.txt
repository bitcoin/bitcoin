
option(USE_PYBIND "bind with pybind1" off)
option(USE_BOOST_PYTHON  "bind with boost::python" off)

if (USE_PYBIND)
  set(PYBIND11_CPP_STANDARD -std=c++14)
  find_package(Boost 1.56 REQUIRED)
  add_subdirectory(lib/pybind11)
  pybind11_add_module(immer_python_module src/immer-pybind.cpp)
  target_link_libraries(immer_python_module PUBLIC
    immer)

elseif(USE_BOOST_PYTHON)
  find_package(PythonInterp)
  find_package(PythonLibs)
  find_package(Boost 1.56 COMPONENTS python)
  python_add_module(immer_python_module src/immer-boost.cpp)
  include_directories(immer_python_module PUBLIC
    ${immer_include_dir}
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS})
  target_link_libraries(immer_python_module PUBLIC
    immer
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES})

else()
  find_package(PythonInterp)
  find_package(PythonLibs)

  if (NOT PYTHONLIBS_FOUND)
    message(STATUS "Disabling Python modules")
    return()
  endif()

  python_add_module(immer_python_module EXCLUDE_FROM_ALL
    src/immer-raw.cpp)
  target_include_directories(immer_python_module PUBLIC
    ${PYTHON_INCLUDE_DIRS})
  target_link_libraries(immer_python_module PUBLIC
    immer
    ${PYTHON_LIBRARIES})

endif()

add_custom_target(python DEPENDS immer_python_module)
