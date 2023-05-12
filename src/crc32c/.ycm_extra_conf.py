# Copyright 2017 The CRC32C Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""YouCompleteMe configuration that interprets a .clang_complete file.

This module implementes the YouCompleteMe configuration API documented at:
https://github.com/ycm-core/ycmd#ycm_extra_confpy-specification

The implementation loads and processes a .clang_complete file, documented at:
https://github.com/xavierd/clang_complete/blob/master/README.md
"""

import os

# Flags added to the list in .clang_complete.
BASE_FLAGS = [
    '-Werror',  # Unlike clang_complete, YCM can also be used as a linter.
    '-DUSE_CLANG_COMPLETER',  # YCM needs this.
    '-xc++',  # YCM needs this to avoid compiling headers as C code.
]

# Clang flags that take in paths.
# See https://clang.llvm.org/docs/ClangCommandLineReference.html
PATH_FLAGS = [
    '-isystem',
    '-I',
    '-iquote',
    '--sysroot='
]


def DirectoryOfThisScript():
  """Returns the absolute path to the directory containing this script."""
  return os.path.dirname(os.path.abspath(__file__))


def MakeRelativePathsInFlagsAbsolute(flags, build_root):
  """Expands relative paths in a list of Clang command-line flags.

  Args:
    flags: The list of flags passed to Clang.
    build_root: The current directory when running the Clang compiler. Should be
        an absolute path.

  Returns:
    A list of flags with relative paths replaced by absolute paths.
  """
  new_flags = []
  make_next_absolute = False
  for flag in flags:
    new_flag = flag

    if make_next_absolute:
      make_next_absolute = False
      if not flag.startswith('/'):
        new_flag = os.path.join(build_root, flag)

    for path_flag in PATH_FLAGS:
      if flag == path_flag:
        make_next_absolute = True
        break

      if flag.startswith(path_flag):
        path = flag[len(path_flag):]
        new_flag = path_flag + os.path.join(build_root, path)
        break

    if new_flag:
      new_flags.append(new_flag)
  return new_flags


def FindNearest(target, path, build_root):
  """Looks for a file with a specific name closest to a project path.

  This is similar to the logic used by a version-control system (like git) to
  find its configuration directory (.git) based on the current directory when a
  command is invoked.

  Args:
    target: The file name to search for.
    path: The directory where the search starts. The search will explore the
        given directory's ascendants using the parent relationship. Should be an
        absolute path.
    build_root: A directory that acts as a fence for the search. If the search
        reaches this directory, it will not advance to its parent. Should be an
        absolute path.

  Returns:
    The path to a file with the desired name. None if the search failed.
  """
  candidate = os.path.join(path, target)
  if os.path.isfile(candidate):
    return candidate

  if path == build_root:
    return None

  parent = os.path.dirname(path)
  if parent == path:
    return None

  return FindNearest(target, parent, build_root)


def FlagsForClangComplete(file_path, build_root):
  """Reads the .clang_complete flags for a source file.

  Args:
    file_path: The path to the source file. Should be inside the project. Used
      to locate the relevant .clang_complete file.
    build_root: The current directory when running the Clang compiler for this
        file. Should be an absolute path.

  Returns:
    A list of strings, where each element is a Clang command-line flag.
  """
  clang_complete_path = FindNearest('.clang_complete', file_path, build_root)
  if clang_complete_path is None:
    return None
  clang_complete_flags = open(clang_complete_path, 'r').read().splitlines()
  return clang_complete_flags


def FlagsForFile(filename, **kwargs):
  """Implements the YouCompleteMe API."""

  # kwargs can be used to pass 'client_data' to the YCM configuration. This
  # configuration script does not need any extra information, so
  # pylint: disable=unused-argument

  build_root = DirectoryOfThisScript()
  file_path = os.path.realpath(filename)

  flags = BASE_FLAGS
  clang_flags = FlagsForClangComplete(file_path, build_root)
  if clang_flags:
    flags += clang_flags

  final_flags = MakeRelativePathsInFlagsAbsolute(flags, build_root)

  return {'flags': final_flags}
