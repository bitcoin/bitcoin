#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the bitcoin wrapper tool."""
from test_framework.test_framework import (
    BitcoinTestFramework,
)
from test_framework.util import (
    append_config,
    assert_equal,
)

import os
import platform
import re
import shutil
import subprocess


class ToolBitcoinTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self):
        """Set up nodes normally, but save a copy of their arguments before starting them."""
        self.add_nodes(self.num_nodes, self.extra_args)
        node_argv = self.get_binaries().node_argv()
        self.node_options = [node.args[len(node_argv):] for node in self.nodes]
        for node in self.nodes:
            assert_equal(node.args[:len(node_argv)], node_argv)

    def set_cmd_args(self, node, args):
        """Set up node so it will be started through bitcoin wrapper command with specified arguments."""
        # Manually construct the `bitcoin node` command, similar to Binaries::node_argv()
        bitcoin_cmd = node.binaries.valgrind_cmd + [node.binaries.paths.bitcoin_bin]
        node.args = bitcoin_cmd + args + ["node"] + self.node_options[node.index]

    def test_args(self, cmd_args, node_args, expect_exe=None, expect_error=None):
        node = self.nodes[0]
        self.set_cmd_args(node, cmd_args)
        extra_args = node_args + ["-version"]
        if expect_error is not None:
            node.assert_start_raises_init_error(expected_msg=expect_error, extra_args=extra_args)
        else:
            assert expect_exe
            node.start(extra_args=extra_args)
            ret, out, err = get_node_output(node)
            try:
                assert_equal(get_exe_name(out), expect_exe.encode())
                assert_equal(err, b"")
            except Exception as e:
                raise RuntimeError(f"Unexpected output from {node.args + extra_args}: {out=!r} {err=!r} {ret=!r}") from e

    def test_windows_exit_status(self):
        self.log.info("Ensure bitcoin preserves child exit status on Windows")
        exe_dir = self.nodes[0].datadir_path / "exit_status"
        exe_dir.mkdir()
        wrapper = exe_dir / "bitcoin.exe"
        shutil.copyfile(self.get_binaries().paths.bitcoin_bin, wrapper)
        # Use cmd.exe as a fake bitcoind with a controllable exit status.
        shutil.copyfile(os.path.join(os.environ["SystemRoot"], "System32", "cmd.exe"), exe_dir / "bitcoind.exe")
        for status in (0, 1, -1):
            result = subprocess.run(
                [str(wrapper), "-M", "node", "/d", "/c", "exit", str(status)],
                capture_output=True, timeout=30,
            )
            # Windows process exit codes are unsigned 32-bit values.
            assert_equal(result.returncode, status & 0xFFFFFFFF)
            assert_equal(result.stdout, b"")
            assert_equal(result.stderr, b"")

    def test_launch_failure(self):
        self.log.info("Ensure bitcoin reports an error if bitcoind can't be started")
        # Copy the wrapper to a directory without bitcoind. The wrapper is run
        # by path, so it doesn't fall back to searching PATH and should fail.
        exe_dir = self.nodes[0].datadir_path / "launch_failure"
        exe_dir.mkdir()
        bitcoin_bin = self.get_binaries().paths.bitcoin_bin
        wrapper = exe_dir / os.path.basename(bitcoin_bin)
        # Use copy instead of copyfile to keep the executable bit.
        shutil.copy(bitcoin_bin, wrapper)
        result = subprocess.run([str(wrapper), "-M", "node", "-version"], capture_output=True, timeout=30)
        assert_equal(result.returncode, 1)
        assert_equal(result.stdout, b"")
        assert b"execvp failed to execute" in result.stderr

    def run_test(self):
        node = self.nodes[0]

        if platform.system() == "Windows":
            self.test_windows_exit_status()

        self.test_launch_failure()

        self.log.info("Ensure bitcoin node command invokes bitcoind by default")
        self.test_args([], [], expect_exe="bitcoind")

        self.log.info("Ensure bitcoin -M invokes bitcoind")
        self.test_args(["-M"], [], expect_exe="bitcoind")

        self.log.info("Ensure bitcoin -M does not accept -ipcbind")
        self.test_args(["-M"], ["-ipcbind=unix"], expect_error='Error: Error parsing command line arguments: Invalid parameter -ipcbind=unix')

        if self.is_ipc_compiled():
            self.log.info("Ensure bitcoin -m invokes bitcoin-node")
            self.test_args(["-m"], [], expect_exe="bitcoin-node")

            self.log.info("Ensure bitcoin -m does accept -ipcbind")
            self.test_args(["-m"], ["-ipcbind=unix"], expect_exe="bitcoin-node")

            self.log.info("Ensure bitcoin accepts -ipcbind by default")
            self.test_args([], ["-ipcbind=unix"], expect_exe="bitcoin-node")

            self.log.info("Ensure bitcoin recognizes -ipcbind in config file")
            append_config(node.datadir_path, ["ipcbind=unix"])
            self.test_args([], [], expect_exe="bitcoin-node")


def get_node_output(node):
    ret = node.process.wait(timeout=60)
    node.stdout.seek(0)
    node.stderr.seek(0)
    out = node.stdout.read()
    err = node.stderr.read()
    node.stdout.close()
    node.stderr.close()

    # Clean up TestNode state
    node.running = False
    node.process = None
    node.rpc_connected = False
    node.rpc = None

    return ret, out, err


def get_exe_name(version_str):
    """Get exe name from last word of first line of version string."""
    return re.match(rb".*?(\S+)\s*?(?:\n|$)", version_str.strip()).group(1)


if __name__ == '__main__':
    ToolBitcoinTest(__file__).main()
