#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Happy-path reobfuscation test:

- Node starts once (framework), which creates blk00000.dat / rev00000.dat.
- Stop it, restart with -reobfuscate-blocks=1.
- Verify xor.dat exists and files changed.
- Start again without the flag and ensure node runs fine.
"""

from pathlib import Path

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

KEY_SIZE = 8  # xor.dat is raw 8 bytes
SLICE = 64 * 1024  # compare first 64 KiB to avoid hashing huge files


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

    def _head(self, p: Path) -> bytes:
        with open(p, "rb") as f:
            return f.read(SLICE)

    def run_test(self):
        paths = self._paths()
        blk0, rev0, xor_dat = paths["blk0"], paths["rev0"], paths["xor_dat"]

        self.log.info("Sanity: blk00000.dat and rev00000.dat should exist")
        assert blk0.exists(), "blk00000.dat missing"
        assert rev0.exists(), "rev00000.dat missing"

        self.log.info("Snapshot the first slice of each file")
        before_blk = self._head(blk0)
        before_rev = self._head(rev0)

        self.log.info("Restarting with reobfuscation enabled")
        self.restart_node(0, extra_args=self.extra_args[0] + ["-reobfuscate-blocks"])

        self.log.info("Stop once reobfuscation finished")
        self.stop_node(0)

        self.log.info("xor.dat must exist and be 8 bytes")
        assert xor_dat.exists(), "xor.dat not created"
        assert_equal(xor_dat.stat().st_size, KEY_SIZE)

        self.log.info("Files should have been rewritten (first slice differs)")
        after_blk = self._head(blk0)
        after_rev = self._head(rev0)
        assert before_blk != after_blk, "blk00000.dat content did not change"
        assert before_rev != after_rev, "rev00000.dat content did not change"

        self.log.info("Start again without the flag; node should run fine")
        self.start_node(0, extra_args=self.extra_args[0])

        self.generate(self.nodes[0], 1)
        self.stop_node(0)

        self.log.info("Reobfuscation smoke test passed")


if __name__ == "__main__":
    ReobfuscateBlocksSmokeTest(__file__).main()
