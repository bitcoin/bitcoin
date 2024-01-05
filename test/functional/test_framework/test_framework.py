#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
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
import types

from .address import create_deterministic_address_bcrt1_p2tr_op_true
from .authproxy import JSONRPCException
from . import coverage
from .p2p import NetworkThread
from .test_node import TestNode
from .util import (
    MAX_NODES,
    PortSeed,
    assert_equal,
    check_json_precision,
    find_vout_for_address,
    get_datadir_path,
    initialize_datadir,
    p2p_port,
    wait_until_helper_internal,
)


class TestStatus(Enum):
    PASSED = 1
    FAILED = 2
    SKIPPED = 3

TEST_EXIT_PASSED = 0
TEST_EXIT_FAILED = 1
TEST_EXIT_SKIPPED = 77

TMPDIR_PREFIX = "bitcoin_func_test_"


class SkipTest(Exception):
    """This exception is raised to skip a test"""

    def __init__(self, message):
        self.message = message


class Binaries:
    """Helper class to provide information about bitcoin binaries

    Attributes:
        paths: Object returned from get_binary_paths() containing information
            which binaries and command lines to use from environment variables and
            the config file.
        bin_dir: An optional string containing a directory path to look for
            binaries, which takes precedence over the paths above, if specified.
            This is used by tests calling binaries from previous releases.
    """
    def __init__(self, paths, bin_dir):
        self.paths = paths
        self.bin_dir = bin_dir

    def daemon_argv(self):
        "Return argv array that should be used to invoke bitcoind"
        return self._argv(self.paths.bitcoind)

    def rpc_argv(self):
        "Return argv array that should be used to invoke bitcoin-cli"
        return self._argv(self.paths.bitcoincli)

    def util_argv(self):
        "Return argv array that should be used to invoke bitcoin-util"
        return self._argv(self.paths.bitcoinutil)

    def wallet_argv(self):
        "Return argv array that should be used to invoke bitcoin-wallet"
        return self._argv(self.paths.bitcoinwallet)

    def chainstate_argv(self):
        "Return argv array that should be used to invoke bitcoin-chainstate"
        return self._argv(self.paths.bitcoinchainstate)

    def _argv(self, bin_path):
        """Return argv array that should be used to invoke the command.
        Normally this will return binary paths directly from the paths object,
        but when bin_dir is set (by tests calling binaries from previous
        releases) it will return paths relative to bin_dir instead."""
        if self.bin_dir is not None:
            return [os.path.join(self.bin_dir, os.path.basename(bin_path))]
        else:
            return [bin_path]


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

    def __init__(self, test_file) -> None:
        """Sets test framework defaults. Do not override this method. Instead, override the set_test_params() method"""
        self.chain: str = 'regtest'
        self.setup_clean_chain: bool = False
        self.noban_tx_relay: bool = False
        self.nodes: list[TestNode] = []
        self.extra_args = None
        self.network_thread = None
        self.rpc_timeout = 60  # Wait for up to 60 seconds for the RPC server to respond
        self.supports_cli = True
        self.bind_to_localhost_only = True
        self.parse_args(test_file)
        self.default_wallet_name = "default_wallet"
        self.wallet_data_filename = "wallet.dat"
        # Optional list of wallet names that can be set in set_test_params to
        # create and import keys to. If unset, default is len(nodes) *
        # [default_wallet_name]. If wallet names are None, wallet creation is
        # skipped. If list is truncated, wallet creation is skipped and keys
        # are not imported.
        self.wallet_names = None
        # By default the wallet is not required. Set to true by skip_if_no_wallet().
        # Can also be set to None to indicate that the wallet will be used if available.
        # When False or None, we ignore wallet_names regardless of what it is.
        self.uses_wallet = False
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
            if self.options.test_methods:
                self.run_test_methods()
            else:
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

    def run_test_methods(self):
        for method_name in self.options.test_methods:
            self.log.info(f"Attempting to execute method: {method_name}")
            method = getattr(self, method_name)
            method()
            self.log.info(f"Method '{method_name}' executed successfully.")

    def parse_args(self, test_file):
        previous_releases_path = os.getenv("PREVIOUS_RELEASES_DIR") or os.getcwd() + "/releases"
        parser = argparse.ArgumentParser(usage="%(prog)s [options]")
        parser.add_argument("--nocleanup", dest="nocleanup", default=False, action="store_true",
                            help="Leave bitcoinds and test.* datadir on exit or error")
        parser.add_argument("--cachedir", dest="cachedir", default=os.path.abspath(os.path.dirname(test_file) + "/../cache"),
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
                            default=os.path.abspath(os.path.dirname(test_file) + "/../config.ini"),
                            help="Location of the test framework config file (default: %(default)s)")
        parser.add_argument("--pdbonfailure", dest="pdbonfailure", default=False, action="store_true",
                            help="Attach a python debugger if test fails")
        parser.add_argument("--usecli", dest="usecli", default=False, action="store_true",
                            help="use bitcoin-cli instead of RPC for all commands")
        parser.add_argument("--perf", dest="perf", default=False, action="store_true",
                            help="profile running nodes with perf for the duration of the test")
        parser.add_argument("--valgrind", dest="valgrind", default=False, action="store_true",
                            help="run nodes under the valgrind memory error detector: expect at least a ~10x slowdown. valgrind 3.14 or later required. Does not apply to previous release binaries.")
        parser.add_argument("--randomseed", type=int,
                            help="set a random seed for deterministically reproducing a previous test run")
        parser.add_argument("--timeout-factor", dest="timeout_factor", type=float, help="adjust test timeouts by a factor. Setting it to 0 disables all timeouts")
        parser.add_argument("--v2transport", dest="v2transport", default=False, action="store_true",
                            help="use BIP324 v2 connections between all nodes by default")
        parser.add_argument("--v1transport", dest="v1transport", default=False, action="store_true",
                            help="Explicitly use v1 transport (can be used to overwrite global --v2transport option)")
        parser.add_argument("--test_methods", dest="test_methods", nargs='*',
                            help="Run specified test methods sequentially instead of the full test. Use only for methods that do not depend on any context set up in run_test or other methods.")

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
        self.binary_paths = self.get_binary_paths()
        if self.options.v1transport:
            self.options.v2transport=False

        PortSeed.n = self.options.port_seed

    def get_binary_paths(self):
        """Get paths of all binaries from environment variables or their default values"""

        paths = types.SimpleNamespace()
        binaries = {
            "bitcoind": ("bitcoind", "BITCOIND"),
            "bitcoin-cli": ("bitcoincli", "BITCOINCLI"),
            "bitcoin-util": ("bitcoinutil", "BITCOINUTIL"),
            "bitcoin-chainstate": ("bitcoinchainstate", "BITCOINCHAINSTATE"),
            "bitcoin-wallet": ("bitcoinwallet", "BITCOINWALLET"),
        }
        for binary, [attribute_name, env_variable_name] in binaries.items():
            default_filename = os.path.join(
                self.config["environment"]["BUILDDIR"],
                "bin",
                binary + self.config["environment"]["EXEEXT"],
            )
            setattr(paths, attribute_name, os.getenv(env_variable_name, default=default_filename))
        return paths

    def get_binaries(self, bin_dir=None):
        return Binaries(self.binary_paths, bin_dir)

    def setup(self):
        """Call this method to start up the test framework object with options set."""

        check_json_precision()

        self.options.cachedir = os.path.abspath(self.options.cachedir)

        config = self.config

        os.environ['PATH'] = os.pathsep.join([
            os.path.join(config['environment']['BUILDDIR'], 'bin'),
            os.environ['PATH']
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
        if self.success == TestStatus.FAILED:
            self.log.info("Not stopping nodes as test failed. The dangling processes will be cleaned up later.")
        else:
            self.log.info("Stopping nodes")
            if self.nodes:
                self.stop_nodes()

        should_clean_up = (
            not self.options.nocleanup and
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
            self.log.error(self.config['environment']['CLIENT_BUGREPORT'])
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
        if self.uses_wallet:
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
                n.createwallet(wallet_name=wallet_name, load_on_startup=True)
            n.importprivkey(privkey=n.get_deterministic_priv_key().key, label='coinbase', rescan=True)

    # Only enables wallet support when the module is available
    def enable_wallet_if_possible(self):
        self._requires_wallet = self.is_wallet_compiled()

    def run_test(self):
        """Tests must override this method to define test logic"""
        raise NotImplementedError

    # Public helper methods. These can be accessed by the subclass test scripts.

    def add_nodes(self, num_nodes: int, extra_args=None, *, rpchost=None, versions=None):
        """Instantiate TestNode objects.

        Should only be called once after the nodes have been specified in
        set_test_params()."""
        def bin_dir_from_version(version):
            if not version:
                return None
            if version > 219999:
                # Starting at client version 220000 the first two digits represent
                # the major version, e.g. v22.0 instead of v0.22.0.
                version *= 100
            return os.path.join(
                self.options.previous_releases_path,
                re.sub(
                    r'\.0$' if version <= 219999 else r'(\.0){1,2}$',
                    '', # Remove trailing dot for point releases, after 22.0 also remove double trailing dot.
                    'v{}.{}.{}.{}'.format(
                        (version % 100000000) // 1000000,
                        (version % 1000000) // 10000,
                        (version % 10000) // 100,
                        (version % 100) // 1,
                    ),
                ),
                'bin',
            )

        if self.bind_to_localhost_only:
            extra_confs = [["bind=127.0.0.1"]] * num_nodes
        else:
            extra_confs = [[]] * num_nodes
        if extra_args is None:
            extra_args = [[]] * num_nodes
        # Whitelist peers to speed up tx relay / mempool sync. Don't use it if testing tx relay or timing.
        if self.noban_tx_relay:
            for i in range(len(extra_args)):
                extra_args[i] = extra_args[i] + ["-whitelist=noban,in,out@127.0.0.1"]
        if versions is None:
            versions = [None] * num_nodes
        bin_dirs = [bin_dir_from_version(v) for v in versions]
        # Fail test if any of the needed release binaries is missing
        bins_missing = False
        for bin_path in (argv[0] for bin_dir in bin_dirs
                                 for binaries in (self.get_binaries(bin_dir),)
                                 for argv in (binaries.daemon_argv(), binaries.rpc_argv())):
            if shutil.which(bin_path) is None:
                self.log.error(f"Binary not found: {bin_path}")
                bins_missing = True
        if bins_missing:
            raise AssertionError("At least one release binary is missing. "
                                 "Previous releases binaries can be downloaded via `test/get_previous_releases.py -b`.")
        assert_equal(len(extra_confs), num_nodes)
        assert_equal(len(extra_args), num_nodes)
        assert_equal(len(versions), num_nodes)
        assert_equal(len(bin_dirs), num_nodes)
        for i in range(num_nodes):
            args = list(extra_args[i])
            test_node_i = TestNode(
                i,
                get_datadir_path(self.options.tmpdir, i),
                chain=self.chain,
                rpchost=rpchost,
                timewait=self.rpc_timeout,
                timeout_factor=self.options.timeout_factor,
                binaries=self.get_binaries(bin_dirs[i]),
                version=versions[i],
                coverage_dir=self.options.coveragedir,
                cwd=self.options.tmpdir,
                extra_conf=extra_confs[i],
                extra_args=args,
                use_cli=self.options.usecli,
                start_perf=self.options.perf,
                use_valgrind=self.options.valgrind,
                v2transport=self.options.v2transport,
                uses_wallet=self.uses_wallet,
            )
            self.nodes.append(test_node_i)
            if not test_node_i.version_is_at_least(170000):
                # adjust conf for pre 17
                test_node_i.replace_in_config([('[regtest]', '')])

    def start_node(self, i, *args, **kwargs):
        """Start a bitcoind"""

        node = self.nodes[i]

        node.start(*args, **kwargs)
        node.wait_for_rpc_connection()

        if self.options.coveragedir is not None:
            coverage.write_all_rpc_commands(self.options.coveragedir, node.rpc)

    def start_nodes(self, extra_args=None, *args, **kwargs):
        """Start multiple bitcoinds"""

        if extra_args is None:
            extra_args = [None] * self.num_nodes
        assert_equal(len(extra_args), self.num_nodes)
        for i, node in enumerate(self.nodes):
            node.start(extra_args[i], *args, **kwargs)
        for node in self.nodes:
            node.wait_for_rpc_connection()

        if self.options.coveragedir is not None:
            for node in self.nodes:
                coverage.write_all_rpc_commands(self.options.coveragedir, node.rpc)

    def stop_node(self, i, expected_stderr='', wait=0):
        """Stop a bitcoind test node"""
        self.nodes[i].stop_node(expected_stderr, wait=wait)

    def stop_nodes(self, wait=0):
        """Stop multiple bitcoind test nodes"""
        for node in self.nodes:
            # Issue RPC to stop nodes
            node.stop_node(wait=wait, wait_until_stopped=False)

        for node in self.nodes:
            # Wait for nodes to stop
            node.wait_until_stopped()

    def restart_node(self, i, extra_args=None, clear_addrman=False, *, expected_stderr=''):
        """Stop and start a test node"""
        self.stop_node(i, expected_stderr=expected_stderr)
        if clear_addrman:
            peers_dat = self.nodes[i].chain_path / "peers.dat"
            os.remove(peers_dat)
            with self.nodes[i].assert_debug_log(expected_msgs=[f'Creating peers.dat because the file was not found ("{peers_dat}")']):
                self.start_node(i, extra_args)
        else:
            self.start_node(i, extra_args)

    def wait_for_node_exit(self, i, timeout):
        self.nodes[i].process.wait(timeout)

    def connect_nodes(self, a, b, *, peer_advertises_v2=None, wait_for_connect: bool = True):
        """
        Kwargs:
            wait_for_connect: if True, block until the nodes are verified as connected. You might
                want to disable this when using -stopatheight with one of the connected nodes,
                since there will be a race between the actual connection and performing
                the assertions before one node shuts down.
        """
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

        if not wait_for_connect:
            return

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

    def no_op(self):
        pass

    def generate(self, generator, *args, sync_fun=None, **kwargs):
        blocks = generator.generate(*args, called_by_framework=True, **kwargs)
        sync_fun() if sync_fun else self.sync_all()
        return blocks

    def generateblock(self, generator, *args, sync_fun=None, **kwargs):
        blocks = generator.generateblock(*args, called_by_framework=True, **kwargs)
        sync_fun() if sync_fun else self.sync_all()
        return blocks

    def generatetoaddress(self, generator, *args, sync_fun=None, **kwargs):
        blocks = generator.generatetoaddress(*args, called_by_framework=True, **kwargs)
        sync_fun() if sync_fun else self.sync_all()
        return blocks

    def generatetodescriptor(self, generator, *args, sync_fun=None, **kwargs):
        blocks = generator.generatetodescriptor(*args, called_by_framework=True, **kwargs)
        sync_fun() if sync_fun else self.sync_all()
        return blocks

    def create_outpoints(self, node, *, outputs):
        """Send funds to a given list of `{address: amount}` targets using the bitcoind
        wallet and return the corresponding outpoints as a list of dictionaries
        `[{"txid": txid, "vout": vout1}, {"txid": txid, "vout": vout2}, ...]`.
        The result can be used to specify inputs for RPCs like `createrawtransaction`,
        `createpsbt`, `lockunspent` etc."""
        assert all(len(output.keys()) == 1 for output in outputs)
        send_res = node.send(outputs)
        assert send_res["complete"]
        utxos = []
        for output in outputs:
            address = list(output.keys())[0]
            vout = find_vout_for_address(node, send_res["txid"], address)
            utxos.append({"txid": send_res["txid"], "vout": vout})
        return utxos

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

    def wait_until(self, test_function, timeout=60, check_interval=0.05):
        return wait_until_helper_internal(test_function, timeout=timeout, timeout_factor=self.options.timeout_factor, check_interval=check_interval)

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
        # Format logs the same as bitcoind's debug.log with microprecision (so log files can be concatenated and sorted)
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
                    binaries=self.get_binaries(),
                    coverage_dir=None,
                    cwd=self.options.tmpdir,
                    uses_wallet=self.uses_wallet,
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
                if entry not in ['chainstate', 'blocks', 'indexes']:  # Only indexes, chainstate and blocks folders
                    os.remove(cache_path(entry))

        for i in range(self.num_nodes):
            self.log.debug("Copy cache directory {} to node {}".format(cache_node_dir, i))
            to_dir = get_datadir_path(self.options.tmpdir, i)
            shutil.copytree(cache_node_dir, to_dir)
            initialize_datadir(self.options.tmpdir, i, self.chain, self.disable_autoconnect)  # Overwrite port/rpcport in bitcoin.conf

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

    def skip_if_no_bitcoind_tracepoints(self):
        """Skip the running test if bitcoind has not been compiled with USDT tracepoint support."""
        if not self.is_usdt_compiled():
            raise SkipTest("bitcoind has not been built with USDT tracepoints enabled.")

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

    def skip_if_no_bitcoind_zmq(self):
        """Skip the running test if bitcoind has not been compiled with zmq support."""
        if not self.is_zmq_compiled():
            raise SkipTest("bitcoind has not been built with zmq enabled.")

    def skip_if_no_wallet(self):
        """Skip the running test if wallet has not been compiled."""
        self.uses_wallet = True
        if not self.is_wallet_compiled():
            raise SkipTest("wallet has not been compiled.")

    def skip_if_no_bdb(self):
        """Skip the running test if BDB has not been compiled."""
        if not self.is_bdb_compiled():
            raise SkipTest("BDB has not been compiled.")

    def skip_if_no_wallet_tool(self):
        """Skip the running test if bitcoin-wallet has not been compiled."""
        if not self.is_wallet_tool_compiled():
            raise SkipTest("bitcoin-wallet has not been compiled")

    def skip_if_no_bitcoin_util(self):
        """Skip the running test if bitcoin-util has not been compiled."""
        if not self.is_bitcoin_util_compiled():
            raise SkipTest("bitcoin-util has not been compiled")

    def skip_if_no_bitcoin_chainstate(self):
        """Skip the running test if bitcoin-chainstate has not been compiled."""
        if not self.is_bitcoin_chainstate_compiled():
            raise SkipTest("bitcoin-chainstate has not been compiled")

    def skip_if_no_cli(self):
        """Skip the running test if bitcoin-cli has not been compiled."""
        if not self.is_cli_compiled():
            raise SkipTest("bitcoin-cli has not been compiled.")

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
        """Checks whether bitcoin-cli was compiled."""
        return self.config["components"].getboolean("ENABLE_CLI")

    def is_external_signer_compiled(self):
        """Checks whether external signer support was compiled."""
        return self.config["components"].getboolean("ENABLE_EXTERNAL_SIGNER")

    def is_wallet_compiled(self):
        """Checks whether the wallet module was compiled."""
        return self.config["components"].getboolean("ENABLE_WALLET")

    def is_wallet_tool_compiled(self):
        """Checks whether bitcoin-wallet was compiled."""
        return self.config["components"].getboolean("ENABLE_WALLET_TOOL")

    def is_bitcoin_util_compiled(self):
        """Checks whether bitcoin-util was compiled."""
        return self.config["components"].getboolean("ENABLE_BITCOIN_UTIL")

    def is_bitcoin_chainstate_compiled(self):
        """Checks whether bitcoin-chainstate was compiled."""
        return self.config["components"].getboolean("ENABLE_BITCOIN_CHAINSTATE")

    def is_zmq_compiled(self):
        """Checks whether the zmq module was compiled."""
        return self.config["components"].getboolean("ENABLE_ZMQ")

    def is_usdt_compiled(self):
        """Checks whether the USDT tracepoints were compiled."""
        return self.config["components"].getboolean("ENABLE_USDT_TRACEPOINTS")

    def is_bdb_compiled(self):
        """Checks whether the wallet module was compiled with BDB support."""
        return self.config["components"].getboolean("USE_BDB")

    def has_blockfile(self, node, filenum: str):
        return (node.blocks_path/ f"blk{filenum}.dat").is_file()
