#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import sys
import os
import subprocess
import time
import argparse
import re
import fnmatch
import difflib
import hashlib
from multiprocessing import Pool

###############################################################################
# settings for the set of files that this applies to
###############################################################################

SOURCE_FILES = ['*.cpp', '*.h']

SOURCE_FILES_COMPILED = re.compile('|'.join([fnmatch.translate(match)
                                             for match in SOURCE_FILES]))

ALWAYS_IGNORE = [
    # files in subtrees:
    'src/secp256k1/*',
    'src/leveldb/*',
    'src/univalue/*',
    'src/crypto/ctaes/*',
]

ALWAYS_IGNORE_COMPILED = re.compile('|'.join([fnmatch.translate(match)
                                              for match in ALWAYS_IGNORE]))

###############################################################################
# obtain list of files in repo to examine
###############################################################################

GIT_LS_CMD = 'git ls-files'


def git_ls():
    out = subprocess.check_output(GIT_LS_CMD.split(' '))
    return [f for f in out.decode("utf-8").split('\n') if f != '']


def filename_is_to_be_examined(filename):
    return (SOURCE_FILES_COMPILED.match(filename) and not
            ALWAYS_IGNORE_COMPILED.match(filename))


def get_filenames_in_scope(full_file_list):
    return sorted([filename for filename in full_file_list if
                   filename_is_to_be_examined(filename)])


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
# obtain formatted file
###############################################################################


def generate_style_arg(opts):
    return '-style={%s}' % ', '.join(["%s: %s" % (k, v) for k, v in
                                      opts.style_params.items()])


UNKNOWN_KEY_REGEX = re.compile("unknown key '(?P<key_name>\w+)'")


def parse_unknown_key(err):
    if len(err) == 0:
        return 0, None
    match = UNKNOWN_KEY_REGEX.search(err)
    if not match:
        return len(err), None
    return len(err), match.group('key_name')


def try_format_file(opts, filename):
    cmd = [opts.bin_path, generate_style_arg(opts), filename]
    return subprocess.Popen(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)


def format_file(opts, filename):
    while True:
        p = try_format_file(opts, filename)
        out = p.stdout.read().decode('utf-8')
        err = p.stderr.read().decode('utf-8')
        p.communicate()
        if p.returncode != 0:
            sys.exit("*** clang-format could not execute")
        # Older versions of clang don't support some style parameter keys, so
        # we work around by redacting any key that gets rejected until we find
        # a subset of parameters that can apply the format without producing
        # any stderr output.
        err_len, unknown_key = parse_unknown_key(err)
        if not unknown_key and err_len > 0:
            sys.exit("*** clang-format produced unknown output to stderr")
        if unknown_key:
            opts.style_params.pop(unknown_key)
            opts.unknown_style_params.append(unknown_key)
            continue
        return out


###############################################################################
# scoring
###############################################################################


def style_score(pre_format, unchanged, added, removed):
    # A crude calculation to give a percentage rating for adherence to the
    # defined style.
    if (added + removed) == 0:
        return 100
    return min(int(abs(1 - (float(pre_format - unchanged) /
                            float(pre_format))) * 100), 99)


def scoreboard(score, pre_format, added, removed, unchanged, post_format):
    return (" +-------+          +------------+--------+---------+-----------+"
            "-------------+\n"
            " | score |          | pre-format |  added | removed | unchanged |"
            " post-format |\n"
            " +-------+  +-------+------------+--------+---------+-----------+"
            "-------------+\n"
            " | %3d%%  |  | lines | %10d | %6d | %7d | %9d | %11d |\n"
            " +-------+  +-------+------------+--------+---------+-----------+"
            "-------------+\n" % (score, pre_format, added, removed,
                                  unchanged, post_format))


###############################################################################
# gather file and diff info
###############################################################################


def classify_diff_lines(diff):
    for l in diff:
        if l.startswith('  '):
            yield 1, 0, 0
        elif l.startswith('+ '):
            yield 0, 1, 0
        elif l.startswith('- '):
            yield 0, 0, 1


def sum_lines_of_type(diff):
    return (sum(c) for c in zip(*classify_diff_lines(diff)))


def gather_file_info(opts, filename):
    start = time.time()
    file_info = {}
    file_info['filename'] = filename
    file_info['contents'] = read_file(filename)
    file_info['formatted'] = format_file(opts, filename)
    file_info['matching'] = (file_info['contents'] == file_info['formatted'])
    file_info['formatted_md5'] = (
        hashlib.md5(file_info['formatted'].encode('utf-8')).hexdigest())
    return file_info


DIFFER = difflib.Differ()


def compute_diff_info(file_info):
    pre_format_lines = file_info['contents'].splitlines()
    post_format_lines = file_info['formatted'].splitlines()
    file_info['pre_format_lines'] = len(pre_format_lines)
    file_info['post_format_lines'] = len(post_format_lines)
    start_time = time.time()
    diff = DIFFER.compare(pre_format_lines, post_format_lines)
    (file_info['unchanged_lines'],
     file_info['added_lines'],
     file_info['removed_lines']) = sum_lines_of_type(diff)
    file_info['diff_time'] = time.time() - start_time
    file_info['score'] = style_score(file_info['pre_format_lines'],
                                     file_info['unchanged_lines'],
                                     file_info['added_lines'],
                                     file_info['removed_lines'])
    return file_info


###############################################################################
# report helpers
###############################################################################


SEPARATOR = '-' * 80 + '\n'
REPORT = []


def report(string):
    REPORT.append(string)


GREEN = '\033[92m'
RED = '\033[91m'
ENDC = '\033[0m'


def red_report(string):
    report(RED + string + ENDC)


def green_report(string):
    report(GREEN + string + ENDC)


def flush_report():
    print(''.join(REPORT), end="")


###############################################################################
# warning for old versions of clang-format
###############################################################################


def report_if_parameters_unsupported(opts):
    if len(opts.unknown_style_params) == 0:
        return
    report(SEPARATOR)
    red_report("WARNING")
    report(" - This version of clang-format does not support the "
           "following style\nparameters, so they were not used:\n\n")
    for param in opts.unknown_style_params:
        report("%s\n" % param)


def exit_if_parameters_unsupported(opts):
    if opts.force:
        return
    if len(opts.unknown_style_params) > 0:
        red_report("\nWARNING: ")
        report("clang-format version %s does not support all "
               "parameters given in\n%s\n\n" % (opts.bin_version,
                                                opts.style_file))
        report("Unsupported parameters:\n")
        for param in opts.unknown_style_params:
            report("\t%s\n" % param)
        # The recommendation is from experimentation where it is found that the
        # applied formating has subtle differences that vary between major
        # releases of clang-format. A chosen standard of formatting should
        # probably be based on the latest stable release and that should be the
        # recommendation.
        report("\nUsing clang-format version 3.9.0 or higher is recommended\n")
        report("Use the --force option to override and proceed anyway.\n\n")
        flush_report()
        sys.exit("*** missing clang-format support.")


###############################################################################
# 'report' subcommand execution
###############################################################################


def report_examined_files(file_infos, in_scope_file_list, full_file_list):
    report("%4d files tracked according to '%s'\n" %
           (len(full_file_list), GIT_LS_CMD))
    report("%4d files in scope according to SOURCE_FILES and ALWAYS_IGNORE "
           "settings\n" % len(in_scope_file_list))
    report("%4d files examined according to listed targets\n" %
           len(file_infos))


def score_in_range_inclusive(score, lower, upper):
    return (score >= lower) and (score <= upper)


def report_files_in_range(file_infos, lower, upper):
    in_range = [file_info for file_info in file_infos if
                score_in_range_inclusive(file_info['score'], lower, upper)]
    report("Files %2d%%-%2d%% matching:        %4d\n" % (lower, upper,
                                                         len(in_range)))


def report_files_in_ranges(file_infos):
    ranges = [(90, 99), (80, 89), (70, 79), (60, 69), (50, 59), (40, 49),
              (30, 39), (20, 29), (10, 19), (0, 9)]
    for lower, upper in ranges:
        report_files_in_range(file_infos, lower, upper)


def report_slowest_diffs(file_infos):
    slowest = [file_info for file_info in file_infos if
               file_info['diff_time'] > 1.0]
    if len(slowest) == 0:
        return
    report("Slowest diffs:\n")
    for file_info in slowest:
        report("%6.02fs for %s\n" % (file_info['diff_time'],
                                     file_info['filename']))


def print_report(opts, elapsed_time, file_infos, in_scope_file_list,
                 full_file_list):
    pre_format_lines = sum(file_info['pre_format_lines'] for file_info in
                           file_infos)
    added_lines = sum(file_info['added_lines'] for file_info in file_infos)
    removed_lines = sum(file_info['removed_lines'] for file_info in file_infos)
    unchanged_lines = sum(file_info['unchanged_lines'] for file_info in
                          file_infos)
    post_format_lines = sum(file_info['post_format_lines'] for file_info in
                            file_infos)
    score = style_score(pre_format_lines, unchanged_lines, added_lines,
                        removed_lines)
    matching = [file_info for file_info in file_infos if
                file_info['matching']]
    not_matching = [file_info for file_info in file_infos if not
                    file_info['matching']]
    h = hashlib.md5()
    for file_info in file_infos:
        h.update(file_info['formatted_md5'].encode('utf-8'))
    formatted_md5 = h.hexdigest()

    report(SEPARATOR)
    report_examined_files(file_infos, in_scope_file_list, full_file_list)
    report(SEPARATOR)
    report("clang-format bin:         %s\n" % opts.bin_path)
    report("clang-format version:     %s\n" % opts.bin_version)
    report("Using style in:           %s\n" % opts.style_file)
    report_if_parameters_unsupported(opts)
    report(SEPARATOR)
    report("Parallel jobs for diffs:   %d\n" % opts.jobs)
    report("Elapsed time:              %.02fs\n" % elapsed_time)
    report_slowest_diffs(file_infos)
    report(SEPARATOR)
    report("Files 100%% matching:       %8d\n" % len(matching))
    report("Files <100%% matching:      %8d\n" % len(not_matching))
    report("Formatted content MD5:      %s\n" % formatted_md5)
    report(SEPARATOR)
    report_files_in_ranges(file_infos)
    report(SEPARATOR)
    report("\n")
    report(scoreboard(score, pre_format_lines, added_lines, removed_lines,
                      unchanged_lines, post_format_lines))
    report("\n")
    report(SEPARATOR)
    flush_report()


def exec_report(opts):
    start_time = time.time()
    full_file_list = git_ls()
    in_scope_file_list = get_filenames_in_scope(full_file_list)
    file_infos = [gather_file_info(opts, filename) for filename in
                  in_scope_file_list if opts.target_regex.match(filename)]
    file_infos = Pool(opts.jobs).map(compute_diff_info, file_infos)
    print_report(opts, time.time() - start_time, file_infos,
                 in_scope_file_list, full_file_list)


###############################################################################
# 'check' subcommand execution
###############################################################################


def get_failures(file_infos):
    return [file_info for file_info in file_infos if not
            file_info['matching']]


def report_failure(failure):
    report("A code format issue was detected in ")
    red_report("%s\n" % failure['filename'])
    report(scoreboard(failure['score'], failure['pre_format_lines'],
                      failure['added_lines'], failure['removed_lines'],
                      failure['unchanged_lines'],
                      failure['post_format_lines']))


def print_check(opts, failures, file_infos, in_scope_file_list,
                full_file_list):
    report(SEPARATOR)
    report_examined_files(file_infos, in_scope_file_list, full_file_list)
    for failure in failures:
        report(SEPARATOR)
        report_failure(failure)
    report(SEPARATOR)
    if len(failures) == 0:
        green_report("No format issues found!\n")
    else:
        red_report("These files can be auto-formatted by running:\n")
        report("$ contrib/devtools/clang_format.py format [target "
               "[target ...]]\n")
    report(SEPARATOR)
    flush_report()


def exec_check(opts):
    full_file_list = git_ls()
    in_scope_file_list = get_filenames_in_scope(full_file_list)
    file_infos = [gather_file_info(opts, filename) for filename in
                  in_scope_file_list if opts.target_regex.match(filename)]
    exit_if_parameters_unsupported(opts)
    file_infos = Pool(opts.jobs).map(compute_diff_info, file_infos)
    failures = get_failures(file_infos)
    print_check(opts, failures, file_infos, in_scope_file_list, full_file_list)
    if len(failures) > 0:
        sys.exit("*** Format issues found!")


###############################################################################
# 'format' subcommand execution
###############################################################################


def exec_format(opts):
    full_file_list = git_ls()
    in_scope_file_list = get_filenames_in_scope(full_file_list)
    file_infos = [gather_file_info(opts, filename) for filename in
                  in_scope_file_list if opts.target_regex.match(filename)]
    exit_if_parameters_unsupported(opts)
    failures = get_failures(file_infos)
    for failure in failures:
        write_file(failure['filename'], failure['formatted'])


###############################################################################
# parse version
###############################################################################


VERSION_REGEX = re.compile("version (?P<version>[0-9]\.[0-9](\.[0-9])?)")


def get_clang_format_version(bin_path):
    p = subprocess.Popen([bin_path, '--version'], stdout=subprocess.PIPE)
    match = VERSION_REGEX.search(p.stdout.read().decode('utf-8'))
    if not match:
        return "(unknown version)"
    return match.group('version')


###############################################################################
# validate inputs
###############################################################################


def is_clang_format_executable(executable):
    return (executable.startswith('clang-format') and not
            executable.startswith('clang-format-diff'))


class PathAction(argparse.Action):
    def _path_exists(self, path):
        return os.path.exists(path)

    def _assert_exists(self, path):
        if not self._path_exists(path):
            sys.exit("*** does not exist: %s" % path)

    def _assert_mode(self, path, flags):
        if not os.access(path, flags):
            sys.exit("*** %s does not have correct mode: %x" % (path, flags))


class TargetsPathAction(PathAction):
    def _assert_in_git_repository(self, path):
        d = os.path.dirname(path) if os.path.isfile(path) else path
        cmd = 'git -C %s status' % d
        dn = open(os.devnull, 'w')
        if (subprocess.call(cmd.split(' '), stderr=dn, stdout=dn) != 0):
            sys.exit("*** %s is not a git repository" % path)

    def _has_dot_git_dir(self, path):
        return os.path.isdir(os.path.join(path, '.git/'))

    def _get_repo_base_dir(self, path):
        self._assert_exists(path)
        self._assert_in_git_repository(path)
        if self._has_dot_git_dir(path):
            return path

        def recurse_repo_base_dir(path):
            self._assert_in_git_repository(path)
            d = os.path.dirname(path)
            if self._has_dot_git_dir(d):
                return d
            return recurse_repo_base_dir(d)

        return recurse_repo_base_dir(path)

    def _assert_under_dir(self, target, repo_base_dir):
        if not target.startswith(repo_base_dir):
            sys.exit("*** %s is under repo other than %s" % (target,
                                                             repo_base_dir))

    def _compile_target_regex(self, repo_base_dir, targets):
        files = [target for target in targets if os.path.isfile(target)]
        wildcards = [os.path.join(target, '*') for target in targets if
                     os.path.isdir(target)]
        fnmatches = (files + wildcards)
        trimmed_fnmatches = [match.split(repo_base_dir + '/')[1] for match in
                             fnmatches]
        return re.compile('|'.join([fnmatch.translate(match)
                                    for match in trimmed_fnmatches]))

    def __call__(self, parser, namespace, values, option_string=None):
        targets = [os.path.abspath(target) for target in values]
        repo_base_dir = self._get_repo_base_dir(targets[0])
        for target in targets:
            self._assert_exists(target)
            self._assert_under_dir(target, repo_base_dir)
            self._assert_mode(target, os.R_OK)
        namespace.repository = repo_base_dir
        namespace.target_regex = self._compile_target_regex(repo_base_dir,
                                                            targets)


class BinPathAction(PathAction):
    def _assert_is_clang_format(self, path):
        executable = os.path.basename(path)
        if not is_clang_format_executable(executable):
            sys.exit("*** %s is not a clang format binary." % path)

    def __call__(self, parser, namespace, values, option_string=None):
        path = os.path.realpath(os.path.abspath(values))
        self._assert_exists(path)
        self._assert_mode(path, os.R_OK | os.X_OK)
        self._assert_is_clang_format(path)
        version = get_clang_format_version(path)
        namespace.bin_path = path
        namespace.bin_version = version


class StylePathAction(PathAction):
    def __call__(self, parser, namespace, values, option_string=None):
        path = os.path.abspath(values)
        self._assert_exists(path)
        self._assert_mode(path, os.R_OK)


###############################################################################
# find installed clang-format binary
###############################################################################


def get_clang_format_binaries():
    for path in os.environ["PATH"].split(os.pathsep):
        for e in os.listdir(path):
            if is_clang_format_executable(e):
                bin_path = os.path.realpath(os.path.join(path, e))
                bin_version = get_clang_format_version(bin_path)
                yield {'bin_path': bin_path, 'bin_version': bin_version}


def locate_installed_binary():
    installed = list(get_clang_format_binaries())
    if len(installed) == 0:
        sys.exit("*** could not locate a clang-format executable.")
    return max(installed, key=lambda v: v['bin_version'])


###############################################################################
# style file
###############################################################################


def locate_repo_style_file(repository):
    path = os.path.join(repository, 'src/.clang-format')
    if not os.path.exists(path):
        sys.exit("*** no style file at: %s" % path)
    return path


def parse_style_file(style_file):
    # Python does not have a built-in yaml parser, so here is a hand-written
    # one that *seems* to minimally work for this purpose.
    contents = read_file(style_file)
    # remove spaces after colon
    many_spaces = re.compile(': +')
    spaces_removed = many_spaces.sub(':', contents)
    # split into a list of lines
    lines = [l for l in spaces_removed.split('\n') if l != '']
    # split by the colon separator
    split = [l.split(':') for l in lines]
    # present as a dictionary
    return {item[0]: ''.join(item[1:]) for item in split}


###############################################################################
# UI
###############################################################################


if __name__ == "__main__":
    # parse arguments
    description = ("A utility for invoking clang-format to observe the state "
                   "of C++ code formatting in the repository. It produces "
                   "reports of style metrics and also can apply formatting.")
    parser = argparse.ArgumentParser(description=description)
    b_help = ("The path to the clang-format binary to be used. "
              "(default=clang-format-[0-9]\.[0-9] installed in PATH with the "
              "highest version number)")
    parser.add_argument("-b", "--bin-path", type=str,
                        action=BinPathAction, help=b_help)
    sf_help = ("The path to the style file to be used. (default=The "
               "src/.clang_format file of the repository which holds the "
               "targets)")
    parser.add_argument("-s", "--style-file", type=str,
                        action=StylePathAction, help=sf_help)
    j_help = ("Parallel jobs for computing diffs. (default=6)")
    parser.add_argument("-j", "--jobs", type=int, default=6, help=j_help)
    f_help = ("Force proceeding with 'check' or 'format' if clang-format "
              "doesn't support all parameters in the style file. "
              "(default=False)")
    parser.add_argument("-f", "--force", action='store_true', help=f_help)
    s_help = ("Selects the action to be taken. 'report' produces a report "
              "with analysis of the selected files taken as a group. 'check' "
              "validates that the selected files match the style and gives "
              "a per-file report and returns a non-zero bash status if there "
              "are any format issues discovered. 'format' applies the style "
              "formatting to the selected files.")
    parser.add_argument("subcommand", type=str,
                        choices=['report', 'check', 'format'], help=s_help)
    t_help = ("A list of files and/or directories that select the subset of "
              "files for this action. If a directory is given as a target, "
              "all files contained in it and its subdirectories are "
              "recursively selected. All targets must be tracked in the same "
              "git repository clone. (default=The current directory)")
    parser.add_argument("target", type=str, action=TargetsPathAction,
                        nargs='*', default='.', help=t_help)
    opts = parser.parse_args()

    # finish setting up parameters
    if not opts.bin_path:
        installed = locate_installed_binary()
        opts.bin_path = installed['bin_path']
        opts.bin_version = installed['bin_version']
    else:
        opts.bin_version = get_clang_format_version(opts.bin_path)
    if not opts.style_file:
        opts.style_file = locate_repo_style_file(opts.repository)
    opts.style_params = parse_style_file(opts.style_file)
    opts.unknown_style_params = []

    # execute commands
    os.chdir(opts.repository)
    if opts.subcommand == 'report':
        exec_report(opts)
    elif opts.subcommand == 'check':
        exec_check(opts)
    else:
        exec_format(opts)
