#!/usr/bin/env python
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Exercise the wallet keypool, and interaction with wallet encryption/locking

# Add python-bitcoinrpc to module search path:
import os
import sys
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "python-bitcoinrpc"))

import json
import shutil
import subprocess
import tempfile
import traceback

from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *


def check_array_result(object_array, to_match, expected):
    """
    Pass in array of JSON objects, a dictionary with key/value pairs
    to match against, and another dictionary with expected key/value
    pairs.
    """
    num_matched = 0
    for item in object_array:
        all_match = True
        for key,value in to_match.items():
            if item[key] != value:
                all_match = False
        if not all_match:
            continue
        for key,value in expected.items():
            if item[key] != value:
                raise AssertionError("%s : expected %s=%s"%(str(item), str(key), str(value)))
            num_matched = num_matched+1
    if num_matched == 0:
        raise AssertionError("No objects matched %s"%(str(to_match)))

def run_test(nodes, tmpdir):
    # Encrypt wallet and wait to terminate
    nodes[0].encryptwallet('test')
    bitcoind_processes[0].wait()
    # Restart node 0
    nodes[0] = start_node(0, tmpdir)
    # Keep creating keys
    addr = nodes[0].getnewaddress()
    try:
        addr = nodes[0].getnewaddress()
        raise AssertionError('Keypool should be exhausted after one address')
    except JSONRPCException,e:
        assert(e.error['code']==-12)

    # put three new keys in the keypool
    nodes[0].walletpassphrase('test', 12000)
    nodes[0].keypoolrefill(3)
    nodes[0].walletlock()

    # drain the keys
    addr = set()
    addr.add(nodes[0].getrawchangeaddress())
    addr.add(nodes[0].getrawchangeaddress())
    addr.add(nodes[0].getrawchangeaddress())
    addr.add(nodes[0].getrawchangeaddress())
    # assert that four unique addresses were returned
    assert(len(addr) == 4)
    # the next one should fail
    try:
        addr = nodes[0].getrawchangeaddress()
        raise AssertionError('Keypool should be exhausted after three addresses')
    except JSONRPCException,e:
        assert(e.error['code']==-12)


def main():
    import optparse

    parser = optparse.OptionParser(usage="%prog [options]")
    parser.add_option("--nocleanup", dest="nocleanup", default=False, action="store_true",
                      help="Leave bitcoinds and test.* datadir on exit or error")
    parser.add_option("--srcdir", dest="srcdir", default="../../src",
                      help="Source directory containing bitcoind/bitcoin-cli (default: %default%)")
    parser.add_option("--tmpdir", dest="tmpdir", default=tempfile.mkdtemp(prefix="test"),
                      help="Root directory for datadirs")
    (options, args) = parser.parse_args()

    os.environ['PATH'] = options.srcdir+":"+os.environ['PATH']

    check_json_precision()

    success = False
    nodes = []
    try:
        print("Initializing test directory "+options.tmpdir)
        if not os.path.isdir(options.tmpdir):
            os.makedirs(options.tmpdir)
        initialize_chain(options.tmpdir)

        nodes = start_nodes(1, options.tmpdir)

        run_test(nodes, options.tmpdir)

        success = True

    except AssertionError as e:
        print("Assertion failed: "+e.message)
    except JSONRPCException as e:
        print("JSONRPC error: "+e.error['message'])
        traceback.print_tb(sys.exc_info()[2])
    except Exception as e:
        print("Unexpected exception caught during testing: "+str(sys.exc_info()[0]))
        traceback.print_tb(sys.exc_info()[2])

    if not options.nocleanup:
        print("Cleaning up")
        stop_nodes(nodes)
        wait_bitcoinds()
        shutil.rmtree(options.tmpdir)

    if success:
        print("Tests successful")
        sys.exit(0)
    else:
        print("Failed")
        sys.exit(1)

if __name__ == '__main__':
    main()
