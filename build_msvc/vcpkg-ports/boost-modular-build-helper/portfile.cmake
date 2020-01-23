set(VCPKG_POLICY_EMPTY_PACKAGE enabled)

file(
    COPY
        ${CMAKE_CURRENT_LIST_DIR}/boost-modular-build.cmake
        ${CMAKE_CURRENT_LIST_DIR}/Jamroot.jam
        ${CMAKE_CURRENT_LIST_DIR}/nothing.bat
        ${CMAKE_CURRENT_LIST_DIR}/user-config.jam
        ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt
    DESTINATION
        ${CURRENT_PACKAGES_DIR}/share/boost-build
)
