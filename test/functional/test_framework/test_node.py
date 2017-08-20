#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Class for dashd node under test"""

import errno
import http.client
import logging
import os
import subprocess
import time

from .util import (
    assert_equal,
    get_rpc_proxy,
    rpc_url,
)
from .authproxy import JSONRPCException

class TestNode():
    """A class for representing a dashd node under test.

    This class contains:

    - state about the node (whether it's running, etc)
    - a Python subprocess.Popen object representing the running process
    - an RPC connection to the node

    To make things easier for the test writer, a bit of magic is happening under the covers.
    Any unrecognised messages will be dispatched to the RPC connection."""

    def __init__(self, i, dirname, extra_args, rpchost, timewait, binary, stderr, mocktime, coverage_dir):
        self.index = i
        self.datadir = os.path.join(dirname, "node" + str(i))
        self.rpchost = rpchost
        self.rpc_timeout = timewait
        if binary is None:
            self.binary = os.getenv("BITCOIND", "dashd")
        else:
            self.binary = binary
        self.stderr = stderr
        self.coverage_dir = coverage_dir
        # Most callers will just need to add extra args to the standard list below. For those callers that need more flexibity, they can just set the args property directly.
        self.extra_args = extra_args
        self.args = [self.binary, "-datadir=" + self.datadir, "-server", "-keypool=1", "-discover=0", "-rest", "-logtimemicros", "-debug", "-debugexclude=libevent", "-debugexclude=leveldb", "-mocktime=" + str(mocktime), "-uacomment=testnode%d" % i]

        # Don't try auto backups (they fail a lot when running tests)
        self.args.append("-createwalletbackups=0")

        self.running = False
        self.process = None
        self.rpc_connected = False
        self.rpc = None
        self.url = None
        self.log = logging.getLogger('TestFramework.node%d' % i)

    def __getattr__(self, *args, **kwargs):
        """Dispatches any unrecognised messages to the RPC connection."""
        assert self.rpc_connected and self.rpc is not None, "Error: no RPC connection"
        return self.rpc.__getattr__(*args, **kwargs)

    def start(self):
        """Start the node."""
        self.process = subprocess.Popen(self.args + self.extra_args, stderr=self.stderr)
        self.running = True
        self.log.debug("dashd started, waiting for RPC to come up")

    def wait_for_rpc_connection(self):
        """Sets up an RPC connection to the dashd process. Returns False if unable to connect."""
        timeout_s = 60 # Wait for up to 60 seconds for the RPC server to respond
        poll_per_s = 4 # Poll at a rate of four times per second
        for _ in range(timeout_s*poll_per_s):
            assert not self.process.poll(), "dashd exited with status %i during initialization" % self.process.returncode
            try:
                self.rpc = get_rpc_proxy(rpc_url(self.datadir, self.index, self.rpchost), self.index, coveragedir=self.coverage_dir)
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
            except ValueError as e:  # cookie file not found and no rpcuser or rpcassword. dashd still starting
                if "No RPC credentials" not in str(e):
                    raise
            time.sleep(1.0 / poll_per_s)
        raise AssertionError("Unable to connect to dashd")

    def get_wallet_rpc(self, wallet_name):
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

        This causes dashd to shutdown, so this method takes
        care of cleaning up resources."""
        self.encryptwallet(passphrase)
        while not self.is_node_stopped():
            time.sleep(0.1)
        self.rpc = None
        self.rpc_connected = False
