#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Copyright (c) 2017-2018 The Raven Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Run regression test suite.

This module calls down into individual test cases via subprocess. It will
forward all unrecognized arguments onto the individual test scripts.

Functional tests are disabled on Windows by default. Use --force to run them anyway.

For a description of arguments recognized by test scripts, see
`test/functional/test_framework/test_framework.py:RavenTestFramework.main`.

"""
from collections import deque
import argparse
import configparser
import datetime
import os
import time
import shutil
import signal
import sys
import subprocess
import tempfile
import re
import logging

# Formatting. Default colors to empty strings.
BOLD, BLUE, RED, GREY = ("", ""), ("", ""), ("", ""), ("", "")
try:
    # Make sure python thinks it can write unicode to its stdout
    "\u2713".encode("utf_8").decode(sys.stdout.encoding)
    TICK = "✓ "
    CROSS = "✖ "
    CIRCLE = "○ "
except UnicodeDecodeError:
    TICK = "P "
    CROSS = "x "
    CIRCLE = "o "

if os.name == 'posix':
    # primitive formatting on supported
    # terminal via ANSI escape sequences:
    BOLD = ('\033[0m', '\033[1m')
    BLUE = ('\033[0m', '\033[0;34m')
    RED = ('\033[0m', '\033[0;31m')
    GREY = ('\033[0m', '\033[1;30m')

TEST_EXIT_PASSED = 0
TEST_EXIT_SKIPPED = 77

BASE_SCRIPTS= [
    # Scripts that are run by the travis build process.
    # Longest test should go first, to favor running tests in parallel
    # 'p2p_fingerprint.py',         TODO - fix mininode rehash methods to use X16R
    # 'p2p_invalid_block.py',       TODO - fix mininode rehash methods to use X16R
    # 'p2p_invalid_tx.py',          TODO - fix mininode rehash methods to use X16R
    # 'feature_segwit.py',          TODO - fix mininode rehash methods to use X16R
    # 'p2p_sendheaders.py',         TODO - fix mininode rehash methods to use X16R
    # 'feature_nulldummy.py',       TODO - fix mininode rehash methods to use X16R
    # 'mining_basic.py',            TODO - fix mininode rehash methods to use X16R
    # 'feature_dersig.py',          TODO - fix mininode rehash methods to use X16R
    # 'feature_cltv.py',            TODO - fix mininode rehash methods to use X16R
    # 'p2p_fullblock.py',           TODO - fix comptool.TestInstance timeout
    # 'p2p_compactblocks.py',       TODO - refactor to assume segwit is always active
    # 'p2p_segwit.py',              TODO - refactor to assume segwit is always active
    # 'feature_csv_activation.py',  TODO - currently testing softfork activations, we need to test the features
    # vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Tests less than 2m vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    'wallet_backup.py',
    'wallet_hd.py',
    # vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Tests less than 45s vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    'rpc_fundrawtransaction.py',
    # vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Tests less than 30s vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#    'wallet_basic.py',
    'mempool_limit.py',
    'feature_assets.py',
    'mining_prioritisetransaction.py',
    # vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Tests less than 15s vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    'rpc_rawtransaction.py',
    'rpc_addressindex.py',
    'wallet_dump.py',
    'wallet_bumpfee.py',
    'mempool_persist.py',
    'rpc_timestampindex.py',
    'wallet_listreceivedby.py',
    'interface_rest.py',
    'wallet_keypool_topup.py',
    'wallet_import_rescan.py',
    'wallet_abandonconflict.py',
    'rpc_blockchain.py',
    'p2p_leak.py',
    'p2p_versionbits.py',
    'rpc_spentindex.py',
    'feature_rawassettransactions.py',
    'wallet_importmulti.py',
    'wallet_accounts.py',
    # vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Tests less than 5s vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    'wallet_listtransactions.py',
    'feature_minchainwork.py',
    'wallet_encryption.py',
    'feature_listmyassets.py',
    'mempool_reorg.py',
    'rpc_merkle_blocks.py',
    'feature_reindex.py',
    'rpc_decodescript.py',
    'wallet_keypool.py',
    'wallet_listsinceblock.py',
    'wallet_zapwallettxes.py',
    'wallet_multiwallet.py',
    'interface_zmq.py',
    # vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Tests less than 3s vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    'rpc_getchaintips.py',
    'wallet_txn_clone.py',
    'wallet_txn_doublespend.py --mineblock',
    'feature_uacomment.py',
    'rpc_users.py',
    'feature_proxy.py',
    'rpc_txindex.py',
    'p2p_disconnect_ban.py',
    'wallet_importprunedfunds.py',
    'feature_unique_assets.py',
    'rpc_preciousblock.py',
    'rpc_net.py',
    'interface_raven_cli.py',
    'mempool_resurrect.py',
    'rpc_signrawtransaction.py',
    'wallet_resendtransactions.py',
    'rpc_signmessage.py',
    'rpc_deprecated.py',
    'wallet_disable.py',
    'interface_http.py',
    'mempool_spend_coinbase.py',
    'p2p_mempool.py',
    'rpc_named_arguments.py',
    'rpc_uptime.py',
    # Don't append tests at the end to avoid merge conflicts
    # Put them in a random line within the section that fits their approximate run-time
]

EXTENDED_SCRIPTS = [
    # These tests are not run by the travis build process.
    # Longest test should go first, to favor running tests in parallel
    'feature_pruning.py',
    # vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Tests less than 20m vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    'feature_fee_estimation.py',
    # vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Tests less than 5m vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    'feature_maxuploadtarget.py',
    'mempool_packages.py',
    'feature_dbcrash.py',
    # vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Tests less than 2m vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    'feature_bip68_sequence.py',
    'mining_getblocktemplate_longpoll.py',
    'p2p_timeouts.py',
    # vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Tests less than 60s vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    # use this for future soft fork testing --> 'feature_bip_softforks.py',
    'p2p_feefilter.py',
    'rpc_bind.py',
    # vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Tests less than 30s vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    'feature_assumevalid.py',
    #'example_test.py',
    'wallet_txn_doublespend.py',
    'wallet_txn_clone.py --mineblock',
    'feature_notifications.py',
    'rpc_invalidateblock.py',
    #'p2p_acceptblock.py',
    'feature_rbf.py',
]

# Place EXTENDED_SCRIPTS first since it has the 3 longest running tests
ALL_SCRIPTS = EXTENDED_SCRIPTS + BASE_SCRIPTS

NON_SCRIPTS = [
    # These are python files that live in the functional tests directory, but are not test scripts.
    "combine_logs.py",
    "create_cache.py",
    "test_runner.py",
]


def main():
    # Parse arguments and pass through unrecognised args
    parser = argparse.ArgumentParser(add_help=False,
                                     usage='%(prog)s [test_runner.py options] [script options] [scripts]',
                                     description=__doc__,
                                     epilog='''
    Help text and arguments for individual test script:''',
                                     formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--combinedlogslen', '-c', type=int, default=0, help='print a combined log (of length n lines) from all test nodes and test framework to the console on failure.')
    parser.add_argument('--coverage', action='store_true', help='generate a basic coverage report for the RPC interface')
    parser.add_argument('--exclude', '-x', help='specify a comma-separated-list of scripts to exclude.')
    parser.add_argument('--extended', action='store_true', help='run the extended test suite in addition to the basic tests')
    parser.add_argument('--onlyextended', action='store_true', help='run only the extended test suite')
    parser.add_argument('--force', '-f', action='store_true', help='run tests even on platforms where they are disabled by default (e.g. windows).')
    parser.add_argument('--help', '-h', '-?', action='store_true', help='print help text and exit')
    parser.add_argument('--jobs', '-j', type=int, default=4, help='how many test scripts to run in parallel. Default=4.')
    parser.add_argument('--keepcache', '-k', action='store_true', help='the default behavior is to flush the cache directory on startup. --keepcache retains the cache from the previous testrun.')
    parser.add_argument('--quiet', '-q', action='store_true', help='only print results summary and failure logs')
    parser.add_argument('--tmpdirprefix', '-t', default=tempfile.gettempdir(), help="Root directory for datadirs")
    args, unknown_args = parser.parse_known_args()

    # args to be passed on always start with two dashes; tests are the remaining unknown args
    tests = [arg for arg in unknown_args if arg[:2] != "--"]
    passon_args = [arg for arg in unknown_args if arg[:2] == "--"]

    # Read config generated by configure.
    config = configparser.ConfigParser()
    configfile = os.path.abspath(os.path.dirname(__file__)) + "/../config.ini"
    config.read_file(open(configfile, encoding="utf8"))

    passon_args.append("--configfile=%s" % configfile)

    # Set up logging
    logging_level = logging.INFO if args.quiet else logging.DEBUG
    logging.basicConfig(format='%(message)s', level=logging_level)

    # Create base test directory
    tmpdir = "%s/raven_test_runner_%s" % (args.tmpdirprefix, datetime.datetime.now().strftime("%Y%m%d_%H%M%S"))
    os.makedirs(tmpdir)

    logging.debug("Temporary test directory at %s" % tmpdir)

    enable_wallet = config["components"].getboolean("ENABLE_WALLET")
    enable_cli = config["components"].getboolean("ENABLE_UTILS")
    enable_ravend = config["components"].getboolean("ENABLE_RAVEND")

    if config["environment"]["EXEEXT"] == ".exe" and not args.force:
        # https://github.com/RavenProject/Ravencoin/commit/d52802551752140cf41f0d9a225a43e84404d3e9
        # https://github.com/RavenProject/Ravencoin/pull/5677#issuecomment-136646964
        print("Tests currently disabled on Windows by default. Use --force option to enable")
        sys.exit(0)

    if not (enable_wallet and enable_cli and enable_ravend):
        print("No functional tests to run. Wallet, utils, and ravend must all be enabled")
        print("Rerun `configure` with --enable-wallet, --with-cli and --with-daemon and rerun make")
        sys.exit(0)

    # Build list of tests
    if tests:
        # Individual tests have been specified. Run specified tests that exist
        # in the ALL_SCRIPTS list. Accept the name with or without .py extension.
        tests = [re.sub("\.py$", "", t) + ".py" for t in tests]
        test_list = []
        for t in tests:
            if t in ALL_SCRIPTS:
                test_list.append(t)
            else:
                print("{}WARNING!{} Test '{}' not found in full test list.".format(BOLD[1], BOLD[0], t))
    else:
        # No individual tests have been specified.
        # Run all base tests, and optionally run extended tests.
        test_list = BASE_SCRIPTS
        if args.extended:
            # place the EXTENDED_SCRIPTS first since the three longest ones
            # are there and the list is shorter
            test_list = EXTENDED_SCRIPTS + test_list
        elif args.onlyextended:
            test_list = EXTENDED_SCRIPTS

    # Remove the test cases that the user has explicitly asked to exclude.
    if args.exclude:
        tests_excl = [re.sub("\.py$", "", t) + ".py" for t in args.exclude.split(',')]
        for exclude_test in tests_excl:
            if exclude_test in test_list:
                test_list.remove(exclude_test)
            else:
                print("{}WARNING!{} Test '{}' not found in current test list.".format(BOLD[1], BOLD[0], exclude_test))

    if not test_list:
        print("No valid test scripts specified. Check that your test is in one "
              "of the test lists in test_runner.py, or run test_runner.py with no arguments to run all tests")
        sys.exit(0)

    if args.help:
        # Print help for test_runner.py, then print help of the first script (with args removed) and exit.
        parser.print_help()
        subprocess.check_call([(config["environment"]["SRCDIR"] + '/test/functional/' + test_list[0].split()[0])] + ['-h'])
        sys.exit(0)

    check_script_list(config["environment"]["SRCDIR"])
    check_script_prefixes()

    if not args.keepcache:
        shutil.rmtree("%s/test/cache" % config["environment"]["BUILDDIR"], ignore_errors=True)

    run_tests(test_list, config["environment"]["SRCDIR"], config["environment"]["BUILDDIR"], config["environment"]["EXEEXT"], tmpdir, args.jobs, args.coverage, passon_args, args.combinedlogslen)


def run_tests(test_list, src_dir, build_dir, exeext, tmpdir, jobs=1, enable_coverage=False, args=[], combined_logs_len=0):
    # Warn if ravend is already running (unix only)
    try:
        if subprocess.check_output(["pidof", "ravend"]) is not None:
            print("%sWARNING!%s There is already a ravend process running on this system. Tests may fail unexpectedly due to resource contention!" % (BOLD[1], BOLD[0]))
    except (OSError, subprocess.SubprocessError):
        pass

    # Warn if there is a cache directory
    cache_dir = "%s/test/cache" % build_dir
    if os.path.isdir(cache_dir):
        print("%sWARNING!%s There is a cache directory here: %s. If tests fail unexpectedly, try deleting the cache directory." % (BOLD[1], BOLD[0], cache_dir))

    #Set env vars
    if "RAVEND" not in os.environ:
        os.environ["RAVEND"] = build_dir + '/src/ravend' + exeext
        os.environ["RAVENCLI"] = build_dir + '/src/raven-cli' + exeext

    tests_dir = src_dir + '/test/functional/'

    flags = ["--srcdir={}/src".format(build_dir)] + args
    flags.append("--cachedir=%s" % cache_dir)

    if enable_coverage:
        coverage = RPCCoverage()
        flags.append(coverage.flag)
        logging.debug("Initializing coverage directory at %s" % coverage.dir)
    else:
        coverage = None

    if len(test_list) > 1 and jobs > 1:
        # Populate cache
        try:
            subprocess.check_output([tests_dir + 'create_cache.py'] + flags + ["--tmpdir=%s/cache" % tmpdir])
        except subprocess.CalledProcessError as e:
            print("\n----<test_runner>----\n")
            print("Error in create_cache.py:\n")
            for line in e.output.decode().split('\n'):
                print(line)
            print('\n')
            print(e.returncode)
            print('\n')
            print("\n----</test_runner>---\n")
            raise

    #Run Tests
    job_queue = TestHandler(jobs, tests_dir, tmpdir, test_list, flags)
    time0 = time.time()
    test_results = []

    max_len_name = len(max(test_list, key=len))

    for _ in range(len(test_list)):
        test_result, testdir, stdout, stderr = job_queue.get_next()
        test_results.append(test_result)
        if test_result.status == "Passed":
            logging.debug("\n%s%s%s passed, Duration: %s s" % (BOLD[1], test_result.name, BOLD[0], test_result.time))
        elif test_result.status == "Skipped":
            logging.debug("\n%s%s%s skipped" % (BOLD[1], test_result.name, BOLD[0]))
        else:
            logging.debug("\n%s%s%s failed, Duration: %s s\n" % (BOLD[1], test_result.name, BOLD[0], test_result.time))
            print("\n%s%s%s failed, Duration: %s s\n" % (BOLD[1], test_result.name, BOLD[0], test_result.time))
            print(BOLD[1] + 'stdout:\n' + BOLD[0] + stdout + '\n')
            print(BOLD[1] + 'stderr:\n' + BOLD[0] + stderr + '\n')

            if combined_logs_len and os.path.isdir(testdir):
                # Print the final `combinedlogslen` lines of the combined logs
                print('{}Combine the logs and print the last {} lines ...{}'.format(BOLD[1], combined_logs_len, BOLD[0]))
                print('\n============')
                print('{}Combined log for {}:{}'.format(BOLD[1], testdir, BOLD[0]))
                print('============\n')
                combined_logs, _ = subprocess.Popen([os.path.join(tests_dir, 'combine_logs.py'), '-c', testdir], universal_newlines=True, stdout=subprocess.PIPE).communicate()
                print("\n".join(deque(combined_logs.splitlines(), 4000)))
                print("\n".join(deque(combined_logs.splitlines(), combined_logs_len)))

        logging.debug("%s / %s tests ran" % (job_queue.num_finished, job_queue.num_jobs))

    print_results(test_results, max_len_name, (int(time.time() - time0)))

    if coverage:
        coverage.report_rpc_coverage()

        logging.debug("Cleaning up coverage data")
        coverage.cleanup()

    # Clear up the temp directory if all subdirectories are gone
    if not os.listdir(tmpdir):
        os.rmdir(tmpdir)

    all_passed = all(map(lambda test_result: test_result.was_successful, test_results))

    sys.exit(not all_passed)


def print_results(test_results, max_len_name, runtime):
    results = "\n" + BOLD[1] + "%s | %s | %s\n\n" % ("TEST".ljust(max_len_name), "STATUS   ", "DURATION") + BOLD[0]

    test_results.sort(key=lambda result: result.name.lower())
    all_passed = True
    time_sum = 0

    for test_result in test_results:
        all_passed = all_passed and test_result.was_successful
        time_sum += test_result.time
        test_result.padding = max_len_name
        results += str(test_result)

    status = TICK + "Passed" if all_passed else CROSS + "Failed"
    results += BOLD[1] + "\n%s | %s | %s s (accumulated) \n" % ("ALL".ljust(max_len_name), status.ljust(9), time_sum) + BOLD[0]
    results += "Runtime: %s s\n" % (runtime)
    print(results)


class TestHandler:
    """
    Trigger the test scripts passed in via the list.
    """


    def __init__(self, num_tests_parallel, tests_dir, tmpdir, test_list=None, flags=None):
        assert(num_tests_parallel >= 1)
        self.num_parallel_jobs = num_tests_parallel
        self.tests_dir = tests_dir
        self.tmpdir = tmpdir
        self.test_list = test_list
        self.flags = flags
        self.num_running = 0
        self.num_finished = 0
        self.num_jobs = len(test_list)
        self.jobs = []


    def get_next(self):
        while self.num_running < self.num_parallel_jobs and self.test_list:
            # Add tests
            self.num_running += 1
            t = self.test_list.pop(0)
            portseed = len(self.test_list)
            portseed_arg = ["--portseed={}".format(portseed)]
            log_stdout = tempfile.SpooledTemporaryFile(max_size=2**16)
            log_stderr = tempfile.SpooledTemporaryFile(max_size=2**16)
            test_argv = t.split()
            testdir = "{}/{}_{}".format(self.tmpdir, re.sub(".py$", "", test_argv[0]), portseed)
            tmpdir_arg = ["--tmpdir={}".format(testdir)]
            self.jobs.append((t,
                              time.time(),
                              subprocess.Popen([self.tests_dir + test_argv[0]] + test_argv[1:] + self.flags + portseed_arg + tmpdir_arg,
                                               universal_newlines=True, stdout=log_stdout, stderr=log_stderr), testdir, log_stdout,log_stderr))
        if not self.jobs:
            raise IndexError('pop from empty list')
        while True:
            # Return first proc that finishes
            time.sleep(.5)
            for j in self.jobs:
                (name, time0, proc, testdir, log_out, log_err) = j
                if os.getenv('TRAVIS') == 'true' and int(time.time() - time0) > 20 * 60:
                    # In travis, timeout individual tests after 20 minutes (to stop tests hanging and not
                    # providing useful output.
                    proc.send_signal(signal.SIGINT)
                if proc.poll() is not None:
                    log_out.seek(0), log_err.seek(0)
                    [stdout, stderr] = [l.read().decode('utf-8') for l in (log_out, log_err)]
                    log_out.close(), log_err.close()
                    if proc.returncode == TEST_EXIT_PASSED and stderr == "":
                        status = "Passed"
                    elif proc.returncode == TEST_EXIT_SKIPPED:
                        status = "Skipped"
                    else:
                        status = "Failed"
                    self.num_running -= 1
                    self.num_finished += 1
                    self.jobs.remove(j)

                    return TestResult(name, status, int(time.time() - time0)), testdir, stdout, stderr
            print('.', end='', flush=True)


class TestResult():
    def __init__(self, name, status, time):
        self.name = name
        self.status = status
        self.time = time
        self.padding = 0

    def __repr__(self):
        if self.status == "Passed":
            color = BLUE
            glyph = TICK
        elif self.status == "Failed":
            color = RED
            glyph = CROSS
        elif self.status == "Skipped":
            color = GREY
            glyph = CIRCLE

        return color[1] + "%s | %s%s | %s s\n" % (self.name.ljust(self.padding), glyph, self.status.ljust(7), self.time) + color[0]

    @property
    def was_successful(self):
        return self.status != "Failed"


def check_script_list(src_dir):
    """Check scripts directory.

    Check that there are no scripts in the functional tests directory which are
    not being run by pull-tester.py."""
    script_dir = src_dir + '/test/functional/'
    python_files = set([t for t in os.listdir(script_dir) if t[-3:] == ".py"])
    missed_tests = list(python_files - set(map(lambda x: x.split()[0], ALL_SCRIPTS + NON_SCRIPTS)))
    if len(missed_tests) != 0:
        print("%sWARNING!%s The following scripts are not being run: %s. Check the test lists in test_runner.py." % (BOLD[1], BOLD[0], str(missed_tests)))
        if os.getenv('TRAVIS') == 'true':
            # On travis this warning is an error to prevent merging incomplete commits into master
            sys.exit(1)


def check_script_prefixes():
    """Check that no more than `EXPECTED_VIOLATION_COUNT` of the
       test scripts don't start with one of the allowed name prefixes."""
    EXPECTED_VIOLATION_COUNT = 0
    # LEEWAY is provided as a transition measure, so that pull-requests
    # that introduce new tests that don't conform with the naming
    # convention don't immediately cause the tests to fail.
    LEEWAY = 1
    good_prefixes_re = re.compile("(example|feature|interface|mempool|mining|p2p|rpc|wallet)_")
    bad_script_names = [script for script in ALL_SCRIPTS if good_prefixes_re.match(script) is None]
    if len(bad_script_names) < EXPECTED_VIOLATION_COUNT:
        print("{}HURRAY!{} Number of functional tests violating naming convention reduced!".format(BOLD[1], BOLD[0]))
        print("Consider reducing EXPECTED_VIOLATION_COUNT from %d to %d" % (EXPECTED_VIOLATION_COUNT, len(bad_script_names)))
    elif len(bad_script_names) > EXPECTED_VIOLATION_COUNT:
        print("WARNING: %d tests not meeting naming conventions.  Please rename with allowed prefix. (expected %d):" % (len(bad_script_names), EXPECTED_VIOLATION_COUNT))
        print("  %s" % ("\n  ".join(sorted(bad_script_names))))
        assert len(bad_script_names) <= EXPECTED_VIOLATION_COUNT + LEEWAY, "Too many tests not following naming convention! (%d found, expected: <= %d)" % (len(bad_script_names), EXPECTED_VIOLATION_COUNT)


class RPCCoverage():
    """
    Coverage reporting utilities for test_runner.

    Coverage calculation works by having each test script subprocess write
    coverage files into a particular directory. These files contain the RPC
    commands invoked during testing, as well as a complete listing of RPC
    commands per `raven-cli help` (`rpc_interface.txt`).

    After all tests complete, the commands run are combined and diff'd against
    the complete list to calculate uncovered RPC commands.

    See also: test/functional/test_framework/coverage.py

    """
    def __init__(self):
        self.dir = tempfile.mkdtemp(prefix="coverage")
        self.flag = '--coveragedir=%s' % self.dir

    def report_rpc_coverage(self):
        """
        Print out RPC commands that were unexercised by tests.

        """
        uncovered = self._get_uncovered_rpc_commands()

        if uncovered:
            print("Uncovered RPC commands:")
            print("".join(("  - %s\n" % i) for i in sorted(uncovered)))
        else:
            print("All RPC commands covered.")

    def cleanup(self):
        return shutil.rmtree(self.dir)

    def _get_uncovered_rpc_commands(self):
        """
        Return a set of currently untested RPC commands.

        """
        # This is shared from `test/functional/test-framework/coverage.py`
        reference_filename = 'rpc_interface.txt'
        coverage_file_prefix = 'coverage.'

        coverage_ref_filename = os.path.join(self.dir, reference_filename)
        coverage_filenames = set()
        all_cmds = set()
        covered_cmds = set()

        if not os.path.isfile(coverage_ref_filename):
            raise RuntimeError("No coverage reference found")

        with open(coverage_ref_filename, 'r', encoding="utf8") as f:
            all_cmds.update([i.strip() for i in f.readlines()])

        for root, dirs, files in os.walk(self.dir):
            for filename in files:
                if filename.startswith(coverage_file_prefix):
                    coverage_filenames.add(os.path.join(root, filename))

        for filename in coverage_filenames:
            with open(filename, 'r', encoding="utf8") as f:
                covered_cmds.update([i.strip() for i in f.readlines()])

        return all_cmds - covered_cmds


if __name__ == '__main__':
    main()
