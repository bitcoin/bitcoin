#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Run regression test suite.

This module calls down into individual test cases via subprocess. It will
forward all unrecognized arguments onto the individual test scripts.

Functional tests are disabled on Windows by default. Use --force to run them anyway.

For a description of arguments recognized by test scripts, see
`test/functional/test_framework/test_framework.py:BitcoinTestFramework.main`.

"""

import argparse
from collections import deque
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

# 30 minutes represented in seconds
TRAVIS_TIMEOUT_DURATION = 30 * 60

BASE_SCRIPTS = [
    # Scripts that are run by the travis build process.
    # Longest test should go first, to favor running tests in parallel
    'feature_dip3_deterministicmns.py', # NOTE: needs dash_hash to pass
    'feature_block_reward_reallocation.py',
    'feature_llmq_data_recovery.py',
    'feature_fee_estimation.py',
    'wallet_hd.py',
    'wallet_backup.py',
    # vv Tests less than 5m vv
    'mining_getblocktemplate_longpoll.py', # FIXME: "socket.error: [Errno 54] Connection reset by peer" on my Mac, same as https://github.com/bitcoin/bitcoin/issues/6651
    'feature_maxuploadtarget.py',
    'feature_block.py', # NOTE: needs dash_hash to pass
    'rpc_fundrawtransaction.py',
    'rpc_fundrawtransaction_hd.py',
    'wallet_multiwallet.py --usecli',
    'p2p_quorum_data.py',
    # vv Tests less than 2m vv
    'p2p_instantsend.py',
    'wallet_basic.py',
    'wallet_labels.py',
    'p2p_timeouts.py',
    'feature_bip68_sequence.py',
    'wallet_dump.py',
    'wallet_listtransactions.py',
    'feature_multikeysporks.py',
    'feature_llmq_signing.py', # NOTE: needs dash_hash to pass
    'feature_llmq_signing.py --spork21', # NOTE: needs dash_hash to pass
    'feature_llmq_chainlocks.py', # NOTE: needs dash_hash to pass
    'feature_llmq_connections.py', # NOTE: needs dash_hash to pass
    'feature_llmq_simplepose.py', # NOTE: needs dash_hash to pass
    'feature_llmq_is_cl_conflicts.py', # NOTE: needs dash_hash to pass
    'feature_llmq_is_retroactive.py', # NOTE: needs dash_hash to pass
    'feature_llmq_dkgerrors.py', # NOTE: needs dash_hash to pass
    'feature_dip4_coinbasemerkleroots.py', # NOTE: needs dash_hash to pass
    # vv Tests less than 60s vv
    'p2p_sendheaders.py', # NOTE: needs dash_hash to pass
    'wallet_zapwallettxes.py',
    'wallet_importmulti.py',
    'mempool_limit.py',
    'rpc_txoutproof.py',
    'wallet_listreceivedby.py',
    'wallet_abandonconflict.py',
    'feature_csv_activation.py',
    'rpc_rawtransaction.py',
    'feature_reindex.py',
    'feature_abortnode.py',
    # vv Tests less than 30s vv
    'wallet_keypool_topup.py',
    'interface_zmq_dash.py',
    'interface_zmq.py',
    'interface_bitcoin_cli.py',
    'mempool_resurrect.py',
    'wallet_txn_doublespend.py --mineblock',
    'wallet_txn_clone.py',
    'rpc_getchaintips.py',
    'interface_rest.py',
    'mempool_spend_coinbase.py',
    'mempool_reorg.py',
    'mempool_persist.py',
    'wallet_multiwallet.py',
    'wallet_disableprivatekeys.py',
    'wallet_disableprivatekeys.py --usecli',
    'interface_http.py',
    'rpc_users.py',
    'feature_proxy.py',
    'rpc_signrawtransaction.py',
    'p2p_addrv2_relay.py',
    'wallet_groups.py',
    'p2p_disconnect_ban.py',
    'feature_addressindex.py',
    'feature_timestampindex.py',
    'feature_spentindex.py',
    'rpc_decodescript.py',
    'rpc_blockchain.py',
    'rpc_deprecated.py',
    'wallet_disable.py',
    'rpc_net.py',
    'wallet_keypool.py',
    'wallet_keypool_hd.py',
    'p2p_mempool.py',
    'mining_prioritisetransaction.py',
    'p2p_invalid_locator.py',
    'p2p_invalid_block.py',
    'p2p_invalid_tx.py',
    'feature_assumevalid.py',
    'example_test.py',
    'wallet_txn_doublespend.py',
    'wallet_txn_clone.py --mineblock',
    'feature_notifications.py',
    'rpc_invalidateblock.py',
    'feature_txindex.py',
    'mempool_packages.py',
    'feature_versionbits_warning.py',
    'rpc_preciousblock.py',
    'wallet_importprunedfunds.py',
    'rpc_zmq.py',
    'rpc_signmessage.py',
    'feature_nulldummy.py',
    'mempool_accept.py',
    'wallet_import_rescan.py',
    'rpc_bind.py --ipv4',
    'rpc_bind.py --ipv6',
    'rpc_bind.py --nonloopback',
    'mining_basic.py',
    'rpc_named_arguments.py',
    'wallet_listsinceblock.py',
    'p2p_leak.py',
    'p2p_compactblocks.py',
    'p2p_connect_to_devnet.py',
    'feature_sporks.py',
    'rpc_getblockstats.py',
    'wallet_encryption.py',
    'wallet_upgradetohd.py',
    'feature_dersig.py',
    'feature_cltv.py',
    'feature_new_quorum_type_activation.py',
    'feature_governance_objects.py',
    'rpc_uptime.py',
    'wallet_resendwallettransactions.py',
    'feature_minchainwork.py',
    'p2p_unrequested_blocks.py', # NOTE: needs dash_hash to pass
    'feature_shutdown.py',
    'rpc_coinjoin.py',
    'rpc_masternode.py',
    'rpc_mnauth.py',
    'rpc_verifyislock.py',
    'rpc_verifychainlock.py',
    'p2p_fingerprint.py',
    'rpc_platform_filter.py',
    'feature_dip0020_activation.py',
    'feature_uacomment.py',
    'p2p_unrequested_blocks.py',
    'feature_asmap.py',
    'feature_includeconf.py',
    'feature_logging.py',
    'p2p_node_network_limited.py',
    'feature_blocksdir.py',
    'feature_config_args.py',
    'rpc_help.py',
    'feature_help.py',
    # Don't append tests at the end to avoid merge conflicts
    # Put them in a random line within the section that fits their approximate run-time
]

EXTENDED_SCRIPTS = [
    # These tests are not run by the travis build process.
    # Longest test should go first, to favor running tests in parallel
    'feature_pruning.py', # NOTE: Prune mode is incompatible with -txindex, should work with governance validation disabled though.
    'feature_dbcrash.py',
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
    parser.add_argument('--combinedlogslen', '-c', type=int, default=0, metavar='n', help='On failure, print a log (of length n lines) to the console, combined from the test framework and all test nodes.')
    parser.add_argument('--coverage', action='store_true', help='generate a basic coverage report for the RPC interface')
    parser.add_argument('--ci', action='store_true', help='Run checks and code that are usually only enabled in a continuous integration environment')
    parser.add_argument('--exclude', '-x', help='specify a comma-separated-list of scripts to exclude.')
    parser.add_argument('--extended', action='store_true', help='run the extended test suite in addition to the basic tests')
    parser.add_argument('--force', '-f', action='store_true', help='run tests even on platforms where they are disabled by default (e.g. windows).')
    parser.add_argument('--help', '-h', '-?', action='store_true', help='print help text and exit')
    parser.add_argument('--jobs', '-j', type=int, default=4, help='how many test scripts to run in parallel. Default=4.')
    parser.add_argument('--keepcache', '-k', action='store_true', help='the default behavior is to flush the cache directory on startup. --keepcache retains the cache from the previous testrun.')
    parser.add_argument('--quiet', '-q', action='store_true', help='only print dots, results summary and failure logs')
    parser.add_argument('--tmpdirprefix', '-t', default=tempfile.gettempdir(), help="Root directory for datadirs")
    parser.add_argument('--failfast', action='store_true', help='stop execution after the first test failure')
    parser.add_argument('--filter', help='filter scripts to run by regular expression')
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
    tmpdir = "%s/test_runner_∋_🏃_%s" % (args.tmpdirprefix, datetime.datetime.now().strftime("%Y%m%d_%H%M%S"))
    os.makedirs(tmpdir)

    logging.debug("Temporary test directory at %s" % tmpdir)

    enable_wallet = config["components"].getboolean("ENABLE_WALLET")
    enable_utils = config["components"].getboolean("ENABLE_UTILS")
    enable_bitcoind = config["components"].getboolean("ENABLE_BITCOIND")

    if config["environment"]["EXEEXT"] == ".exe" and not args.force:
        # https://github.com/bitcoin/bitcoin/commit/d52802551752140cf41f0d9a225a43e84404d3e9
        # https://github.com/bitcoin/bitcoin/pull/5677#issuecomment-136646964
        print("Tests currently disabled on Windows by default. Use --force option to enable")
        sys.exit(0)

    if not (enable_wallet and enable_utils and enable_bitcoind):
        print("No functional tests to run. Wallet, utils, and dashd must all be enabled")
        print("Rerun `configure` with -enable-wallet, -with-utils and -with-daemon and rerun make")
        sys.exit(0)

    # Build list of tests
    test_list = []
    if tests:
        # Individual tests have been specified. Run specified tests that exist
        # in the ALL_SCRIPTS list. Accept names with or without a .py extension.
        # Specified tests can contain wildcards, but in that case the supplied
        # paths should be coherent, e.g. the same path as that provided to call
        # test_runner.py. Examples:
        #   `test/functional/test_runner.py test/functional/wallet*`
        #   `test/functional/test_runner.py ./test/functional/wallet*`
        #   `test_runner.py wallet*`
        #   but not:
        #   `test/functional/test_runner.py wallet*`
        # Multiple wildcards can be passed:
        #   `test_runner.py tool* mempool*`
        for test in tests:
            script = test.split("/")[-1]
            script = script + ".py" if ".py" not in script else script
            if script in ALL_SCRIPTS:
                test_list.append(script)
            else:
                print("{}WARNING!{} Test '{}' not found in full test list.".format(BOLD[1], BOLD[0], test))
    elif args.extended:
        # Include extended tests
        test_list += ALL_SCRIPTS
    else:
        # Run base tests only
        test_list += BASE_SCRIPTS

    # Remove the test cases that the user has explicitly asked to exclude.
    if args.exclude:
        exclude_tests = [re.sub("\.py$", "", test) + ".py" for test in args.exclude.split(',')]
        for exclude_test in exclude_tests:
            if exclude_test in test_list:
                test_list.remove(exclude_test)
            else:
                print("{}WARNING!{} Test '{}' not found in current test list.".format(BOLD[1], BOLD[0], exclude_test))

    if args.filter:
        test_list = list(filter(re.compile(args.filter).search, test_list))

    if not test_list:
        print("No valid test scripts specified. Check that your test is in one "
              "of the test lists in test_runner.py, or run test_runner.py with no arguments to run all tests")
        sys.exit(0)

    if args.help:
        # Print help for test_runner.py, then print help of the first script (with args removed) and exit.
        parser.print_help()
        subprocess.check_call([sys.executable, os.path.join(config["environment"]["SRCDIR"], 'test', 'functional', test_list[0].split()[0]), '-h'])
        sys.exit(0)

    check_script_list(src_dir=config["environment"]["SRCDIR"], fail_on_warn=args.ci)
    check_script_prefixes()

    if not args.keepcache:
        shutil.rmtree("%s/test/cache" % config["environment"]["BUILDDIR"], ignore_errors=True)

    run_tests(
        test_list=test_list,
        src_dir=config["environment"]["SRCDIR"],
        build_dir=config["environment"]["BUILDDIR"],
        tmpdir=tmpdir,
        jobs=args.jobs,
        enable_coverage=args.coverage,
        args=passon_args,
        runs_ci=args.ci,
        combined_logs_len=args.combinedlogslen,
        failfast=args.failfast
    )

def run_tests(*, test_list, src_dir, build_dir, tmpdir, jobs=1, enable_coverage=False, args=None, runs_ci=False, combined_logs_len=0,failfast=False):
    args = args or []

    # Warn if dashd is already running (unix only)
    try:
        pidof_output = subprocess.check_output(["pidof", "dashd"])
        if not (pidof_output is None or pidof_output == b''):
            print("%sWARNING!%s There is already a dashd process running on this system. Tests may fail unexpectedly due to resource contention!" % (BOLD[1], BOLD[0]))
    except (OSError, subprocess.SubprocessError):
        pass

    # Warn if there is a cache directory
    cache_dir = "%s/test/cache" % build_dir
    if os.path.isdir(cache_dir):
        print("%sWARNING!%s There is a cache directory here: %s. If tests fail unexpectedly, try deleting the cache directory." % (BOLD[1], BOLD[0], cache_dir))

    tests_dir = src_dir + '/test/functional/'

    flags = ['--cachedir={}'.format(cache_dir)] + args

    if enable_coverage:
        coverage = RPCCoverage()
        flags.append(coverage.flag)
        logging.debug("Initializing coverage directory at %s" % coverage.dir)
    else:
        coverage = None

    if len(test_list) > 1 and jobs > 1:
        # Populate cache
        try:
            subprocess.check_output([sys.executable, tests_dir + 'create_cache.py'] + flags + ["--tmpdir=%s/cache" % tmpdir])
        except subprocess.CalledProcessError as e:
            sys.stdout.buffer.write(e.output)
            raise

    #Run Tests
    job_queue = TestHandler(
        num_tests_parallel=jobs,
        tests_dir=tests_dir,
        tmpdir=tmpdir,
        test_list=test_list,
        flags=flags,
        timeout_duration=TRAVIS_TIMEOUT_DURATION if runs_ci else float('inf'),  # in seconds
    )
    start_time = time.time()
    test_results = []

    max_len_name = len(max(test_list, key=len))
    test_count = len(test_list)
    for i in range(test_count):
        test_result, testdir, stdout, stderr = job_queue.get_next()
        test_results.append(test_result)
        done_str = "{}/{} - {}{}{}".format(i + 1, test_count, BOLD[1], test_result.name, BOLD[0])
        if test_result.status == "Passed":
            logging.debug("%s passed, Duration: %s s" % (done_str, test_result.time))
        elif test_result.status == "Skipped":
            logging.debug("%s skipped" % (done_str))
        else:
            print("%s failed, Duration: %s s\n" % (done_str, test_result.time))
            print(BOLD[1] + 'stdout:\n' + BOLD[0] + stdout + '\n')
            print(BOLD[1] + 'stderr:\n' + BOLD[0] + stderr + '\n')
            if combined_logs_len and os.path.isdir(testdir):
                # Print the final `combinedlogslen` lines of the combined logs
                print('{}Combine the logs and print the last {} lines ...{}'.format(BOLD[1], combined_logs_len, BOLD[0]))
                print('\n============')
                print('{}Combined log for {}:{}'.format(BOLD[1], testdir, BOLD[0]))
                print('============\n')
                combined_logs, _ = subprocess.Popen([sys.executable, os.path.join(tests_dir, 'combine_logs.py'), '-c', testdir], universal_newlines=True, stdout=subprocess.PIPE).communicate()
                print("\n".join(deque(combined_logs.splitlines(), combined_logs_len)))

            if failfast:
                logging.debug("Early exiting after test failure")
                break

    print_results(test_results, max_len_name, (int(time.time() - start_time)))

    if coverage:
        coverage.report_rpc_coverage()

        logging.debug("Cleaning up coverage data")
        coverage.cleanup()

    # Clear up the temp directory if all subdirectories are gone
    if not os.listdir(tmpdir):
        os.rmdir(tmpdir)

    all_passed = all(map(lambda test_result: test_result.was_successful, test_results))

    # This will be a no-op unless failfast is True in which case there may be dangling
    # processes which need to be killed.
    job_queue.kill_and_join()

    sys.exit(not all_passed)

def print_results(test_results, max_len_name, runtime):
    results = "\n" + BOLD[1] + "%s | %s | %s\n\n" % ("TEST".ljust(max_len_name), "STATUS   ", "DURATION") + BOLD[0]

    test_results.sort(key=TestResult.sort_key)
    all_passed = True
    time_sum = 0

    for test_result in test_results:
        all_passed = all_passed and test_result.was_successful
        time_sum += test_result.time
        test_result.padding = max_len_name
        results += str(test_result)

    status = TICK + "Passed" if all_passed else CROSS + "Failed"
    if not all_passed:
        results += RED[1]
    results += BOLD[1] + "\n%s | %s | %s s (accumulated) \n" % ("ALL".ljust(max_len_name), status.ljust(9), time_sum) + BOLD[0]
    if not all_passed:
        results += RED[0]
    results += "Runtime: %s s\n" % (runtime)
    print(results)

class TestHandler:
    """
    Trigger the test scripts passed in via the list.
    """

    def __init__(self, *, num_tests_parallel, tests_dir, tmpdir, test_list, flags, timeout_duration):
        assert num_tests_parallel >= 1
        self.num_jobs = num_tests_parallel
        self.tests_dir = tests_dir
        self.tmpdir = tmpdir
        self.timeout_duration = timeout_duration
        self.test_list = test_list
        self.flags = flags
        self.num_running = 0
        self.jobs = []

    def get_next(self):
        while self.num_running < self.num_jobs and self.test_list:
            # Add tests
            self.num_running += 1
            test = self.test_list.pop(0)
            portseed = len(self.test_list)
            portseed_arg = ["--portseed={}".format(portseed)]
            log_stdout = tempfile.SpooledTemporaryFile(max_size=2**16)
            log_stderr = tempfile.SpooledTemporaryFile(max_size=2**16)
            test_argv = test.split()
            testdir = "{}/{}_{}".format(self.tmpdir, re.sub(".py$", "", test_argv[0]), portseed)
            tmpdir_arg = ["--tmpdir={}".format(testdir)]
            self.jobs.append((test,
                              time.time(),
                              subprocess.Popen([sys.executable, self.tests_dir + test_argv[0]] + test_argv[1:] + self.flags + portseed_arg + tmpdir_arg,
                                               universal_newlines=True,
                                               stdout=log_stdout,
                                               stderr=log_stderr),
                              testdir,
                              log_stdout,
                              log_stderr))
        if not self.jobs:
            raise IndexError('pop from empty list')
        dot_count = 0
        while True:
            # Return first proc that finishes
            time.sleep(.5)
            for job in self.jobs:
                (name, start_time, proc, testdir, log_out, log_err) = job
                if int(time.time() - start_time) > self.timeout_duration:
                    # In travis, timeout individual tests (to stop tests hanging and not providing useful output).
                    proc.send_signal(signal.SIGINT)
                if proc.poll() is not None:
                    log_out.seek(0), log_err.seek(0)
                    [stdout, stderr] = [log_file.read().decode('utf-8') for log_file in (log_out, log_err)]
                    log_out.close(), log_err.close()
                    if proc.returncode == TEST_EXIT_PASSED and stderr == "":
                        status = "Passed"
                    elif proc.returncode == TEST_EXIT_SKIPPED:
                        status = "Skipped"
                    else:
                        status = "Failed"
                    self.num_running -= 1
                    self.jobs.remove(job)
                    clearline = '\r' + (' ' * dot_count) + '\r'
                    print(clearline, end='', flush=True)
                    dot_count = 0
                    return TestResult(name, status, int(time.time() - start_time)), testdir, stdout, stderr
            print('.', end='', flush=True)
            dot_count += 1

    def kill_and_join(self):
        """Send SIGKILL to all jobs and block until all have ended."""
        procs = [i[2] for i in self.jobs]

        for proc in procs:
            proc.kill()

        for proc in procs:
            proc.wait()


class TestResult():
    def __init__(self, name, status, time):
        self.name = name
        self.status = status
        self.time = time
        self.padding = 0

    def sort_key(self):
        if self.status == "Passed":
            return 0, self.name.lower()
        elif self.status == "Failed":
            return 2, self.name.lower()
        elif self.status == "Skipped":
            return 1, self.name.lower()

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


def check_script_prefixes():
    """Check that test scripts start with one of the allowed name prefixes."""

    good_prefixes_re = re.compile("(example|feature|interface|mempool|mining|p2p|rpc|wallet)_")
    bad_script_names = [script for script in ALL_SCRIPTS if good_prefixes_re.match(script) is None]

    if bad_script_names:
        print("%sERROR:%s %d tests not meeting naming conventions:" % (BOLD[1], BOLD[0], len(bad_script_names)))
        print("  %s" % ("\n  ".join(sorted(bad_script_names))))
        raise AssertionError("Some tests are not following naming convention!")


def check_script_list(*, src_dir, fail_on_warn):
    """Check scripts directory.

    Check that there are no scripts in the functional tests directory which are
    not being run by pull-tester.py."""
    script_dir = src_dir + '/test/functional/'
    python_files = set([test_file for test_file in os.listdir(script_dir) if test_file.endswith(".py")])
    missed_tests = list(python_files - set(map(lambda x: x.split()[0], ALL_SCRIPTS + NON_SCRIPTS)))
    if len(missed_tests) != 0:
        print("%sWARNING!%s The following scripts are not being run: %s. Check the test lists in test_runner.py." % (BOLD[1], BOLD[0], str(missed_tests)))
        if fail_on_warn:
            # On travis this warning is an error to prevent merging incomplete commits into master
            sys.exit(1)

class RPCCoverage():
    """
    Coverage reporting utilities for test_runner.

    Coverage calculation works by having each test script subprocess write
    coverage files into a particular directory. These files contain the RPC
    commands invoked during testing, as well as a complete listing of RPC
    commands per `dash-cli help` (`rpc_interface.txt`).

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
            print("".join(("  - %s\n" % command) for command in sorted(uncovered)))
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

        with open(coverage_ref_filename, 'r', encoding="utf8") as coverage_ref_file:
            all_cmds.update([line.strip() for line in coverage_ref_file.readlines()])

        for root, dirs, files in os.walk(self.dir):
            for filename in files:
                if filename.startswith(coverage_file_prefix):
                    coverage_filenames.add(os.path.join(root, filename))

        for filename in coverage_filenames:
            with open(filename, 'r', encoding="utf8") as coverage_file:
                covered_cmds.update([line.strip() for line in coverage_file.readlines()])

        return all_cmds - covered_cmds


if __name__ == '__main__':
    main()
