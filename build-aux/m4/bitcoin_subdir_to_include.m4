dnl Copyright (c) 2013-2014 The Bitcoin Core developers
dnl Distributed under the MIT software license, see the accompanying
dnl file COPYING or http://www.opensource.org/licenses/mit-license.php.

dnl BITCOIN_SUBDIR_TO_INCLUDE([CPPFLAGS-VARIABLE-NAME],[SUBDIRECTORY-NAME],[HEADER-FILE])
dnl SUBDIRECTORY-NAME must end with a path separator
AC_DEFUN([BITCOIN_SUBDIR_TO_INCLUDE],[
  m4_pushdef([_result_var],[$1])
  m4_pushdef([_rel_path],[$2])
  m4_pushdef([_header_file],[$3.h])
  if test "x[]_rel_path" = "x"; then
    AC_MSG_RESULT([default])
  else
    echo '[#]include <'"_rel_path"'/_header_file>' >conftest.cpp
    newinclpath=$(
      ${CXXCPP} ${CPPFLAGS} -M conftest.cpp 2>/dev/null |
      ${SED} -E m4_bpatsubsts([[
        :build_line
# If the line doesn't end with a backslash, it is complete; go on to process it
        /\\$/!b have_complete_line
# Otherwise, read the next line, and concatenate it to the current one with a space
        N
        s/\\\n/ /
# Then go back and check for a trailing backslash again.
        t build_line

# When we get here, we have the completed line, with all continuations collapsed.
        :have_complete_line
        s/^[^:]*:[[:space:]]*(([^[:space:]\]|\\.)*[[:space:]])*(([^[:space:]\]|\\.)*)(\\|\\\\|\/)?]]patsubst(]_header_file[,[\.],[\\.])[[([[:space:]].*)?$/\3/
#         ^^^^^^^ The Make line begins with a target (which we don't care about)
#                ^^^^^^^^^^^^ Ignore any spaces following it
#                            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Match any number of other dependencies
#                                                              ^^^^^^^^^^^^^^^^^^^^^^ Match any path components for our dependency; note this is reference 3, which we are replacing with
#                                                                                    ^^^^^^^^^^^^^ Accept the path ending in a backslash, a double-backslash (ie escaped), or a forward slash
#                                                                                                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ The filename must match exactly (periods are escaped, since a normal period matches any character in regex)
#                                                                                                                                      ^^^^^^^^^^^^^^^^^ Filename must be followed by a space, but after that we don't care; we still need to match it all so it gets replaced, however
# Delete the line, but only if we failed to find the directory (t jumps past the d if we matched)
        t
        d
]],[
\s*\(#.*\)?$],[],[
        \(.*\)],[ -e '\1'])
dnl ^^^^^^^^^^^^^^^^^^^^^^^ Deletes comments and processes sed expressions into -e arguments
    )

    AC_MSG_RESULT([${newinclpath}])
    if test "x${newinclpath}" != "x"; then
      eval "_result_var=\"\$_result_var\"' -I${newinclpath}'"
    fi
  fi
  m4_popdef([_result_var],[_rel_path],[_header_file])
])
