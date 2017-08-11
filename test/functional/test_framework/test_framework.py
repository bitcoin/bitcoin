#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Base class for RPC testing."""

from collections import deque
import errno
from enum import Enum
import http.client
import logging
import optparse
import os
import pdb
import shutil
import subprocess
import sys
import tempfile
import time
import traceback

from .authproxy import JSONRPCException
from . import coverage
from .util import (
    MAX_NODES,
    PortSeed,
    assert_equal,
    check_json_precision,
    connect_nodes_bi,
    disconnect_nodes,
    get_rpc_proxy,
    initialize_datadir,
    get_datadir_path,
    log_filename,
    p2p_port,
    rpc_url,
    set_node_times,
    sync_blocks,
    sync_mempools,
)

class TestStatus(Enum):
    PASSED = 1
    FAILED = 2
    SKIPPED = 3

TEST_EXIT_PASSED = 0
TEST_EXIT_FAILED = 1
TEST_EXIT_SKIPPED = 77

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
    def __init__(self):
        self.num_nodes = 4
        self.setup_clean_chain = False
        self.nodes = []
        self.bitcoind_processes = {}
        self.mocktime = 0

    def add_options(self, parser):
        pass

    def setup_chain(self):
        self.log.info("Initializing test directory " + self.options.tmpdir)
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
        self.nodes = self.start_nodes(self.num_nodes, self.options.tmpdir, extra_args)

    def run_test(self):
        raise NotImplementedError

    # Main function. This should not be overridden by the subclass test scripts.

    def main(self):

        parser = optparse.OptionParser(usage="%prog [options]")
        parser.add_option("--nocleanup", dest="nocleanup", default=False, action="store_true",
                          help="Leave bitcoinds and test.* datadir on exit or error")
        parser.add_option("--noshutdown", dest="noshutdown", default=False, action="store_true",
                          help="Don't stop bitcoinds after the test execution")
        parser.add_option("--srcdir", dest="srcdir", default=os.path.normpath(os.path.dirname(os.path.realpath(__file__)) + "/../../../src"),
                          help="Source directory containing bitcoind/bitcoin-cli (default: %default)")
        parser.add_option("--cachedir", dest="cachedir", default=os.path.normpath(os.path.dirname(os.path.realpath(__file__)) + "/../../cache"),
                          help="Directory for caching pregenerated datadirs")
        parser.add_option("--tmpdir", dest="tmpdir", help="Root directory for datadirs")
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
        parser.add_option("--pdbonfailure", dest="pdbonfailure", default=False, action="store_true",
                          help="Attach a python debugger if test fails")
        self.add_options(parser)
        (self.options, self.args) = parser.parse_args()

        PortSeed.n = self.options.port_seed

        os.environ['PATH'] = self.options.srcdir + ":" + self.options.srcdir + "/qt:" + os.environ['PATH']

        check_json_precision()

        # Set up temp directory and start logging
        if self.options.tmpdir:
            os.makedirs(self.options.tmpdir, exist_ok=False)
        else:
            self.options.tmpdir = tempfile.mkdtemp(prefix="test")
        self._start_logging()

        success = TestStatus.FAILED

        try:
            self.setup_chain()
            self.setup_network()
            self.run_test()
            success = TestStatus.PASSED
        except JSONRPCException as e:
            self.log.exception("JSONRPC error")
        except SkipTest as e:
            self.log.warning("Test Skipped: %s" % e.message)
            success = TestStatus.SKIPPED
        except AssertionError as e:
            self.log.exception("Assertion failed")
        except KeyError as e:
            self.log.exception("Key error")
        except Exception as e:
            self.log.exception("Unexpected exception caught during testing")
        except KeyboardInterrupt as e:
            self.log.warning("Exiting after keyboard interrupt")

        if success == TestStatus.FAILED and self.options.pdbonfailure:
            print("Testcase failed. Attaching python debugger. Enter ? for help")
            pdb.set_trace()

        if not self.options.noshutdown:
            self.log.info("Stopping nodes")
            if self.nodes:
                self.stop_nodes()
        else:
            self.log.info("Note: bitcoinds were not stopped and may still be running")

        if not self.options.nocleanup and not self.options.noshutdown and success != TestStatus.FAILED:
            self.log.info("Cleaning up")
            shutil.rmtree(self.options.tmpdir)
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
                            print("From", fn, ":")
                            print("".join(deque(f, MAX_LINES_TO_PRINT)))
                    except OSError:
                        print("Opening file %s failed." % fn)
                        traceback.print_exc()

        if success == TestStatus.PASSED:
            self.log.info("Tests successful")
            sys.exit(TEST_EXIT_PASSED)
        elif success == TestStatus.SKIPPED:
            self.log.info("Test skipped")
            sys.exit(TEST_EXIT_SKIPPED)
        else:
            self.log.error("Test failed. Test logging available at %s/test_framework.log", self.options.tmpdir)
            logging.shutdown()
            sys.exit(TEST_EXIT_FAILED)

    # Public helper methods. These can be accessed by the subclass test scripts.

    def start_node(self, i, dirname, extra_args=None, rpchost=None, timewait=None, binary=None, stderr=None):
        """Start a bitcoind and return RPC connection to it"""

        datadir = os.path.join(dirname, "node" + str(i))
        if binary is None:
            binary = os.getenv("BITCOIND", "bitcoind")
        args = [binary, "-datadir=" + datadir, "-server", "-keypool=1", "-discover=0", "-rest", "-logtimemicros", "-debug", "-debugexclude=libevent", "-debugexclude=leveldb", "-mocktime=" + str(self.mocktime), "-uacomment=testnode%d" % i]
        if extra_args is not None:
            args.extend(extra_args)
        self.bitcoind_processes[i] = subprocess.Popen(args, stderr=stderr)
        self.log.debug("initialize_chain: bitcoind started, waiting for RPC to come up")
        self._wait_for_bitcoind_start(self.bitcoind_processes[i], datadir, i, rpchost)
        self.log.debug("initialize_chain: RPC successfully started")
        proxy = get_rpc_proxy(rpc_url(datadir, i, rpchost), i, timeout=timewait)

        if self.options.coveragedir:
            coverage.write_all_rpc_commands(self.options.coveragedir, proxy)

        return proxy

    def start_nodes(self, num_nodes, dirname, extra_args=None, rpchost=None, timewait=None, binary=None):
        """Start multiple bitcoinds, return RPC connections to them"""

        if extra_args is None:
            extra_args = [None] * num_nodes
        if binary is None:
            binary = [None] * num_nodes
        assert_equal(len(extra_args), num_nodes)
        assert_equal(len(binary), num_nodes)
        rpcs = []
        try:
            for i in range(num_nodes):
                rpcs.append(self.start_node(i, dirname, extra_args[i], rpchost, timewait=timewait, binary=binary[i]))
        except:
            # If one node failed to start, stop the others
            # TODO: abusing self.nodes in this way is a little hacky.
            # Eventually we should do a better job of tracking nodes
            self.nodes.extend(rpcs)
            self.stop_nodes()
            self.nodes = []
            raise
        return rpcs

    def stop_node(self, i):
        """Stop a bitcoind test node"""

        self.log.debug("Stopping node %d" % i)
        try:
            self.nodes[i].stop()
        except http.client.CannotSendRequest as e:
            self.log.exception("Unable to stop node")
        return_code = self.bitcoind_processes[i].wait(timeout=BITCOIND_PROC_WAIT_TIMEOUT)
        del self.bitcoind_processes[i]
        assert_equal(return_code, 0)

    def stop_nodes(self):
        """Stop multiple bitcoind test nodes"""

        for i in range(len(self.nodes)):
            self.stop_node(i)
        assert not self.bitcoind_processes.values()  # All connections must be gone now

    def assert_start_raises_init_error(self, i, dirname, extra_args=None, expected_msg=None):
        with tempfile.SpooledTemporaryFile(max_size=2**16) as log_stderr:
            try:
                self.start_node(i, dirname, extra_args, stderr=log_stderr)
                self.stop_node(i)
            except Exception as e:
                assert 'bitcoind exited' in str(e)  # node must have shutdown
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

    def wait_for_node_exit(self, i, timeout):
        self.bitcoind_processes[i].wait(timeout)

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

    def enable_mocktime(self):
        """Enable mocktime for the script.

        mocktime may be needed for scripts that use the cached version of the
        blockchain.  If the cached version of the blockchain is used without
        mocktime then the mempools will not sync due to IBD.

        For backwared compatibility of the python scripts with previous
        versions of the cache, this helper function sets mocktime to Jan 1,
        2014 + (201 * 10 * 60)"""
        self.mocktime = 1388534400 + (201 * 10 * 60)

    def disable_mocktime(self):
        self.mocktime = 0

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
        formatter = logging.Formatter(fmt='%(asctime)s.%(msecs)03d000 %(name)s (%(levelname)s): %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
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
                self.bitcoind_processes[i] = subprocess.Popen(args)
                self.log.debug("initialize_chain: bitcoind started, waiting for RPC to come up")
                self._wait_for_bitcoind_start(self.bitcoind_processes[i], datadir, i)
                self.log.debug("initialize_chain: RPC successfully started")

            self.nodes = []
            for i in range(MAX_NODES):
                try:
                    self.nodes.append(get_rpc_proxy(rpc_url(get_datadir_path(cachedir, i), i), i))
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
            self.enable_mocktime()
            block_time = self.mocktime - (201 * 10 * 60)
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
            self.disable_mocktime()
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

    def _wait_for_bitcoind_start(self, process, datadir, i, rpchost=None):
        """Wait for bitcoind to start.

        This means that RPC is accessible and fully initialized.
        Raise an exception if bitcoind exits during initialization."""
        while True:
            if process.poll() is not None:
                raise Exception('bitcoind exited with status %i during initialization' % process.returncode)
            try:
                # Check if .cookie file to be created
                rpc = get_rpc_proxy(rpc_url(datadir, i, rpchost), i, coveragedir=self.options.coveragedir)
                rpc.getblockcount()
                break  # break out of loop on success
            except IOError as e:
                if e.errno != errno.ECONNREFUSED:  # Port not yet open?
                    raise  # unknown IO error
            except JSONRPCException as e:  # Initialization phase
                if e.error['code'] != -28:  # RPC in warmup?
                    raise  # unknown JSON RPC exception
            except ValueError as e:  # cookie file not found and no rpcuser or rpcassword. bitcoind still starting
                if "No RPC credentials" not in str(e):
                    raise
            time.sleep(0.25)

class ComparisonTestFramework(BitcoinTestFramework):
    """Test framework for doing p2p comparison testing

    Sets up some bitcoind binaries:
    - 1 binary: test binary
    - 2 binaries: 1 test binary, 1 ref binary
    - n>2 binaries: 1 test binary, n-1 ref binaries"""

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
        extra_args = [['-whitelist=127.0.0.1']]*self.num_nodes
        if hasattr(self, "extra_args"):
            extra_args = self.extra_args
        self.nodes = self.start_nodes(
            self.num_nodes, self.options.tmpdir, extra_args,
            binary=[self.options.testbinary] +
            [self.options.refbinary] * (self.num_nodes - 1))

class SkipTest(Exception):
    """This exception is raised to skip a test"""
    def __init__(self, message):
        self.message = message
