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
import re
import shutil
import subprocess
from pathlib import Path


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
        # Install directories configured at build time. CMAKE_INSTALL_BINDIR and
        # CMAKE_INSTALL_LIBEXECDIR can be relative to the install prefix (the
        # default) or absolute (Nix passes absolute paths), so config.ini has
        # the absolute CMAKE_INSTALL_FULL_* values. The fake install tree only
        # needs to reproduce the bindir name and the relative path from bindir
        # to libexecdir, e.g. "bin" and "../libexec".
        bindir = Path(self.config["environment"]["BINDIR"])
        libexecdir = Path(os.path.relpath(self.config["environment"]["LIBEXECDIR"], bindir))
        prefix = Path(self.options.tmpdir) / "fake-prefix"
        datadir = self.nodes[0].datadir_path / "wrapper-datadir"
        datadir.mkdir()

        def make_layout(name, internal_dir):
            # Lay out <prefix>/<name>/.../<bindir name>/bitcoin and
            # <bindir>/<internal_dir>/bitcoind, nesting the bindir deep enough
            # that ".." components in internal_dir stay inside <prefix>/<name>.
            wrapper_dir = prefix / name / Path(*["sub"] * internal_dir.parts.count("..")) / bindir.name
            internal_path = Path(os.path.normpath(wrapper_dir / internal_dir))
            wrapper_dir.mkdir(parents=True)
            internal_path.mkdir(parents=True, exist_ok=True)
            wrapper = wrapper_dir / f"bitcoin{exeext}"
            shutil.copy2(paths.bitcoin_bin, wrapper)
            # `bitcoin node` (with no -ipc* option) execs `bitcoind`. Placing
            # bitcoind only in the internal directory, not next to the wrapper,
            # forces the wrapper to resolve it through the bindir -> internal-dir
            # lookup rather than finding it as a sibling.
            shutil.copy2(paths.bitcoind, internal_path / f"bitcoind{exeext}")
            return wrapper

        def run_wrapper(wrapper):
            valgrind_cmd = self.nodes[0].binaries.valgrind_cmd
            if valgrind_cmd:
                # which(): PATH is blanked below, so resolve valgrind now.
                valgrind_cmd = [shutil.which(valgrind_cmd[0]) or valgrind_cmd[0], *valgrind_cmd[1:]]
            env = os.environ.copy()
            # The wrapper is invoked by absolute path; blank out PATH so a lookup
            # miss cannot be satisfied by an unrelated bitcoind on the system.
            env["PATH"] = ""
            return subprocess.run(valgrind_cmd + [wrapper, "node", f"-datadir={datadir}", "-version"],
                                  capture_output=True, env=env, timeout=60)

        self.log.info(f"Ensure installed wrapper finds internal binaries in configured {libexecdir}/")
        result = run_wrapper(make_layout("found", libexecdir))
        assert_equal(result.returncode, 0)
        assert_equal(get_exe_name(result.stdout), b"bitcoind")

        other_dir = libexecdir.with_name(libexecdir.name + "_notfound")
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
