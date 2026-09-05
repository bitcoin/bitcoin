# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)
include(GNUInstallDirs)

function(install_binary_component component)
  cmake_parse_arguments(PARSE_ARGV 1
    IC                          # prefix
    "HAS_MANPAGE;INTERNAL"      # options
    ""                          # one_value_keywords
    ""                          # multi_value_keywords
  )
  set(target_name ${component})
  if(IC_INTERNAL)
    set(runtime_dest ${CMAKE_INSTALL_LIBEXECDIR})
  else()
    set(runtime_dest ${CMAKE_INSTALL_BINDIR})
  endif()
  install(TARGETS ${target_name}
    RUNTIME DESTINATION ${runtime_dest}
    COMPONENT ${component}
  )
  if(INSTALL_MAN AND IC_HAS_MANPAGE)
    install(FILES ${PROJECT_SOURCE_DIR}/doc/man/${target_name}.1
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      COMPONENT ${component}
    )
  endif()
  if(INSTALL_COMPLETIONS)
    set(bash_comp ${PROJECT_SOURCE_DIR}/contrib/completions/bash/${target_name}.bash)
    if(EXISTS ${bash_comp})
      install(FILES ${bash_comp}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME ${target_name}
        COMPONENT ${component}
      )
    endif()
    set(fish_comp ${PROJECT_SOURCE_DIR}/contrib/completions/fish/${target_name}.fish)
    if(EXISTS ${fish_comp})
      install(FILES ${fish_comp}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/fish/vendor_completions.d
        COMPONENT ${component}
      )
    endif()
  endif()
endfunction()
