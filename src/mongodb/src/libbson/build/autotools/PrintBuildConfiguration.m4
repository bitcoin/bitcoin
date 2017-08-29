AC_OUTPUT

if test -n "$BSON_PRERELEASE_VERSION"; then
cat << EOF
 *** IMPORTANT *** 

 This is an unstable version of libbson.
 It is for test purposes only.

 Please, DO NOT use it in a production environment.
 It will probably crash and you will lose your data.

 Additionally, the API/ABI may change during the course
 of development.

 Thanks,

   The libbson team.

 *** END OF WARNING ***

EOF
fi

echo "
libbson $BSON_VERSION was configured with the following options:

Build configuration:
  Enable debugging (slow)                          : ${enable_debug}
  Enable extra alignment (required for 1.0 ABI)    : ${enable_extra_align}
  Compile with debug symbols (slow)                : ${enable_debug_symbols}
  Enable GCC build optimization                    : ${enable_optimizations}
  Code coverage support                            : ${enable_coverage}
  Cross Compiling                                  : ${enable_crosscompile}
  Big endian                                       : ${enable_bigendian}
  Link Time Optimization (experimental)            : ${enable_lto}

Documentation:
  man                                              : ${enable_man_pages}
  HTML                                             : ${enable_html_docs}
"
