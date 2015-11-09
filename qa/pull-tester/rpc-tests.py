#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Run Regression Test Suite
#

import os
import sys
import subprocess
import re
from tests_config import *
from sets import Set

#If imported values are not defined then set to zero (or disabled)
if not vars().has_key('ENABLE_WALLET'):
    ENABLE_WALLET=0
if not vars().has_key('ENABLE_BITCOIND'):
    ENABLE_BITCOIND=0
if not vars().has_key('ENABLE_UTILS'):
    ENABLE_UTILS=0
if not vars().has_key('ENABLE_ZMQ'):
    ENABLE_ZMQ=0

#Create a set to store arguments and create the passOn string
opts = Set()
passOn = ""
p = re.compile("^--")
for i in range(1,len(sys.argv)):
    if (p.match(sys.argv[i]) or sys.argv[i] == "-h"):
        passOn += " " + sys.argv[i]
    else:
        opts.add(sys.argv[i])

#Set env vars
buildDir = BUILDDIR
os.environ["BITCOIND"] = buildDir + '/src/bitcoind' + EXEEXT
os.environ["BITCOINCLI"] = buildDir + '/src/bitcoin-cli' + EXEEXT

#Disable Windows tests by default
if EXEEXT == ".exe" and "-win" not in opts:
    print "Win tests currently disabled.  Use -win option to enable"
    sys.exit(0)

#Tests
testScripts = [
    'wallet.py',
    'listtransactions.py',
    'mempool_resurrect_test.py',
    'txn_doublespend.py --mineblock',
    'txn_clone.py',
    'getchaintips.py',
    'rawtransactions.py',
    'rest.py',
    'mempool_spendcoinbase.py',
    'mempool_coinbase_spends.py',
    'httpbasics.py',
    'zapwallettxes.py',
    'proxy_test.py',
    'merkle_blocks.py',
    'fundrawtransaction.py',
    'signrawtransactions.py',
    'walletbackup.py',
    'nodehandling.py',
    'reindex.py',
    'decodescript.py',
    'p2p-fullblocktest.py',
    'blockchain.py',
    'disablewallet.py',
]
testScriptsExt = [
    'bip65-cltv.py',
    'bip65-cltv-p2p.py',
    'bipdersig-p2p.py',
    'bipdersig.py',
    'getblocktemplate_longpoll.py',
    'getblocktemplate_proposals.py',
    'txn_doublespend.py',
    'txn_clone.py --mineblock',
    'pruning.py',
    'forknotify.py',
    'invalidateblock.py',
    'keypool.py',
    'receivedby.py',
#    'rpcbind_test.py', #temporary, bug in libevent, see #6655
#    'script_test.py', #used for manual comparison of 2 binaries
    'smartfees.py',
    'maxblocksinflight.py',
    'invalidblockrequest.py',
    'p2p-acceptblock.py',
    'mempool_packages.py',
    'maxuploadtarget.py',
]

#Enable ZMQ tests
if ENABLE_ZMQ == 1:
    testScripts.append('zmq_test.py')

if(ENABLE_WALLET == 1 and ENABLE_UTILS == 1 and ENABLE_BITCOIND == 1):
    rpcTestDir = buildDir + '/qa/rpc-tests/'
    #Run Tests
    for i in range(len(testScripts)):
       if (len(opts) == 0 or (len(opts) == 1 and "-win" in opts ) or '-extended' in opts
           or testScripts[i] in opts or  re.sub(".py$", "", testScripts[i]) in opts ):
            print  "Running testscript " + testScripts[i] + "..."
            subprocess.check_call(rpcTestDir + testScripts[i] + " --srcdir " + buildDir + '/src ' + passOn,shell=True)
	    #exit if help is called so we print just one set of instructions
            p = re.compile(" -h| --help")
            if p.match(passOn):
                sys.exit(0)

    #Run Extended Tests
    for i in range(len(testScriptsExt)):
        if ('-extended' in opts or testScriptsExt[i] in opts
           or re.sub(".py$", "", testScriptsExt[i]) in opts):
            print  "Running 2nd level testscript " + testScriptsExt[i] + "..."
            subprocess.check_call(rpcTestDir + testScriptsExt[i] + " --srcdir " + buildDir + '/src ' + passOn,shell=True)
else:
    print "No rpc tests to run. Wallet, utils, and bitcoind must all be enabled"
