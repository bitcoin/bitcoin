set(ENV_ATISTREAMSDKROOT $ENV{ATISTREAMSDKROOT})
if(ENV_ATISTREAMSDKROOT)
  find_path(
    OPENCL_INCLUDE_DIR
    NAMES CL/cl.h OpenCL/cl.h
    PATHS $ENV{ATISTREAMSDKROOT}/include
    NO_DEFAULT_PATH
    )

  if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(
        OPENCL_LIB_SEARCH_PATH
        ${OPENCL_LIB_SEARCH_PATH}
        $ENV{ATISTREAMSDKROOT}/lib/x86
        )
    else(CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(
        OPENCL_LIB_SEARCH_PATH
        ${OPENCL_LIB_SEARCH_PATH}
        $ENV{ATISTREAMSDKROOT}/lib/x86_64
        )
    endif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  endif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  find_library(
    OPENCL_LIBRARY
    NAMES OpenCL
    PATHS ${OPENCL_LIB_SEARCH_PATH}
    NO_DEFAULT_PATH
    )
else(ENV_ATISTREAMSDKROOT)
  find_path(
    OPENCL_INCLUDE_DIR
    NAMES CL/cl.h OpenCL/cl.h
    )

  find_library(
    OPENCL_LIBRARY
    NAMES OpenCL
    )
endif(ENV_ATISTREAMSDKROOT)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  OPENCL
  DEFAULT_MSG
  OPENCL_LIBRARY OPENCL_INCLUDE_DIR
  )

if(OPENCL_FOUND)
  set(OPENCL_LIBRARIES ${OPENCL_LIBRARY})
else(OPENCL_FOUND)
  set(OPENCL_LIBRARIES)
endif(OPENCL_FOUND)

mark_as_advanced(
  OPENCL_INCLUDE_DIR
  OPENCL_LIBRARY
  )
