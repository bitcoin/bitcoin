# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

function(generate_setup_nsi)
  set(abs_top_srcdir ${PROJECT_SOURCE_DIR})
  set(abs_top_builddir ${PROJECT_BINARY_DIR})
  set(CLIENT_URL ${PROJECT_HOMEPAGE_URL})
  set(CLIENT_TARNAME "bitcoin")
  set(BITCOIN_WRAPPER_NAME "bitcoin")
  set(BITCOIN_GUI_NAME "bitcoin-qt")
  set(BITCOIN_DAEMON_NAME "bitcoind")
  set(BITCOIN_CLI_NAME "bitcoin-cli")
  set(BITCOIN_TX_NAME "bitcoin-tx")
  set(BITCOIN_WALLET_TOOL_NAME "bitcoin-wallet")
  set(BITCOIN_TEST_NAME "test_bitcoin")
  set(EXEEXT ${CMAKE_EXECUTABLE_SUFFIX})
  set(nsi_out ${PROJECT_BINARY_DIR}/bitcoin-win64-setup.nsi)
  configure_file(${PROJECT_SOURCE_DIR}/share/setup.nsi.in ${nsi_out} USE_SOURCE_PERMISSIONS @ONLY)

  # When makensis runs on a native Windows host (e.g. an MSYS2 build), its `File`
  # directive and the MUI bitmap/icon paths reject forward slashes ("no files
  # found"); they require backslash separators, and a mixed path still fails.
  # The template uses forward-slash @abs_top_srcdir@/@abs_top_builddir@ paths,
  # which are correct for the usual Linux/Guix cross-build (Linux makensis), so
  # only rewrite the project-rooted path tokens to native separators here. URLs,
  # NSIS flags (/SOLID, /REBOOTOK, ...) and $INSTDIR paths are untouched.
  #
  # Gated on the MSYS_STAGING environment variable (the same opt-in knob used by
  # the depends MSYS2 patches), so this stays a no-op for every other build. Set
  # `export MSYS_STAGING=1` in the shell before configuring when building on
  # MSYS2. See doc/build-windows-msys2.md.
  if(DEFINED ENV{MSYS_STAGING})
    file(READ ${nsi_out} _nsi_content)
    foreach(_root IN ITEMS "${abs_top_srcdir}" "${abs_top_builddir}")
      string(REGEX MATCHALL "${_root}[^ \t\r\n\"]*" _toks "${_nsi_content}")
      if(_toks)
        list(REMOVE_DUPLICATES _toks)
        foreach(_t IN LISTS _toks)
          string(REPLACE "/" "\\" _nt "${_t}")
          string(REPLACE "${_t}" "${_nt}" _nsi_content "${_nsi_content}")
        endforeach()
      endif()
    endforeach()
    file(WRITE ${nsi_out} "${_nsi_content}")
  endif()
endfunction()
