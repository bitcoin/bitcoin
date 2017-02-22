#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import re
import fnmatch
import sys
import subprocess
import datetime
import os


###############################################################################
# helper for converting fnmatch expressions into python regexes and compiling
###############################################################################


def compile_fnmatches(fnmatches):
    if fnmatches is None or len(fnmatches) == 0:
        # a regex that matches nothing (including not matching an empty string)
        return re.compile('(?!)')
    return re.compile('|'.join([fnmatch.translate(m) for m in fnmatches]))


###############################################################################
# common constants for regexes
###############################################################################


# this script is only applied to files in 'git ls-files' of these extensions:
SOURCE_FILES = ['*.h', '*.cpp', '*.cc', '*.c', '*.py', '*.sh', '*.am', '*.m4',
                '*.include']

SOURCE_FILES_COMPILED = compile_fnmatches(SOURCE_FILES)

ALWAYS_IGNORE = [
    # empty files for python init:
    '*__init__.py',
    # files in subtrees:
    'src/secp256k1/*',
    'src/leveldb/*',
    'src/univalue/*',
    'src/crypto/ctaes/*',
]

ALWAYS_IGNORE_COMPILED = compile_fnmatches(ALWAYS_IGNORE)

YEAR = "20[0-9][0-9]"
YEAR_RANGE = '(?P<start_year>%s)(-(?P<end_year>%s))?' % (YEAR, YEAR)

YEAR_RANGE_COMPILED = re.compile(YEAR_RANGE)


###############################################################################
# header regex and ignore list for the base bitcoin core repository
###############################################################################


HOLDERS = [
    "Satoshi Nakamoto",
    "The Bitcoin Core developers",
    "Pieter Wuille",
    "Wladimir J\\. van der Laan",
    "Jeff Garzik",
    "BitPay Inc\\.",
    "MarcoFalke",
    "ArtForz -- public domain half-a-node",
    "Jeremy Rubin",
]
ANY_HOLDER = '|'.join([h for h in HOLDERS])
COPYRIGHT_LINE = (
    "(#|//|dnl) Copyright \\(c\\) %s (%s)" % (YEAR_RANGE, ANY_HOLDER))
LAST_TWO_LINES = ("(#|//|dnl) Distributed under the MIT software license, see "
                  "the accompanying\n(#|//|dnl) file COPYING or "
                  "http://www\\.opensource\\.org/licenses/mit-license\\.php\\."
                  "\n")

HEADER = "(%s\n)+%s" % (COPYRIGHT_LINE, LAST_TWO_LINES)

HEADER_COMPILED = re.compile(HEADER)

NO_HEADER_EXPECTED = [
    # build scripts
    'doc/man/Makefile.am',
    'build-aux/m4/ax_boost_base.m4',
    'build-aux/m4/ax_boost_chrono.m4',
    'build-aux/m4/ax_boost_filesystem.m4',
    'build-aux/m4/ax_boost_program_options.m4',
    'build-aux/m4/ax_boost_system.m4',
    'build-aux/m4/ax_boost_thread.m4',
    'build-aux/m4/ax_boost_unit_test_framework.m4',
    'build-aux/m4/ax_check_compile_flag.m4',
    'build-aux/m4/ax_check_link_flag.m4',
    'build-aux/m4/ax_check_preproc_flag.m4',
    'build-aux/m4/ax_cxx_compile_stdcxx.m4',
    'build-aux/m4/ax_gcc_func_attribute.m4',
    'build-aux/m4/ax_pthread.m4',
    'build-aux/m4/l_atomic.m4',
    # auto generated files:
    'src/qt/bitcoinstrings.cpp',
    'src/chainparamsseeds.h',
    # other copyright notices:
    'src/tinyformat.h',
    'qa/rpc-tests/test_framework/bignum.py',
    'contrib/devtools/clang-format-diff.py',
    'qa/rpc-tests/test_framework/authproxy.py',
    'qa/rpc-tests/test_framework/key.py',
]

NO_HEADER_EXPECTED_COMPILED = compile_fnmatches(NO_HEADER_EXPECTED)

OTHER_COPYRIGHT_EXPECTED = [
    # Uses of the word 'copyright' that are unrelated to the header:
    'contrib/devtools/copyright_header.py',
    'contrib/devtools/gen-manpages.sh',
    'share/qt/extract_strings_qt.py',
    'src/Makefile.qt.include',
    'src/clientversion.h',
    'src/init.cpp',
    'src/qt/bitcoinstrings.cpp',
    'src/qt/splashscreen.cpp',
    'src/util.cpp',
    'src/util.h',
    # other, non-core copyright notices:
    'src/tinyformat.h',
    'contrib/devtools/clang-format-diff.py',
    'qa/rpc-tests/test_framework/authproxy.py',
    'qa/rpc-tests/test_framework/key.py',
    'contrib/devtools/git-subtree-check.sh',
    'build-aux/m4/l_atomic.m4',
    # build scripts:
    'build-aux/m4/ax_boost_base.m4',
    'build-aux/m4/ax_boost_chrono.m4',
    'build-aux/m4/ax_boost_filesystem.m4',
    'build-aux/m4/ax_boost_program_options.m4',
    'build-aux/m4/ax_boost_system.m4',
    'build-aux/m4/ax_boost_thread.m4',
    'build-aux/m4/ax_boost_unit_test_framework.m4',
    'build-aux/m4/ax_check_compile_flag.m4',
    'build-aux/m4/ax_check_link_flag.m4',
    'build-aux/m4/ax_check_preproc_flag.m4',
    'build-aux/m4/ax_cxx_compile_stdcxx.m4',
    'build-aux/m4/ax_gcc_func_attribute.m4',
    'build-aux/m4/ax_pthread.m4',
]

OTHER_COPYRIGHT_EXPECTED_COMPILED = compile_fnmatches(OTHER_COPYRIGHT_EXPECTED)


###############################################################################
# obtain list of files in repo to check that match extensions
###############################################################################


GIT_LS_CMD = 'git ls-files'


def git_ls():
    out = subprocess.check_output(GIT_LS_CMD.split(' '))
    return [f for f in out.decode("utf-8").split('\n') if f != '']


SOURCE_FILES_COMPILED = re.compile('|'.join([fnmatch.translate(match)
                                             for match in SOURCE_FILES]))

ALWAYS_IGNORE_COMPILED = re.compile('|'.join([fnmatch.translate(match)
                                              for match in ALWAYS_IGNORE]))


def filename_is_to_be_examined(filename):
    return (SOURCE_FILES_COMPILED.match(filename) and not
            ALWAYS_IGNORE_COMPILED.match(filename))


def get_filenames_to_examine(full_file_list):
    return sorted([filename for filename in full_file_list if
                   filename_is_to_be_examined(filename)])


###############################################################################
# detect if file contents have the copyright header in the right place
###############################################################################


def starts_with_shebang(contents):
    if len(contents) < 2:
        return False
    return contents[:2] == '#!'


def header_match_in_correct_place(contents, header_match):
    start = header_match.start(0)
    shebang = starts_with_shebang(contents)
    if start == 0:
        return not shebang
    return shebang and (contents[:start].count('\n') == 1)


def file_has_header(contents):
    header_match = HEADER_COMPILED.search(contents)
    if not header_match:
        return False
    return header_match_in_correct_place(contents, header_match)


###############################################################################
# detect if file has a copyright message in a place outside the header
###############################################################################


OTHER_COPYRIGHT = "(Copyright|COPYRIGHT|copyright)"
OTHER_COPYRIGHT_COMPILED = re.compile(OTHER_COPYRIGHT)


def has_copyright_in_region(contents_region):
    return OTHER_COPYRIGHT_COMPILED.search(contents_region)


def file_has_other_copyright(contents):
    # look for the OTHER_COPYRIGHT regex outside the normal header regex match
    header_match = HEADER_COMPILED.search(contents)
    if header_match:
        return has_copyright_in_region(contents[header_match.end():])
    return has_copyright_in_region(contents)


###############################################################################
# file IO
###############################################################################


def read_file(filename):
    file = open(os.path.abspath(filename), 'r')
    contents = file.read()
    file.close()
    return contents


def write_file(filename, contents):
    file = open(os.path.abspath(filename), 'w')
    file.write(contents)
    file.close()


###############################################################################
# get file info
###############################################################################


FAILURE_REASON_1 = {
    'description': "A valid header was expected, but the file does not match "
                   "the regex",
    'resolution': """
A correct MIT License header copyrighted by 'The Bitcoin Core developers' in
the present year can be inserted into a file by running:

    $ ./contrib/devtools/copyright_header.py insert <filename>

If there was a preexisting invalid header in the file, that will need to be
manually deleted. If there is a new copyright holder for the MIT License, the
holder will need to be added to the HOLDERS list to include it in the regex
check.
"""
}

FAILURE_REASON_2 = {
    'description': "A valid header was found in the file, but it wasn't "
                   "expected",
    'resolution': """
The header was not expected due to a setting in copyright_header.py. If a valid
copyright header has been added to the file, the filename can be removed from
the NO_HEADER_EXPECTED listing.
"""
}

FAILURE_REASON_3 = {
    'description': "Another 'copyright' occurrence was found, but it wasn't "
                   "expected",
    'resolution': """
This file's body has a regular expression match for the (case-sensitive) words
"Copyright", "COPYRIGHT" or 'copyright". If this was an appropriate addition,
copyright_header.py can be edited to add the file to the
OTHER_COPYRIGHT_EXPECTED listing.
"""
}

FAILURE_REASON_4 = {
    'description': "Another 'copyright' occurrence was expected, but wasn't "
                   "found.",
    'resolution': """
A use of the (case-sensitive) words "Copyright", "COPYRIGHT", or 'copyright'
outside of the regular copyright header was expected due to a setting in
copyright_header.py but it was not found. If this text was appropriately
removed from the file, copyright_header.py can be edited to remove the file
from the OTHER_COPYRIGHT_EXPECTED listing.
"""
}

FAILURE_REASONS = [FAILURE_REASON_1, FAILURE_REASON_2, FAILURE_REASON_3,
                   FAILURE_REASON_4]

NO_FAILURE = {
    'description': "Everything is excellent",
    'resolution': "(none)"
}


def evaluate(file_info):
    if not file_info['has_header'] and file_info['hdr_expected']:
        return FAILURE_REASON_1
    if file_info['has_header'] and not file_info['hdr_expected']:
        return FAILURE_REASON_2
    if file_info['has_other'] and not file_info['other_copyright_expected']:
        return FAILURE_REASON_3
    if not file_info['has_other'] and file_info['other_copyright_expected']:
        return FAILURE_REASON_4
    return NO_FAILURE


def gather_file_info(filename):
    info = {}
    info['filename'] = filename
    info['contents'] = read_file(filename)
    info['hdr_expected'] = not NO_HEADER_EXPECTED_COMPILED.match(filename)
    info['other_copyright_expected'] = (
        OTHER_COPYRIGHT_EXPECTED_COMPILED.match(filename))
    info['has_header'] = file_has_header(info['contents'])
    info['has_other'] = file_has_other_copyright(info['contents'])
    info['evaluation'] = evaluate(info)
    info['pass'] = info['evaluation'] is NO_FAILURE
    return info


###############################################################################
# report execution
###############################################################################


SEPARATOR = 80 * '-' + '\n'
REPORT = []


def report(string):
    REPORT.append(string)


def report_filenames(file_infos):
    if len(file_infos) == 0:
        return
    report('\t')
    report('\n\t'.join([file_info['filename'] for file_info in file_infos]))
    report('\n')


def report_summary(file_infos, full_file_list):
    report("%4d files tracked according to '%s'\n" %
           (len(full_file_list), GIT_LS_CMD))
    report("%4d files examined according to SOURCE_FILES and ALWAYS_IGNORE "
           "fnmatch rules\n" % len(file_infos))


def report_failure_reason(reason, failed_file_infos):
    report('Reason - "%s":\n' % reason['description'])
    report_filenames(failed_file_infos)


def report_failure_reasons(failed_file_infos):
    for failure_reason in FAILURE_REASONS:
        file_infos = [file_info for file_info in failed_file_infos if
                      file_info['evaluation'] is failure_reason]
        if len(file_infos) == 0:
            continue
        report_failure_reason(failure_reason, file_infos)


def report_details(file_infos):
    hdr_expected = [file_info for file_info in file_infos if
                    file_info['hdr_expected']]
    no_hdr_expected = [file_info for file_info in file_infos if not
                       file_info['hdr_expected']]
    no_other_copyright_expected = [file_info for file_info in file_infos if not
                                   file_info['other_copyright_expected']]
    other_copyright_expected = [file_info for file_info in file_infos if
                                file_info['other_copyright_expected']]
    failed = [file_info for file_info in file_infos if not file_info['pass']]
    passed = [file_info for file_info in file_infos if file_info['pass']]

    report("Files expected to have header:                                    "
           "%4d\n" % len(hdr_expected))
    report("Files not expected to have header:                                "
           "%4d\n" % len(no_hdr_expected))
    report("Files not expected to have 'copyright' occurrence outside header: "
           "%4d\n" % len(no_other_copyright_expected))
    report("Files expected to have 'copyright' occurrence outside header:     "
           "%4d\n" % len(other_copyright_expected))
    report("Files passed:                                                     "
           "%4d\n" % len(passed))
    report("Files failed:                                                     "
           "%4d\n" % len(failed))
    report_failure_reasons(failed)


def print_report(file_infos, full_file_list):
    report(SEPARATOR)
    report_summary(file_infos, full_file_list)
    report(SEPARATOR)
    report_details(file_infos)
    report(SEPARATOR)
    print(''.join(REPORT), end="")


def exec_report(base_directory):
    original_cwd = os.getcwd()
    os.chdir(base_directory)
    full_file_list = git_ls()
    file_infos = [gather_file_info(filename) for filename in
                  get_filenames_to_examine(full_file_list)]
    print_report(file_infos, full_file_list)
    os.chdir(original_cwd)


###############################################################################
# report cmd
###############################################################################


REPORT_USAGE = """
Produces a report of all copyright header notices found inside the source files
of a repository.

Usage:
    $ ./copyright_header.py report <base_directory>

Arguments:
    <base_directory> - The base directory of a bitcoin core source code
    repository.
"""


def report_cmd(argv):
    if len(argv) != 3:
        sys.exit(REPORT_USAGE)

    base_directory = argv[2]
    if not os.path.exists(base_directory):
        sys.exit("*** bad <base_directory>: %s" % base_directory)

    exec_report(base_directory)


###############################################################################
# check execution
###############################################################################


GREEN = '\033[92m'
RED = '\033[91m'
ENDC = '\033[0m'


def red_report(string):
    report(RED + string + ENDC)


def green_report(string):
    report(GREEN + string + ENDC)


def get_failures(file_infos):
    return [file_info for file_info in file_infos if not file_info['pass']]


def report_failure(failure):
    report("An issue was found with ")
    red_report("%s" % failure['filename'])
    report('\n\n%s\n\n' % failure['evaluation']['description'])
    report('Info for resolution:\n')
    report(failure['evaluation']['resolution'])


def print_check_report(full_file_list, file_infos, failures):
    report(SEPARATOR)
    report_summary(file_infos, full_file_list)
    for failure in failures:
        report(SEPARATOR)
        report_failure(failure)
    report(SEPARATOR)
    if len(failures) == 0:
        green_report("No copyright header issues found!\n")
        report(SEPARATOR)
    print(''.join(REPORT), end="")


def exec_check(base_directory):
    original_cwd = os.getcwd()
    os.chdir(base_directory)
    full_file_list = git_ls()
    file_infos = [gather_file_info(filename) for filename in
                  get_filenames_to_examine(full_file_list)]
    failures = get_failures(file_infos)
    print_check_report(full_file_list, file_infos, failures)
    os.chdir(original_cwd)
    if len(failures) > 0:
        sys.exit("*** Copyright header issues found!")


###############################################################################
# check cmd
###############################################################################


CHECK_USAGE = """
Checks over the repository for any issues with the copyright header. Returns a
non-zero status if there are any issues found. Also, a report is printed
specifically identifying which files are problematic and a suggestion for what
can be done to resolve the issue.

Usage:
    $ ./copyright_header.py check <base_directory>

Arguments:
    <base_directory> - The base directory of a bitcoin core source code
    repository.
"""


def check_cmd(argv):
    if len(argv) != 3:
        sys.exit(CHECK_USAGE)

    base_directory = argv[2]
    if not os.path.exists(base_directory):
        sys.exit("*** bad <base_directory>: %s" % base_directory)

    exec_check(base_directory)


###############################################################################
# query git for year of last change
###############################################################################


GIT_LOG_CMD = "git log --follow --pretty=format:%%ai %s"


def call_git_log(filename):
    out = subprocess.check_output((GIT_LOG_CMD % filename).split(' '))
    decoded = out.decode("utf-8")
    if decoded == '':
        return []
    return decoded.split('\n')


def get_git_change_years(filename):
    git_log_lines = call_git_log(filename)
    if len(git_log_lines) == 0:
        return [datetime.date.today().year]
    # timestamp is in ISO 8601 format. e.g. "2016-09-05 14:25:32 -0600"
    return [line.split(' ')[0].split('-')[0] for line in git_log_lines]


def get_most_recent_git_change_year(filename):
    return max(get_git_change_years(filename))


def get_git_change_year_range(filename):
    years = get_git_change_years(filename)
    return min(years), max(years)


###############################################################################
# update header years execution
###############################################################################


COPYRIGHT = 'Copyright \\(c\\)'
HOLDER = 'The Bitcoin Core developers'
UPDATEABLE_LINE_COMPILED = re.compile(' '.join([COPYRIGHT, YEAR_RANGE,
                                                HOLDER]))


def get_updatable_copyright_line(file_lines):
    index = 0
    for line in file_lines:
        if UPDATEABLE_LINE_COMPILED.search(line) is not None:
            return index, line
        index = index + 1
    return None, None


def year_range_to_str(start_year, end_year):
    if start_year == end_year:
        return start_year
    return "%s-%s" % (start_year, end_year)


def create_updated_copyright_line(line, last_git_change_year):
    match = YEAR_RANGE_COMPILED.search(line)
    start_year = match.group('start_year')
    end_year = match.group('end_year')
    if end_year is None:
        end_year = start_year
    if end_year == last_git_change_year:
        return line
    new_range_str = year_range_to_str(start_year, last_git_change_year)
    return YEAR_RANGE_COMPILED.sub(new_range_str, line)


def update_copyright(file_info):
    filename = file_info['filename']
    file_lines = file_info['contents'].split('\n')
    index, line = get_updatable_copyright_line(file_lines)
    if line is None:
        print_file_action_message(filename, "Bitcoin Core not a holder.")
        return
    last_git_change_year = get_most_recent_git_change_year(filename)
    new_line = create_updated_copyright_line(line, last_git_change_year)
    if line == new_line:
        print_file_action_message(filename, "Copyright up-to-date.")
        return
    file_lines[index] = new_line
    file_info['contents'] = '\n'.join(file_lines)
    write_file(file_info['filename'], file_info['contents'])
    print_file_action_message(filename,
                              "Copyright updated! -> %s" %
                              last_git_change_year)


def update_updatable_copyrights(file_infos):
    for file_info in file_infos:
        if not file_info['has_header']:
            print_file_action_message(file_info['filename'],
                                      "No header to update.")
            continue
        update_copyright(file_info)


def exec_update_header_year(base_directory):
    original_cwd = os.getcwd()
    os.chdir(base_directory)
    full_file_list = git_ls()
    file_infos = [gather_file_info(filename) for filename in
                  get_filenames_to_examine(full_file_list)]
    update_updatable_copyrights(file_infos)
    os.chdir(original_cwd)


###############################################################################
# update cmd
###############################################################################


UPDATE_USAGE = """
Updates all the copyright headers of "The Bitcoin Core developers" which were
changed in a year more recent than is listed. For example:

// Copyright (c) <firstYear>-<lastYear> The Bitcoin Core developers

will be updated to:

// Copyright (c) <firstYear>-<lastModifiedYear> The Bitcoin Core developers

where <lastModifiedYear> is obtained from the 'git log' history.

This subcommand also handles copyright headers that have only a single year.
In those cases:

// Copyright (c) <year> The Bitcoin Core developers

will be updated to:

// Copyright (c) <year>-<lastModifiedYear> The Bitcoin Core developers

where the update is appropriate.

Usage:
    $ ./copyright_header.py update <base_directory>

Arguments:
    <base_directory> - The base directory of a bitcoin core source code
    repository.
"""


def print_file_action_message(filename, action):
    print("%-52s %s" % (filename, action))


def update_cmd(argv):
    if len(argv) != 3:
        sys.exit(UPDATE_USAGE)

    base_directory = argv[2]
    if not os.path.exists(base_directory):
        sys.exit("*** bad base_directory: %s" % base_directory)
    exec_update_header_year(base_directory)


###############################################################################
# inserted copyright header format
###############################################################################


SCRIPT_HEADER = ("# Copyright (c) %s The Bitcoin Core developers\n"
                 "# Distributed under the MIT software license, see the "
                 "accompanying\n# file COPYING or http://www.opensource.org/"
                 "licenses/mit-license.php.\n")

CPP_HEADER = ("// Copyright (c) %s The Bitcoin Core developers\n// "
              "Distributed under the MIT software license, see the "
              "accompanying\n// file COPYING or http://www.opensource.org/"
              "licenses/mit-license.php.\n")


def get_cpp_header(start_year, end_year):
    return CPP_HEADER % year_range_to_str(start_year, end_year)


def get_script_header(start_year, end_year):
    return SCRIPT_HEADER % year_range_to_str(start_year, end_year)


###############################################################################
# insert header execution
###############################################################################


def insert_script_header(file_info, start_year, end_year):
    header = get_script_header(start_year, end_year)
    if starts_with_shebang(file_info['contents']):
        insertion_point = file_info['contents'].find('\n') + 1
    else:
        insertion_point = 0
    contents = file_info['contents']
    file_info['contents'] = (contents[:insertion_point] + header +
                             contents[insertion_point:])


def insert_cpp_header(file_info, start_year, end_year):
    header = get_cpp_header(start_year, end_year)
    file_info['contents'] = header + file_info['contents']


def exec_insert_header(filename, style):
    file_info = gather_file_info(filename)
    if file_info['pass']:
        sys.exit("*** %s already passes copyright header check." % filename)

    start_year, end_year = get_git_change_year_range(filename)
    if style == 'script':
        insert_script_header(file_info, start_year, end_year)
    else:
        insert_cpp_header(file_info, start_year, end_year)

    write_file(file_info['filename'], file_info['contents'])
    file_info = gather_file_info(filename)
    if not file_info['pass']:
        sys.exit("*** failed to fix issue in %s by inserting header?" %
                 filename)


###############################################################################
# insert cmd
###############################################################################


INSERT_USAGE = """
Inserts a copyright header for "The Bitcoin Core developers" at the top of the
file in either Python or C++ style as determined by the file extension. If the
file is a Python file and it has a '#!' starting the first line, the header is
inserted in the line below it.

The copyright dates will be set to be:

"<year_introduced>-<current_year>"

where <year_introduced> is according to the 'git log' history. If
<year_introduced> is equal to <current_year>, the date will be set to be:

"<current_year>"

If the file already has a valid copyright for "The Bitcoin Core developers",
the script will exit. If the file is in a subdir that should have a non-default
copyright header, the script will exit.

Usage:
    $ ./copyright_header.py insert <file>

Arguments:
    <file> - A source file in the bitcoin repository.
"""


def insert_cmd(argv):
    if len(argv) != 3:
        sys.exit(INSERT_USAGE)

    filename = argv[2]
    if not os.path.isfile(filename):
        sys.exit("*** bad filename: %s" % filename)
    _, extension = os.path.splitext(filename)
    if extension not in ['.h', '.cpp', '.cc', '.c', '.py', '.sh']:
        sys.exit("*** cannot insert for file extension %s" % extension)

    if extension in ['.py', '.sh', '*.am', '*.m4', '*.include']:
        style = 'script'
    else:
        style = 'cpp'
    exec_insert_header(filename, style)


###############################################################################
# UI
###############################################################################


USAGE = """
copyright_header.py - utilities for managing copyright headers of 'The Bitcoin
Core developers' in repository source files.

Usage:
    $ ./copyright_header.py <subcommand>

Subcommands:
    report
    check
    update
    insert

To see subcommand usage, run them without arguments.
"""

SUBCOMMANDS = ['report', 'check', 'update', 'insert']


if __name__ == "__main__":
    if len(sys.argv) == 1:
        sys.exit(USAGE)
    if sys.argv[1] not in SUBCOMMANDS:
        sys.exit(USAGE)
    if sys.argv[1] == 'report':
        report_cmd(sys.argv)
    elif sys.argv[1] == 'check':
        check_cmd(sys.argv)
    elif sys.argv[1] == 'update':
        update_cmd(sys.argv)
    elif sys.argv[1] == 'insert':
        insert_cmd(sys.argv)
