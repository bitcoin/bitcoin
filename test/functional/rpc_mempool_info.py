#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPCs that retrieve information from the mempool."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet


class RPCMempoolInfoTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [
            ["-txospenderindex", "-whitelist=noban@127.0.0.1"],
            ["-txospenderindex", "-whitelist=noban@127.0.0.1"],
            ["-whitelist=noban@127.0.0.1"],
        ]

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        confirmed_utxo = self.wallet.get_utxo()

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

        def create_tx(**kwargs):
            return self.wallet.send_self_transfer_multi(
                from_node=self.nodes[0],
                **kwargs,
            )

        txA = create_tx(utxos_to_spend=[confirmed_utxo], num_outputs=2)
        txB = create_tx(utxos_to_spend=[txA["new_utxos"][0]], num_outputs=2)
        txC = create_tx(utxos_to_spend=[txA["new_utxos"][1]], num_outputs=2)
        txD = create_tx(utxos_to_spend=[txB["new_utxos"][0]], num_outputs=1)
        txE = create_tx(utxos_to_spend=[txB["new_utxos"][1]], num_outputs=1)
        txF = create_tx(utxos_to_spend=[txC["new_utxos"][0]], num_outputs=2)
        txG = create_tx(utxos_to_spend=[txC["new_utxos"][1]], num_outputs=1)
        txH = create_tx(utxos_to_spend=[txE["new_utxos"][0],txF["new_utxos"][0]], num_outputs=1)
        txidA, txidB, txidC, txidD, txidE, txidF, txidG, txidH = [
            tx["txid"] for tx in [txA, txB, txC, txD, txE, txF, txG, txH]
        ]

        mempool = self.nodes[0].getrawmempool()
        assert_equal(len(mempool), 8)
        for txid in [txidA, txidB, txidC, txidD, txidE, txidF, txidG, txidH]:
            assert_equal(txid in mempool, True)

        self.log.info("Find transactions spending outputs")
        # wait until spending transactions are found in the mempool of node 0, 1 and 2
        result = self.nodes[0].gettxspendingprevout([ {'txid' : confirmed_utxo['txid'], 'vout' : 0}, {'txid' : txidA, 'vout' : 1} ])
        assert_equal(result, [ {'txid' : confirmed_utxo['txid'], 'vout' : 0, 'spendingtxid' : txidA}, {'txid' : txidA, 'vout' : 1, 'spendingtxid' : txidC} ])
        self.wait_until(lambda: self.nodes[1].gettxspendingprevout([ {'txid' : confirmed_utxo['txid'], 'vout' : 0}, {'txid' : txidA, 'vout' : 1} ]) == result)
        self.wait_until(lambda: self.nodes[2].gettxspendingprevout([ {'txid' : confirmed_utxo['txid'], 'vout' : 0}, {'txid' : txidA, 'vout' : 1} ]) == result)

        self.log.info("Find transaction spending multiple outputs")
        result = self.nodes[0].gettxspendingprevout([ {'txid' : txidE, 'vout' : 0}, {'txid' : txidF, 'vout' : 0} ])
        assert_equal(result, [ {'txid' : txidE, 'vout' : 0, 'spendingtxid' : txidH}, {'txid' : txidF, 'vout' : 0, 'spendingtxid' : txidH} ])

        self.log.info("Find no transaction when output is unspent")
        result = self.nodes[0].gettxspendingprevout([ {'txid' : txidH, 'vout' : 0} ])
        assert_equal(result, [ {'txid' : txidH, 'vout' : 0} ])
        result = self.nodes[0].gettxspendingprevout([ {'txid' : txidA, 'vout' : 5} ])
        assert_equal(result, [ {'txid' : txidA, 'vout' : 5} ])
        result = self.nodes[1].gettxspendingprevout([ {'txid' : txidA, 'vout' : 5} ])
        assert_equal(result, [ {'txid' : txidA, 'vout' : 5} ])
        # on node 2 you also get a warning as txospenderindex is not activated
        result = self.nodes[2].gettxspendingprevout([ {'txid' : txidA, 'vout' : 5} ])
        assert_equal(result, [ {'txid' : txidA, 'vout' : 5, 'warnings': ['txospenderindex is unavailable.']} ])

        self.log.info("Mixed spent and unspent outputs")
        result = self.nodes[0].gettxspendingprevout([ {'txid' : txidB, 'vout' : 0}, {'txid' : txidG, 'vout' : 3} ])
        assert_equal(result, [ {'txid' : txidB, 'vout' : 0, 'spendingtxid' : txidD}, {'txid' : txidG, 'vout' : 3} ])

        self.log.info("Unknown input fields")
        assert_raises_rpc_error(-3, "Unexpected key unknown", self.nodes[0].gettxspendingprevout, [{'txid' : txidC, 'vout' : 1, 'unknown' : 42}])

        self.log.info("Invalid vout provided")
        assert_raises_rpc_error(-8, "Invalid parameter, vout cannot be negative", self.nodes[0].gettxspendingprevout, [{'txid' : txidA, 'vout' : -1}])

        self.log.info("Invalid txid provided")
        assert_raises_rpc_error(-3, "JSON value of type number for field txid is not of expected type string", self.nodes[0].gettxspendingprevout, [{'txid' : 42, 'vout' : 0}])

        self.log.info("Missing outputs")
        assert_raises_rpc_error(-8, "Invalid parameter, outputs are missing", self.nodes[0].gettxspendingprevout, [])

        self.log.info("Missing vout")
        assert_raises_rpc_error(-3, "Missing vout", self.nodes[0].gettxspendingprevout, [{'txid' : txidA}])

        self.log.info("Missing txid")
        assert_raises_rpc_error(-3, "Missing txid", self.nodes[0].gettxspendingprevout, [{'vout' : 3}])

        self.generate(self.wallet, 1)
        # spending transactions are found in the index of nodes 0 and 1 but not node 2
        result = self.nodes[0].gettxspendingprevout([ {'txid' : confirmed_utxo['txid'], 'vout' : 0}, {'txid' : txidA, 'vout' : 1} ])
        assert_equal(result, [ {'txid' : confirmed_utxo['txid'], 'vout' : 0, 'spendingtxid' : txidA}, {'txid' : txidA, 'vout' : 1, 'spendingtxid' : txidC} ])
        result = self.nodes[1].gettxspendingprevout([ {'txid' : confirmed_utxo['txid'], 'vout' : 0}, {'txid' : txidA, 'vout' : 1} ])
        assert_equal(result, [ {'txid' : confirmed_utxo['txid'], 'vout' : 0, 'spendingtxid' : txidA}, {'txid' : txidA, 'vout' : 1, 'spendingtxid' : txidC} ])
        result = self.nodes[2].gettxspendingprevout([ {'txid' : confirmed_utxo['txid'], 'vout' : 0}, {'txid' : txidA, 'vout' : 1} ])
        assert_equal(result, [ {'txid' : confirmed_utxo['txid'], 'vout' : 0, 'warnings': ['txospenderindex is unavailable.']}, {'txid' : txidA, 'vout' : 1, 'warnings': ['txospenderindex is unavailable.']} ])


        self.log.info("Check that our txospenderindex is updated when a reorg replaces a spending transaction")
        confirmed_utxo = self.wallet.get_utxo(mark_as_spent = False)
        tx1 = create_tx(utxos_to_spend=[confirmed_utxo], num_outputs=1)
        self.generate(self.wallet, 1)
        # tx1 is confirmed, and indexed in txospenderindex as spending our utxo
        assert not tx1["txid"] in self.nodes[0].getrawmempool()
        result = self.nodes[0].gettxspendingprevout([ {'txid' : confirmed_utxo['txid'], 'vout' : 0} ])
        assert_equal(result, [ {'txid' : confirmed_utxo['txid'], 'vout' : 0, 'spendingtxid' : tx1["txid"]} ])
        # replace tx1 with tx2
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
        self.nodes[1].invalidateblock(self.nodes[1].getbestblockhash())
        self.nodes[2].invalidateblock(self.nodes[2].getbestblockhash())
        assert tx1["txid"] in self.nodes[0].getrawmempool()
        assert tx1["txid"] in self.nodes[1].getrawmempool()
        tx2 = create_tx(utxos_to_spend=[confirmed_utxo], num_outputs=2)
        assert tx2["txid"] in self.nodes[0].getrawmempool()

        # check that when we find tx2 when we look in the mempool for a tx spending our output
        result = self.nodes[0].gettxspendingprevout([ {'txid' : confirmed_utxo['txid'], 'vout' : 0} ])
        assert_equal(result, [ {'txid' : confirmed_utxo['txid'], 'vout' : 0, 'spendingtxid' : tx2["txid"]} ])

        # check that our txospenderindex has been updated
        self.generate(self.wallet, 1)
        result = self.nodes[0].gettxspendingprevout([ {'txid' : confirmed_utxo['txid'], 'vout' : 0} ])
        assert_equal(result, [ {'txid' : confirmed_utxo['txid'], 'vout' : 0, 'spendingtxid' : tx2["txid"]} ])

        self.log.info("Check that our txospenderindex is updated when a reorg cancels a spending transaction")
        confirmed_utxo = self.wallet.get_utxo(mark_as_spent = False)
        tx1 = create_tx(utxos_to_spend=[confirmed_utxo], num_outputs=1)
        tx2 = create_tx(utxos_to_spend=[tx1["new_utxos"][0]], num_outputs=1)
        # tx1 spends our utxo, tx2 spends tx1
        self.generate(self.wallet, 1)
        # tx1 and tx2 are confirmed, and indexed in txospenderindex
        result = self.nodes[0].gettxspendingprevout([ {'txid' : confirmed_utxo['txid'], 'vout' : 0} ])
        assert_equal(result, [ {'txid' : confirmed_utxo['txid'], 'vout' : 0, 'spendingtxid' : tx1["txid"]} ])
        result = self.nodes[0].gettxspendingprevout([ {'txid' : tx1['txid'], 'vout' : 0} ])
        assert_equal(result, [ {'txid' : tx1['txid'], 'vout' : 0, 'spendingtxid' : tx2["txid"]} ])
        # replace tx1 with tx3
        blockhash= self.nodes[0].getbestblockhash()
        self.nodes[0].invalidateblock(blockhash)
        self.nodes[1].invalidateblock(blockhash)
        self.nodes[2].invalidateblock(blockhash)
        tx3 = create_tx(utxos_to_spend=[confirmed_utxo], num_outputs=2, fee_per_output=2000)
        assert tx3["txid"] in self.nodes[0].getrawmempool()
        assert not tx1["txid"] in self.nodes[0].getrawmempool()
        assert not tx2["txid"] in self.nodes[0].getrawmempool()
        # tx2 is not in the mempool anymore, but still in txospender index which has not been rewound yet
        result = self.nodes[0].gettxspendingprevout([ {'txid' : tx1['txid'], 'vout' : 0} ])
        assert_equal(result, [ {'txid' : tx1['txid'], 'vout' : 0, 'spendingtxid' : tx2["txid"]} ])
        txinfo = self.nodes[0].getrawtransaction(tx2["txid"], verbose = True, blockhash = blockhash)
        assert_equal(txinfo["confirmations"], 0)
        assert_equal(txinfo["in_active_chain"], False)

        self.generate(self.wallet, 1)
        # we check that the spending tx for tx1 is now tx3
        result = self.nodes[0].gettxspendingprevout([ {'txid' : confirmed_utxo['txid'], 'vout' : 0} ])
        assert_equal(result, [ {'txid' : confirmed_utxo['txid'], 'vout' : 0, 'spendingtxid' : tx3["txid"]} ])
        # we check that there is no more spending tx for tx1
        result = self.nodes[0].gettxspendingprevout([ {'txid' : tx1['txid'], 'vout' : 0} ])
        assert_equal(result, [ {'txid' : tx1['txid'], 'vout' : 0} ])


if __name__ == '__main__':
    RPCMempoolInfoTest().main()
