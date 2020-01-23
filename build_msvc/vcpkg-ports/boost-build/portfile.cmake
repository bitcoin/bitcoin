include(vcpkg_common_functions)

set(VCPKG_POLICY_EMPTY_PACKAGE enabled)

if(NOT VCPKG_TARGET_ARCHITECTURE STREQUAL "x64" AND NOT VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    return()
elseif(CMAKE_HOST_WIN32 AND VCPKG_CMAKE_SYSTEM_NAME AND NOT VCPKG_CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    return()
endif()

set(BOOST_VERSION 1.72.0)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO boostorg/build
    REF boost-${BOOST_VERSION}
    SHA512 744816ba805013a49029373a4d2aa5b5f543275a1cdef2812c2120c868c55bf36a0bb0fb891cd955ad7319e582fd5212bd52ff071703a8654b345c478e810a19
    HEAD_REF master
)

vcpkg_download_distfile(ARCHIVE
    URLS "https://raw.githubusercontent.com/boostorg/boost/boost-${BOOST_VERSION}/LICENSE_1_0.txt"
    FILENAME "boost_LICENSE_1_0.txt"
    SHA512 d6078467835dba8932314c1c1e945569a64b065474d7aced27c9a7acc391d52e9f234138ed9f1aa9cd576f25f12f557e0b733c14891d42c16ecdc4a7bd4d60b8
)

vcpkg_download_distfile(BOOSTCPP_ARCHIVE
    URLS "https://raw.githubusercontent.com/boostorg/boost/boost-${BOOST_VERSION}/boostcpp.jam"
    FILENAME "boost-${BOOST_VERSION}-boostcpp.jam"
    SHA512 7fac16c1f082821dd52cae39601f60bbdbd5ce043fbd19699da54c74fc5df1ed2ad6d3cefd3ae9a0a7697a2c34737f0c9e2b4bd3590c1f45364254875289cd17
)

file(INSTALL ${ARCHIVE} DESTINATION ${CURRENT_PACKAGES_DIR}/share/boost-build RENAME copyright)
file(INSTALL ${BOOSTCPP_ARCHIVE} DESTINATION ${CURRENT_PACKAGES_DIR}/tools/boost-build RENAME boostcpp.jam)

# This fixes the lib path to use desktop libs instead of uwp -- TODO: improve this with better "host" compilation
string(REPLACE "\\store\\;" "\\;" LIB "$ENV{LIB}")
set(ENV{LIB} "${LIB}")

file(COPY
    ${SOURCE_PATH}/
    DESTINATION ${CURRENT_PACKAGES_DIR}/tools/boost-build
)

file(READ "${CURRENT_PACKAGES_DIR}/tools/boost-build/src/tools/msvc.jam" _contents)
string(REPLACE " /ZW /EHsc " "" _contents "${_contents}")
string(REPLACE "-nologo" "" _contents "${_contents}")
string(REPLACE "/nologo" "" _contents "${_contents}")
string(REPLACE "/Zm800" "" _contents "${_contents}")
string(REPLACE "<define>_WIN32_WINNT=0x0602" "" _contents "${_contents}")
file(WRITE "${CURRENT_PACKAGES_DIR}/tools/boost-build/src/tools/msvc.jam" "${_contents}")

message(STATUS "Bootstrapping...")
if(CMAKE_HOST_WIN32)
    vcpkg_execute_required_process(
        COMMAND "${CURRENT_PACKAGES_DIR}/tools/boost-build/bootstrap.bat" msvc
        WORKING_DIRECTORY ${CURRENT_PACKAGES_DIR}/tools/boost-build
        LOGNAME bootstrap-${TARGET_TRIPLET}
    )
else()
    vcpkg_execute_required_process(
        COMMAND "${CURRENT_PACKAGES_DIR}/tools/boost-build/bootstrap.sh"
        WORKING_DIRECTORY ${CURRENT_PACKAGES_DIR}/tools/boost-build
        LOGNAME bootstrap-${TARGET_TRIPLET}
    )
endif()
