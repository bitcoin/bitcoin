#!/usr/bin/env python2
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Copyright (c) 2015-2016 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Run Regression Test Suite

This module calls down into individual test cases via subprocess. It will
forward all unrecognized arguments onto the individual test scripts, other
than:

    - `-extended`: run the "extended" test suite in addition to the basic one.
    - `-win`: signal that this is running in a Windows environment, and we
      should run the tests.
    - `--coverage`: this generates a basic coverage report for the RPC
      interface.

For a description of arguments recognized by test scripts, see
`qa/pull-tester/test_framework/test_framework.py:BitcoinTestFramework.main`.

"""

import os
import time
import shutil
import sys
import subprocess
import tempfile
import re

from tests_config import *
from test_classes import RpcTest, Disabled, Skip

#If imported values are not defined then set to zero (or disabled)
if not vars().has_key('ENABLE_WALLET'):
    ENABLE_WALLET=0
if not vars().has_key('ENABLE_BITCOIND'):
    ENABLE_BITCOIND=0
if not vars().has_key('ENABLE_UTILS'):
    ENABLE_UTILS=0
if not vars().has_key('ENABLE_ZMQ'):
    ENABLE_ZMQ=0

ENABLE_COVERAGE=0

#Create a set to store arguments and create the passOn string
opts = set()
passOn = ""
p = re.compile("^--")

bold = ("","")
if (os.name == 'posix'):
    bold = ('\033[0m', '\033[1m')

for arg in sys.argv[1:]:
    if arg == '--coverage':
        ENABLE_COVERAGE = 1
    elif (p.match(arg) or arg == "-h"):
        passOn += " " + arg
    else:
        opts.add(arg)

#Set env vars
buildDir = BUILDDIR
if "BITCOIND" not in os.environ:
    os.environ["BITCOIND"] = buildDir + '/src/bitcoind' + EXEEXT
if "BITCOINCLI" not in os.environ:
    os.environ["BITCOINCLI"] = buildDir + '/src/bitcoin-cli' + EXEEXT

#Disable Windows tests by default
if EXEEXT == ".exe" and "-win" not in opts:
    print "Win tests currently disabled.  Use -win option to enable"
    sys.exit(0)

#Tests
testScripts = [ RpcTest(t) for t in [
    'bip68-112-113-p2p',
    'wallet',
    'excessive',
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
    Disabled('sendheaders', "BU requests INVs not headers -- in the future we may add support for headers, at least by treating them like INVs)"),
    'keypool',
    Disabled('prioritise_transaction', "TODO"),
    Disabled('invalidblockrequest', "TODO"),
    'invalidtxrequest',
    'abandonconflict',
    'p2p-versionbits-warning'
    ,
] ]

testScriptsExt = [ RpcTest(t) for t in [
    'bip9-softforks',
    'bip65-cltv',
    'bip65-cltv-p2p',
    Disabled('bip68-sequence', "TODO"),
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




def runtests():
    coverage = None

    run_only_extended = '-only-extended' in opts

    if ENABLE_COVERAGE:
        coverage = RPCCoverage()
        print("Initializing coverage directory at %s\n" % coverage.dir)

    if(ENABLE_WALLET == 1 and ENABLE_UTILS == 1 and ENABLE_BITCOIND == 1):
        rpcTestDir = buildDir + '/qa/rpc-tests/'
        run_extended = ('-extended' in opts) or run_only_extended
        cov_flag = coverage.flag if coverage else ''
        flags = " --srcdir %s/src %s %s" % (buildDir, cov_flag, passOn)

        #Run Tests
        for i in range(len(testScripts)):
            if ((len(opts) == 0
                    or (len(opts) == 1 and "-win" in opts )
                    or run_extended
                    or str(testScripts[i]) in opts
                    or re.sub(".py$", "", str(testScripts[i])) in opts )
                and not run_only_extended):

                if testScripts[i].is_disabled():
                    print("Disabled testscript %s%s%s (reason: %s)" % (bold[1], testScripts[i], bold[0], testScripts[i].reason))
                elif testScripts[i].is_skipped():
                    print("Skipping testscript %s%s%s on this platform (reason: %s)" % (bold[1], testScripts[i], bold[0], testScripts[i].reason))
                else:
                    # not disabled or skipped - execute test

                    print("Running testscript %s%s%s ..." % (bold[1], testScripts[i], bold[0]))
                    time0 = time.time()
                    subprocess.check_call(
                        rpcTestDir + repr(testScripts[i]) + flags, shell=True)
                    print("Duration: %s s\n" % (int(time.time() - time0)))

                    # exit if help is called so we print just one set of
                    # instructions
                    p = re.compile(" -h| --help")
                    if p.match(passOn):
                        sys.exit(0)

        # Run Extended Tests
        for i in range(len(testScriptsExt)):
            if (run_extended or str(testScriptsExt[i]) in opts
                    or re.sub(".py$", "", str(testScriptsExt[i])) in opts):

                if testScriptsExt[i].is_disabled():
                    print("Disabled testscript %s%s%s (reason: %s)" % (bold[1], testScriptsExt[i], bold[0], testScriptsExt[i].reason))
                elif testScripts[i].is_skipped():
                    print("Skipping testscript %s%s%s on this platform (reason: %s)" % (bold[1], testScriptsExt[i], bold[0], testScriptsExt[i].reason))
                else:
                    # not disabled or skipped - execute test
                    print(
                        "Running 2nd level testscript "
                        + "%s%s%s ..." % (bold[1], testScriptsExt[i], bold[0]))
                    time0 = time.time()
                    subprocess.check_call(
                        rpcTestDir + str(testScriptsExt[i]) + flags, shell=True)
                    print("Duration: %s s\n" % (int(time.time() - time0)))

        if coverage:
            coverage.report_rpc_coverage()

            print("Cleaning up coverage data")
            coverage.cleanup()

    else:
        print "No rpc tests to run. Wallet, utils, and bitcoind must all be enabled"


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
