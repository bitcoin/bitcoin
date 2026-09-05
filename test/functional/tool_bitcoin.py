#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the bitcoin wrapper tool."""
from test_framework.test_framework import (
    BitcoinTestFramework,
    SkipTest,
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

    def skip_test_if_missing_module(self):
        # Skip test on windows because currently when `bitcoin node -version` is
        # run on windows, python doesn't capture output from the child
        # `bitcoind` and `bitcoin-node` process started with _wexecvp, and
        # stdout/stderr are always empty. See
        # https://github.com/bitcoin/bitcoin/pull/33229#issuecomment-3265524908
        if platform.system() == "Windows":
            raise SkipTest("Test does not currently work on windows")

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

    def run_test(self):
        node = self.nodes[0]

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

        self.test_installed_layout()

    def test_installed_layout(self):
        """Check how an installed bitcoin wrapper resolves internal executables.

        When the wrapper is installed into a `bin/` directory, it looks for
        internal executables (bitcoind, bitcoin-node, ...) in a sibling
        directory under the same install prefix. The normal build-tree tests
        never exercise this because every binary sits next to the wrapper, so
        build a fake install prefix and invoke the wrapper by absolute path.
        """
        paths = self.get_binaries().paths
        exeext = self.config["environment"]["EXEEXT"]
        # Directory (relative to the install prefix) where the wrapper looks for
        # internal executables, configured at build time via
        # CMAKE_INSTALL_LIBEXECDIR. An absolute value is not relative to the
        # prefix constructed here, so skip the check in that case.
        libexecdir = self.config["environment"]["LIBEXECDIR"]
        if os.path.isabs(libexecdir):
            self.log.info("Skipping installed-layout check; CMAKE_INSTALL_LIBEXECDIR is absolute")
            return
        prefix = self.nodes[0].datadir_path / "fake-prefix"
        datadir = self.nodes[0].datadir_path / "wrapper-datadir"
        datadir.mkdir()

        def make_layout(name, internal_dir):
            # Lay out <prefix>/<name>/bin/bitcoin and <prefix>/<name>/<internal_dir>/bitcoind.
            root = prefix / name
            bindir = root / "bin"
            internaldir = root / internal_dir
            bindir.mkdir(parents=True)
            internaldir.mkdir(parents=True)
            wrapper = bindir / f"bitcoin{exeext}"
            shutil.copy2(paths.bitcoin_bin, wrapper)
            # `bitcoin node` (with no -ipc* option) execs `bitcoind`. Placing
            # bitcoind only in the internal directory, not next to the wrapper,
            # forces the wrapper to resolve it through the bin/ -> internal-dir
            # lookup rather than finding it as a sibling.
            shutil.copy2(paths.bitcoind, internaldir / f"bitcoind{exeext}")
            return wrapper

        def run_wrapper(wrapper):
            env = os.environ.copy()
            # The wrapper is invoked by absolute path; blank out PATH so a lookup
            # miss cannot be satisfied by an unrelated bitcoind on the system.
            env["PATH"] = ""
            return subprocess.run([wrapper, "node", f"-datadir={datadir}", "-version"],
                                  capture_output=True, env=env, timeout=60)

        self.log.info(f"Ensure installed wrapper finds internal binaries in configured {libexecdir}/")
        result = run_wrapper(make_layout("found", libexecdir))
        assert_equal(result.returncode, 0)
        assert_equal(get_exe_name(result.stdout), b"bitcoind")

        # A directory that differs from the configured one must not be searched.
        other_dir = "lib" if libexecdir != "lib" else "libexec"
        self.log.info(f"Ensure installed wrapper does not find internal binaries in unconfigured {other_dir}/")
        result = run_wrapper(make_layout("notfound", other_dir))
        assert result.returncode != 0, f"wrapper unexpectedly ran bitcoind from {other_dir}/: {result.stdout!r}"


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
