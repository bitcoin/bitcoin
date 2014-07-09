#!/usr/bin/env python
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Base class for RPC testing

# Add python-bitcoinrpc to module search path:
import os
import sys
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "python-bitcoinrpc"))

import shutil
import tempfile
import traceback

from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *


class BitcoinTestFramework(object):

    # These may be over-ridden by subclasses:
    def run_test(self, nodes):
            assert_equal(node.getblockcount(), 200)
            assert_equal(node.getbalance(), 25*50)

    def add_options(self, parser):
        pass

    def setup_chain(self, tmp_directory):
        print("Initializing test directory "+tmp_directory)
        initialize_chain(tmp_directory)

    def setup_network(self, tmp_directory):
        nodes = start_nodes(2, tmp_directory)
        connect_nodes(nodes[1], 0)
        sync_blocks(nodes)
        return nodes

    def main(self):
        import optparse

        parser = optparse.OptionParser(usage="%prog [options]")
        parser.add_option("--nocleanup", dest="nocleanup", default=False, action="store_true",
                          help="Leave bitcoinds and test.* datadir on exit or error")
        parser.add_option("--srcdir", dest="srcdir", default="../../src",
                          help="Source directory containing bitcoind/bitcoin-cli (default: %default%)")
        parser.add_option("--tmpdir", dest="tmpdir", default=tempfile.mkdtemp(prefix="test"),
                          help="Root directory for datadirs")
        self.add_options(parser)
        (self.options, self.args) = parser.parse_args()

        os.environ['PATH'] = self.options.srcdir+":"+os.environ['PATH']

        check_json_precision()

        success = False
        nodes = []
        try:
            if not os.path.isdir(self.options.tmpdir):
                os.makedirs(self.options.tmpdir)
            self.setup_chain(self.options.tmpdir)

            nodes = self.setup_network(self.options.tmpdir)

            self.run_test(nodes)

            success = True

        except JSONRPCException as e:
            print("JSONRPC error: "+e.error['message'])
            traceback.print_tb(sys.exc_info()[2])
        except AssertionError as e:
            print("Assertion failed: "+e.message)
            traceback.print_tb(sys.exc_info()[2])
        except Exception as e:
            print("Unexpected exception caught during testing: "+str(e))
            traceback.print_tb(sys.exc_info()[2])

        if not self.options.nocleanup:
            print("Cleaning up")
            stop_nodes(nodes)
            wait_bitcoinds()
            shutil.rmtree(self.options.tmpdir)

        if success:
            print("Tests successful")
            sys.exit(0)
        else:
            print("Failed")
            sys.exit(1)
