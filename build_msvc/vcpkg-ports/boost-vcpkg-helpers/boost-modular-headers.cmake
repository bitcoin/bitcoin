function(boost_modular_headers)
    cmake_parse_arguments(_bm "" "SOURCE_PATH" "" ${ARGN})

    if(NOT DEFINED _bm_SOURCE_PATH)
        message(FATAL_ERROR "SOURCE_PATH is a required argument to boost_modular_headers.")
    endif()

    message(STATUS "Packaging headers")

    file(
        COPY ${_bm_SOURCE_PATH}/include/boost
        DESTINATION ${CURRENT_PACKAGES_DIR}/include
    )

    message(STATUS "Packaging headers done")

    vcpkg_download_distfile(ARCHIVE
        URLS "https://raw.githubusercontent.com/boostorg/boost/boost-1.72.0/LICENSE_1_0.txt"
        FILENAME "boost_LICENSE_1_0.txt"
        SHA512 d6078467835dba8932314c1c1e945569a64b065474d7aced27c9a7acc391d52e9f234138ed9f1aa9cd576f25f12f557e0b733c14891d42c16ecdc4a7bd4d60b8
    )

    file(INSTALL ${ARCHIVE} DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
endfunction()
