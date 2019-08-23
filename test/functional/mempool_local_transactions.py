#!/usr/bin/env python3
# Copyright (c) 2009-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Ensure that transactions submitted locally (via rpc or wallet) are successfully
broadcast to at least one peer.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
        assert_greater_than,
        wait_until,
        create_confirmed_utxos,
        disconnect_nodes
)
from test_framework.mininode import P2PTxInvStore
import time

# Constant from txmempool.h
MAX_REBROADCAST_WEIGHT = 3000000

class MempoolLocalTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [[
            "-whitelist=127.0.0.1",
            "-blockmaxweight=3000000"
            ]] * self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("test that mempool will ensure initial broadcast of locally submitted txns")

        node = self.nodes[0]
        min_relay_fee = node.getnetworkinfo()["relayfee"]
        utxos = create_confirmed_utxos(min_relay_fee, node, 2000)

        self.log.info("populate mempool with transactions")
        node.settxfee(min_relay_fee * 3)

        addresses = []
        for _ in range(50):
            addresses.append(self.nodes[1].getnewaddress())

        # create large txns by sending to all the addresses
        outputs = {}
        for addr in addresses:
            outputs[addr] = 0.0001

        self.sync_blocks()

        # create lots of txns with that large output
        for _ in range(len(utxos) - 2):
            utxo = utxos.pop()
            inputs = [{'txid': utxo['txid'], 'vout': utxo['vout']}]
            raw_tx_hex = node.createrawtransaction(inputs, outputs)
            signed_tx = node.signrawtransactionwithwallet(raw_tx_hex)
            node.sendrawtransaction(hexstring=signed_tx['hex'], maxfeerate=0)
            self.nodes[1].sendrawtransaction(hexstring=signed_tx['hex'], maxfeerate=0)

        self.sync_mempools()

        # confirm txns are more than max rebroadcast amount
        assert_greater_than(node.getmempoolinfo()['bytes'], MAX_REBROADCAST_WEIGHT)

        # node needs any connection to pass check in getblocktemplate
        node.add_p2p_connection(P2PTxInvStore())

        self.log.info("generate txns that won't be marked as broadcast")
        disconnect_nodes(node, 1)

        # generate a txn using sendrawtransaction
        us0 = utxos.pop()
        inputs = [{ "txid" : us0["txid"], "vout" : us0["vout"]}]
        addr = node.getnewaddress()
        outputs = {addr: 0.0001}
        tx = node.createrawtransaction(inputs, outputs)
        node.settxfee(min_relay_fee) # specifically fund this tx with low fee
        txF = node.fundrawtransaction(tx)
        txFS = node.signrawtransactionwithwallet(txF['hex'])
        rpc_tx_hsh = node.sendrawtransaction(txFS['hex'])  # txhsh in hex

        # generate a wallet txn
        utxo = utxos.pop()
        wallet_tx_hsh = node.sendtoaddress(addr, 0.0001)

        # ensure the txns won't be rebroadcast due to top-of-mempool rule
        tx_hshs = []
        tmpl = node.getblocktemplate({'rules': ['segwit']})

        for tx in tmpl['transactions']:
            tx_hshs.append(tx['hash'])

        assert(rpc_tx_hsh not in tx_hshs)
        assert(wallet_tx_hsh not in tx_hshs)

        # trigger rebroadcast
        conn = node.add_p2p_connection(P2PTxInvStore())
        mocktime = int(time.time()) + 300 * 60
        node.setmocktime(mocktime)

        # verify the txn invs were sent due to tracking unbroadcast set
        wallet_tx_id = int(wallet_tx_hsh, 16)
        rpc_tx_id = int(rpc_tx_hsh, 16)

        self.log.info("verify invs were sent due to unbroadcast tracking")
        wait_until(lambda: wallet_tx_id in conn.get_invs())
        wait_until(lambda: rpc_tx_id in conn.get_invs())

if __name__ == '__main__':
    MempoolLocalTransactionsTest().main()

