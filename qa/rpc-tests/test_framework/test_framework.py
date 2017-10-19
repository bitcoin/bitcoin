#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Copyright (c) 2015-2017 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Base class for RPC testing

import logging
import optparse
import os
import sys
import time       # BU added
import random     # BU added
import pdb        # BU added
import shutil
import tempfile
import traceback

from .util import (
    initialize_chain,
    assert_equal,
    start_nodes,
    connect_nodes_bi,
    sync_blocks,
    sync_mempools,
    stop_nodes,
    wait_bitcoinds,
    enable_coverage,
    check_json_precision,
    initialize_chain_clean,
    PortSeed,
)
from .authproxy import JSONRPCException


class BitcoinTestFramework(object):
    drop_to_pdb = os.getenv("DROP_TO_PDB", "")

    # These may be over-ridden by subclasses:
    def run_test(self):
        for node in self.nodes:
            assert_equal(node.getblockcount(), 200)
            assert_equal(node.getbalance(), 25*50)

    def add_options(self, parser):
        pass

    def setup_chain(self,bitcoinConfDict=None, wallets=None):
        """
        Sets up the blockchain for the bitcoin nodes.  It also sets up the daemon configuration.
        bitcoinConfDict:  Pass a dictionary of values you want written to bitcoin.conf.  If you have a key with multiple values, pass a list of the values as the value, for example:
        { "debug":["net","blk","thin","lck","mempool","req","bench","evict"] }        
        This framework provides values for the necessary fields (like regtest=1).  But you can override these
        defaults by setting them in this dictionary.

        wallets: Pass a list of wallet filenames.  Each wallet file will be copied into the node's directory
        before starting the node.
        """
        print("Initializing test directory ", self.options.tmpdir, "Bitcoin conf: ", str(bitcoinConfDict), "walletfiles: ", wallets)
        initialize_chain(self.options.tmpdir,bitcoinConfDict, wallets)

    def setup_nodes(self):
        return start_nodes(4, self.options.tmpdir)

    def setup_network(self, split = False):
        self.nodes = self.setup_nodes()

        # Connect the nodes as a "chain".  This allows us
        # to split the network between nodes 1 and 2 to get
        # two halves that can work on competing chains.

        # If we joined network halves, connect the nodes from the joint
        # on outward.  This ensures that chains are properly reorganised.
        if not split:
            connect_nodes_bi(self.nodes, 1, 2)
            sync_blocks(self.nodes[1:3])
            sync_mempools(self.nodes[1:3])

        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 2, 3)
        self.is_network_split = split
        self.sync_all()

    def split_network(self):
        """
        Split the network of four nodes into nodes 0/1 and 2/3.
        """
        assert not self.is_network_split
        stop_nodes(self.nodes)
        wait_bitcoinds()
        self.setup_network(True)

    def sync_all(self):
        """Synchronizes blocks and mempools"""
        if self.is_network_split:
            sync_blocks(self.nodes[:2])
            sync_blocks(self.nodes[2:])
            sync_mempools(self.nodes[:2])
            sync_mempools(self.nodes[2:])
        else:
            sync_blocks(self.nodes)
            sync_mempools(self.nodes)

    def sync_blocks(self):
        """Synchronizes blocks"""
        if self.is_network_split:
            sync_blocks(self.nodes[:2])
            sync_blocks(self.nodes[2:])
        else:
            sync_blocks(self.nodes)

    def join_network(self):
        """
        Join the (previously split) network halves together.
        """
        assert self.is_network_split
        stop_nodes(self.nodes)
        wait_bitcoinds()
        self.setup_network(False)

    def main(self,argsOverride=None,bitcoinConfDict=None,wallets=None):
        """
        argsOverride: pass your own values for sys.argv in this field (or pass None) to use sys.argv
        bitcoinConfDict:  Pass a dictionary of values you want written to bitcoin.conf.  If you have a key with multiple values, pass a list of the values as the value, for example:
        { "debug":["net","blk","thin","lck","mempool","req","bench","evict"] }
        This framework provides values for the necessary fields (like regtest=1).  But you can override these
        defaults by setting them in this dictionary.

        wallets: Pass a list of wallet filenames.  Each wallet file will be copied into the node's directory
        before starting the node.
        """
        import optparse

        parser = optparse.OptionParser(usage="%prog [options]")
        parser.add_option("--nocleanup", dest="nocleanup", default=False, action="store_true",
                          help="Leave bitcoinds and test.* datadir on exit or error")
        parser.add_option("--noshutdown", dest="noshutdown", default=False, action="store_true",
                          help="Don't stop bitcoinds after the test execution")
        parser.add_option("--srcdir", dest="srcdir", default=os.path.normpath(os.path.dirname(os.path.realpath(__file__))+"/../../../src"),
                          help="Source directory containing bitcoind/bitcoin-cli (default: %default)")
        parser.add_option("--tmpdir", dest="tmpdir", default=tempfile.mkdtemp(prefix="test"),
                          help="Root directory for datadirs")
        parser.add_option("--tracerpc", dest="trace_rpc", default=False, action="store_true",
                          help="Print out all RPC calls as they are made")
        parser.add_option("--portseed", dest="port_seed", default=os.getpid(), type='int',
                          help="The seed to use for assigning port numbers (default: current process id)")
        parser.add_option("--coveragedir", dest="coveragedir",
                          help="Write tested RPC commands into this directory")
        # BU: added for tests using randomness (e.g. excessive.py)
        parser.add_option("--randomseed", dest="randomseed",
                          help="Set RNG seed for tests that use randomness (ignored otherwise)")
        self.add_options(parser)
        (self.options, self.args) = parser.parse_args(argsOverride)

        # BU: initialize RNG seed based on time if no seed specified
        if self.options.randomseed:
            self.randomseed = int(self.options.randomseed)
        else:
            self.randomseed = int(time.time())
        random.seed(self.randomseed)
        print("Random seed: %s" % self.randomseed)

        self.options.tmpdir = os.path.join(self.options.tmpdir, str(self.options.port_seed))

        if self.options.trace_rpc:
            logging.basicConfig(level=logging.DEBUG, stream=sys.stdout)

        if self.options.coveragedir:
            enable_coverage(self.options.coveragedir)

        PortSeed.n = self.options.port_seed

        os.environ['PATH'] = self.options.srcdir + ":" + os.path.join(self.options.srcdir, "qt") + ":" + os.environ['PATH']

        check_json_precision()

        success = False
        try:
            os.makedirs(self.options.tmpdir, exist_ok=False)

            # Not pretty but, I changed the function signature
            # of setup_chain to allow customization of the setup.
            # However derived object may still use the old format
            if self.setup_chain.__defaults__ is None:
              self.setup_chain()
            else:
              self.setup_chain(bitcoinConfDict, wallets)

            self.setup_network()
            self.run_test()
            success = True
        except JSONRPCException as e:
            print("JSONRPC error: "+e.error['message'])
            typ, value, tb = sys.exc_info()
            traceback.print_tb(tb)
            if self.drop_to_pdb: pdb.post_mortem(tb)
        except AssertionError as e:
            print("Assertion failed: " + str(e))
            typ, value, tb = sys.exc_info()
            traceback.print_tb(tb)
            if self.drop_to_pdb: pdb.post_mortem(tb)
        except KeyError as e:
            print("key not found: "+ str(e))
            typ, value, tb = sys.exc_info()
            traceback.print_tb(tb)
            if self.drop_to_pdb: pdb.post_mortem(tb)
        except Exception as e:
            print("Unexpected exception caught during testing: " + repr(e))
            typ, value, tb = sys.exc_info()
            traceback.print_tb(tb)
            if self.drop_to_pdb: pdb.post_mortem(tb)
        except KeyboardInterrupt as e:
            print("Exiting after " + repr(e))

        if not self.options.noshutdown:
            print("Stopping nodes")
            stop_nodes(self.nodes)
            wait_bitcoinds()
        else:
            print("Note: bitcoinds were not stopped and may still be running")

        if not self.options.nocleanup and not self.options.noshutdown and success:
            print("Cleaning up")
            shutil.rmtree(self.options.tmpdir)

        else:
            print("Not cleaning up dir %s" % self.options.tmpdir)
            if os.getenv("PYTHON_DEBUG", ""):
                # Dump the end of the debug logs, to aid in debugging rare
                # travis failures.
                import glob
                filenames = glob.glob(self.options.tmpdir + "/node*/regtest/debug.log")
                MAX_LINES_TO_PRINT = 1000
                for f in filenames:
                    print("From" , f, ":")
                    from collections import deque
                    print("".join(deque(open(f), MAX_LINES_TO_PRINT)))

        if success:
            print("Tests successful")
            sys.exit(0)
        else:
            print("Failed")
            sys.exit(1)


# Test framework for doing p2p comparison testing, which sets up some bitcoind
# binaries:
# 1 binary: test binary
# 2 binaries: 1 test binary, 1 ref binary
# n>2 binaries: 1 test binary, n-1 ref binaries

class ComparisonTestFramework(BitcoinTestFramework):

    # Can override the num_nodes variable to indicate how many nodes to run.
    def __init__(self):
        self.num_nodes = 2

    def add_options(self, parser):
        parser.add_option("--testbinary", dest="testbinary",
                          default=os.getenv("BITCOIND", "bitcoind"),
                          help="bitcoind binary to test")
        parser.add_option("--refbinary", dest="refbinary",
                          default=os.getenv("BITCOIND", "bitcoind"),
                          help="bitcoind binary to use for reference nodes (if any)")

    def setup_chain(self,bitcoinConfDict=None, wallets=None):  # BU add config params
        print("Initializing test directory ", self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, self.num_nodes,bitcoinConfDict, wallets)

    def setup_network(self):
        self.nodes = start_nodes(
            self.num_nodes, self.options.tmpdir,
            extra_args=[['-debug', '-whitelist=127.0.0.1']] * self.num_nodes,
            binary=[self.options.testbinary] +
            [self.options.refbinary]*(self.num_nodes-1))
