# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

function(separate_configs options)
  string(GENEX_STRIP "${${options}}" ${options}_ALL)
  string(REPLACE ";" " " ${options}_ALL "${${options}_ALL}")
  set(${options}_ALL "${${options}_ALL}" PARENT_SCOPE)

  foreach(conf "RelWithDebInfo" "Release" "Debug" "MinSizeRel")
    string(TOUPPER "${conf}" conf_upper)
    set(var ${options}_${conf_upper})
    string(REGEX MATCHALL "\\$<\\$<CONFIG:${conf}>[^\n]*>" ${var} "${${options}}")
    string(REPLACE "\$<\$<CONFIG:${conf}>:" "" ${var} "${${var}}")
    string(REPLACE ">" "" ${var} "${${var}}")
    string(REPLACE ";" " " ${var} "${${var}}")
    set(${var} "${${var}}" PARENT_SCOPE)
  endforeach()
endfunction()
