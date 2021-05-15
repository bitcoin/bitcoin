#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.messages import CTransaction, FromHex, hash256, ser_compact_size, ser_string
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_raises_rpc_error, bytes_to_hex_str, satoshi_round, wait_until

'''
rpc_verifyislock.py

Test verifyislock rpc

'''

class RPCVerifyISLockTest(DashTestFramework):
    def set_test_params(self):
        # -whitelist is needed to avoid the trickling logic on node0
        self.set_dash_test_params(6, 5, [["-whitelist=127.0.0.1"], [], [], [], [], []], fast_dip3_enforcement=True)
        self.set_dash_llmq_test_params(5, 3)

    def get_request_id(self, tx_hex):
        tx = FromHex(CTransaction(), tx_hex)

        request_id_buf = ser_string(b"islock") + ser_compact_size(len(tx.vin))
        for txin in tx.vin:
            request_id_buf += txin.prevout.serialize()
        return hash256(request_id_buf)[::-1].hex()

    def run_test(self):

        node = self.nodes[0]
        node.spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        self.mine_quorum()

        txid = node.sendtoaddress(node.getnewaddress(), 1)
        self.wait_for_instantlock(txid, node)

        request_id = self.get_request_id(self.nodes[0].getrawtransaction(txid))
        wait_until(lambda: node.quorum("hasrecsig", 100, request_id, txid))

        rec_sig = node.quorum("getrecsig", 100, request_id, txid)['sig']
        assert(node.verifyislock(request_id, txid, rec_sig))
        # Not mined, should use maxHeight
        assert not node.verifyislock(request_id, txid, rec_sig, 1)
        node.generate(1)
        assert(txid not in node.getrawmempool())
        # Mined but at higher height, should use maxHeight
        assert not node.verifyislock(request_id, txid, rec_sig, 1)
        # Mined, should ignore higher maxHeight
        assert(node.verifyislock(request_id, txid, rec_sig, node.getblockcount() + 100))

        # Mine one more quorum to have a full active set
        self.mine_quorum()
        # Create an ISLOCK for the oldest quorum i.e. the active quorum which will be moved
        # out of the active set when a new quorum appears
        selected_hash = None
        request_id = None
        oldest_quorum_hash = node.quorum("list")["llmq_test"][-1]
        utxos = node.listunspent()
        fee = 0.001
        amount = 1
        # Try all available utxo's until we have one resulting in a request id which selects the
        # last active quorum
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
            selected_hash = node.quorum('selectquorum', 100, request_id)["quorumHash"]
            if selected_hash == oldest_quorum_hash:
                break
        assert selected_hash == oldest_quorum_hash
        # Create the ISLOCK, then mine a quorum to move the signing quorum out of the active set
        islock = self.create_islock(rawtx)
        # Mine one block to trigger the "signHeight + dkgInterval" verification for the ISLOCK
        self.mine_quorum()
        # Verify the ISLOCK for a transaction that is not yet known by the node
        rawtx_txid = node.decoderawtransaction(rawtx)["txid"]
        assert_raises_rpc_error(-5, "No such mempool or blockchain transaction", node.getrawtransaction, rawtx_txid)
        assert node.verifyislock(request_id, rawtx_txid, bytes_to_hex_str(islock.sig), node.getblockcount())
        # Send the tx and verify the ISLOCK for a now known transaction
        assert rawtx_txid == node.sendrawtransaction(rawtx)
        assert node.verifyislock(request_id, rawtx_txid, bytes_to_hex_str(islock.sig), node.getblockcount())


if __name__ == '__main__':
    RPCVerifyISLockTest().main()
