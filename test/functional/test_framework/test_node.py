#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Class for bitcoind node under test"""

import contextlib
import decimal
import errno
from enum import Enum
import json
import logging
import os
import pathlib
import platform
import re
import subprocess
import tempfile
import time
import urllib.parse
import collections
import shlex
import shutil
import sys
from pathlib import Path

from .authproxy import (
    JSONRPCException,
    serialization_fallback,
)
from .messages import NODE_P2P_V2
from .p2p import P2P_SERVICES, P2P_SUBVERSION
from .util import (
    MAX_NODES,
    assert_equal,
    assert_not_equal,
    append_config,
    delete_cookie_file,
    get_auth_cookie,
    get_rpc_proxy,
    rpc_url,
    wait_until_helper_internal,
    p2p_port,
    tor_port,
)

BITCOIND_PROC_WAIT_TIMEOUT = 60
# The size of the blocks xor key
# from InitBlocksdirXorKey::xor_key.size()
NUM_XOR_BYTES = 8
# Many systems have a 128kB limit for a command size. Depending on the
# platform, this limit may be larger or smaller. Moreover, when using the
# 'bitcoin' command, it may internally insert more args, which must be
# accounted for. There is no need to pick the largest possible value here
# anyway and it should be fine to set it to 1kB in tests.
TEST_CLI_MAX_ARG_SIZE = 1024

# The null blocks key (all 0s)
NULL_BLK_XOR_KEY = bytes([0] * NUM_XOR_BYTES)
BITCOIN_PID_FILENAME_DEFAULT = "bitcoind.pid"

if sys.platform.startswith("linux"):
    UNIX_PATH_MAX = 108          # includes the trailing NUL
elif sys.platform.startswith(("darwin", "freebsd", "netbsd", "openbsd")):
    UNIX_PATH_MAX = 104
else:                            # safest portable value
    UNIX_PATH_MAX = 92


class FailedToStartError(Exception):
    """Raised when a node fails to start correctly."""


class ErrorMatch(Enum):
    FULL_TEXT = 1
    FULL_REGEX = 2
    PARTIAL_REGEX = 3


class TestNode():
    """A class for representing a bitcoind node under test.

    This class contains:

    - state about the node (whether it's running, etc)
    - a Python subprocess.Popen object representing the running process
    - an RPC connection to the node
    - one or more P2P connections to the node


    To make things easier for the test writer, any unrecognised messages will
    be dispatched to the RPC connection."""

    def __init__(self, i, datadir_path, *, chain, rpchost, timewait, timeout_factor, binaries, coverage_dir, cwd, extra_conf=None, extra_args=None, use_cli=False, start_perf=False, use_valgrind=False, version=None, v2transport=False, uses_wallet=False, ipcbind=False):
        """
        Kwargs:
            start_perf (bool): If True, begin profiling the node with `perf` as soon as
                the node starts.
        """

        self.index = i
        self.p2p_conn_index = 1
        self.datadir_path = datadir_path
        self.bitcoinconf = self.datadir_path / "bitcoin.conf"
        self.stdout_dir = self.datadir_path / "stdout"
        self.stderr_dir = self.datadir_path / "stderr"
        self.chain = chain
        self.rpchost = rpchost
        self.rpc_timeout = timewait
        self.binaries = binaries
        self.coverage_dir = coverage_dir
        self.cwd = cwd
        self.has_explicit_bind = False
        if extra_conf is not None:
            append_config(self.datadir_path, extra_conf)
            # Remember if there is bind=... in the config file.
            self.has_explicit_bind = any(e.startswith("bind=") for e in extra_conf)
        # Most callers will just need to add extra args to the standard list below.
        # For those callers that need more flexibility, they can just set the args property directly.
        # Note that common args are set in the config file (see initialize_datadir)
        self.extra_args = extra_args
        self.version = version
        # Configuration for logging is set as command-line args rather than in the bitcoin.conf file.
        # This means that starting a bitcoind using the temp dir to debug a failed test won't
        # spam debug.log.
        self.args = self.binaries.node_argv(need_ipc=ipcbind) + [
            f"-datadir={self.datadir_path}",
            "-logtimemicros",
            "-debug",
            "-debugexclude=libevent",
            "-debugexclude=leveldb",
            "-debugexclude=rand",
            "-uacomment=testnode%d" % i,  # required for subversion uniqueness across peers
        ]
        if uses_wallet is not None and not uses_wallet:
            self.args.append("-disablewallet")

        self.ipc_tmp_dir = None
        if ipcbind:
            self.ipc_socket_path = self.chain_path / "node.sock"
            if len(os.fsencode(self.ipc_socket_path)) < UNIX_PATH_MAX:
                self.args.append("-ipcbind=unix")
            else:
                # Work around default CI path exceeding maximum socket path length.
                self.ipc_tmp_dir = pathlib.Path(tempfile.mkdtemp(prefix="test-ipc-"))
                self.ipc_socket_path = self.ipc_tmp_dir / "node.sock"
                self.args.append(f"-ipcbind=unix:{self.ipc_socket_path}")

        # Use valgrind, expect for previous release binaries
        if use_valgrind and version is None:
            default_suppressions_file = Path(__file__).parents[3] / "contrib" / "valgrind.supp"
            suppressions_file = os.getenv("VALGRIND_SUPPRESSIONS_FILE",
                                          default_suppressions_file)
            self.args = ["valgrind", "--suppressions={}".format(suppressions_file),
                         "--gen-suppressions=all", "--exit-on-first-error=yes",
                         "--error-exitcode=1", "--quiet"] + self.args

        if self.version_is_at_least(190000):
            self.args.append("-logthreadnames")
        if self.version_is_at_least(219900):
            self.args.append("-logsourcelocations")
        if self.version_is_at_least(239000):
            self.args.append("-loglevel=trace")
        if self.version_is_at_least(299900):
            self.args.append("-nologratelimit")

        # Default behavior from global -v2transport flag is added to args to persist it over restarts.
        # May be overwritten in individual tests, using extra_args.
        self.default_to_v2 = v2transport
        if self.version_is_at_least(260000):
            # 26.0 and later support v2transport
            if v2transport:
                self.args.append("-v2transport=1")
            else:
                self.args.append("-v2transport=0")
        # if v2transport is requested via global flag but not supported for node version, ignore it

        self.cli = TestNodeCLI(binaries, self.datadir_path)
        self.use_cli = use_cli
        self.start_perf = start_perf

        self.running = False
        self.process = None
        self.rpc_connected = False
        self._rpc = None # Should usually not be accessed directly in tests to allow for --usecli mode
        self.reuse_http_connections = True # Must be set before calling get_rpc_proxy() i.e. before restarting node
        self.url = None
        self.log = logging.getLogger('TestFramework.node%d' % i)
        # Cache perf subprocesses here by their data output filename.
        self.perf_subprocesses = {}

        self.p2ps = []
        self.timeout_factor = timeout_factor

        self.mocktime = None

    AddressKeyPair = collections.namedtuple('AddressKeyPair', ['address', 'key'])
    PRIV_KEYS = [
            # address , privkey
            AddressKeyPair('mjTkW3DjgyZck4KbiRusZsqTgaYTxdSz6z', 'cVpF924EspNh8KjYsfhgY96mmxvT6DgdWiTYMtMjuM74hJaU5psW'),
            AddressKeyPair('msX6jQXvxiNhx3Q62PKeLPrhrqZQdSimTg', 'cUxsWyKyZ9MAQTaAhUQWJmBbSvHMwSmuv59KgxQV7oZQU3PXN3KE'),
            AddressKeyPair('mnonCMyH9TmAsSj3M59DsbH8H63U3RKoFP', 'cTrh7dkEAeJd6b3MRX9bZK8eRmNqVCMH3LSUkE3dSFDyzjU38QxK'),
            AddressKeyPair('mqJupas8Dt2uestQDvV2NH3RU8uZh2dqQR', 'cVuKKa7gbehEQvVq717hYcbE9Dqmq7KEBKqWgWrYBa2CKKrhtRim'),
            AddressKeyPair('msYac7Rvd5ywm6pEmkjyxhbCDKqWsVeYws', 'cQDCBuKcjanpXDpCqacNSjYfxeQj8G6CAtH1Dsk3cXyqLNC4RPuh'),
            AddressKeyPair('n2rnuUnwLgXqf9kk2kjvVm8R5BZK1yxQBi', 'cQakmfPSLSqKHyMFGwAqKHgWUiofJCagVGhiB4KCainaeCSxeyYq'),
            AddressKeyPair('myzuPxRwsf3vvGzEuzPfK9Nf2RfwauwYe6', 'cQMpDLJwA8DBe9NcQbdoSb1BhmFxVjWD5gRyrLZCtpuF9Zi3a9RK'),
            AddressKeyPair('mumwTaMtbxEPUswmLBBN3vM9oGRtGBrys8', 'cSXmRKXVcoouhNNVpcNKFfxsTsToY5pvB9DVsFksF1ENunTzRKsy'),
            AddressKeyPair('mpV7aGShMkJCZgbW7F6iZgrvuPHjZjH9qg', 'cSoXt6tm3pqy43UMabY6eUTmR3eSUYFtB2iNQDGgb3VUnRsQys2k'),
            AddressKeyPair('mq4fBNdckGtvY2mijd9am7DRsbRB4KjUkf', 'cN55daf1HotwBAgAKWVgDcoppmUNDtQSfb7XLutTLeAgVc3u8hik'),
            AddressKeyPair('mpFAHDjX7KregM3rVotdXzQmkbwtbQEnZ6', 'cT7qK7g1wkYEMvKowd2ZrX1E5f6JQ7TM246UfqbCiyF7kZhorpX3'),
            AddressKeyPair('mzRe8QZMfGi58KyWCse2exxEFry2sfF2Y7', 'cPiRWE8KMjTRxH1MWkPerhfoHFn5iHPWVK5aPqjW8NxmdwenFinJ'),
    ]

    def get_deterministic_priv_key(self):
        """Return a deterministic priv key in base58, that only depends on the node's index"""
        assert len(self.PRIV_KEYS) == MAX_NODES
        return self.PRIV_KEYS[self.index]

    def _node_msg(self, msg: str) -> str:
        """Return a modified msg that identifies this node by its index as a debugging aid."""
        return "[node %d] %s" % (self.index, msg)

    def _raise_assertion_error(self, msg: str):
        """Raise an AssertionError with msg modified to identify this node."""
        raise AssertionError(self._node_msg(msg))

    def __del__(self):
        # Ensure that we don't leave any bitcoind processes lying around after
        # the test ends
        if self.process:
            # Should only happen on test failure
            # Avoid using logger, as that may have already been shutdown when
            # this destructor is called.
            print(self._node_msg("Cleaning up leftover process"), file=sys.stderr)
            self.process.kill()
        if self.ipc_tmp_dir:
            print(self._node_msg(f"Cleaning up ipc directory {str(self.ipc_tmp_dir)!r}"))
            shutil.rmtree(self.ipc_tmp_dir)

    def __getattr__(self, name):
        """Dispatches any unrecognised messages to the RPC connection or a CLI instance."""
        if self.use_cli:
            return getattr(self.cli, name)
        else:
            assert self.rpc_connected and self._rpc is not None, self._node_msg("Error: no RPC connection")
            return getattr(self._rpc, name)

    def start(self, extra_args=None, *, cwd=None, stdout=None, stderr=None, env=None, **kwargs):
        """Start the node."""
        if extra_args is None:
            extra_args = self.extra_args

        # If listening and no -bind is given, then bitcoind would bind P2P ports on
        # 0.0.0.0:P and 127.0.0.1:P+1 (for incoming Tor connections), where P is
        # a unique port chosen by the test framework and configured as port=P in
        # bitcoin.conf. To avoid collisions, change it to 127.0.0.1:tor_port().
        will_listen = all(e != "-nolisten" and e != "-listen=0" for e in extra_args)
        has_explicit_bind = self.has_explicit_bind or any(e.startswith("-bind=") for e in extra_args)
        if will_listen and not has_explicit_bind:
            extra_args.append(f"-bind=0.0.0.0:{p2p_port(self.index)}")
            extra_args.append(f"-bind=127.0.0.1:{tor_port(self.index)}=onion")

        self.use_v2transport = "-v2transport=1" in extra_args or (self.default_to_v2 and "-v2transport=0" not in extra_args)

        # Add a new stdout and stderr file each time bitcoind is started
        if stderr is None:
            stderr = tempfile.NamedTemporaryFile(dir=self.stderr_dir, delete=False)
        if stdout is None:
            stdout = tempfile.NamedTemporaryFile(dir=self.stdout_dir, delete=False)
        self.stderr = stderr
        self.stdout = stdout

        if cwd is None:
            cwd = self.cwd

        # Delete any existing cookie file -- if such a file exists (eg due to
        # unclean shutdown), it will get overwritten anyway by bitcoind, and
        # potentially interfere with our attempt to authenticate
        delete_cookie_file(self.datadir_path, self.chain)

        # add environment variable LIBC_FATAL_STDERR_=1 so that libc errors are written to stderr and not the terminal
        subp_env = dict(os.environ, LIBC_FATAL_STDERR_="1")
        if env is not None:
            subp_env.update(env)

        self.process = subprocess.Popen(self.args + extra_args, env=subp_env, stdout=stdout, stderr=stderr, cwd=cwd, **kwargs)

        self.running = True
        self.log.debug("bitcoind started, waiting for RPC to come up")

        if self.start_perf:
            self._start_perf()

    def wait_for_rpc_connection(self, *, wait_for_import=True):
        """Sets up an RPC connection to the bitcoind process. Returns False if unable to connect."""
        # Poll at a rate of four times per second
        poll_per_s = 4

        suppressed_errors = collections.defaultdict(int)
        latest_error = None
        def suppress_error(category: str, e: Exception):
            suppressed_errors[category] += 1
            return (category, repr(e))

        for _ in range(poll_per_s * self.rpc_timeout):
            if self.process.poll() is not None:
                # Attach abrupt shutdown error/s to the exception message
                self.stderr.seek(0)
                str_error = ''.join(line.decode('utf-8') for line in self.stderr)
                str_error += "************************\n" if str_error else ''

                raise FailedToStartError(self._node_msg(
                    f'bitcoind exited with status {self.process.returncode} during initialization. {str_error}'))
            try:
                rpc = get_rpc_proxy(
                    rpc_url(self.datadir_path, self.index, self.chain, self.rpchost),
                    self.index,
                    timeout=self.rpc_timeout // 2,  # Shorter timeout to allow for one retry in case of ETIMEDOUT
                    coveragedir=self.coverage_dir,
                )
                rpc.auth_service_proxy_instance.reuse_http_connections = self.reuse_http_connections
                rpc.getblockcount()
                # If the call to getblockcount() succeeds then the RPC connection is up
                if self.version_is_at_least(190000) and wait_for_import:
                    # getmempoolinfo.loaded is available since commit
                    # bb8ae2c (version 0.19.0)
                    self.wait_until(lambda: rpc.getmempoolinfo()['loaded'])
                    # Wait for the node to finish reindex, block import, and
                    # loading the mempool. Usually importing happens fast or
                    # even "immediate" when the node is started. However, there
                    # is no guarantee and sometimes ImportBlocks might finish
                    # later. This is going to cause intermittent test failures,
                    # because generally the tests assume the node is fully
                    # ready after being started.
                    #
                    # For example, the node will reject block messages from p2p
                    # when it is still importing with the error "Unexpected
                    # block message received"
                    #
                    # The wait is done here to make tests as robust as possible
                    # and prevent racy tests and intermittent failures as much
                    # as possible. Some tests might not need this, but the
                    # overhead is trivial, and the added guarantees are worth
                    # the minimal performance cost.
                self.log.debug("RPC successfully started")
                # Set rpc_connected even if we are in use_cli mode so that we know we can call self.stop() if needed.
                self.rpc_connected = True
                if self.use_cli:
                    return
                self._rpc = rpc
                self.url = self._rpc.rpc_url
                return
            except JSONRPCException as e:
                # Suppress these as they are expected during initialization.
                # -28 RPC in warmup
                # -342 Service unavailable, could be starting up or shutting down
                if e.error['code'] not in [-28, -342]:
                    raise  # unknown JSON RPC exception
                latest_error = suppress_error(f"JSONRPCException {e.error['code']}", e)
            except OSError as e:
                error_num = e.errno
                # Work around issue where socket timeouts don't have errno set.
                # https://github.com/python/cpython/issues/109601
                if error_num is None and isinstance(e, TimeoutError):
                    error_num = errno.ETIMEDOUT

                # Suppress similarly to the above JSONRPCException errors.
                if error_num not in [
                    errno.ECONNRESET,   # This might happen when the RPC server is in warmup,
                                        # but shut down before the call to getblockcount succeeds.
                    errno.ETIMEDOUT,    # Treat identical to ECONNRESET
                    errno.ECONNREFUSED  # Port not yet open?
                ]:
                    raise  # unknown OS error
                latest_error = suppress_error(f"OSError {errno.errorcode[error_num]}", e)
            except ValueError as e:
                # Suppress if cookie file isn't generated yet and no rpcuser or rpcpassword; bitcoind may be starting.
                if "No RPC credentials" not in str(e):
                    raise
                latest_error = suppress_error("missing_credentials", e)
            time.sleep(1.0 / poll_per_s)
        self._raise_assertion_error(f"Unable to connect to bitcoind after {self.rpc_timeout}s (ignored errors: {dict(suppressed_errors)!s}{'' if latest_error is None else f', latest: {latest_error[0]!r}/{latest_error[1]}'})")

    def wait_for_cookie_credentials(self):
        """Ensures auth cookie credentials can be read, e.g. for testing CLI with -rpcwait before RPC connection is up."""
        self.log.debug("Waiting for cookie credentials")
        # Poll at a rate of four times per second.
        poll_per_s = 4
        for _ in range(poll_per_s * self.rpc_timeout):
            try:
                get_auth_cookie(self.datadir_path, self.chain)
                self.log.debug("Cookie credentials successfully retrieved")
                return
            except ValueError:  # cookie file not found and no rpcuser or rpcpassword; bitcoind is still starting
                pass            # so we continue polling until RPC credentials are retrieved
            time.sleep(1.0 / poll_per_s)
        self._raise_assertion_error("Unable to retrieve cookie credentials after {}s".format(self.rpc_timeout))

    def generate(self, nblocks, maxtries=1000000, **kwargs):
        self.log.debug("TestNode.generate() dispatches `generate` call to `generatetoaddress`")
        return self.generatetoaddress(nblocks=nblocks, address=self.get_deterministic_priv_key().address, maxtries=maxtries, **kwargs)

    def generateblock(self, *args, called_by_framework, **kwargs):
        assert called_by_framework, "Direct call of this mining RPC is discouraged. Please use one of the self.generate* methods on the test framework, which sync the nodes to avoid intermittent test issues. You may use sync_fun=self.no_op to disable the sync explicitly."
        return self.__getattr__('generateblock')(*args, **kwargs)

    def generatetoaddress(self, *args, called_by_framework, **kwargs):
        assert called_by_framework, "Direct call of this mining RPC is discouraged. Please use one of the self.generate* methods on the test framework, which sync the nodes to avoid intermittent test issues. You may use sync_fun=self.no_op to disable the sync explicitly."
        return self.__getattr__('generatetoaddress')(*args, **kwargs)

    def generatetodescriptor(self, *args, called_by_framework, **kwargs):
        assert called_by_framework, "Direct call of this mining RPC is discouraged. Please use one of the self.generate* methods on the test framework, which sync the nodes to avoid intermittent test issues. You may use sync_fun=self.no_op to disable the sync explicitly."
        return self.__getattr__('generatetodescriptor')(*args, **kwargs)

    def setmocktime(self, timestamp):
        """Wrapper for setmocktime RPC, sets self.mocktime"""
        if timestamp == 0:
            # setmocktime(0) resets to system time.
            self.mocktime = None
        else:
            self.mocktime = timestamp
        return self.__getattr__('setmocktime')(timestamp)

    def get_wallet_rpc(self, wallet_name):
        if self.use_cli:
            return self.cli("-rpcwallet={}".format(wallet_name))
        else:
            assert self.rpc_connected and self._rpc, self._node_msg("RPC not connected")
            wallet_path = "wallet/{}".format(urllib.parse.quote(wallet_name))
            return self._rpc / wallet_path

    def version_is_at_least(self, ver):
        return self.version is None or self.version >= ver

    def stop_node(self, expected_stderr='', *, wait=0, wait_until_stopped=True):
        """Stop the node."""
        if not self.running:
            return
        assert self.rpc_connected, self._node_msg(
            "Should only call stop_node() on a running node after wait_for_rpc_connection() succeeded. "
            f"Did you forget to call the latter after start()? Not connected to process: {self.process.pid}")
        self.log.debug("Stopping node")
        # Do not use wait argument when testing older nodes, e.g. in wallet_backwards_compatibility.py
        if self.version_is_at_least(180000):
            self.stop(wait=wait)
        else:
            self.stop()

        # If there are any running perf processes, stop them.
        for profile_name in tuple(self.perf_subprocesses.keys()):
            self._stop_perf(profile_name)

        del self.p2ps[:]

        assert (not expected_stderr) or wait_until_stopped  # Must wait to check stderr
        if wait_until_stopped:
            self.wait_until_stopped(expected_stderr=expected_stderr)

    def is_node_stopped(self, *, expected_stderr="", expected_ret_code=0):
        """Checks whether the node has stopped.

        Returns True if the node has stopped. False otherwise.
        This method is responsible for freeing resources (self.process)."""
        if not self.running:
            return True
        return_code = self.process.poll()
        if return_code is None:
            return False

        # process has stopped. Assert that it didn't return an error code.
        assert return_code == expected_ret_code, self._node_msg(
            f"Node returned unexpected exit code ({return_code}) vs ({expected_ret_code}) when stopping")
        # Check that stderr is as expected
        self.stderr.seek(0)
        stderr = self.stderr.read().decode('utf-8').strip()
        if stderr != expected_stderr:
            raise AssertionError("Unexpected stderr {} != {}".format(stderr, expected_stderr))

        self.stdout.close()
        self.stderr.close()

        self.running = False
        self.process = None
        self.rpc_connected = False
        self._rpc = None
        self.log.debug("Node stopped")
        return True

    def wait_until_stopped(self, *, timeout=BITCOIND_PROC_WAIT_TIMEOUT, expect_error=False, **kwargs):
        if "expected_ret_code" not in kwargs:
            kwargs["expected_ret_code"] = 1 if expect_error else 0  # Whether node shutdown return EXIT_FAILURE or EXIT_SUCCESS
        self.wait_until(lambda: self.is_node_stopped(**kwargs), timeout=timeout)

    def kill_process(self):
        self.process.kill()
        self.wait_until_stopped(expected_ret_code=1 if platform.system() == "Windows" else -9)
        assert self.is_node_stopped()

    def replace_in_config(self, replacements):
        """
        Perform replacements in the configuration file.
        The substitutions are passed as a list of search-replace-tuples, e.g.
            [("old", "new"), ("foo", "bar"), ...]
        """
        with open(self.bitcoinconf, 'r', encoding='utf8') as conf:
            conf_data = conf.read()
        for replacement in replacements:
            assert_equal(len(replacement), 2)
            old, new = replacement[0], replacement[1]
            conf_data = conf_data.replace(old, new)
        with open(self.bitcoinconf, 'w', encoding='utf8') as conf:
            conf.write(conf_data)

    @property
    def chain_path(self) -> Path:
        return self.datadir_path / self.chain

    @property
    def debug_log_path(self) -> Path:
        return self.chain_path / 'debug.log'

    @property
    def blocks_path(self) -> Path:
        return self.chain_path / "blocks"

    @property
    def blocks_key_path(self) -> Path:
        return self.blocks_path / "xor.dat"

    def read_xor_key(self) -> bytes:
        with open(self.blocks_key_path, "rb") as xor_f:
            return xor_f.read(NUM_XOR_BYTES)

    @property
    def wallets_path(self) -> Path:
        return self.chain_path / "wallets"

    def debug_log_size(self, **kwargs) -> int:
        with open(self.debug_log_path, **kwargs) as dl:
            dl.seek(0, 2)
            return dl.tell()

    @contextlib.contextmanager
    def assert_debug_log(self, expected_msgs, unexpected_msgs=None, timeout=2):
        if unexpected_msgs is None:
            unexpected_msgs = []
        assert_equal(type(expected_msgs), list)
        assert_equal(type(unexpected_msgs), list)

        time_end = time.time() + timeout * self.timeout_factor
        prev_size = self.debug_log_size(encoding="utf-8")  # Must use same encoding that is used to read() below

        yield

        while True:
            found = True
            with open(self.debug_log_path, encoding="utf-8", errors="replace") as dl:
                dl.seek(prev_size)
                log = dl.read()
            print_log = " - " + "\n - ".join(log.splitlines())
            for unexpected_msg in unexpected_msgs:
                if re.search(re.escape(unexpected_msg), log, flags=re.MULTILINE):
                    self._raise_assertion_error('Unexpected message "{}" partially matches log:\n\n{}\n\n'.format(unexpected_msg, print_log))
            for expected_msg in expected_msgs:
                if re.search(re.escape(expected_msg), log, flags=re.MULTILINE) is None:
                    found = False
            if found:
                return
            if time.time() >= time_end:
                break
            time.sleep(0.05)
        self._raise_assertion_error('Expected messages "{}" does not partially match log:\n\n{}\n\n'.format(str(expected_msgs), print_log))

    @contextlib.contextmanager
    def busy_wait_for_debug_log(self, expected_msgs, timeout=60):
        """
        Block until we see a particular debug log message fragment or until we exceed the timeout.
        Return:
            the number of log lines we encountered when matching
        """
        time_end = time.time() + timeout * self.timeout_factor
        prev_size = self.debug_log_size(mode="rb")  # Must use same mode that is used to read() below

        yield

        while True:
            found = True
            with open(self.debug_log_path, "rb") as dl:
                dl.seek(prev_size)
                log = dl.read()

            for expected_msg in expected_msgs:
                if expected_msg not in log:
                    found = False

            if found:
                return

            if time.time() >= time_end:
                print_log = " - " + "\n - ".join(log.decode("utf8", errors="replace").splitlines())
                break

            # No sleep here because we want to detect the message fragment as fast as
            # possible.

        self._raise_assertion_error(
            'Expected messages "{}" does not partially match log:\n\n{}\n\n'.format(
                str(expected_msgs), print_log))

    @contextlib.contextmanager
    def wait_for_new_peer(self, timeout=5):
        """
        Wait until the node is connected to at least one new peer. We detect this
        by watching for an increased highest peer id, using the `getpeerinfo` RPC call.
        Note that the simpler approach of only accounting for the number of peers
        suffers from race conditions, as disconnects from unrelated previous peers
        could happen anytime in-between.
        """
        def get_highest_peer_id():
            peer_info = self.getpeerinfo()
            return peer_info[-1]["id"] if peer_info else -1

        initial_peer_id = get_highest_peer_id()
        yield
        self.wait_until(lambda: get_highest_peer_id() > initial_peer_id, timeout=timeout)

    @contextlib.contextmanager
    def profile_with_perf(self, profile_name: str):
        """
        Context manager that allows easy profiling of node activity using `perf`.

        See `test/functional/README.md` for details on perf usage.

        Args:
            profile_name: This string will be appended to the
                profile data filename generated by perf.
        """
        subp = self._start_perf(profile_name)

        yield

        if subp:
            self._stop_perf(profile_name)

    def _start_perf(self, profile_name=None):
        """Start a perf process to profile this node.

        Returns the subprocess running perf."""
        subp = None

        def test_success(cmd):
            return subprocess.call(
                # shell=True required for pipe use below
                cmd, shell=True,
                stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL) == 0

        if platform.system() != 'Linux':
            self.log.warning("Can't profile with perf; only available on Linux platforms")
            return None

        if not test_success('which perf'):
            self.log.warning("Can't profile with perf; must install perf-tools")
            return None

        if not test_success('readelf -S {} | grep .debug_str'.format(shlex.quote(self.binary))):
            self.log.warning(
                "perf output won't be very useful without debug symbols compiled into bitcoind")

        output_path = tempfile.NamedTemporaryFile(
            dir=self.datadir_path,
            prefix="{}.perf.data.".format(profile_name or 'test'),
            delete=False,
        ).name

        cmd = [
            'perf', 'record',
            '-g',                     # Record the callgraph.
            '--call-graph', 'dwarf',  # Compatibility for gcc's --fomit-frame-pointer.
            '-F', '101',              # Sampling frequency in Hz.
            '-p', str(self.process.pid),
            '-o', output_path,
        ]
        subp = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.perf_subprocesses[profile_name] = subp

        return subp

    def _stop_perf(self, profile_name):
        """Stop (and pop) a perf subprocess."""
        subp = self.perf_subprocesses.pop(profile_name)
        output_path = subp.args[subp.args.index('-o') + 1]

        subp.terminate()
        subp.wait(timeout=10)

        stderr = subp.stderr.read().decode()
        if 'Consider tweaking /proc/sys/kernel/perf_event_paranoid' in stderr:
            self.log.warning(
                "perf couldn't collect data! Try "
                "'sudo sysctl -w kernel.perf_event_paranoid=-1'")
        else:
            report_cmd = "perf report -i {}".format(output_path)
            self.log.info("See perf output by running '{}'".format(report_cmd))

    def assert_start_raises_init_error(self, extra_args=None, expected_msg=None, match=ErrorMatch.FULL_TEXT, *args, **kwargs):
        """Attempt to start the node and expect it to raise an error.

        extra_args: extra arguments to pass through to bitcoind
        expected_msg: regex that stderr should match when bitcoind fails

        Will throw if bitcoind starts without an error.
        Will throw if an expected_msg is provided and it does not match bitcoind's stdout."""
        assert not self.running
        with tempfile.NamedTemporaryFile(dir=self.stderr_dir, delete=False) as log_stderr, \
             tempfile.NamedTemporaryFile(dir=self.stdout_dir, delete=False) as log_stdout:
            try:
                self.start(extra_args, stdout=log_stdout, stderr=log_stderr, *args, **kwargs)
                ret = self.process.wait(timeout=self.rpc_timeout)
                self.log.debug(self._node_msg(f'bitcoind exited with status {ret} during initialization'))
                assert_not_equal(ret, 0) # Exit code must indicate failure
                self.running = False
                self.process = None
                # Check stderr for expected message
                if expected_msg is not None:
                    log_stderr.seek(0)
                    stderr = log_stderr.read().decode('utf-8').strip()
                    if match == ErrorMatch.PARTIAL_REGEX:
                        if re.search(expected_msg, stderr, flags=re.MULTILINE) is None:
                            self._raise_assertion_error(
                                'Expected message "{}" does not partially match stderr:\n"{}"'.format(expected_msg, stderr))
                    elif match == ErrorMatch.FULL_REGEX:
                        if re.fullmatch(expected_msg, stderr) is None:
                            self._raise_assertion_error(
                                'Expected message "{}" does not fully match stderr:\n"{}"'.format(expected_msg, stderr))
                    elif match == ErrorMatch.FULL_TEXT:
                        if expected_msg != stderr:
                            self._raise_assertion_error(
                                'Expected message "{}" does not fully match stderr:\n"{}"'.format(expected_msg, stderr))
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.running = False
                self.process = None
                assert_msg = f'bitcoind should have exited within {self.rpc_timeout}s '
                if expected_msg is None:
                    assert_msg += "with an error"
                else:
                    assert_msg += "with expected error " + expected_msg
                self._raise_assertion_error(assert_msg)

    def add_p2p_connection(self, p2p_conn, *, wait_for_verack=True, send_version=True, supports_v2_p2p=None, wait_for_v2_handshake=True, expect_success=True, **kwargs):
        """Add an inbound p2p connection to the node.

        This method adds the p2p connection to the self.p2ps list and also
        returns the connection to the caller.

        When self.use_v2transport is True, TestNode advertises NODE_P2P_V2 service flag

        An inbound connection is made from TestNode <------ P2PConnection
        - if TestNode doesn't advertise NODE_P2P_V2 service, P2PConnection sends version message and v1 P2P is followed
        - if TestNode advertises NODE_P2P_V2 service, (and if P2PConnections supports v2 P2P)
                P2PConnection sends ellswift bytes and v2 P2P is followed
        """
        if 'dstport' not in kwargs:
            kwargs['dstport'] = p2p_port(self.index)
        if 'dstaddr' not in kwargs:
            kwargs['dstaddr'] = '127.0.0.1'
        if supports_v2_p2p is None:
            supports_v2_p2p = self.use_v2transport

        if self.use_v2transport:
            kwargs['services'] = kwargs.get('services', P2P_SERVICES) | NODE_P2P_V2
        supports_v2_p2p = self.use_v2transport and supports_v2_p2p
        p2p_conn.peer_connect(**kwargs, send_version=send_version, net=self.chain, timeout_factor=self.timeout_factor, supports_v2_p2p=supports_v2_p2p)()

        self.p2ps.append(p2p_conn)
        if not expect_success:
            return p2p_conn
        p2p_conn.wait_until(lambda: p2p_conn.is_connected, check_connected=False)
        if supports_v2_p2p and wait_for_v2_handshake:
            p2p_conn.wait_until(lambda: p2p_conn.v2_state.tried_v2_handshake)
        if send_version:
            p2p_conn.wait_until(lambda: not p2p_conn.on_connection_send_msg)
        if wait_for_verack:
            # Wait for the node to send us the version and verack
            p2p_conn.wait_for_verack()
            # At this point we have sent our version message and received the version and verack, however the full node
            # has not yet received the verack from us (in reply to their version). So, the connection is not yet fully
            # established (fSuccessfullyConnected).
            #
            # This shouldn't lead to any issues when sending messages, since the verack will be in-flight before the
            # message we send. However, it might lead to races where we are expecting to receive a message. E.g. a
            # transaction that will be added to the mempool as soon as we return here.
            #
            # So syncing here is redundant when we only want to send a message, but the cost is low (a few milliseconds)
            # in comparison to the upside of making tests less fragile and unexpected intermittent errors less likely.
            p2p_conn.sync_with_ping()

            # Consistency check that the node received our user agent string.
            # Find our connection in getpeerinfo by our address:port and theirs, as this combination is unique.
            sockname = p2p_conn._transport.get_extra_info("socket").getsockname()
            our_addr_and_port = f"{sockname[0]}:{sockname[1]}"
            dst_addr_and_port = f"{p2p_conn.dstaddr}:{p2p_conn.dstport}"
            info = [peer for peer in self.getpeerinfo() if peer["addr"] == our_addr_and_port and peer["addrbind"] == dst_addr_and_port]
            assert_equal(len(info), 1)
            assert_equal(info[0]["subver"], P2P_SUBVERSION)

        return p2p_conn

    def add_outbound_p2p_connection(self, p2p_conn, *, wait_for_verack=True, wait_for_disconnect=False, p2p_idx, connection_type="outbound-full-relay", supports_v2_p2p=None, advertise_v2_p2p=None, **kwargs):
        """Add an outbound p2p connection from node. Must be an
        "outbound-full-relay", "block-relay-only", "addr-fetch" or "feeler" connection.

        This method adds the p2p connection to the self.p2ps list and returns
        the connection to the caller.

        p2p_idx must be different for simultaneously connected peers. When reusing it for the next peer
        after disconnecting the previous one, it is necessary to wait for the disconnect to finish to avoid
        a race condition.

        Parameters:
            supports_v2_p2p: whether p2p_conn supports v2 P2P or not
            advertise_v2_p2p: whether p2p_conn is advertised to support v2 P2P or not

        An outbound connection is made from TestNode -------> P2PConnection
            - if P2PConnection doesn't advertise_v2_p2p, TestNode sends version message and v1 P2P is followed
            - if P2PConnection both supports_v2_p2p and advertise_v2_p2p, TestNode sends ellswift bytes and v2 P2P is followed
            - if P2PConnection doesn't supports_v2_p2p but advertise_v2_p2p,
                TestNode sends ellswift bytes and P2PConnection disconnects,
                TestNode reconnects by sending version message and v1 P2P is followed
        """

        def addconnection_callback(address, port):
            self.log.debug("Connecting to %s:%d %s" % (address, port, connection_type))
            self.addconnection('%s:%d' % (address, port), connection_type, advertise_v2_p2p)

        if supports_v2_p2p is None:
            supports_v2_p2p = self.use_v2transport
        if advertise_v2_p2p is None:
            advertise_v2_p2p = self.use_v2transport

        if advertise_v2_p2p:
            kwargs['services'] = kwargs.get('services', P2P_SERVICES) | NODE_P2P_V2
            assert self.use_v2transport  # only a v2 TestNode could make a v2 outbound connection

        # if P2PConnection is advertised to support v2 P2P when it doesn't actually support v2 P2P,
        # reconnection needs to be attempted using v1 P2P by sending version message
        reconnect = advertise_v2_p2p and not supports_v2_p2p
        # P2PConnection needs to be advertised to support v2 P2P so that ellswift bytes are sent instead of msg_version
        supports_v2_p2p = supports_v2_p2p and advertise_v2_p2p
        p2p_conn.peer_accept_connection(connect_cb=addconnection_callback, connect_id=p2p_idx + 1, net=self.chain, timeout_factor=self.timeout_factor, supports_v2_p2p=supports_v2_p2p, reconnect=reconnect, **kwargs)()

        if reconnect:
            p2p_conn.wait_for_reconnect()

        if connection_type == "feeler" or wait_for_disconnect:
            # feeler connections are closed as soon as the node receives a `version` message
            p2p_conn.wait_until(lambda: p2p_conn.message_count["version"] == 1, check_connected=False)
            p2p_conn.wait_until(lambda: not p2p_conn.is_connected, check_connected=False)
        else:
            p2p_conn.wait_for_connect()
            self.p2ps.append(p2p_conn)

            if supports_v2_p2p:
                p2p_conn.wait_until(lambda: p2p_conn.v2_state.tried_v2_handshake)
            p2p_conn.wait_until(lambda: not p2p_conn.on_connection_send_msg)
            if wait_for_verack:
                p2p_conn.wait_for_verack()
                p2p_conn.sync_with_ping()

        return p2p_conn

    def num_test_p2p_connections(self):
        """Return number of test framework p2p connections to the node."""
        return len([peer for peer in self.getpeerinfo() if peer['subver'] == P2P_SUBVERSION])

    def disconnect_p2ps(self):
        """Close all p2p connections to the node.
        The state of the peers (such as txrequests) may not be fully cleared
        yet, even after this method returns."""
        for p in self.p2ps:
            p.peer_disconnect()
        del self.p2ps[:]

        self.wait_until(lambda: self.num_test_p2p_connections() == 0)

    def bumpmocktime(self, seconds):
        """Fast forward using setmocktime to self.mocktime + seconds. Requires setmocktime to have
        been called at some point in the past."""
        assert self.mocktime
        self.mocktime += seconds
        self.setmocktime(self.mocktime)

    def wait_until(self, test_function, timeout=60, check_interval=0.05):
        return wait_until_helper_internal(test_function, timeout=timeout, timeout_factor=self.timeout_factor, check_interval=check_interval)


class TestNodeCLIAttr:
    def __init__(self, cli, command):
        self.cli = cli
        self.command = command

    def __call__(self, *args, **kwargs):
        return self.cli.send_cli(self.command, *args, **kwargs)

    def get_request(self, *args, **kwargs):
        return lambda: self(*args, **kwargs)


def arg_to_cli(arg):
    if isinstance(arg, bool):
        return str(arg).lower()
    elif arg is None:
        return 'null'
    elif isinstance(arg, dict) or isinstance(arg, list) or isinstance(arg, tuple):
        return json.dumps(arg, default=serialization_fallback)
    else:
        return str(arg)


class TestNodeCLI():
    """Interface to bitcoin-cli for an individual node"""
    def __init__(self, binaries, datadir):
        self.options = []
        self.binaries = binaries
        self.datadir = datadir
        self.input = None
        self.log = logging.getLogger('TestFramework.bitcoincli')

    def __call__(self, *options, input=None):
        # TestNodeCLI is callable with bitcoin-cli command-line options
        cli = TestNodeCLI(self.binaries, self.datadir)
        cli.options = [str(o) for o in options]
        cli.input = input
        return cli

    def __getattr__(self, command):
        return TestNodeCLIAttr(self, command)

    def batch(self, requests):
        results = []
        for request in requests:
            try:
                results.append(dict(result=request()))
            except JSONRPCException as e:
                results.append(dict(error=e))
        return results

    def send_cli(self, clicommand=None, *args, **kwargs):
        """Run bitcoin-cli command. Deserializes returned string as python object."""
        pos_args = [arg_to_cli(arg) for arg in args]
        named_args = [key + "=" + arg_to_cli(value) for (key, value) in kwargs.items() if value is not None]
        p_args = self.binaries.rpc_argv() + [f"-datadir={self.datadir}"] + self.options
        if named_args:
            p_args += ["-named"]
        base_arg_pos = len(p_args)
        if clicommand is not None:
            p_args += [clicommand]
        p_args += pos_args + named_args

        # TEST_CLI_MAX_ARG_SIZE is set low enough that checking the string
        # length is enough and encoding to bytes is not needed before
        # calculating the sum.
        sum_arg_size = sum(len(arg) for arg in p_args)
        stdin_data = self.input
        if sum_arg_size >= TEST_CLI_MAX_ARG_SIZE:
            self.log.debug(f"Cli: Command size {sum_arg_size} too large, using stdin")
            rpc_args = "\n".join([arg for arg in p_args[base_arg_pos:]])
            if stdin_data is not None:
                stdin_data += "\n" + rpc_args
            else:
                stdin_data = rpc_args
            p_args = p_args[:base_arg_pos] + ['-stdin']

        self.log.debug("Running bitcoin-cli {}".format(p_args[2:]))
        process = subprocess.Popen(p_args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        cli_stdout, cli_stderr = process.communicate(input=stdin_data)
        returncode = process.poll()
        if returncode:
            match = re.match(r'error code: ([-0-9]+)\nerror message:\n(.*)', cli_stderr)
            if match:
                code, message = match.groups()
                raise JSONRPCException(dict(code=int(code), message=message))
            # Ignore cli_stdout, raise with cli_stderr
            raise subprocess.CalledProcessError(returncode, p_args, output=cli_stderr)
        try:
            if not cli_stdout.strip():
                return None
            return json.loads(cli_stdout, parse_float=decimal.Decimal)
        except (json.JSONDecodeError, decimal.InvalidOperation):
            return cli_stdout.rstrip("\n")
