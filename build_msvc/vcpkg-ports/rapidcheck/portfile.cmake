include(vcpkg_common_functions)

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO emil-e/rapidcheck
    REF cf9e0d8bd8c94e9dc00dc0ab302352bfaf1a3ac5
    SHA512 6cef62edbda391c3527d63db350842f669841ad2c751a64773250cd40bb65f26c2c394b107ef5530c2d3bd15b7079148fa9778d68a7346225bbb15227b1553c5
    HEAD_REF master
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
        -DRC_INSTALL_ALL_EXTRAS=ON
)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH share/rapidcheck/cmake)
vcpkg_copy_pdbs()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include ${CURRENT_PACKAGES_DIR}/debug/share)

# Handle copyright
configure_file(${SOURCE_PATH}/LICENSE.md ${CURRENT_PACKAGES_DIR}/share/rapidcheck/copyright COPYONLY)

# Post-build test for cmake libraries
vcpkg_test_cmake(PACKAGE_NAME rapidcheck)
