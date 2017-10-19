#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Copyright (c) 2015-2017 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Run Regression Test Suite

This module calls down into individual test cases via subprocess. It will
forward all unrecognized arguments onto the individual test scripts, other
than:

    - `-h` or '--help': print help about all options
    - `-extended`: run the "extended" test suite in addition to the basic one.
    - `-extended-only`: run ONLY the "extended" test suite
    - `-list`: only list the test scripts, do not run. Works in combination
      with '-extended' and '-extended-only' too, to print subsets.
    - `-win`: signal that this is running in a Windows environment, and we
      should run the tests.
    - `--coverage`: this generates a basic coverage report for the RPC
      interface.

For more detailed help on options, run with '--help'.

For a description of arguments recognized by test scripts, see
`qa/pull-tester/test_framework/test_framework.py:BitcoinTestFramework.main`.

"""
import pdb
import os
import time
import shutil
import signal
import sys
import subprocess
import tempfile
import re

sys.path.append("qa/pull-tester/")
from tests_config import *
from test_classes import RpcTest, Disabled, Skip

BOLD = ("","")
if os.name == 'posix':
    # primitive formatting on supported
    # terminal via ANSI escape sequences:
    BOLD = ('\033[0m', '\033[1m')

RPC_TESTS_DIR = SRCDIR + '/qa/rpc-tests/'

#If imported values are not defined then set to zero (or disabled)
if 'ENABLE_WALLET' not in vars():
    ENABLE_WALLET=0
if 'ENABLE_BITCOIND' not in vars():
    ENABLE_BITCOIND=0
if 'ENABLE_UTILS' not in vars():
    ENABLE_UTILS=0
if 'ENABLE_ZMQ' not in vars():
    ENABLE_ZMQ=0

ENABLE_COVERAGE=0

#Create a set to store arguments and create the passOn string
opts = set()
double_opts = set()  # BU: added for checking validity of -- opts
passOn = ""
showHelp = False  # if we need to print help
p = re.compile("^--")
p_parallel = re.compile('^-parallel=')
run_parallel = 4

# some of the single-dash options applicable only to this runner script
# are also allowed in double-dash format (but are not passed on to the
# test scripts themselves)
private_single_opts = ('-h',
                       '-f',    # equivalent to -force-enable
                       '-help',
                       '-list',
                       '-extended',
                       '-extended-only',
                       '-only-extended',
                       '-force-enable',
                       '-win')
private_double_opts = ('--list',
                       '--extended',
                       '--extended-only',
                       '--only-extended',
                       '--force-enable',
                       '--win')
framework_opts = ('--tracerpc',
                  '--help',
                  '--noshutdown',
                  '--nocleanup',
                  '--srcdir',
                  '--tmpdir',
                  '--coveragedir',
                  '--randomseed',
                  '--testbinary',
                  '--refbinary')
test_script_opts = ('--mineblock',
                    '--extensive')

def option_passed(option_without_dashes):
    """check if option was specified in single-dash or double-dash format"""
    return ('-' + option_without_dashes in opts
            or '--' + option_without_dashes in double_opts)

bold = ("","")
if (os.name == 'posix'):
    bold = ('\033[0m', '\033[1m')

for arg in sys.argv[1:]:
    if arg == '--coverage':
        ENABLE_COVERAGE = 1
    elif (p.match(arg) or arg in ('-h', '-help')):
        if arg not in private_double_opts:
            if arg == '--help' or arg == '-help' or arg == '-h':
                passOn = '--help'
                showHelp = True
            else:
                if passOn is not '--help':
                    passOn += " " + arg
        # add it to double_opts only for validation
        double_opts.add(arg)
    elif p_parallel.match(arg):
        run_parallel = int(arg.split(sep='=', maxsplit=1)[1])

    else:
        # this is for single-dash options only
        # they are interpreted only by this script
        opts.add(arg)

# check for unrecognized options
bad_opts_found = []
bad_opt_str="Unrecognized option: %s"
for o in opts | double_opts:
    if o.startswith('--'):
        if o not in framework_opts + test_script_opts + private_double_opts:
            print(bad_opt_str % o)
            bad_opts_found.append(o)
    elif o.startswith('-'):
        if o not in private_single_opts:
            print(bad_opt_str % o)
            bad_opts_found.append(o)
            print("Run with -h to get help on usage.")
            sys.exit(1)

#Set env vars
if "BITCOIND" not in os.environ:
    os.environ["BITCOIND"] = BUILDDIR + '/src/bitcoind' + EXEEXT
if "BITCOINCLI" not in os.environ:
    os.environ["BITCOINCLI"] = BUILDDIR + '/src/bitcoin-cli' + EXEEXT

#Disable Windows tests by default
if EXEEXT == ".exe" and not option_passed('win'):
    print("Win tests currently disabled.  Use -win option to enable")
    sys.exit(0)

if not (ENABLE_WALLET == 1 and ENABLE_UTILS == 1 and ENABLE_BITCOIND == 1):
    print("No rpc tests to run. Wallet, utils, and bitcoind must all be enabled")
    sys.exit(0)

# python3-zmq may not be installed. Handle this gracefully and with some helpful info
if ENABLE_ZMQ:
    try:
        import zmq
    except ImportError as e:
        print("ERROR: \"import zmq\" failed. Set ENABLE_ZMQ=0 or " \
            "to run zmq tests, see dependency info in /qa/README.md.")
        raise e

#Tests
testScripts = [ RpcTest(t) for t in [
    'bip68-112-113-p2p',
    'validateblocktemplate',
    'parallel',
    'wallet',
    'excessive',
    'buip055',
    'listtransactions',
    'receivedby',
    'mempool_resurrect_test',
    'txn_doublespend --mineblock',
    'txn_clone',
    'getchaintips',
    'rawtransactions',
    'rest',
    'mempool_spendcoinbase',
    'mempool_reorg',
    Disabled('mempool_limit', "mempool priority changes causes create_lots_of_big_transactions to fail"),
    'httpbasics',
    'multi_rpc',
    'zapwallettxes',
    'proxy_test',
    'merkle_blocks',
    'fundrawtransaction',
    'signrawtransactions',
    'walletbackup',
    'nodehandling',
    'reindex',
    'decodescript',
    Disabled('p2p-fullblocktest', "TODO"),
    'blockchain',
    'disablewallet',
    'sendheaders',
    'keypool',
    Disabled('prioritise_transaction', "TODO"),
    Disabled('invalidblockrequest', "TODO"),
    'invalidtxrequest',
    'abandonconflict',
    'p2p-versionbits-warning',
    'importprunedfunds',
    'thinblocks'
] ]

testScriptsExt = [ RpcTest(t) for t in [
    'txPerf',
    'excessive --extensive',
    'parallel --extensive',
    'bip9-softforks',
    'bip65-cltv',
    'bip65-cltv-p2p',
    'bip68-sequence',
    'bipdersig-p2p',
    'bipdersig',
    'getblocktemplate_longpoll',
    'getblocktemplate_proposals',
    'txn_doublespend',
    'txn_clone --mineblock',
    Disabled('pruning', "too much disk"),
    'forknotify',
    'invalidateblock',
    Disabled('rpcbind_test', "temporary, bug in libevent, see #6655"),
    'smartfees',
    'maxblocksinflight',
    'p2p-acceptblock',
    'mempool_packages',
    'maxuploadtarget',
    Disabled('replace-by-fee', "disabled while Replace By Fee is disabled in code")
] ]

#Enable ZMQ tests
if ENABLE_ZMQ == 1:
    testScripts.append(RpcTest('zmq_test'))


def show_wrapper_options():
    """ print command line options specific to wrapper """
    print("Wrapper options:")
    print()
    print("  -extended/--extended  run the extended set of tests")
    print("  -only-extended / -extended-only\n" + \
          "  --only-extended / --extended-only\n" + \
          "                        run ONLY the extended tests")
    print("  -list / --list        only list test names")
    print("  -win / --win          signal running on Windows and run those tests")
    print("  -f / -force-enable / --force-enable\n" + \
          "                        attempt to run disabled/skipped tests")
    print("  -h / -help / --help   print this help")

def runtests():
    global passOn
    coverage = None
    test_passed = []
    disabled = []
    skipped = []
    tests_to_run = []

    force_enable = option_passed('force-enable') or '-f' in opts
    run_only_extended = option_passed('only-extended') or option_passed('extended-only')

    if option_passed('list'):
        if run_only_extended:
            for t in testScriptsExt:
                print(t)
        else:
            for t in testScripts:
                print(t)
            if option_passed('extended'):
                for t in testScriptsExt:
                    print(t)
        sys.exit(0)

    if ENABLE_COVERAGE:
        coverage = RPCCoverage()
        print("Initializing coverage directory at %s\n" % coverage.dir)

    if(ENABLE_WALLET == 1 and ENABLE_UTILS == 1 and ENABLE_BITCOIND == 1):
        rpcTestDir = RPC_TESTS_DIR
        buildDir   = BUILDDIR
        run_extended = option_passed('extended') or run_only_extended
        cov_flag = coverage.flag if coverage else ''
        flags = " --srcdir %s/src %s %s" % (buildDir, cov_flag, passOn)

        # compile the list of tests to check

        # check for explicit tests
        if showHelp:
            tests_to_run = [ testScripts[0] ]
        else:
            for o in opts:
                if not o.startswith('-'):
                    found = False
                    for t in testScripts + testScriptsExt:
                        t_rep = str(t).split(' ')
                        if (t_rep[0] == o or t_rep[0] == o + '.py') and len(t_rep) > 1:
                            # it is a test with args - check all args match what was passed, otherwise don't add this test
                            t_args = t_rep[1:]
                            all_args_found = True
                            for targ in t_args:
                                if not targ in passOn.split(' '):
                                    all_args_found = False
                            if all_args_found:
                                tests_to_run.append(t)
                                found = True
                        elif t_rep[0] == o or t_rep[0] == o + '.py':
                            passOnSplit = [x for x in passOn.split(' ') if x != '']
                            found_non_framework_opt = False
                            for p in passOnSplit:
                                if p in test_script_opts:
                                    found_non_framework_opt = True
                            if not found_non_framework_opt:
                                tests_to_run.append(t)
                                found = True
                    if not found:
                        print("Error: %s is not a known test." % o)
                        sys.exit(1)

        # if no explicit tests specified, use the lists
        if not len(tests_to_run):
            if run_only_extended:
                tests_to_run = testScriptsExt
            else:
                tests_to_run += testScripts
                if run_extended:
                    tests_to_run += testScriptsExt

        # weed out the disabled / skipped tests and print them beforehand
        # this allows earlier intervention in case a test is unexpectedly
        # skipped
        if not force_enable:
            trimmed_tests_to_run = []
            for t in tests_to_run:
                if t.is_disabled():
                    print("Disabled testscript %s%s%s (reason: %s)" % (bold[1], t, bold[0], t.reason))
                    disabled.append(str(t))
                elif t.is_skipped():
                    print("Skipping testscript %s%s%s on this platform (reason: %s)" % (bold[1], t, bold[0], t.reason))
                    skipped.append(str(t))
                else:
                    trimmed_tests_to_run.append(t)
            tests_to_run = trimmed_tests_to_run

        if len(tests_to_run) > 1 and run_parallel:
            # Populate cache
            subprocess.check_output([RPC_TESTS_DIR + 'create_cache.py'] + [flags])

        tests_to_run = list(map(str,tests_to_run))
        max_len_name = len(max(tests_to_run, key=len))
        time_sum = 0
        time0 = time.time()
        job_queue = RPCTestHandler(run_parallel, tests_to_run, flags)
        results = BOLD[1] + "%s | %s | %s\n\n" % ("TEST".ljust(max_len_name), "PASSED", "DURATION") + BOLD[0]
        all_passed = True

        for _ in range(len(tests_to_run)):
            (name, stdout, stderr, passed, duration) = job_queue.get_next()
            test_passed.append(passed)
            all_passed = all_passed and passed
            time_sum += duration

            print('\n' + BOLD[1] + name + BOLD[0] + ":")
            print(stdout)
            print('stderr:\n' if not stderr == '' else '', stderr)
            results += "%s | %s | %s s\n" % (name.ljust(max_len_name), str(passed).ljust(6), duration)
            print("Pass: %s%s%s, Duration: %s s\n" % (BOLD[1], passed, BOLD[0], duration))

        results += BOLD[1] + "\n%s | %s | %s s (accumulated)" % ("ALL".ljust(max_len_name), str(all_passed).ljust(6), time_sum) + BOLD[0]
        print(results)
        print("\nRuntime: %s s" % (int(time.time() - time0)))

        if coverage:
            coverage.report_rpc_coverage()

            print("Cleaning up coverage data")
            coverage.cleanup()

        if not showHelp:
            # show some overall results and aggregates
            print()
            print("%d test(s) passed / %d test(s) failed / %d test(s) executed" % (test_passed.count(True),
                                                                       test_passed.count(False),
                                                                       len(test_passed)))
            print("%d test(s) disabled / %d test(s) skipped due to platform" % (len(disabled), len(skipped)))

        # signal that tests have failed using exit code
        sys.exit(not all_passed)

    else:
        print("No rpc tests to run. Wallet, utils, and bitcoind must all be enabled")

class RPCTestHandler:
    """
    Trigger the testscrips passed in via the list.
    """

    def __init__(self, num_tests_parallel, test_list=None, flags=None):
        assert(num_tests_parallel >= 1)
        self.num_jobs = num_tests_parallel
        self.test_list = test_list
        self.flags = flags
        self.num_running = 0
        # In case there is a graveyard of zombie bitcoinds, we can apply a
        # pseudorandom offset to hopefully jump over them.
        # (625 is PORT_RANGE/MAX_NODES)
        self.portseed_offset = int(time.time() * 1000) % 625
        self.jobs = []

    def get_next(self):
        while self.num_running < self.num_jobs and self.test_list:
            # Add tests
            self.num_running += 1
            t = self.test_list.pop(0)
            port_seed = ["--portseed={}".format(len(self.test_list) + self.portseed_offset)]
            log_stdout = tempfile.SpooledTemporaryFile(max_size=2**16)
            log_stderr = tempfile.SpooledTemporaryFile(max_size=2**16)
            self.jobs.append((t,
                              time.time(),
                              subprocess.Popen((RPC_TESTS_DIR + t).split() + self.flags.split() + port_seed,
                                               universal_newlines=True,
                                               stdout=subprocess.PIPE,
                                               stderr=subprocess.PIPE)))
        if not self.jobs:
            raise IndexError('pop from empty list')
        while True:
            # Return first proc that finishes
            time.sleep(.5)
            for j in self.jobs:
                (name, time0, proc) = j
                if os.getenv('TRAVIS') == 'true' and int(time.time() - time0) > 20 * 60:
                    # In travis, timeout individual tests after 20 minutes (to stop tests hanging and not
                    # providing useful output.
                    proc.send_signal(signal.SIGINT)
                if proc.poll() is not None:
                    (stdout, stderr) = proc.communicate(timeout=3)
                    passed = stderr == "" and proc.returncode == 0
                    self.num_running -= 1
                    self.jobs.remove(j)
                    return name, stdout, stderr, passed, int(time.time() - time0)
            print('.', end='', flush=True)

class RPCCoverage(object):
    """
    Coverage reporting utilities for pull-tester.

    Coverage calculation works by having each test script subprocess write
    coverage files into a particular directory. These files contain the RPC
    commands invoked during testing, as well as a complete listing of RPC
    commands per `bitcoin-cli help` (`rpc_interface.txt`).

    After all tests complete, the commands run are combined and diff'd against
    the complete list to calculate uncovered RPC commands.

    See also: qa/rpc-tests/test_framework/coverage.py

    """
    def __init__(self):
        self.dir = tempfile.mkdtemp(prefix="coverage")
        self.flag = '--coveragedir %s' % self.dir

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
        # This is shared from `qa/rpc-tests/test-framework/coverage.py`
        REFERENCE_FILENAME = 'rpc_interface.txt'
        COVERAGE_FILE_PREFIX = 'coverage.'

        coverage_ref_filename = os.path.join(self.dir, REFERENCE_FILENAME)
        coverage_filenames = set()
        all_cmds = set()
        covered_cmds = set()

        if not os.path.isfile(coverage_ref_filename):
            raise RuntimeError("No coverage reference found")

        with open(coverage_ref_filename, 'r') as f:
            all_cmds.update([i.strip() for i in f.readlines()])

        for root, dirs, files in os.walk(self.dir):
            for filename in files:
                if filename.startswith(COVERAGE_FILE_PREFIX):
                    coverage_filenames.add(os.path.join(root, filename))

        for filename in coverage_filenames:
            with open(filename, 'r') as f:
                covered_cmds.update([i.strip() for i in f.readlines()])

        return all_cmds - covered_cmds


if __name__ == '__main__':
    runtests()
