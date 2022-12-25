#!/usr/bin/env python3
# Copyright (c) 2021-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Stress tests related to node initialization."""
import os
from pathlib import Path

from test_framework.test_framework import SyscoinTestFramework, SkipTest
from test_framework.test_node import ErrorMatch
from test_framework.util import assert_equal


class InitStressTest(SyscoinTestFramework):
    """
    Ensure that initialization can be interrupted at a number of points and not impair
    subsequent starts.
    """

    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1

    def run_test(self):
        """
        - test terminating initialization after seeing a certain log line.
        - test removing certain essential files to test startup error paths.
        """
        # TODO: skip Windows for now since it isn't clear how to SIGTERM.
        #
        # Windows doesn't support `process.terminate()`.
        # and other approaches (like below) don't work:
        #
        #   os.kill(node.process.pid, signal.CTRL_C_EVENT)
        if os.name == 'nt':
            raise SkipTest("can't SIGTERM on Windows")

        self.stop_node(0)
        node = self.nodes[0]

        def sigterm_node():
            node.process.terminate()
            node.process.wait()

        def check_clean_start():
            """Ensure that node restarts successfully after various interrupts."""
            node.start()
            node.wait_for_rpc_connection()
            assert_equal(200, node.getblockcount())

        lines_to_terminate_after = [
            b'Validating signatures for all blocks',
            b'scheduler thread start',
            b'Starting HTTP server',
            b'Loading P2P addresses',
            b'Loading banlist',
            b'Loading block index',
            b'Checking all blk files are present',
            b'Loaded best chain:',
            b'init message: Verifying blocks',
            b'init message: Starting network threads',
            b'net thread start',
            b'addcon thread start',
            b'loadblk thread start',
            b'txindex thread start',
            b'block filter index thread start',
            b'coinstatsindex thread start',
            b'msghand thread start',
            b'net thread start',
            b'addcon thread start',
        ]
        if self.is_wallet_compiled():
            lines_to_terminate_after.append(b'Verifying wallet')

        for terminate_line in lines_to_terminate_after:
            self.log.info(f"Starting node and will exit after line {terminate_line}")
            with node.wait_for_debug_log([terminate_line]):
                node.start(extra_args=['-txindex=1', '-blockfilterindex=1', '-coinstatsindex=1'])
            self.log.debug("Terminating node after terminate line was found")
            sigterm_node()

        check_clean_start()
        self.stop_node(0)

        self.log.info("Test startup errors after removing certain essential files")

        files_to_disturb = {
            'blocks/index/*.ldb': 'Error opening block database.',
            'chainstate/*.ldb': 'Error opening block database.',
            'blocks/blk*.dat': 'Error loading block database.',
        }

        for file_patt, err_fragment in files_to_disturb.items():
            target_files = list(node.chain_path.glob(file_patt))

            for target_file in target_files:
                self.log.info(f"Tweaking file to ensure failure {target_file}")
                bak_path = str(target_file) + ".bak"
                target_file.rename(bak_path)

            # TODO: at some point, we should test perturbing the files instead of removing
            # them, e.g.
            #
            # contents = target_file.read_bytes()
            # tweaked_contents = bytearray(contents)
            # tweaked_contents[50:250] = b'1' * 200
            # target_file.write_bytes(bytes(tweaked_contents))
            #
            # At the moment I can't get this to work (syscoind loads successfully?) so
            # investigate doing this later.

            node.assert_start_raises_init_error(
                extra_args=['-txindex=1', '-blockfilterindex=1', '-coinstatsindex=1'],
                expected_msg=err_fragment,
                match=ErrorMatch.PARTIAL_REGEX,
            )

            for target_file in target_files:
                bak_path = str(target_file) + ".bak"
                self.log.debug(f"Restoring file from {bak_path} and restarting")
                Path(bak_path).rename(target_file)

            check_clean_start()
            self.stop_node(0)


if __name__ == '__main__':
    InitStressTest().main()
