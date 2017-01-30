#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import re
import fnmatch
import sys
import subprocess
import datetime
import os

# Overview - This script enforces rules for MIT License copyright headers in
# the source code repository. If TravisCI is failing due to this script
# returning an error code, this means that either 1) the copyright header of
# one or more files is wrong and this needs to be corrected or 2) the rules of
# this script need to be adjusted to accommodate.
#
# There are five rulesets enforced depending on the path of the particular
# source file. The main one is for the base path, where the default is a
# copyright held by 'The Bitcoin Core developers' (though there are accepted
# variants). The acceptable form is defined CORE_HEADER regex below. The four
# other rulesets are for the secp256k1, leveldb, univalue and ctaes subtrees
# and have headers defined by regexes SECP256K1_HEADER, LEVELDB_HEADER,
# UNIVALUE_HEADER, CTAES_HEADER respectively below.
#
# The files that this check applies to are defined in the SOURCE_FILES
# variable. Exceptions for the base directory and the subtree are also defined
# in each respective section below.

###############################################################################
# common constants for regexes
###############################################################################


# this script is only applied to files in 'git ls-files' of these extensions:
SOURCE_FILES = ['*.h', '*.cpp', '*.cc', '*.c', '*.py', '*.sh', '*.am', '*.m4',
                '*.include']

# global setting to ignore files of this kind
ALWAYS_IGNORE = ['*__init__.py']

YEAR = "20[0-9][0-9]"
YEAR_RANGE = '(?P<start_year>%s)(-(?P<end_year>%s))?' % (YEAR, YEAR)
YEAR_RANGE_COMPILED = re.compile(YEAR_RANGE)


###############################################################################
# header regex and ignore list for the base bitcoin core repository
###############################################################################


CORE_HOLDERS = [
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
ANY_CORE_HOLDER = '|'.join([h for h in CORE_HOLDERS])
CORE_COPYRIGHT_LINE = (
    "(#|//|dnl) Copyright \\(c\\) %s (%s)" % (YEAR_RANGE, ANY_CORE_HOLDER))
CORE_LAST_TWO_LINES = ("(#|//|dnl) Distributed under the MIT software license, "
                       "see the accompanying\n(#|//|dnl) file COPYING or "
                       "http://www\\.opensource\\.org/licenses/mit-license"
                       "\\.php\\.\n")

CORE_HEADER = "(%s\n)+%s" % (CORE_COPYRIGHT_LINE, CORE_LAST_TWO_LINES)

CORE_IGNORE_ISSUES = [
]

CORE_NO_HEADER_EXPECTED = [
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

CORE_OTHER_COPYRIGHT_EXPECTED = [
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


###############################################################################
# header regex and ignore list for secp256k1 subtree
###############################################################################


SECP256K1_HOLDERS = [
    "Pieter Wuille +\\*",
    "Pieter Wuille, Gregory Maxwell +\\*",
    "Pieter Wuille, Andrew Poelstra +\\*",
    "Andrew Poelstra +\\*",
    "Diederik Huys, Pieter Wuille +\\*",
    "Thomas Daede, Cory Fields +\\*",
]
ANY_SECP256K1_HOLDER = '(%s)' % '|'.join([h for h in SECP256K1_HOLDERS])
YEAR_LIST = '(%s)((, %s)?)+' % (YEAR, YEAR)
YEAR_LIST_OR_RANGE = "(%s|%s)" % (YEAR_LIST, YEAR_RANGE)

SECP256K1_HEADER = (
    "(/\\*+\n \\* Copyright \\(c\\) %s %s\n \\* Distributed under the MIT "
    "software license, see the accompanying +\\*\n \\* file COPYING or "
    "http://www\\.opensource\\.org/licenses/mit-license\\.php\\. ?\\*\n "
    "\\*+\\/\n)" %
    (YEAR_LIST_OR_RANGE, ANY_SECP256K1_HOLDER))

SECP256K1_IGNORE_ISSUES = None

SECP256K1_NO_HEADER_EXPECTED = [
    # bitcoin-core/secp256k1 issue 412 - no header in these files:
    'src/secp256k1/include/secp256k1.h',
    'src/secp256k1/include/secp256k1_ecdh.h',
    'src/secp256k1/include/secp256k1_recovery.h',
    'src/secp256k1/include/secp256k1_schnorr.h',
    'src/secp256k1/src/java/org_bitcoin_NativeSecp256k1.c',
    'src/secp256k1/src/java/org_bitcoin_NativeSecp256k1.h',
    'src/secp256k1/src/java/org_bitcoin_Secp256k1Context.c',
    'src/secp256k1/src/java/org_bitcoin_Secp256k1Context.h',
    # build scripts:
    'src/secp256k1/Makefile.am',
    'src/secp256k1/autogen.sh',
    'src/secp256k1/build-aux/m4/ax_jni_include_dir.m4',
    'src/secp256k1/build-aux/m4/ax_prog_cc_for_build.m4',
    'src/secp256k1/build-aux/m4/bitcoin_secp.m4',
    'src/secp256k1/src/modules/ecdh/Makefile.am.include',
    'src/secp256k1/src/modules/recovery/Makefile.am.include',
    'src/secp256k1/src/modules/schnorr/Makefile.am.include',
]

SECP256K1_OTHER_COPYRIGHT_EXPECTED = [
    'src/secp256k1/build-aux/m4/ax_jni_include_dir.m4',
    'src/secp256k1/build-aux/m4/ax_prog_cc_for_build.m4',
]


###############################################################################
# header regex and ignore list for leveldb subtree
###############################################################################


# '//' style:
LEVELDB_1 = ("// Copyright \\(c\\) %s The LevelDB Authors\\. All rights "
             "reserved\\.\n// Use of this source code is governed by a "
             "BSD-style license that can be\n// found in the LICENSE file\\. "
             "See the AUTHORS file for names of contributors\\.\n" % YEAR)
# '//' style on the 3rd line due to ifdef:
LEVELDB_2 = (".+\n.+\n\n" + LEVELDB_1)
# '//' style with no '(c)' between 'Copyright' and the date:
LEVELDB_3 = ("// Copyright %s The LevelDB Authors\\. All rights reserved"
             "\\.\n// Use of this source code is governed by a BSD-style "
             "license that can be\n// found in the LICENSE file\\. See the "
             "AUTHORS file for names of contributors\\.\n" % YEAR)
# starts with "LevelDB":
LEVELDB_4 = ("// LevelDB Copyright \\(c\\) %s The LevelDB Authors\\. All "
             "rights reserved\\.\n// Use of this source code is governed by a "
             "BSD-style license that can be\n// found in the LICENSE file\\. "
             "See the AUTHORS file for names of contributors\\.\n" % YEAR)
# '/*' style with 3 spaces at the start of next line:
LEVELDB_5 = ("/\\* Copyright \\(c\\) %s The LevelDB Authors\\. All rights "
             "reserved\\.\n   Use of this source code is governed by a "
             "BSD-style license that can be\n   found in the LICENSE file\\. "
             "See the AUTHORS file for names of contributors\\. \\*\\/\n" %
             YEAR)
# '/*' style with 2 spaces at the start of next line:
LEVELDB_6 = ("/\\* Copyright \\(c\\) %s The LevelDB Authors\\. All rights "
             "reserved\\.\n  Use of this source code is governed by a "
             "BSD-style license that can be\n  found in the LICENSE file\\. "
             "See the AUTHORS file for names of contributors\\.\n" % YEAR)

LEVELDB_HEADER = (
    "(%s)" % '|'.join([LEVELDB_1, LEVELDB_2, LEVELDB_3, LEVELDB_4, LEVELDB_5,
                       LEVELDB_6]))

LEVELDB_IGNORE_ISSUES = None

LEVELDB_NO_HEADER_EXPECTED = {
    # other copyright notice:
    'src/leveldb/util/env_win.cc',
}

LEVELDB_OTHER_COPYRIGHT_EXPECTED = {
    # use of the word 'copyright' in header text:
    'src/leveldb/port/port_win.cc',
    'src/leveldb/port/port_win.h',
}


###############################################################################
# header regex and ignore list for univalue subtree
###############################################################################


UNIVALUE_HOLDERS = [
    "Jeff Garzik",
    "BitPay Inc\\.",
    "Bitcoin Core Developers",
    "Wladimir J. van der Laan",
]
ANY_UNIVALUE_HOLDER = '|'.join([h for h in UNIVALUE_HOLDERS])
UNIVALUE_COPYRIGHT_LINE = "// Copyright %s (%s)" % (YEAR_RANGE,
                                                    ANY_UNIVALUE_HOLDER)
UNIVALUE_LAST_2_LINES = (
    "// Distributed under the MIT(/X11)? software license, see the "
    "accompanying\n// file COPYING or "
    "http://www\\.opensource\\.org/licenses/mit-license\\.php\\.\n")

UNIVALUE_HEADER = "(%s\n)+%s" % (UNIVALUE_COPYRIGHT_LINE,
                                 UNIVALUE_LAST_2_LINES)

UNIVALUE_IGNORE_ISSUES = None

UNIVALUE_NO_HEADER_EXPECTED = [
    # build scripts:
    'src/univalue/autogen.sh',
    'src/univalue/Makefile.am',
    # auto generated files:
    'src/univalue/lib/univalue_escapes.h',
]

UNIVALUE_OTHER_COPYRIGHT_EXPECTED = None


###############################################################################
# header regex and ignore list for ctaes subtree
###############################################################################


CTAES_HEADER = (
    "( /\\*+\n \\* Copyright \\(c\\) %s Pieter Wuille +\\*\n \\* Distributed "
    "under the MIT software license, see the accompanying +\\*\n \\* file "
    "COPYING or http://www\\.opensource\\.org/licenses/mit-license\\.php\\. ?"
    "\\*\n \\*+/\n)" % YEAR)

CTAES_IGNORE_ISSUES = [
    # fixed by bitcoin-core/ctaes PR #6:
    'src/crypto/ctaes/bench.c',
]

CTAES_NO_HEADER_EXPECTED = None

CTAES_OTHER_COPYRIGHT_EXPECTED = None


###############################################################################
# declares the rules for matching a given file path to a header check rule
###############################################################################


HEADER_RULES = [
    {'title': 'The secp256k1 subtree.',
     'subdir': 'src/secp256k1/*',
     'header': SECP256K1_HEADER,
     'ignore': SECP256K1_IGNORE_ISSUES,
     'no_hdr_expected': SECP256K1_NO_HEADER_EXPECTED,
     'other_c_expected': SECP256K1_OTHER_COPYRIGHT_EXPECTED},
    {'title': 'The LevelDB subtree.',
     'subdir': 'src/leveldb/*',
     'header': LEVELDB_HEADER,
     'ignore': LEVELDB_IGNORE_ISSUES,
     'no_hdr_expected': LEVELDB_NO_HEADER_EXPECTED,
     'other_c_expected': LEVELDB_OTHER_COPYRIGHT_EXPECTED},
    {'title': 'The univalue subtree.',
     'subdir': 'src/univalue/*',
     'header': UNIVALUE_HEADER,
     'ignore': UNIVALUE_IGNORE_ISSUES,
     'no_hdr_expected': UNIVALUE_NO_HEADER_EXPECTED,
     'other_c_expected': UNIVALUE_OTHER_COPYRIGHT_EXPECTED},
    {'title': 'The ctaes subtree.',
     'subdir': 'src/crypto/ctaes/*',
     'header': CTAES_HEADER,
     'ignore': CTAES_IGNORE_ISSUES,
     'no_hdr_expected': CTAES_NO_HEADER_EXPECTED,
     'other_c_expected': CTAES_OTHER_COPYRIGHT_EXPECTED},
    {'title': 'The Bitcoin Core repository.',
     'subdir': '*',
     'header': CORE_HEADER,
     'ignore': CORE_IGNORE_ISSUES,
     'no_hdr_expected': CORE_NO_HEADER_EXPECTED,
     'other_c_expected': CORE_OTHER_COPYRIGHT_EXPECTED},
]


def compile_fnmatches(fnmatches):
    if fnmatches is None or len(fnmatches) == 0:
        # a regex that matches nothing (including not matching an empty string)
        return re.compile('(?!)')
    return re.compile('|'.join([fnmatch.translate(m) for m in fnmatches]))


for hr in HEADER_RULES:
    # compile path fnmatches and so we can do quick matches
    hr['subdir_compiled'] = compile_fnmatches([hr['subdir']])
    hr['header_compiled'] = re.compile(hr['header'])
    hr['ignore_compiled'] = compile_fnmatches(hr['ignore'])
    hr['no_hdr_expected_compiled'] = compile_fnmatches(hr['no_hdr_expected'])
    hr['other_c_expected_compiled'] = compile_fnmatches(hr['other_c_expected'])


def match_filename_to_rule(filename):
    for rule in HEADER_RULES:
        if rule['subdir_compiled'].match(filename):
            return rule
    raise Exception("%s did not match a rule in HEADER_RULES")


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


def filename_is_globally_included(filename):
    return (SOURCE_FILES_COMPILED.match(filename) and not
            ALWAYS_IGNORE_COMPILED.match(filename))


def get_filenames_to_examine():
    return sorted([filename for filename in git_ls() if
                   filename_is_globally_included(filename)])


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


def get_header_match(contents, rule):
    return rule['header_compiled'].search(contents)


def file_has_header(contents, rule):
    header_match = get_header_match(contents, rule)
    if not header_match:
        return False
    return header_match_in_correct_place(contents, header_match)


###############################################################################
# detect if file has a copyright message other than matches the rule
###############################################################################


OTHER_COPYRIGHT = "(Copyright|COPYRIGHT|copyright)"
OTHER_COPYRIGHT_COMPILED = re.compile(OTHER_COPYRIGHT)


def has_copyright_in_region(contents_region):
    return OTHER_COPYRIGHT_COMPILED.search(contents_region)


def file_has_other_copyright(contents, rule):
    # look for the OTHER_COPYRIGHT regex outside the normal header regex match
    header_match = get_header_match(contents, rule)
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
    'description': "A valid header was expected, but the file does not have "
                   "one.",
    'resolution': """
A correct MIT License header copyrighted by 'The Bitcoin Core developers' can
be inserted into a file by running:

    $ ./contrib/devtools/copyright_header.py insert <filename>

If there was a preexisting invalid header in the file, that will need to be
manually deleted.

If there is a new copyright holder for the MIT License, the holder will need to
be added to the CORE_HOLDERS list to include it in the regex check.
"""
}

FAILURE_REASON_2 = {
    'description': "A valid header was expected, but the file does not have "
                   "one (in subtree).",
    'resolution': """
The file is in a subtree and it's header doesn't match the existing defined
rules for the subtree. Either the header is incorrect and needs to change or
the rules for the subtree need to be adjusted to accommodate this new header.
"""
}

FAILURE_REASON_3 = {
    'description': "A valid header was found in the file, but it wasn't "
                   "expected.",
    'resolution': """
The header was not expected due to a deliberate setting in copyright_header.py
corresponding to the subtree of the file. If this pull request appropriately
adds a valid copyright header to the file, the file can be removed from the
NO_HEADER_EXPECTED listing for the subtree of the file.
"""
}

FAILURE_REASON_4 = {
    'description': "Another copyright instance was found, but it wasn't "
                   "expected.",
    'resolution': """
This file's body has a regular expression match for the (case-sensitive) words
"Copyright", "COPYRIGHT" or 'copyright". If this is an appropriate addition for
this pull request, copyright_header.py can be edited to add the file to the
OTHER_COPYRIGHT_EXPECTED listing for the subtree of the file.
"""
}

FAILURE_REASON_5 = {
    'description': "Another copyright was expected, but this file does not "
                   "have one.",
    'resolution': """
A use of the (case-sensitive) words "Copyright", "COPYRIGHT", or 'copyright'
outside of the regular copyright header was expected due to a setting in
copyright_header.py but such a word was not found by a regular expression
search. If this text was appropriately removed from the file as part of this
pull request, copyright_header.py can be edited to remove the file from the
OTHER_COPYRIGHT_EXPECTED listing for the subtree of the file.
"""
}

FAILURE_REASON_6 = {
    'description': "There were no issues found with the file, but issues were "
                   "expected.",
    'resolution': """
Issues were expected to be found with this file due to a setting in
copyright_header.py, however this file is currently not exhibiting any issues.
If the cause for the setting for the file is appropriately removed as part of
this pull request, copyright_header.py can be edited to remove the file from
the IGNORE_ISSUES listing for the subtree of the file.
"""
}

FAILURE_REASONS = [FAILURE_REASON_1, FAILURE_REASON_2, FAILURE_REASON_3,
                   FAILURE_REASON_4, FAILURE_REASON_5, FAILURE_REASON_6]

NO_FAILURE = {
    'description': "Everything is excellent.",
    'resolution': "(none)"
}


def ignore_issues(filename, rule):
    return rule['ignore_compiled'].match(filename)


def evaluate_rule_expectations(filename, rule, has_header, has_other):
    hdr_expected = not rule['no_hdr_expected_compiled'].match(filename)
    other_c_expected = rule['other_c_expected_compiled'].match(filename)
    core_header = rule['title'] == 'The Bitcoin Core repository.'
    if not has_header and hdr_expected and core_header:
        return FAILURE_REASON_1
    if not has_header and hdr_expected and not core_header:
        return FAILURE_REASON_2
    if has_header and not hdr_expected:
        return FAILURE_REASON_3
    if has_other and not other_c_expected:
        return FAILURE_REASON_4
    if not has_other and other_c_expected:
        return FAILURE_REASON_5
    return NO_FAILURE


def evaluate_rule(filename, rule, ignore, has_header, has_other):
    result = evaluate_rule_expectations(filename, rule, has_header, has_other)
    if ignore and result is NO_FAILURE:
        return FAILURE_REASON_6
    if result is not NO_FAILURE and not ignore:
        return result
    return NO_FAILURE


def gather_file_info(filename):
    info = {}
    info['filename'] = filename
    info['contents'] = read_file(filename)
    info['rule'] = match_filename_to_rule(info['filename'])
    info['ignore'] = ignore_issues(info['filename'], info['rule'])
    info['has_header'] = file_has_header(info['contents'], info['rule'])
    info['has_other'] = file_has_other_copyright(info['contents'],
                                                 info['rule'])
    info['evaluation'] = evaluate_rule(info['filename'], info['rule'],
                                       info['ignore'], info['has_header'],
                                       info['has_other'])
    info['pass'] = info['evaluation'] is NO_FAILURE
    return info


###############################################################################
# report execution
###############################################################################


SEPARATOR = '-'.join(['' for _ in range(80)]) + '\n'
REPORT = []


def report(string):
    REPORT.append(string)


def report_filenames(file_infos):
    if len(file_infos) == 0:
        return
    report('\t')
    report('\n\t'.join([file_info['filename'] for file_info in file_infos]))
    report('\n')


def report_summary(file_infos):
    report("%d files examined according to SOURCE_FILES and ALWAYS_IGNORE "
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


def report_rule(rule, file_infos):
    covered = [file_info for file_info in file_infos if
               file_info['rule'] is rule]
    ignored = [file_info for file_info in covered if file_info['ignore']]

    passed = [file_info for file_info in covered if file_info['pass']]
    failed = [file_info for file_info in covered if not file_info['pass']]

    report('Rule title: "%s"\n' % rule['title'])
    report("Rule subdir: %s\n" % rule['subdir'])
    report("files covered by rule subdir:      %4d\n" % len(covered))
    report("ignored due to setting:            %4d\n" % len(ignored))
    report("files passed:                      %4d\n" % len(passed))
    report("files failed:                      %4d\n" % len(failed))
    report_failure_reasons(failed)


def print_report(file_infos):
    report(SEPARATOR)
    report_summary(file_infos)
    for rule in HEADER_RULES:
        report(SEPARATOR)
        report_rule(rule, file_infos)
    report(SEPARATOR)
    print(''.join(REPORT), end="")


def exec_report(base_directory):
    original_cwd = os.getcwd()
    os.chdir(base_directory)
    file_infos = [gather_file_info(filename) for filename in
                  get_filenames_to_examine()]
    print_report(file_infos)
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
# ci_check execution
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


def print_ci_report(file_infos, failures):
    report(SEPARATOR)
    report_summary(file_infos)
    for failure in failures:
        report(SEPARATOR)
        report_failure(failure)
    report(SEPARATOR)
    if len(failures) == 0:
        green_report("No copyright header issues found!\n")
        report(SEPARATOR)
    print(''.join(REPORT), end="")


def exec_ci_check(base_directory):
    original_cwd = os.getcwd()
    os.chdir(base_directory)
    file_infos = [gather_file_info(filename) for filename in
                  get_filenames_to_examine()]
    failures = get_failures(file_infos)
    print_ci_report(file_infos, failures)
    os.chdir(original_cwd)
    if len(failures) > 0:
        sys.exit("*** Copyright header issues found!")


###############################################################################
# ci_check cmd
###############################################################################


CI_CHECK_USAGE = """
Checks over the repository for any issues with the copyright header that need
to be resolved prior to merge. Returns a non-zero status if there are any
issues found. Also, a report is printed specifically identifying which files
are problematic and a suggestion for what can be done to resolve the issue.

Usage:
    $ ./copyright_header.py ci_check <base_directory>

Arguments:
    <base_directory> - The base directory of a bitcoin core source code
    repository.
"""


def ci_check_cmd(argv):
    if len(argv) != 3:
        sys.exit(CI_CHECK_USAGE)

    base_directory = argv[2]
    if not os.path.exists(base_directory):
        sys.exit("*** bad <base_directory>: %s" % base_directory)

    exec_ci_check(base_directory)


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
        print_file_action_message(filename, "Bitcoin Core is not a holder.")
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
        if not file_info['rule']['title'] != "The Bitcoin Core repository":
            print_file_action_message(file_info['filename'], "Not updatable.")
            continue
        update_copyright(file_info)


def exec_update_header_year(base_directory):
    original_cwd = os.getcwd()
    os.chdir(base_directory)
    file_infos = [gather_file_info(filename) for filename in
                  get_filenames_to_examine()]
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
    if file_info['rule']['title'] != 'The Bitcoin Core repository.':
        sys.exit("*** cannot insert a header in the %s subtree." %
                 file_info['rule']['subdir'])

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
    $ ./copyright_header <subcommand>

Subcommands:
    report
    ci_check
    update
    insert

To see subcommand usage, run them without arguments.
"""

SUBCOMMANDS = ['report', 'ci_check', 'update', 'insert']


if __name__ == "__main__":
    if len(sys.argv) == 1:
        sys.exit(USAGE)
    if sys.argv[1] not in SUBCOMMANDS:
        sys.exit(USAGE)
    if sys.argv[1] == 'report':
        report_cmd(sys.argv)
    elif sys.argv[1] == 'ci_check':
        ci_check_cmd(sys.argv)
    elif sys.argv[1] == 'update':
        update_cmd(sys.argv)
    elif sys.argv[1] == 'insert':
        insert_cmd(sys.argv)
