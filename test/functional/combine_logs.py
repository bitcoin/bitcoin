#!/usr/bin/env python3
# Copyright (c) 2017-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Combine logs from multiple bitcoin nodes as well as the test_framework log.

This streams the combined log output to stdout. Use combine_logs.py > outputfile
to write to an outputfile.

If no argument is provided, the most recent test directory will be used."""

import argparse
from collections import defaultdict, namedtuple
import heapq
import itertools
import os
import pathlib
import re
import sys
import tempfile

# N.B.: don't import any local modules here - this script must remain executable
# without the parent module installed.

# Should match same symbol in `test_framework.test_framework`.
TMPDIR_PREFIX = "bitcoin_func_test_"

# Matches on the date format at the start of the log event
TIMESTAMP_PATTERN = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d{6})?Z")

LogEvent = namedtuple('LogEvent', ['timestamp', 'source', 'event'])

def main():
    """Main function. Parses args, reads the log files and renders them as text or html."""
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument(
        'testdir', nargs='?', default='',
        help=('temporary test directory to combine logs from. '
              'Defaults to the most recent'))
    parser.add_argument('-c', '--color', dest='color', action='store_true', help='outputs the combined log with events colored by source (requires posix terminal colors. Use less -r for viewing)')
    parser.add_argument('--html', dest='html', action='store_true', help='outputs the combined log as html. Requires jinja2. pip install jinja2')
    args = parser.parse_args()

    if args.html and args.color:
        print("Only one out of --color or --html should be specified")
        sys.exit(1)

    testdir = args.testdir or find_latest_test_dir()

    if not testdir:
        print("No test directories found")
        sys.exit(1)

    if not args.testdir:
        print("Opening latest test directory: {}".format(testdir), file=sys.stderr)

    colors = defaultdict(lambda: '')
    if args.color:
        colors["test"] = "\033[0;36m"  # CYAN
        colors["node0"] = "\033[0;34m"  # BLUE
        colors["node1"] = "\033[0;32m"  # GREEN
        colors["node2"] = "\033[0;31m"  # RED
        colors["node3"] = "\033[0;33m"  # YELLOW
        colors["reset"] = "\033[0m"  # Reset font color

    log_events = read_logs(testdir)

    if args.html:
        print_logs_html(log_events)
    else:
        print_logs_plain(log_events, colors)
        print_node_warnings(testdir, colors)


def read_logs(tmp_dir):
    """Reads log files.

    Delegates to generator function get_log_events() to provide individual log events
    for each of the input log files."""

    # Find out what the folder is called that holds the debug.log file
    glob = pathlib.Path(tmp_dir).glob('node0/**/debug.log')
    path = next(glob, None)
    if path:
        assert next(glob, None) is None #  more than one debug.log, should never happen
        chain = re.search(r'node0/(.+?)/debug\.log$', path.as_posix()).group(1)  # extract the chain name
    else:
        chain = 'regtest'  # fallback to regtest (should only happen when none exists)

    files = [("test", "%s/test_framework.log" % tmp_dir)]
    for i in itertools.count():
        logfile = "{}/node{}/{}/debug.log".format(tmp_dir, i, chain)
        if not os.path.isfile(logfile):
            break
        files.append(("node%d" % i, logfile))

    return heapq.merge(*[get_log_events(source, f) for source, f in files])


def print_node_warnings(tmp_dir, colors):
    """Print nodes' errors and warnings"""

    warnings = []
    for stream in ['stdout', 'stderr']:
        for i in itertools.count():
            folder = "{}/node{}/{}".format(tmp_dir, i, stream)
            if not os.path.isdir(folder):
                break
            for (_, _, fns) in os.walk(folder):
                for fn in fns:
                    warning = pathlib.Path('{}/{}'.format(folder, fn)).read_text().strip()
                    if warning:
                        warnings.append(("node{} {}".format(i, stream), warning))

    print()
    for w in warnings:
        print("{} {} {} {}".format(colors[w[0].split()[0]], w[0], w[1], colors["reset"]))


def find_latest_test_dir():
    """Returns the latest tmpfile test directory prefix."""
    tmpdir = tempfile.gettempdir()

    def join_tmp(basename):
        return os.path.join(tmpdir, basename)

    def is_valid_test_tmpdir(basename):
        fullpath = join_tmp(basename)
        return (
            os.path.isdir(fullpath)
            and basename.startswith(TMPDIR_PREFIX)
            and os.access(fullpath, os.R_OK)
        )

    testdir_paths = [
        join_tmp(name) for name in os.listdir(tmpdir) if is_valid_test_tmpdir(name)
    ]

    return max(testdir_paths, key=os.path.getmtime) if testdir_paths else None


def get_log_events(source, logfile):
    """Generator function that returns individual log events.

    Log events may be split over multiple lines. We use the timestamp
    regex match as the marker for a new log event."""
    try:
        with open(logfile, 'r', encoding='utf-8') as infile:
            event = ''
            timestamp = ''
            for line in infile:
                # skip blank lines
                if line == '\n':
                    continue
                # if this line has a timestamp, it's the start of a new log event.
                time_match = TIMESTAMP_PATTERN.match(line)
                if time_match:
                    if event:
                        yield LogEvent(timestamp=timestamp, source=source, event=event.rstrip())
                    timestamp = time_match.group()
                    if time_match.group(1) is None:
                        # timestamp does not have microseconds. Add zeroes.
                        timestamp_micro = timestamp.replace("Z", ".000000Z")
                        line = line.replace(timestamp, timestamp_micro)
                        timestamp = timestamp_micro
                    event = line
                # if it doesn't have a timestamp, it's a continuation line of the previous log.
                else:
                    # Add the line. Prefix with space equivalent to the source + timestamp so log lines are aligned
                    event += "                                   " + line
            # Flush the final event
            yield LogEvent(timestamp=timestamp, source=source, event=event.rstrip())
    except FileNotFoundError:
        print("File %s could not be opened. Continuing without it." % logfile, file=sys.stderr)


def print_logs_plain(log_events, colors):
    """Renders the iterator of log events into text."""
    for event in log_events:
        lines = event.event.splitlines()
        print("{0} {1: <5} {2} {3}".format(colors[event.source.rstrip()], event.source, lines[0], colors["reset"]))
        if len(lines) > 1:
            for line in lines[1:]:
                print("{0}{1}{2}".format(colors[event.source.rstrip()], line, colors["reset"]))


def print_logs_html(log_events):
    """Renders the iterator of log events into html."""
    try:
        import jinja2
    except ImportError:
        print("jinja2 not found. Try `pip install jinja2`")
        sys.exit(1)
    print(jinja2.Environment(loader=jinja2.FileSystemLoader('./'))
                    .get_template('combined_log_template.html')
                    .render(title="Combined Logs from testcase", log_events=[event._asdict() for event in log_events]))


if __name__ == '__main__':
    main()
