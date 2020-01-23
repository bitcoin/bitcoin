include(vcpkg_common_functions)

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/double-conversion
    REF v3.1.5
    SHA512 0aeabdbfa06c3c4802905ac4bf8c2180840577677b47d45e1c91034fe07746428c9db79260ce6bdbdf8b584746066cea9247ba43a9c38155caf1ef44e214180a
    HEAD_REF master
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
)

vcpkg_install_cmake()

# Rename exported target files into something vcpkg_fixup_cmake_targets expects
if(EXISTS ${CURRENT_PACKAGES_DIR}/lib/cmake/double-conversion)
    vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/double-conversion)
endif()

vcpkg_copy_pdbs()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)

# Handle copyright
configure_file(${SOURCE_PATH}/LICENSE ${CURRENT_PACKAGES_DIR}/share/double-conversion/copyright COPYONLY)
