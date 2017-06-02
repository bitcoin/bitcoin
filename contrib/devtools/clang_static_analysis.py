#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import sys
import os
import subprocess
import time
import plistlib
import itertools
import argparse

###############################################################################
# parse .plist files in directory into issues
###############################################################################


def find_events(paths, files):
    for p in paths:
        if p['kind'] == 'event':
            yield {'message': p['extended_message'],
                   'line':    p['location']['line'],
                   'col':     p['location']['col'],
                   'file':    files[p['location']['file']]}


def plist_to_issue(plist):
    files = plist['files']
    for d in plist['diagnostics']:
        yield {'type':        d['type'],
               'description': d['description'],
               'line':        d['location']['line'],
               'col':         d['location']['col'],
               'file':        files[d['location']['file']],
               'events':      list(find_events(d['path'], files))}


def parse_plist_files(directory):
    plist_files = [os.path.join(directory, f) for f in os.listdir(directory) if
                   f.endswith('.plist')]
    read_plists = [plistlib.readPlist(plist_file) for plist_file in
                   plist_files]
    relevant_plists = [plist for plist in read_plists if
                       len(plist['diagnostics']) > 0]
    return list(itertools.chain(*[plist_to_issue(plist) for plist in
                                  relevant_plists]))


###############################################################################
# do analysis
###############################################################################


def call_cmd(cmd, outfile):
    file = open(os.path.abspath(outfile), 'w')
    if subprocess.call(cmd.split(' '), stdout=file, stderr=file) != 0:
        sys.exit("*** '%s' returned a non-zero status" % cmd)
    file.close()


def find_results_subdirectory(opts):
    # scan-build puts results in a subdirectory where the directory name is a
    # timestamp. e.g. /tmp/bitcoin-scan-build/2017-01-23-115243-901-1
    # We want the most recent directory, so we sort and return the path name
    # sorted highest.
    subdir = sorted([d for d in os.listdir(opts.report_path) if
                     os.path.isdir(os.path.join(opts.report_path, d))])[-1]
    return os.path.join(opts.report_path, subdir)


def run_scan_build(opts):
    print("Running command:     %s" % opts.make_clean_cmd)
    call_cmd(opts.make_clean_cmd, opts.make_clean_log)
    print("Running command:     %s" % opts.scan_build_cmd)
    print("stderr/stdout to:    %s" % opts.scan_build_log)
    print("This might take a few minutes...")
    call_cmd(opts.scan_build_cmd, opts.scan_build_log)
    print("Done.")
    directory = find_results_subdirectory(opts)
    print("Results in:          %s\n" % directory)
    return directory


def do_analysis(opts):
    result_subdir = run_scan_build(opts)
    return result_subdir, parse_plist_files(result_subdir)


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
# issue reporting
###############################################################################


def report_issues_compact(issues):
    issue_no = 0
    for issue in issues:
        report("%d: %s:%d:%d - %s\n" % (issue_no, issue['file'], issue['line'],
                                        issue['col'], issue['description']))
        issue_no = issue_no + 1


def report_issue(issue):
    report("An issue has been found in ")
    red_report("%s:%d:%d\n" % (issue['file'], issue['line'], issue['col']))
    report("Type:         %s\n" % issue['type'])
    report("Description:  %s\n\n" % issue['description'])
    event_no = 0
    for event in issue['events']:
        report("%d: " % event_no)
        report("%s:%d:%d - " % (event['file'], event['line'], event['col']))
        report("%s\n" % event['message'])
        event_no = event_no + 1


def report_issues(issues):
    for issue in issues:
        report_issue(issue)
        report(SEPARATOR)


###############################################################################
# 'report' subcommand execution
###############################################################################


def report_output(opts, result_subdir, issues, elapsed_time):
    report(SEPARATOR)
    report("Took %.2f seconds to analyze with scan-build\n" % elapsed_time)
    report("Found %d issues:\n" % len(issues))
    report(SEPARATOR)
    if len(issues) > 0:
        report_issues_compact(issues)
        report(SEPARATOR)
        report("Full details can be seen in a browser by running:\n")
        report("    $ %s %s\n" % (opts.scan_view, result_subdir))
        report(SEPARATOR)
    flush_report()


def exec_report(opts):
    start_time = time.time()
    result_subdir, issues = do_analysis(opts)
    elapsed_time = time.time() - start_time
    report_output(opts, result_subdir, issues, elapsed_time)


###############################################################################
# 'check' subcommand execution
###############################################################################


def check_output(opts, result_subdir, issues):
    report(SEPARATOR)
    report_issues(issues)
    if len(issues) == 0:
        green_report("No static analysis issues found!\n")
    else:
        red_report("Full details can be seen in a browser by running:\n")
        report("    $ %s %s\n" % (opts.scan_view, result_subdir))
    report(SEPARATOR)
    flush_report()


def exec_check(opts):
    result_subdir, issues = do_analysis(opts)
    check_output(opts, result_subdir, issues)
    if len(issues) > 0:
        sys.exit("*** Static analysis issues found!")


###############################################################################
# validate inputs
###############################################################################


class PathAction(argparse.Action):
    def _path_exists(self, path):
        return os.path.exists(path)

    def _assert_exists(self, path):
        if not self._path_exists(path):
            sys.exit("*** does not exist: %s" % path)

    def _assert_mode(self, path, flags):
        if not os.access(path, flags):
            sys.exit("*** %s does not have correct mode: %x" % (path, flags))


class RepositoryPathAction(PathAction):
    def _assert_has_makefile(self, path):
        if not self._path_exists(os.path.join(path, "Makefile")):
            sys.exit("*** no Makefile found in %s. You must ./autogen.sh "
                     "and/or ./configure first" % path)

    def _assert_git_repository(self, path):
        cmd = 'git -C %s status' % path
        dn = open(os.devnull, 'w')
        if (subprocess.call(cmd.split(' '), stderr=dn, stdout=dn) != 0):
            sys.exit("*** %s is not a git repository" % path)

    def __call__(self, parser, namespace, values, option_string=None):
        path = os.path.abspath(values)
        self._assert_exists(path)
        self._assert_git_repository(path)
        self._assert_has_makefile(path)
        namespace.repository = path


class ReportPathAction(PathAction):
    def __call__(self, parser, namespace, values, option_string=None):
        path = os.path.abspath(values)
        self._assert_exists(path)
        self._assert_mode(path, os.R_OK | os.W_OK)
        namespace.report_path = path


class BinPathAction(PathAction):
    def __call__(self, parser, namespace, values, option_string=None):
        path = os.path.abspath(values)
        self._assert_exists(path)
        scan_build = os.path.join(path, 'scan-build')
        scan_view = os.path.join(path, 'scan-view')

        self._assert_exists(scan_build)
        self._assert_mode(scan_build, os.R_OK | os.X_OK)
        self._assert_exists(scan_view)
        self._assert_mode(scan_view, os.R_OK | os.X_OK)

        namespace.scan_build = scan_build
        namespace.scan_view = scan_view


###############################################################################
# helpers for defaults
###############################################################################


DEFAULT_REPORT_PATH = "/tmp/bitcoin-scan-build/"


def make_report_path_if_missing():
    if not os.path.exists(DEFAULT_REPORT_PATH):
        os.makedirs(DEFAULT_REPORT_PATH)


def locate_installed_binaries():
    def which(binary):
        out = subprocess.check_output(['which', binary])
        lines = [l for l in out.decode("utf-8").split('\n') if l != '']
        if len(lines) != 1:
            sys.exit("*** could not find installed %s" % binary)
        return lines[0]
    return (os.path.realpath(which('scan-build')),
            os.path.realpath(which('scan-view')))


###############################################################################
# UI
###############################################################################


if __name__ == "__main__":
    # parse arguments
    description = ("A utility for running clang static analysis on the "
                   "codebase in a consistent way.")
    parser = argparse.ArgumentParser(description=description)
    b_help = ("The path holding 'scan-build' and 'scan-view' binaries. "
              "(Uses 'scan-build' and 'scan-view' installed in PATH by "
              "default)")
    parser.add_argument("-b", "--bin-path", type=str,
                        action=BinPathAction, help=b_help)
    r_help = ("The path for scan-build to write its report files. "
              "(default=/tmp/bitcoin-scan-build/)")
    parser.add_argument("-r", "--report-path",
                        default=DEFAULT_REPORT_PATH,
                        type=str, action=ReportPathAction, help=r_help)
    j_help = "The number of parallel jobs to run with 'make'. (default=6)"
    parser.add_argument("-j", "--jobs", type=int, default=6, help=j_help)
    s_help = ("Selects the output behavior. 'report' generates a summary "
              "report on the issues found. 'check' compares the state of the "
              "repository against a standard, provides a return code for the "
              "shell and prints more specific details when issues are found.")
    parser.add_argument("subcommand", type=str, choices=['report', 'check'],
                        help=s_help)
    repo_help = ("A source code repository for which the static analysis is "
                 "to be performed upon.")
    parser.add_argument("repository", type=str, action=RepositoryPathAction,
                        help=repo_help)
    opts = parser.parse_args()

    # additional setup for default opts
    if not (hasattr(opts, 'scan_build') and hasattr(opts, 'scan_view')):
        opts.scan_build, opts.scan_view = locate_installed_binaries()
    if opts.report_path == DEFAULT_REPORT_PATH:
        make_report_path_if_missing()

    # non-configurable defaults
    opts.make_clean_cmd = 'make clean'
    opts.make_clean_log = 'make_clean.log'
    opts.scan_build_log = 'scan_build.log'
    opts.scan_build_cmd = ('%s -k -plist-html --keep-empty -o %s make -j%d' %
                           (opts.scan_build, opts.report_path, opts.jobs))

    # execute commands
    os.chdir(opts.repository)
    if opts.subcommand == 'report':
        exec_report(opts)
    else:
        exec_check(opts)
