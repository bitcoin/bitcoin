#!/usr/bin/env python3
# Copyright (c) 2021-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests related to node initialization."""
from pathlib import Path
import os
import platform
import shutil
import signal
import subprocess

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import (
    BITCOIN_PID_FILENAME_DEFAULT,
    ErrorMatch,
)
from test_framework.util import assert_equal


class InitTest(BitcoinTestFramework):
    """
    Ensure that initialization can be interrupted at a number of points and not impair
    subsequent starts.
    """

    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.uses_wallet = None

    def init_stress_test(self):
        """
        - test terminating initialization after seeing a certain log line.
        - test removing certain essential files to test startup error paths.
        """
        self.stop_node(0)
        node = self.nodes[0]

        def sigterm_node():
            if platform.system() == 'Windows':
                # Don't call Python's terminate() since it calls
                # TerminateProcess(), which unlike SIGTERM doesn't allow
                # bitcoind to perform any shutdown logic.
                os.kill(node.process.pid, signal.CTRL_BREAK_EVENT)
            else:
                node.process.terminate()
            node.process.wait()

        def start_expecting_error(err_fragment):
            node.assert_start_raises_init_error(
                extra_args=['-txindex=1', '-blockfilterindex=1', '-coinstatsindex=1', '-checkblocks=200', '-checklevel=4'],
                expected_msg=err_fragment,
                match=ErrorMatch.PARTIAL_REGEX,
            )

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
            b'initload thread start',
            b'txindex thread start',
            b'block filter index thread start',
            b'coinstatsindex thread start',
            b'msghand thread start',
            b'net thread start',
            b'addcon thread start',
        ]
        if self.is_wallet_compiled():
            lines_to_terminate_after.append(b'Verifying wallet')

        args = ['-txindex=1', '-blockfilterindex=1', '-coinstatsindex=1']
        for terminate_line in lines_to_terminate_after:
            self.log.info(f"Starting node and will terminate after line {terminate_line}")
            with node.busy_wait_for_debug_log([terminate_line]):
                if platform.system() == 'Windows':
                    # CREATE_NEW_PROCESS_GROUP is required in order to be able
                    # to terminate the child without terminating the test.
                    node.start(extra_args=args, creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)
                else:
                    node.start(extra_args=args)
            self.log.debug("Terminating node after terminate line was found")
            sigterm_node()

        check_clean_start()
        self.stop_node(0)

        self.log.info("Test startup errors after removing certain essential files")

        files_to_delete = {
            'blocks/index/*.ldb': 'Error opening block database.',
            'chainstate/*.ldb': 'Error opening coins database.',
            'blocks/blk*.dat': 'Error loading block database.',
            'indexes/txindex/MANIFEST*': 'LevelDB error: Corruption: CURRENT points to a non-existent file',
            # Removing these files does not result in a startup error:
            # 'indexes/blockfilter/basic/*.dat', 'indexes/blockfilter/basic/db/*.*', 'indexes/coinstats/db/*.*',
            # 'indexes/txindex/*.log', 'indexes/txindex/CURRENT', 'indexes/txindex/LOCK'
        }

        files_to_perturb = {
            'blocks/index/*.ldb': 'Error loading block database.',
            'chainstate/*.ldb': 'Error opening coins database.',
            'blocks/blk*.dat': 'Corrupted block database detected.',
            'indexes/blockfilter/basic/db/*.*': 'LevelDB error: Corruption',
            'indexes/coinstats/db/*.*': 'LevelDB error: Corruption',
            'indexes/txindex/*.log': 'LevelDB error: Corruption',
            'indexes/txindex/CURRENT': 'LevelDB error: Corruption',
            # Perturbing these files does not result in a startup error:
            # 'indexes/blockfilter/basic/*.dat', 'indexes/txindex/MANIFEST*', 'indexes/txindex/LOCK'
        }

        for file_patt, err_fragment in files_to_delete.items():
            target_files = list(node.chain_path.glob(file_patt))

            for target_file in target_files:
                self.log.info(f"Deleting file to ensure failure {target_file}")
                bak_path = str(target_file) + ".bak"
                target_file.rename(bak_path)

            start_expecting_error(err_fragment)

            for target_file in target_files:
                bak_path = str(target_file) + ".bak"
                self.log.debug(f"Restoring file from {bak_path} and restarting")
                Path(bak_path).rename(target_file)

            check_clean_start()
            self.stop_node(0)

        self.log.info("Test startup errors after perturbing certain essential files")
        dirs = ["blocks", "chainstate", "indexes"]
        for file_patt, err_fragment in files_to_perturb.items():
            for dir in dirs:
                shutil.copytree(node.chain_path / dir, node.chain_path / f"{dir}_bak")
            target_files = list(node.chain_path.glob(file_patt))

            for target_file in target_files:
                self.log.info(f"Perturbing file to ensure failure {target_file}")
                with open(target_file, "r+b") as tf:
                    # Since the genesis block is not checked by -checkblocks, the
                    # perturbation window must be chosen such that a higher block
                    # in blk*.dat is affected.
                    tf.seek(150)
                    tf.write(b"1" * 200)

            start_expecting_error(err_fragment)

            for dir in dirs:
                shutil.rmtree(node.chain_path / dir)
                shutil.move(node.chain_path / f"{dir}_bak", node.chain_path / dir)

    def init_pid_test(self):
        BITCOIN_PID_FILENAME_CUSTOM = "my_fancy_bitcoin_pid_file.foobar"

        self.log.info("Test specifying custom pid file via -pid command line option")
        custom_pidfile_relative = BITCOIN_PID_FILENAME_CUSTOM
        self.log.info(f"-> path relative to datadir ({custom_pidfile_relative})")
        self.restart_node(0, [f"-pid={custom_pidfile_relative}"])
        datadir = self.nodes[0].chain_path
        assert not (datadir / BITCOIN_PID_FILENAME_DEFAULT).exists()
        assert (datadir / custom_pidfile_relative).exists()
        self.stop_node(0)
        assert not (datadir / custom_pidfile_relative).exists()

        custom_pidfile_absolute = Path(self.options.tmpdir) / BITCOIN_PID_FILENAME_CUSTOM
        self.log.info(f"-> absolute path ({custom_pidfile_absolute})")
        self.restart_node(0, [f"-pid={custom_pidfile_absolute}"])
        assert not (datadir / BITCOIN_PID_FILENAME_DEFAULT).exists()
        assert custom_pidfile_absolute.exists()
        self.stop_node(0)
        assert not custom_pidfile_absolute.exists()

    def run_test(self):
        self.init_pid_test()
        self.init_stress_test()


if __name__ == '__main__':
    InitTest(__file__).main()
