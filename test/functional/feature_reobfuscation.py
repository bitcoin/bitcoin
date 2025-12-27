#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Happy-path reobfuscation test:

- Node starts once (framework), which creates blk00000.dat / rev00000.dat.
- Add many tiny blk*/rev* files so progress logging has milestones.
- Restart with -reobfuscate-blocks=1 and check logs + xor.dat.
- Restart without the flag; verify the active obfuscation key in logs matches xor.dat.
"""

from pathlib import Path

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class ReobfuscateBlocksSmokeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.uses_wallet = None
        self.extra_args = [[
            "-checkblocks=0",
            "-checklevel=0",
            "-txindex=0",
            "-coinstatsindex=0",
            "-blockfilterindex=0",
        ]]

    def _paths(self):
        blocks_dir = Path(self.nodes[0].blocks_path)
        return {
            "blocks_dir": blocks_dir,
            "blk0": blocks_dir / "blk00000.dat",
            "rev0": blocks_dir / "rev00000.dat",
            "xor_dat": blocks_dir / "xor.dat",
        }

    def run_test(self):
        paths = self._paths()
        blocks_dir, blk0, rev0, xor_dat = paths["blocks_dir"], paths["blk0"], paths["rev0"], paths["xor_dat"]

        assert blk0.exists() and rev0.exists(), "Sanity: blk00000.dat and rev00000.dat should exist"

        self.log.info("Snapshot initial contents")
        before_blk = blk0.read_bytes()
        before_rev = rev0.read_bytes()

        self.log.info("Add many dummy block/undo files for progress logging (10 pairs in total)")
        for i in range(1, 10):
            (blocks_dir / f"blk{i:05d}.dat").write_bytes(b"\0")
            (blocks_dir / f"rev{i:05d}.dat").write_bytes(b"\0")

        self.log.info("Restarting with reobfuscation enabled")
        with self.nodes[0].assert_debug_log(expected_msgs=[
            "[obfuscate] Reobfuscating 20 block and undo files",
            "% done",
        ]):
            self.restart_node(0, extra_args=self.extra_args[0] + ["-reobfuscate-blocks"])
            self.stop_node(0)

        assert xor_dat.exists(), "xor.dat not created"
        assert_equal(xor_dat.stat().st_size, 8)

        self.log.info("Files should have been rewritten")
        assert before_blk != blk0.read_bytes(), "blk00000.dat content did not change"
        assert before_rev != rev0.read_bytes(), "rev00000.dat content did not change"

        self.log.info("Start again without the flag; node should log the active obfuscation key matching xor.dat")
        with self.nodes[0].assert_debug_log(expected_msgs=[
            "Using obfuscation key for blocksdir *.dat files",
            f"'{xor_dat.read_bytes().hex()}'",
        ]):
            self.start_node(0, extra_args=self.extra_args[0])
            self.generate(self.nodes[0], 1)
            self.stop_node(0)


if __name__ == "__main__":
    ReobfuscateBlocksSmokeTest(__file__).main()
