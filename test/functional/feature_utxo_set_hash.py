#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test UTXO set hash value calculation in gettxoutsetinfo."""

import struct

from test_framework.blocktools import create_transaction
from test_framework.messages import (
    CBlock,
    COutPoint,
    FromHex,
)
from test_framework.muhash import MuHash3072
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class UTXOSetHashTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_deterministic_hash_results(self):
        self.log.info("Test deterministic UTXO set hash results")

        # These depend on the setup_clean_chain option, the chain loaded from the cache
        assert_equal(self.nodes[0].gettxoutsetinfo()['hash_serialized_2'], "b61ee2cb582d2f4f94493f3d480e9a59d064706e98a12be0f335a3eeadd5678a")
        assert_equal(self.nodes[0].gettxoutsetinfo("muhash")['muhash'], "dd5ad2a105c2d29495f577245c357409002329b9f4d6182c0af3dc2f462555c8")

    def test_muhash_implementation(self):
        self.log.info("Test MuHash implementation consistency")

        node = self.nodes[0]

        # Generate 100 blocks and remove the first since we plan to spend its
        # coinbase
        block_hashes = node.generate(100)
        blocks = list(map(lambda block: FromHex(CBlock(), node.getblock(block, False)), block_hashes))
        spending = blocks.pop(0)

        # Create a spending transaction and mine a block which includes it
        tx = create_transaction(node, spending.vtx[0].rehash(), node.getnewaddress(), amount=49)
        txid = node.sendrawtransaction(hexstring=tx.serialize().hex(), maxfeerate=0)

        tx_block = node.generateblock(node.getnewaddress(), [txid])['hash']
        blocks.append(FromHex(CBlock(), node.getblock(tx_block, False)))

        # Serialize the outputs that should be in the UTXO set and add them to
        # a MuHash object
        muhash = MuHash3072()

        for height, block in enumerate(blocks):
            # The Genesis block coinbase is not part of the UTXO set and we
            # spent the first mined block
            height += 2

            for tx in block.vtx:
                for n, tx_out in enumerate(tx.vout):
                    coinbase = 1 if not tx.vin[0].prevout.hash else 0

                    # Skip witness commitment
                    if (coinbase and n > 0):
                        continue

                    data = COutPoint(int(tx.rehash(), 16), n).serialize()
                    data += struct.pack("<i", height * 2 + coinbase)
                    data += tx_out.serialize()

                    muhash.insert(data)

        finalized = muhash.digest()
        node_muhash = node.gettxoutsetinfo("muhash")['muhash']

        assert_equal(finalized[::-1].hex(), node_muhash)

    def run_test(self):
        self.test_deterministic_hash_results()
        self.test_muhash_implementation()


if __name__ == '__main__':
    UTXOSetHashTest().main()
