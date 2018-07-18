# ===========================================================================
#   https://www.gnu.org/software/autoconf-archive/ax_subdirs_configure.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_SUBDIRS_CONFIGURE( [subdirs], [mandatory arguments], [possibly merged arguments], [replacement arguments], [forbidden arguments])
#
# DESCRIPTION
#
#   AX_SUBDIRS_CONFIGURE attempts to be the equivalent of AC_CONFIG_SUBDIRS
#   with customizable options for configure scripts.
#
#   Run the configure script for each directory from the comma-separated m4
#   list 'subdirs'. This macro can be used multiple times. All arguments of
#   this macro must be comma-separated lists.
#
#   All command line arguments from the parent configure script will be
#   given to the subdirectory configure script after the following
#   modifications (in that order):
#
#   1. The arguments from the 'mandatory arguments' list shall always be
#   appended to the argument list.
#
#   2. The arguments from the 'possibly merged arguments' list shall be
#   added if not present in the arguments of the parent configure script or
#   merged with the existing argument otherwise.
#
#   3. The arguments from the 'replacement arguments' list shall be added if
#   not present in the arguments of the parent configure script or replace
#   the existing argument otherwise.
#
#   4. The arguments from the 'forbidden arguments' list shall always be
#   removed from the argument list.
#
#   The lists 'mandatory arguments' and 'forbidden arguments' can hold any
#   kind of argument. The 'possibly merged arguments' and 'replacement
#   arguments' expect their arguments to be of the form --option-name=value.
#
#   This macro aims to remain as close as possible to the AC_CONFIG_SUBDIRS
#   macro. It corrects the paths for '--cache-file' and '--srcdir' and adds
#   '--disable-option-checking' and '--silent' if necessary.
#
#   This macro also sets the output variable subdirs_extra to the list of
#   directories recorded with AX_SUBDIRS_CONFIGURE. This variable can be
#   used in Makefile rules or substituted in configured files.
#
#   This macro shall do nothing more than managing the arguments of the
#   configure script. Just like when using AC_CONFIG_SUBDIRS, it is up to
#   the user to check any requirements or define and substitute any required
#   variable for the remainder of the project.
#
#   Configure scripts recorded with AX_SUBDIRS_CONFIGURE may be executed
#   before configure scripts recorded with AC_CONFIG_SUBDIRS.
#
#   Without additional arguments, the behaviour of AX_SUBDIRS_CONFIGURE
#   should be identical to the behaviour of AC_CONFIG_SUBDIRS, apart from
#   the contents of the variables subdirs and subdirs_extra (except that
#   AX_SUBDIRS_CONFIGURE expects a comma-separated m4 list):
#
#     AC_CONFIG_SUBDIRS([something])
#     AX_SUBDIRS_CONFIGURE([something])
#
#   This macro may be called multiple times.
#
#   Usage example:
#
#   Let us assume our project has 4 dependencies, namely A, B, C and D. Here
#   are some characteristics of our project and its dependencies:
#
#   - A does not require any special option.
#
#   - we want to build B with an optional feature which can be enabled with
#   its configure script's option '--enable-special-feature'.
#
#   - B's configure script is strange and has an option '--with-B=build'.
#   After close inspection of its documentation, we don't want B to receive
#   this option.
#
#   - C and D both need B.
#
#   - Just like our project, C and D can build B themselves with the option
#   '--with-B=build'.
#
#   - We want C and D to use the B we build instead of building it
#   themselves.
#
#   Our top-level configure script will be called as follows:
#
#     $ <path/to/configure> --with-A=build --with-B=build --with-C=build \
#       --with-D=build --some-option
#
#   Thus we have to make sure that:
#
#   - neither B, C or D receive the option '--with-B=build'
#
#   - C and D know where to find the headers and libraries of B.
#
#   Under those conditions, we can use the AC_CONFIG_SUBDIRS macro for A,
#   but need to use AX_SUBDIRS_CONFIGURE for B, C and D:
#
#   - B must receive '--enable-special-feature' but cannot receive
#   '--with-B=build'
#
#   - C and D cannot receive '--with-B=build' (or else it would be built
#   thrice) and need to be told where to find B (since we are building it,
#   it would probably not be available in standard paths).
#
#   Here is a configure.ac snippet that solves our problem:
#
#     AC_CONFIG_SUBDIRS([dependencies/A])
#     AX_SUBDIRS_CONFIGURE(
#         [dependencies/B], [--enable-special-feature], [], [],
#         [--with-B=build])
#     AX_SUBDIRS_CONFIGURE(
#         [[dependencies/C],[dependencies/D]],
#         [],
#         [[CPPFLAGS=-I${ac_top_srcdir}/dependencies/B -I${ac_top_builddir}/dependencies/B],
#          [LDFLAGS=-L${ac_abs_top_builddir}/dependencies/B/.libs]],
#         [--with-B=system],
#         [])
#
#   If using automake, the following can be added to the Makefile.am (we use
#   both $(subdirs) and $(subdirs_extra) since our example above used both
#   AC_CONFIG_SUBDIRS and AX_SUBDIRS_CONFIGURE):
#
#     SUBDIRS = $(subdirs) $(subdirs_extra)
#
# LICENSE
#
#   Copyright (c) 2017 Harenome Ranaivoarivony-Razanajato <ranaivoarivony-razanajato@hareno.me>
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation; either version 3 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   Under Section 7 of GPL version 3, you are granted additional permissions
#   described in the Autoconf Configure Script Exception, version 3.0, as
#   published by the Free Software Foundation.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <https://www.gnu.org/licenses/>.

#serial 4

AC_DEFUN([AX_SUBDIRS_CONFIGURE],
[
  dnl Calls to AC_CONFIG_SUBDIRS perform preliminary steps and build a list
  dnl '$subdirs' which is used later by _AC_OUTPUT_SUBDIRS (used by AC_OUTPUT)
  dnl to actually run the configure scripts.
  dnl This macro performs similiar preliminary steps but uses
  dnl AC_CONFIG_COMMANDS_PRE to delay the final tasks instead of building an
  dnl intermediary list and relying on another macro.
  dnl
  dnl Since each configure script can get different options, a special variable
  dnl named 'ax_sub_configure_args_<subdir>' is constructed for each
  dnl subdirectory.

  # Various preliminary checks.
  AC_REQUIRE([AC_DISABLE_OPTION_CHECKING])
  AC_REQUIRE([AC_CONFIG_AUX_DIR_DEFAULT])
  AS_LITERAL_IF([$1], [],
      [AC_DIAGNOSE([syntax], [$0: you should use literals])])

  m4_foreach(subdir_path, [$1],
  [
    ax_dir="subdir_path"

    dnl Build the argument list in a similiar fashion to AC_CONFIG_SUBDIRS.
    dnl A few arguments found in the final call to the configure script are not
    dnl added here because they rely on variables that may not yet be available
    dnl (see below the part that is similiar to _AC_OUTPUT_SUBDIRS).
    # Do not complain, so a configure script can configure whichever parts of a
    # large source tree are present.
    if test -d "$srcdir/$ax_dir"; then
      _AC_SRCDIRS(["$ax_dir"])
      # Remove --cache-file, --srcdir, and --disable-option-checking arguments
      # so they do not pile up.
      ax_args=
      ax_prev=
      eval "set x $ac_configure_args"
      shift
      for ax_arg; do
        if test -n "$ax_prev"; then
          ax_prev=
          continue
        fi
        case $ax_arg in
          -cache-file | --cache-file | --cache-fil | --cache-fi | --cache-f \
          | --cache- | --cache | --cach | --cac | --ca | --c)
            ax_prev=cache_file ;;
          -cache-file=* | --cache-file=* | --cache-fil=* | --cache-fi=* \
          | --cache-f=* | --cache-=* | --cache=* | --cach=* | --cac=* | --ca=* \
          | --c=*)
            ;;
          --config-cache | -C)
            ;;
          -srcdir | --srcdir | --srcdi | --srcd | --src | --sr)
            ax_prev=srcdir ;;
          -srcdir=* | --srcdir=* | --srcdi=* | --srcd=* | --src=* | --sr=*)
            ;;
          -prefix | --prefix | --prefi | --pref | --pre | --pr | --p)
            ax_prev=prefix ;;
          -prefix=* | --prefix=* | --prefi=* | --pref=* | --pre=* | --pr=* \
          | --p=*)
            ;;
          --disable-option-checking)
            ;;
          *) case $ax_arg in
              *\'*) ax_arg=$(AS_ECHO(["$ax_arg"]) | sed "s/'/'\\\\\\\\''/g");;
            esac
            AS_VAR_APPEND([ax_args], [" '$ax_arg'"]) ;;
        esac
      done
      # Always prepend --disable-option-checking to silence warnings, since
      # different subdirs can have different --enable and --with options.
      ax_args="--disable-option-checking $ax_args"
      # Options that must be added as they are provided.
      m4_ifnblank([$2], [m4_foreach(opt, [$2], [AS_VAR_APPEND(ax_args, " 'opt'")
      ])])
      # New options that may need to be merged with existing options.
      m4_ifnblank([$3], [m4_foreach(opt, [$3],
          [ax_candidate="opt"
           ax_candidate_flag="${ax_candidate%%=*}"
           ax_candidate_content="${ax_candidate#*=}"
           if test "x$ax_candidate" != "x" -a "x$ax_candidate_flag" != "x"; then
             if echo "$ax_args" | grep -- "${ax_candidate_flag}=" >/dev/null 2>&1; then
               [ax_args=$(echo $ax_args | sed "s,\(${ax_candidate_flag}=[^']*\),\1 ${ax_candidate_content},")]
             else
               AS_VAR_APPEND(ax_args, " 'opt'")
             fi
           fi
      ])])
      # New options that must replace existing options.
      m4_ifnblank([$4], [m4_foreach(opt, [$4],
          [ax_candidate="opt"
           ax_candidate_flag="${ax_candidate%%=*}"
           ax_candidate_content="${ax_candidate#*=}"
           if test "x$ax_candidate" != "x" -a "x$ax_candidate_flag" != "x"; then
             if echo "$ax_args" | grep -- "${ax_candidate_flag}=" >/dev/null 2>&1; then
               [ax_args=$(echo $ax_args | sed "s,${ax_candidate_flag}=[^']*,${ax_candidate},")]
             else
               AS_VAR_APPEND(ax_args, " 'opt'")
             fi
           fi
      ])])
      # Options that must be removed.
      m4_ifnblank([$5], [m4_foreach(opt, [$5], [ax_args=$(echo $ax_args | sed "s,'opt',,")
      ])])
      AS_VAR_APPEND([ax_args], [" '--srcdir=$ac_srcdir'"])

      # Add the subdirectory to the list of target subdirectories.
      ax_subconfigures="$ax_subconfigures $ax_dir"
      # Save the argument list for this subdirectory.
      dnl $1 is a path to some subdirectory: m4_bpatsubsts() is used to convert
      dnl $1 into a valid shell variable name.
      dnl For instance, "ax_sub_configure_args_path/to/subdir" becomes
      dnl "ax_sub_configure_args_path_to_subdir".
      ax_var=$(printf "$ax_dir" | tr -c "0-9a-zA-Z_" "_")
      eval "ax_sub_configure_args_$ax_var=\"$ax_args\""
      eval "ax_sub_configure_$ax_var=\"yes\""
    else
      AC_MSG_WARN([could not find source tree for $ax_dir])
    fi

    dnl Add some more arguments to the argument list and then actually run the
    dnl configure script. This is mostly what happens in _AC_OUTPUT_SUBDIRS
    dnl except it does not iterate over an intermediary list.
    AC_CONFIG_COMMANDS_PRE(
      dnl This very line cannot be quoted! m4_foreach has some work here.
      ax_dir="subdir_path"
    [
      # Convert the path to the subdirectory into a shell variable name.
      ax_var=$(printf "$ax_dir" | tr -c "0-9a-zA-Z_" "_")
      ax_configure_ax_var=$(eval "echo \"\$ax_sub_configure_$ax_var\"")
      if test "$no_recursion" != "yes" -a "x$ax_configure_ax_var" = "xyes"; then
        AC_SUBST([subdirs_extra], ["$subdirs_extra $ax_dir"])
        ax_msg="=== configuring in $ax_dir ($(pwd)/$ax_dir)"
        _AS_ECHO_LOG([$ax_msg])
        _AS_ECHO([$ax_msg])
        AS_MKDIR_P(["$ax_dir"])
        _AC_SRCDIRS(["$ax_dir"])

        ax_popdir=$(pwd)
        cd "$ax_dir"

        # Check for guested configure; otherwise get Cygnus style configure.
        if test -f "$ac_srcdir/configure.gnu"; then
          ax_sub_configure=$ac_srcdir/configure.gnu
        elif test -f "$ac_srcdir/configure"; then
          ax_sub_configure=$ac_srcdir/configure
        elif test -f "$ac_srcdir/configure.in"; then
          # This should be Cygnus configure.
          ax_sub_configure=$ac_aux_dir/configure
        else
          AC_MSG_WARN([no configuration information is in $ax_dir])
          ax_sub_configure=
        fi

        if test -n "$ax_sub_configure"; then
          # Get the configure arguments for the current configure.
          eval "ax_sub_configure_args=\"\$ax_sub_configure_args_${ax_var}\""

          # Always prepend --prefix to ensure using the same prefix
          # in subdir configurations.
          ax_arg="--prefix=$prefix"
          case $ax_arg in
            *\'*) ax_arg=$(AS_ECHO(["$ax_arg"]) | sed "s/'/'\\\\\\\\''/g");;
          esac
          ax_sub_configure_args="'$ax_arg' $ax_sub_configure_args"
          if test "$silent" = yes; then
            ax_sub_configure_args="--silent $ax_sub_configure_args"
          fi
          # Make the cache file name correct relative to the subdirectory.
          case $cache_file in
            [[\\/]]* | ?:[[\\/]]* )
              ax_sub_cache_file=$cache_file ;;
            *) # Relative name.
              ax_sub_cache_file=$ac_top_build_prefix$cache_file ;;
          esac

          AC_MSG_NOTICE([running $SHELL $ax_sub_configure $ax_sub_configure_args --cache-file=$ac_sub_cache_file])
          eval "\$SHELL \"$ax_sub_configure\" $ax_sub_configure_args --cache-file=\"$ax_sub_cache_file\"" \
              || AC_MSG_ERROR([$ax_sub_configure failed for $ax_dir])
        fi

        cd "$ax_popdir"
      fi
    ])
  ])
])
