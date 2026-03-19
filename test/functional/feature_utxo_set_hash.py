#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test UTXO set hash value calculation in gettxoutsetinfo."""

from test_framework.messages import (
    CBlock,
    COutPoint,
    from_hex,
)
from test_framework.crypto.muhash import MuHash3072
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

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

        # Generate 100 blocks and remove the first since we plan to spend its
        # coinbase
        block_hashes = self.generate(wallet, 1) + self.generate(node, 99)
        blocks = list(map(lambda block: from_hex(CBlock(), node.getblock(block, False)), block_hashes))
        blocks.pop(0)

        # Create a spending transaction and mine a block which includes it
        txid = wallet.send_self_transfer(from_node=node)['txid']
        tx_block = self.generateblock(node, output=wallet.get_address(), transactions=[txid])
        blocks.append(from_hex(CBlock(), node.getblock(tx_block['hash'], False)))

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

                    data = COutPoint(tx.txid_int, n).serialize()
                    data += (height * 2 + coinbase).to_bytes(4, "little")
                    data += tx_out.serialize()

                    muhash.insert(data)

        finalized = muhash.digest()
        node_muhash = node.gettxoutsetinfo("muhash")['muhash']

        assert_equal(finalized[::-1].hex(), node_muhash)

        self.log.info("Test deterministic UTXO set hash results")
        assert_equal(node.gettxoutsetinfo()['hash_serialized_3'], "396058cfd7f3b4c05dcdfaf360be9986d859d2c8315082d3aadda6d2d16add25")
        assert_equal(node.gettxoutsetinfo("muhash")['muhash'], "97f341d4226cb4451dd9da2856759fb97659cfc238a0b8d5452e64af6032bd3d")

    def run_test(self):
        self.test_muhash_implementation()


if __name__ == '__main__':
    UTXOSetHashTest(__file__).main()
