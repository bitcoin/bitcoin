#!/usr/bin/env python3
# Copyright (c) 2016-2019 The Bitcoin Core developers
# Copyright (c) 2019 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import re
import fnmatch
import sys
import subprocess
import datetime
import os

################################################################################
# file filtering
################################################################################

EXCLUDE = [
    # auto generated:
    'src/qt/bitcoinstrings.cpp',
    'src/chainparamsseeds.h',
    # other external copyrights:
    'src/tinyformat.h',
    'src/bench/nanobench.h',
    'test/functional/test_framework/bignum.py',
    # python init:
    '*__init__.py',
]
EXCLUDE_COMPILED = re.compile('|'.join([fnmatch.translate(m) for m in EXCLUDE]))

EXCLUDE_DIRS = [
    # git subtrees
    "src/crypto/ctaes/",
    "src/leveldb/",
    "src/secp256k1/",
    "src/univalue/",
    "src/crc32c/",
]

INCLUDE = ['*.h', '*.cpp', '*.cc', '*.c', '*.mm', '*.py', '*.sh', '*.bash-completion']
INCLUDE_COMPILED = re.compile('|'.join([fnmatch.translate(m) for m in INCLUDE]))

def applies_to_file(filename):
    for excluded_dir in EXCLUDE_DIRS:
        if filename.startswith(excluded_dir):
            return False
    return ((EXCLUDE_COMPILED.match(filename) is None) and
            (INCLUDE_COMPILED.match(filename) is not None))

################################################################################
# obtain list of files in repo according to INCLUDE and EXCLUDE
################################################################################

GIT_LS_CMD = 'git ls-files --full-name'.split(' ')
GIT_TOPLEVEL_CMD = 'git rev-parse --show-toplevel'.split(' ')

def call_git_ls(base_directory):
    out = subprocess.check_output([*GIT_LS_CMD, base_directory])
    return [f for f in out.decode("utf-8").split('\n') if f != '']

def call_git_toplevel():
    "Returns the absolute path to the project root"
    return subprocess.check_output(GIT_TOPLEVEL_CMD).strip().decode("utf-8")

def get_filenames_to_examine(base_directory):
    "Returns an array of absolute paths to any project files in the base_directory that pass the include/exclude filters"
    root = call_git_toplevel()
    filenames = call_git_ls(base_directory)
    return sorted([os.path.join(root, filename) for filename in filenames if
                   applies_to_file(filename)])

################################################################################
# define and compile regexes for the patterns we are looking for
################################################################################


COPYRIGHT_WITH_C = 'Copyright \(c\)'
COPYRIGHT_WITHOUT_C = 'Copyright'
ANY_COPYRIGHT_STYLE = '(%s|%s)' % (COPYRIGHT_WITH_C, COPYRIGHT_WITHOUT_C)

YEAR = "20[0-9][0-9]"
YEAR_RANGE = '(%s)(-%s)?' % (YEAR, YEAR)
YEAR_LIST = '(%s)(, %s)+' % (YEAR, YEAR)
ANY_YEAR_STYLE = '(%s|%s)' % (YEAR_RANGE, YEAR_LIST)
ANY_COPYRIGHT_STYLE_OR_YEAR_STYLE = ("%s %s" % (ANY_COPYRIGHT_STYLE,
                                                ANY_YEAR_STYLE))

ANY_COPYRIGHT_COMPILED = re.compile(ANY_COPYRIGHT_STYLE_OR_YEAR_STYLE)

def compile_copyright_regex(copyright_style, year_style, name):
    return re.compile('%s %s,? %s' % (copyright_style, year_style, name))

EXPECTED_HOLDER_NAMES = [
    "Satoshi Nakamoto\n",
    "The Bitcoin Core developers\n",
    "BitPay Inc\.\n",
    "University of Illinois at Urbana-Champaign\.\n",
    "Pieter Wuille\n",
    "Wladimir J. van der Laan\n",
    "Jeff Garzik\n",
    "Jan-Klaas Kollhof\n",
    "ArtForz -- public domain half-a-node\n",
    "The Dash Core developers\n",
    "Intel Corporation",
    "The Zcash developers",
    "Jeremy Rubin",
]

DOMINANT_STYLE_COMPILED = {}
YEAR_LIST_STYLE_COMPILED = {}
WITHOUT_C_STYLE_COMPILED = {}

for holder_name in EXPECTED_HOLDER_NAMES:
    DOMINANT_STYLE_COMPILED[holder_name] = (
        compile_copyright_regex(COPYRIGHT_WITH_C, YEAR_RANGE, holder_name))
    YEAR_LIST_STYLE_COMPILED[holder_name] = (
        compile_copyright_regex(COPYRIGHT_WITH_C, YEAR_LIST, holder_name))
    WITHOUT_C_STYLE_COMPILED[holder_name] = (
        compile_copyright_regex(COPYRIGHT_WITHOUT_C, ANY_YEAR_STYLE,
                                holder_name))

################################################################################
# search file contents for copyright message of particular category
################################################################################

def get_count_of_copyrights_of_any_style_any_holder(contents):
    return len(ANY_COPYRIGHT_COMPILED.findall(contents))

def file_has_dominant_style_copyright_for_holder(contents, holder_name):
    match = DOMINANT_STYLE_COMPILED[holder_name].search(contents)
    return match is not None

def file_has_year_list_style_copyright_for_holder(contents, holder_name):
    match = YEAR_LIST_STYLE_COMPILED[holder_name].search(contents)
    return match is not None

def file_has_without_c_style_copyright_for_holder(contents, holder_name):
    match = WITHOUT_C_STYLE_COMPILED[holder_name].search(contents)
    return match is not None

################################################################################
# get file info
################################################################################

def read_file(filename):
    return open(filename, 'r', encoding="utf8").read()

def gather_file_info(filename):
    info = {}
    info['filename'] = filename
    c = read_file(filename)
    info['contents'] = c

    info['all_copyrights'] = get_count_of_copyrights_of_any_style_any_holder(c)

    info['classified_copyrights'] = 0
    info['dominant_style'] = {}
    info['year_list_style'] = {}
    info['without_c_style'] = {}
    for holder_name in EXPECTED_HOLDER_NAMES:
        has_dominant_style = (
            file_has_dominant_style_copyright_for_holder(c, holder_name))
        has_year_list_style = (
            file_has_year_list_style_copyright_for_holder(c, holder_name))
        has_without_c_style = (
            file_has_without_c_style_copyright_for_holder(c, holder_name))
        info['dominant_style'][holder_name] = has_dominant_style
        info['year_list_style'][holder_name] = has_year_list_style
        info['without_c_style'][holder_name] = has_without_c_style
        if has_dominant_style or has_year_list_style or has_without_c_style:
            info['classified_copyrights'] = info['classified_copyrights'] + 1
    return info

################################################################################
# report execution
################################################################################

SEPARATOR = '-'.join(['' for _ in range(80)])

def print_filenames(filenames, verbose):
    if not verbose:
        return
    for filename in filenames:
        print("\t%s" % filename)

def print_report(file_infos, verbose):
    print(SEPARATOR)
    examined = [i['filename'] for i in file_infos]
    print("%d files examined according to INCLUDE and EXCLUDE fnmatch rules" %
          len(examined))
    print_filenames(examined, verbose)

    print(SEPARATOR)
    print('')
    zero_copyrights = [i['filename'] for i in file_infos if
                       i['all_copyrights'] == 0]
    print("%4d with zero copyrights" % len(zero_copyrights))
    print_filenames(zero_copyrights, verbose)
    one_copyright = [i['filename'] for i in file_infos if
                     i['all_copyrights'] == 1]
    print("%4d with one copyright" % len(one_copyright))
    print_filenames(one_copyright, verbose)
    two_copyrights = [i['filename'] for i in file_infos if
                      i['all_copyrights'] == 2]
    print("%4d with two copyrights" % len(two_copyrights))
    print_filenames(two_copyrights, verbose)
    three_copyrights = [i['filename'] for i in file_infos if
                        i['all_copyrights'] == 3]
    print("%4d with three copyrights" % len(three_copyrights))
    print_filenames(three_copyrights, verbose)
    four_or_more_copyrights = [i['filename'] for i in file_infos if
                               i['all_copyrights'] >= 4]
    print("%4d with four or more copyrights" % len(four_or_more_copyrights))
    print_filenames(four_or_more_copyrights, verbose)
    print('')
    print(SEPARATOR)
    print('Copyrights with dominant style:\ne.g. "Copyright (c)" and '
          '"<year>" or "<startYear>-<endYear>":\n')
    for holder_name in EXPECTED_HOLDER_NAMES:
        dominant_style = [i['filename'] for i in file_infos if
                          i['dominant_style'][holder_name]]
        if len(dominant_style) > 0:
            print("%4d with '%s'" % (len(dominant_style),
                                     holder_name.replace('\n', '\\n')))
            print_filenames(dominant_style, verbose)
    print('')
    print(SEPARATOR)
    print('Copyrights with year list style:\ne.g. "Copyright (c)" and '
          '"<year1>, <year2>, ...":\n')
    for holder_name in EXPECTED_HOLDER_NAMES:
        year_list_style = [i['filename'] for i in file_infos if
                           i['year_list_style'][holder_name]]
        if len(year_list_style) > 0:
            print("%4d with '%s'" % (len(year_list_style),
                                     holder_name.replace('\n', '\\n')))
            print_filenames(year_list_style, verbose)
    print('')
    print(SEPARATOR)
    print('Copyrights with no "(c)" style:\ne.g. "Copyright" and "<year>" or '
          '"<startYear>-<endYear>":\n')
    for holder_name in EXPECTED_HOLDER_NAMES:
        without_c_style = [i['filename'] for i in file_infos if
                           i['without_c_style'][holder_name]]
        if len(without_c_style) > 0:
            print("%4d with '%s'" % (len(without_c_style),
                                     holder_name.replace('\n', '\\n')))
            print_filenames(without_c_style, verbose)

    print('')
    print(SEPARATOR)

    unclassified_copyrights = [i['filename'] for i in file_infos if
                               i['classified_copyrights'] < i['all_copyrights']]
    print("%d with unexpected copyright holder names" %
          len(unclassified_copyrights))
    print_filenames(unclassified_copyrights, verbose)
    print(SEPARATOR)

def exec_report(base_directory, verbose):
    filenames = get_filenames_to_examine(base_directory)
    file_infos = [gather_file_info(f) for f in filenames]
    print_report(file_infos, verbose)

################################################################################
# report cmd
################################################################################

REPORT_USAGE = """
Produces a report of all copyright header notices found inside the source files
of a repository.

Usage:
    $ ./copyright_header.py report <base_directory> [verbose]

Arguments:
    <base_directory> - The base directory of a Dash Core source code repository.
    [verbose] - Includes a list of every file of each subcategory in the report.
"""

def report_cmd(argv):
    if len(argv) == 2:
        sys.exit(REPORT_USAGE)

    base_directory = argv[2]
    if not os.path.exists(base_directory):
        sys.exit("*** bad <base_directory>: %s" % base_directory)

    if len(argv) == 3:
        verbose = False
    elif argv[3] == 'verbose':
        verbose = True
    else:
        sys.exit("*** unknown argument: %s" % argv[2])

    exec_report(base_directory, verbose)

################################################################################
# query git for year of last change
################################################################################

GIT_LOG_CMD = "git log --pretty=format:%%ai %s"

def call_git_log(filename):
    out = subprocess.check_output((GIT_LOG_CMD % filename).split(' '))
    return out.decode("utf-8").split('\n')

def get_git_change_years(filename):
    git_log_lines = call_git_log(filename)
    if len(git_log_lines) == 0:
        return [datetime.date.today().year]
    # timestamp is in ISO 8601 format. e.g. "2016-09-05 14:25:32 -0600"
    return [line.split(' ')[0].split('-')[0] for line in git_log_lines]

def get_most_recent_git_change_year(filename):
    return max(get_git_change_years(filename))

################################################################################
# read and write to file
################################################################################

def read_file_lines(filename):
    f = open(filename, 'r', encoding="utf8")
    file_lines = f.readlines()
    f.close()
    return file_lines

def write_file_lines(filename, file_lines):
    f = open(filename, 'w', encoding="utf8")
    f.write(''.join(file_lines))
    f.close()

################################################################################
# update header years execution
################################################################################

COPYRIGHT = 'Copyright \(c\)'
YEAR = "20[0-9][0-9]"
YEAR_RANGE = '(%s)(-%s)?' % (YEAR, YEAR)
HOLDER = 'The Dash Core developers'
UPDATEABLE_LINE_COMPILED = re.compile(' '.join([COPYRIGHT, YEAR_RANGE, HOLDER]))

def get_updatable_copyright_line(file_lines):
    index = 0
    for line in file_lines:
        if UPDATEABLE_LINE_COMPILED.search(line) is not None:
            return index, line
        index = index + 1
    return None, None

def parse_year_range(year_range):
    year_split = year_range.split('-')
    start_year = year_split[0]
    if len(year_split) == 1:
        return start_year, start_year
    return start_year, year_split[1]

def year_range_to_str(start_year, end_year):
    if start_year == end_year:
        return start_year
    return "%s-%s" % (start_year, end_year)

def create_updated_copyright_line(line, last_git_change_year):
    copyright_splitter = 'Copyright (c) '
    copyright_split = line.split(copyright_splitter)
    # Preserve characters on line that are ahead of the start of the copyright
    # notice - they are part of the comment block and vary from file-to-file.
    before_copyright = copyright_split[0]
    after_copyright = copyright_split[1]

    space_split = after_copyright.split(' ')
    year_range = space_split[0]
    start_year, end_year = parse_year_range(year_range)
    if end_year == last_git_change_year:
        return line
    return (before_copyright + copyright_splitter +
            year_range_to_str(start_year, last_git_change_year) + ' ' +
            ' '.join(space_split[1:]))

def update_updatable_copyright(filename):
    file_lines = read_file_lines(filename)
    index, line = get_updatable_copyright_line(file_lines)
    if not line:
        print_file_action_message(filename, "No updatable copyright.")
        return
    last_git_change_year = get_most_recent_git_change_year(filename)
    new_line = create_updated_copyright_line(line, last_git_change_year)
    if line == new_line:
        print_file_action_message(filename, "Copyright up-to-date.")
        return
    file_lines[index] = new_line
    write_file_lines(filename, file_lines)
    print_file_action_message(filename,
                              "Copyright updated! -> %s" % last_git_change_year)

def exec_update_header_year(base_directory):
    for filename in get_filenames_to_examine(base_directory):
        update_updatable_copyright(filename)

################################################################################
# update cmd
################################################################################

UPDATE_USAGE = """
Updates all the copyright headers of "The Dash Core developers" which were
changed in a year more recent than is listed. For example:

// Copyright (c) <firstYear>-<lastYear> The Dash Core developers

will be updated to:

// Copyright (c) <firstYear>-<lastModifiedYear> The Dash Core developers

where <lastModifiedYear> is obtained from the 'git log' history.

This subcommand also handles copyright headers that have only a single year. In those cases:

// Copyright (c) <year> The Dash Core developers

will be updated to:

// Copyright (c) <year>-<lastModifiedYear> The Dash Core developers

where the update is appropriate.

Usage:
    $ ./copyright_header.py update <base_directory>

Arguments:
    <base_directory> - The base directory of Dash Core source code repository.
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

################################################################################
# inserted copyright header format
################################################################################

def get_header_lines(header, start_year, end_year):
    lines = header.split('\n')[1:-1]
    lines[0] = lines[0] % year_range_to_str(start_year, end_year)
    return [line + '\n' for line in lines]

CPP_HEADER = '''
// Copyright (c) %s The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''

def get_cpp_header_lines_to_insert(start_year, end_year):
    return reversed(get_header_lines(CPP_HEADER, start_year, end_year))

PYTHON_HEADER = '''
# Copyright (c) %s The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''

def get_python_header_lines_to_insert(start_year, end_year):
    return reversed(get_header_lines(PYTHON_HEADER, start_year, end_year))

################################################################################
# query git for year of last change
################################################################################

def get_git_change_year_range(filename):
    years = get_git_change_years(filename)
    return min(years), max(years)

################################################################################
# check for existing core copyright
################################################################################

def file_already_has_core_copyright(file_lines):
    index, _ = get_updatable_copyright_line(file_lines)
    return index != None

################################################################################
# insert header execution
################################################################################

def file_has_hashbang(file_lines):
    if len(file_lines) < 1:
        return False
    if len(file_lines[0]) <= 2:
        return False
    return file_lines[0][:2] == '#!'

def insert_python_header(filename, file_lines, start_year, end_year):
    if file_has_hashbang(file_lines):
        insert_idx = 1
    else:
        insert_idx = 0
    header_lines = get_python_header_lines_to_insert(start_year, end_year)
    for line in header_lines:
        file_lines.insert(insert_idx, line)
    write_file_lines(filename, file_lines)

def insert_cpp_header(filename, file_lines, start_year, end_year):
    header_lines = get_cpp_header_lines_to_insert(start_year, end_year)
    for line in header_lines:
        file_lines.insert(0, line)
    write_file_lines(filename, file_lines)

def exec_insert_header(filename, style):
    file_lines = read_file_lines(filename)
    if file_already_has_core_copyright(file_lines):
        sys.exit('*** %s already has a copyright by The Dash Core developers'
                 % (filename))
    start_year, end_year = get_git_change_year_range(filename)
    if style == 'python':
        insert_python_header(filename, file_lines, start_year, end_year)
    else:
        insert_cpp_header(filename, file_lines, start_year, end_year)

################################################################################
# insert cmd
################################################################################

INSERT_USAGE = """
Inserts a copyright header for "The Dash Core developers" at the top of the
file in either Python or C++ style as determined by the file extension. If the
file is a Python file and it has a '#!' starting the first line, the header is
inserted in the line below it.

The copyright dates will be set to be:

"<year_introduced>-<current_year>"

where <year_introduced> is according to the 'git log' history. If
<year_introduced> is equal to <current_year>, the date will be set to be:

"<current_year>"

If the file already has a copyright for "The Dash Core developers", the
script will exit.

Usage:
    $ ./copyright_header.py insert <file>

Arguments:
    <file> - A source file in the Dash Core repository.
"""

def insert_cmd(argv):
    if len(argv) != 3:
        sys.exit(INSERT_USAGE)

    filename = argv[2]
    if not os.path.isfile(filename):
        sys.exit("*** bad filename: %s" % filename)
    _, extension = os.path.splitext(filename)
    if extension not in ['.h', '.cpp', '.cc', '.c', '.py']:
        sys.exit("*** cannot insert for file extension %s" % extension)

    if extension == '.py':
        style = 'python'
    else:
        style = 'cpp'
    exec_insert_header(filename, style)

################################################################################
# UI
################################################################################

USAGE = """
copyright_header.py - utilities for managing copyright headers of 'The Dash
Core developers' in repository source files.

Usage:
    $ ./copyright_header <subcommand>

Subcommands:
    report
    update
    insert

To see subcommand usage, run them without arguments.
"""

SUBCOMMANDS = ['report', 'update', 'insert']

if __name__ == "__main__":
    if len(sys.argv) == 1:
        sys.exit(USAGE)
    subcommand = sys.argv[1]
    if subcommand not in SUBCOMMANDS:
        sys.exit(USAGE)
    if subcommand == 'report':
        report_cmd(sys.argv)
    elif subcommand == 'update':
        update_cmd(sys.argv)
    elif subcommand == 'insert':
        insert_cmd(sys.argv)
