#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool wtxid acceptance in case of an already known transaction
with identical non-witness data but lower feerate witness."""

from codecs import encode
from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework

from test_framework.messages import (
        CTransaction,
        CTxIn,
        CTxInWitness,
        CTxOut,
        COutPoint,
        sha256,
        COIN,
        hash256,
)

from test_framework.util import (
        assert_equal,
        assert_not_equal,
)

from test_framework.script import (
        CScript,
        OP_0,
        OP_TRUE,
        OP_IF,
        OP_HASH160,
        OP_EQUAL,
        OP_ELSE,
        OP_ENDIF,
        hash160,
)

class MempoolWtxidTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.supports_cli = False
        self.extra_args = [["-incrementalrelayfee=0"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]

        self.log.info('Start with empty mempool, and 200 blocks')
        self.mempool_size = 0
        assert_equal(node.getblockcount(), 200)
        assert_equal(node.getmempoolinfo()['size'], self.mempool_size)

        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), Decimal("10"))

        # Create a Segwit output
        hashlock = hash160(b'Preimage')
        witness_script = CScript([OP_IF, OP_HASH160, hashlock, OP_EQUAL, OP_ELSE, OP_TRUE, OP_ENDIF])
        witness_program = sha256(witness_script)
        script_pubkey = CScript([OP_0, witness_program])

        parent = CTransaction()
        parent.vin.append(CTxIn(COutPoint(int(txid, 16), 0), b""))
        parent.vout.append(CTxOut(int(9.99998 * COIN), script_pubkey))
        parent.rehash()

        # Confirm parent with alternative script branches witnessScript
        raw_parent = self.nodes[0].signrawtransactionwithwallet(parent.serialize().hex())['hex']
        parent_txid = self.nodes[0].sendrawtransaction(hexstring=raw_parent, maxfeerate=0)
        self.nodes[0].generate(1)

        # Create a new segwit transaction with witness solving first branch
        child_witness_script = CScript([OP_TRUE])
        child_witness_program = sha256(child_witness_script)
        child_script_pubkey = CScript([OP_0, child_witness_program])

        child_one = CTransaction()
        child_one.vin.append(CTxIn(COutPoint(int(parent_txid, 16), 0), b""))
        child_one.vout.append(CTxOut(int(9.99996 * COIN), child_script_pubkey))
        child_one.wit.vtxinwit.append(CTxInWitness())
        child_one.wit.vtxinwit[0].scriptWitness.stack = [b'Preimage', b'\x01', witness_script]
        child_one_txid = self.nodes[0].sendrawtransaction(hexstring=child_one.serialize().hex())
        child_one_wtxid = encode(hash256(child_one.serialize_with_witness())[::-1], 'hex_codec').decode('ascii')

        self.log.info('Verify that transaction with lower-feerate witness gets in the mempool')
        in_mempool_wtxid = self.nodes[0].getrawmempool(True)[child_one_txid]['wtxid']
        assert_equal(child_one_wtxid, in_mempool_wtxid)

        # Create another identical segwit transaction with witness solving second branch
        child_two = CTransaction()
        child_two.vin.append(CTxIn(COutPoint(int(parent_txid, 16), 0), b""))
        child_two.vout.append(CTxOut(int(9.99996 * COIN), child_script_pubkey))
        child_two.wit.vtxinwit.append(CTxInWitness())
        child_two.wit.vtxinwit[0].scriptWitness.stack = [b'', witness_script]
        child_two_txid = self.nodes[0].sendrawtransaction(hexstring=child_two.serialize().hex())
        child_two_wtxid = encode(hash256(child_two.serialize_with_witness())[::-1], 'hex_codec').decode('ascii')

        assert_equal(child_one_txid, child_two_txid)
        assert_not_equal(child_one_wtxid, child_two_wtxid)

        self.log.info('Verify that identical transaction with higher-feerate witness gets in the mempool')
        in_mempool_wtxid = self.nodes[0].getrawmempool(True)[child_one_txid]['wtxid']
        assert_equal(child_two_wtxid, in_mempool_wtxid)

if __name__ == '__main__':
    MempoolWtxidTest().main()
