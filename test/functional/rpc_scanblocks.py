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
    JSONRPCException,
)
from test_framework.wallet import (
    MiniWallet,
    getnewdestination,
)


class ScanblocksTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-blockfilterindex=1"], []]

    def run_test(self):
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
        self.wait_until(lambda: all(i["synced"] for i in node.getindexinfo().values()))

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

        self.test_scanblocks_unindexed_range()

    def test_scanblocks_unindexed_range(self):
        """scanblocks must not silently skip ranges the filter index has not written yet.

        Like getblockfilter, a scan succeeds when the requested range is already
        available even if the index is still behind the tip, and it errors (instead
        of silently skipping the range and still reporting completed=true) when the
        requested range is not yet available.
        """
        node = self.nodes[0]
        wallet = MiniWallet(node)

        def bfi():
            return node.getindexinfo()["basic block filter index"]

        _, spk, addr = getnewdestination()
        wallet.send_to(from_node=node, scriptPubKey=spk, amount=1 * COIN)
        match_hash = self.generate(node, 1, sync_fun=self.no_op)[0]
        # A large enough chain that rebuilding the wiped index after a restart
        # leaves the node briefly behind the tip. Missing that window is harmless.
        self.generate(node, 3000, sync_fun=self.no_op)
        self.wait_until(lambda: bfi()["synced"])

        self.stop_node(0)
        self.cleanup_folder(node.chain_path / "indexes" / "blockfilter")
        self.start_node(0, extra_args=["-blockfilterindex=1"])
        tip = node.getblockcount()

        if not bfi()["synced"]:
            self.log.info("scanning to the not-yet-indexed tip errors instead of skipping silently")
            try:
                out = node.scanblocks("start", [f"addr({addr})"], 0, tip)
            except JSONRPCException as e:
                assert "still in the process of being indexed" in e.error["message"]
            else:
                # A returning scan must be complete: the match must be present.
                assert_equal(out["completed"], True)
                assert match_hash in out["relevant_blocks"]

            self.log.info("scanning an already-indexed prefix succeeds while still behind the tip")
            # Genesis is indexed first, so it is available while the tip is not.
            while not bfi()["synced"]:
                try:
                    out = node.scanblocks("start", [f"addr({addr})"], 0, 0)
                except JSONRPCException as e:
                    assert "still in the process of being indexed" in e.error["message"]
                    continue
                assert_equal(out["completed"], True)
                assert_equal(out["to_height"], 0)
                break

        self.log.info("once the index is synced the same scan completes and finds the match")
        self.wait_until(lambda: bfi()["synced"])
        out = node.scanblocks("start", [f"addr({addr})"])
        assert_equal(out["completed"], True)
        assert match_hash in out["relevant_blocks"]


if __name__ == '__main__':
    ScanblocksTest(__file__).main()
