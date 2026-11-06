#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test block-file reobfuscation and resuming interrupted migrations."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import NULL_BLK_XOR_KEY, NUM_XOR_BYTES
from test_framework.util import assert_equal, assert_not_equal, util_xor


class ReobfuscationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.uses_wallet = None
        self.extra_args = [["-checkblocks=0"]]

    def run_startup(self, *extra_args):
        self.start_node(0, extra_args=self.extra_args[0] + list(extra_args))
        self.stop_node(0)

    def run_test(self):
        node = self.nodes[0]
        blocks_dir = node.blocks_path
        blk0 = blocks_dir / "blk00000.dat"
        rev0 = blocks_dir / "rev00000.dat"
        xor_dat = node.blocks_key_path

        def staged(path):
            return path.with_name(path.name + ".reobfuscated")

        xor_new = staged(xor_dat)

        assert blk0.exists() and rev0.exists(), "Sanity: blk00000.dat and rev00000.dat should exist"

        self.log.info("Snapshot initial contents")
        before_blk = blk0.read_bytes()
        before_rev = rev0.read_bytes()

        self.log.info("Add many dummy block/undo files for progress logging (10 pairs in total)")
        for i in range(1, 10):
            (blocks_dir / f"blk{i:05d}.dat").write_bytes(b"\0")
            (blocks_dir / f"rev{i:05d}.dat").write_bytes(b"\0")
        self.stop_node(0)

        self.log.info("Restarting with reobfuscation enabled")
        with node.assert_debug_log(expected_msgs=[
            "[obfuscate] Reobfuscating 20 block and undo files",
            "% done",
        ]):
            self.run_startup("-reobfuscate-blocks")

        assert xor_dat.exists(), "xor.dat not created"
        assert_equal(xor_dat.stat().st_size, NUM_XOR_BYTES)

        self.log.info("Files should have been rewritten")
        assert_not_equal(before_blk, blk0.read_bytes(), error_message="blk00000.dat content did not change")
        assert_not_equal(before_rev, rev0.read_bytes(), error_message="rev00000.dat content did not change")

        self.log.info("Rerunning with the already-active key skips the rewrite")
        with node.assert_debug_log(expected_msgs=["[obfuscate] Requested XOR key is already active"], unexpected_msgs=["[obfuscate] Reobfuscating"]):
            self.run_startup(f"-reobfuscate-blocks={node.read_xor_key().hex()}")

        for partial_copy in (False, True):
            self.log.info(f"Resume a staged migration with partial copy: {partial_copy}")
            old_key = node.read_xor_key()
            staged_key = bytes(byte ^ 0xff for byte in old_key)
            xor_new.write_bytes(staged_key)
            if partial_copy:
                delta = util_xor(old_key, staged_key, offset=0)
                staged(blk0).write_bytes(util_xor(blk0.read_bytes(), delta, offset=0))
                blk0.unlink()
                staged(rev0).write_bytes(b"partial copy")
            self.run_startup()
            assert_equal(node.read_xor_key(), staged_key)
            assert not any(blocks_dir.glob("*.reobfuscated"))

        self.log.info("Start again without the flag; node should log the active obfuscation key matching xor.dat")
        with node.assert_debug_log(expected_msgs=[
            "Using obfuscation key for blocksdir *.dat files",
            f"'{node.read_xor_key().hex()}'",
        ]):
            self.start_node(0, extra_args=self.extra_args[0])
            self.generate(node, 1)
            self.stop_node(0)

        self.log.info("Simulate interrupted rename stage; ensure resume without flag")
        dummy_reobfuscated = blocks_dir / "blk99999.dat.reobfuscated"
        dummy_reobfuscated.write_bytes(b"\x00")
        xor_new.write_bytes(node.read_xor_key())
        xor_dat.unlink()
        self.run_startup()

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
        nonzero_key = "ff" * NUM_XOR_BYTES
        xor_new.write_bytes(NULL_BLK_XOR_KEY)
        node.assert_start_raises_init_error(
            extra_args=self.extra_args[0] + [f"-reobfuscate-blocks={nonzero_key}"],
            expected_msg="Error: Block reobfuscation failed: Requested XOR key does not match staged xor.dat.reobfuscated",
        )
        xor_new.unlink()

        self.log.info("Reobfuscation should fail early when block XOR is disabled")
        blk_before_disabled_xor = blk0.read_bytes()
        xor_before_disabled_xor = node.read_xor_key()
        node.assert_start_raises_init_error(
            extra_args=self.extra_args[0] + ["-blocksxor=0", f"-reobfuscate-blocks={nonzero_key}"],
            expected_msg="Error: Block reobfuscation cannot proceed with -blocksxor=0",
        )
        assert_equal(blk0.read_bytes(), blk_before_disabled_xor)
        assert_equal(node.read_xor_key(), xor_before_disabled_xor)

        self.log.info("Do not suggest reobfuscation when block XOR is disabled")
        self.run_startup("-blocksxor=0", f"-reobfuscate-blocks={NULL_BLK_XOR_KEY.hex()}")
        assert_equal(node.read_xor_key(), NULL_BLK_XOR_KEY)
        xor_new.write_bytes(NULL_BLK_XOR_KEY)
        with node.assert_debug_log(expected_msgs=[], unexpected_msgs=["To obfuscate existing files"]):
            self.run_startup("-blocksxor=0")


if __name__ == "__main__":
    ReobfuscationTest(__file__).main()
