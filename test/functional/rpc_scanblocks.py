#!/usr/bin/env python3
# Copyright (c) 2021-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the scanblocks RPC call."""
from test_framework.address import address_to_scriptpubkey
from test_framework.blockfilter import (
    bip158_basic_element_hash,
    bip158_relevant_scriptpubkeys,
)
from test_framework.messages import COIN
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import (
    MiniWallet,
    getnewdestination,
)

START_HEIGHT = 199
# Hardcoded in regtest chainparams
SNAPSHOT_BASE_HEIGHT = 299


class ScanblocksTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [["-blockfilterindex=1"], [], ["-blockfilterindex=1"]]

    def setup_network(self):
        """Keep node2 disconnected, so that it cannot download the blocks above
        the cached chain before it has loaded a snapshot."""
        self.add_nodes(self.num_nodes, self.extra_args)
        self.start_nodes()
        self.connect_nodes(0, 1)

    def test_unindexed_range(self):
        self.log.info("Test scanning a range that the filter index has not reached yet")
        miner, node = self.nodes[0], self.nodes[2]

        # Mine the chain of the regtest snapshot and give node2 only the headers
        miner.setmocktime(miner.getblockheader(miner.getbestblockhash())["time"])
        wallet = MiniWallet(miner)
        for i in range(SNAPSHOT_BASE_HEIGHT - START_HEIGHT):
            if i % 3 == 0:
                wallet.send_self_transfer(from_node=miner)
            block_hash = self.generate(miner, nblocks=1, sync_fun=self.no_op)[0]
            node.submitheader(miner.getblock(block_hash, 0))
        miner.setmocktime(0)
        snapshot = miner.dumptxoutset("utxos.dat", "latest")

        # The index follows the fully validated chainstate, which cannot get
        # past START_HEIGHT while node2 has no peer to download blocks from
        node.loadtxoutset(snapshot["path"])
        assert_equal(node.getindexinfo()["basic block filter index"], {"synced": True, "best_block_height": START_HEIGHT})

        _, _, addr = getnewdestination()
        assert_raises_rpc_error(-1, "Block filters are still in the process of being indexed.", node.scanblocks, "start", [f"addr({addr})"])
        out = node.scanblocks("start", [f"addr({addr})"], stop_height=START_HEIGHT)
        assert_equal(out["to_height"], START_HEIGHT)
        assert_equal(out["completed"], True)

        # Let node2 download the missing blocks and index them
        self.connect_nodes(0, 2)
        self.sync_all()

    def test_unreadable_range(self):
        self.log.info("Test scanning a range that the synced filter index cannot read")
        node = self.nodes[0]
        self.stop_node(0)
        # The index database still points into the deleted filter file
        (node.chain_path / "indexes" / "blockfilter" / "basic" / "fltr00000.dat").unlink()
        self.start_node(0)

        _, _, addr = getnewdestination()
        assert_raises_rpc_error(-32603, "Failed to read block filter range. This error is unexpected and indicates index corruption.", node.scanblocks, "start", [f"addr({addr})"])

    def run_test(self):
        # Needs the cached chain, so it runs before any other block is mined
        self.test_unindexed_range()

        node = self.nodes[0]
        wallet = MiniWallet(node)

        # send 1.0, mempool only
        _, spk_1, addr_1 = getnewdestination()
        wallet.send_to(from_node=node, scriptPubKey=spk_1, amount=1 * COIN)

        parent_key = "tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B"
        # send 1.0, mempool only
        # childkey 5 of `parent_key`
        wallet.send_to(from_node=node,
                       scriptPubKey=address_to_scriptpubkey("mkS4HXoTYWRTescLGaUTGbtTTYX5EjJyEE"),
                       amount=1 * COIN)

        # mine a block and assure that the mined blockhash is in the filterresult
        blockhash = self.generate(node, 1)[0]
        height = node.getblockheader(blockhash)['height']

        out = node.scanblocks("start", [f"addr({addr_1})"])
        assert blockhash in out['relevant_blocks']
        assert_equal(height, out['to_height'])
        assert_equal(0, out['from_height'])
        assert_equal(True, out['completed'])

        # mine another block
        blockhash_new = self.generate(node, 1)[0]
        height_new = node.getblockheader(blockhash_new)['height']

        # make sure the blockhash is not in the filter result if we set the start_height
        # to the just mined block (unlikely to hit a false positive)
        assert blockhash not in node.scanblocks(
            "start", [f"addr({addr_1})"], height_new)['relevant_blocks']

        # make sure the blockhash is present when using the first mined block as start_height
        assert blockhash in node.scanblocks(
            "start", [f"addr({addr_1})"], height)['relevant_blocks']
        for v in [False, True]:
            assert blockhash in node.scanblocks(
                action="start",
                scanobjects=[f"addr({addr_1})"],
                start_height=height,
                options={"filter_false_positives": v})['relevant_blocks']

        # also test the stop height
        assert blockhash in node.scanblocks(
            "start", [f"addr({addr_1})"], height, height)['relevant_blocks']

        # use the stop_height to exclude the relevant block
        assert blockhash not in node.scanblocks(
            "start", [f"addr({addr_1})"], 0, height - 1)['relevant_blocks']

        # make sure the blockhash is present when using the first mined block as start_height
        assert blockhash in node.scanblocks(
            "start", [{"desc": f"pkh({parent_key}/*)", "range": [0, 100]}], height)['relevant_blocks']

        # check that false-positives are included in the result now; note that
        # finding a false-positive at runtime would take too long, hence we simply
        # use a pre-calculated one that collides with the regtest genesis block's
        # coinbase output and verify that their BIP158 ranged hashes match
        genesis_blockhash = node.getblockhash(0)
        genesis_spks = bip158_relevant_scriptpubkeys(node, genesis_blockhash)
        assert_equal(len(genesis_spks), 1)
        genesis_coinbase_spk = list(genesis_spks)[0]
        false_positive_spk = bytes.fromhex("001400000000000000000000000000000000000cadcb")

        genesis_coinbase_hash = bip158_basic_element_hash(genesis_coinbase_spk, 1, genesis_blockhash)
        false_positive_hash = bip158_basic_element_hash(false_positive_spk, 1, genesis_blockhash)
        assert_equal(genesis_coinbase_hash, false_positive_hash)

        assert genesis_blockhash in node.scanblocks(
            "start", [{"desc": f"raw({genesis_coinbase_spk.hex()})"}], 0, 0)['relevant_blocks']
        assert genesis_blockhash in node.scanblocks(
            "start", [{"desc": f"raw({false_positive_spk.hex()})"}], 0, 0)['relevant_blocks']

        # check that the filter_false_positives option works
        assert genesis_blockhash in node.scanblocks(
            "start", [{"desc": f"raw({genesis_coinbase_spk.hex()})"}], 0, 0, "basic", {"filter_false_positives": True})['relevant_blocks']
        assert genesis_blockhash not in node.scanblocks(
            "start", [{"desc": f"raw({false_positive_spk.hex()})"}], 0, 0, "basic", {"filter_false_positives": True})['relevant_blocks']

        # test node with disabled blockfilterindex
        assert_raises_rpc_error(-1, "Index is not enabled for filtertype basic",
                                self.nodes[1].scanblocks, "start", [f"addr({addr_1})"])

        # test unknown filtertype
        assert_raises_rpc_error(-5, "Unknown filtertype",
                                node.scanblocks, "start", [f"addr({addr_1})"], 0, 10, "extended")

        # test invalid start_height
        assert_raises_rpc_error(-1, "Invalid start_height",
                                node.scanblocks, "start", [f"addr({addr_1})"], 100000000)

        # test invalid stop_height
        assert_raises_rpc_error(-1, "Invalid stop_height",
                                node.scanblocks, "start", [f"addr({addr_1})"], 10, 0)
        assert_raises_rpc_error(-1, "Invalid stop_height",
                                node.scanblocks, "start", [f"addr({addr_1})"], 10, 100000000)

        # test accessing the status (must be empty)
        assert_equal(node.scanblocks("status"), None)

        # test aborting the current scan (there is no, must return false)
        assert_equal(node.scanblocks("abort"), False)

        # test invalid command
        assert_raises_rpc_error(-8, "Invalid action 'foobar'", node.scanblocks, "foobar")

        # test that null scanobjects is rejected for start
        assert_raises_rpc_error(-1, "scanobjects argument is required for the start action", node.scanblocks, "start", None)

        self.test_unreadable_range()


if __name__ == '__main__':
    ScanblocksTest(__file__).main()
