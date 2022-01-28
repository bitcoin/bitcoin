#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Litecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Random assortment of utility functions"""

from test_framework.messages import COIN, COutPoint, CTransaction, CTxIn, CTxOut
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error, satoshi_round
from test_framework.script_util import DUMMY_P2WPKH_SCRIPT

"""Create a txout with a given amount and scriptPubKey

Mines coins as needed.

confirmed - txouts created will be confirmed in the blockchain;
            unconfirmed otherwise.
"""
def make_utxo(node, amount, confirmed=True, scriptPubKey=DUMMY_P2WPKH_SCRIPT):
    fee = 1*COIN
    while node.getbalance() < satoshi_round((amount + fee)/COIN):
        node.generate(100)

    new_addr = node.getnewaddress()
    txid = node.sendtoaddress(new_addr, satoshi_round((amount+fee)/COIN))
    tx1 = node.getrawtransaction(txid, 1)
    txid = int(txid, 16)
    i = None

    for i, txout in enumerate(tx1['vout']):
        if txout['scriptPubKey']['addresses'] == [new_addr]:
            break
    assert i is not None

    tx2 = CTransaction()
    tx2.vin = [CTxIn(COutPoint(txid, i))]
    tx2.vout = [CTxOut(amount, scriptPubKey)]
    tx2.rehash()

    signed_tx = node.signrawtransactionwithwallet(tx2.serialize().hex())

    txid = node.sendrawtransaction(signed_tx['hex'], 0)

    # If requested, ensure txouts are confirmed.
    if confirmed:
        mempool_size = len(node.getrawmempool())
        while mempool_size > 0:
            node.generate(1)
            new_size = len(node.getrawmempool())
            # Error out if we have something stuck in the mempool, as this
            # would likely be a bug.
            assert new_size < mempool_size
            mempool_size = new_size

    return COutPoint(int(txid, 16), 0)