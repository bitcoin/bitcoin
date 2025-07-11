function(generate_pkg_config_file in_file)
  set(prefix "@CMAKE_INSTALL_PREFIX@")
  set(exec_prefix \${prefix})
  set(libdir \${exec_prefix}/${CMAKE_INSTALL_LIBDIR})
  set(includedir \${prefix}/${CMAKE_INSTALL_INCLUDEDIR})
  set(PACKAGE_VERSION ${PROJECT_VERSION})
  configure_file(${in_file} ${PROJECT_NAME}.pc.in @ONLY)
  install(CODE "configure_file(
  \"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc.in\"
  \"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc\"
  @ONLY)" ALL_COMPONENTS)
endfunction()
