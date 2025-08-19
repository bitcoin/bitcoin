#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Copyright (c) 2014-2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Base class for RPC testing."""

import configparser
import copy
from _decimal import Decimal, ROUND_DOWN
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
from concurrent.futures import ThreadPoolExecutor

from typing import List, Optional, Union
from .address import ADDRESS_BCRT1_P2SH_OP_TRUE
from .authproxy import JSONRPCException
from test_framework.masternodes import check_banned, check_punished
from test_framework.blocktools import TIME_GENESIS_BLOCK
from . import coverage
from .messages import (
    hash256,
    msg_isdlock,
    ser_compact_size,
    ser_string,
    tx_from_hex,
)
from .script import hash160
from .p2p import NetworkThread
from .test_node import TestNode
from .util import (
    PortSeed,
    MAX_NODES,
    append_config,
    assert_equal,
    assert_raises_rpc_error,
    check_json_precision,
    copy_datadir,
    force_finish_mnsync,
    get_chain_conf_names,
    get_datadir_path,
    initialize_datadir,
    p2p_port,
    set_node_times,
    satoshi_round,
    softfork_active,
    wait_until_helper,
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

    chain = None  # type: str
    setup_clean_chain = None  # type: bool

    def __init__(self):
        """Sets test framework defaults. Do not override this method. Instead, override the set_test_params() method"""
        self.chain: str = 'regtest'
        self.setup_clean_chain: bool = False
        self.disable_mocktime: bool = False
        self.nodes: List[TestNode] = []
        self.extra_args = None
        self.network_thread = None
        self.mocktime = 0
        self.rpc_timeout = 60  # Wait for up to 60 seconds for the RPC server to respond
        self.supports_cli = True
        self.bind_to_localhost_only = True
        self.parse_args()
        self.default_wallet_name = "default_wallet" if self.options.descriptors else ""
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
                            help="Leave dashds and test.* datadir on exit or error")
        parser.add_argument("--noshutdown", dest="noshutdown", default=False, action="store_true",
                            help="Don't stop dashds after the test execution")
        parser.add_argument("--cachedir", dest="cachedir", default=os.path.abspath(os.path.dirname(os.path.realpath(__file__)) + "/../../cache"),
                            help="Directory for caching pregenerated datadirs (default: %(default)s)")
        parser.add_argument("--tmpdir", dest="tmpdir", help="Root directory for datadirs (must not exist)")
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
                            help="use dash-cli instead of RPC for all commands")
        parser.add_argument("--dashd-arg", dest="dashd_extra_args", default=[], action="append",
                            help="Pass extra args to all dashd instances")
        parser.add_argument("--timeoutscale", dest="timeout_scale", default=1, type=int,
                            help=argparse.SUPPRESS)
        parser.add_argument("--perf", dest="perf", default=False, action="store_true",
                            help="profile running nodes with perf for the duration of the test")
        parser.add_argument("--valgrind", dest="valgrind", default=False, action="store_true",
                            help="run nodes under the valgrind memory error detector: expect at least a ~10x slowdown, valgrind 3.14 or later required. Does not apply to previous release binaries.")
        parser.add_argument("--randomseed", type=int,
                            help="set a random seed for deterministically reproducing a previous test run")
        parser.add_argument("--timeout-factor", dest="timeout_factor", type=float, help="adjust test timeouts by a factor. Setting it to 0 disables all timeouts")
        parser.add_argument("--v2transport", dest="v2transport", default=False, action="store_true",
                            help="use BIP324 v2 connections between all nodes by default")
        parser.add_argument("--v1transport", dest="v1transport", default=False, action="store_true",
                            help="Explicitly use v1 transport (can be used to overwrite global --v2transport option)")

        group = parser.add_mutually_exclusive_group()
        group.add_argument("--descriptors", action='store_const', const=True,
                            help="Run test using a descriptor wallet", dest='descriptors')
        group.add_argument("--legacy-wallet", action='store_const', const=False,
                            help="Run test using legacy wallets", dest='descriptors')

        self.add_options(parser)
        self.options = parser.parse_args()
        if self.options.timeout_factor == 0:
            self.options.timeout_factor = 99999
        self.options.timeout_factor = self.options.timeout_factor or (4 if self.options.valgrind else 1)
        self.options.previous_releases_path = previous_releases_path

        config = configparser.ConfigParser()
        config.read_file(open(self.options.configfile))
        self.config = config
        if self.options.v1transport:
            self.options.v2transport=False

        # Running TestShell in a Jupyter notebook causes an additional -f argument
        # To keep TestShell from failing with an "unrecognized argument" error, we add a dummy "-f" argument
        # source: https://stackoverflow.com/questions/48796169/how-to-fix-ipykernel-launcher-py-error-unrecognized-arguments-in-jupyter/56349168#56349168
        parser.add_argument("-f", "--fff", help="a dummy argument to fool ipython", default="1")

        if self.options.descriptors is None:
            # Prefer BDB unless it isn't available
            if self.is_bdb_compiled():
                self.options.descriptors = False
            elif self.is_sqlite_compiled():
                self.options.descriptors = True
            else:
                # If neither are compiled, tests requiring a wallet will be skipped and the value of self.options.descriptors won't matter
                # It still needs to exist and be None in order for tests to work however.
                self.options.descriptors = None

        PortSeed.n = self.options.port_seed

    def set_binary_paths(self):
        """Update self.options with the paths of all binaries from environment variables or their default values"""

        binaries = {
            "dashd": ("bitcoind", "DASHD"),
            "dash-cli": ("bitcoincli", "DASHCLI"),
            "dash-util": ("bitcoinutil", "DASHUTIL"),
            "dash-wallet": ("bitcoinwallet", "DASHWALLET"),
        }
        for binary, [attribute_name, env_variable_name] in binaries.items():
            default_filename = os.path.join(
                self.config["environment"]["BUILDDIR"],
                "src",
                binary + self.config["environment"]["EXEEXT"],
            )
            setattr(self.options, attribute_name, os.getenv(env_variable_name, default=default_filename))

    def setup(self):
        """Call this method to start up the test framework object with options set."""

        check_json_precision()

        self.options.cachedir = os.path.abspath(self.options.cachedir)

        config = self.config

        self.set_binary_paths()

        self.extra_args_from_options = self.options.dashd_extra_args

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
        else:
            self._initialize_chain()
        if not self.disable_mocktime:
            self._initialize_mocktime(is_genesis=self.setup_clean_chain)

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

        """If this method is updated - backport changes to  DashTestFramework.setup_nodes"""
        self.add_nodes(self.num_nodes, self.extra_args)
        self.start_nodes()
        if self.requires_wallet:
            self.import_deterministic_coinbase_privkeys()
        if not self.setup_clean_chain:
            for n in self.nodes:
                assert_equal(n.getblockchaininfo()["blocks"], 199)
            # To ensure that all nodes are out of IBD, the most recent block
            # must have a timestamp not too old (see IsInitialBlockDownload()).
            if not self.disable_mocktime:
                self.log.debug('Generate a block with current mocktime')
                self.bump_mocktime(156 * 200, update_schedulers=False)
            block_hash = self.generate(self.nodes[0], 1, sync_fun=self.no_op)[0]
            block = self.nodes[0].getblock(blockhash=block_hash, verbosity=0)
            for n in self.nodes:
                n.submitblock(block)
                chain_info = n.getblockchaininfo()
                assert_equal(chain_info["blocks"], 200)
                assert_equal(chain_info["initialblockdownload"], False)

    def import_deterministic_coinbase_privkeys(self):
        for i in range(len(self.nodes)):
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

    def add_nodes(self, num_nodes: int, extra_args=None, *, rpchost=None, binary=None, binary_cli=None, versions=None):
        """Instantiate TestNode objects.

        Should only be called once after the nodes have been specified in
        set_test_params()."""
        def get_bin_from_version(version, bin_name, bin_default):
            if not version:
                return bin_default
            return os.path.join(
                self.options.previous_releases_path,
                re.sub(
                    r'\.0$' if version != 150000 else r'^$',
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
        if binary is None:
            binary = [get_bin_from_version(v, 'dashd', self.options.bitcoind) for v in versions]
        if binary_cli is None:
            binary_cli = [get_bin_from_version(v, 'dash-cli', self.options.bitcoincli) for v in versions]
        assert_equal(len(extra_confs), num_nodes)
        assert_equal(len(extra_args), num_nodes)
        assert_equal(len(versions), num_nodes)
        assert_equal(len(binary), num_nodes)
        assert_equal(len(binary_cli), num_nodes)
        old_num_nodes = len(self.nodes)
        for i in range(num_nodes):
            test_node_i = TestNode(
                old_num_nodes + i,
                get_datadir_path(self.options.tmpdir, old_num_nodes + i),
                self.extra_args_from_options,
                chain=self.chain,
                rpchost=rpchost,
                timewait=self.rpc_timeout,
                timeout_factor=self.options.timeout_factor,
                bitcoind=binary[i],
                bitcoin_cli=binary_cli[i],
                version=versions[i],
                mocktime=self.mocktime,
                coverage_dir=self.options.coveragedir,
                cwd=self.options.tmpdir,
                extra_conf=extra_confs[i],
                extra_args=extra_args[i],
                use_cli=self.options.usecli,
                start_perf=self.options.perf,
                use_valgrind=self.options.valgrind,
                descriptors=self.options.descriptors,
                v2transport=self.options.v2transport,
            )
            self.nodes.append(test_node_i)
            if not test_node_i.version_is_at_least(160000):
                # adjust conf for pre 16
                conf_file = test_node_i.bitcoinconf
                with open(conf_file, 'r', encoding='utf8') as conf:
                    conf_data = conf.read()
                with open(conf_file, 'w', encoding='utf8') as conf:
                    conf.write(conf_data.replace('[regtest]', ''))

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

    # TODO: move it to DashTestFramework where it belongs
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

        (chain_name_conf_arg, chain_name_conf_arg_value, chain_name_conf_section) = get_chain_conf_names(self.chain)

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
            f.write("dip3params=2:2\n")
            f.write(f"testactivationheight=v20@{self.v20_height}\n")
            f.write(f"testactivationheight=mn_rr@{self.mn_rr_height}\n")
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

    def stop_nodes(self, expected_stderr='', wait=0):
        """Stop multiple dashd test nodes"""
        for node in self.nodes:
            # Issue RPC to stop nodes
            node.stop_node(expected_stderr=expected_stderr, wait=wait, wait_until_stopped=False)

        for node in self.nodes:
            # Wait for nodes to stop
            node.wait_until_stopped()

    def restart_node(self, i, extra_args=None, expected_stderr=''):
        """Stop and start a test node"""
        self.stop_node(i, expected_stderr)
        self.start_node(i, extra_args)

    def wait_for_node_exit(self, i, timeout):
        self.nodes[i].process.wait(timeout)

    def connect_nodes(self, a, b, *, peer_advertises_v2=None):
        # A node cannot connect to itself, bail out early
        if (a == b):
            return

        from_connection = self.nodes[a]
        to_connection = self.nodes[b]
        ip_port = "127.0.0.1:" + str(p2p_port(b))

        if peer_advertises_v2 is None:
            peer_advertises_v2 = from_connection.use_v2transport

        if peer_advertises_v2 != from_connection.use_v2transport:
            from_connection.addnode(node=ip_port, command="onetry", v2transport=peer_advertises_v2)
        else:
            # skip the optional third argument if it matches the default, for
            # compatibility with older clients
            from_connection.addnode(ip_port, "onetry")

        # Use subversion as peer id. Test nodes have their node number appended to the user agent string
        from_connection_subver = from_connection.getnetworkinfo()['subversion']
        to_connection_subver = to_connection.getnetworkinfo()['subversion']

        def find_conn(node, peer_subversion, inbound):
            return next(filter(lambda peer: peer['subver'] == peer_subversion and peer['inbound'] == inbound, node.getpeerinfo()), None)

        self.wait_until(lambda: find_conn(from_connection, to_connection_subver, inbound=False) is not None)
        self.wait_until(lambda: find_conn(to_connection, from_connection_subver, inbound=True) is not None)

        def check_bytesrecv(peer, msg_type, min_bytes_recv):
            assert peer is not None, "Error: peer disconnected"
            return peer['bytesrecv_per_msg'].pop(msg_type, 0) >= min_bytes_recv

        # Poll until version handshake (fSuccessfullyConnected) is complete to
        # avoid race conditions, because some message types are blocked from
        # being sent or received before fSuccessfullyConnected.
        #
        # As the flag fSuccessfullyConnected is not exposed, check it by
        # waiting for a pong, which can only happen after the flag was set.
        self.wait_until(lambda: check_bytesrecv(find_conn(from_connection, to_connection_subver, inbound=False), 'pong', 29))
        self.wait_until(lambda: check_bytesrecv(find_conn(to_connection, from_connection_subver, inbound=True), 'pong', 29))

    def disconnect_nodes(self, a, b):
        # A node cannot disconnect from itself, bail out early
        if (a == b):
            return

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
                    if e.error['code'] != -29: # RPC_CLIENT_NODE_NOT_CONNECTED
                        raise

            # wait to disconnect
            self.wait_until(lambda: not get_peer_ids(node_a, node_b.index), timeout=5)
            self.wait_until(lambda: not get_peer_ids(node_b, node_a.index), timeout=5)

        disconnect_nodes_helper(self.nodes[a], self.nodes[b])

    def isolate_node(self, node_num, timeout=5):
        self.nodes[node_num].setnetworkactive(False)
        self.wait_until(lambda: self.nodes[node_num].getconnectioncount() == 0, timeout=timeout)

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
                        if r.version_is_at_least(170000):
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

    def bump_mocktime(self, t, update_nodes=True, nodes=None, update_schedulers=True):
        if self.mocktime == 0:
            return

        self.mocktime += t

        if not update_nodes:
            return

        nodes_to_update = nodes or self.nodes
        set_node_times(nodes_to_update, self.mocktime)

        if not update_schedulers:
            return

        for node in nodes_to_update:
            if node.version_is_at_least(180100):
                node.mockscheduler(t)

    def _initialize_mocktime(self, is_genesis):
        if is_genesis:
            self.mocktime = TIME_GENESIS_BLOCK
        else:
            self.mocktime = TIME_GENESIS_BLOCK + (199 * 156)
        for node in self.nodes:
            node.mocktime = self.mocktime

    def wait_until(self, test_function, timeout=60, lock=None, sleep=0.05, do_assert=True):
        return wait_until_helper(test_function, timeout=timeout, lock=lock, timeout_factor=self.options.timeout_factor, sleep=sleep, do_assert=do_assert)

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
                    extra_args=['-disablewallet', f"-mocktime={TIME_GENESIS_BLOCK}"],
                    extra_args_from_options=self.extra_args_from_options,
                    rpchost=None,
                    timewait=self.rpc_timeout,
                    timeout_factor=self.options.timeout_factor,
                    bitcoind=self.options.bitcoind,
                    bitcoin_cli=self.options.bitcoincli,
                    mocktime=self.mocktime,
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
            self._initialize_mocktime(is_genesis=True)
            gen_addresses = [k.address for k in TestNode.PRIV_KEYS][:3] + [ADDRESS_BCRT1_P2SH_OP_TRUE]
            assert_equal(len(gen_addresses), 4)
            for i in range(8):
                self.bump_mocktime((25 if i != 7 else 24) * 156, update_schedulers=False)
                self.generatetoaddress(
                    cache_node,
                    nblocks=25 if i != 7 else 24,
                    address=gen_addresses[i % len(gen_addresses)],
                )

            assert_equal(cache_node.getblockchaininfo()["blocks"], 199)

            # Shut it down, and clean up cache directories:
            self.stop_nodes()
            self.nodes = []
            self.mocktime = 0

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

    def skip_if_no_python_bcc(self):
        """Attempt to import the bcc package and skip the tests if the import fails."""
        try:
            import bcc  # type: ignore[import] # noqa: F401
        except ImportError:
            raise SkipTest("bcc python module not available")

    def skip_if_no_bitcoind_tracepoints(self):
        """Skip the running test if dashd has not been compiled with USDT tracepoint support."""
        if not self.is_usdt_compiled():
            raise SkipTest("dashd has not been built with USDT tracepoints enabled.")

    def skip_if_no_bpf_permissions(self):
        """Skip the running test if we don't have permissions to do BPF syscalls and load BPF maps."""
        # check for 'root' permissions
        if os.geteuid() != 0:
            raise SkipTest("no permissions to use BPF (please review the tests carefully before running them with higher privileges)")

    def skip_if_platform_not_linux(self):
        """Skip the running test if we are not on a Linux platform"""
        if platform.system() != "Linux":
            raise SkipTest("not on a Linux system")

    def skip_if_no_bitcoind_zmq(self):
        """Skip the running test if dashd has not been compiled with zmq support."""
        if not self.is_zmq_compiled():
            raise SkipTest("dashd has not been built with zmq enabled.")

    def skip_if_no_wallet(self):
        """Skip the running test if wallet has not been compiled."""
        self.requires_wallet = True
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
        if not os.path.isdir(self.options.previous_releases_path):
            if self.options.prev_releases:
                raise AssertionError("Force test of previous releases but releases missing: {}".format(
                    self.options.previous_releases_path))
        return self.options.prev_releases

    def is_cli_compiled(self):
        """Checks whether dash-cli was compiled."""
        return self.config["components"].getboolean("ENABLE_CLI")

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
        """Checks whether dash-wallet was compiled."""
        return self.config["components"].getboolean("ENABLE_WALLET_TOOL")

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

MASTERNODE_COLLATERAL = 1000
EVONODE_COLLATERAL = 4000

class MasternodeInfo:
    proTxHash: str = ""
    fundsAddr: str = ""
    ownerAddr: str = ""
    votingAddr: str = ""
    rewards_address: str = ""
    operator_reward: int = 0
    pubKeyOperator: str = ""
    keyOperator: str = ""
    collateral_address: str = ""
    collateral_txid: str = ""
    collateral_vout: int = 0
    nodePort: int = 0
    evo: bool = False
    legacy: bool = False
    nodeIdx: Optional[int] = None
    friendlyName: Optional[str] = None

    def bury_tx(self, test: BitcoinTestFramework, genIdx: int, txid: str, depth: int):
        chain_tip = test.generate(test.nodes[genIdx], depth)[0]
        assert_equal(test.nodes[genIdx].getrawtransaction(txid, 1, chain_tip)['confirmations'], depth)

    def generate_addresses(self, node: TestNode, force_all: bool = False):
        if not self.collateral_address or force_all:
            self.collateral_address = node.getnewaddress()
        if not self.fundsAddr or force_all:
            self.fundsAddr = node.getnewaddress()
        if not self.ownerAddr or force_all:
            self.ownerAddr = node.getnewaddress()
        if not self.rewards_address or force_all:
            self.rewards_address = node.getnewaddress()
        if not self.votingAddr or force_all:
            self.votingAddr = node.getnewaddress()
        if not self.pubKeyOperator or not self.keyOperator or force_all:
            bls_ret = node.bls('generate', True) if self.legacy else node.bls('generate')
            self.pubKeyOperator = bls_ret['public']
            self.keyOperator = bls_ret['secret']

    def get_collateral_value(self) -> int:
        return EVONODE_COLLATERAL if self.evo else MASTERNODE_COLLATERAL

    def get_collateral_vout(self, node: TestNode, txid: str, should_throw: bool = True) -> int:
        for txout in node.getrawtransaction(txid, 1)['vout']:
            if txout['value'] == self.get_collateral_value():
                return txout['n']
        if should_throw:
            raise AssertionError(f"Unable to find collateral vout from txid {txid}")
        else:
            return -1

    def __init__(self, evo: bool, legacy: bool):
        if evo and legacy:
            raise AssertionError("EvoNodes are not allowed to use legacy scheme")
        self.evo = evo
        self.legacy = legacy

    def set_params(self, proTxHash: Optional[str] = None, operator_reward: Optional[int] = None, collateral_txid: Optional[str] = None,
                   collateral_vout: Optional[int] = None, nodePort: Optional[int] = None):
        if proTxHash is not None:
            self.proTxHash = proTxHash
        if operator_reward is not None:
            self.operator_reward = operator_reward
        if collateral_txid is not None:
            self.collateral_txid = collateral_txid
        if collateral_vout is not None:
            self.collateral_vout = collateral_vout
        if nodePort is not None:
            self.nodePort = nodePort

    def set_node(self, nodeIdx: int, friendlyName: Optional[str] = None):
        self.nodeIdx = nodeIdx
        self.nodePort = p2p_port(nodeIdx)
        self.friendlyName = friendlyName or f"mn-{'evo' if self.evo else 'reg'}-{self.nodeIdx}"

    def get_node(self, test: BitcoinTestFramework) -> TestNode:
        if self.nodeIdx is None:
            raise AssertionError("nodeIdx must be set, did you call set_node()")
        if self.nodeIdx > len(test.nodes):
            raise AssertionError(f"Node at pos {self.nodeIdx} not present, did you start the node?")
        return test.nodes[self.nodeIdx]

    def register(self, node: TestNode, submit: bool, collateral_txid: Optional[str] = None, collateral_vout: Optional[int] = None,
                 coreP2PAddrs: Union[str, List[str], None] = None, ownerAddr: Optional[str] = None, pubKeyOperator: Optional[str] = None, votingAddr: Optional[str] = None,
                 operator_reward: Optional[int] = None, rewards_address: Optional[str] = None, fundsAddr: Optional[str] = None,
                 platform_node_id: Optional[str] = None, platform_p2p_port: Optional[int] = None, platform_http_port: Optional[int] = None,
                 expected_assert_code: Optional[int] = None, expected_assert_msg: Optional[str] = None) -> Optional[str]:
        if (expected_assert_code and not expected_assert_msg) or (not expected_assert_code and expected_assert_msg):
            raise AssertionError("Intending to use assert_raises_rpc_error() but didn't specify code and message")

        # EvoNode-specific fields are ignored for regular masternodes
        if self.evo:
            if platform_node_id is None:
                raise AssertionError("EvoNode but platform_node_id is missing, must be specified!")
            if platform_p2p_port is None:
                raise AssertionError("EvoNode but platform_p2p_port is missing, must be specified!")
            if platform_http_port is None:
                raise AssertionError("EvoNode but platform_http_port is missing, must be specified!")

        # Common arguments shared between regular masternodes and EvoNodes
        args = [
            collateral_txid or self.collateral_txid,
            collateral_vout or self.collateral_vout,
            coreP2PAddrs or [f'127.0.0.1:{self.nodePort}'],
            ownerAddr or self.ownerAddr,
            pubKeyOperator or self.pubKeyOperator,
            votingAddr or self.votingAddr,
            operator_reward or self.operator_reward,
            rewards_address or self.rewards_address,
        ]
        address_funds = fundsAddr or self.fundsAddr

        # Use assert_raises_rpc_error if we expect to error out
        use_assert: bool = bool(expected_assert_code and expected_assert_msg)

        # Flip a coin and decide if we will submit the transaction directly or use sendrawtransaction
        use_srt: bool = bool(random.getrandbits(1)) and submit and not use_assert
        if use_srt:
            submit = False

        # Construct final command and arguments
        if self.evo:
            command = "register_evo"
            args = args + [platform_node_id, platform_p2p_port, platform_http_port, address_funds, submit] # type: ignore
        else:
            command = "register_legacy" if self.legacy else "register"
            args = args + [address_funds, submit] # type: ignore

        if use_assert:
            assert_raises_rpc_error(expected_assert_code, expected_assert_msg, node.protx, command, *args)
            return None

        ret: str = node.protx(command, *args)
        if use_srt:
            ret = node.sendrawtransaction(ret)

        return ret

    def register_fund(self, node: TestNode, submit: bool, collateral_address: Optional[str] = None, coreP2PAddrs: Union[str, List[str], None] = None,
                      ownerAddr: Optional[str] = None, pubKeyOperator: Optional[str] = None, votingAddr: Optional[str] = None,
                      operator_reward: Optional[int] = None, rewards_address: Optional[str] = None, fundsAddr: Optional[str] = None,
                      platform_node_id: Optional[str] = None, platform_p2p_port: Optional[int] = None, platform_http_port: Optional[int] = None,
                      expected_assert_code: Optional[int] = None, expected_assert_msg: Optional[str] = None) -> Optional[str]:
        if (expected_assert_code and not expected_assert_msg) or (not expected_assert_code and expected_assert_msg):
            raise AssertionError("Intending to use assert_raises_rpc_error() but didn't specify code and message")

        # EvoNode-specific fields are ignored for regular masternodes
        if self.evo:
            if platform_node_id is None:
                raise AssertionError("EvoNode but platform_node_id is missing, must be specified!")
            if platform_p2p_port is None:
                raise AssertionError("EvoNode but platform_p2p_port is missing, must be specified!")
            if platform_http_port is None:
                raise AssertionError("EvoNode but platform_http_port is missing, must be specified!")

        # Use assert_raises_rpc_error if we expect to error out
        use_assert: bool = bool(expected_assert_code and expected_assert_msg)

        # Flip a coin and decide if we will submit the transaction directly or use sendrawtransaction
        use_srt: bool = bool(random.getrandbits(1)) and submit and not use_assert
        if use_srt:
            submit = False

        # Common arguments shared between regular masternodes and EvoNodes
        args = [
            collateral_address or self.collateral_address,
            coreP2PAddrs or [f'127.0.0.1:{self.nodePort}'],
            ownerAddr or self.ownerAddr,
            pubKeyOperator or self.pubKeyOperator,
            votingAddr or self.votingAddr,
            operator_reward or self.operator_reward,
            rewards_address or self.rewards_address,
        ]
        address_funds = fundsAddr or self.fundsAddr

        # Construct final command and arguments
        if self.evo:
            command = "register_fund_evo"
            args = args + [platform_node_id, platform_p2p_port, platform_http_port, address_funds, submit] # type: ignore
        else:
            command = "register_fund_legacy" if self.legacy else "register_fund"
            args = args + [address_funds, submit] # type: ignore

        if use_assert:
            assert_raises_rpc_error(expected_assert_code, expected_assert_msg, node.protx, command, *args)
            return None

        ret: str = node.protx(command, *args)
        if use_srt:
            ret = node.sendrawtransaction(ret)

        return ret

    def revoke(self, node: TestNode, submit: bool, reason: int, fundsAddr: Optional[str] = None,
               expected_assert_code: Optional[int] = None, expected_assert_msg: Optional[str] = None) -> Optional[str]:
        if (expected_assert_code and not expected_assert_msg) or (not expected_assert_code and expected_assert_msg):
            raise AssertionError("Intending to use assert_raises_rpc_error() but didn't specify code and message")

        # Update commands should be run from the appropriate MasternodeInfo instance, we do not allow overriding some values for this reason
        if self.proTxHash is None:
            raise AssertionError("proTxHash not set, did you call set_params()")
        if self.keyOperator is None:
            raise AssertionError("keyOperator not set, did you call generate_addresses()")

        # Use assert_raises_rpc_error if we expect to error out
        use_assert: bool = bool(expected_assert_code and expected_assert_msg)

        # Flip a coin and decide if we will submit the transaction directly or use sendrawtransaction
        use_srt: bool = bool(random.getrandbits(1)) and submit and (fundsAddr is not None) and not use_assert
        if use_srt:
            submit = False

        command = 'revoke'
        args = [
            self.proTxHash,
            self.keyOperator,
            reason,
        ]

        # fundsAddr is an optional field that results in different behavior if omitted, so we don't fallback here
        if fundsAddr is not None:
            args = args + [fundsAddr, submit] # type: ignore
        elif submit is not True:
            raise AssertionError("Cannot withhold transaction if relying on fundsAddr fallback behavior")

        if use_assert:
            assert_raises_rpc_error(expected_assert_code, expected_assert_msg, node.protx, command, *args)
            return None

        ret: str = node.protx(command, *args)
        if use_srt:
            ret = node.sendrawtransaction(ret)

        return ret

    def update_registrar(self, node: TestNode, submit: bool, pubKeyOperator: Optional[str] = None, votingAddr: Optional[str] = None,
                         rewards_address: Optional[str] = None, fundsAddr: Optional[str] = None,
                         expected_assert_code: Optional[int] = None, expected_assert_msg: Optional[str] = None) -> Optional[str]:
        if (expected_assert_code and not expected_assert_msg) or (not expected_assert_code and expected_assert_msg):
            raise AssertionError("Intending to use assert_raises_rpc_error() but didn't specify code and message")

        # Update commands should be run from the appropriate MasternodeInfo instance, we do not allow overriding proTxHash for this reason
        if self.proTxHash is None:
            raise AssertionError("proTxHash not set, did you call set_params()")

        # Use assert_raises_rpc_error if we expect to error out
        use_assert: bool = bool(expected_assert_code and expected_assert_msg)

        # Flip a coin and decide if we will submit the transaction directly or use sendrawtransaction
        use_srt: bool = bool(random.getrandbits(1)) and submit and (fundsAddr is not None) and not use_assert
        if use_srt:
            submit = False

        command = 'update_registrar_legacy' if self.legacy else 'update_registrar'
        args = [
            self.proTxHash,
            pubKeyOperator or self.pubKeyOperator,
            votingAddr or self.votingAddr,
            rewards_address or self.rewards_address,
        ]

        # fundsAddr is an optional field that results in different behavior if omitted, so we don't fallback here
        if fundsAddr is not None:
            args = args + [fundsAddr, submit] # type: ignore
        elif submit is not True:
            raise AssertionError("Cannot withhold transaction if relying on fundsAddr fallback behavior")

        if use_assert:
            assert_raises_rpc_error(expected_assert_code, expected_assert_msg, node.protx, command, *args)
            return None

        ret: str = node.protx(command, *args)
        if use_srt:
            ret = node.sendrawtransaction(ret)

        return ret

    def update_service(self, node: TestNode, submit: bool, coreP2PAddrs: Union[str, List[str], None] = None, platform_node_id: Optional[str] = None, platform_p2p_port: Optional[int] = None,
                       platform_http_port: Optional[int] = None, address_operator: Optional[str] = None, fundsAddr: Optional[str] = None,
                       expected_assert_code: Optional[int] = None, expected_assert_msg: Optional[str] = None) -> Optional[str]:
        if (expected_assert_code and not expected_assert_msg) or (not expected_assert_code and expected_assert_msg):
            raise AssertionError("Intending to use assert_raises_rpc_error() but didn't specify code and message")

        # Update commands should be run from the appropriate MasternodeInfo instance, we do not allow overriding some values for this reason
        if self.proTxHash is None:
            raise AssertionError("proTxHash not set, did you call set_params()")
        if self.keyOperator is None:
            raise AssertionError("keyOperator not set, did you call generate_addresses()")

        # EvoNode-specific fields are ignored for regular masternodes
        if self.evo:
            if platform_node_id is None:
                raise AssertionError("EvoNode but platform_node_id is missing, must be specified!")
            if platform_p2p_port is None:
                raise AssertionError("EvoNode but platform_p2p_port is missing, must be specified!")
            if platform_http_port is None:
                raise AssertionError("EvoNode but platform_http_port is missing, must be specified!")

        # Use assert_raises_rpc_error if we expect to error out
        use_assert: bool = bool(expected_assert_code and expected_assert_msg)

        # Flip a coin and decide if we will submit the transaction directly or use sendrawtransaction
        use_srt: bool = bool(random.getrandbits(1)) and submit and not use_assert
        if use_srt:
            submit = False

        # Common arguments shared between regular masternodes and EvoNodes
        args = [
            self.proTxHash,
            coreP2PAddrs or [f'127.0.0.1:{self.nodePort}'],
            self.keyOperator,
        ]
        address_funds = fundsAddr or self.fundsAddr
        address_operator = address_operator or ""

        # Construct final command and arguments
        if self.evo:
            command = "update_service_evo"
            args = args + [platform_node_id, platform_p2p_port, platform_http_port, address_operator, address_funds, submit] # type: ignore
        else:
            command = "update_service"
            args = args + [address_operator, address_funds, submit] # type: ignore

        if use_assert:
            assert_raises_rpc_error(expected_assert_code, expected_assert_msg, node.protx, command, *args)
            return None

        ret: str = node.protx(command, *args)
        if use_srt:
            ret = node.sendrawtransaction(ret)

        return ret

class DashTestFramework(BitcoinTestFramework):
    def set_test_params(self):
        """Tests must this method to change default values for number of nodes, topology, etc"""
        raise NotImplementedError

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        """Tests must override this method to define test logic"""
        raise NotImplementedError

    def add_nodes(self, num_nodes: int, extra_args=None, *, rpchost=None, binary=None, binary_cli=None, versions=None):
        old_num_nodes = len(self.nodes)
        super().add_nodes(num_nodes, extra_args, rpchost=rpchost, binary=binary, binary_cli=binary_cli, versions=versions)
        for i in range(old_num_nodes, old_num_nodes + num_nodes):
            append_config(self.nodes[i].datadir, [
                "dip3params=2:2",
                f"testactivationheight=v20@{self.v20_height}",
                f"testactivationheight=mn_rr@{self.mn_rr_height}",
            ])
        if old_num_nodes == 0:
            # controller node is the only node that has an extra option allowing it to submit sporks
            append_config(self.nodes[0].datadir, ["sporkkey=cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK"])

    def connect_nodes(self, a, b, *, peer_advertises_v2=None):
        for mn2 in self.mninfo: # type: MasternodeInfo
            if mn2.nodeIdx is not None:
                mn2.get_node(self).setmnthreadactive(False)
        super().connect_nodes(a, b, peer_advertises_v2=peer_advertises_v2)
        for mn2 in self.mninfo: # type: MasternodeInfo
            if mn2.nodeIdx is not None:
                mn2.get_node(self).setmnthreadactive(True)

    def set_dash_test_params(self, num_nodes, masterodes_count, extra_args=None, evo_count=0):
        self.mn_count = masterodes_count
        self.evo_count = evo_count
        self.num_nodes = num_nodes
        self.mninfo = []
        self.setup_clean_chain = True
        self.v20_height = 100
        self.mn_rr_height = 100
        # additional args
        if extra_args is None:
            extra_args = [[]] * num_nodes
        assert_equal(len(extra_args), num_nodes)
        self.extra_args = [copy.deepcopy(a) for a in extra_args]

        # LLMQ default test params (no need to pass -llmqtestparams)
        self.llmq_size = 3
        self.llmq_threshold = 2
        self.llmq_size_dip0024 = 4

        # This is nRequestTimeout in dash-q-recovery thread
        self.quorum_data_thread_request_timeout_seconds = 10
        # This is EXPIRATION_TIMEOUT + EXPIRATION_BIAS in CQuorumDataRequest
        self.quorum_data_request_expiration_timeout = 360

        # used by helper mine_cycle_quorum
        self.cycle_quorum_is_ready = False

    def delay_v20_and_mn_rr(self, height=None):
        self.v20_height = height
        self.mn_rr_height = height

    def activate_by_name(self, name, expected_activation_height=None, slow_mode=True):
        assert not softfork_active(self.nodes[0], name)
        self.log.info("Wait for " + name + " activation")

        # disable spork17 while mining blocks to activate "name" to prevent accidental quorum formation
        spork17_value = self.nodes[0].spork('show')['SPORK_17_QUORUM_DKG_ENABLED']
        self.bump_mocktime(1)
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 4070908800)
        self.wait_for_sporks_same()

        # mine blocks in batches
        batch_size = 50 if not slow_mode else 10
        if expected_activation_height is not None:
            height = self.nodes[0].getblockcount()
            assert height < expected_activation_height
            # NOTE: getblockchaininfo shows softforks active at block (window * 3 - 1)
            # since it's returning whether a softwork is active for the _next_ block.
            # Hence the last block prior to the activation is (expected_activation_height - 2).
            while expected_activation_height - height - 2 > batch_size:
                self.bump_mocktime(batch_size)
                self.generate(self.nodes[0], batch_size, sync_fun=lambda: self.sync_blocks())
                height += batch_size
            blocks_left = expected_activation_height - height - 2
            assert blocks_left <= batch_size
            self.bump_mocktime(blocks_left)
            self.generate(self.nodes[0], blocks_left, sync_fun=lambda: self.sync_blocks())
            assert not softfork_active(self.nodes[0], name)
            self.bump_mocktime(1)
            self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks())
        else:
            while not softfork_active(self.nodes[0], name):
                self.bump_mocktime(batch_size)
                self.generate(self.nodes[0], batch_size, sync_fun=lambda: self.sync_blocks())

        assert softfork_active(self.nodes[0], name)

        # revert spork17 changes
        self.bump_mocktime(1)
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", spork17_value)
        self.wait_for_sporks_same()

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
            self.extra_args[i].append("-llmqtestplatformparams=%d:%d" % (self.llmq_size, self.llmq_threshold))

    def create_simple_node(self, extra_args=None):
        idx = len(self.nodes)
        self.add_nodes(1, extra_args=[extra_args[idx]])
        self.start_node(idx)
        for i in range(0, idx):
            self.connect_nodes(i, idx)

    def dynamically_add_masternode(self, evo=False, rnd=None, should_be_rejected=False) -> Optional[MasternodeInfo]:
        mn_idx = len(self.nodes)

        node_p2p_port = p2p_port(mn_idx)
        node_rpc_port = rpc_port(mn_idx)

        protx_success = False
        try:
            created_mn_info = self.dynamically_prepare_masternode(mn_idx, node_p2p_port, evo, rnd)
            protx_success = True
        except:
            self.log.info("dynamically_prepare_masternode failed")

        assert_equal(protx_success, not should_be_rejected)

        if should_be_rejected:
            # nothing to do
            return None

        self.dynamically_initialize_datadir(node_p2p_port, node_rpc_port)
        node_info = self.add_dynamically_node(self.extra_args[0])

        args = ['-masternodeblsprivkey=%s' % created_mn_info.keyOperator] + node_info.extra_args
        self.start_node(mn_idx, args)

        for mn_info in self.mninfo: # type: MasternodeInfo
            if mn_info.proTxHash == created_mn_info.proTxHash:
                mn_info.set_node(mn_idx)

        self.connect_nodes(mn_idx, 0)

        self.wait_for_sporks_same()
        self.sync_blocks()
        force_finish_mnsync(self.nodes[mn_idx])

        self.log.info("Successfully started and synced proTx:"+str(created_mn_info.proTxHash))
        return created_mn_info

    def dynamically_prepare_masternode(self, idx, node_p2p_port, evo=False, rnd=None) -> MasternodeInfo:
        mn = MasternodeInfo(evo=evo, legacy=(not softfork_active(self.nodes[0], 'v19')))
        mn.generate_addresses(self.nodes[0])

        platform_node_id = hash160(b'%d' % rnd).hex() if rnd is not None else hash160(b'%d' % node_p2p_port).hex()
        platform_p2p_port = node_p2p_port + 101
        platform_http_port = node_p2p_port + 102

        outputs = {mn.collateral_address: mn.get_collateral_value(), mn.fundsAddr: 1}
        collateral_txid = self.nodes[0].sendmany("", outputs)
        self.bump_mocktime(10 * 60 + 1) # to make tx safe to include in block
        mn.bury_tx(self, genIdx=0, txid=collateral_txid, depth=1)
        collateral_vout = mn.get_collateral_vout(self.nodes[0], collateral_txid)

        coreP2PAddrs = ['127.0.0.1:%d' % node_p2p_port]
        operatorReward = idx

        # platform_node_id, platform_p2p_port and platform_http_port are ignored for regular masternodes
        protx_result = mn.register(self.nodes[0], submit=True, collateral_txid=collateral_txid, collateral_vout=collateral_vout, coreP2PAddrs=coreP2PAddrs, operator_reward=operatorReward,
                                   platform_node_id=platform_node_id, platform_p2p_port=platform_p2p_port, platform_http_port=platform_http_port)
        assert protx_result is not None

        self.bump_mocktime(10 * 60 + 1) # to make tx safe to include in block
        mn.bury_tx(self, genIdx=0, txid=protx_result, depth=1)

        mn.set_params(proTxHash=protx_result, operator_reward=operatorReward, collateral_txid=collateral_txid, collateral_vout=collateral_vout, nodePort=node_p2p_port)
        self.mninfo.append(mn)

        mn_type_str = "EvoNode" if evo else "MN"
        self.log.info("Prepared %s %d: collateral_txid=%s, collateral_vout=%d, protxHash=%s" % (mn_type_str, idx, collateral_txid, collateral_vout, protx_result))
        return mn

    def dynamically_evo_update_service(self, evo_info: MasternodeInfo, rnd=None, should_be_rejected=False):
        funds_address = self.nodes[0].getnewaddress()
        operator_reward_address = self.nodes[0].getnewaddress()

        # For the sake of the test, generate random nodeid, p2p and http platform values
        r = rnd if rnd is not None else random.randint(21000, 65000)
        platform_node_id = hash160(b'%d' % r).hex()
        platform_p2p_port = r + 1
        platform_http_port = r + 2

        fund_txid = self.nodes[0].sendtoaddress(funds_address, 1)
        self.bump_mocktime(10 * 60 + 1) # to make tx safe to include in block
        evo_info.bury_tx(self, genIdx=0, txid=fund_txid, depth=1)

        protx_success = False
        try:
            protx_result = evo_info.update_service(self.nodes[0], True, f'127.0.0.1:{evo_info.nodePort}', platform_node_id, platform_p2p_port, platform_http_port, operator_reward_address, funds_address)
            assert protx_result is not None
            self.bump_mocktime(10 * 60 + 1) # to make tx safe to include in block
            evo_info.bury_tx(self, genIdx=0, txid=protx_result, depth=1)
            self.log.info("Updated EvoNode %s: platformNodeID=%s, platformP2PPort=%s, platformHTTPPort=%s" % (evo_info.proTxHash, platform_node_id, platform_p2p_port, platform_http_port))
            protx_success = True
        except:
            self.log.info("protx_evo rejected")

        assert_equal(protx_success, not should_be_rejected)

    def prepare_masternodes(self):
        self.log.info("Preparing %d masternodes" % self.mn_count)
        for idx in range(0, self.mn_count):
            self.prepare_masternode(idx)
        self.sync_all()

    def prepare_masternode(self, idx):
        mn = MasternodeInfo(evo=False, legacy=(not softfork_active(self.nodes[0], 'v19')))
        mn.generate_addresses(self.nodes[0])

        txid = self.nodes[0].sendtoaddress(mn.fundsAddr, mn.get_collateral_value())
        collateral_vout = 0
        register_fund = (idx % 2) == 0
        if not register_fund:
            collateral_vout = mn.get_collateral_vout(self.nodes[0], txid)
            self.nodes[0].lockunspent(False, [{'txid': txid, 'vout': collateral_vout}])

        # send to same address to reserve some funds for fees
        self.nodes[0].sendtoaddress(mn.fundsAddr, 0.001)

        port = p2p_port(len(self.nodes) + idx)
        coreP2PAddrs = ['127.0.0.1:%d' % port]
        operatorReward = idx

        submit = (idx % 4) < 2

        if register_fund:
            protx_result = mn.register_fund(self.nodes[0], submit=submit, coreP2PAddrs=coreP2PAddrs, operator_reward=operatorReward)
        else:
            self.generate(self.nodes[0], 1, sync_fun=self.no_op)
            protx_result = mn.register(self.nodes[0], submit=submit, collateral_txid=txid, collateral_vout=collateral_vout, coreP2PAddrs=coreP2PAddrs,
                                       operator_reward=operatorReward)
        if submit:
            proTxHash = protx_result
        else:
            proTxHash = self.nodes[0].sendrawtransaction(protx_result)

        mn.set_params(proTxHash=proTxHash, operator_reward=operatorReward, collateral_txid=txid, collateral_vout=collateral_vout, nodePort=port)

        if operatorReward > 0:
            self.generate(self.nodes[0], 1, sync_fun=self.no_op)
            operatorPayoutAddress = self.nodes[0].getnewaddress()
            mn.update_service(self.nodes[0], submit=True, coreP2PAddrs=coreP2PAddrs, address_operator=operatorPayoutAddress)

        self.mninfo.append(mn)
        self.log.info("Prepared MN %d: collateral_txid=%s, collateral_vout=%d, protxHash=%s" % (idx, txid, collateral_vout, proTxHash))

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

        jobs = []

        # start up nodes in parallel
        for idx in range(0, self.mn_count):
            self.mninfo[idx].set_node(start_idx + idx)
            jobs.append(executor.submit(self.start_masternode, self.mninfo[idx]))

        # wait for all nodes to start up
        for job in jobs:
            job.result()
        jobs.clear()

        executor.shutdown()

        # Connect to the control node only, masternodes should take care of intra-quorum connections themselves
        for idx in range(0, self.mn_count):
            self.connect_nodes(self.mninfo[idx].nodeIdx, 0)

    def start_masternode(self, mninfo: MasternodeInfo, extra_args=None):
        args = ['-masternodeblsprivkey=%s' % mninfo.keyOperator] + self.extra_args[mninfo.nodeIdx]
        if extra_args is not None:
            args += extra_args
        self.start_node(mninfo.nodeIdx, extra_args=args)
        force_finish_mnsync(mninfo.get_node(self))

    def dynamically_start_masternode(self, mnidx, extra_args=None):
        args = []
        if extra_args is not None:
            args += extra_args
        self.start_node(mnidx, extra_args=args)
        force_finish_mnsync(self.nodes[mnidx])

    def setup_nodes(self):
        self.log.info("Creating and starting controller node")
        num_simple_nodes = self.num_nodes - self.mn_count
        self.log.info("Creating and starting %s simple nodes", num_simple_nodes)
        for i in range(0, num_simple_nodes):
            self.create_simple_node(self.extra_args)
        if self.requires_wallet:
            self.import_deterministic_coinbase_privkeys()
        required_balance = EVONODE_COLLATERAL * self.evo_count
        required_balance += MASTERNODE_COLLATERAL * (self.mn_count - self.evo_count) + 100
        self.log.info("Generating %d coins" % required_balance)
        while self.nodes[0].getbalance() < required_balance:
            self.bump_mocktime(1)
            self.generate(self.nodes[0], 10, sync_fun=self.no_op)

        # create masternodes
        self.prepare_masternodes()
        self.prepare_datadirs()

    def setup_network(self):
        self.setup_nodes()

        # non-masternodes where disconnected from the control node during prepare_datadirs,
        # let's reconnect them back to make sure they receive updates
        num_simple_nodes = self.num_nodes - self.mn_count
        for i in range(1, num_simple_nodes):
            self.connect_nodes(i, 0)

        self.start_masternodes()

        # it should be at least 8 blocks since v20 when MN can be used in quorums
        self.bump_mocktime(8)
        self.generate(self.nodes[0], 8)
        for i in range(1, num_simple_nodes):
            force_finish_mnsync(self.nodes[i])

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

        # helper which has been supposed to be removed with bitcoin#20159 but we use it
        def make_change(from_node, amount_in, amount_out, fee):
            """
            Create change output(s), return them
            """
            outputs = {}
            amount = amount_out + fee
            change = amount_in - amount
            if change > amount * 2:
                # Create an extra change output to break up big inputs
                change_address = from_node.getnewaddress()
                # Split change in two, being careful of rounding:
                outputs[change_address] = Decimal(change / 2).quantize(Decimal('0.00000001'), rounding=ROUND_DOWN)
                change = amount_in - amount - outputs[change_address]
            if change > 0:
                outputs[from_node.getnewaddress()] = change
            return outputs


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

    def create_isdlock(self, hextx):
        tx = tx_from_hex(hextx)
        tx.rehash()

        request_id_buf = ser_string(b"islock") + ser_compact_size(len(tx.vin))
        inputs = []
        for txin in tx.vin:
            request_id_buf += txin.prevout.serialize()
            inputs.append(txin.prevout)
        request_id = hash256(request_id_buf)[::-1].hex()
        message_hash = tx.hash

        llmq_type = 103

        rec_sig = self.get_recovered_sig(request_id, message_hash, llmq_type=llmq_type)

        block_count = self.mninfo[0].get_node(self).getblockcount()
        cycle_hash = int(self.mninfo[0].get_node(self).getblockhash(block_count - (block_count % 24)), 16)
        isdlock = msg_isdlock(1, inputs, tx.sha256, cycle_hash, bytes.fromhex(rec_sig['sig']))

        return isdlock

    # due to privacy reasons random delay is used before sending transaction by network
    # most times is just 2-5 seconds, but once in 1000 it's up to 1000 seconds.
    # it's recommended to bump mocktime for 30 seconds before wait_for_instantlock
    def wait_for_instantlock(self, txid, node, timeout=60):

        def check_instantlock():
            try:
                return node.getrawtransaction(txid, True)["instantlock"]
            except:
                return False

        self.log.info(f"Expecting InstantLock for {txid}")
        self.wait_until(check_instantlock, timeout=timeout)

    def wait_for_chainlocked_block(self, node, block_hash, expected=True, timeout=15):
        def check_chainlocked_block():
            try:
                block = node.getblock(block_hash)
                return block["confirmations"] > 0 and block["chainlock"]
            except:
                return False
        self.log.info(f"Expecting ChainLock for {block_hash}")
        if self.wait_until(check_chainlocked_block, timeout=timeout, do_assert=expected) and not expected:
            raise AssertionError("waiting unexpectedly succeeded")

    def wait_for_chainlocked_block_all_nodes(self, block_hash, timeout=15, expected=True):
        for node in self.nodes:
            self.wait_for_chainlocked_block(node, block_hash, expected=expected, timeout=timeout)

    def wait_for_best_chainlock(self, node, block_hash, timeout=15):
        self.wait_until(lambda: node.getbestchainlock()["blockhash"] == block_hash, timeout=timeout)

    def wait_for_sporks_same(self, timeout=30):
        def check_sporks_same():
            self.bump_mocktime(1)
            sporks = self.nodes[0].spork('show')
            return all(node.spork('show') == sporks for node in self.nodes[1:])
        self.wait_until(check_sporks_same, timeout=timeout, sleep=1)

    def wait_for_quorum_connections(self, quorum_hash, expected_connections, mninfos, llmq_type_name="llmq_test", timeout = 60, wait_proc=None):
        def check_quorum_connections():
            def ret():
                if wait_proc is not None:
                    wait_proc()
                return False

            for mn in mninfos:
                s = mn.get_node(self).quorum("dkgstatus")
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

        self.wait_until(check_quorum_connections, timeout=timeout, sleep=1)

    def wait_for_masternode_probes(self, quorum_hash, mninfos, timeout = 30, wait_proc=None, llmq_type_name="llmq_test"):
        def check_probes():
            def ret():
                if wait_proc is not None:
                    wait_proc()
                return False

            for mn in mninfos:
                s = mn.get_node(self).quorum('dkgstatus')
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
                                mn2 = mn.get_node(self).protx('info', c["proTxHash"])
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

        self.wait_until(check_probes, timeout=timeout, sleep=1)

    def wait_for_quorum_phase(self, quorum_hash, phase, expected_member_count, check_received_messages, check_received_messages_count, mninfos, llmq_type_name="llmq_test", timeout=30, sleep=0.5):
        def check_dkg_session():
            member_count = 0
            for mn in mninfos:
                s = mn.get_node(self).quorum("dkgstatus")["session"]
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

        self.wait_until(check_dkg_session, timeout=timeout, sleep=sleep)

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
                    if c["quorumPublicKey"] == '0' * 96:
                        continue
                    c_ok = True
                    break
                if not c_ok:
                    return False
            return True

        self.wait_until(check_dkg_comitments, timeout=timeout)

    def wait_for_quorum_list(self, quorum_hash, nodes, timeout=15, llmq_type_name="llmq_test"):
        def wait_func():
            return quorum_hash in self.nodes[0].quorum('list')[llmq_type_name]
        self.log.info(f"quorums: {self.nodes[0].quorum('list')}")
        self.wait_until(wait_func, timeout=timeout, sleep=0.05)

    def wait_for_quorums_list(self, quorum_hash_0, quorum_hash_1, nodes, llmq_type_name="llmq_test",  timeout=15):
        def wait_func():
            quorums = self.nodes[0].quorum("list")[llmq_type_name]
            return quorum_hash_0 in quorums and quorum_hash_1 in quorums
        self.log.info(f"h({self.nodes[0].getblockcount()}) quorums: {self.nodes[0].quorum('list')}")
        self.wait_until(wait_func, timeout=timeout, sleep=0.05)

    def move_blocks(self, nodes, num_blocks):
        self.bump_mocktime(1, nodes=nodes)
        self.generate(self.nodes[0], num_blocks, sync_fun=lambda: self.sync_blocks(nodes))

    def mine_quorum(self, llmq_type_name="llmq_test", llmq_type=100, expected_connections=None, expected_members=None, expected_contributions=None, expected_complaints=0, expected_justifications=0, expected_commitments=None, mninfos_online=None, mninfos_valid=None, skip_maturity=False):
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

        nodes = [self.nodes[0]] + [mn.get_node(self) for mn in mninfos_online]

        # move forward to next DKG
        skip_count = 24 - (self.nodes[0].getblockcount() % 24)
        if skip_count != 0:
            self.bump_mocktime(1)
            self.generate(self.nodes[0], skip_count, sync_fun=lambda: self.sync_blocks(nodes))

        q = self.nodes[0].getbestblockhash()
        self.log.info("Expected quorum_hash:"+str(q))
        self.log.info("Waiting for phase 1 (init)")
        self.wait_for_quorum_phase(q, 1, expected_members, None, 0, mninfos_online, llmq_type_name=llmq_type_name)
        self.wait_for_quorum_connections(q, expected_connections, mninfos_online, wait_proc=lambda: self.bump_mocktime(1), llmq_type_name=llmq_type_name)
        if spork23_active:
            self.wait_for_masternode_probes(q, mninfos_online, wait_proc=lambda: self.bump_mocktime(1))

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
        self.bump_mocktime(1)
        self.nodes[0].getblocktemplate() # this calls CreateNewBlock
        self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks(nodes))

        self.log.info("Waiting for quorum to appear in the list")
        self.wait_for_quorum_list(q, nodes, llmq_type_name=llmq_type_name)

        new_quorum = self.nodes[0].quorum("list", 1)[llmq_type_name][0]
        assert_equal(q, new_quorum)
        quorum_info = self.nodes[0].quorum("info", llmq_type, new_quorum)

        if not skip_maturity:
            # Mine 8 (SIGN_HEIGHT_OFFSET) more blocks to make sure that the new quorum gets eligible for signing sessions
            self.generate(self.nodes[0], 8, sync_fun=lambda: self.sync_blocks(nodes))

        self.log.info(f"New quorum: height={quorum_info['height']}, quorumHash={new_quorum}, is_mature={not skip_maturity} quorumIndex={quorum_info['quorumIndex']}, minedBlock={quorum_info['minedBlock']}")

        for mn in mninfos_valid:
            assert not check_punished(self.nodes[0], mn)
            assert not check_banned(self.nodes[0], mn)

        return new_quorum

    def mine_cycle_quorum(self):
        spork21_active = self.nodes[0].spork('show')['SPORK_21_QUORUM_ALL_CONNECTED'] <= 1
        spork23_active = self.nodes[0].spork('show')['SPORK_23_QUORUM_POSE'] <= 1

        llmq_type_name="llmq_test_dip0024"
        llmq_type=103
        expected_connections = (self.llmq_size_dip0024 - 1) if spork21_active else 2
        expected_members = self.llmq_size_dip0024
        expected_contributions = self.llmq_size_dip0024
        expected_commitments = self.llmq_size_dip0024
        mninfos_online = self.mninfo.copy()
        expected_complaints=0
        expected_justifications=0

        self.log.info(f"Mining quorum: expected_members={expected_members}, expected_connections={expected_connections}, expected_contributions={expected_contributions}, expected_commitments={expected_commitments}, no complains and justfications expected")

        nodes = [self.nodes[0]] + [mn.get_node(self) for mn in mninfos_online]

        cycle_length = 24
        cur_block = self.nodes[0].getblockcount()

        skip_count = cycle_length - (cur_block % cycle_length)
        # move forward to next 3 DKG rounds for the first quorum
        extra_blocks = 0 if self.cycle_quorum_is_ready else 24 * 3
        self.move_blocks(nodes, extra_blocks + skip_count)
        self.log.info('Moved from block %d to %d' % (cur_block, self.nodes[0].getblockcount()))

        self.cycle_quorum_is_ready = True

        q_0 = self.nodes[0].getbestblockhash()
        self.log.info("Expected quorum_0 at:" + str(self.nodes[0].getblockcount()))
        self.log.info("Expected quorum_0 hash:" + str(q_0))
        self.log.info("quorumIndex 0: Waiting for phase 1 (init)")
        self.wait_for_quorum_phase(q_0, 1, expected_members, None, 0, mninfos_online, llmq_type_name)
        self.log.info("quorumIndex 0: Waiting for quorum connections (init)")
        self.wait_for_quorum_connections(q_0, expected_connections, mninfos_online, llmq_type_name, wait_proc=lambda: self.bump_mocktime(1))
        if spork23_active:
            self.wait_for_masternode_probes(q_0, mninfos_online, wait_proc=lambda: self.bump_mocktime(1), llmq_type_name=llmq_type_name)

        self.move_blocks(nodes, 1)

        q_1 = self.nodes[0].getbestblockhash()
        self.log.info("Expected quorum_1 at:" + str(self.nodes[0].getblockcount()))
        self.log.info("Expected quorum_1 hash:" + str(q_1))
        self.log.info("quorumIndex 1: Waiting for phase 1 (init)")
        self.wait_for_quorum_phase(q_1, 1, expected_members, None, 0, mninfos_online, llmq_type_name)
        self.log.info("quorumIndex 1: Waiting for quorum connections (init)")
        self.wait_for_quorum_connections(q_1, expected_connections, mninfos_online, llmq_type_name, wait_proc=lambda: self.bump_mocktime(1))
        if spork23_active:
            self.wait_for_masternode_probes(q_1, mninfos_online, wait_proc=lambda: self.bump_mocktime(1), llmq_type_name=llmq_type_name)

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
        self.log.info("Mining final commitments")
        self.bump_mocktime(1)
        self.nodes[0].getblocktemplate() # this calls CreateNewBlock
        self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks(nodes))

        self.log.info("Waiting for quorum(s) to appear in the list")
        self.wait_for_quorums_list(q_0, q_1, nodes, llmq_type_name)

        quorum_info_0 = self.nodes[0].quorum("info", llmq_type, q_0)
        quorum_info_1 = self.nodes[0].quorum("info", llmq_type, q_1)
        # Mine 8 (SIGN_HEIGHT_OFFSET) more blocks to make sure that the new quorum gets eligible for signing sessions
        self.generate(self.nodes[0], 8, sync_fun=lambda: self.sync_blocks(nodes))

        self.log.info("New quorum: height=%d, quorumHash=%s, quorumIndex=%d, minedBlock=%s" % (quorum_info_0["height"], q_0, quorum_info_0["quorumIndex"], quorum_info_0["minedBlock"]))
        self.log.info("New quorum: height=%d, quorumHash=%s, quorumIndex=%d, minedBlock=%s" % (quorum_info_1["height"], q_1, quorum_info_1["quorumIndex"], quorum_info_1["minedBlock"]))

        self.log.info("quorum_info_0:"+str(quorum_info_0))
        self.log.info("quorum_info_1:"+str(quorum_info_1))

        best_block_hash = self.nodes[0].getbestblockhash()
        block_height = self.nodes[0].getblockcount()
        quorum_rotation_info = self.nodes[0].quorum("rotationinfo", best_block_hash)
        self.log.info("h("+str(block_height)+"):"+str(quorum_rotation_info))

        return (quorum_info_0, quorum_info_1)

    def wait_for_recovered_sig(self, rec_sig_id, rec_sig_msg_hash, llmq_type=100, timeout=10):
        def check_recovered_sig():
            self.bump_mocktime(1)
            for mn in self.mninfo: # type: MasternodeInfo
                if not mn.get_node(self).quorum("hasrecsig", llmq_type, rec_sig_id, rec_sig_msg_hash):
                    return False
            return True
        self.wait_until(check_recovered_sig, timeout=timeout, sleep=1)

    def get_recovered_sig(self, rec_sig_id, rec_sig_msg_hash, llmq_type=100, use_platformsign=False):
        # Note: recsigs aren't relayed to regular nodes by default,
        # make sure to pick a mn as a node to query for recsigs.
        try:
            quorumHash = self.mninfo[0].get_node(self).quorum("selectquorum", llmq_type, rec_sig_id)["quorumHash"]
            for mn in self.mninfo: # type: MasternodeInfo
                if use_platformsign:
                    mn.get_node(self).quorum("platformsign", rec_sig_id, rec_sig_msg_hash, quorumHash)
                else:
                    mn.get_node(self).quorum("sign", llmq_type, rec_sig_id, rec_sig_msg_hash, quorumHash)
            self.wait_for_recovered_sig(rec_sig_id, rec_sig_msg_hash, llmq_type, 10)
            return self.mninfo[0].get_node(self).quorum("getrecsig", llmq_type, rec_sig_id, rec_sig_msg_hash)
        except JSONRPCException as e:
            self.log.info(f"getrecsig failed with '{e}'")
        assert False

    def get_quorum_masternodes(self, q, llmq_type=100) -> List[Optional[MasternodeInfo]]:
        qi = self.nodes[0].quorum('info', llmq_type, q)
        result = []
        for m in qi['members']:
            result.append(self.get_mninfo(m['proTxHash']))
        return result

    def get_mninfo(self, proTxHash) -> Optional[MasternodeInfo]:
        for mn in self.mninfo: # type: MasternodeInfo
            if mn.proTxHash == proTxHash:
                return mn
        return None

    def test_mn_quorum_data(self, test_mn: MasternodeInfo, quorum_type_in, quorum_hash_in, test_secret=True, expect_secret=True):
        quorum_info = test_mn.get_node(self).quorum("info", quorum_type_in, quorum_hash_in, True)
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
                    self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks())
                else:
                    self.bump_mocktime(self.quorum_data_thread_request_timeout_seconds + 1)

            for test_mn in mns:
                valid += self.test_mn_quorum_data(test_mn, quorum_type_in, quorum_hash_in, test_secret, expect_secret)
            self.log.debug("wait_for_quorum_data: %d/%d - quorum_type=%d quorum_hash=%s" %
                           (valid, len(mns), quorum_type_in, quorum_hash_in))
            return valid == len(mns)

        self.wait_until(test_mns, timeout=timeout, sleep=0.5)

    def wait_for_mnauth(self, node, count, timeout=10):
        def test():
            pi = node.getpeerinfo()
            c = 0
            for p in pi:
                if "verified_proregtx_hash" in p and p["verified_proregtx_hash"] != "":
                    c += 1
            return c >= count
        self.wait_until(test, timeout=timeout)
