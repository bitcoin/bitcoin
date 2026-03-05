#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test gettxspendingprevout RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet

#### Query Helpers ####
def prevout(txid, vout):
    """Build a prevout query dict for use with gettxspendingprevout"""
    return {'txid': txid, 'vout': vout}

#### Result Helpers ####

def unspent_out(txid, vout):
    """Expected result for an available output (not spent)"""
    return {'txid': txid, 'vout': vout}

def spent_out(txid, vout, spending_tx_id):
    """Expected result for an output with a known spender"""
    return {'txid': txid, 'vout': vout, 'spendingtxid': spending_tx_id}

def spent_out_in_block(txid, vout, spending_tx_id, blockhash, spending_tx):
    """Expected result for an output spent in a confirmed block, with full tx data"""
    return {'txid': txid, 'vout': vout, 'spendingtxid': spending_tx_id, 'blockhash': blockhash, 'spendingtx': spending_tx}

class GetTxSpendingPrevoutTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.noban_tx_relay = True
        self.extra_args = [
            ["-txospenderindex"],
            ["-txospenderindex"],
            [],
        ]

    def run_test(self):
        node0, node1, node2 = self.nodes
        self.wallet = MiniWallet(node0)
        root_utxo = self.wallet.get_utxo()
        txid_root_utxo = root_utxo['txid']

        def create_tx(**kwargs):
            return self.wallet.send_self_transfer_multi(from_node=node0, **kwargs)

        # Create a tree of unconfirmed transactions in the mempool:
        #             txA
        #             / \
        #            /   \
        #           /     \
        #          /       \
        #         /         \
        #       txB         txC
        #       / \         / \
        #      /   \       /   \
        #    txD   txE   txF   txG
        #            \   /
        #             \ /
        #             txH

        txA = create_tx(utxos_to_spend=[root_utxo], num_outputs=2)
        txB = create_tx(utxos_to_spend=[txA["new_utxos"][0]], num_outputs=2)
        txC = create_tx(utxos_to_spend=[txA["new_utxos"][1]], num_outputs=2)
        txD = create_tx(utxos_to_spend=[txB["new_utxos"][0]], num_outputs=1)
        txE = create_tx(utxos_to_spend=[txB["new_utxos"][1]], num_outputs=1)
        txF = create_tx(utxos_to_spend=[txC["new_utxos"][0]], num_outputs=2)
        txG = create_tx(utxos_to_spend=[txC["new_utxos"][1]], num_outputs=1)
        txH = create_tx(utxos_to_spend=[txE["new_utxos"][0], txF["new_utxos"][0]], num_outputs=1)

        txs = [txA, txB, txC, txD, txE, txF, txG, txH]
        txidA, txidB, txidC, txidD, txidE, txidF, txidG, txidH = [tx["txid"] for tx in txs]

        self.sync_mempools()
        self.wait_until(lambda: node0.getindexinfo()["txospenderindex"]["synced"])
        self.wait_until(lambda: node1.getindexinfo()["txospenderindex"]["synced"])
        mempool = node0.getrawmempool()
        assert_equal(len(mempool), 8)
        for tx in txs:
            assert tx["txid"] in mempool

        self.log.info("Find transactions spending outputs")
        # wait until spending transactions are found in the mempool of node 0, 1 and 2
        result = node0.gettxspendingprevout([prevout(txid_root_utxo, vout=0), prevout(txidA, vout=1)])
        assert_equal(result, [spent_out(txid_root_utxo, vout=0, spending_tx_id=txidA),
                              spent_out(txidA, vout=1, spending_tx_id=txidC)])
        self.wait_until(lambda: node1.gettxspendingprevout([prevout(txid_root_utxo, vout=0), prevout(txidA, vout=1)]) == result)
        self.wait_until(lambda: node2.gettxspendingprevout([prevout(txid_root_utxo, vout=0), prevout(txidA, vout=1)], mempool_only=True) == result)

        self.log.info("Find transaction spending multiple outputs")
        result = node0.gettxspendingprevout([prevout(txidE, vout=0), prevout(txidF, vout=0)])
        assert_equal(result, [spent_out(txidE, vout=0, spending_tx_id=txidH),
                              spent_out(txidF, vout=0, spending_tx_id=txidH)])

        self.log.info("Find no transaction when output is unspent")
        assert_equal(node0.gettxspendingprevout([prevout(txidH, vout=0)]), [unspent_out(txidH, vout=0)])
        for node in self.nodes:
            assert_equal(node.gettxspendingprevout([prevout(txidA, vout=5)]), [unspent_out(txidA, vout=5)])

        self.log.info("Mixed spent and unspent outputs")
        result = node0.gettxspendingprevout([prevout(txidB, vout=0), prevout(txidG, vout=3)])
        assert_equal(result, [spent_out(txidB, vout=0, spending_tx_id=txidD),
                              unspent_out(txidG, vout=3)])

        self.log.info("Unknown input fields")
        assert_raises_rpc_error(-3, "Unexpected key unknown", node0.gettxspendingprevout, [{'txid' : txidC, 'vout' : 1, 'unknown' : 42}])

        self.log.info("Invalid vout provided")
        assert_raises_rpc_error(-8, "Invalid parameter, vout cannot be negative", node0.gettxspendingprevout, [prevout(txidA, vout=-1)])

        self.log.info("Invalid txid provided")
        assert_raises_rpc_error(-3, "JSON value of type number for field txid is not of expected type string", node0.gettxspendingprevout, [prevout(42, vout=0)])

        self.log.info("Missing outputs")
        assert_raises_rpc_error(-8, "Invalid parameter, outputs are missing", node0.gettxspendingprevout, [])

        self.log.info("Missing vout")
        assert_raises_rpc_error(-3, "Missing vout", node0.gettxspendingprevout, [{'txid' : txidA}])

        self.log.info("Missing txid")
        assert_raises_rpc_error(-3, "Missing txid", node0.gettxspendingprevout, [{'vout' : 3}])

        blockhash = self.generate(self.wallet, 1)[0]
        # spending transactions are found in the index of nodes 0 and 1 but not node 2
        for node in [node0, node1]:
            result = node.gettxspendingprevout([prevout(txid_root_utxo, vout=0), prevout(txidA, vout=1)], return_spending_tx=True)
            assert_equal(result, [spent_out_in_block(txid_root_utxo, vout=0, spending_tx_id=txidA, blockhash=blockhash, spending_tx=txA['hex']),
                                  spent_out_in_block(txidA, vout=1, spending_tx_id=txidC, blockhash=blockhash, spending_tx=txC['hex'])])
        # Spending transactions not in node 2 (no spending index)
        result = node2.gettxspendingprevout([prevout(txid_root_utxo, vout=0), prevout(txidA, vout=1)], return_spending_tx=True)
        assert_equal(result, [unspent_out(txid_root_utxo, vout=0), unspent_out(txidA, vout=1)])

        # spending transaction is not found if we only search the mempool
        result = node0.gettxspendingprevout([prevout(txid_root_utxo, vout=0), prevout(txidA, vout=1)], return_spending_tx=True, mempool_only=True)
        assert_equal(result, [unspent_out(txid_root_utxo, vout=0), unspent_out(txidA, vout=1)])

        self.log.info("Check that our txospenderindex is updated when a reorg replaces a spending transaction")
        reorg_replace_utxo = self.wallet.get_utxo(mark_as_spent=False)
        txid_reorg_replace_utxo = reorg_replace_utxo['txid']

        tx1 = create_tx(utxos_to_spend=[reorg_replace_utxo], num_outputs=1)
        blockhash = self.generate(self.wallet, 1)[0]

        # tx1 is confirmed, and indexed in txospenderindex as spending our utxo
        assert tx1["txid"] not in node0.getrawmempool()
        result = node0.gettxspendingprevout([prevout(txid_reorg_replace_utxo, vout=0)], return_spending_tx=True)
        assert_equal(result, [spent_out_in_block(txid_reorg_replace_utxo, vout=0, spending_tx_id=tx1["txid"], blockhash=blockhash, spending_tx=tx1['hex'])])

        # replace tx1 with tx2 triggering a "reorg"
        best_block_hash = node0.getbestblockhash()
        for node in self.nodes:
            node.invalidateblock(best_block_hash)
            assert tx1["txid"] in node.getrawmempool()

        # create and submit replacement
        tx2 = create_tx(utxos_to_spend=[reorg_replace_utxo], num_outputs=2)
        assert tx2["txid"] in node0.getrawmempool()

        # check that when we find tx2 when we look in the mempool for a tx spending our output
        result = node0.gettxspendingprevout([prevout(txid_reorg_replace_utxo, vout=0)], return_spending_tx=True)
        assert_equal(result, [ {'txid' : txid_reorg_replace_utxo, 'vout' : 0, 'spendingtxid' : tx2["txid"], 'spendingtx' : tx2['hex']} ])

        # check that our txospenderindex has been updated
        blockhash = self.generate(self.wallet, 1)[0]
        result = node0.gettxspendingprevout([prevout(txid_reorg_replace_utxo, vout=0)], return_spending_tx=True)
        assert_equal(result, [spent_out_in_block(txid_reorg_replace_utxo, vout=0, spending_tx_id=tx2["txid"], blockhash=blockhash, spending_tx=tx2['hex'])])

        self.log.info("Check that our txospenderindex is updated when a reorg cancels a spending transaction")
        reorg_cancel_utxo = self.wallet.get_utxo(mark_as_spent=False)
        txid_reorg_cancel_utxo = reorg_cancel_utxo['txid']

        tx1 = create_tx(utxos_to_spend=[reorg_cancel_utxo], num_outputs=1)
        tx2 = create_tx(utxos_to_spend=[tx1["new_utxos"][0]], num_outputs=1)
        # tx1 spends our utxo, tx2 spends tx1
        blockhash = self.generate(self.wallet, 1)[0]
        # tx1 and tx2 are confirmed, and indexed in txospenderindex
        result = node0.gettxspendingprevout([prevout(txid_reorg_cancel_utxo, vout=0)], return_spending_tx=True)
        assert_equal(result, [spent_out_in_block(txid_reorg_cancel_utxo, vout=0, spending_tx_id=tx1["txid"], blockhash=blockhash, spending_tx=tx1['hex'])])
        result = node0.gettxspendingprevout([prevout(tx1['txid'], vout=0)], return_spending_tx=True)
        assert_equal(result, [spent_out_in_block(tx1['txid'], vout=0, spending_tx_id=tx2["txid"], blockhash=blockhash, spending_tx=tx2['hex'])])

        # replace tx1 with tx3
        blockhash = node0.getbestblockhash()
        for node in self.nodes:
            node.invalidateblock(blockhash)

        tx3 = create_tx(utxos_to_spend=[reorg_cancel_utxo], num_outputs=2, fee_per_output=2000)
        node0_mempool = node0.getrawmempool()
        assert tx3["txid"] in node0_mempool
        assert tx1["txid"] not in node0_mempool
        assert tx2["txid"] not in node0_mempool

        # tx2 is not in the mempool anymore, but still in txospender index which has not been rewound yet
        result = node0.gettxspendingprevout([prevout(tx1['txid'], vout=0)], return_spending_tx=True)
        assert_equal(result, [spent_out_in_block(tx1['txid'], vout=0, spending_tx_id=tx2["txid"], blockhash=blockhash, spending_tx=tx2['hex'])])

        txinfo = node0.getrawtransaction(tx2["txid"], verbose = True, blockhash = blockhash)
        assert_equal(txinfo["confirmations"], 0)
        assert_equal(txinfo["in_active_chain"], False)

        blockhash = self.generate(self.wallet, 1)[0]
        # we check that the spending tx for tx1 is now tx3
        result = node0.gettxspendingprevout([prevout(txid_reorg_cancel_utxo, vout=0)], return_spending_tx=True)
        assert_equal(result, [spent_out_in_block(txid_reorg_cancel_utxo, vout=0, spending_tx_id=tx3["txid"], blockhash=blockhash, spending_tx=tx3['hex'])])

        # we check that there is no more spending tx for tx1
        result = node0.gettxspendingprevout([prevout(tx1['txid'], vout=0)], return_spending_tx=True)
        assert_equal(result, [unspent_out(tx1['txid'], vout=0)])


if __name__ == '__main__':
    GetTxSpendingPrevoutTest(__file__).main()
