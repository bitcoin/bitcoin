#!/usr/bin/env python3
# Copyright (c) 2021-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests related to node initialization."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import os
import platform
import shutil
import signal
import subprocess
import time

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

        def start_expecting_error(err_fragment, args):
            node.assert_start_raises_init_error(
                extra_args=args,
                expected_msg=err_fragment,
                match=ErrorMatch.PARTIAL_REGEX,
            )

        def check_clean_start(extra_args):
            """Ensure that node restarts successfully after various interrupts."""
            node.start(extra_args)
            node.wait_for_rpc_connection()
            height = node.getblockcount()
            assert_equal(200, height)
            self.wait_until(lambda: all(i["synced"] and i["best_block_height"] == height for i in node.getindexinfo().values()))

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

        # Prior to deleting/perturbing index files, start node with all indexes enabled.
        # 'check_clean_start' will ensure indexes are synchronized (i.e., data exists to modify)
        check_clean_start(args)
        self.stop_node(0)

        self.log.info("Test startup errors after removing certain essential files")

        deletion_rounds = [
            {
                'filepath_glob': 'blocks/index/*.ldb',
                'error_message': 'Error opening block database.',
                'startup_args': [],
            },
            {
                'filepath_glob': 'chainstate/*.ldb',
                'error_message': 'Error opening coins database.',
                'startup_args': ['-checklevel=4'],
            },
            {
                'filepath_glob': 'blocks/blk*.dat',
                'error_message': 'Error loading block database.',
                'startup_args': ['-checkblocks=200', '-checklevel=4'],
            },
            {
                'filepath_glob': 'indexes/txindex/MANIFEST*',
                'error_message': 'LevelDB error: Corruption: CURRENT points to a non-existent file',
                'startup_args': ['-txindex=1'],
            },
            # Removing these files does not result in a startup error:
            # 'indexes/blockfilter/basic/*.dat', 'indexes/blockfilter/basic/db/*.*', 'indexes/coinstatsindex/db/*.*',
            # 'indexes/txindex/*.log', 'indexes/txindex/CURRENT', 'indexes/txindex/LOCK'
        ]

        perturbation_rounds = [
            {
                'filepath_glob': 'blocks/index/*.ldb',
                'error_message': 'Error loading block database.',
                'startup_args': [],
            },
            {
                'filepath_glob': 'chainstate/*.ldb',
                'error_message': 'Error opening coins database.',
                'startup_args': [],
            },
            {
                'filepath_glob': 'blocks/blk*.dat',
                'error_message': 'Corrupted block database detected.',
                'startup_args': ['-checkblocks=200', '-checklevel=4'],
            },
            {
                'filepath_glob': 'indexes/blockfilter/basic/db/*.*',
                'error_message': 'LevelDB error: Corruption',
                'startup_args': ['-blockfilterindex=1'],
            },
            {
                'filepath_glob': 'indexes/coinstatsindex/db/*.*',
                'error_message': 'LevelDB error: Corruption',
                'startup_args': ['-coinstatsindex=1'],
            },
            {
                'filepath_glob': 'indexes/txindex/*.log',
                'error_message': 'LevelDB error: Corruption',
                'startup_args': ['-txindex=1'],
            },
            {
                'filepath_glob': 'indexes/txindex/CURRENT',
                'error_message': 'LevelDB error: Corruption',
                'startup_args': ['-txindex=1'],
            },
            # Perturbing these files does not result in a startup error:
            # 'indexes/blockfilter/basic/*.dat', 'indexes/txindex/MANIFEST*', 'indexes/txindex/LOCK'
        ]

        for round_info in deletion_rounds:
            file_patt = round_info['filepath_glob']
            err_fragment = round_info['error_message']
            startup_args = round_info['startup_args']
            target_files = list(node.chain_path.glob(file_patt))

            for target_file in target_files:
                self.log.info(f"Deleting file to ensure failure {target_file}")
                bak_path = str(target_file) + ".bak"
                target_file.rename(bak_path)

            start_expecting_error(err_fragment, startup_args)

            for target_file in target_files:
                bak_path = str(target_file) + ".bak"
                self.log.debug(f"Restoring file from {bak_path} and restarting")
                Path(bak_path).rename(target_file)

            check_clean_start(args)
            self.stop_node(0)

        self.log.info("Test startup errors after perturbing certain essential files")
        dirs = ["blocks", "chainstate", "indexes"]
        for round_info in perturbation_rounds:
            file_patt = round_info['filepath_glob']
            err_fragment = round_info['error_message']
            startup_args = round_info['startup_args']

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

            start_expecting_error(err_fragment, startup_args)

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

    def break_wait_test(self):
        """Test what happens when a break signal is sent during a
        waitforblockheight RPC call with a long timeout. Ctrl-Break is sent on
        Windows and SIGTERM is sent on other platforms, to trigger the same node
        shutdown sequence that would happen if Ctrl-C were pressed in a
        terminal. (This can be different than the node shutdown sequence that
        happens when the stop RPC is sent.)

        The waitforblockheight call should be interrupted and return right away,
        and not time out."""

        self.log.info("Testing waitforblockheight RPC call followed by break signal")
        node = self.nodes[0]

        if platform.system() == 'Windows':
            # CREATE_NEW_PROCESS_GROUP prevents python test from exiting
            # with STATUS_CONTROL_C_EXIT (-1073741510) when break is sent.
            self.start_node(node.index, creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)
        else:
            self.start_node(node.index)

        current_height = node.getblock(node.getbestblockhash())['height']

        with ThreadPoolExecutor(max_workers=1) as ex:
            # Call waitforblockheight with wait timeout longer than RPC timeout,
            # so it is possible to distinguish whether it times out or returns
            # early. If it times out it will throw an exception, and if it
            # returns early it will return the current block height.
            self.log.debug(f"Calling waitforblockheight with {self.rpc_timeout} sec RPC timeout")
            fut = ex.submit(node.waitforblockheight, height=current_height+1, timeout=self.rpc_timeout*1000*2)
            time.sleep(1)

            self.log.debug(f"Sending break signal to pid {node.process.pid}")
            if platform.system() == 'Windows':
                # Note: CTRL_C_EVENT should not be sent here because unlike
                # CTRL_BREAK_EVENT it can not be targeted at a specific process
                # group and may behave unpredictably.
                node.process.send_signal(signal.CTRL_BREAK_EVENT)
            else:
                # Note: signal.SIGINT would work here as well
                node.process.send_signal(signal.SIGTERM)
            node.process.wait()

            result = fut.result()
            self.log.debug(f"waitforblockheight returned {result!r}")
            assert_equal(result["height"], current_height)
            node.wait_until_stopped()

    def run_test(self):
        self.init_pid_test()
        self.init_stress_test()
        self.break_wait_test()


if __name__ == '__main__':
    InitTest(__file__).main()
