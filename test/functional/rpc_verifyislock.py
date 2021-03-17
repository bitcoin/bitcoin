#!/usr/bin/env python3
# Copyright (c) 2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.messages import CTransaction, FromHex, hash256, ser_compact_size, ser_string
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_raises_rpc_error, wait_until

'''
rpc_verifyislock.py

Test verifyislock rpc

'''

class RPCVerifyISLockTest(DashTestFramework):
    def set_test_params(self):
        # -whitelist is needed to avoid the trickling logic on node0
        self.set_dash_test_params(6, 5, [["-whitelist=127.0.0.1"], [], [], [], [], []], fast_dip3_enforcement=True)
        self.set_dash_llmq_test_params(5, 3)

    def run_test(self):

        node = self.nodes[0]
        node.spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        self.mine_quorum()

        txid = node.sendtoaddress(node.getnewaddress(), 1)
        self.wait_for_instantlock(txid, node)

        tx = FromHex(CTransaction(), node.getrawtransaction(txid))

        request_id_buf = ser_string(b"islock") + ser_compact_size(len(tx.vin))
        for txin in tx.vin:
            request_id_buf += txin.prevout.serialize()
        request_id = hash256(request_id_buf)[::-1].hex()

        wait_until(lambda: node.quorum("hasrecsig", 100, request_id, txid))

        rec_sig = node.quorum("getrecsig", 100, request_id, txid)['sig']
        assert(node.verifyislock(request_id, txid, rec_sig))
        # Not mined, should use maxHeight
        assert_raises_rpc_error(-8, "quorum not found", node.verifyislock, request_id, txid, rec_sig, 1)
        node.generate(1)
        assert(txid not in node.getrawmempool())
        # Mined but at higher height, should use maxHeight
        assert_raises_rpc_error(-8, "quorum not found", node.verifyislock, request_id, txid, rec_sig, 1)
        # Mined, should ignore higher maxHeight
        assert(node.verifyislock(request_id, txid, rec_sig, node.getblockcount() + 100))


if __name__ == '__main__':
    RPCVerifyISLockTest().main()
