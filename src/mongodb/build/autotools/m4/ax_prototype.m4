# ===========================================================================
#       https://www.gnu.org/software/autoconf-archive/ax_prototype.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_PROTOTYPE(function, includes, code, TAG1, values1 [, TAG2, values2 [...]])
#
# DESCRIPTION
#
#   Try all the combinations of <TAG1>, <TAG2>... to successfully compile
#   <code>. <TAG1>, <TAG2>, ... are substituted in <code> and <include> with
#   values found in <values1>, <values2>, ... respectively. <values1>,
#   <values2>, ... contain a list of possible values for each corresponding
#   tag and all combinations are tested. When AC_TRY_COMPILE(include, code)
#   is successfull for a given substitution, the macro stops and defines the
#   following macros: FUNCTION_TAG1, FUNCTION_TAG2, ... using AC_DEFINE()
#   with values set to the current values of <TAG1>, <TAG2>, ... If no
#   combination is successfull the configure script is aborted with a
#   message.
#
#   Intended purpose is to find which combination of argument types is
#   acceptable for a given function <function>. It is recommended to list
#   the most specific types first. For instance ARG1, [size_t, int] instead
#   of ARG1, [int, size_t].
#
#   Generic usage pattern:
#
#   1) add a call in configure.in
#
#    AX_PROTOTYPE(...)
#
#   2) call autoheader to see which symbols are not covered
#
#   3) add the lines in acconfig.h
#
#    /* Type of Nth argument of function */
#    #undef FUNCTION_ARGN
#
#   4) Within the code use FUNCTION_ARGN instead of an hardwired type
#
#   Complete example:
#
#   1) configure.in
#
#    AX_PROTOTYPE(getpeername,
#    [
#     #include <sys/types.h>
#     #include <sys/socket.h>
#    ],
#    [
#     int a = 0;
#     ARG2 * b = 0;
#     ARG3 * c = 0;
#     getpeername(a, b, c);
#    ],
#    ARG2, [struct sockaddr, void],
#    ARG3, [socklen_t, size_t, int, unsigned int, long unsigned int])
#
#   2) call autoheader
#
#    autoheader: Symbol `GETPEERNAME_ARG2' is not covered by ./acconfig.h
#    autoheader: Symbol `GETPEERNAME_ARG3' is not covered by ./acconfig.h
#
#   3) acconfig.h
#
#    /* Type of second argument of getpeername */
#    #undef GETPEERNAME_ARG2
#
#    /* Type of third argument of getpeername */
#    #undef GETPEERNAME_ARG3
#
#   4) in the code
#
#     ...
#     GETPEERNAME_ARG2 name;
#     GETPEERNAME_ARG3 namelen;
#     ...
#     ret = getpeername(socket, &name, &namelen);
#     ...
#
#   Implementation notes: generating all possible permutations of the
#   arguments is not easily done with the usual mixture of shell and m4,
#   that is why this macro is almost 100% m4 code. It generates long but
#   simple to read code.
#
# LICENSE
#
#   Copyright (c) 2009 Loic Dachary <loic@senga.org>
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation; either version 2 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <https://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

#serial 6

AU_ALIAS([AC_PROTOTYPE], [AX_PROTOTYPE])
AC_DEFUN([AX_PROTOTYPE],[
dnl
dnl Upper case function name
dnl
 pushdef([function],translit([$1], [a-z], [A-Z]))
dnl
dnl Collect tags that will be substituted
dnl
 pushdef([tags],[AX_PROTOTYPE_TAGS(builtin([shift],builtin([shift],builtin([shift],$@))))])
dnl
dnl Wrap in a 1 time loop, when a combination is found break to stop the combinatory exploration
dnl
 for i in 1
 do
   AX_PROTOTYPE_LOOP(AX_PROTOTYPE_REVERSE($1, AX_PROTOTYPE_SUBST($2,tags),AX_PROTOTYPE_SUBST($3,tags),builtin([shift],builtin([shift],builtin([shift],$@)))))
   AC_MSG_ERROR($1 unable to find a working combination)
 done
 popdef([tags])
 popdef([function])
])

dnl
dnl AX_PROTOTYPE_REVERSE(list)
dnl
dnl Reverse the order of the <list>
dnl
AC_DEFUN([AX_PROTOTYPE_REVERSE],[ifelse($#,0,,$#,1,[[$1]],[AX_PROTOTYPE_REVERSE(builtin([shift],$@)),[$1]])])

dnl
dnl AX_PROTOTYPE_SUBST(string, tag)
dnl
dnl Substitute all occurence of <tag> in <string> with <tag>_VAL.
dnl Assumes that tag_VAL is a macro containing the value associated to tag.
dnl
AC_DEFUN([AX_PROTOTYPE_SUBST],[ifelse($2,,[$1],[AX_PROTOTYPE_SUBST(patsubst([$1],[$2],[$2[]_VAL]),builtin([shift],builtin([shift],$@)))])])

dnl
dnl AX_PROTOTYPE_TAGS([tag, values, [tag, values ...]])
dnl
dnl Generate a list of <tag> by skipping <values>.
dnl
AC_DEFUN([AX_PROTOTYPE_TAGS],[ifelse($1,,[],[$1, AX_PROTOTYPE_TAGS(builtin([shift],builtin([shift],$@)))])])

dnl
dnl AX_PROTOTYPE_DEFINES(tags)
dnl
dnl Generate a AC_DEFINE(function_tag, tag_VAL) for each tag in <tags> list
dnl Assumes that function is a macro containing the name of the function in upper case
dnl and that tag_VAL is a macro containing the value associated to tag.
dnl
AC_DEFUN([AX_PROTOTYPE_DEFINES],[ifelse($1,,[],
	[AC_DEFINE(function[]_$1, $1_VAL, [ ])
	AC_SUBST(function[]_$1, "$1_VAL")
	AX_PROTOTYPE_DEFINES(builtin([shift],$@))])])

dnl
dnl AX_PROTOTYPE_STATUS(tags)
dnl
dnl Generates a message suitable for argument to AC_MSG_* macros. For each tag
dnl in the <tags> list the message tag => tag_VAL is generated.
dnl Assumes that tag_VAL is a macro containing the value associated to tag.
dnl
AC_DEFUN([AX_PROTOTYPE_STATUS],[ifelse($1,,[],[$1 => $1_VAL AX_PROTOTYPE_STATUS(builtin([shift],$@))])])

dnl
dnl AX_PROTOTYPE_EACH(tag, values)
dnl
dnl Call AX_PROTOTYPE_LOOP for each values and define the macro tag_VAL to
dnl the current value.
dnl
AC_DEFUN([AX_PROTOTYPE_EACH],[
  ifelse($2,, [
  ], [
    pushdef([$1_VAL], $2)
    AX_PROTOTYPE_LOOP(rest)
    popdef([$1_VAL])
    AX_PROTOTYPE_EACH($1, builtin([shift], builtin([shift], $@)))
  ])
])

dnl
dnl AX_PROTOTYPE_LOOP([tag, values, [tag, values ...]], code, include, function)
dnl
dnl If there is a tag/values pair, call AX_PROTOTYPE_EACH with it.
dnl If there is no tag/values pair left, tries to compile the code and include
dnl using AC_TRY_COMPILE. If it compiles, AC_DEFINE all the tags to their
dnl current value and exit with success.
dnl
AC_DEFUN([AX_PROTOTYPE_LOOP],[
 ifelse(builtin([eval], $# > 3), 1,
   [
     pushdef([rest],[builtin([shift],builtin([shift],$@))])
     AX_PROTOTYPE_EACH($2,$1)
     popdef([rest])
   ], [
     AC_MSG_CHECKING($3 AX_PROTOTYPE_STATUS(tags))
dnl
dnl Activate fatal warnings if possible, gives better guess
dnl
     ac_save_CPPFLAGS="$CPPFLAGS"
     if test "$GCC" = "yes" ; then CPPFLAGS="$CPPFLAGS -Werror" ; fi
     AC_TRY_COMPILE($2, $1, [
      CPPFLAGS="$ac_save_CPPFLAGS"
      AC_MSG_RESULT(ok)
      AX_PROTOTYPE_DEFINES(tags)
      break;
     ], [
      CPPFLAGS="$ac_save_CPPFLAGS"
      AC_MSG_RESULT(not ok)
     ])
   ]
 )
])
