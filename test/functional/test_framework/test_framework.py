#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Base class for RPC testing."""

import errno
import http.client
import logging
import optparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

from . import coverage
from .util import (
    MAX_NODES,
    PortSeed,
    assert_equal,
    check_json_precision,
    connect_nodes_bi,
    disable_mocktime,
    enable_mocktime,
    get_mocktime,
    get_rpc_proxy,
    initialize_datadir,
    log_filename,
    p2p_port,
    rpc_url,
    set_node_times,
    sync_blocks,
    sync_mempools,
)
from .authproxy import JSONRPCException
from .mininode import NodeConn, SingleNodeConnCB

BITCOIND_PROC_WAIT_TIMEOUT = 60

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
        self.nodes = []
        self.p2p_conn_type = SingleNodeConnCB

    def add_options(self, parser):
        pass

    def setup_chain(self):
        self.log.info("Initializing test directory "+self.options.tmpdir)
        if self.setup_clean_chain:
            self._initialize_chain_clean(self.options.tmpdir, self.num_nodes)
        else:
            self._initialize_chain(self.options.tmpdir, self.num_nodes, self.options.cachedir)

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

    def setup_nodes(self):
        return self.start_nodes()

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
        parser.add_option("--coveragedir", dest="coveragedir", default=None,
                          help="Write tested RPC commands into this directory")
        self.add_options(parser)
        (self.options, self.args) = parser.parse_args()

        # backup dir variable for removal at cleanup
        self.options.root, self.options.tmpdir = self.options.tmpdir, self.options.tmpdir + '/' + str(self.options.port_seed)

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
                filenames = glob.glob(self.options.tmpdir + "/node*/regtest/debug.log")
                MAX_LINES_TO_PRINT = 1000
                for f in filenames:
                    print("From" , f, ":")
                    from collections import deque
                    print("".join(deque(open(f), MAX_LINES_TO_PRINT)))
        if success:
            self.log.info("Tests successful")
            sys.exit(self.TEST_EXIT_PASSED)
        else:
            self.log.error("Test failed. Test logging available at %s/test_framework.log", self.options.tmpdir)
            logging.shutdown()
            sys.exit(self.TEST_EXIT_FAILED)

    # Public helper methods. These can be accessed by the subclass test scripts.

    def start_nodes(self, num_nodes=None, dirname=None, extra_args=None, rpchost=None, timewait=None, binary=None):
        """Start multiple bitcoinds, return RPC connections to them."""
        if num_nodes is None:
            num_nodes = self.num_nodes
        if dirname is None:
            dirname = self.options.tmpdir
        if extra_args is None:
            extra_args = [[] for _ in range(num_nodes)]
        if binary is None:
            binary = [None for _ in range(num_nodes)]
        nodes = []
        try:
            for i in range(num_nodes):
                nodes.append(TestNode(i, dirname, extra_args[i], rpchost, timewait=timewait, binary=binary[i], stderr=None, coverage_dir=self.options.coveragedir))
                nodes[i].p2p_conn_type = self.p2p_conn_type
                nodes[i].start()
            for node in nodes:
                while not node.is_rpc_connected():
                    time.sleep(0.1)
        except:  # If one node failed to start, stop the others
            self.stop_nodes()
            raise

        if self.options.coveragedir is not None:
            coverage.write_all_rpc_commands(self.options.coveragedir, node.rpc)

        return nodes

    def start_node(self, i, dirname=None, extra_args=[], rpchost=None, timewait=None, binary=None, stderr=None):
        """Start a bitcoind and return RPC connection to it."""
        if dirname is None:
            dirname = self.options.tmpdir
        datadir = os.path.join(dirname, "node" + str(i))
        if binary is None:
            binary = os.getenv("BITCOIND", "bitcoind")
        node = TestNode(i, dirname, extra_args, rpchost, timewait, binary, stderr, coverage_dir=self.options.coveragedir)
        node.p2p_conn_type = self.p2p_conn_type
        node.start()
        while not node.is_rpc_connected():
            time.sleep(0.1)

        if self.options.coveragedir is not None:
            coverage.write_all_rpc_commands(self.options.coveragedir, node.rpc)

        return node

    def assert_start_raises_init_error(self, i, dirname, extra_args=None, expected_msg=None):
        with tempfile.SpooledTemporaryFile(max_size=2**16) as log_stderr:
            try:
                node = self.start_node(i, dirname, extra_args, stderr=log_stderr)
                self.stop_node(i)
            except Exception as e:
                assert 'bitcoind exited' in str(e)  # node must have shutdown
                self.nodes[i].running = False
                self.nodes[i].process = None
                if expected_msg is not None:
                    log_stderr.seek(0)
                    stderr = log_stderr.read().decode('utf-8')
                    if expected_msg not in stderr:
                        raise AssertionError("Expected error \"" + expected_msg + "\" not found in:\n" + stderr)
            else:
                if expected_msg is None:
                    assert_msg = "bitcoind should have exited with an error"
                else:
                    assert_msg = "bitcoind should have exited with expected error " + expected_msg
                raise AssertionError(assert_msg)

    def stop_nodes(self):
        [node.stop_node() for node in self.nodes]
        # All connections must be gone now
        for node in self.nodes:
            while not node.is_node_stopped():
                time.sleep(0.1)

    def stop_node(self, i):
        self.nodes[i].stop_node()
        while not self.nodes[i].is_node_stopped():
            time.sleep(0.1)

    def split_network(self):
        """
        Split the network of four nodes into nodes 0/1 and 2/3.
        """
        assert not self.is_network_split
        self.stop_nodes()
        self.setup_network(True)

    def join_network(self):
        """
        Join the (previously split) network halves together.
        """
        assert self.is_network_split
        self.stop_nodes()
        self.setup_network(False)

    def sync_all(self):
        if self.is_network_split:
            sync_blocks(self.nodes[:2])
            sync_blocks(self.nodes[2:])
            sync_mempools(self.nodes[:2])
            sync_mempools(self.nodes[2:])
        else:
            sync_blocks(self.nodes)
            sync_mempools(self.nodes)

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
                self.nodes.append(TestNode(i, datadir, extra_args=[], rpchost=None, timewait=None, binary=None, stderr=None, coverage_dir=None))
                self.nodes[i].args = args
                self.nodes[i].start()

            # Wait for RPC connections to be ready
            for node in self.nodes:
                while not node.is_rpc_connected():
                    time.sleep(0.1)

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

class TestNode():
    """A class for representing a bitcoind node under test.

    This class contains:

    - state about the node (whether it's running, etc)
    - a Python subprocess.Popen object representing the running process
    - an RPC connection to the node
    - one or more P2P connections to the node

    To make things easier for the test writer, a bit of magic is happening under the covers.
    send_message is dispatched to the first P2P connection. If there hasn't been a P2P connection
    before, this class will automatically open a P2P connection to send the message. If there has
    been a P2P connection open but it has subsequently closed, then the class won't automatically
    open a new P2P connection. The test writer must explicitly re-open a connection.
    Any unrecognised messages will be dispatched to the RPC connection."""
    def __init__(self, i, dirname, extra_args, rpchost, timewait, binary, stderr, coverage_dir):
        self.index = i
        self.datadir = os.path.join(dirname, "node" + str(i))
        self.rpchost = rpchost
        self.rpc_timeout = timewait
        if binary is None:
            self.binary = os.getenv("BITCOIND", "bitcoind")
        else:
            self.binary = binary
        self.stderr = stderr
        self.coverage_dir = coverage_dir
        # Most callers will just need to add extra args to the standard list below. For those callers that need more flexibity, they can just set the args property directly.
        self.extra_args = extra_args
        self.args = [self.binary, "-datadir=" + self.datadir, "-server", "-keypool=1", "-discover=0", "-rest", "-logtimemicros", "-debug", "-mocktime=" + str(get_mocktime())]

        self.running = False
        self.process = None
        self.rpc_connected = False
        self.rpc = None
        self.url = None
        self.log = logging.getLogger('TestFramework.node%d' % i)
        self.p2p = None
        self.p2ps = []
        self.p2p_connected = False
        self.p2p_ever_connected = False
        self.p2p_conn_type = SingleNodeConnCB

    def __getattr__(self, name):
        """Dispatches any unrecognised messages to the RPC connection."""
        assert self.rpc_connected and not self.rpc is None, "Error: no RPC connection"
        return self.rpc.__getattr__(name)

    def start(self):
        """Start the node."""
        self.process = subprocess.Popen(self.args + self.extra_args, stderr=self.stderr)
        self.running = True
        self.log.debug("bitcoind started, waiting for RPC to come up")

    def is_rpc_connected(self):
        """Sets up an RPC connection to the bitcoind process. Returns False if unable to connect."""
        url = rpc_url(self.index, self.rpchost)

        assert not self.process.poll(), "bitcoind exited with status %i during initialization" % self.process.returncode

        try:
            self.rpc = get_rpc_proxy(url, self.index, self.rpc_timeout, self.coverage_dir)
            self.rpc.getblockcount()
            # If the call to getblockcount() succeeds then the RPC connection is up
            self.rpc_connected = True
            self.url = self.rpc.url
            self.log.debug("RPC successfully started")
            return True
        except IOError as e:
            assert e.errno == errno.ECONNREFUSED, "RPC connection returned unexpected error: %d" % e.errno
        except JSONRPCException as e:  # Initialization phase
            assert e.error['code'] == -28, "RPC connection returned unknown JSON RPC error: %d" % e.error['code']
        # RPC connection not yet up. We received either a CONNREFUSED error or a JSON error -28 (RPC_IN_WARMUP). Return None.
        return False

    def stop_node(self):
        """Stop the node."""
        if not self.running:
            return
        self.log.debug("Stopping node")
        try:
            self.stop()
        except http.client.CannotSendRequest as e:
            self.log.exception("Unable to stop node.")

    def is_node_stopped(self):
        """Checks whether the node has stopped.

        Returns True if the node has stopped. False otherwise.
        This method is responsible for freeing resources (self.process)."""
        if not self.running:
            return True
        return_code = self.process.poll()
        if return_code is not None:
            # process has stopped. Assert that it didn't return an error code.
            assert_equal(return_code, 0)
            self.running = False
            self.process = None
            self.log.debug("Node stopped")
            return True
        return False

    def node_encrypt_wallet(self, passphrase):
        """"Encrypts the wallet.

        This causes bitcoind to shutdown, so this method takes
        care of cleaning up resources."""
        self.encryptwallet(passphrase)
        while not self.is_node_stopped():
            time.sleep(0.1)
        self.rpc = None
        self.rpc_connected = False

    def add_p2p_connections(self, number=1, dstaddr='127.0.0.1', dstport=None, p2p_conn_type=None):
        if dstport is None:
            dstport = p2p_port(self.index)
        if p2p_conn_type is None:
            p2p_conn_type = self.p2p_conn_type
        for i in range(number):
            p2p_conn = p2p_conn_type()
            self.p2ps.append(p2p_conn)
            p2p_conn.add_connection(NodeConn(dstaddr, dstport, self.rpc, p2p_conn))
        if self.p2p is None:
            self.p2p = self.p2ps[0]
        while p2p_conn.connection.state != "connected":
            time.sleep(0.1)
        p2p_conn.wait_for_verack()
        self.p2p_connected = True
        self.p2p_ever_connected = True

    def send_message(self, message):
        if not self.p2p_ever_connected:
            self.add_p2p_connections()
        assert self.p2ps != [], "No p2p connection"
        self.p2p.send_message(message)

    def disconnect_p2p(self, index=0):
        self.p2ps[index].connection.disconnect_node()
        self.p2ps.pop(index)
        if len(self.p2ps) == 0:
            self.p2p_connected = False

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
        self.nodes = self.start_nodes(extra_args=[['-whitelist=127.0.0.1']] * self.num_nodes,
                                      binary=[self.options.testbinary] + [self.options.refbinary]*(self.num_nodes-1))
