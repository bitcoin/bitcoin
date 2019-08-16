#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Copyright (c) 2014-2019 The Dash Core developers
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
from concurrent.futures import ThreadPoolExecutor

from .authproxy import JSONRPCException
from . import coverage
from .util import (
    PortSeed,
    MAX_NODES,
    assert_equal,
    check_json_precision,
    connect_nodes_bi,
    connect_nodes,
    copy_datadir,
    disconnect_nodes,
    get_rpc_proxy,
    initialize_datadir,
    get_datadir_path,
    log_filename,
    p2p_port,
    rpc_url,
    set_node_times,
    satoshi_round,
    sync_blocks,
    sync_mempools,
    sync_masternodes,
    wait_to_sync)

class TestStatus(Enum):
    PASSED = 1
    FAILED = 2
    SKIPPED = 3

TEST_EXIT_PASSED = 0
TEST_EXIT_FAILED = 1
TEST_EXIT_SKIPPED = 77

BITCOIND_PROC_WAIT_TIMEOUT = 60

GENESISTIME = 1417713337

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
            self.set_genesis_mocktime()
        else:
            self._initialize_chain(self.options.tmpdir, self.num_nodes, self.options.cachedir)
            self.set_cache_mocktime()

    def setup_network(self):
        self.setup_nodes()

        # Connect the nodes as a "chain".  This allows us
        # to split the network between nodes 1 and 2 to get
        # two halves that can work on competing chains.
        for i in range(self.num_nodes - 1):
            connect_nodes_bi(self.nodes, i, i + 1)
        self.sync_all()

    def setup_nodes(self, stderr=None):
        extra_args = None
        if hasattr(self, "extra_args"):
            extra_args = self.extra_args
        self.nodes = self.start_nodes(self.num_nodes, self.options.tmpdir, extra_args, stderr=stderr)

    def run_test(self):
        raise NotImplementedError

    # Main function. This should not be overridden by the subclass test scripts.

    def main(self):

        parser = optparse.OptionParser(usage="%prog [options]")
        parser.add_option("--nocleanup", dest="nocleanup", default=False, action="store_true",
                          help="Leave dashds and test.* datadir on exit or error")
        parser.add_option("--noshutdown", dest="noshutdown", default=False, action="store_true",
                          help="Don't stop dashds after the test execution")
        parser.add_option("--srcdir", dest="srcdir", default=os.path.normpath(os.path.dirname(os.path.realpath(__file__)) + "/../../../src"),
                          help="Source directory containing dashd/dash-cli (default: %default)")
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
            try:
                if self.nodes:
                    self.stop_nodes()
            except BaseException as e:
                success = False
                self.log.exception("Unexpected exception caught during shutdown")
        else:
            self.log.info("Note: dashds were not stopped and may still be running")

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
        """Start a dashd and return RPC connection to it"""

        datadir = os.path.join(dirname, "node" + str(i))
        if binary is None:
            binary = os.getenv("BITCOIND", "dashd")
        args = [binary, "-datadir=" + datadir, "-server", "-keypool=1", "-discover=0", "-rest", "-logtimemicros", "-debug", "-debugexclude=libevent", "-debugexclude=leveldb", "-mocktime=" + str(self.mocktime), "-uacomment=testnode%d" % i]
        # Don't try auto backups (they fail a lot when running tests)
        args += [ "-createwalletbackups=0" ]
        if extra_args is not None:
            args.extend(extra_args)
        self.bitcoind_processes[i] = subprocess.Popen(args, stderr=stderr)
        self.log.debug("initialize_chain: dashd started, waiting for RPC to come up")
        self._wait_for_bitcoind_start(self.bitcoind_processes[i], datadir, i, rpchost)
        self.log.debug("initialize_chain: RPC successfully started")
        proxy = get_rpc_proxy(rpc_url(datadir, i, rpchost), i, timeout=timewait)

        if self.options.coveragedir:
            coverage.write_all_rpc_commands(self.options.coveragedir, proxy)

        return proxy

    def start_nodes(self, num_nodes, dirname, extra_args=None, rpchost=None, timewait=None, binary=None, stderr=None):
        """Start multiple dashds, return RPC connections to them"""

        if extra_args is None:
            extra_args = [None] * num_nodes
        if binary is None:
            binary = [None] * num_nodes
        assert_equal(len(extra_args), num_nodes)
        assert_equal(len(binary), num_nodes)
        rpcs = []
        try:
            for i in range(num_nodes):
                rpcs.append(self.start_node(i, dirname, extra_args[i], rpchost, timewait=timewait, binary=binary[i], stderr=stderr))
        except:
            # If one node failed to start, stop the others
            # TODO: abusing self.nodes in this way is a little hacky.
            # Eventually we should do a better job of tracking nodes
            self.nodes.extend(rpcs)
            self.stop_nodes()
            self.nodes = []
            raise
        return rpcs

    def stop_node(self, i, wait=True):
        """Stop a dashd test node"""

        self.log.debug("Stopping node %d" % i)
        try:
            self.nodes[i].stop()
        except http.client.CannotSendRequest as e:
            self.log.exception("Unable to stop node")
        if wait:
            self.wait_node(i)

    def stop_nodes(self, fast=True):
        """Stop multiple dashd test nodes"""

        for i in range(len(self.nodes)):
            self.stop_node(i, not fast)
        if fast:
            for i in range(len(self.nodes)):
                self.wait_node(i)
        assert not self.bitcoind_processes.values()  # All connections must be gone now

    def assert_start_raises_init_error(self, i, dirname, extra_args=None, expected_msg=None):
        with tempfile.SpooledTemporaryFile(max_size=2**16) as log_stderr:
            try:
                self.start_node(i, dirname, extra_args, stderr=log_stderr)
                self.stop_node(i)
            except Exception as e:
                assert 'dashd exited' in str(e)  # node must have shutdown
                if expected_msg is not None:
                    log_stderr.seek(0)
                    stderr = log_stderr.read().decode('utf-8')
                    if expected_msg not in stderr:
                        raise AssertionError("Expected error \"" + expected_msg + "\" not found in:\n" + stderr)
            else:
                if expected_msg is None:
                    assert_msg = "dashd should have exited with an error"
                else:
                    assert_msg = "dashd should have exited with expected error " + expected_msg
                raise AssertionError(assert_msg)

    def wait_node(self, i):
        return_code = self.bitcoind_processes[i].wait(timeout=BITCOIND_PROC_WAIT_TIMEOUT)
        del self.bitcoind_processes[i]
        assert_equal(return_code, 0)

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

    def disable_mocktime(self):
        self.mocktime = 0

    def bump_mocktime(self, t):
        self.mocktime += t

    def set_cache_mocktime(self):
        # For backwared compatibility of the python scripts
        # with previous versions of the cache, set MOCKTIME
        # to regtest genesis time + (201 * 156)
        self.mocktime = GENESISTIME + (201 * 156)

    def set_genesis_mocktime(self):
        self.mocktime = GENESISTIME

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
        # Format logs the same as dashd's debug.log with microprecision (so log files can be concatenated and sorted)
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

    def _initialize_chain(self, test_dir, num_nodes, cachedir, extra_args=None, stderr=None):
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

            # Create cache directories, run dashds:
            self.set_genesis_mocktime()
            for i in range(MAX_NODES):
                datadir = initialize_datadir(cachedir, i)
                args = [os.getenv("DASHD", "dashd"), "-server", "-keypool=1", "-datadir=" + datadir, "-discover=0", "-mocktime="+str(GENESISTIME)]
                if i > 0:
                    args.append("-connect=127.0.0.1:" + str(p2p_port(0)))
                if extra_args is not None:
                    args.extend(extra_args)
                self.bitcoind_processes[i] = subprocess.Popen(args, stderr=stderr)
                self.log.debug("initialize_chain: dashd started, waiting for RPC to come up")
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
            block_time = GENESISTIME
            for i in range(2):
                for peer in range(4):
                    for j in range(25):
                        set_node_times(self.nodes, block_time)
                        self.nodes[peer].generate(1)
                        block_time += 156
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
            initialize_datadir(test_dir, i)  # Overwrite port/rpcport in dsah.conf

    def _initialize_chain_clean(self, test_dir, num_nodes):
        """Initialize empty blockchain for use by the test.

        Create an empty blockchain and num_nodes wallets.
        Useful if a test case wants complete control over initialization."""
        for i in range(num_nodes):
            initialize_datadir(test_dir, i)

    def _wait_for_bitcoind_start(self, process, datadir, i, rpchost=None):
        """Wait for dashd to start.

        This means that RPC is accessible and fully initialized.
        Raise an exception if dashd exits during initialization."""
        while True:
            if process.poll() is not None:
                raise Exception('dashd exited with status %i during initialization' % process.returncode)
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
            except ValueError as e:  # cookie file not found and no rpcuser or rpcassword. dashd still starting
                if "No RPC credentials" not in str(e):
                    raise
            time.sleep(0.25)

MASTERNODE_COLLATERAL = 1000


class MasternodeInfo:
    def __init__(self, proTxHash, ownerAddr, votingAddr, pubKeyOperator, keyOperator, collateral_address, collateral_txid, collateral_vout):
        self.proTxHash = proTxHash
        self.ownerAddr = ownerAddr
        self.votingAddr = votingAddr
        self.pubKeyOperator = pubKeyOperator
        self.keyOperator = keyOperator
        self.collateral_address = collateral_address
        self.collateral_txid = collateral_txid
        self.collateral_vout = collateral_vout


class DashTestFramework(BitcoinTestFramework):
    def __init__(self, num_nodes, masterodes_count, extra_args, fast_dip3_enforcement=False):
        super().__init__()
        self.mn_count = masterodes_count
        self.num_nodes = num_nodes
        self.mninfo = []
        self.setup_clean_chain = True
        self.is_network_split = False
        # additional args
        self.extra_args = extra_args

        self.extra_args += ["-sporkkey=cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK"]

        self.fast_dip3_enforcement = fast_dip3_enforcement
        if fast_dip3_enforcement:
            self.extra_args += ["-dip3params=30:50"]

    def create_simple_node(self):
        idx = len(self.nodes)
        args = self.extra_args
        self.nodes.append(self.start_node(idx, self.options.tmpdir, args))
        for i in range(0, idx):
            connect_nodes(self.nodes[i], idx)

    def prepare_masternodes(self):
        for idx in range(0, self.mn_count):
            self.prepare_masternode(idx)

    def prepare_masternode(self, idx):
        bls = self.nodes[0].bls('generate')
        address = self.nodes[0].getnewaddress()
        txid = self.nodes[0].sendtoaddress(address, MASTERNODE_COLLATERAL)

        txraw = self.nodes[0].getrawtransaction(txid, True)
        collateral_vout = 0
        for vout_idx in range(0, len(txraw["vout"])):
            vout = txraw["vout"][vout_idx]
            if vout["value"] == MASTERNODE_COLLATERAL:
                collateral_vout = vout_idx
        self.nodes[0].lockunspent(False, [{'txid': txid, 'vout': collateral_vout}])

        # send to same address to reserve some funds for fees
        self.nodes[0].sendtoaddress(address, 0.001)

        ownerAddr = self.nodes[0].getnewaddress()
        votingAddr = self.nodes[0].getnewaddress()
        rewardsAddr = self.nodes[0].getnewaddress()

        port = p2p_port(len(self.nodes) + idx)
        if (idx % 2) == 0:
            self.nodes[0].lockunspent(True, [{'txid': txid, 'vout': collateral_vout}])
            proTxHash = self.nodes[0].protx('register_fund', address, '127.0.0.1:%d' % port, ownerAddr, bls['public'], votingAddr, 0, rewardsAddr, address)
        else:
            self.nodes[0].generate(1)
            proTxHash = self.nodes[0].protx('register', txid, collateral_vout, '127.0.0.1:%d' % port, ownerAddr, bls['public'], votingAddr, 0, rewardsAddr, address)
        self.nodes[0].generate(1)

        self.mninfo.append(MasternodeInfo(proTxHash, ownerAddr, votingAddr, bls['public'], bls['secret'], address, txid, collateral_vout))
        self.sync_all()

    def remove_mastermode(self, idx):
        mn = self.mninfo[idx]
        rawtx = self.nodes[0].createrawtransaction([{"txid": mn.collateral_txid, "vout": mn.collateral_vout}], {self.nodes[0].getnewaddress(): 999.9999})
        rawtx = self.nodes[0].signrawtransaction(rawtx)
        self.nodes[0].sendrawtransaction(rawtx["hex"])
        self.nodes[0].generate(1)
        self.sync_all()
        self.mninfo.remove(mn)

    def prepare_datadirs(self):
        # stop faucet node so that we can copy the datadir
        self.stop_node(0)

        start_idx = len(self.nodes)
        for idx in range(0, self.mn_count):
            copy_datadir(0, idx + start_idx, self.options.tmpdir)

        # restart faucet node
        self.nodes[0] = self.start_node(0, self.options.tmpdir, self.extra_args)

    def start_masternodes(self):
        start_idx = len(self.nodes)

        for idx in range(0, self.mn_count):
            self.nodes.append(None)
        executor = ThreadPoolExecutor(max_workers=20)

        def do_start(idx):
            args = ['-masternode=1',
                    '-masternodeblsprivkey=%s' % self.mninfo[idx].keyOperator] + self.extra_args
            node = self.start_node(idx + start_idx, self.options.tmpdir, args)
            self.mninfo[idx].nodeIdx = idx + start_idx
            self.mninfo[idx].node = node
            self.nodes[idx + start_idx] = node
            wait_to_sync(node, True)

        def do_connect(idx):
            for i in range(0, idx + 1):
                connect_nodes(self.nodes[idx + start_idx], i)

        jobs = []

        # start up nodes in parallel
        for idx in range(0, self.mn_count):
            jobs.append(executor.submit(do_start, idx))

        # wait for all nodes to start up
        for job in jobs:
            job.result()
        jobs.clear()

        # connect nodes in parallel
        for idx in range(0, self.mn_count):
            jobs.append(executor.submit(do_connect, idx))

        # wait for all nodes to connect
        for job in jobs:
            job.result()
        jobs.clear()

        sync_masternodes(self.nodes, True)

        executor.shutdown()

    def setup_network(self):
        self.nodes = []
        # create faucet node for collateral and transactions
        self.nodes.append(self.start_node(0, self.options.tmpdir, self.extra_args))
        required_balance = MASTERNODE_COLLATERAL * self.mn_count + 1
        while self.nodes[0].getbalance() < required_balance:
            self.bump_mocktime(1)
            set_node_times(self.nodes, self.mocktime)
            self.nodes[0].generate(1)
        # create connected simple nodes
        for i in range(0, self.num_nodes - self.mn_count - 1):
            self.create_simple_node()
        sync_masternodes(self.nodes, True)

        # activate DIP3
        if not self.fast_dip3_enforcement:
            while self.nodes[0].getblockcount() < 500:
                self.nodes[0].generate(10)
        self.sync_all()

        # create masternodes
        self.prepare_masternodes()
        self.prepare_datadirs()
        self.start_masternodes()

        self.bump_mocktime(1)
        set_node_times(self.nodes, self.mocktime)
        self.nodes[0].generate(1)
        # sync nodes
        self.sync_all()
        self.bump_mocktime(1)
        set_node_times(self.nodes, self.mocktime)

        mn_info = self.nodes[0].masternodelist("status")
        assert (len(mn_info) == self.mn_count)
        for status in mn_info.values():
            assert (status == 'ENABLED')

    def create_raw_tx(self, node_from, node_to, amount, min_inputs, max_inputs):
        assert (min_inputs <= max_inputs)
        # fill inputs
        inputs = []
        balances = node_from.listunspent()
        in_amount = 0.0
        last_amount = 0.0
        for tx in balances:
            if len(inputs) < min_inputs:
                input = {}
                input["txid"] = tx['txid']
                input['vout'] = tx['vout']
                in_amount += float(tx['amount'])
                inputs.append(input)
            elif in_amount > amount:
                break
            elif len(inputs) < max_inputs:
                input = {}
                input["txid"] = tx['txid']
                input['vout'] = tx['vout']
                in_amount += float(tx['amount'])
                inputs.append(input)
            else:
                input = {}
                input["txid"] = tx['txid']
                input['vout'] = tx['vout']
                in_amount -= last_amount
                in_amount += float(tx['amount'])
                inputs[-1] = input
            last_amount = float(tx['amount'])

        assert (len(inputs) >= min_inputs)
        assert (len(inputs) <= max_inputs)
        assert (in_amount >= amount)
        # fill outputs
        receiver_address = node_to.getnewaddress()
        change_address = node_from.getnewaddress()
        fee = 0.001
        outputs = {}
        outputs[receiver_address] = satoshi_round(amount)
        outputs[change_address] = satoshi_round(in_amount - amount - fee)
        rawtx = node_from.createrawtransaction(inputs, outputs)
        ret = node_from.signrawtransaction(rawtx)
        decoded = node_from.decoderawtransaction(ret['hex'])
        ret = {**decoded, **ret}
        return ret

    def wait_for_instantlock(self, txid, node):
        # wait for instantsend locks
        start = time.time()
        locked = False
        while True:
            try:
                is_tx = node.getrawtransaction(txid, True)
                if is_tx['instantlock']:
                    locked = True
                    break
            except:
                # TX not received yet?
                pass
            if time.time() > start + 10:
                break
            time.sleep(0.5)
        return locked

    def wait_for_sporks_same(self, timeout=30):
        st = time.time()
        while time.time() < st + timeout:
            if self.check_sporks_same():
                return
            time.sleep(0.5)
        raise AssertionError("wait_for_sporks_same timed out")

    def check_sporks_same(self):
        sporks = self.nodes[0].spork('show')
        for node in self.nodes[1:]:
            sporks2 = node.spork('show')
            if sporks != sporks2:
                return False
        return True

    def wait_for_quorum_phase(self, phase, check_received_messages, check_received_messages_count, timeout=30):
        t = time.time()
        while time.time() - t < timeout:
            all_ok = True
            for mn in self.mninfo:
                s = mn.node.quorum("dkgstatus")["session"]
                if "llmq_5_60" not in s:
                    all_ok = False
                    break
                s = s["llmq_5_60"]
                if "phase" not in s:
                    all_ok = False
                    break
                if s["phase"] != phase:
                    all_ok = False
                    break
                if check_received_messages is not None:
                    if s[check_received_messages] < check_received_messages_count:
                        all_ok = False
                        break
            if all_ok:
                return
            time.sleep(0.1)
        raise AssertionError("wait_for_quorum_phase timed out")

    def wait_for_quorum_commitment(self, timeout = 15):
        t = time.time()
        while time.time() - t < timeout:
            all_ok = True
            for node in self.nodes:
                s = node.quorum("dkgstatus")
                if "minableCommitments" not in s:
                    all_ok = False
                    break
                s = s["minableCommitments"]
                if "llmq_5_60" not in s:
                    all_ok = False
                    break
            if all_ok:
                return
            time.sleep(0.1)
        raise AssertionError("wait_for_quorum_commitment timed out")

    def mine_quorum(self, expected_contributions=5, expected_complaints=0, expected_justifications=0, expected_commitments=5):
        quorums = self.nodes[0].quorum("list")

        # move forward to next DKG
        skip_count = 24 - (self.nodes[0].getblockcount() % 24)
        if skip_count != 0:
            self.bump_mocktime(1)
            set_node_times(self.nodes, self.mocktime)
            self.nodes[0].generate(skip_count)
        sync_blocks(self.nodes)

        # Make sure all reached phase 1 (init)
        self.wait_for_quorum_phase(1, None, 0)
        # Give nodes some time to connect to neighbors
        time.sleep(2)
        self.bump_mocktime(1)
        set_node_times(self.nodes, self.mocktime)
        self.nodes[0].generate(2)
        sync_blocks(self.nodes)

        # Make sure all reached phase 2 (contribute) and received all contributions
        self.wait_for_quorum_phase(2, "receivedContributions", expected_contributions)
        self.bump_mocktime(1)
        set_node_times(self.nodes, self.mocktime)
        self.nodes[0].generate(2)
        sync_blocks(self.nodes)

        # Make sure all reached phase 3 (complain) and received all complaints
        self.wait_for_quorum_phase(3, "receivedComplaints", expected_complaints)
        self.bump_mocktime(1)
        set_node_times(self.nodes, self.mocktime)
        self.nodes[0].generate(2)
        sync_blocks(self.nodes)

        # Make sure all reached phase 4 (justify)
        self.wait_for_quorum_phase(4, "receivedJustifications", expected_justifications)
        self.bump_mocktime(1)
        set_node_times(self.nodes, self.mocktime)
        self.nodes[0].generate(2)
        sync_blocks(self.nodes)

        # Make sure all reached phase 5 (commit)
        self.wait_for_quorum_phase(5, "receivedPrematureCommitments", expected_commitments)
        self.bump_mocktime(1)
        set_node_times(self.nodes, self.mocktime)
        self.nodes[0].generate(2)
        sync_blocks(self.nodes)

        # Make sure all reached phase 6 (mining)
        self.wait_for_quorum_phase(6, None, 0)

        # Wait for final commitment
        self.wait_for_quorum_commitment()

        # mine the final commitment
        self.bump_mocktime(1)
        set_node_times(self.nodes, self.mocktime)
        self.nodes[0].generate(1)
        while quorums == self.nodes[0].quorum("list"):
            time.sleep(2)
            self.bump_mocktime(1)
            set_node_times(self.nodes, self.mocktime)
            self.nodes[0].generate(1)
            sync_blocks(self.nodes)
        new_quorum = self.nodes[0].quorum("list", 1)["llmq_5_60"][0]

        # Mine 8 (SIGN_HEIGHT_OFFSET) more blocks to make sure that the new quorum gets eligable for signing sessions
        self.nodes[0].generate(8)

        sync_blocks(self.nodes)

        return new_quorum

class ComparisonTestFramework(BitcoinTestFramework):
    """Test framework for doing p2p comparison testing

    Sets up some dashd binaries:
    - 1 binary: test binary
    - 2 binaries: 1 test binary, 1 ref binary
    - n>2 binaries: 1 test binary, n-1 ref binaries"""

    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = True

    def add_options(self, parser):
        parser.add_option("--testbinary", dest="testbinary",
                          default=os.getenv("BITCOIND", "dashd"),
                          help="dashd binary to test")
        parser.add_option("--refbinary", dest="refbinary",
                          default=os.getenv("BITCOIND", "dashd"),
                          help="dashd binary to use for reference nodes (if any)")

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
