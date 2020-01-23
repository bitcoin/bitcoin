
file(
    COPY ${CMAKE_CURRENT_LIST_DIR}/boost-modular-headers.cmake
    DESTINATION ${CURRENT_PACKAGES_DIR}/share/boost-vcpkg-helpers
)

set(VCPKG_POLICY_EMPTY_PACKAGE enabled)
