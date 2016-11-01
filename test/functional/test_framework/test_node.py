#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Class for bitcoind node under test"""

import decimal
import errno
import http.client
import json
import logging
import os
import re
import subprocess
import time

from .authproxy import JSONRPCException
from .util import (
    assert_equal,
    get_rpc_proxy,
    rpc_url,
    wait_until,
    p2p_port,
)

# For Python 3.4 compatibility
JSONDecodeError = getattr(json, "JSONDecodeError", ValueError)

BITCOIND_PROC_WAIT_TIMEOUT = 60

class TestNode():
    """A class for representing a bitcoind node under test.

    This class contains:

    - state about the node (whether it's running, etc)
    - a Python subprocess.Popen object representing the running process
    - an RPC connection to the node
    - one or more P2P connections to the node


    To make things easier for the test writer, any unrecognised messages will
    be dispatched to the RPC connection."""

    def __init__(self, i, dirname, extra_args, rpchost, timewait, binary, stderr, mocktime, coverage_dir, use_cli=False):
        self.index = i
        self.datadir = os.path.join(dirname, "node" + str(i))
        self.rpchost = rpchost
        if timewait:
            self.rpc_timeout = timewait
        else:
            # Wait for up to 60 seconds for the RPC server to respond
            self.rpc_timeout = 60
        if binary is None:
            self.binary = os.getenv("BITCOIND", "bitcoind")
        else:
            self.binary = binary
        self.stderr = stderr
        self.coverage_dir = coverage_dir
        # Most callers will just need to add extra args to the standard list below. For those callers that need more flexibity, they can just set the args property directly.
        self.extra_args = extra_args
        self.args = [self.binary, "-datadir=" + self.datadir, "-server", "-keypool=1", "-discover=0", "-rest", "-logtimemicros", "-debug", "-debugexclude=libevent", "-debugexclude=leveldb", "-mocktime=" + str(mocktime), "-uacomment=testnode%d" % i]

        self.cli = TestNodeCLI(os.getenv("BITCOINCLI", "bitcoin-cli"), self.datadir)
        self.use_cli = use_cli

        self.running = False
        self.process = None
        self.rpc_connected = False
        self.rpc = None
        self.url = None
        self.log = logging.getLogger('TestFramework.node%d' % i)

        self.p2ps = []

    def __getattr__(self, name):
        """Dispatches any unrecognised messages to the RPC connection or a CLI instance."""
        if self.use_cli:
            return getattr(self.cli, name)
        else:
            assert self.rpc_connected and self.rpc is not None, "Error: no RPC connection"
            return getattr(self.rpc, name)

    def start(self, extra_args=None, stderr=None, *args, **kwargs):
        """Start the node."""
        if extra_args is None:
            extra_args = self.extra_args
        if stderr is None:
            stderr = self.stderr
        self.process = subprocess.Popen(self.args + extra_args, stderr=stderr, *args, **kwargs)
        self.running = True
        self.log.debug("bitcoind started, waiting for RPC to come up")

    def wait_for_rpc_connection(self):
        """Sets up an RPC connection to the bitcoind process. Returns False if unable to connect."""
        # Poll at a rate of four times per second
        poll_per_s = 4
        for _ in range(poll_per_s * self.rpc_timeout):
            assert self.process.poll() is None, "bitcoind exited with status %i during initialization" % self.process.returncode
            try:
                self.rpc = get_rpc_proxy(rpc_url(self.datadir, self.index, self.rpchost), self.index, timeout=self.rpc_timeout, coveragedir=self.coverage_dir)
                self.rpc.getblockcount()
                # If the call to getblockcount() succeeds then the RPC connection is up
                self.rpc_connected = True
                self.url = self.rpc.url
                self.log.debug("RPC successfully started")
                return
            except IOError as e:
                if e.errno != errno.ECONNREFUSED:  # Port not yet open?
                    raise  # unknown IO error
            except JSONRPCException as e:  # Initialization phase
                if e.error['code'] != -28:  # RPC in warmup?
                    raise  # unknown JSON RPC exception
            except ValueError as e:  # cookie file not found and no rpcuser or rpcassword. bitcoind still starting
                if "No RPC credentials" not in str(e):
                    raise
            time.sleep(1.0 / poll_per_s)
        raise AssertionError("Unable to connect to bitcoind")

    def get_wallet_rpc(self, wallet_name):
        if self.use_cli:
            return self.cli("-rpcwallet={}".format(wallet_name))
        else:
            assert self.rpc_connected
            assert self.rpc
            wallet_path = "wallet/%s" % wallet_name
            return self.rpc / wallet_path

    def stop_node(self):
        """Stop the node."""
        if not self.running:
            return
        self.log.debug("Stopping node")
        try:
            self.stop()
        except http.client.CannotSendRequest:
            self.log.exception("Unable to stop node.")
        del self.p2ps[:]

    def is_node_stopped(self):
        """Checks whether the node has stopped.

        Returns True if the node has stopped. False otherwise.
        This method is responsible for freeing resources (self.process)."""
        if not self.running:
            return True
        return_code = self.process.poll()
        if return_code is None:
            return False

        # process has stopped. Assert that it didn't return an error code.
        assert_equal(return_code, 0)
        self.running = False
        self.process = None
        self.rpc_connected = False
        self.rpc = None
        self.log.debug("Node stopped")
        return True

    def wait_until_stopped(self, timeout=BITCOIND_PROC_WAIT_TIMEOUT):
        wait_until(self.is_node_stopped, timeout=timeout)

    def node_encrypt_wallet(self, passphrase):
        """"Encrypts the wallet.

        This causes bitcoind to shutdown, so this method takes
        care of cleaning up resources."""
        self.encryptwallet(passphrase)
        self.wait_until_stopped()

    def add_p2p_connection(self, p2p_conn, *args, **kwargs):
        """Add a p2p connection to the node.

        This method adds the p2p connection to the self.p2ps list and also
        returns the connection to the caller."""
        if 'dstport' not in kwargs:
            kwargs['dstport'] = p2p_port(self.index)
        if 'dstaddr' not in kwargs:
            kwargs['dstaddr'] = '127.0.0.1'

        p2p_conn.peer_connect(*args, **kwargs)
        self.p2ps.append(p2p_conn)

        return p2p_conn

    @property
    def p2p(self):
        """Return the first p2p connection

        Convenience property - most tests only use a single p2p connection to each
        node, so this saves having to write node.p2ps[0] many times."""
        assert self.p2ps, "No p2p connection"
        return self.p2ps[0]

    def disconnect_p2ps(self):
        """Close all p2p connections to the node."""
        for p in self.p2ps:
            p.peer_disconnect()
        del self.p2ps[:]

class TestNodeCLIAttr:
    def __init__(self, cli, command):
        self.cli = cli
        self.command = command

    def __call__(self, *args, **kwargs):
        return self.cli.send_cli(self.command, *args, **kwargs)

    def get_request(self, *args, **kwargs):
        return lambda: self(*args, **kwargs)

class TestNodeCLI():
    """Interface to bitcoin-cli for an individual node"""

    def __init__(self, binary, datadir):
        self.options = []
        self.binary = binary
        self.datadir = datadir
        self.input = None
        self.log = logging.getLogger('TestFramework.bitcoincli')

    def __call__(self, *options, input=None):
        # TestNodeCLI is callable with bitcoin-cli command-line options
        cli = TestNodeCLI(self.binary, self.datadir)
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

    def send_cli(self, command=None, *args, **kwargs):
        """Run bitcoin-cli command. Deserializes returned string as python object."""

        pos_args = [str(arg) for arg in args]
        named_args = [str(key) + "=" + str(value) for (key, value) in kwargs.items()]
        assert not (pos_args and named_args), "Cannot use positional arguments and named arguments in the same bitcoin-cli call"
        p_args = [self.binary, "-datadir=" + self.datadir] + self.options
        if named_args:
            p_args += ["-named"]
        if command is not None:
            p_args += [command]
        p_args += pos_args + named_args
        self.log.debug("Running litecoin-cli command: %s" % command)
        process = subprocess.Popen(p_args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        cli_stdout, cli_stderr = process.communicate(input=self.input)
        returncode = process.poll()
        if returncode:
            match = re.match(r'error code: ([-0-9]+)\nerror message:\n(.*)', cli_stderr)
            if match:
                code, message = match.groups()
                raise JSONRPCException(dict(code=int(code), message=message))
            # Ignore cli_stdout, raise with cli_stderr
            raise subprocess.CalledProcessError(returncode, self.binary, output=cli_stderr)
        try:
            return json.loads(cli_stdout, parse_float=decimal.Decimal)
        except JSONDecodeError:
            return cli_stdout.rstrip("\n")
