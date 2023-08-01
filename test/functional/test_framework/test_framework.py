#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Copyright (c) 2014-2022 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Base class for RPC testing."""

import configparser
import copy
from _decimal import Decimal
from enum import Enum
import argparse
import logging
import os
import pdb
import random
import shutil
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor

from .authproxy import JSONRPCException
from test_framework.blocktools import TIME_GENESIS_BLOCK
from . import coverage
from .messages import (
    CTransaction,
    FromHex,
    hash256,
    msg_islock,
    msg_isdlock,
    ser_compact_size,
    ser_string,
)
from .script import hash160
from .test_node import TestNode
from .mininode import NetworkThread
from .util import (
    PortSeed,
    MAX_NODES,
    assert_equal,
    check_json_precision,
    copy_datadir,
    force_finish_mnsync,
    get_datadir_path,
    hex_str_to_bytes,
    initialize_datadir,
    make_change,
    p2p_port,
    set_node_times,
    satoshi_round,
    softfork_active,
    wait_until,
    get_chain_folder, rpc_port,
)


class TestStatus(Enum):
    PASSED = 1
    FAILED = 2
    SKIPPED = 3

TEST_EXIT_PASSED = 0
TEST_EXIT_FAILED = 1
TEST_EXIT_SKIPPED = 77

TMPDIR_PREFIX = "dash_func_test_"

class SkipTest(Exception):
    """This exception is raised to skip a test"""

    def __init__(self, message):
        self.message = message


class BitcoinTestMetaClass(type):
    """Metaclass for BitcoinTestFramework.

    Ensures that any attempt to register a subclass of `BitcoinTestFramework`
    adheres to a standard whereby the subclass overrides `set_test_params` and
    `run_test` but DOES NOT override either `__init__` or `main`. If any of
    those standards are violated, a ``TypeError`` is raised."""

    def __new__(cls, clsname, bases, dct):
        if not clsname == 'BitcoinTestFramework':
            if not ('run_test' in dct and 'set_test_params' in dct):
                raise TypeError("BitcoinTestFramework subclasses must override "
                                "'run_test' and 'set_test_params'")
            if '__init__' in dct or 'main' in dct:
                raise TypeError("BitcoinTestFramework subclasses may not override "
                                "'__init__' or 'main'")

        return super().__new__(cls, clsname, bases, dct)


class BitcoinTestFramework(metaclass=BitcoinTestMetaClass):
    """Base class for a bitcoin test script.

    Individual bitcoin test scripts should subclass this class and override the set_test_params() and run_test() methods.

    Individual tests can also override the following methods to customize the test setup:

    - add_options()
    - setup_chain()
    - setup_network()
    - setup_nodes()

    The __init__() and main() methods should not be overridden.

    This class also contains various public and private helper methods."""

    def __init__(self):
        """Sets test framework defaults. Do not override this method. Instead, override the set_test_params() method"""
        self.chain = 'regtest'
        self.setup_clean_chain = False
        self.nodes = []
        self.network_thread = None
        self.mocktime = 0
        self.rpc_timeout = 60  # Wait for up to 60 seconds for the RPC server to respond
        self.supports_cli = True
        self.bind_to_localhost_only = True
        self.parse_args()
        self.default_wallet_name = "default_wallet" if self.options.is_sqlite_only else ""
        self.wallet_data_filename = "wallet.dat"
        self.extra_args_from_options = []
        # Optional list of wallet names that can be set in set_test_params to
        # create and import keys to. If unset, default is len(nodes) *
        # [default_wallet_name]. If wallet names are None, wallet creation is
        # skipped. If list is truncated, wallet creation is skipped and keys
        # are not imported.
        self.wallet_names = None
        # By default the wallet is not required. Set to true by skip_if_no_wallet().
        # When False, we ignore wallet_names regardless of what it is.
        self.requires_wallet = False
        self.set_test_params()
        assert self.wallet_names is None or len(self.wallet_names) <= self.num_nodes
        if self.options.timeout_scale != 1:
            print("DEPRECATED: --timeoutscale option is no longer available, please use --timeout-factor instead")
            if self.options.timeout_factor == 1:
                self.options.timeout_factor = self.options.timeout_scale
        if self.options.timeout_factor == 0 :
            self.options.timeout_factor = 99999
        self.rpc_timeout = int(self.rpc_timeout * self.options.timeout_factor) # optionally, increase timeout by a factor

    def main(self):
        """Main function. This should not be overridden by the subclass test scripts."""

        assert hasattr(self, "num_nodes"), "Test must set self.num_nodes in set_test_params()"

        try:
            self.setup()
            self.run_test()
        except JSONRPCException:
            self.log.exception("JSONRPC error")
            self.success = TestStatus.FAILED
        except SkipTest as e:
            self.log.warning("Test Skipped: %s" % e.message)
            self.success = TestStatus.SKIPPED
        except AssertionError:
            self.log.exception("Assertion failed")
            self.success = TestStatus.FAILED
        except KeyError:
            self.log.exception("Key error")
            self.success = TestStatus.FAILED
        except subprocess.CalledProcessError as e:
            self.log.exception("Called Process failed with '{}'".format(e.output))
            self.success = TestStatus.FAILED
        except Exception:
            self.log.exception("Unexpected exception caught during testing")
            self.success = TestStatus.FAILED
        except KeyboardInterrupt:
            self.log.warning("Exiting after keyboard interrupt")
            self.success = TestStatus.FAILED
        finally:
            exit_code = self.shutdown()
            sys.exit(exit_code)

    def parse_args(self):
        parser = argparse.ArgumentParser(usage="%(prog)s [options]")
        parser.add_argument("--nocleanup", dest="nocleanup", default=False, action="store_true",
                            help="Leave dashds and test.* datadir on exit or error")
        parser.add_argument("--noshutdown", dest="noshutdown", default=False, action="store_true",
                            help="Don't stop dashds after the test execution")
        parser.add_argument("--cachedir", dest="cachedir", default=os.path.abspath(os.path.dirname(os.path.realpath(__file__)) + "/../../cache"),
                            help="Directory for caching pregenerated datadirs (default: %(default)s)")
        parser.add_argument("--tmpdir", dest="tmpdir", help="Root directory for datadirs")
        parser.add_argument("-l", "--loglevel", dest="loglevel", default="INFO",
                            help="log events at this level and higher to the console. Can be set to DEBUG, INFO, WARNING, ERROR or CRITICAL. Passing --loglevel DEBUG will output all logs to console. Note that logs at all levels are always written to the test_framework.log file in the temporary test directory.")
        parser.add_argument("--tracerpc", dest="trace_rpc", default=False, action="store_true",
                            help="Print out all RPC calls as they are made")
        parser.add_argument("--portseed", dest="port_seed", default=os.getpid(), type=int,
                            help="The seed to use for assigning port numbers (default: current process id)")
        parser.add_argument("--coveragedir", dest="coveragedir",
                            help="Write tested RPC commands into this directory")
        parser.add_argument("--configfile", dest="configfile",
                            default=os.path.abspath(os.path.dirname(os.path.realpath(__file__)) + "/../../config.ini"),
                            help="Location of the test framework config file (default: %(default)s)")
        parser.add_argument("--pdbonfailure", dest="pdbonfailure", default=False, action="store_true",
                            help="Attach a python debugger if test fails")
        parser.add_argument("--usecli", dest="usecli", default=False, action="store_true",
                            help="use dash-cli instead of RPC for all commands")
        parser.add_argument("--dashd-arg", dest="dashd_extra_args", default=[], action="append",
                            help="Pass extra args to all dashd instances")
        parser.add_argument("--timeoutscale", dest="timeout_scale", default=1, type=int,
                            help=argparse.SUPPRESS)
        parser.add_argument("--perf", dest="perf", default=False, action="store_true",
                            help="profile running nodes with perf for the duration of the test")
        parser.add_argument("--valgrind", dest="valgrind", default=False, action="store_true",
                            help="run nodes under the valgrind memory error detector: expect at least a ~10x slowdown, valgrind 3.14 or later required")
        parser.add_argument("--randomseed", type=int,
                            help="set a random seed for deterministically reproducing a previous test run")
        parser.add_argument('--timeout-factor', dest="timeout_factor", type=float, default=1.0, help='adjust test timeouts by a factor. Setting it to 0 disables all timeouts')

        self.add_options(parser)
        self.options = parser.parse_args()

        config = configparser.ConfigParser()
        config.read_file(open(self.options.configfile))
        self.config = config

        # Passthrough SQLite-only availability check output as option
        self.options.is_sqlite_only = self.is_sqlite_compiled() and not self.is_bdb_compiled()

        # Running TestShell in a Jupyter notebook causes an additional -f argument
        # To keep TestShell from failing with an "unrecognized argument" error, we add a dummy "-f" argument
        # source: https://stackoverflow.com/questions/48796169/how-to-fix-ipykernel-launcher-py-error-unrecognized-arguments-in-jupyter/56349168#56349168
        parser.add_argument("-f", "--fff", help="a dummy argument to fool ipython", default="1")

    def setup(self):
        """Call this method to start up the test framework object with options set."""

        PortSeed.n = self.options.port_seed

        check_json_precision()

        self.options.cachedir = os.path.abspath(self.options.cachedir)

        config = self.config

        fname_bitcoind = os.path.join(
            config["environment"]["BUILDDIR"],
            "src",
            "dashd" + config["environment"]["EXEEXT"]
        )
        fname_bitcoincli = os.path.join(
            config["environment"]["BUILDDIR"],
            "src",
            "dash-cli" + config["environment"]["EXEEXT"]
        )
        self.options.bitcoind = os.getenv("BITCOIND", default=fname_bitcoind)
        self.options.bitcoincli = os.getenv("BITCOINCLI", default=fname_bitcoincli)

        self.extra_args_from_options = self.options.dashd_extra_args

        self.options.previous_releases_path = os.getenv("PREVIOUS_RELEASES_DIR") or os.getcwd() + "/releases"

        os.environ['PATH'] = os.pathsep.join([
            os.path.join(config['environment']['BUILDDIR'], 'src'),
            os.path.join(config['environment']['BUILDDIR'], 'src', 'qt'), os.environ['PATH']
        ])

        # Set up temp directory and start logging
        if self.options.tmpdir:
            self.options.tmpdir = os.path.abspath(self.options.tmpdir)
            os.makedirs(self.options.tmpdir, exist_ok=False)
        else:
            self.options.tmpdir = tempfile.mkdtemp(prefix=TMPDIR_PREFIX)
        self._start_logging()

        # Seed the PRNG. Note that test runs are reproducible if and only if
        # a single thread accesses the PRNG. For more information, see
        # https://docs.python.org/3/library/random.html#notes-on-reproducibility.
        # The network thread shouldn't access random. If we need to change the
        # network thread to access randomness, it should instantiate its own
        # random.Random object.
        seed = self.options.randomseed

        if seed is None:
            seed = random.randrange(sys.maxsize)
        else:
            self.log.debug("User supplied random seed {}".format(seed))

        random.seed(seed)
        self.log.debug("PRNG seed is: {}".format(seed))

        self.log.debug('Setting up network thread')
        self.network_thread = NetworkThread()
        self.network_thread.start()

        if self.options.usecli:
            if not self.supports_cli:
                raise SkipTest("--usecli specified but test does not support using CLI")
            self.skip_if_no_cli()
        self.skip_test_if_missing_module()
        self.setup_chain()
        self.setup_network()

        self.success = TestStatus.PASSED

    def shutdown(self):
        """Call this method to shut down the test framework object."""

        if self.success == TestStatus.FAILED and self.options.pdbonfailure:
            print("Testcase failed. Attaching python debugger. Enter ? for help")
            pdb.set_trace()

        self.log.debug('Closing down network thread')
        self.network_thread.close()
        if not self.options.noshutdown:
            self.log.info("Stopping nodes")
            try:
                if self.nodes:
                    self.stop_nodes()
            except BaseException:
                self.success = TestStatus.FAILED
                self.log.exception("Unexpected exception caught during shutdown")
        else:
            for node in self.nodes:
                node.cleanup_on_exit = False
            self.log.info("Note: dashds were not stopped and may still be running")

        should_clean_up = (
            not self.options.nocleanup and
            not self.options.noshutdown and
            self.success != TestStatus.FAILED and
            not self.options.perf
        )
        if should_clean_up:
            self.log.info("Cleaning up {} on exit".format(self.options.tmpdir))
            cleanup_tree_on_exit = True
        elif self.options.perf:
            self.log.warning("Not cleaning up dir {} due to perf data".format(self.options.tmpdir))
            cleanup_tree_on_exit = False
        else:
            self.log.warning("Not cleaning up dir {}".format(self.options.tmpdir))
            cleanup_tree_on_exit = False

        if self.success == TestStatus.PASSED:
            self.log.info("Tests successful")
            exit_code = TEST_EXIT_PASSED
        elif self.success == TestStatus.SKIPPED:
            self.log.info("Test skipped")
            exit_code = TEST_EXIT_SKIPPED
        else:
            self.log.error("Test failed. Test logging available at %s/test_framework.log", self.options.tmpdir)
            self.log.error("")
            self.log.error("Hint: Call {} '{}' to consolidate all logs".format(os.path.normpath(os.path.dirname(os.path.realpath(__file__)) + "/../combine_logs.py"), self.options.tmpdir))
            self.log.error("")
            self.log.error("If this failure happened unexpectedly or intermittently, please file a bug and provide a link or upload of the combined log.")
            self.log.error(self.config['environment']['PACKAGE_BUGREPORT'])
            self.log.error("")
            exit_code = TEST_EXIT_FAILED
        # Logging.shutdown will not remove stream- and filehandlers, so we must
        # do it explicitly. Handlers are removed so the next test run can apply
        # different log handler settings.
        # See: https://docs.python.org/3/library/logging.html#logging.shutdown
        for h in list(self.log.handlers):
            h.flush()
            h.close()
            self.log.removeHandler(h)
        rpc_logger = logging.getLogger("BitcoinRPC")
        for h in list(rpc_logger.handlers):
            h.flush()
            rpc_logger.removeHandler(h)
        if cleanup_tree_on_exit:
            shutil.rmtree(self.options.tmpdir)

        self.nodes.clear()
        return exit_code

    # Methods to override in subclass test scripts.
    def set_test_params(self):
        """Tests must override this method to change default values for number of nodes, topology, etc"""
        raise NotImplementedError

    def add_options(self, parser):
        """Override this method to add command-line options to the test"""
        pass

    def skip_test_if_missing_module(self):
        """Override this method to skip a test if a module is not compiled"""
        pass

    def setup_chain(self):
        """Override this method to customize blockchain setup"""
        self.log.info("Initializing test directory " + self.options.tmpdir)
        if self.setup_clean_chain:
            self._initialize_chain_clean()
            self.set_genesis_mocktime()
        else:
            self._initialize_chain()
            self.set_cache_mocktime()

    def setup_network(self):
        """Override this method to customize test network topology"""
        self.setup_nodes()

        # Connect the nodes as a "chain".  This allows us
        # to split the network between nodes 1 and 2 to get
        # two halves that can work on competing chains.
        #
        # Topology looks like this:
        # node0 <-- node1 <-- node2 <-- node3
        #
        # If all nodes are in IBD (clean chain from genesis), node0 is assumed to be the source of blocks (miner). To
        # ensure block propagation, all nodes will establish outgoing connections toward node0.
        # See fPreferredDownload in net_processing.
        #
        # If further outbound connections are needed, they can be added at the beginning of the test with e.g.
        # self.connect_nodes(1, 2)
        for i in range(self.num_nodes - 1):
            self.connect_nodes(i + 1, i)
        self.sync_all()

    def setup_nodes(self):
        """Override this method to customize test node setup"""
        extra_args = None
        if hasattr(self, "extra_args"):
            extra_args = self.extra_args
        self.add_nodes(self.num_nodes, extra_args)
        self.start_nodes()
        if self.requires_wallet:
            self.import_deterministic_coinbase_privkeys()
        if not self.setup_clean_chain:
            for n in self.nodes:
                assert_equal(n.getblockchaininfo()["blocks"], 199)
            # To ensure that all nodes are out of IBD, the most recent block
            # must have a timestamp not too old (see IsInitialBlockDownload()).
            self.log.debug('Generate a block with current mocktime')
            self.bump_mocktime(156 * 200)
            block_hash = self.nodes[0].generate(1)[0]
            block = self.nodes[0].getblock(blockhash=block_hash, verbosity=0)
            for n in self.nodes:
                n.submitblock(block)
                chain_info = n.getblockchaininfo()
                assert_equal(chain_info["blocks"], 200)
                assert_equal(chain_info["initialblockdownload"], False)

    def import_deterministic_coinbase_privkeys(self):
        for i in range(len(self.nodes)):
            self.init_wallet(i)

    def init_wallet(self, i):
        wallet_name = self.default_wallet_name if self.wallet_names is None else self.wallet_names[i] if i < len(self.wallet_names) else False
        if wallet_name is not False:
            n = self.nodes[i]
            if wallet_name is not None:
                n.createwallet(wallet_name=wallet_name, load_on_startup=True)
            n.importprivkey(privkey=n.get_deterministic_priv_key().key, label='coinbase')

    def run_test(self):
        """Tests must override this method to define test logic"""
        raise NotImplementedError

    # Public helper methods. These can be accessed by the subclass test scripts.

    def add_nodes(self, num_nodes, extra_args=None, *, rpchost=None, binary=None):
        """Instantiate TestNode objects.

        Should only be called once after the nodes have been specified in
        set_test_params()."""
        if self.bind_to_localhost_only:
            extra_confs = [["bind=127.0.0.1"]] * num_nodes
        else:
            extra_confs = [[]] * num_nodes
        if extra_args is None:
            extra_args = [[]] * num_nodes
        if binary is None:
            binary = [self.options.bitcoind] * num_nodes
        assert_equal(len(extra_confs), num_nodes)
        assert_equal(len(extra_args), num_nodes)
        assert_equal(len(binary), num_nodes)
        old_num_nodes = len(self.nodes)
        for i in range(num_nodes):
            self.nodes.append(TestNode(
                old_num_nodes + i,
                get_datadir_path(self.options.tmpdir, old_num_nodes + i),
                self.extra_args_from_options,
                chain=self.chain,
                rpchost=rpchost,
                timewait=self.rpc_timeout,
                timeout_factor=self.options.timeout_factor,
                bitcoind=binary[i],
                bitcoin_cli=self.options.bitcoincli,
                mocktime=self.mocktime,
                coverage_dir=self.options.coveragedir,
                cwd=self.options.tmpdir,
                extra_conf=extra_confs[i],
                extra_args=extra_args[i],
                use_cli=self.options.usecli,
                start_perf=self.options.perf,
                use_valgrind=self.options.valgrind,
            ))

    def add_dynamically_node(self, extra_args=None, *, rpchost=None, binary=None):
        if self.bind_to_localhost_only:
            extra_confs = [["bind=127.0.0.1"]]
        else:
            extra_confs = [[]]
        if extra_args is None:
            extra_args = [[]]
        if binary is None:
            binary = [self.options.bitcoind]
        assert_equal(len(extra_confs), 1)
        assert_equal(len(binary), 1)
        old_num_nodes = len(self.nodes)

        p0 = old_num_nodes
        p1 = get_datadir_path(self.options.tmpdir, old_num_nodes)
        p2 = self.extra_args_from_options
        p3 = self.chain
        p4 = rpchost
        p5 = self.rpc_timeout
        p6 = self.options.timeout_factor
        p7 = binary[0]
        p8 = self.options.bitcoincli
        p9 = self.mocktime
        p10 = self.options.coveragedir
        p11 = self.options.tmpdir
        p12 = extra_confs[0]
        p13 = extra_args
        p14 = self.options.usecli
        p15 = self.options.perf
        p16 = self.options.valgrind

        t_node = TestNode(p0, p1, p2, chain=p3, rpchost=p4, timewait=p5, timeout_factor=p6, bitcoind=p7, bitcoin_cli=p8, mocktime=p9, coverage_dir=p10, cwd=p11, extra_conf=p12, extra_args=p13, use_cli=p14, start_perf=p15, use_valgrind=p16)
        self.nodes.append(t_node)
        return t_node

    def dynamically_initialize_datadir(self, node_p2p_port, node_rpc_port):
        source_data_dir = get_datadir_path(self.options.tmpdir, 0)  # use node0 as a source
        new_data_dir = get_datadir_path(self.options.tmpdir, len(self.nodes))

        # In general, it's a pretty bad idea to copy datadir folder on the fly...
        # But we flush all state changes to disk via gettxoutsetinfo call and
        # we don't care about wallets, so it works
        self.nodes[0].gettxoutsetinfo()
        shutil.copytree(source_data_dir, new_data_dir)

        shutil.rmtree(os.path.join(new_data_dir, self.chain, 'wallets'))
        shutil.rmtree(os.path.join(new_data_dir, self.chain, 'llmq'))

        for entry in os.listdir(os.path.join(new_data_dir, self.chain)):
            if entry not in ['chainstate', 'blocks', 'indexes', 'evodb']:
                os.remove(os.path.join(new_data_dir, self.chain, entry))

        # Translate chain name to config name
        if self.chain == 'testnet3':
            chain_name_conf_arg = 'testnet'
            chain_name_conf_section = 'test'
            chain_name_conf_arg_value = '1'
        elif self.chain == 'devnet':
            chain_name_conf_arg = 'devnet'
            chain_name_conf_section = 'devnet'
            chain_name_conf_arg_value = 'devnet1'
        else:
            chain_name_conf_arg = self.chain
            chain_name_conf_section = self.chain
            chain_name_conf_arg_value = '1'

        with open(os.path.join(new_data_dir, "dash.conf"), 'w', encoding='utf8') as f:
            f.write("{}={}\n".format(chain_name_conf_arg, chain_name_conf_arg_value))
            f.write("[{}]\n".format(chain_name_conf_section))
            f.write("port=" + str(node_p2p_port) + "\n")
            f.write("rpcport=" + str(node_rpc_port) + "\n")
            f.write("server=1\n")
            f.write("debug=1\n")
            f.write("keypool=1\n")
            f.write("discover=0\n")
            f.write("listenonion=0\n")
            f.write("printtoconsole=0\n")
            f.write("upnp=0\n")
            f.write("natpmp=0\n")
            f.write("shrinkdebugfile=0\n")
            os.makedirs(os.path.join(new_data_dir, 'stderr'), exist_ok=True)
            os.makedirs(os.path.join(new_data_dir, 'stdout'), exist_ok=True)

    def start_node(self, i, *args, **kwargs):
        """Start a dashd"""

        node = self.nodes[i]

        node.start(*args, **kwargs)
        node.wait_for_rpc_connection()

        if self.options.coveragedir is not None:
            coverage.write_all_rpc_commands(self.options.coveragedir, node.rpc)

    def start_nodes(self, extra_args=None, *args, **kwargs):
        """Start multiple dashds"""

        if extra_args is None:
            extra_args = [None] * self.num_nodes
        assert_equal(len(extra_args), self.num_nodes)
        try:
            for i, node in enumerate(self.nodes):
                node.start(extra_args[i], *args, **kwargs)
            for node in self.nodes:
                node.wait_for_rpc_connection()
        except:
            # If one node failed to start, stop the others
            self.stop_nodes()
            raise

        if self.options.coveragedir is not None:
            for node in self.nodes:
                coverage.write_all_rpc_commands(self.options.coveragedir, node.rpc)

    def stop_node(self, i, expected_stderr='', wait=0):
        """Stop a dashd test node"""
        self.nodes[i].stop_node(expected_stderr=expected_stderr, wait=wait)
        self.nodes[i].wait_until_stopped()

    def stop_nodes(self, expected_stderr='', wait=0):
        """Stop multiple dashd test nodes"""
        for node in self.nodes:
            # Issue RPC to stop nodes
            node.stop_node(expected_stderr=expected_stderr, wait=wait)

        for node in self.nodes:
            # Wait for nodes to stop
            node.wait_until_stopped()

    def restart_node(self, i, extra_args=None, expected_stderr=''):
        """Stop and start a test node"""
        self.stop_node(i, expected_stderr)
        self.start_node(i, extra_args)

    def wait_for_node_exit(self, i, timeout):
        self.nodes[i].process.wait(timeout)

    def connect_nodes(self, a, b):
        def connect_nodes_helper(from_connection, node_num):
            ip_port = "127.0.0.1:" + str(p2p_port(node_num))
            from_connection.addnode(ip_port, "onetry")
            # poll until version handshake complete to avoid race conditions
            # with transaction relaying
            # See comments in net_processing:
            # * Must have a version message before anything else
            # * Must have a verack message before anything else
            wait_until(lambda: all(peer['version'] != 0 for peer in from_connection.getpeerinfo()))
            wait_until(lambda: all(peer['bytesrecv_per_msg'].pop('verack', 0) == 24 for peer in from_connection.getpeerinfo()))

        connect_nodes_helper(self.nodes[a], b)

    def disconnect_nodes(self, a, b):
        def disconnect_nodes_helper(from_connection, node_num):
            def get_peer_ids():
                result = []
                for peer in from_connection.getpeerinfo():
                    if "testnode{}".format(node_num) in peer['subver']:
                        result.append(peer['id'])
                return result

            peer_ids = get_peer_ids()
            if not peer_ids:
                self.log.warning("disconnect_nodes: {} and {} were not connected".format(
                    from_connection.index,
                    node_num,
                ))
                return
            for peer_id in peer_ids:
                try:
                    from_connection.disconnectnode(nodeid=peer_id)
                except JSONRPCException as e:
                    # If this node is disconnected between calculating the peer id
                    # and issuing the disconnect, don't worry about it.
                    # This avoids a race condition if we're mass-disconnecting peers.
                    if e.error['code'] != -29: # RPC_CLIENT_NODE_NOT_CONNECTED
                        raise

            # wait to disconnect
            wait_until(lambda: not get_peer_ids(), timeout=5)

        disconnect_nodes_helper(self.nodes[a], b)

    def isolate_node(self, node_num, timeout=5):
        self.nodes[node_num].setnetworkactive(False)
        wait_until(lambda: self.nodes[node_num].getconnectioncount() == 0, timeout=timeout)

    def reconnect_isolated_node(self, a, b):
        self.nodes[a].setnetworkactive(True)
        self.connect_nodes(a, b)

    def split_network(self):
        """
        Split the network of four nodes into nodes 0/1 and 2/3.
        """
        self.disconnect_nodes(1, 2)
        self.sync_all(self.nodes[:2])
        self.sync_all(self.nodes[2:])

    def join_network(self):
        """
        Join the (previously split) network halves together.
        """
        self.connect_nodes(1, 2)
        self.sync_all()

    def sync_blocks(self, nodes=None, wait=1, timeout=60):
        """
        Wait until everybody has the same tip.
        sync_blocks needs to be called with an rpc_connections set that has least
        one node already synced to the latest, stable tip, otherwise there's a
        chance it might return before all nodes are stably synced.
        """
        rpc_connections = nodes or self.nodes
        timeout = int(timeout * self.options.timeout_factor)
        stop_time = time.time() + timeout
        while time.time() <= stop_time:
            best_hash = [x.getbestblockhash() for x in rpc_connections]
            if best_hash.count(best_hash[0]) == len(rpc_connections):
                return
            # Check that each peer has at least one connection
            assert (all([len(x.getpeerinfo()) for x in rpc_connections]))
            time.sleep(wait)
        raise AssertionError("Block sync timed out after {}s:{}".format(
            timeout,
            "".join("\n  {!r}".format(b) for b in best_hash),
        ))

    def sync_mempools(self, nodes=None, wait=1, timeout=60, flush_scheduler=True, wait_func=None):
        """
        Wait until everybody has the same transactions in their memory
        pools
        """
        rpc_connections = nodes or self.nodes
        timeout = int(timeout * self.options.timeout_factor)
        stop_time = time.time() + timeout
        if self.mocktime != 0 and wait_func is None:
            wait_func = lambda: self.bump_mocktime(3, nodes=nodes)
        while time.time() <= stop_time:
            pool = [set(r.getrawmempool()) for r in rpc_connections]
            if pool.count(pool[0]) == len(rpc_connections):
                if flush_scheduler:
                    for r in rpc_connections:
                        r.syncwithvalidationinterfacequeue()
                return
            # Check that each peer has at least one connection
            assert (all([len(x.getpeerinfo()) for x in rpc_connections]))
            if wait_func is not None:
                wait_func()
            time.sleep(wait)
        raise AssertionError("Mempool sync timed out after {}s:{}".format(
            timeout,
            "".join("\n  {!r}".format(m) for m in pool),
        ))

    def sync_all(self, nodes=None):
        self.sync_blocks(nodes)
        self.sync_mempools(nodes)

    def disable_mocktime(self):
        self.mocktime = 0
        for node in self.nodes:
            node.mocktime = 0

    def bump_mocktime(self, t, update_nodes=True, nodes=None):
        if self.mocktime != 0:
            self.mocktime += t
            if update_nodes:
                set_node_times(nodes or self.nodes, self.mocktime)

    def set_cache_mocktime(self):
        self.mocktime = TIME_GENESIS_BLOCK + (199 * 156)
        for node in self.nodes:
            node.mocktime = self.mocktime

    def set_genesis_mocktime(self):
        self.mocktime = TIME_GENESIS_BLOCK
        for node in self.nodes:
            node.mocktime = self.mocktime

    # Private helper methods. These should not be accessed by the subclass test scripts.

    def _start_logging(self):
        # Add logger and logging handlers
        self.log = logging.getLogger('TestFramework')
        self.log.setLevel(logging.DEBUG)
        # Create file handler to log all messages
        fh = logging.FileHandler(self.options.tmpdir + '/test_framework.log', encoding='utf-8')
        fh.setLevel(logging.DEBUG)
        # Create console handler to log messages to stderr. By default this logs only error messages, but can be configured with --loglevel.
        ch = logging.StreamHandler(sys.stdout)
        # User can provide log level as a number or string (eg DEBUG). loglevel was caught as a string, so try to convert it to an int
        ll = int(self.options.loglevel) if self.options.loglevel.isdigit() else self.options.loglevel.upper()
        ch.setLevel(ll)
        # Format logs the same as dashd's debug.log with microprecision (so log files can be concatenated and sorted)
        formatter = logging.Formatter(fmt='%(asctime)s.%(msecs)03d000Z %(name)s (%(levelname)s): %(message)s', datefmt='%Y-%m-%dT%H:%M:%S')
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

    def _initialize_chain(self):
        """Initialize a pre-mined blockchain for use by the test.

        Create a cache of a 199-block-long chain
        Afterward, create num_nodes copies from the cache."""

        CACHE_NODE_ID = 0  # Use node 0 to create the cache for all other nodes
        cache_node_dir = get_datadir_path(self.options.cachedir, CACHE_NODE_ID)
        assert self.num_nodes <= MAX_NODES

        if not os.path.isdir(cache_node_dir):
            self.log.debug("Creating cache directory {}".format(cache_node_dir))

            initialize_datadir(self.options.cachedir, CACHE_NODE_ID, self.chain)
            self.nodes.append(
                TestNode(
                    CACHE_NODE_ID,
                    cache_node_dir,
                    chain=self.chain,
                    extra_conf=["bind=127.0.0.1"],
                    extra_args=['-disablewallet', "-mocktime=%d" % TIME_GENESIS_BLOCK],
                    extra_args_from_options=self.extra_args_from_options,
                    rpchost=None,
                    timewait=self.rpc_timeout,
                    timeout_factor=self.options.timeout_factor,
                    bitcoind=self.options.bitcoind,
                    bitcoin_cli=self.options.bitcoincli,
                    mocktime=self.mocktime,
                    coverage_dir=None,
                    cwd=self.options.tmpdir,
                ))
            self.start_node(CACHE_NODE_ID)
            cache_node = self.nodes[CACHE_NODE_ID]

            # Wait for RPC connections to be ready
            cache_node.wait_for_rpc_connection()

            # Set a time in the past, so that blocks don't end up in the future
            cache_node.setmocktime(cache_node.getblockheader(cache_node.getbestblockhash())['time'])

            # Create a 199-block-long chain; each of the 4 first nodes
            # gets 25 mature blocks and 25 immature.
            # The 4th node gets only 24 immature blocks so that the very last
            # block in the cache does not age too much (have an old tip age).
            # This is needed so that we are out of IBD when the test starts,
            # see the tip age check in IsInitialBlockDownload().
            self.set_genesis_mocktime()
            for i in range(8):
                self.bump_mocktime((25 if i != 7 else 24) * 156)
                cache_node.generatetoaddress(
                    nblocks=25 if i != 7 else 24,
                    address=TestNode.PRIV_KEYS[i % 4].address,
                )

            assert_equal(cache_node.getblockchaininfo()["blocks"], 199)

            # Shut it down, and clean up cache directories:
            self.stop_nodes()
            self.nodes = []
            self.disable_mocktime()

            def cache_path(*paths):
                chain = get_chain_folder(cache_node_dir, self.chain)
                return os.path.join(cache_node_dir, chain, *paths)

            os.rmdir(cache_path('wallets'))  # Remove empty wallets dir
            for entry in os.listdir(cache_path()):
                if entry not in ['chainstate', 'blocks', 'indexes', 'evodb', 'llmq']:  # Keep some folders
                    os.remove(cache_path(entry))

        for i in range(self.num_nodes):
            self.log.debug("Copy cache directory {} to node {}".format(cache_node_dir, i))
            to_dir = get_datadir_path(self.options.tmpdir, i)
            shutil.copytree(cache_node_dir, to_dir)
            initialize_datadir(self.options.tmpdir, i, self.chain)  # Overwrite port/rpcport in dash.conf

    def _initialize_chain_clean(self):
        """Initialize empty blockchain for use by the test.

        Create an empty blockchain and num_nodes wallets.
        Useful if a test case wants complete control over initialization."""
        for i in range(self.num_nodes):
            initialize_datadir(self.options.tmpdir, i, self.chain)

    def skip_if_no_py3_zmq(self):
        """Attempt to import the zmq package and skip the test if the import fails."""
        try:
            import zmq  # noqa
        except ImportError:
            raise SkipTest("python3-zmq module not available.")

    def skip_if_no_bitcoind_zmq(self):
        """Skip the running test if dashd has not been compiled with zmq support."""
        if not self.is_zmq_compiled():
            raise SkipTest("dashd has not been built with zmq enabled.")

    def skip_if_no_wallet(self):
        """Skip the running test if wallet has not been compiled."""
        self.requires_wallet = True
        if not self.is_wallet_compiled():
            raise SkipTest("wallet has not been compiled.")

    def skip_if_no_sqlite(self):
        """Skip the running test if sqlite has not been compiled."""
        if not self.is_sqlite_compiled():
            raise SkipTest("sqlite has not been compiled.")

    def skip_if_no_bdb(self):
        """Skip the running test if BDB has not been compiled."""
        if not self.is_bdb_compiled():
            raise SkipTest("BDB has not been compiled.")

    def skip_if_no_wallet_tool(self):
        """Skip the running test if dash-wallet has not been compiled."""
        if not self.is_wallet_tool_compiled():
            raise SkipTest("dash-wallet has not been compiled")

    def skip_if_no_cli(self):
        """Skip the running test if dash-cli has not been compiled."""
        if not self.is_cli_compiled():
            raise SkipTest("dash-cli has not been compiled.")

    def skip_if_no_previous_releases(self):
        """Skip the running test if previous releases are not available."""
        if not self.has_previous_releases():
            raise SkipTest("previous releases not available or disabled")

    def has_previous_releases(self):
        """Checks whether previous releases are present and enabled."""
        if os.getenv("TEST_PREVIOUS_RELEASES") == "false":
            # disabled
            return False

        if not os.path.isdir(self.options.previous_releases_path):
            if os.getenv("TEST_PREVIOUS_RELEASES") == "true":
                raise AssertionError("TEST_PREVIOUS_RELEASES=true but releases missing: {}".format(
                    self.options.previous_releases_path))
            # missing
            return False
        return True

    def is_cli_compiled(self):
        """Checks whether dash-cli was compiled."""
        return self.config["components"].getboolean("ENABLE_CLI")

    def is_wallet_compiled(self):
        """Checks whether the wallet module was compiled."""
        return self.config["components"].getboolean("ENABLE_WALLET")

    def is_wallet_tool_compiled(self):
        """Checks whether dash-wallet was compiled."""
        return self.config["components"].getboolean("ENABLE_WALLET_TOOL")

    def is_zmq_compiled(self):
        """Checks whether the zmq module was compiled."""
        return self.config["components"].getboolean("ENABLE_ZMQ")

    def is_sqlite_compiled(self):
        """Checks whether the wallet module was compiled with Sqlite support."""
        return self.config["components"].getboolean("USE_SQLITE")

    def is_bdb_compiled(self):
        """Checks whether the wallet module was compiled with BDB support."""
        return self.config["components"].getboolean("USE_BDB")

MASTERNODE_COLLATERAL = 1000
HIGHPERFORMANCE_MASTERNODE_COLLATERAL = 4000

class MasternodeInfo:
    def __init__(self, proTxHash, ownerAddr, votingAddr, pubKeyOperator, keyOperator, collateral_address, collateral_txid, collateral_vout, addr, hpmn=False):
        self.proTxHash = proTxHash
        self.ownerAddr = ownerAddr
        self.votingAddr = votingAddr
        self.pubKeyOperator = pubKeyOperator
        self.keyOperator = keyOperator
        self.collateral_address = collateral_address
        self.collateral_txid = collateral_txid
        self.collateral_vout = collateral_vout
        self.addr = addr
        self.hpmn = hpmn


class DashTestFramework(BitcoinTestFramework):
    def set_test_params(self):
        """Tests must this method to change default values for number of nodes, topology, etc"""
        raise NotImplementedError

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        """Tests must override this method to define test logic"""
        raise NotImplementedError

    def set_dash_test_params(self, num_nodes, masterodes_count, extra_args=None, fast_dip3_enforcement=False, hpmn_count=0):
        self.mn_count = masterodes_count
        self.hpmn_count = hpmn_count
        self.num_nodes = num_nodes
        self.mninfo = []
        self.setup_clean_chain = True
        # additional args
        if extra_args is None:
            extra_args = [[]] * num_nodes
        assert_equal(len(extra_args), num_nodes)
        self.extra_args = [copy.deepcopy(a) for a in extra_args]
        self.extra_args[0] += ["-sporkkey=cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK"]
        self.fast_dip3_enforcement = fast_dip3_enforcement
        if fast_dip3_enforcement:
            for i in range(0, num_nodes):
                self.extra_args[i].append("-dip3params=30:50")

        # make sure to activate dip8 after prepare_masternodes has finished its job already
        self.set_dash_dip8_activation(200)

        # LLMQ default test params (no need to pass -llmqtestparams)
        self.llmq_size = 3
        self.llmq_threshold = 2

        # This is nRequestTimeout in dash-q-recovery thread
        self.quorum_data_thread_request_timeout_seconds = 10
        # This is EXPIRATION_TIMEOUT + EXPIRATION_BIAS in CQuorumDataRequest
        self.quorum_data_request_expiration_timeout = 360

    def set_dash_dip8_activation(self, activate_after_block):
        self.dip8_activation_height = activate_after_block
        for i in range(0, self.num_nodes):
            self.extra_args[i].append("-dip8params=%d" % (activate_after_block))

    def activate_dip8(self, slow_mode=False):
        # NOTE: set slow_mode=True if you are activating dip8 after a huge reorg
        # or nodes might fail to catch up otherwise due to a large
        # (MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16 blocks) reorg error.
        self.log.info("Wait for dip0008 activation")
        while self.nodes[0].getblockcount() < self.dip8_activation_height:
            self.bump_mocktime(10)
            self.nodes[0].generate(10)
            if slow_mode:
                self.sync_blocks()
        self.sync_blocks()

    def activate_by_name(self, name, expected_activation_height=None):
        self.log.info("Wait for " + name + " activation")

        # disable spork17 while mining blocks to activate "name" to prevent accidental quorum formation
        spork17_value = self.nodes[0].spork('show')['SPORK_17_QUORUM_DKG_ENABLED']
        self.bump_mocktime(1)
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 4070908800)
        self.wait_for_sporks_same()

        # mine blocks in batches
        batch_size = 10
        if expected_activation_height is not None:
            height = self.nodes[0].getblockcount()
            # NOTE: getblockchaininfo shows softforks active at block (window * 3 - 1)
            # since it's returning whether a softwork is active for the _next_ block.
            # Hence the last block prior to the activation is (expected_activation_height - 2).
            while expected_activation_height - height - 2 >= batch_size:
                self.bump_mocktime(batch_size)
                self.nodes[0].generate(batch_size)
                height += batch_size
                self.sync_blocks()
            blocks_left = expected_activation_height - height - 2
            assert blocks_left < batch_size
            self.bump_mocktime(blocks_left)
            self.nodes[0].generate(blocks_left)
            self.sync_blocks()
            assert not softfork_active(self.nodes[0], name)
            self.bump_mocktime(1)
            self.nodes[0].generate(1)
            self.sync_blocks()
        else:
            while not softfork_active(self.nodes[0], name):
                self.bump_mocktime(batch_size)
                self.nodes[0].generate(batch_size)
                self.sync_blocks()

        assert softfork_active(self.nodes[0], name)

        # revert spork17 changes
        self.bump_mocktime(1)
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", spork17_value)
        self.wait_for_sporks_same()

    def activate_dip0024(self, expected_activation_height=None):
        self.activate_by_name('dip0024', expected_activation_height)

    def activate_v19(self, expected_activation_height=None):
        self.activate_by_name('v19', expected_activation_height)

    def activate_v20(self, expected_activation_height=None):
        self.activate_by_name('v20', expected_activation_height)
    def activate_mn_rr(self, expected_activation_height=None):
        self.activate_by_name('mn_rr', expected_activation_height)

    def set_dash_llmq_test_params(self, llmq_size, llmq_threshold):
        self.llmq_size = llmq_size
        self.llmq_threshold = llmq_threshold
        for i in range(0, self.num_nodes):
            self.extra_args[i].append("-llmqtestparams=%d:%d" % (self.llmq_size, self.llmq_threshold))
            self.extra_args[i].append("-llmqtestinstantsendparams=%d:%d" % (self.llmq_size, self.llmq_threshold))

    def create_simple_node(self):
        idx = len(self.nodes)
        self.add_nodes(1, extra_args=[self.extra_args[idx]])
        self.start_node(idx)
        self.nodes[idx].createwallet(self.default_wallet_name)
        for i in range(0, idx):
            self.connect_nodes(i, idx)

    def dynamically_add_masternode(self, hpmn=False, rnd=None, should_be_rejected=False):
        mn_idx = len(self.nodes)

        node_p2p_port = p2p_port(mn_idx)
        node_rpc_port = rpc_port(mn_idx)

        protx_success = False
        try:
            created_mn_info = self.dynamically_prepare_masternode(mn_idx, node_p2p_port, hpmn, rnd)
            protx_success = True
        except:
            self.log.info("dynamically_prepare_masternode failed")

        assert_equal(protx_success, not should_be_rejected)

        if should_be_rejected:
            # nothing to do
            return

        self.dynamically_initialize_datadir(node_p2p_port, node_rpc_port)
        node_info = self.add_dynamically_node(self.extra_args[1])

        args = ['-masternodeblsprivkey=%s' % created_mn_info.keyOperator] + node_info.extra_args
        self.start_node(mn_idx, args)

        for mn_info in self.mninfo:
            if mn_info.proTxHash == created_mn_info.proTxHash:
                mn_info.nodeIdx = mn_idx
                mn_info.node = self.nodes[mn_idx]

        self.connect_nodes(mn_idx, 0)

        self.wait_for_sporks_same()
        self.sync_blocks(self.nodes)
        force_finish_mnsync(self.nodes[mn_idx])

        self.log.info("Successfully started and synced proTx:"+str(created_mn_info.proTxHash))
        return created_mn_info

    def dynamically_prepare_masternode(self, idx, node_p2p_port, hpmn=False, rnd=None):
        bls = self.nodes[0].bls('generate')
        collateral_address = self.nodes[0].getnewaddress()
        funds_address = self.nodes[0].getnewaddress()
        owner_address = self.nodes[0].getnewaddress()
        voting_address = self.nodes[0].getnewaddress()
        reward_address = self.nodes[0].getnewaddress()

        platform_node_id = hash160(b'%d' % rnd).hex() if rnd is not None else hash160(b'%d' % node_p2p_port).hex()
        platform_p2p_port = '%d' % (node_p2p_port + 101)
        platform_http_port = '%d' % (node_p2p_port + 102)

        collateral_amount = 4000 if hpmn else 1000
        outputs = {collateral_address: collateral_amount, funds_address: 1}
        collateral_txid = self.nodes[0].sendmany("", outputs)
        self.wait_for_instantlock(collateral_txid, self.nodes[0])
        tip = self.nodes[0].generate(1)[0]
        self.sync_all(self.nodes)

        rawtx = self.nodes[0].getrawtransaction(collateral_txid, 1, tip)
        assert_equal(rawtx['confirmations'], 1)
        collateral_vout = 0
        for txout in rawtx['vout']:
            if txout['value'] == Decimal(collateral_amount):
                collateral_vout = txout['n']
                break
        assert collateral_vout is not None

        ipAndPort = '127.0.0.1:%d' % node_p2p_port
        operatorReward = idx

        protx_result = None
        if hpmn:
            protx_result = self.nodes[0].protx("register_hpmn", collateral_txid, collateral_vout, ipAndPort, owner_address, bls['public'], voting_address, operatorReward, reward_address, platform_node_id, platform_p2p_port, platform_http_port, funds_address, True)
        else:
            protx_result = self.nodes[0].protx("register", collateral_txid, collateral_vout, ipAndPort, owner_address, bls['public'], voting_address, operatorReward, reward_address, funds_address, True)

        self.wait_for_instantlock(protx_result, self.nodes[0])
        tip = self.nodes[0].generate(1)[0]
        self.sync_all(self.nodes)

        assert_equal(self.nodes[0].getrawtransaction(protx_result, 1, tip)['confirmations'], 1)
        mn_info = MasternodeInfo(protx_result, owner_address, voting_address, bls['public'], bls['secret'], collateral_address, collateral_txid, collateral_vout, ipAndPort, hpmn)
        self.mninfo.append(mn_info)

        mn_type_str = "HPMN" if hpmn else "MN"
        self.log.info("Prepared %s %d: collateral_txid=%s, collateral_vout=%d, protxHash=%s" % (mn_type_str, idx, collateral_txid, collateral_vout, protx_result))
        return mn_info

    def dynamically_hpmn_update_service(self, hpmn_info, rnd=None, should_be_rejected=False):
        funds_address = self.nodes[0].getnewaddress()
        operator_reward_address = self.nodes[0].getnewaddress()

        # For the sake of the test, generate random nodeid, p2p and http platform values
        r = rnd if rnd is not None else random.randint(21000, 65000)
        platform_node_id = hash160(b'%d' % r).hex()
        platform_p2p_port = '%d' % (r + 1)
        platform_http_port = '%d' % (r + 2)

        fund_txid = self.nodes[0].sendtoaddress(funds_address, 1)
        self.wait_for_instantlock(fund_txid, self.nodes[0])
        tip = self.nodes[0].generate(1)[0]
        assert_equal(self.nodes[0].getrawtransaction(fund_txid, 1, tip)['confirmations'], 1)
        self.sync_all(self.nodes)

        protx_success = False
        try:
            protx_result = self.nodes[0].protx('update_service_hpmn', hpmn_info.proTxHash, hpmn_info.addr, hpmn_info.keyOperator, platform_node_id, platform_p2p_port, platform_http_port, operator_reward_address, funds_address)
            self.wait_for_instantlock(protx_result, self.nodes[0])
            tip = self.nodes[0].generate(1)[0]
            assert_equal(self.nodes[0].getrawtransaction(protx_result, 1, tip)['confirmations'], 1)
            self.sync_all(self.nodes)
            self.log.info("Updated HPMN %s: platformNodeID=%s, platformP2PPort=%s, platformHTTPPort=%s" % (hpmn_info.proTxHash, platform_node_id, platform_p2p_port, platform_http_port))
            protx_success = True
        except:
            self.log.info("protx_hpmn rejected")

        assert_equal(protx_success, not should_be_rejected)

    def prepare_masternodes(self):
        self.log.info("Preparing %d masternodes" % self.mn_count)
        rewardsAddr = self.nodes[0].getnewaddress()

        for idx in range(0, self.mn_count):
            self.prepare_masternode(idx, rewardsAddr, False)
        self.sync_all()

    def prepare_masternode(self, idx, rewardsAddr=None, hpmn=False):

        register_fund = (idx % 2) == 0

        bls = self.nodes[0].bls('generate')
        address = self.nodes[0].getnewaddress()

        collateral_amount = HIGHPERFORMANCE_MASTERNODE_COLLATERAL if hpmn else MASTERNODE_COLLATERAL
        txid = None
        txid = self.nodes[0].sendtoaddress(address, collateral_amount)
        collateral_vout = 0
        if not register_fund:
            txraw = self.nodes[0].getrawtransaction(txid, True)
            for vout_idx in range(0, len(txraw["vout"])):
                vout = txraw["vout"][vout_idx]
                if vout["value"] == collateral_amount:
                    collateral_vout = vout_idx
            self.nodes[0].lockunspent(False, [{'txid': txid, 'vout': collateral_vout}])

        # send to same address to reserve some funds for fees
        self.nodes[0].sendtoaddress(address, 0.001)

        ownerAddr = self.nodes[0].getnewaddress()
        # votingAddr = self.nodes[0].getnewaddress()
        if rewardsAddr is None:
            rewardsAddr = self.nodes[0].getnewaddress()
        votingAddr = ownerAddr
        # rewardsAddr = ownerAddr

        port = p2p_port(len(self.nodes) + idx)
        ipAndPort = '127.0.0.1:%d' % port
        operatorReward = idx

        submit = (idx % 4) < 2

        if register_fund:
            # self.nodes[0].lockunspent(True, [{'txid': txid, 'vout': collateral_vout}])
            protx_result = self.nodes[0].protx('register_fund', address, ipAndPort, ownerAddr, bls['public'], votingAddr, operatorReward, rewardsAddr, address, submit)
        else:
            self.nodes[0].generate(1)
            protx_result = self.nodes[0].protx('register', txid, collateral_vout, ipAndPort, ownerAddr, bls['public'], votingAddr, operatorReward, rewardsAddr, address, submit)

        if submit:
            proTxHash = protx_result
        else:
            proTxHash = self.nodes[0].sendrawtransaction(protx_result)

        if operatorReward > 0:
            self.nodes[0].generate(1)
            operatorPayoutAddress = self.nodes[0].getnewaddress()
            self.nodes[0].protx('update_service', proTxHash, ipAndPort, bls['secret'], operatorPayoutAddress, address)

        self.mninfo.append(MasternodeInfo(proTxHash, ownerAddr, votingAddr, bls['public'], bls['secret'], address, txid, collateral_vout, ipAndPort, hpmn))
        # self.sync_all()

        mn_type_str = "HPMN" if hpmn else "MN"
        self.log.info("Prepared %s %d: collateral_txid=%s, collateral_vout=%d, protxHash=%s" % (mn_type_str, idx, txid, collateral_vout, proTxHash))

    def remove_masternode(self, idx):
        mn = self.mninfo[idx]
        rawtx = self.nodes[0].createrawtransaction([{"txid": mn.collateral_txid, "vout": mn.collateral_vout}], {self.nodes[0].getnewaddress(): 999.9999})
        rawtx = self.nodes[0].signrawtransactionwithwallet(rawtx)
        self.nodes[0].sendrawtransaction(rawtx["hex"])
        self.nodes[0].generate(1)
        self.sync_all()
        self.mninfo.remove(mn)

        self.log.info("Removed masternode %d", idx)

    def prepare_datadirs(self):
        # stop faucet node so that we can copy the datadir
        self.stop_node(0)

        start_idx = len(self.nodes)
        for idx in range(0, self.mn_count):
            copy_datadir(0, idx + start_idx, self.options.tmpdir, self.chain)

        # restart faucet node
        self.start_node(0)
        force_finish_mnsync(self.nodes[0])

    def start_masternodes(self):
        self.log.info("Starting %d masternodes", self.mn_count)

        start_idx = len(self.nodes)

        self.add_nodes(self.mn_count)
        executor = ThreadPoolExecutor(max_workers=20)

        def do_connect(idx):
            # Connect to the control node only, masternodes should take care of intra-quorum connections themselves
            self.connect_nodes(self.mninfo[idx].nodeIdx, 0)

        jobs = []

        # start up nodes in parallel
        for idx in range(0, self.mn_count):
            self.mninfo[idx].nodeIdx = idx + start_idx
            jobs.append(executor.submit(self.start_masternode, self.mninfo[idx]))

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

        executor.shutdown()

    def start_masternode(self, mninfo, extra_args=None):
        args = ['-masternodeblsprivkey=%s' % mninfo.keyOperator] + self.extra_args[mninfo.nodeIdx]
        if extra_args is not None:
            args += extra_args
        self.start_node(mninfo.nodeIdx, extra_args=args)
        mninfo.node = self.nodes[mninfo.nodeIdx]
        force_finish_mnsync(mninfo.node)

    def dynamically_start_masternode(self, mnidx, extra_args=None):
        args = []
        if extra_args is not None:
            args += extra_args
        self.start_node(mnidx, extra_args=args)
        force_finish_mnsync(self.nodes[mnidx])

    def setup_network(self):
        self.log.info("Creating and starting controller node")
        self.add_nodes(1, extra_args=[self.extra_args[0]])
        self.start_node(0)
        self.import_deterministic_coinbase_privkeys()
        required_balance = HIGHPERFORMANCE_MASTERNODE_COLLATERAL * self.hpmn_count
        required_balance += MASTERNODE_COLLATERAL * (self.mn_count - self.hpmn_count) + 100
        self.log.info("Generating %d coins" % required_balance)
        while self.nodes[0].getbalance() < required_balance:
            self.bump_mocktime(1)
            self.nodes[0].generate(10)
        num_simple_nodes = self.num_nodes - self.mn_count - 1
        self.log.info("Creating and starting %s simple nodes", num_simple_nodes)
        for i in range(0, num_simple_nodes):
            self.create_simple_node()

        self.log.info("Activating DIP3")
        if not self.fast_dip3_enforcement:
            while self.nodes[0].getblockcount() < 500:
                self.nodes[0].generate(10)
        self.sync_all()

        # create masternodes
        self.prepare_masternodes()
        self.prepare_datadirs()
        self.start_masternodes()

        # non-masternodes where disconnected from the control node during prepare_datadirs,
        # let's reconnect them back to make sure they receive updates
        for i in range(0, num_simple_nodes):
            self.connect_nodes(i+1, 0)

        self.bump_mocktime(1)
        self.nodes[0].generate(1)
        # sync nodes
        self.sync_all()
        for i in range(0, num_simple_nodes):
            force_finish_mnsync(self.nodes[i + 1])

        # Enable InstantSend (including block filtering) and ChainLocks by default
        self.nodes[0].sporkupdate("SPORK_2_INSTANTSEND_ENABLED", 0)
        self.nodes[0].sporkupdate("SPORK_3_INSTANTSEND_BLOCK_FILTERING", 0)
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()
        self.bump_mocktime(1)

        mn_info = self.nodes[0].masternodelist("status")
        assert len(mn_info) == self.mn_count
        for status in mn_info.values():
            assert status == 'ENABLED'

    def create_raw_tx(self, node_from, node_to, amount, min_inputs, max_inputs):
        assert min_inputs <= max_inputs
        # fill inputs
        fee = 0.001
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
            elif in_amount >= amount + fee:
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

        assert len(inputs) >= min_inputs
        assert len(inputs) <= max_inputs
        assert in_amount >= amount + fee
        # fill outputs
        outputs = make_change(node_from, satoshi_round(in_amount), satoshi_round(amount), satoshi_round(fee))
        receiver_address = node_to.getnewaddress()
        outputs[receiver_address] = satoshi_round(amount)
        rawtx = node_from.createrawtransaction(inputs, outputs)
        ret = node_from.signrawtransactionwithwallet(rawtx)
        decoded = node_from.decoderawtransaction(ret['hex'])
        ret = {**decoded, **ret}
        return ret

    def wait_for_tx(self, txid, node, expected=True, timeout=60):
        def check_tx():
            try:
                self.bump_mocktime(1)
                return node.getrawtransaction(txid)
            except:
                return False
        if wait_until(check_tx, timeout=timeout, sleep=1, do_assert=expected) and not expected:
            raise AssertionError("waiting unexpectedly succeeded")

    def create_islock(self, hextx, deterministic=False):
        tx = FromHex(CTransaction(), hextx)
        tx.rehash()

        request_id_buf = ser_string(b"islock") + ser_compact_size(len(tx.vin))
        inputs = []
        for txin in tx.vin:
            request_id_buf += txin.prevout.serialize()
            inputs.append(txin.prevout)
        request_id = hash256(request_id_buf)[::-1].hex()
        message_hash = tx.hash

        llmq_type = 103 if deterministic else 104

        rec_sig = self.get_recovered_sig(request_id, message_hash, llmq_type=llmq_type)

        if deterministic:
            block_count = self.mninfo[0].node.getblockcount()
            cycle_hash = int(self.mninfo[0].node.getblockhash(block_count - (block_count % 24)), 16)
            islock = msg_isdlock(1, inputs, tx.sha256, cycle_hash, hex_str_to_bytes(rec_sig['sig']))
        else:
            islock = msg_islock(inputs, tx.sha256, hex_str_to_bytes(rec_sig['sig']))

        return islock

    def wait_for_instantlock(self, txid, node, expected=True, timeout=60):
        def check_instantlock():
            self.bump_mocktime(1)
            try:
                return node.getrawtransaction(txid, True)["instantlock"]
            except:
                return False
        if wait_until(check_instantlock, timeout=timeout, sleep=1, do_assert=expected) and not expected:
            raise AssertionError("waiting unexpectedly succeeded")

    def wait_for_chainlocked_block(self, node, block_hash, expected=True, timeout=15):
        def check_chainlocked_block():
            try:
                block = node.getblock(block_hash)
                return block["confirmations"] > 0 and block["chainlock"]
            except:
                return False
        if wait_until(check_chainlocked_block, timeout=timeout, sleep=0.1, do_assert=expected) and not expected:
            raise AssertionError("waiting unexpectedly succeeded")

    def wait_for_chainlocked_block_all_nodes(self, block_hash, timeout=15, expected=True):
        for node in self.nodes:
            self.wait_for_chainlocked_block(node, block_hash, expected=expected, timeout=timeout)

    def wait_for_best_chainlock(self, node, block_hash, timeout=15):
        wait_until(lambda: node.getbestchainlock()["blockhash"] == block_hash, timeout=timeout, sleep=0.1)

    def wait_for_sporks_same(self, timeout=30):
        def check_sporks_same():
            self.bump_mocktime(1)
            sporks = self.nodes[0].spork('show')
            return all(node.spork('show') == sporks for node in self.nodes[1:])
        wait_until(check_sporks_same, timeout=timeout, sleep=0.5)

    def wait_for_quorum_connections(self, quorum_hash, expected_connections, mninfos, llmq_type_name="llmq_test", timeout = 60, wait_proc=None):
        def check_quorum_connections():
            def ret():
                if wait_proc is not None:
                    wait_proc()
                return False

            for mn in mninfos:
                s = mn.node.quorum("dkgstatus")
                for qs in s["session"]:
                    if qs["llmqType"] != llmq_type_name:
                        continue
                    if qs["status"]["quorumHash"] != quorum_hash:
                        continue
                    for qc in s["quorumConnections"]:
                        if "quorumConnections" not in qc:
                            continue
                        if qc["llmqType"] != llmq_type_name:
                            continue
                        if qc["quorumHash"] != quorum_hash:
                            continue
                        if len(qc["quorumConnections"]) == 0:
                            continue
                        cnt = 0
                        for c in qc["quorumConnections"]:
                            if c["connected"]:
                                cnt += 1
                        if cnt < expected_connections:
                            return ret()
                        return True
                    # a session with no matching connections - not ok
                    return ret()
                # a node with no sessions - ok
                pass
            # no sessions at all - not ok
            return ret()

        wait_until(check_quorum_connections, timeout=timeout, sleep=1)

    def wait_for_masternode_probes(self, quorum_hash, mninfos, timeout = 30, wait_proc=None, llmq_type_name="llmq_test"):
        def check_probes():
            def ret():
                if wait_proc is not None:
                    wait_proc()
                return False

            for mn in mninfos:
                s = mn.node.quorum('dkgstatus')
                for qs in s["session"]:
                    if qs["llmqType"] != llmq_type_name:
                        continue
                    if qs["status"]["quorumHash"] != quorum_hash:
                        continue
                    for qc in s["quorumConnections"]:
                        if qc["llmqType"] != llmq_type_name:
                            continue
                        if qc["quorumHash"] != quorum_hash:
                            continue
                        for c in qc["quorumConnections"]:
                            if c["proTxHash"] == mn.proTxHash:
                                continue
                            if not c["outbound"]:
                                mn2 = mn.node.protx('info', c["proTxHash"])
                                if [m for m in mninfos if c["proTxHash"] == m.proTxHash]:
                                    # MN is expected to be online and functioning, so let's verify that the last successful
                                    # probe is not too old. Probes are retried after 50 minutes, while DKGs consider a probe
                                    # as failed after 60 minutes
                                    if mn2['metaInfo']['lastOutboundSuccessElapsed'] > 55 * 60:
                                        return ret()
                                else:
                                    # MN is expected to be offline, so let's only check that the last probe is not too long ago
                                    if mn2['metaInfo']['lastOutboundAttemptElapsed'] > 55 * 60 and mn2['metaInfo']['lastOutboundSuccessElapsed'] > 55 * 60:
                                        return ret()
            return True

        wait_until(check_probes, timeout=timeout, sleep=1)

    def wait_for_quorum_phase(self, quorum_hash, phase, expected_member_count, check_received_messages, check_received_messages_count, mninfos, llmq_type_name="llmq_test", timeout=30, sleep=0.5):
        def check_dkg_session():
            member_count = 0
            for mn in mninfos:
                s = mn.node.quorum("dkgstatus")["session"]
                for qs in s:
                    if qs["llmqType"] != llmq_type_name:
                        continue
                    qstatus = qs["status"]
                    if qstatus["quorumHash"] != quorum_hash:
                        continue
                    if qstatus["phase"] != phase:
                        return False
                    if check_received_messages is not None:
                        if qstatus[check_received_messages] < check_received_messages_count:
                            return False
                    member_count += 1
                    break
            return member_count >= expected_member_count

        wait_until(check_dkg_session, timeout=timeout, sleep=sleep)

    def wait_for_quorum_commitment(self, quorum_hash, nodes, llmq_type=100, timeout=15):
        def check_dkg_comitments():
            for node in nodes:
                s = node.quorum("dkgstatus")
                if "minableCommitments" not in s:
                    return False
                commits = s["minableCommitments"]
                c_ok = False
                for c in commits:
                    if c["llmqType"] != llmq_type:
                        continue
                    if c["quorumHash"] != quorum_hash:
                        continue
                    c_ok = True
                    break
                if not c_ok:
                    return False
            return True

        wait_until(check_dkg_comitments, timeout=timeout, sleep=1)

    def wait_for_quorum_list(self, quorum_hash, nodes, timeout=15, sleep=2, llmq_type_name="llmq_test"):
        def wait_func():
            self.log.info("quorums: " + str(self.nodes[0].quorum("list")))
            if quorum_hash in self.nodes[0].quorum("list")[llmq_type_name]:
                return True
            self.bump_mocktime(sleep, nodes=nodes)
            self.nodes[0].generate(1)
            self.sync_blocks(nodes)
            return False
        wait_until(wait_func, timeout=timeout, sleep=sleep)

    def wait_for_quorums_list(self, quorum_hash_0, quorum_hash_1, nodes, llmq_type_name="llmq_test",  timeout=15, sleep=2):
        def wait_func():
            self.log.info("h("+str(self.nodes[0].getblockcount())+") quorums: " + str(self.nodes[0].quorum("list")))
            if quorum_hash_0 in self.nodes[0].quorum("list")[llmq_type_name]:
                if quorum_hash_1 in self.nodes[0].quorum("list")[llmq_type_name]:
                    return True
            self.bump_mocktime(sleep, nodes=nodes)
            self.nodes[0].generate(1)
            self.sync_blocks(nodes)
            return False
        wait_until(wait_func, timeout=timeout, sleep=sleep)

    def move_blocks(self, nodes, num_blocks):
        time.sleep(1)
        self.bump_mocktime(1, nodes=nodes)
        self.nodes[0].generate(num_blocks)
        self.sync_blocks(nodes)

    def mine_quorum(self, llmq_type_name="llmq_test", llmq_type=100, expected_connections=None, expected_members=None, expected_contributions=None, expected_complaints=0, expected_justifications=0, expected_commitments=None, mninfos_online=None, mninfos_valid=None):
        spork21_active = self.nodes[0].spork('show')['SPORK_21_QUORUM_ALL_CONNECTED'] <= 1
        spork23_active = self.nodes[0].spork('show')['SPORK_23_QUORUM_POSE'] <= 1

        if expected_connections is None:
            expected_connections = (self.llmq_size - 1) if spork21_active else 2
        if expected_members is None:
            expected_members = self.llmq_size
        if expected_contributions is None:
            expected_contributions = self.llmq_size
        if expected_commitments is None:
            expected_commitments = self.llmq_size
        if mninfos_online is None:
            mninfos_online = self.mninfo.copy()
        if mninfos_valid is None:
            mninfos_valid = self.mninfo.copy()

        self.log.info("Mining quorum: llmq_type_name=%s, llmq_type=%d, expected_members=%d, expected_connections=%d, expected_contributions=%d, expected_complaints=%d, expected_justifications=%d, "
                      "expected_commitments=%d" % (llmq_type_name, llmq_type, expected_members, expected_connections, expected_contributions, expected_complaints,
                                                   expected_justifications, expected_commitments))

        nodes = [self.nodes[0]] + [mn.node for mn in mninfos_online]

        # move forward to next DKG
        skip_count = 24 - (self.nodes[0].getblockcount() % 24)
        if skip_count != 0:
            self.bump_mocktime(1, nodes=nodes)
            self.nodes[0].generate(skip_count)
        self.sync_blocks(nodes)

        q = self.nodes[0].getbestblockhash()
        self.log.info("Expected quorum_hash:"+str(q))
        self.log.info("Waiting for phase 1 (init)")
        self.wait_for_quorum_phase(q, 1, expected_members, None, 0, mninfos_online, llmq_type_name=llmq_type_name)
        self.wait_for_quorum_connections(q, expected_connections, mninfos_online, wait_proc=lambda: self.bump_mocktime(1, nodes=nodes), llmq_type_name=llmq_type_name)
        if spork23_active:
            self.wait_for_masternode_probes(q, mninfos_online, wait_proc=lambda: self.bump_mocktime(1, nodes=nodes))

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 2 (contribute)")
        self.wait_for_quorum_phase(q, 2, expected_members, "receivedContributions", expected_contributions, mninfos_online, llmq_type_name=llmq_type_name)

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 3 (complain)")
        self.wait_for_quorum_phase(q, 3, expected_members, "receivedComplaints", expected_complaints, mninfos_online, llmq_type_name=llmq_type_name)

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 4 (justify)")
        self.wait_for_quorum_phase(q, 4, expected_members, "receivedJustifications", expected_justifications, mninfos_online, llmq_type_name=llmq_type_name)

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 5 (commit)")
        self.wait_for_quorum_phase(q, 5, expected_members, "receivedPrematureCommitments", expected_commitments, mninfos_online, llmq_type_name=llmq_type_name)

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 6 (mining)")
        self.wait_for_quorum_phase(q, 6, expected_members, None, 0, mninfos_online, llmq_type_name=llmq_type_name)

        self.log.info("Waiting final commitment")
        self.wait_for_quorum_commitment(q, nodes, llmq_type=llmq_type)

        self.log.info("Mining final commitment")
        self.bump_mocktime(1, nodes=nodes)
        self.nodes[0].getblocktemplate() # this calls CreateNewBlock
        self.nodes[0].generate(1)
        self.sync_blocks(nodes)

        self.log.info("Waiting for quorum to appear in the list")
        self.wait_for_quorum_list(q, nodes, llmq_type_name=llmq_type_name)

        new_quorum = self.nodes[0].quorum("list", 1)[llmq_type_name][0]
        assert_equal(q, new_quorum)
        quorum_info = self.nodes[0].quorum("info", llmq_type, new_quorum)

        # Mine 8 (SIGN_HEIGHT_OFFSET) more blocks to make sure that the new quorum gets eligible for signing sessions
        self.nodes[0].generate(8)

        self.sync_blocks(nodes)

        self.log.info("New quorum: height=%d, quorumHash=%s, quorumIndex=%d, minedBlock=%s" % (quorum_info["height"], new_quorum, quorum_info["quorumIndex"], quorum_info["minedBlock"]))

        return new_quorum

    def mine_cycle_quorum(self, llmq_type_name="llmq_test_dip0024", llmq_type=103,  expected_connections=None, expected_members=None, expected_contributions=None, expected_complaints=0, expected_justifications=0, expected_commitments=None, mninfos_online=None, mninfos_valid=None):
        spork21_active = self.nodes[0].spork('show')['SPORK_21_QUORUM_ALL_CONNECTED'] <= 1
        spork23_active = self.nodes[0].spork('show')['SPORK_23_QUORUM_POSE'] <= 1

        if expected_connections is None:
            expected_connections = (self.llmq_size - 1) if spork21_active else 2
        if expected_members is None:
            expected_members = self.llmq_size
        if expected_contributions is None:
            expected_contributions = self.llmq_size
        if expected_commitments is None:
            expected_commitments = self.llmq_size
        if mninfos_online is None:
            mninfos_online = self.mninfo.copy()
        if mninfos_valid is None:
            mninfos_valid = self.mninfo.copy()

        self.log.info("Mining quorum: expected_members=%d, expected_connections=%d, expected_contributions=%d, expected_complaints=%d, expected_justifications=%d, "
                      "expected_commitments=%d" % (expected_members, expected_connections, expected_contributions, expected_complaints,
                                                   expected_justifications, expected_commitments))

        nodes = [self.nodes[0]] + [mn.node for mn in mninfos_online]

        # move forward to next DKG
        skip_count = 24 - (self.nodes[0].getblockcount() % 24)

        # if skip_count != 0:
        #     self.bump_mocktime(1, nodes=nodes)
        #     self.nodes[0].generate(skip_count)
        #     time.sleep(4)
        # self.sync_blocks(nodes)

        self.move_blocks(nodes, skip_count)

        q_0 = self.nodes[0].getbestblockhash()
        self.log.info("Expected quorum_0 at:" + str(self.nodes[0].getblockcount()))
        # time.sleep(4)
        self.log.info("Expected quorum_0 hash:" + str(q_0))
        # time.sleep(4)
        self.log.info("quorumIndex 0: Waiting for phase 1 (init)")
        self.wait_for_quorum_phase(q_0, 1, expected_members, None, 0, mninfos_online, llmq_type_name)
        self.log.info("quorumIndex 0: Waiting for quorum connections (init)")
        self.wait_for_quorum_connections(q_0, expected_connections, mninfos_online, llmq_type_name, wait_proc=lambda: self.bump_mocktime(1, nodes=nodes))
        if spork23_active:
            self.wait_for_masternode_probes(q_0, mninfos_online, wait_proc=lambda: self.bump_mocktime(1, nodes=nodes), llmq_type_name=llmq_type_name)

        self.move_blocks(nodes, 1)

        q_1 = self.nodes[0].getbestblockhash()
        self.log.info("Expected quorum_1 at:" + str(self.nodes[0].getblockcount()))
        # time.sleep(2)
        self.log.info("Expected quorum_1 hash:" + str(q_1))
        # time.sleep(2)
        self.log.info("quorumIndex 1: Waiting for phase 1 (init)")
        self.wait_for_quorum_phase(q_1, 1, expected_members, None, 0, mninfos_online, llmq_type_name)
        self.log.info("quorumIndex 1: Waiting for quorum connections (init)")
        self.wait_for_quorum_connections(q_1, expected_connections, mninfos_online, llmq_type_name, wait_proc=lambda: self.bump_mocktime(1, nodes=nodes))
        if spork23_active:
            self.wait_for_masternode_probes(q_1, mninfos_online, wait_proc=lambda: self.bump_mocktime(1, nodes=nodes), llmq_type_name=llmq_type_name)

        self.move_blocks(nodes, 1)

        self.log.info("quorumIndex 0: Waiting for phase 2 (contribute)")
        self.wait_for_quorum_phase(q_0, 2, expected_members, "receivedContributions", expected_contributions, mninfos_online, llmq_type_name)

        self.move_blocks(nodes, 1)

        self.log.info("quorumIndex 1: Waiting for phase 2 (contribute)")
        self.wait_for_quorum_phase(q_1, 2, expected_members, "receivedContributions", expected_contributions, mninfos_online, llmq_type_name)

        self.move_blocks(nodes, 1)

        self.log.info("quorumIndex 0: Waiting for phase 3 (complain)")
        self.wait_for_quorum_phase(q_0, 3, expected_members, "receivedComplaints", expected_complaints, mninfos_online, llmq_type_name)

        self.move_blocks(nodes, 1)

        self.log.info("quorumIndex 1: Waiting for phase 3 (complain)")
        self.wait_for_quorum_phase(q_1, 3, expected_members, "receivedComplaints", expected_complaints, mninfos_online, llmq_type_name)

        self.move_blocks(nodes, 1)

        self.log.info("quorumIndex 0: Waiting for phase 4 (justify)")
        self.wait_for_quorum_phase(q_0, 4, expected_members, "receivedJustifications", expected_justifications, mninfos_online, llmq_type_name)

        self.move_blocks(nodes, 1)

        self.log.info("quorumIndex 1: Waiting for phase 4 (justify)")
        self.wait_for_quorum_phase(q_1, 4, expected_members, "receivedJustifications", expected_justifications, mninfos_online, llmq_type_name)

        self.move_blocks(nodes, 1)

        self.log.info("quorumIndex 0: Waiting for phase 5 (commit)")
        self.wait_for_quorum_phase(q_0, 5, expected_members, "receivedPrematureCommitments", expected_commitments, mninfos_online, llmq_type_name)

        self.move_blocks(nodes, 1)

        self.log.info("quorumIndex 1: Waiting for phase 5 (commit)")
        self.wait_for_quorum_phase(q_1, 5, expected_members, "receivedPrematureCommitments", expected_commitments, mninfos_online, llmq_type_name)

        self.move_blocks(nodes, 1)

        self.log.info("quorumIndex 0: Waiting for phase 6 (finalization)")
        self.wait_for_quorum_phase(q_0, 6, expected_members, None, 0, mninfos_online, llmq_type_name)

        self.move_blocks(nodes, 1)

        self.log.info("quorumIndex 1: Waiting for phase 6 (finalization)")
        self.wait_for_quorum_phase(q_1, 6, expected_members, None, 0, mninfos_online, llmq_type_name)
        time.sleep(6)
        self.log.info("Mining final commitments")
        self.bump_mocktime(1, nodes=nodes)
        self.nodes[0].getblocktemplate() # this calls CreateNewBlock
        self.nodes[0].generate(1)
        self.sync_blocks(nodes)

        time.sleep(6)
        self.log.info("Waiting for quorum(s) to appear in the list")
        self.wait_for_quorums_list(q_0, q_1, nodes, llmq_type_name)

        quorum_info_0 = self.nodes[0].quorum("info", llmq_type, q_0)
        quorum_info_1 = self.nodes[0].quorum("info", llmq_type, q_1)
        # Mine 8 (SIGN_HEIGHT_OFFSET) more blocks to make sure that the new quorum gets eligible for signing sessions
        self.nodes[0].generate(8)

        self.sync_blocks(nodes)
        self.log.info("New quorum: height=%d, quorumHash=%s, quorumIndex=%d, minedBlock=%s" % (quorum_info_0["height"], q_0, quorum_info_0["quorumIndex"], quorum_info_0["minedBlock"]))
        self.log.info("New quorum: height=%d, quorumHash=%s, quorumIndex=%d, minedBlock=%s" % (quorum_info_1["height"], q_1, quorum_info_1["quorumIndex"], quorum_info_1["minedBlock"]))

        self.log.info("quorum_info_0:"+str(quorum_info_0))
        self.log.info("quorum_info_1:"+str(quorum_info_1))

        best_block_hash = self.nodes[0].getbestblockhash()
        block_height = self.nodes[0].getblockcount()
        quorum_rotation_info = self.nodes[0].quorum("rotationinfo", best_block_hash)
        self.log.info("h("+str(block_height)+"):"+str(quorum_rotation_info))

        return (quorum_info_0, quorum_info_1)

    def move_to_next_cycle(self):
        cycle_length = 24
        mninfos_online = self.mninfo.copy()
        nodes = [self.nodes[0]] + [mn.node for mn in mninfos_online]
        cur_block = self.nodes[0].getblockcount()

        # move forward to next DKG
        skip_count = cycle_length - (cur_block % cycle_length)
        if skip_count != 0:
            self.bump_mocktime(1, nodes=nodes)
            self.nodes[0].generate(skip_count)
        self.sync_blocks(nodes)
        time.sleep(1)
        self.log.info('Moved from block %d to %d' % (cur_block, self.nodes[0].getblockcount()))

    def wait_for_recovered_sig(self, rec_sig_id, rec_sig_msg_hash, llmq_type=100, timeout=10):
        def check_recovered_sig():
            self.bump_mocktime(1)
            for mn in self.mninfo:
                if not mn.node.quorum("hasrecsig", llmq_type, rec_sig_id, rec_sig_msg_hash):
                    return False
            return True
        wait_until(check_recovered_sig, timeout=timeout, sleep=1)

    def get_recovered_sig(self, rec_sig_id, rec_sig_msg_hash, llmq_type=100):
        # Note: recsigs aren't relayed to regular nodes by default,
        # make sure to pick a mn as a node to query for recsigs.
        try:
            quorumHash = self.mninfo[0].node.quorum("selectquorum", llmq_type, rec_sig_id)["quorumHash"]
            for mn in self.mninfo:
                mn.node.quorum("sign", llmq_type, rec_sig_id, rec_sig_msg_hash, quorumHash)
            self.wait_for_recovered_sig(rec_sig_id, rec_sig_msg_hash, llmq_type, 10)
            return self.mninfo[0].node.quorum("getrecsig", llmq_type, rec_sig_id, rec_sig_msg_hash)
        except JSONRPCException as e:
            self.log.info(f"getrecsig failed with '{e}'")
        assert False

    def get_quorum_masternodes(self, q, llmq_type=100):
        qi = self.nodes[0].quorum('info', llmq_type, q)
        result = []
        for m in qi['members']:
            result.append(self.get_mninfo(m['proTxHash']))
        return result

    def get_mninfo(self, proTxHash):
        for mn in self.mninfo:
            if mn.proTxHash == proTxHash:
                return mn
        return None

    def test_mn_quorum_data(self, test_mn, quorum_type_in, quorum_hash_in, test_secret=True, expect_secret=True):
        quorum_info = test_mn.node.quorum("info", quorum_type_in, quorum_hash_in, True)
        if test_secret and expect_secret != ("secretKeyShare" in quorum_info):
            return False
        if "members" not in quorum_info or len(quorum_info["members"]) == 0:
            return False
        pubkey_count = 0
        valid_count = 0
        for quorum_member in quorum_info["members"]:
            valid_count += quorum_member["valid"]
            pubkey_count += "pubKeyShare" in quorum_member
        return pubkey_count == valid_count

    def wait_for_quorum_data(self, mns, quorum_type_in, quorum_hash_in, test_secret=True, expect_secret=True,
                             recover=False, timeout=60):
        def test_mns():
            valid = 0
            if recover:
                if self.mocktime % 2:
                    self.bump_mocktime(self.quorum_data_request_expiration_timeout + 1)
                    self.nodes[0].generate(1)
                    self.sync_blocks()
                else:
                    self.bump_mocktime(self.quorum_data_thread_request_timeout_seconds + 1)

            for test_mn in mns:
                valid += self.test_mn_quorum_data(test_mn, quorum_type_in, quorum_hash_in, test_secret, expect_secret)
            self.log.debug("wait_for_quorum_data: %d/%d - quorum_type=%d quorum_hash=%s" %
                           (valid, len(mns), quorum_type_in, quorum_hash_in))
            return valid == len(mns)

        wait_until(test_mns, timeout=timeout, sleep=0.5)

    def wait_for_mnauth(self, node, count, timeout=10):
        def test():
            pi = node.getpeerinfo()
            c = 0
            for p in pi:
                if "verified_proregtx_hash" in p and p["verified_proregtx_hash"] != "":
                    c += 1
            return c >= count
        wait_until(test, timeout=timeout)
