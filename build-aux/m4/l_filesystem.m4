m4_define([_CHECK_FILESYSTEM_testbody], [[
  #include <filesystem>

  namespace fs = std::filesystem;

  int main() {
    (void)fs::current_path().root_name();
    return 0;
  }
]])

AC_DEFUN([CHECK_FILESYSTEM], [

  AC_LANG_PUSH(C++)

  AC_MSG_CHECKING([whether std::filesystem can be used without link library])

  AC_LINK_IFELSE([AC_LANG_SOURCE([_CHECK_FILESYSTEM_testbody])],[
      AC_MSG_RESULT([yes])
    ],[
      AC_MSG_RESULT([no])
      LIBS="$LIBS -lstdc++fs"
      AC_MSG_CHECKING([whether std::filesystem needs -lstdc++fs])
      AC_LINK_IFELSE([AC_LANG_SOURCE([_CHECK_FILESYSTEM_testbody])],[
          AC_MSG_RESULT([yes])
        ],[
          AC_MSG_RESULT([no])
          AC_MSG_FAILURE([cannot figure out how to use std::filesystem])
        ])
    ])

  AC_LANG_POP
])
