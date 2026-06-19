#[=[
This emulates Libtool to make sure Libtool and CMake agree on
the ABI version and file naming for shared libraries.

The `version_type` variable is set in `libtool.m4` (installed
by autoreconf into autotools-aux/m4/).
For the `major` and `versuffix` variables, see below "Calculate
the version variables" in `ltmain.sh` (installed by autoreconf
into autotools-aux/).
]=]
function(set_libtool_abi_version target current revision age)
  if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|FreeBSD)$")
    # version_type = linux | freebsd-elf
    # major = $current - $age
    # versuffix = $major.$age.$revision
    math(EXPR _major "${current} - ${age}")
    set_target_properties(${target} PROPERTIES
      SOVERSION ${_major}
      VERSION ${_major}.${age}.${revision}
    )
  elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    # version_type = sunos
    # major = $current
    # versuffix = $current.$revision
    set_target_properties(${target} PROPERTIES
      SOVERSION ${current}
      VERSION ${current}.${revision}
    )
  elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    # version_type = sunos
    # major = $current
    # versuffix = $current.$revision
    set_target_properties(${target} PROPERTIES
      # OpenBSD has no `soname_spec` defined in `libtool.m4`.
      VERSION ${current}.${revision}
    )
  elseif(APPLE)
    # version_type = darwin
    # major = $current - $age
    math(EXPR _major "${current} - ${age}")
    math(EXPR _compatibility "${current} + 1")
    set_target_properties(${target} PROPERTIES
      SOVERSION ${_major}
      MACHO_COMPATIBILITY_VERSION ${_compatibility}
      MACHO_CURRENT_VERSION ${_compatibility}.${revision}
    )
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # version_type = windows
    # major = $current - $age
    # versuffix = $major
    math(EXPR _major "${current} - ${age}")
    set(_windows_name "secp256k1")
    if(MSVC)
      set(_windows_name "${PROJECT_NAME}")
    endif()
    set_target_properties(${target} PROPERTIES
      ARCHIVE_OUTPUT_NAME "${_windows_name}"
      RUNTIME_OUTPUT_NAME "${_windows_name}-${_major}"
    )
  endif()
endfunction()
