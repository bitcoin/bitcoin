#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that a taproot output whose key is not a valid x-only pubkey is unspendable.

The output key does not parse, so no signature over it can ever be valid. Such a
signature never reaches Schnorr batch verification, which makes this a case that
batch validation must reject explicitly rather than let pass. Script verification
is exercised both single-threaded (-par=1) and multi-threaded, because the two
use different code paths.
"""

import random

from test_framework.blocktools import (
    add_witness_commitment,
    create_block,
    create_coinbase,
)
from test_framework.crypto import secp256k1
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
)
from test_framework.script import CScript, OP_1
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


def invalid_output_key():
    """Return 32 bytes that are not the x coordinate of any curve point."""
    while True:
        key = random.randbytes(32)
        if not secp256k1.GE.is_valid_x(int.from_bytes(key, 'big')):
            return key


class TaprootInvalidOutputKeyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        # -par=1 leaves no script check worker threads, so scripts are verified
        # on the main thread instead of through the check queue.
        self.extra_args = [["-par=1"], ["-par=4"]]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        self.generate(wallet, 101)

        self.log.info("Create a taproot output with an unparsable output key")
        spk = CScript([OP_1, invalid_output_key()])
        funding = wallet.send_to(from_node=node, scriptPubKey=spk, amount=10000)
        self.generate(wallet, 1)

        self.log.info("Build a key path spend of that output")
        spend = CTransaction()
        spend.vin = [CTxIn(COutPoint(int(funding["txid"], 16), funding["sent_vout"]))]
        spend.vout = [CTxOut(9000, CScript([OP_1]))]
        spend.wit.vtxinwit = [CTxInWitness()]
        spend.wit.vtxinwit[0].scriptWitness.stack = [bytes(64)]

        self.sync_all()
        # Each node has to validate the block itself rather than learn that it is
        # invalid from its peer.
        self.disconnect_nodes(0, 1)

        tip = node.getbestblockhash()
        block = create_block(
            int(tip, 16),
            create_coinbase(node.getblockcount() + 1),
            ntime=node.getblockheader(tip)["mediantime"] + 1,
            txlist=[spend],
        )
        add_witness_commitment(block)
        block.solve()

        self.log.info("The block must be rejected regardless of -par")
        for i, n in enumerate(self.nodes):
            assert_equal(n.getbestblockhash(), tip)
            reason = n.submitblock(block.serialize().hex())
            assert reason is not None, f"node{i} accepted an unspendable taproot spend"
            self.log.info(f"node{i} ({self.extra_args[i][0]}) rejected the block: {reason}")
            assert_equal(n.getbestblockhash(), tip)


if __name__ == '__main__':
    TaprootInvalidOutputKeyTest(__file__).main()
