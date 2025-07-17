#!/usr/bin/env python3
# Copyright (c) 2020-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.messages import CTransaction, from_hex, hash256, ser_compact_size, ser_string
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error, satoshi_round

'''
rpc_verifyislock.py

Test verifyislock rpc

'''

class RPCVerifyISLockTest(DashTestFramework):
    def set_test_params(self):
        # -whitelist is needed to avoid the trickling logic on node0
        self.set_dash_test_params(6, 5, [["-whitelist=127.0.0.1"], [], [], [], [], []])

    def get_request_id(self, tx_hex):
        tx = from_hex(CTransaction(), tx_hex)

        request_id_buf = ser_string(b"islock") + ser_compact_size(len(tx.vin))
        for txin in tx.vin:
            request_id_buf += txin.prevout.serialize()
        return hash256(request_id_buf)[::-1].hex()

    def run_test(self):

        node = self.nodes[0]
        node.sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        self.mine_cycle_quorum()
        self.bump_mocktime(1)
        self.generate(self.nodes[0], 8, sync_fun=self.sync_blocks())

        txid = node.sendtoaddress(node.getnewaddress(), 1)
        self.bump_mocktime(30)
        self.wait_for_instantlock(txid, node)

        request_id = self.get_request_id(self.nodes[0].getrawtransaction(txid))
        request_id_rpc = self.nodes[0].getislocks([txid])[0]["id"]
        assert_equal(request_id, request_id_rpc)
        self.wait_until(lambda: node.quorum("hasrecsig", 103, request_id, txid))

        rec_sig = node.quorum("getrecsig", 103, request_id, txid)['sig']
        assert node.verifyislock(request_id, txid, rec_sig)
        # Not mined, should use maxHeight
        assert not node.verifyislock(request_id, txid, rec_sig, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        assert txid not in node.getrawmempool()
        # Mined but at higher height, should use maxHeight
        assert not node.verifyislock(request_id, txid, rec_sig, 1)
        # Mined, should ignore higher maxHeight
        assert node.verifyislock(request_id, txid, rec_sig, node.getblockcount() + 100)

        # Mine one more cycle of rotated quorums
        self.mine_cycle_quorum(is_first=False)
        # Create an ISLOCK using an active quorum which will be replaced when a new cycle happens
        request_id = None
        utxos = node.listunspent()
        fee = 0.001
        amount = 1
        # Try all available utxo's until we have one valid in_amount
        for utxo in utxos:
            in_amount = float(utxo['amount'])
            if in_amount < amount + fee:
                continue
            outputs = dict()
            outputs[node.getnewaddress()] = satoshi_round(amount)
            change = in_amount - amount - fee
            if change > 0:
                outputs[node.getnewaddress()] = satoshi_round(change)
            rawtx = node.createrawtransaction([utxo], outputs)
            rawtx = node.signrawtransactionwithwallet(rawtx)["hex"]
            request_id = self.get_request_id(rawtx)
            break
        # Create the ISDLOCK, then mine a cycle quorum to move renew active set
        isdlock = self.create_isdlock(rawtx)
        # Mine one block to trigger the "signHeight + dkgInterval" verification for the ISDLOCK
        self.mine_cycle_quorum(is_first=False)
        # Verify the ISLOCK for a transaction that is not yet known by the node
        rawtx_txid = node.decoderawtransaction(rawtx)["txid"]
        assert_raises_rpc_error(-5, "No such mempool or blockchain transaction", node.getrawtransaction, rawtx_txid)
        assert node.verifyislock(request_id, rawtx_txid, isdlock.sig.hex(), node.getblockcount())
        # Send the tx and verify the ISDLOCK for a now known transaction
        assert rawtx_txid == node.sendrawtransaction(rawtx)
        assert node.verifyislock(request_id, rawtx_txid, isdlock.sig.hex(), node.getblockcount())


if __name__ == '__main__':
    RPCVerifyISLockTest().main()
