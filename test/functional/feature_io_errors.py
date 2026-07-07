#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
"""Test handling of I/O errors."""

import contextlib
import os
import re
import stat

from test_framework.test_framework import (
    BitcoinTestFramework,
    SkipTest,
)


@contextlib.contextmanager
def simulate_io_error(target_path):
    # Make files under target_path inaccessible.
    # Simulating a detached volume or permission change.
    backup = target_path.with_name(target_path.name + "_bak")
    parent_dir = target_path.parent
    old_mode = stat.S_IMODE(os.stat(parent_dir).st_mode)

    os.rename(target_path, backup)
    os.chmod(parent_dir, 0o500)  # Prevent dir re-creation
    try:
        yield
    finally:
        os.chmod(parent_dir, old_mode)
        os.rename(backup, target_path)


class BlockstoreIOErrorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_posix()
        if os.geteuid() == 0:
            raise SkipTest("Unable to enforce dir permissions")

    def test_scheduler_thread_disk_space_check(self):
        node = self.nodes[0]

        with (
            simulate_io_error(node.blocks_path),
            node.assert_debug_log(["EXCEPTION: "]),
        ):
            self.log.info("Mock scheduler to run disk space check")
            try:
                node.mockscheduler(5 * 60)
            except Exception:
                pass  # Node may shut down

            node.wait_until_stopped(
                expect_error=True,
                expected_ret_code=-6,
                expected_stderr=re.compile(".*EXCEPTION: .*"),
            )

    def test_blockfilterindex_allocation_failure(self):
        node = self.nodes[0]
        self.restart_node(
            0,
            [
                "-test=blockfilterindex_small_chunks",
                "-blockfilterindex",
            ],
        )

        self.wait_until(
            lambda: node.getindexinfo()["basic block filter index"]["synced"]
        )
        index_path = node.chain_path / "indexes" / "blockfilter" / "basic"
        self.generate(node, 26)  # fill first chunk of the index file

        self.log.info("Generate block to allocate next chunk in blockfilterindex file")

        with (
            simulate_io_error(index_path),
            node.assert_debug_log(["EXCEPTION: "]),
        ):
            try:
                self.generate(node, 1)
            except Exception:
                pass  # Node may shut down

            node.wait_until_stopped(
                expect_error=True,
                expected_ret_code=-6,
                expected_stderr=re.compile(".*EXCEPTION: .*"),
            )

    def test_block_file_out_of_space_error(self):
        node = self.nodes[0]
        self.restart_node(0, ["-fastprune"])

        self.log.info("Generate block big enough to allocate next chunk in block file")
        fastprune_chunk_size = 0x4000
        opreturn = "6a"
        nulldata = fastprune_chunk_size * "ff"

        with (
            simulate_io_error(node.blocks_path),
            node.assert_debug_log(["System error while saving block"]),
        ):
            try:
                self.generateblock(
                    node, output=f"raw({opreturn}{nulldata})", transactions=[]
                )
            except Exception:
                pass  # Node may shut down

            node.wait_until_stopped(
                expect_error=True,
                expected_stderr=re.compile(
                    r"Error: A fatal internal error occurred, see debug.log for details: "
                    r"System error while saving block to disk: .*"
                ),
            )

    def run_test(self):
        self.test_scheduler_thread_disk_space_check()
        self.test_blockfilterindex_allocation_failure()
        self.test_block_file_out_of_space_error()


if __name__ == "__main__":
    BlockstoreIOErrorTest(__file__).main()
