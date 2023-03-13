#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Base class for RPC testing."""

import configparser
from enum import Enum
import argparse
import logging
import os
import platform
import pdb
import random
import re
import shutil
import subprocess
import sys
import tempfile
import time
import copy

from typing import List
from .address import create_deterministic_address_bcrt1_p2tr_op_true
from .authproxy import JSONRPCException
from . import coverage
from .p2p import NetworkThread
from .test_node import TestNode
from .util import (
    MAX_NODES,
    MASTERNODE_COLLATERAL,
    PortSeed,
    assert_equal,
    check_json_precision,
    get_datadir_path,
    initialize_datadir,
    set_node_times,
    p2p_port,
    copy_datadir,
    force_finish_mnsync,
    wait_until_helper,
    bump_node_times,
    satoshi_round,
)


class TestStatus(Enum):
    PASSED = 1
    FAILED = 2
    SKIPPED = 3

TEST_EXIT_PASSED = 0
TEST_EXIT_FAILED = 1
TEST_EXIT_SKIPPED = 77

TMPDIR_PREFIX = "syscoin_func_test_"

class SkipTest(Exception):
    """This exception is raised to skip a test"""

    def __init__(self, message):
        self.message = message


class SyscoinTestMetaClass(type):
    """Metaclass for SyscoinTestFramework.

    Ensures that any attempt to register a subclass of `SyscoinTestFramework`
    adheres to a standard whereby the subclass overrides `set_test_params` and
    `run_test` but DOES NOT override either `__init__` or `main`. If any of
    those standards are violated, a ``TypeError`` is raised."""

    def __new__(cls, clsname, bases, dct):
        if not clsname == 'SyscoinTestFramework':
            if not ('run_test' in dct and 'set_test_params' in dct):
                raise TypeError("SyscoinTestFramework subclasses must override "
                                "'run_test' and 'set_test_params'")
            if '__init__' in dct or 'main' in dct:
                raise TypeError("SyscoinTestFramework subclasses may not override "
                                "'__init__' or 'main'")

        return super().__new__(cls, clsname, bases, dct)


class SyscoinTestFramework(metaclass=SyscoinTestMetaClass):
    """Base class for a syscoin test script.

    Individual syscoin test scripts should subclass this class and override the set_test_params() and run_test() methods.

    Individual tests can also override the following methods to customize the test setup:

    - add_options()
    - setup_chain()
    - setup_network()
    - setup_nodes()

    The __init__() and main() methods should not be overridden.

    This class also contains various public and private helper methods."""

    def __init__(self):
        """Sets test framework defaults. Do not override this method. Instead, override the set_test_params() method"""
        self.chain: str = 'regtest'
        self.setup_clean_chain: bool = False
        self.nodes: List[TestNode] = []
        self.extra_args = None
        self.network_thread = None
        self.rpc_timeout = 60  # Wait for up to 60 seconds for the RPC server to respond
        self.supports_cli = True
        self.bind_to_localhost_only = True
        self.parse_args()
        self.disable_syscall_sandbox = self.options.nosandbox or self.options.valgrind
        self.default_wallet_name = "default_wallet" if self.options.descriptors else ""
        self.wallet_data_filename = "wallet.dat"
        # Optional list of wallet names that can be set in set_test_params to
        # create and import keys to. If unset, default is len(nodes) *
        # [default_wallet_name]. If wallet names are None, wallet creation is
        # skipped. If list is truncated, wallet creation is skipped and keys
        # are not imported.
        self.wallet_names = None
        # SYSCOIN
        self.mocktime = None
        # By default the wallet is not required. Set to true by skip_if_no_wallet().
        # When False, we ignore wallet_names regardless of what it is.
        self._requires_wallet = False
        # Disable ThreadOpenConnections by default, so that adding entries to
        # addrman will not result in automatic connections to them.
        self.disable_autoconnect = True
        self.set_test_params()
        assert self.wallet_names is None or len(self.wallet_names) <= self.num_nodes
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
        previous_releases_path = os.getenv("PREVIOUS_RELEASES_DIR") or os.getcwd() + "/releases"
        parser = argparse.ArgumentParser(usage="%(prog)s [options]")
        parser.add_argument("--nocleanup", dest="nocleanup", default=False, action="store_true",
                            help="Leave syscoinds and test.* datadir on exit or error")
        parser.add_argument("--nosandbox", dest="nosandbox", default=False, action="store_true",
                            help="Don't use the syscall sandbox")
        parser.add_argument("--noshutdown", dest="noshutdown", default=False, action="store_true",
                            help="Don't stop syscoinds after the test execution")
        parser.add_argument("--cachedir", dest="cachedir", default=os.path.abspath(os.path.dirname(os.path.realpath(__file__)) + "/../../cache"),
                            help="Directory for caching pregenerated datadirs (default: %(default)s)")
        parser.add_argument("--tmpdir", dest="tmpdir", help="Root directory for datadirs")
        parser.add_argument("-l", "--loglevel", dest="loglevel", default="INFO",
                            help="log events at this level and higher to the console. Can be set to DEBUG, INFO, WARNING, ERROR or CRITICAL. Passing --loglevel DEBUG will output all logs to console. Note that logs at all levels are always written to the test_framework.log file in the temporary test directory.")
        parser.add_argument("--tracerpc", dest="trace_rpc", default=False, action="store_true",
                            help="Print out all RPC calls as they are made")
        parser.add_argument("--portseed", dest="port_seed", default=os.getpid(), type=int,
                            help="The seed to use for assigning port numbers (default: current process id)")
        parser.add_argument("--previous-releases", dest="prev_releases", action="store_true",
                            default=os.path.isdir(previous_releases_path) and bool(os.listdir(previous_releases_path)),
                            help="Force test of previous releases (default: %(default)s)")
        parser.add_argument("--coveragedir", dest="coveragedir",
                            help="Write tested RPC commands into this directory")
        parser.add_argument("--configfile", dest="configfile",
                            default=os.path.abspath(os.path.dirname(os.path.realpath(__file__)) + "/../../config.ini"),
                            help="Location of the test framework config file (default: %(default)s)")
        parser.add_argument("--pdbonfailure", dest="pdbonfailure", default=False, action="store_true",
                            help="Attach a python debugger if test fails")
        parser.add_argument("--usecli", dest="usecli", default=False, action="store_true",
                            help="use syscoin-cli instead of RPC for all commands")
        parser.add_argument("--perf", dest="perf", default=False, action="store_true",
                            help="profile running nodes with perf for the duration of the test")
        parser.add_argument("--valgrind", dest="valgrind", default=False, action="store_true",
                            help="run nodes under the valgrind memory error detector: expect at least a ~10x slowdown. valgrind 3.14 or later required. Forces --nosandbox.")
        parser.add_argument("--randomseed", type=int,
                            help="set a random seed for deterministically reproducing a previous test run")
        parser.add_argument("--timeout-factor", dest="timeout_factor", type=float, help="adjust test timeouts by a factor. Setting it to 0 disables all timeouts")

        self.add_options(parser)
        # Running TestShell in a Jupyter notebook causes an additional -f argument
        # To keep TestShell from failing with an "unrecognized argument" error, we add a dummy "-f" argument
        # source: https://stackoverflow.com/questions/48796169/how-to-fix-ipykernel-launcher-py-error-unrecognized-arguments-in-jupyter/56349168#56349168
        parser.add_argument("-f", "--fff", help="a dummy argument to fool ipython", default="1")
        self.options = parser.parse_args()
        if self.options.timeout_factor == 0:
            self.options.timeout_factor = 99999
        self.options.timeout_factor = self.options.timeout_factor or (4 if self.options.valgrind else 1)
        self.options.previous_releases_path = previous_releases_path

        config = configparser.ConfigParser()
        config.read_file(open(self.options.configfile))
        self.config = config

        if "descriptors" not in self.options:
            # Wallet is not required by the test at all and the value of self.options.descriptors won't matter.
            # It still needs to exist and be None in order for tests to work however.
            # So set it to None to force -disablewallet, because the wallet is not needed.
            self.options.descriptors = None
        elif self.options.descriptors is None:
            # Some wallet is either required or optionally used by the test.
            # Prefer SQLite unless it isn't available
            if self.is_sqlite_compiled():
                self.options.descriptors = True
            elif self.is_bdb_compiled():
                self.options.descriptors = False
            else:
                # If neither are compiled, tests requiring a wallet will be skipped and the value of self.options.descriptors won't matter
                # It still needs to exist and be None in order for tests to work however.
                # So set it to None, which will also set -disablewallet.
                self.options.descriptors = None

        PortSeed.n = self.options.port_seed

    def setup(self):
        """Call this method to start up the test framework object with options set."""

        check_json_precision()

        self.options.cachedir = os.path.abspath(self.options.cachedir)

        config = self.config

        fname_syscoind = os.path.join(
            config["environment"]["BUILDDIR"],
            "src",
            "syscoind" + config["environment"]["EXEEXT"],
        )
        fname_syscoincli = os.path.join(
            config["environment"]["BUILDDIR"],
            "src",
            "syscoin-cli" + config["environment"]["EXEEXT"],
        )
        fname_syscoinutil = os.path.join(
            config["environment"]["BUILDDIR"],
            "src",
            "syscoin-util" + config["environment"]["EXEEXT"],
        )
        self.options.syscoind = os.getenv("SYSCOIND", default=fname_syscoind)
        self.options.syscoincli = os.getenv("SYSCOINCLI", default=fname_syscoincli)
        self.options.syscoinutil = os.getenv("SYSCOINUTIL", default=fname_syscoinutil)

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
            self.log.info("User supplied random seed {}".format(seed))

        random.seed(seed)
        self.log.info("PRNG seed is: {}".format(seed))

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
            if self.nodes:
                self.stop_nodes()
        else:
            for node in self.nodes:
                node.cleanup_on_exit = False
            self.log.info("Note: syscoinds were not stopped and may still be running")

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
        rpc_logger = logging.getLogger("SyscoinRPC")
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
        else:
            self._initialize_chain()

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
        self.add_nodes(self.num_nodes, self.extra_args)
        self.start_nodes()
        if self._requires_wallet:
            self.import_deterministic_coinbase_privkeys()
        if not self.setup_clean_chain:
            for n in self.nodes:
                assert_equal(n.getblockchaininfo()["blocks"], 199)
            # To ensure that all nodes are out of IBD, the most recent block
            # must have a timestamp not too old (see IsInitialBlockDownload()).
            self.log.debug('Generate a block with current time')
            block_hash = self.generate(self.nodes[0], 1, sync_fun=self.no_op)[0]
            block = self.nodes[0].getblock(blockhash=block_hash, verbosity=0)
            for n in self.nodes:
                n.submitblock(block)
                chain_info = n.getblockchaininfo()
                assert_equal(chain_info["blocks"], 200)
                assert_equal(chain_info["initialblockdownload"], False)

    def import_deterministic_coinbase_privkeys(self):
        for i in range(self.num_nodes):
            self.init_wallet(node=i)

    def init_wallet(self, *, node):
        wallet_name = self.default_wallet_name if self.wallet_names is None else self.wallet_names[node] if node < len(self.wallet_names) else False
        if wallet_name is not False:
            n = self.nodes[node]
            if wallet_name is not None:
                n.createwallet(wallet_name=wallet_name, descriptors=self.options.descriptors, load_on_startup=True)
            n.importprivkey(privkey=n.get_deterministic_priv_key().key, label='coinbase', rescan=True)

    def run_test(self):
        """Tests must override this method to define test logic"""
        raise NotImplementedError

    # Public helper methods. These can be accessed by the subclass test scripts.

    def add_wallet_options(self, parser, *, descriptors=True, legacy=True):
        kwargs = {}
        if descriptors + legacy == 1:
            # If only one type can be chosen, set it as default
            kwargs["default"] = descriptors
        group = parser.add_mutually_exclusive_group(
            # If only one type is allowed, require it to be set in test_runner.py
            required=os.getenv("REQUIRE_WALLET_TYPE_SET") == "1" and "default" in kwargs)
        if descriptors:
            group.add_argument("--descriptors", action='store_const', const=True, **kwargs,
                               help="Run test using a descriptor wallet", dest='descriptors')
        if legacy:
            group.add_argument("--legacy-wallet", action='store_const', const=False, **kwargs,
                               help="Run test using legacy wallets", dest='descriptors')
    # SYSCOIN add offset
    def add_nodes(self, num_nodes: int, extra_args=None, *, rpchost=None, binary=None, binary_cli=None, versions=None, offset = None):
        """Instantiate TestNode objects.

        Should only be called once after the nodes have been specified in
        set_test_params()."""
        def get_bin_from_version(version, bin_name, bin_default):
            if not version:
                return bin_default
            return os.path.join(
                self.options.previous_releases_path,
                re.sub(
                    r'\.0$',
                    '',  # remove trailing .0 for point releases
                    'v{}.{}.{}.{}'.format(
                        (version % 100000000) // 1000000,
                        (version % 1000000) // 10000,
                        (version % 10000) // 100,
                        (version % 100) // 1,
                    ),
                ),
                'bin',
                bin_name,
            )

        if self.bind_to_localhost_only:
            extra_confs = [["bind=127.0.0.1"]] * num_nodes
        else:
            extra_confs = [[]] * num_nodes
        if extra_args is None:
            extra_args = [[]] * num_nodes
        if versions is None:
            versions = [None] * num_nodes
        if self.is_syscall_sandbox_compiled() and not self.disable_syscall_sandbox:
            for i in range(len(extra_args)):
                if versions[i] is None or versions[i] >= 219900:
                    extra_args[i] = extra_args[i] + ["-sandbox=log-and-abort"]
        if binary is None:
            binary = [get_bin_from_version(v, 'syscoind', self.options.syscoind) for v in versions]
        if binary_cli is None:
            binary_cli = [get_bin_from_version(v, 'syscoin-cli', self.options.syscoincli) for v in versions]
        assert_equal(len(extra_confs), num_nodes)
        assert_equal(len(extra_args), num_nodes)
        assert_equal(len(versions), num_nodes)
        assert_equal(len(binary), num_nodes)
        assert_equal(len(binary_cli), num_nodes)
        offsetNum = 0
        if offset is not None:
            offsetNum = offset
        for i in range(num_nodes):
            index = i + offsetNum
            test_node_i = TestNode(
                index,
                get_datadir_path(self.options.tmpdir, index),
                chain=self.chain,
                rpchost=rpchost,
                timewait=self.rpc_timeout,
                timeout_factor=self.options.timeout_factor,
                syscoind=binary[i],
                syscoin_cli=binary_cli[i],
                version=versions[i],
                coverage_dir=self.options.coveragedir,
                cwd=self.options.tmpdir,
                extra_conf=extra_confs[i],
                extra_args=extra_args[i],
                use_cli=self.options.usecli,
                start_perf=self.options.perf,
                use_valgrind=self.options.valgrind,
                descriptors=self.options.descriptors,
            )
            self.nodes.append(test_node_i)
            if not test_node_i.version_is_at_least(170000):
                # adjust conf for pre 17
                test_node_i.replace_in_config([('[regtest]', '')])

    def start_node(self, i, *args, **kwargs):
        """Start a syscoind"""

        node = self.nodes[i]
        node.start(*args, **kwargs)
        node.wait_for_rpc_connection()

        if self.options.coveragedir is not None:
            coverage.write_all_rpc_commands(self.options.coveragedir, node.rpc)

    def start_nodes(self, extra_args=None, *args, **kwargs):
        """Start multiple syscoinds"""

        if extra_args is None:
            extra_args = [None] * self.num_nodes

        assert_equal(len(extra_args), self.num_nodes)
        try:
            for i, node in enumerate(self.nodes):
                node.start(extra_args[i], *args, **kwargs)
            for node in self.nodes:
                node.wait_for_rpc_connection()
        except Exception:
            # If one node failed to start, stop the others
            self.stop_nodes()
            raise

        if self.options.coveragedir is not None:
            for node in self.nodes:
                coverage.write_all_rpc_commands(self.options.coveragedir, node.rpc)

    def stop_node(self, i, expected_stderr='', wait=0):
        """Stop a syscoind test node"""
        self.nodes[i].stop_node(expected_stderr, wait=wait)

    def stop_nodes(self, wait=0):
        """Stop multiple syscoind test nodes"""
        for node in self.nodes:
            # Issue RPC to stop nodes
            node.stop_node(wait=wait, wait_until_stopped=False)

        for node in self.nodes:
            # Wait for nodes to stop
            node.wait_until_stopped()

    def restart_node(self, i, extra_args=None):
        """Stop and start a test node"""
        self.stop_node(i)
        self.start_node(i, extra_args)

    def wait_for_node_exit(self, i, timeout):
        self.nodes[i].process.wait(timeout)

    def connect_nodes(self, a, b):
        from_connection = self.nodes[a]
        to_connection = self.nodes[b]
        ip_port = "127.0.0.1:" + str(p2p_port(b))
        from_connection.addnode(ip_port, "onetry")
        # poll until version handshake complete to avoid race conditions
        # with transaction relaying
        # See comments in net_processing:
        # * Must have a version message before anything else
        # * Must have a verack message before anything else
        wait_until_helper(lambda: all(peer['version'] != 0 for peer in from_connection.getpeerinfo()))
        wait_until_helper(lambda: all(peer['version'] != 0 for peer in to_connection.getpeerinfo()))
        wait_until_helper(lambda: all(peer['bytesrecv_per_msg'].pop('verack', 0) == 24 for peer in from_connection.getpeerinfo()))
        wait_until_helper(lambda: all(peer['bytesrecv_per_msg'].pop('verack', 0) == 24 for peer in to_connection.getpeerinfo()))
        # The message bytes are counted before processing the message, so make
        # sure it was fully processed by waiting for a ping.
        wait_until_helper(lambda: all(peer["bytesrecv_per_msg"].pop("pong", 0) >= 32 for peer in from_connection.getpeerinfo()))
        wait_until_helper(lambda: all(peer["bytesrecv_per_msg"].pop("pong", 0) >= 32 for peer in to_connection.getpeerinfo()))

    def disconnect_nodes(self, a, b):
        def disconnect_nodes_helper(node_a, node_b):
            def get_peer_ids(from_connection, node_num):
                result = []
                for peer in from_connection.getpeerinfo():
                    if "testnode{}".format(node_num) in peer['subver']:
                        result.append(peer['id'])
                return result

            peer_ids = get_peer_ids(node_a, node_b.index)
            if not peer_ids:
                self.log.warning("disconnect_nodes: {} and {} were not connected".format(
                    node_a.index,
                    node_b.index,
                ))
                return
            for peer_id in peer_ids:
                try:
                    node_a.disconnectnode(nodeid=peer_id)
                except JSONRPCException as e:
                    # If this node is disconnected between calculating the peer id
                    # and issuing the disconnect, don't worry about it.
                    # This avoids a race condition if we're mass-disconnecting peers.
                    if e.error['code'] != -29:  # RPC_CLIENT_NODE_NOT_CONNECTED
                        raise

            # wait to disconnect
            self.wait_until(lambda: not get_peer_ids(node_a, node_b.index), timeout=5)
            self.wait_until(lambda: not get_peer_ids(node_b, node_a.index), timeout=5)

        disconnect_nodes_helper(self.nodes[a], self.nodes[b])

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

    # SYSCOIN
    def isolate_node(self, node, timeout=5):
        node.setnetworkactive(False)
        st = time.time()
        while time.time() < st + timeout:
            if node.getconnectioncount() == 0:
                return
            time.sleep(0.5)
        raise AssertionError("disconnect_node timed out")

    def reconnect_isolated_node(self, node, node_num):
        node.setnetworkactive(True)
        self.connect_nodes(node.index, node_num)

    def no_op(self):
        pass

    def generate(self, generator, *args, sync_fun=None, **kwargs):
        blocks = generator.generate(*args, invalid_call=False, **kwargs)
        sync_fun() if sync_fun else self.sync_all()
        return blocks

    def generateblock(self, generator, *args, sync_fun=None, **kwargs):
        blocks = generator.generateblock(*args, invalid_call=False, **kwargs)
        sync_fun() if sync_fun else self.sync_all()
        return blocks

    def generatetoaddress(self, generator, *args, sync_fun=None, **kwargs):
        blocks = generator.generatetoaddress(*args, invalid_call=False, **kwargs)
        sync_fun() if sync_fun else self.sync_all()
        return blocks

    def generatetodescriptor(self, generator, *args, sync_fun=None, **kwargs):
        blocks = generator.generatetodescriptor(*args, invalid_call=False, **kwargs)
        sync_fun() if sync_fun else self.sync_all()
        return blocks

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

    def sync_mempools(self, nodes=None, wait=1, timeout=60, flush_scheduler=True):
        """
        Wait until everybody has the same transactions in their memory
        pools
        """
        rpc_connections = nodes or self.nodes
        timeout = int(timeout * self.options.timeout_factor)
        stop_time = time.time() + timeout
        while time.time() <= stop_time:
            pool = [set(r.getrawmempool()) for r in rpc_connections]
            if pool.count(pool[0]) == len(rpc_connections):
                if flush_scheduler:
                    for r in rpc_connections:
                        r.syncwithvalidationinterfacequeue()
                return
            # Check that each peer has at least one connection
            assert (all([len(x.getpeerinfo()) for x in rpc_connections]))
            time.sleep(wait)
        raise AssertionError("Mempool sync timed out after {}s:{}".format(
            timeout,
            "".join("\n  {!r}".format(m) for m in pool),
        ))

    def sync_all(self, nodes=None):
        self.sync_blocks(nodes)
        self.sync_mempools(nodes)

    def wait_until(self, test_function, timeout=60):
        return wait_until_helper(test_function, timeout=timeout, timeout_factor=self.options.timeout_factor)

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
        # Format logs the same as syscoind's debug.log with microprecision (so log files can be concatenated and sorted)
        formatter = logging.Formatter(fmt='%(asctime)s.%(msecs)03d000Z %(name)s (%(levelname)s): %(message)s', datefmt='%Y-%m-%dT%H:%M:%S')
        formatter.converter = time.gmtime
        fh.setFormatter(formatter)
        ch.setFormatter(formatter)
        # add the handlers to the logger
        self.log.addHandler(fh)
        self.log.addHandler(ch)

        if self.options.trace_rpc:
            rpc_logger = logging.getLogger("SyscoinRPC")
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

            initialize_datadir(self.options.cachedir, CACHE_NODE_ID, self.chain, self.disable_autoconnect)
            self.nodes.append(
                TestNode(
                    CACHE_NODE_ID,
                    cache_node_dir,
                    chain=self.chain,
                    extra_conf=["bind=127.0.0.1"],
                    extra_args=['-disablewallet'],
                    rpchost=None,
                    timewait=self.rpc_timeout,
                    timeout_factor=self.options.timeout_factor,
                    syscoind=self.options.syscoind,
                    syscoin_cli=self.options.syscoincli,
                    coverage_dir=None,
                    cwd=self.options.tmpdir,
                    descriptors=self.options.descriptors,
                ))
            self.start_node(CACHE_NODE_ID)
            cache_node = self.nodes[CACHE_NODE_ID]

            # Wait for RPC connections to be ready
            cache_node.wait_for_rpc_connection()

            # Set a time in the past, so that blocks don't end up in the future
            cache_node.setmocktime(cache_node.getblockheader(cache_node.getbestblockhash())['time'])

            # Create a 199-block-long chain; each of the 3 first nodes
            # gets 25 mature blocks and 25 immature.
            # The 4th address gets 25 mature and only 24 immature blocks so that the very last
            # block in the cache does not age too much (have an old tip age).
            # This is needed so that we are out of IBD when the test starts,
            # see the tip age check in IsInitialBlockDownload().
            gen_addresses = [k.address for k in TestNode.PRIV_KEYS][:3] + [create_deterministic_address_bcrt1_p2tr_op_true()[0]]
            assert_equal(len(gen_addresses), 4)
            for i in range(8):
                self.generatetoaddress(
                    cache_node,
                    nblocks=25 if i != 7 else 24,
                    address=gen_addresses[i % len(gen_addresses)],
                )

            assert_equal(cache_node.getblockchaininfo()["blocks"], 199)

            # Shut it down, and clean up cache directories:
            self.stop_nodes()
            self.nodes = []

            def cache_path(*paths):
                return os.path.join(cache_node_dir, self.chain, *paths)

            os.rmdir(cache_path('wallets'))  # Remove empty wallets dir
            for entry in os.listdir(cache_path()):
                if entry not in ['chainstate', 'blocks', 'indexes', 'nevmminttx', 'nevmtxroots', 'geth', 'dbblockindex', 'asset', 'assetnft', 'llmq', 'evodb', 'nevmdata']:  # Only keep chainstate and blocks folder
                    os.remove(cache_path(entry))

        for i in range(self.num_nodes):
            self.log.debug("Copy cache directory {} to node {}".format(cache_node_dir, i))
            to_dir = get_datadir_path(self.options.tmpdir, i)
            shutil.copytree(cache_node_dir, to_dir)
            initialize_datadir(self.options.tmpdir, i, self.chain, self.disable_autoconnect)  # Overwrite port/rpcport in syscoin.conf

    def _initialize_chain_clean(self):
        """Initialize empty blockchain for use by the test.

        Create an empty blockchain and num_nodes wallets.
        Useful if a test case wants complete control over initialization."""
        for i in range(self.num_nodes):
            initialize_datadir(self.options.tmpdir, i, self.chain, self.disable_autoconnect)

    def skip_if_no_py3_zmq(self):
        """Attempt to import the zmq package and skip the test if the import fails."""
        try:
            import zmq  # noqa
        except ImportError:
            raise SkipTest("python3-zmq module not available.")

    def skip_if_no_py_sqlite3(self):
        """Attempt to import the sqlite3 package and skip the test if the import fails."""
        try:
            import sqlite3  # noqa
        except ImportError:
            raise SkipTest("sqlite3 module not available.")

    def skip_if_no_python_bcc(self):
        """Attempt to import the bcc package and skip the tests if the import fails."""
        try:
            import bcc  # type: ignore[import] # noqa: F401
        except ImportError:
            raise SkipTest("bcc python module not available")

    def skip_if_no_syscoind_tracepoints(self):
        """Skip the running test if syscoind has not been compiled with USDT tracepoint support."""
        if not self.is_usdt_compiled():
            raise SkipTest("syscoind has not been built with USDT tracepoints enabled.")

    def skip_if_no_bpf_permissions(self):
        """Skip the running test if we don't have permissions to do BPF syscalls and load BPF maps."""
        # check for 'root' permissions
        if os.geteuid() != 0:
            raise SkipTest("no permissions to use BPF (please review the tests carefully before running them with higher privileges)")

    def skip_if_platform_not_linux(self):
        """Skip the running test if we are not on a Linux platform"""
        if platform.system() != "Linux":
            raise SkipTest("not on a Linux system")

    def skip_if_platform_not_posix(self):
        """Skip the running test if we are not on a POSIX platform"""
        if os.name != 'posix':
            raise SkipTest("not on a POSIX system")

    def skip_if_no_syscoind_zmq(self):
        """Skip the running test if syscoind has not been compiled with zmq support."""
        if not self.is_zmq_compiled():
            raise SkipTest("syscoind has not been built with zmq enabled.")

    def skip_if_no_wallet(self):
        """Skip the running test if wallet has not been compiled."""
        self._requires_wallet = True
        if not self.is_wallet_compiled():
            raise SkipTest("wallet has not been compiled.")
        if self.options.descriptors:
            self.skip_if_no_sqlite()
        else:
            self.skip_if_no_bdb()

    def skip_if_no_sqlite(self):
        """Skip the running test if sqlite has not been compiled."""
        if not self.is_sqlite_compiled():
            raise SkipTest("sqlite has not been compiled.")

    def skip_if_no_bdb(self):
        """Skip the running test if BDB has not been compiled."""
        if not self.is_bdb_compiled():
            raise SkipTest("BDB has not been compiled.")

    def skip_if_no_wallet_tool(self):
        """Skip the running test if syscoin-wallet has not been compiled."""
        if not self.is_wallet_tool_compiled():
            raise SkipTest("syscoin-wallet has not been compiled")

    def skip_if_no_syscoin_util(self):
        """Skip the running test if syscoin-util has not been compiled."""
        if not self.is_syscoin_util_compiled():
            raise SkipTest("syscoin-util has not been compiled")

    def skip_if_no_cli(self):
        """Skip the running test if syscoin-cli has not been compiled."""
        if not self.is_cli_compiled():
            raise SkipTest("syscoin-cli has not been compiled.")

    def skip_if_no_previous_releases(self):
        """Skip the running test if previous releases are not available."""
        if not self.has_previous_releases():
            raise SkipTest("previous releases not available or disabled")

    def has_previous_releases(self):
        """Checks whether previous releases are present and enabled."""
        if not os.path.isdir(self.options.previous_releases_path):
            if self.options.prev_releases:
                raise AssertionError("Force test of previous releases but releases missing: {}".format(
                    self.options.previous_releases_path))
        return self.options.prev_releases

    def skip_if_no_external_signer(self):
        """Skip the running test if external signer support has not been compiled."""
        if not self.is_external_signer_compiled():
            raise SkipTest("external signer support has not been compiled.")

    def is_cli_compiled(self):
        """Checks whether syscoin-cli was compiled."""
        return self.config["components"].getboolean("ENABLE_CLI")

    def is_external_signer_compiled(self):
        """Checks whether external signer support was compiled."""
        return self.config["components"].getboolean("ENABLE_EXTERNAL_SIGNER")

    def is_wallet_compiled(self):
        """Checks whether the wallet module was compiled."""
        return self.config["components"].getboolean("ENABLE_WALLET")

    def is_specified_wallet_compiled(self):
        """Checks whether wallet support for the specified type
           (legacy or descriptor wallet) was compiled."""
        if self.options.descriptors:
            return self.is_sqlite_compiled()
        else:
            return self.is_bdb_compiled()

    def is_wallet_tool_compiled(self):
        """Checks whether syscoin-wallet was compiled."""
        return self.config["components"].getboolean("ENABLE_WALLET_TOOL")

    def is_syscoin_util_compiled(self):
        """Checks whether syscoin-util was compiled."""
        return self.config["components"].getboolean("ENABLE_SYSCOIN_UTIL")

    def is_zmq_compiled(self):
        """Checks whether the zmq module was compiled."""
        return self.config["components"].getboolean("ENABLE_ZMQ")

    def is_usdt_compiled(self):
        """Checks whether the USDT tracepoints were compiled."""
        return self.config["components"].getboolean("ENABLE_USDT_TRACEPOINTS")

    def is_sqlite_compiled(self):
        """Checks whether the wallet module was compiled with Sqlite support."""
        return self.config["components"].getboolean("USE_SQLITE")

    def is_bdb_compiled(self):
        """Checks whether the wallet module was compiled with BDB support."""
        return self.config["components"].getboolean("USE_BDB")

    def bump_mocktime(self, t, nodes=None):
        if self.mocktime is None:
            self.mocktime = int(time.time())
        else:
            self.mocktime += t
        set_node_times(nodes or self.nodes, self.mocktime)

    def is_syscall_sandbox_compiled(self):
        """Checks whether the syscall sandbox was compiled."""
        return self.config["components"].getboolean("ENABLE_SYSCALL_SANDBOX")



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


class DashTestFramework(SyscoinTestFramework):
    # Methods to override in subclass test scripts.
    def run_test(self):
        """Tests must override this method to define test logic"""
        raise NotImplementedError
    def set_test_params(self):
        """Tests must this method to change default values for number of nodes, topology, etc"""
        raise NotImplementedError
    def set_dash_test_params(self, num_nodes, masterodes_count, extra_args=None, fast_dip3_enforcement=False):
        self.mn_count = masterodes_count
        self.num_nodes = num_nodes
        self.mninfo = []
        self.setup_clean_chain = True
        # additional args
        if extra_args is None:
            extra_args = [[]] * num_nodes
        assert_equal(len(extra_args), num_nodes)
        self.extra_args = [copy.deepcopy(a) for a in extra_args]
        self.extra_args[0] += ["-sporkkey=cVpF924EspNh8KjYsfhgY96mmxvT6DgdWiTYMtMjuM74hJaU5psW"]
        self.fast_dip3_enforcement = fast_dip3_enforcement
        if fast_dip3_enforcement:
            for i in range(0, num_nodes):
                self.extra_args[i].append("-dip3params=30:50")
        for i in range(0, num_nodes):
            self.extra_args[i].append("-mncollateral=100")
        # LLMQ default test params (no need to pass -llmqtestparams)
        self.llmq_size = 3
        self.llmq_threshold = 2
        self.disable_autoconnect = False

    def confirm_mns(self):
        while True:
            diff = self.nodes[0].protx_diff(self.nodes[0].getblockhash(1), self.nodes[0].getblockhash(self.nodes[0].getblockcount()))
            found_unconfirmed = False
            for mn in diff["mnList"]:
                if int(mn["confirmedHash"], 16) == 0:
                    found_unconfirmed = True
                    break
            if not found_unconfirmed:
                break
            self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        self.sync_blocks()

    def bump_scheduler(self, t, nodes=None):
        bump_node_times(nodes or self.nodes, t)

    def set_dash_llmq_test_params(self, llmq_size, llmq_threshold):
        self.llmq_size = llmq_size
        self.llmq_threshold = llmq_threshold
        for i in range(0, self.num_nodes):
            self.extra_args[i].append("-llmqtestparams=%d:%d" % (self.llmq_size, self.llmq_threshold))

    def create_simple_node(self):
        idx = len(self.nodes)
        self.add_nodes(1, extra_args=[self.extra_args[idx]])
        self.start_node(idx)
        for i in range(0, idx):
            self.connect_nodes(i, idx)

    def prepare_masternodes(self):
        self.log.info("Preparing %d masternodes" % self.mn_count)
        for idx in range(0, self.mn_count):
            self.prepare_masternode(idx)
        self.sync_all()

    def prepare_masternode(self, idx):
        register_fund = (idx % 2) == 0

        bls = self.nodes[0].bls_generate()
        address = self.nodes[0].getnewaddress()
        txid = self.nodes[0].sendtoaddress(address, MASTERNODE_COLLATERAL)

        collateral_vout = 0
        if not register_fund:
            txraw = self.nodes[0].getrawtransaction(txid, True)
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
        ipAndPort = '127.0.0.1:%d' % port
        operatorReward = idx
        submit = (idx % 4) < 2
        if register_fund:
            # self.nodes[0].lockunspent(True, [{'txid': txid, 'vout': collateral_vout}])
            protx_result = self.nodes[0].protx_register_fund(address, ipAndPort, ownerAddr, bls['public'], votingAddr, operatorReward, rewardsAddr, address, submit)
        else:
            self.generate(self.nodes[0], 1, sync_fun=self.no_op)
            protx_result = self.nodes[0].protx_register(txid, collateral_vout, ipAndPort, ownerAddr, bls['public'], votingAddr, operatorReward, rewardsAddr, address, submit)

        if submit:
            proTxHash = protx_result
        else:
            proTxHash = self.nodes[0].sendrawtransaction(protx_result)

        if operatorReward > 0:
            self.generate(self.nodes[0], 1)
            operatorPayoutAddress = self.nodes[0].getnewaddress()
            self.nodes[0].protx_update_service(proTxHash, ipAndPort, bls['secret'], operatorPayoutAddress, address)

        self.mninfo.append(MasternodeInfo(proTxHash, ownerAddr, votingAddr, bls['public'], bls['secret'], address, txid, collateral_vout))

        self.log.info("Prepared masternode %d: collateral_txid=%s, collateral_vout=%d, protxHash=%s" % (idx, txid, collateral_vout, proTxHash))

    def remove_masternode(self, idx):
        mn = self.mninfo[idx]
        rawtx = self.nodes[0].createrawtransaction([{"txid": mn.collateral_txid, "vout": mn.collateral_vout}], {self.nodes[0].getnewaddress(): 99.9999})
        rawtx = self.nodes[0].signrawtransactionwithwallet(rawtx)
        self.nodes[0].sendrawtransaction(rawtx["hex"])
        self.generate(self.nodes[0], 1)
        self.mninfo.remove(mn)

        self.log.info("Removed masternode %d", idx)

    def prepare_datadirs(self):
        # stop faucet node so that we can copy the datadir
        self.stop_node(0)

        start_idx = len(self.nodes)
        for idx in range(0, self.mn_count):
            copy_datadir(0, idx + start_idx, self.options.tmpdir)

        # restart faucet node
        self.start_node(0)
        force_finish_mnsync(self.nodes[0])

    def start_masternodes(self):
        self.log.info("Starting %d masternodes", self.mn_count)

        start_idx = len(self.nodes)
        # SYSCOIN add offset and add nodes individually with offset and custom args
        for idx in range(0, self.mn_count):
            self.add_nodes(1, offset=idx + start_idx, extra_args=[self.extra_args[idx + start_idx]])

        def do_connect(idx):
            # Connect to the control node only, masternodes should take care of intra-quorum connections themselves
            self.connect_nodes(self.mninfo[idx].nodeIdx, 0)


        # start up nodes in parallel
        for idx in range(0, self.mn_count):
            self.mninfo[idx].nodeIdx = idx + start_idx
            time.sleep(0.5)
            self.start_masternode(self.mninfo[idx])


        for idx in range(0, self.mn_count):
            do_connect(idx)


    def start_masternode(self, mninfo, extra_args=None):
        args = ['-masternodeblsprivkey=%s' % mninfo.keyOperator] + self.extra_args[mninfo.nodeIdx]
        self.extra_args[mninfo.nodeIdx].append('-masternodeblsprivkey=%s' % mninfo.keyOperator)
        if extra_args is not None:
            args += extra_args
        self.start_node(mninfo.nodeIdx, extra_args=args)
        mninfo.node = self.nodes[mninfo.nodeIdx]
        force_finish_mnsync(mninfo.node)

    def setup_network(self):
        self.options.descriptors = False
        self.log.info("Creating and starting controller node")
        self.add_nodes(1, extra_args=[self.extra_args[0]])
        self.start_node(0)
        num_nodes_copy = self.num_nodes
        self.num_nodes = 1
        if self.is_wallet_compiled():
            self.import_deterministic_coinbase_privkeys()
        self.num_nodes = num_nodes_copy
        required_balance = MASTERNODE_COLLATERAL * self.mn_count + 1
        self.log.info("Generating %d coins" % required_balance)
        while self.nodes[0].getbalance() < required_balance:
            self.bump_mocktime(1)
            self.generatetoaddress(self.nodes[0], 10, self.nodes[0].getnewaddress())
        num_simple_nodes = self.num_nodes - self.mn_count - 1
        self.log.info("Creating and starting %s simple nodes", num_simple_nodes)
        for i in range(0, num_simple_nodes):
            self.create_simple_node()

        self.log.info("Activating DIP3")
        if not self.fast_dip3_enforcement:
            while self.nodes[0].getblockcount() < 500:
                self.generatetoaddress(self.nodes[0], 10, self.nodes[0].getnewaddress())
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
        self.generate(self.nodes[0], 1)
        # Enable ChainLocks by default
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()
        self.bump_mocktime(1)

        mn_info = self.nodes[0].masternode_list("status")
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
        ret = node_from.signrawtransactionwithwallet(rawtx)
        decoded = node_from.decoderawtransaction(ret['hex'])
        ret = {**decoded, **ret}
        return ret

    def wait_for_chainlocked_block(self, node, block_hash, expected=True, timeout=60):
        def check_chainlocked_block():
            try:
                self.bump_mocktime(1)
                block = node.getblock(block_hash)
                return block["confirmations"] > 0 and block["chainlock"] is True
            except Exception:
                return False
        if wait_until_helper(check_chainlocked_block, timeout=timeout, do_assert=expected, sleep=0.5) and not expected:
            raise AssertionError("waiting unexpectedly succeeded")

    def wait_for_chainlocked_block_all_nodes(self, block_hash, timeout=60):
        beforeTime = int(time.time())
        for node in self.nodes:
            self.wait_for_chainlocked_block(node, block_hash, timeout=timeout)
        afterTime = int(time.time())
        self.log.info("Chainlock found in {} seconds across {} nodes".format(int(afterTime - beforeTime), len(self.nodes)))

    def wait_for_most_recent_chainlock(self, node, block_hash, timeout=30):
        def check_cl():
            try:
                self.bump_mocktime(1)
                return node.getchainlocks()["recent_chainlock"]["blockhash"] == block_hash
            except Exception:
                return False
        wait_until_helper(check_cl, timeout=timeout, sleep=0.5)

    def wait_for_most_active_chainlock(self, node, block_hash, timeout=30):
        def check_cl():
            try:
                self.bump_mocktime(1)
                return node.getchainlocks()["active_chainlock"]["blockhash"] == block_hash
            except Exception:
                return False
        wait_until_helper(check_cl, timeout=timeout, sleep=0.5)

    def wait_for_sporks_same(self, timeout=30):
        def check_sporks_same():
            self.bump_mocktime(1)
            sporks = self.nodes[0].spork('show')
            return all(node.spork('show') == sporks for node in self.nodes)
        wait_until_helper(check_sporks_same, timeout=timeout, sleep=0.5)

    def wait_for_quorum_connections(self, quorum_hash, expected_connections, nodes, llmq_type_name="llmq_test", timeout = 60, wait_proc=None):
        def check_quorum_connections():
            all_ok = True
            for node in nodes:
                s = node.quorum_dkgstatus()
                mn_ok = True
                for qs in s:
                    if "llmqType" not in qs:
                        continue
                    if qs["llmqType"] != llmq_type_name:
                        continue
                    if "quorumConnections" not in qs:
                        continue
                    qconnections = qs["quorumConnections"]
                    if qconnections["quorumHash"] != quorum_hash:
                        mn_ok = False
                        continue
                    cnt = 0
                    for c in qconnections["quorumConnections"]:
                        if c["connected"]:
                            cnt += 1
                    if cnt < expected_connections:
                        mn_ok = False
                        break
                    break
                if not mn_ok:
                    all_ok = False
                    break
            if not all_ok and wait_proc is not None:
                wait_proc()
            return all_ok
        wait_until_helper(check_quorum_connections, timeout=timeout, sleep=1)

    def wait_for_masternode_probes(self, mninfos, timeout = 60, wait_proc=None, llmq_type_name="llmq_test"):
        def check_probes():
            def ret():
                if wait_proc is not None:
                    wait_proc()
                return False

            for mn in mninfos:
                s = mn.node.quorum_dkgstatus()
                if llmq_type_name not in s["session"]:
                    continue
                if "quorumConnections" not in s:
                    return ret()
                s = s["quorumConnections"]
                if llmq_type_name not in s:
                    return ret()

                for c in s[llmq_type_name]:
                    if c["proTxHash"] == mn.proTxHash:
                        continue
                    if not c["outbound"]:
                        mn2 = mn.node.protx_info(c["proTxHash"])
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
        wait_until_helper(check_probes, timeout=timeout, sleep=1)

    def wait_for_quorum_phase(self, quorum_hash, phase, expected_member_count, check_received_messages, check_received_messages_count, mninfos, wait_proc=None, llmq_type_name="llmq_test", timeout=60, sleep=0.5):
        def check_dkg_session():
            all_ok = True
            member_count = 0
            if wait_proc is not None:
                wait_proc()
            for mn in mninfos:
                s = mn.node.quorum_dkgstatus()["session"]
                mn_ok = True
                for qs in s:
                    if qs["llmqType"] != llmq_type_name:
                        continue
                    qstatus = qs["status"]
                    if qstatus["quorumHash"] != quorum_hash:
                        continue
                    member_count += 1
                    if "phase" not in qstatus:
                        mn_ok = False
                        break
                    if qstatus["phase"] != phase:
                        mn_ok = False
                        break
                    if check_received_messages is not None:
                        if qstatus[check_received_messages] < check_received_messages_count:
                            mn_ok = False
                            break
                    break
                if not mn_ok:
                    all_ok = False
                    break
            if all_ok and member_count != expected_member_count:
                return False
            return all_ok
        wait_until_helper(check_dkg_session, timeout=timeout, sleep=sleep)

    def wait_for_quorum_commitment(self, quorum_hash, nodes, wait_proc=None, llmq_type=100, timeout=60):
        def check_dkg_comitments():
            time.sleep(2)
            all_ok = True
            if wait_proc is not None:
                wait_proc()
            for node in nodes:
                s = node.quorum_dkgstatus()
                if "minableCommitments" not in s:
                    all_ok = False
                    break
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
                    all_ok = False
                    break
            return all_ok
        wait_until_helper(check_dkg_comitments, timeout=timeout, sleep=1)

    def wait_for_quorum_list(self, quorum_hash, nodes, timeout=60, sleep=2, llmq_type_name="llmq_test"):
        def wait_func():
            self.log.info("quorums: " + str(self.nodes[0].quorum_list()))
            if quorum_hash in self.nodes[0].quorum_list()[llmq_type_name]:
                return True
            self.bump_mocktime(sleep, nodes=nodes)
            self.generate(self.nodes[0], 1, sync_fun=self.no_op)
            self.sync_blocks(nodes)
            return False
        wait_until_helper(wait_func, timeout=timeout, sleep=sleep)

    def move_blocks(self, nodes, num_blocks):
        time.sleep(1)
        self.bump_mocktime(1, nodes=nodes)
        self.generate(self.nodes[0], num_blocks, sync_fun=self.no_op)
        self.sync_blocks(nodes)

    def mine_quorum(self, llmq_type_name="llmq_test", llmq_type=100, expected_connections=None, expected_members=None, expected_contributions=None, expected_complaints=0, expected_justifications=0, expected_commitments=None, mninfos_online=None, mninfos_valid=None, mod5=False):
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

        def timeout_func():
            self.bump_mocktime(1, nodes=nodes)

        # move forward to next DKG
        skip_count = 24 - (self.nodes[0].getblockcount() % 24)
        if skip_count != 0:
            self.bump_mocktime(1, nodes=nodes)
            self.generate(self.nodes[0], skip_count, sync_fun=self.no_op)
        self.sync_blocks(nodes)

        q = self.nodes[0].getbestblockhash()
        self.log.info("Expected quorum_hash:"+str(q))
        self.log.info("Waiting for phase 1 (init)")
        self.wait_for_quorum_phase(q, 1, expected_members, None, 0, mninfos_online, wait_proc=timeout_func, llmq_type_name=llmq_type_name)
        self.wait_for_quorum_connections(q, expected_connections, nodes, wait_proc=lambda: self.bump_mocktime(1, nodes=nodes), llmq_type_name=llmq_type_name)
        if spork23_active:
            self.wait_for_masternode_probes(mninfos_valid, wait_proc=lambda: self.bump_mocktime(1, nodes=nodes))

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 2 (contribute)")
        self.wait_for_quorum_phase(q, 2, expected_members, "receivedContributions", expected_contributions, mninfos_online, wait_proc=timeout_func, llmq_type_name=llmq_type_name)

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 3 (complain)")
        self.wait_for_quorum_phase(q, 3, expected_members, "receivedComplaints", expected_complaints, mninfos_online, wait_proc=timeout_func, llmq_type_name=llmq_type_name)

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 4 (justify)")
        self.wait_for_quorum_phase(q, 4, expected_members, "receivedJustifications", expected_justifications, mninfos_online, wait_proc=timeout_func, llmq_type_name=llmq_type_name)

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 5 (commit)")
        self.wait_for_quorum_phase(q, 5, expected_members, "receivedPrematureCommitments", expected_commitments, mninfos_online, wait_proc=timeout_func, llmq_type_name=llmq_type_name)

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 6 (mining)")
        self.wait_for_quorum_phase(q, 6, expected_members, None, 0, mninfos_online, wait_proc=timeout_func, llmq_type_name=llmq_type_name)

        self.log.info("Waiting final commitment")
        self.wait_for_quorum_commitment(q, nodes, wait_proc=timeout_func, llmq_type=llmq_type)

        self.log.info("Mining final commitment")
        self.bump_mocktime(1, nodes=nodes)
        self.nodes[0].getblocktemplate({"rules": ["segwit"]}) # this calls CreateNewBlock
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        self.sync_blocks(nodes)

        self.log.info("Waiting for quorum to appear in the list")
        self.wait_for_quorum_list(q, nodes, llmq_type_name=llmq_type_name)

        new_quorum = self.nodes[0].quorum_list(1)[llmq_type_name][0]
        assert_equal(q, new_quorum)
        quorum_info = self.nodes[0].quorum_info(llmq_type, new_quorum)

        # Mine 5 (SIGN_HEIGHT_LOOKBACK) more blocks to make sure that the new quorum gets eligible for signing sessions
        self.generate(self.nodes[0], 5, sync_fun=self.no_op)
        # Make sure we are mod of 5 block count before we start tests
        skip_count = 5 - (self.nodes[0].getblockcount() % 5)
        if skip_count != 0 and mod5 is True:
            self.bump_mocktime(1, nodes=nodes)
            self.generate(self.nodes[0], skip_count, sync_fun=self.no_op)
        self.sync_blocks(nodes)

        self.log.info("New quorum: height=%d, quorumHash=%s, minedBlock=%s" % (quorum_info["height"], new_quorum, quorum_info["minedBlock"]))

        return new_quorum

    def move_to_next_cycle(self):
        cycle_length = 24
        mninfos_online = self.mninfo.copy()
        nodes = [self.nodes[0]] + [mn.node for mn in mninfos_online]
        cur_block = self.nodes[0].getblockcount()

        # move forward to next DKG
        skip_count = cycle_length - (cur_block % cycle_length)
        if skip_count != 0:
            self.bump_mocktime(1, nodes=nodes)
            self.generate(self.nodes[0], skip_count)
        self.sync_blocks(nodes)
        time.sleep(1)
        self.log.info('Moved from block %d to %d' % (cur_block, self.nodes[0].getblockcount()))

    def get_recovered_sig(self, rec_sig_id, rec_sig_msg_hash, llmq_type=100, node=None):
        # Note: recsigs aren't relayed no regular nodes by default,
        # make sure to pick a mn as a node to query for recsigs.
        node = self.mninfo[0].node if node is None else node
        time_start = time.time()
        while time.time() - time_start < 10:
            try:
                self.bump_mocktime(5, nodes=self.nodes)
                return node.quorum_getrecsig(llmq_type, rec_sig_id, rec_sig_msg_hash)
            except JSONRPCException:
                time.sleep(0.1)
        return False

    def get_quorum_masternodes(self, q):
        qi = self.nodes[0].quorum_info(100, q)
        result = []
        for m in qi['members']:
            result.append(self.get_mninfo(m['proTxHash']))
        return result

    def get_mninfo(self, proTxHash):
        for mn in self.mninfo:
            if mn.proTxHash == proTxHash:
                return mn
        return None

    def wait_for_mnauth(self, node, count, timeout=10):
        def test():
            pi = node.getpeerinfo()
            c = 0
            for p in pi:
                if "verified_proregtx_hash" in p and p["verified_proregtx_hash"] != "":
                    c += 1
            return c >= count
        wait_until_helper(test, timeout=timeout)
