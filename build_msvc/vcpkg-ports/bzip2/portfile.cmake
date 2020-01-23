include(vcpkg_common_functions)
set(BZIP2_VERSION 1.0.6)
vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/past-due/bzip2-mirror/releases/download/v${BZIP2_VERSION}/bzip2-${BZIP2_VERSION}.tar.gz"
    FILENAME "bzip2-${BZIP2_VERSION}.tar.gz"
    SHA512 00ace5438cfa0c577e5f578d8a808613187eff5217c35164ffe044fbafdfec9e98f4192c02a7d67e01e5a5ccced630583ad1003c37697219b0f147343a3fdd12)
    
vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${ARCHIVE}
    REF ${BZIP2_VERSION}
    PATCHES
        fix-import-export-macros.patch
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS_DEBUG
        -DBZIP2_SKIP_HEADERS=ON
        -DBZIP2_SKIP_TOOLS=ON
)

vcpkg_install_cmake()
vcpkg_copy_pdbs()

file(READ ${CURRENT_PACKAGES_DIR}/include/bzlib.h BZLIB_H)
if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    string(REPLACE "defined(BZ_IMPORT)" "0" BZLIB_H "${BZLIB_H}")
else()
    string(REPLACE "defined(BZ_IMPORT)" "1" BZLIB_H "${BZLIB_H}")
endif()
file(WRITE ${CURRENT_PACKAGES_DIR}/include/bzlib.h "${BZLIB_H}")

file(COPY ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/bzip2)
file(RENAME ${CURRENT_PACKAGES_DIR}/share/bzip2/LICENSE ${CURRENT_PACKAGES_DIR}/share/bzip2/copyright)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/usage DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT})

vcpkg_test_cmake(PACKAGE_NAME BZip2 MODULE)
