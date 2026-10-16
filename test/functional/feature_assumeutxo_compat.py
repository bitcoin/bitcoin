#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test upgrades from completed and unfinished AssumeUTXO datadirs."""

import shutil

from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import CBlockHeader
from test_framework.script import CScript, OP_0, OP_TRUE
from test_framework.script_util import script_to_p2wsh_script
from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch
from test_framework.util import assert_equal

INDEX_ARGS = ["-txindex", "-blockfilterindex=basic"]


class AssumeutxoCompatibilityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

    def skip_test_if_missing_module(self):
        self.skip_if_no_previous_releases()

    def setup_network(self):
        self.add_nodes(3, versions=[310100, 310100, None])
        self.start_node(0)
        self.start_node(1, extra_args=INDEX_ARGS)

    def copy_chainstate(self):
        source, target = self.nodes[1:]
        if target.chain_path.exists():
            self.cleanup_folder(target.chain_path)
        target.chain_path.mkdir()
        for name in ("blocks", "chainstate", "chainstate_snapshot", "indexes"):
            if (source.chain_path / name).exists():
                shutil.copytree(source.chain_path / name, target.chain_path / name)

    def run_test(self):
        source, snapshot, upgraded = self.nodes
        genesis = source.getblockheader(source.getbestblockhash())
        for node in (source, snapshot):
            node.setmocktime(genesis['time'] + 210)
        previous = int(genesis['hash'], 16)

        self.log.info("Build the v31.1 regtest snapshot chain at height 200")
        for height in range(1, 201):
            coinbase = create_coinbase(height, script_pubkey=script_to_p2wsh_script(CScript([OP_TRUE])))
            # The released snapshot fixture always includes a dummy extraNonce.
            coinbase.vin[0].scriptSig = CScript([height, OP_0])
            block = create_block(previous, coinbase, ntime=genesis['time'] + height)
            block.solve()
            assert_equal(source.submitblock(block.serialize().hex()), None)
            snapshot.submitheader(CBlockHeader(block).serialize().hex())
            previous = block.hash_int
        assert_equal(source.getbestblockhash(), "385901ccbd69dff6bbd00065d01fb8a9e464dede7cfe0372443884f9b1dcf6b9")
        dump = source.dumptxoutset("snapshot.dat", "latest")
        snapshot.loadtxoutset(dump['path'])
        for height in range(1, 101):
            assert_equal(snapshot.submitblock(source.getblock(source.getblockhash(height), 0)), None)
        self.wait_until(lambda: snapshot.getchainstates()['chainstates'][0]['blocks'] == 100)
        for blockhash in self.generate(source, 10, sync_fun=self.no_op):
            assert_equal(snapshot.submitblock(source.getblock(blockhash, 0)), None)
        historical, active = snapshot.getchainstates()['chainstates']
        assert_equal(historical['blocks'], 100)
        assert_equal(active['blocks'], 210)
        self.stop_node(1)

        self.log.info("Reject a real unfinished snapshot and allow discarding it")
        self.copy_chainstate()
        upgraded.assert_start_raises_init_error(expected_msg="An unfinished AssumeUTXO snapshot was found in", match=ErrorMatch.PARTIAL_REGEX)
        self.cleanup_folder(upgraded.chain_path / "chainstate_snapshot")
        for args in ([], ["-reindex-chainstate"]):
            upgraded.assert_start_raises_init_error(
                extra_args=args,
                expected_msg="Error: Block index contains unfinished AssumeUTXO validation. Please restart with -reindex to rebuild the block index.",
            )
        self.start_node(2, extra_args=["-reindex"])
        assert_equal(upgraded.getblockcount(), 100)
        self.connect_nodes(2, 0)
        self.sync_blocks(nodes=(source, upgraded))
        assert upgraded.verifychain(4, 0)
        self.stop_node(2)

        self.log.info("Complete validation and convert the snapshot with v31.1")
        self.start_node(1, extra_args=INDEX_ARGS)
        self.connect_nodes(0, 1)
        self.sync_blocks(nodes=(source, snapshot))
        self.wait_until(lambda: len(snapshot.getchainstates()['chainstates']) == 1)
        self.stop_node(1)

        self.log.info("Require the cleanup restart even after validation completes")
        self.copy_chainstate()
        upgraded.assert_start_raises_init_error(expected_msg="An unfinished AssumeUTXO snapshot was found in", match=ErrorMatch.PARTIAL_REGEX)
        self.log.info("Reject cleanup interrupted between its two renames")
        (upgraded.chain_path / "chainstate").rename(upgraded.chain_path / "chainstate_todelete")
        upgraded.assert_start_raises_init_error(expected_msg="An unfinished AssumeUTXO snapshot was found in", match=ErrorMatch.PARTIAL_REGEX)

        self.start_node(1)
        assert not (snapshot.chain_path / "chainstate_snapshot").exists()
        assert (snapshot.chain_path / "chainstate" / "base_blockhash").exists()
        assert len(list(snapshot.blocks_path.glob("blk*.dat"))) >= 2
        expected = snapshot.gettxoutsetinfo("muhash")
        self.stop_node(1)
        self.copy_chainstate()

        self.log.info("Load and reindex the converted datadir with the current node")
        for args in (INDEX_ARGS, ["-reindex-chainstate", *INDEX_ARGS], ["-reindex", "-prune=550"]):
            self.start_node(2, extra_args=args)
            self.wait_until(lambda: upgraded.getblockcount() == expected['height'])
            if "-txindex" in args:
                self.wait_until(lambda: all(index['synced'] for index in upgraded.getindexinfo().values()))
                assert_equal(len(upgraded.getindexinfo()), 2)
            actual = upgraded.gettxoutsetinfo("muhash")
            assert_equal(actual['bestblock'], expected['bestblock'])
            assert_equal(actual['muhash'], expected['muhash'])
            assert upgraded.verifychain(4, 0)
            self.stop_node(2)


if __name__ == '__main__':
    AssumeutxoCompatibilityTest(__file__).main()
