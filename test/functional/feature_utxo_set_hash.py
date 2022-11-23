#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test UTXO set hash value calculation in gettxoutsetinfo."""
from decimal import Decimal

from test_framework.messages import (
    COIN,
    COutPoint,
    CTxOut,
)
from test_framework.muhash import MuHash3072
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


def txout_serialize(txid_hex: str, output_index: int, height: int,
                    is_coinbase: bool, value: Decimal, scriptPubKey_hex: str):
    """Serialize UTXO for MuHash (see function `TxOutSer` in the coinstats module)."""
    data = COutPoint(int(txid_hex, 16), output_index).serialize()
    data += (height * 2 + is_coinbase).to_bytes(4, 'little')
    data += CTxOut(int(value * COIN), bytes.fromhex(scriptPubKey_hex)).serialize()
    return data


class UTXOSetHashTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def test_muhash_implementation(self):
        self.log.info("Test MuHash implementation consistency")

        node = self.nodes[0]
        wallet = MiniWallet(node)
        mocktime = node.getblockheader(node.getblockhash(0))['time'] + 1
        node.setmocktime(mocktime)

        # Generate 100 blocks, create spending transaction and mine another block
        self.generate(wallet, 1) + self.generate(node, 99)
        txid = wallet.send_self_transfer(from_node=node)['txid']
        self.generateblock(node, output=wallet.get_address(), transactions=[txid])

        # Serialize (created/spent) transaction outputs in each block and
        # apply (add/remove) them to the MuHash object accordingly
        muhash = MuHash3072()

        for height in range(1, node.getblockcount() + 1):
            for tx in node.getblock(blockhash=node.getblockhash(height), verbosity=3)['tx']:
                # add created coins
                is_coinbase_tx = 'coinbase' in tx['vin'][0]
                for o in tx['vout']:
                    if o['scriptPubKey']['type'] != 'nulldata':
                        muhash.insert(txout_serialize(tx['txid'], o['n'], height, is_coinbase_tx,
                                                      o['value'], o['scriptPubKey']['hex']))
                # remove spent coins
                if not is_coinbase_tx:
                    for i in tx['vin']:
                        prev = i['prevout']
                        muhash.remove(txout_serialize(i['txid'], i['vout'], prev['height'], prev['generated'],
                                                      prev['value'], prev['scriptPubKey']['hex']))

        finalized = muhash.digest()
        node_muhash = node.gettxoutsetinfo("muhash")['muhash']

        assert_equal(finalized[::-1].hex(), node_muhash)

        self.log.info("Test deterministic UTXO set hash results")
        assert_equal(node.gettxoutsetinfo()['hash_serialized_2'], "f9aa4fb5ffd10489b9a6994e70ccf1de8a8bfa2d5f201d9857332e9954b0855d")
        assert_equal(node.gettxoutsetinfo("muhash")['muhash'], "d1725b2fe3ef43e55aa4907480aea98d406fc9e0bf8f60169e2305f1fbf5961b")

    def run_test(self):
        self.test_muhash_implementation()


if __name__ == '__main__':
    UTXOSetHashTest().main()
