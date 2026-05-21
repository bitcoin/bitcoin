#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests for the `TestShell` submodule."""

from contextlib import redirect_stdout
from decimal import Decimal
from io import StringIO
from pathlib import Path

# Note that we need to import from functional test framework modules
# *after* extending the Python path via sys.path.insert(0, ...) below,
# in order to work with the full symlinked (unresolved) path within the
# build directory (usually ./build/test/functional).


# Test matching the minimal example from the documentation. Should be kept
# in sync with the interactive shell instructions ('>>> ') in test-shell.md.
def run_testshell_doc_example(functional_tests_dir):
    import sys
    sys.path.insert(0, functional_tests_dir)
    from test_framework.test_shell import TestShell
    from test_framework.util import assert_equal

    test = TestShell().setup(num_nodes=2, setup_clean_chain=True)
    try:
        assert test is not None
        stdout_buf = StringIO()
        with redirect_stdout(stdout_buf):
            test2 = TestShell().setup()
        assert test2 is None
        assert_equal(stdout_buf.getvalue().rstrip(), "TestShell is already running!")
        assert_equal(test.nodes[0].getblockchaininfo()["blocks"], 0)
        if test.is_wallet_compiled():
            res = test.nodes[0].createwallet('default')
            assert_equal(res, {'name': 'default'})
            address = test.nodes[0].getnewaddress()
            res = test.generatetoaddress(test.nodes[0], 101, address)
            assert_equal(len(res), 101)
            test.sync_blocks()
            assert_equal(test.nodes[1].getblockchaininfo()["blocks"], 101)
            assert_equal(test.nodes[0].getbalance(), Decimal('50.0'))
            test.nodes[0].log.info("Successfully mined regtest chain!")
    finally:
        test.shutdown()
        test.reset()
        assert test.num_nodes is None


if __name__ == "__main__":
    run_testshell_doc_example(str(Path(__file__).parent))
