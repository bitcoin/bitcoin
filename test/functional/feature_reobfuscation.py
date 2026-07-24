#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Reobfuscation coverage:

- Node starts once (framework), which creates blk00000.dat / rev00000.dat.
- Add many tiny blk*/rev* files so progress logging has milestones.
- Restart with -reobfuscate-blocks=1 and check logs + xor.dat.
- Restart without the flag; verify the active obfuscation key in logs matches xor.dat.
- Simulate interrupted rename stage and ensure resume.
- Invalid key values should fail fast.
- Reobfuscation should fail before rewriting files when block XOR is disabled.
- Do not suggest reobfuscation when block XOR is disabled.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import (
    NULL_BLK_XOR_KEY,
    NUM_XOR_BYTES,
)
from test_framework.util import (
    assert_equal,
    assert_not_equal,
    util_xor,
)


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

    def run_test(self):
        node = self.nodes[0]
        blocks_dir = node.blocks_path
        blk0 = blocks_dir / "blk00000.dat"
        rev0 = blocks_dir / "rev00000.dat"
        xor_dat = node.blocks_key_path

        assert blk0.exists() and rev0.exists(), "Sanity: blk00000.dat and rev00000.dat should exist"

        self.log.info("Snapshot initial contents")
        before_blk = blk0.read_bytes()
        before_rev = rev0.read_bytes()

        self.log.info("Add many dummy block/undo files for progress logging (10 pairs in total)")
        for i in range(1, 10):
            (blocks_dir / f"blk{i:05d}.dat").write_bytes(b"\0")
            (blocks_dir / f"rev{i:05d}.dat").write_bytes(b"\0")

        self.log.info("Restarting with reobfuscation enabled")
        with node.assert_debug_log(expected_msgs=[
            "[obfuscate] Reobfuscating 20 block and undo files",
            "% done",
        ]):
            self.restart_node(0, extra_args=self.extra_args[0] + ["-reobfuscate-blocks"])
            self.stop_node(0)

        assert xor_dat.exists(), "xor.dat not created"
        assert_equal(xor_dat.stat().st_size, NUM_XOR_BYTES)

        self.log.info("Files should have been rewritten")
        assert_not_equal(before_blk, blk0.read_bytes(), error_message="blk00000.dat content did not change")
        assert_not_equal(before_rev, rev0.read_bytes(), error_message="rev00000.dat content did not change")

        self.log.info("Start again without the flag; node should log the active obfuscation key matching xor.dat")
        with node.assert_debug_log(expected_msgs=[
            "Using obfuscation key for blocksdir *.dat files",
            f"'{node.read_xor_key().hex()}'",
        ]):
            self.start_node(0, extra_args=self.extra_args[0])
            self.generate(node, 1)
            self.stop_node(0)

        self.log.info("Simulate interrupted rename stage; ensure resume without flag")
        xor_new = blocks_dir / "xor.dat.reobfuscated"
        dummy_reobfuscated = blocks_dir / "blk99999.dat.reobfuscated"
        dummy_reobfuscated.write_bytes(b"\x00")
        xor_new.write_bytes(node.read_xor_key())
        xor_dat.unlink()

        self.restart_node(0, extra_args=self.extra_args[0])
        self.stop_node(0)

        assert (blocks_dir / "blk99999.dat").exists(), "resume did not rename staged block file"
        assert not dummy_reobfuscated.exists(), "resume did not remove reobfuscated suffix"
        assert xor_dat.exists(), "resume did not restore xor.dat"
        assert not xor_new.exists(), "resume did not remove xor.dat.reobfuscated"

        self.log.info("Invalid key values should fail early")
        for invalid_key in ("z" * (2 * NUM_XOR_BYTES), "f" * (2 * NUM_XOR_BYTES - 1)):
            node.assert_start_raises_init_error(
                extra_args=self.extra_args[0] + [f"-reobfuscate-blocks={invalid_key}"],
                expected_msg=f"Error: Invalid -reobfuscate-blocks value '{invalid_key}'",
            )

        self.log.info("Mismatched staged key should fail")
        mismatch_key = "ff" * NUM_XOR_BYTES
        xor_new.write_bytes(NULL_BLK_XOR_KEY)
        expected_msg = "Error: Block obfuscation failed"
        node.assert_start_raises_init_error(extra_args=self.extra_args[0] + [f"-reobfuscate-blocks={mismatch_key}"], expected_msg=expected_msg)
        xor_new.unlink()

        self.log.info("Reobfuscation should fail early when block XOR is disabled")
        before_blk = blk0.read_bytes()
        before_xor = node.read_xor_key()
        requested_key = util_xor(before_xor, b"\xff", offset=0).hex()
        expected_msg = "Error: Block reobfuscation cannot proceed with -blocksxor=0"
        node.assert_start_raises_init_error(extra_args=self.extra_args[0] + ["-blocksxor=0", f"-reobfuscate-blocks={requested_key}"], expected_msg=expected_msg)
        assert_equal(blk0.read_bytes(), before_blk)
        assert_equal(node.read_xor_key(), before_xor)

        self.log.info("Do not suggest reobfuscation when block XOR is disabled")
        self.start_node(0, extra_args=self.extra_args[0] + [f"-reobfuscate-blocks={NULL_BLK_XOR_KEY.hex()}"])
        self.stop_node(0)
        assert_equal(node.read_xor_key(), NULL_BLK_XOR_KEY)
        with node.assert_debug_log(expected_msgs=[], unexpected_msgs=["To obfuscate existing files"]):
            self.start_node(0, extra_args=self.extra_args[0] + ["-blocksxor=0"])
            self.stop_node(0)


if __name__ == "__main__":
    ReobfuscateBlocksSmokeTest(__file__).main()
