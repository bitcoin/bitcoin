#!/usr/bin/env python
#
#===- clang-format-diff.py - ClangFormat Diff Reformatter ----*- python -*--===#
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License.
#
#           ============================================================
#
# University of Illinois/NCSA
# Open Source License
#
# Copyright (c) 2007-2015 University of Illinois at Urbana-Champaign.
# All rights reserved.
#
# Developed by:
#
#     LLVM Team
#
#     University of Illinois at Urbana-Champaign
#
#     http://llvm.org
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal with
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do
# so, subject to the following conditions:
#
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimers.
#
#     * Redistributions in binary form must reproduce the above copyright notice,
#       this list of conditions and the following disclaimers in the
#       documentation and/or other materials provided with the distribution.
#
#     * Neither the names of the LLVM Team, University of Illinois at
#       Urbana-Champaign, nor the names of its contributors may be used to
#       endorse or promote products derived from this Software without specific
#       prior written permission.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
# SOFTWARE.
#
#           ============================================================
#
#===------------------------------------------------------------------------===#

r"""
ClangFormat Diff Reformatter
============================

This script reads input from a unified diff and reformats all the changed
lines. This is useful to reformat all the lines touched by a specific patch.
Example usage for git/svn users:

  git diff -U0 HEAD^ | clang-format-diff.py -p1 -i
  svn diff --diff-cmd=diff -x-U0 | clang-format-diff.py -i

"""

import argparse
import difflib
import re
import string
import subprocess
import StringIO
import sys


# Change this to the full path if clang-format is not on the path.
binary = 'clang-format'


def main():
  parser = argparse.ArgumentParser(description=
                                   'Reformat changed lines in diff. Without -i '
                                   'option just output the diff that would be '
                                   'introduced.')
  parser.add_argument('-i', action='store_true', default=False,
                      help='apply edits to files instead of displaying a diff')
  parser.add_argument('-p', metavar='NUM', default=0,
                      help='strip the smallest prefix containing P slashes')
  parser.add_argument('-regex', metavar='PATTERN', default=None,
                      help='custom pattern selecting file paths to reformat '
                      '(case sensitive, overrides -iregex)')
  parser.add_argument('-iregex', metavar='PATTERN', default=
                      r'.*\.(cpp|cc|c\+\+|cxx|c|cl|h|hpp|m|mm|inc|js|ts|proto'
                      r'|protodevel|java)',
                      help='custom pattern selecting file paths to reformat '
                      '(case insensitive, overridden by -regex)')
  parser.add_argument('-sort-includes', action='store_true', default=False,
                      help='let clang-format sort include blocks')
  parser.add_argument('-v', '--verbose', action='store_true',
                      help='be more verbose, ineffective without -i')
  args = parser.parse_args()

  # Extract changed lines for each file.
  filename = None
  lines_by_file = {}
  for line in sys.stdin:
    match = re.search('^\+\+\+\ (.*?/){%s}(\S*)' % args.p, line)
    if match:
      filename = match.group(2)
    if filename == None:
      continue

    if args.regex is not None:
      if not re.match('^%s$' % args.regex, filename):
        continue
    else:
      if not re.match('^%s$' % args.iregex, filename, re.IGNORECASE):
        continue

    match = re.search('^@@.*\+(\d+)(,(\d+))?', line)
    if match:
      start_line = int(match.group(1))
      line_count = 1
      if match.group(3):
        line_count = int(match.group(3))
      if line_count == 0:
        continue
      end_line = start_line + line_count - 1
      lines_by_file.setdefault(filename, []).extend(
          ['-lines', str(start_line) + ':' + str(end_line)])

  # Reformat files containing changes in place.
  for filename, lines in lines_by_file.iteritems():
    if args.i and args.verbose:
      print 'Formatting', filename
    command = [binary, filename]
    if args.i:
      command.append('-i')
    if args.sort_includes:
      command.append('-sort-includes')
    command.extend(lines)
    command.extend(['-style=file', '-fallback-style=none'])
    p = subprocess.Popen(command, stdout=subprocess.PIPE,
                         stderr=None, stdin=subprocess.PIPE)
    stdout, stderr = p.communicate()
    if p.returncode != 0:
      sys.exit(p.returncode)

    if not args.i:
        with open(filename, encoding="utf8") as f:
        code = f.readlines()
      formatted_code = StringIO.StringIO(stdout).readlines()
      diff = difflib.unified_diff(code, formatted_code,
                                  filename, filename,
                                  '(before formatting)', '(after formatting)')
      diff_string = string.join(diff, '')
      if len(diff_string) > 0:
        sys.stdout.write(diff_string)

if __name__ == '__main__':
  main()
