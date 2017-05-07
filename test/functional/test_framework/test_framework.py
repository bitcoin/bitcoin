#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Base class for RPC testing."""

from collections import deque
import logging
import optparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

from .util import (
    PortSeed,
    MAX_NODES,
    bitcoind_processes,
    check_json_precision,
    connect_nodes_bi,
    disable_mocktime,
    disconnect_nodes,
    enable_coverage,
    enable_mocktime,
    get_mocktime,
    get_rpc_proxy,
    initialize_datadir,
    log_filename,
    p2p_port,
    rpc_url,
    set_node_times,
    start_node,
    start_nodes,
    stop_node,
    stop_nodes,
    sync_blocks,
    sync_mempools,
    wait_for_bitcoind_start,
)
from .authproxy import JSONRPCException

class BitcoinTestFramework(object):
    """Base class for a bitcoin test script.

    Individual bitcoin test scripts should subclass this class and override the following methods:

    - __init__()
    - add_options()
    - setup_chain()
    - setup_network()
    - run_test()

    The main() method should not be overridden.

    This class also contains various public and private helper methods."""

    # Methods to override in subclass test scripts.

    TEST_EXIT_PASSED = 0
    TEST_EXIT_FAILED = 1
    TEST_EXIT_SKIPPED = 77

    def __init__(self):
        self.num_nodes = 4
        self.setup_clean_chain = False
        self.nodes = None

    def add_options(self, parser):
        pass

    def setup_chain(self):
        self.log.info("Initializing test directory "+self.options.tmpdir)
        if self.setup_clean_chain:
            self._initialize_chain_clean(self.options.tmpdir, self.num_nodes)
        else:
            self._initialize_chain(self.options.tmpdir, self.num_nodes, self.options.cachedir)

    def setup_network(self):
        self.setup_nodes()

        # Connect the nodes as a "chain".  This allows us
        # to split the network between nodes 1 and 2 to get
        # two halves that can work on competing chains.
        for i in range(self.num_nodes - 1):
            connect_nodes_bi(self.nodes, i, i + 1)
        self.sync_all()

    def setup_nodes(self):
        extra_args = None
        if hasattr(self, "extra_args"):
            extra_args = self.extra_args
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, extra_args)

    def run_test(self):
        raise NotImplementedError

    # Main function. This should not be overridden by the subclass test scripts.

    def main(self):

        parser = optparse.OptionParser(usage="%prog [options]")
        parser.add_option("--nocleanup", dest="nocleanup", default=False, action="store_true",
                          help="Leave bitcoinds and test.* datadir on exit or error")
        parser.add_option("--noshutdown", dest="noshutdown", default=False, action="store_true",
                          help="Don't stop bitcoinds after the test execution")
        parser.add_option("--srcdir", dest="srcdir", default=os.path.normpath(os.path.dirname(os.path.realpath(__file__))+"/../../../src"),
                          help="Source directory containing bitcoind/bitcoin-cli (default: %default)")
        parser.add_option("--cachedir", dest="cachedir", default=os.path.normpath(os.path.dirname(os.path.realpath(__file__))+"/../../cache"),
                          help="Directory for caching pregenerated datadirs")
        parser.add_option("--tmpdir", dest="tmpdir", default=tempfile.mkdtemp(prefix="test"),
                          help="Root directory for datadirs")
        parser.add_option("-l", "--loglevel", dest="loglevel", default="INFO",
                          help="log events at this level and higher to the console. Can be set to DEBUG, INFO, WARNING, ERROR or CRITICAL. Passing --loglevel DEBUG will output all logs to console. Note that logs at all levels are always written to the test_framework.log file in the temporary test directory.")
        parser.add_option("--tracerpc", dest="trace_rpc", default=False, action="store_true",
                          help="Print out all RPC calls as they are made")
        parser.add_option("--portseed", dest="port_seed", default=os.getpid(), type='int',
                          help="The seed to use for assigning port numbers (default: current process id)")
        parser.add_option("--coveragedir", dest="coveragedir",
                          help="Write tested RPC commands into this directory")
        parser.add_option("--configfile", dest="configfile",
                          help="Location of the test framework config file")
        self.add_options(parser)
        (self.options, self.args) = parser.parse_args()

        # backup dir variable for removal at cleanup
        self.options.root, self.options.tmpdir = self.options.tmpdir, self.options.tmpdir + '/' + str(self.options.port_seed)

        if self.options.coveragedir:
            enable_coverage(self.options.coveragedir)

        PortSeed.n = self.options.port_seed

        os.environ['PATH'] = self.options.srcdir+":"+self.options.srcdir+"/qt:"+os.environ['PATH']

        check_json_precision()

        # Set up temp directory and start logging
        os.makedirs(self.options.tmpdir, exist_ok=False)
        self._start_logging()

        success = False

        try:
            self.setup_chain()
            self.setup_network()
            self.run_test()
            success = True
        except JSONRPCException as e:
            self.log.exception("JSONRPC error")
        except AssertionError as e:
            self.log.exception("Assertion failed")
        except KeyError as e:
            self.log.exception("Key error")
        except Exception as e:
            self.log.exception("Unexpected exception caught during testing")
        except KeyboardInterrupt as e:
            self.log.warning("Exiting after keyboard interrupt")

        if not self.options.noshutdown:
            self.log.info("Stopping nodes")
            self.stop_nodes()
        else:
            self.log.info("Note: bitcoinds were not stopped and may still be running")

        if not self.options.nocleanup and not self.options.noshutdown and success:
            self.log.info("Cleaning up")
            shutil.rmtree(self.options.tmpdir)
            if not os.listdir(self.options.root):
                os.rmdir(self.options.root)
        else:
            self.log.warning("Not cleaning up dir %s" % self.options.tmpdir)
            if os.getenv("PYTHON_DEBUG", ""):
                # Dump the end of the debug logs, to aid in debugging rare
                # travis failures.
                import glob
                filenames = [self.options.tmpdir + "/test_framework.log"]
                filenames += glob.glob(self.options.tmpdir + "/node*/regtest/debug.log")
                MAX_LINES_TO_PRINT = 1000
                for fn in filenames:
                    try:
                        with open(fn, 'r') as f:
                            print("From" , fn, ":")
                            print("".join(deque(f, MAX_LINES_TO_PRINT)))
                    except OSError:
                        print("Opening file %s failed." % fn)
                        traceback.print_exc()
        if success:
            self.log.info("Tests successful")
            sys.exit(self.TEST_EXIT_PASSED)
        else:
            self.log.error("Test failed. Test logging available at %s/test_framework.log", self.options.tmpdir)
            logging.shutdown()
            sys.exit(self.TEST_EXIT_FAILED)

    # Public helper methods. These can be accessed by the subclass test scripts.

    def start_node(self, i, dirname, extra_args=None, rpchost=None, timewait=None, binary=None, stderr=None):
        return start_node(i, dirname, extra_args, rpchost, timewait, binary, stderr)

    def start_nodes(self, num_nodes, dirname, extra_args=None, rpchost=None, timewait=None, binary=None):
        return start_nodes(num_nodes, dirname, extra_args, rpchost, timewait, binary)

    def stop_node(self, num_node):
        stop_node(self.nodes[num_node], num_node)

    def stop_nodes(self):
        stop_nodes(self.nodes)

    def split_network(self):
        """
        Split the network of four nodes into nodes 0/1 and 2/3.
        """
        disconnect_nodes(self.nodes[1], 2)
        disconnect_nodes(self.nodes[2], 1)
        self.sync_all([self.nodes[:2], self.nodes[2:]])

    def join_network(self):
        """
        Join the (previously split) network halves together.
        """
        connect_nodes_bi(self.nodes, 1, 2)
        self.sync_all()

    def sync_all(self, node_groups=None):
        if not node_groups:
            node_groups = [self.nodes]

        for group in node_groups:
            sync_blocks(group)
            sync_mempools(group)

    # Private helper methods. These should not be accessed by the subclass test scripts.

    def _start_logging(self):
        # Add logger and logging handlers
        self.log = logging.getLogger('TestFramework')
        self.log.setLevel(logging.DEBUG)
        # Create file handler to log all messages
        fh = logging.FileHandler(self.options.tmpdir + '/test_framework.log')
        fh.setLevel(logging.DEBUG)
        # Create console handler to log messages to stderr. By default this logs only error messages, but can be configured with --loglevel.
        ch = logging.StreamHandler(sys.stdout)
        # User can provide log level as a number or string (eg DEBUG). loglevel was caught as a string, so try to convert it to an int
        ll = int(self.options.loglevel) if self.options.loglevel.isdigit() else self.options.loglevel.upper()
        ch.setLevel(ll)
        # Format logs the same as bitcoind's debug.log with microprecision (so log files can be concatenated and sorted)
        formatter = logging.Formatter(fmt = '%(asctime)s.%(msecs)03d000 %(name)s (%(levelname)s): %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
        formatter.converter = time.gmtime
        fh.setFormatter(formatter)
        ch.setFormatter(formatter)
        # add the handlers to the logger
        self.log.addHandler(fh)
        self.log.addHandler(ch)

        if self.options.trace_rpc:
            rpc_logger = logging.getLogger("BitcoinRPC")
            rpc_logger.setLevel(logging.DEBUG)
            rpc_handler = logging.StreamHandler(sys.stdout)
            rpc_handler.setLevel(logging.DEBUG)
            rpc_logger.addHandler(rpc_handler)

    def _initialize_chain(self, test_dir, num_nodes, cachedir):
        """Initialize a pre-mined blockchain for use by the test.

        Create a cache of a 200-block-long chain (with wallet) for MAX_NODES
        Afterward, create num_nodes copies from the cache."""

        assert num_nodes <= MAX_NODES
        create_cache = False
        for i in range(MAX_NODES):
            if not os.path.isdir(os.path.join(cachedir, 'node' + str(i))):
                create_cache = True
                break

        if create_cache:
            self.log.debug("Creating data directories from cached datadir")

            # find and delete old cache directories if any exist
            for i in range(MAX_NODES):
                if os.path.isdir(os.path.join(cachedir, "node" + str(i))):
                    shutil.rmtree(os.path.join(cachedir, "node" + str(i)))

            # Create cache directories, run bitcoinds:
            for i in range(MAX_NODES):
                datadir = initialize_datadir(cachedir, i)
                args = [os.getenv("BITCOIND", "bitcoind"), "-server", "-keypool=1", "-datadir=" + datadir, "-discover=0"]
                if i > 0:
                    args.append("-connect=127.0.0.1:" + str(p2p_port(0)))
                bitcoind_processes[i] = subprocess.Popen(args)
                self.log.debug("initialize_chain: bitcoind started, waiting for RPC to come up")
                wait_for_bitcoind_start(bitcoind_processes[i], rpc_url(i), i)
                self.log.debug("initialize_chain: RPC successfully started")

            self.nodes = []
            for i in range(MAX_NODES):
                try:
                    self.nodes.append(get_rpc_proxy(rpc_url(i), i))
                except:
                    self.log.exception("Error connecting to node %d" % i)
                    sys.exit(1)

            # Create a 200-block-long chain; each of the 4 first nodes
            # gets 25 mature blocks and 25 immature.
            # Note: To preserve compatibility with older versions of
            # initialize_chain, only 4 nodes will generate coins.
            #
            # blocks are created with timestamps 10 minutes apart
            # starting from 2010 minutes in the past
            enable_mocktime()
            block_time = get_mocktime() - (201 * 10 * 60)
            for i in range(2):
                for peer in range(4):
                    for j in range(25):
                        set_node_times(self.nodes, block_time)
                        self.nodes[peer].generate(1)
                        block_time += 10 * 60
                    # Must sync before next peer starts generating blocks
                    sync_blocks(self.nodes)

            # Shut them down, and clean up cache directories:
            self.stop_nodes()
            self.nodes = []
            disable_mocktime()
            for i in range(MAX_NODES):
                os.remove(log_filename(cachedir, i, "debug.log"))
                os.remove(log_filename(cachedir, i, "db.log"))
                os.remove(log_filename(cachedir, i, "peers.dat"))
                os.remove(log_filename(cachedir, i, "fee_estimates.dat"))

        for i in range(num_nodes):
            from_dir = os.path.join(cachedir, "node" + str(i))
            to_dir = os.path.join(test_dir, "node" + str(i))
            shutil.copytree(from_dir, to_dir)
            initialize_datadir(test_dir, i)  # Overwrite port/rpcport in bitcoin.conf

    def _initialize_chain_clean(self, test_dir, num_nodes):
        """Initialize empty blockchain for use by the test.

        Create an empty blockchain and num_nodes wallets.
        Useful if a test case wants complete control over initialization."""
        for i in range(num_nodes):
            initialize_datadir(test_dir, i)

# Test framework for doing p2p comparison testing, which sets up some bitcoind
# binaries:
# 1 binary: test binary
# 2 binaries: 1 test binary, 1 ref binary
# n>2 binaries: 1 test binary, n-1 ref binaries

class ComparisonTestFramework(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = True

    def add_options(self, parser):
        parser.add_option("--testbinary", dest="testbinary",
                          default=os.getenv("BITCOIND", "bitcoind"),
                          help="bitcoind binary to test")
        parser.add_option("--refbinary", dest="refbinary",
                          default=os.getenv("BITCOIND", "bitcoind"),
                          help="bitcoind binary to use for reference nodes (if any)")

    def setup_network(self):
        self.nodes = self.start_nodes(
            self.num_nodes, self.options.tmpdir,
            extra_args=[['-whitelist=127.0.0.1']] * self.num_nodes,
            binary=[self.options.testbinary] +
            [self.options.refbinary]*(self.num_nodes-1))
