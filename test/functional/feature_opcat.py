#!/usr/bin/env python3
# Copyright (c) 2014-2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test (OP_CAT)
"""

from test_framework.blocktools import (
    create_coinbase,
    create_block,
    add_witness_commitment,
)

from test_framework.messages import (
    CTransaction,
    CTxOut,
    CTxIn,
    CTxInWitness,
    COutPoint,
    COIN,
    sha256
)
from test_framework.address import (
    hash160,
)
from test_framework.p2p import P2PInterface
from test_framework.script import (
    CScript,
    OP_CAT,
    OP_HASH160,
    OP_EQUAL,
    taproot_construct,
)
from test_framework.script_util import script_to_p2sh_script
from test_framework.key import ECKey, compute_xonly_pubkey
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet, MiniWalletMode
from decimal import Decimal
import random
from io import BytesIO
from test_framework.address import script_to_p2sh

DISABLED_OP_CODE = (
    "mandatory-script-verify-flag-failed (Attempted to use a disabled opcode)"
)

DISCOURAGED_CAT_ERROR = (
    "OP_SUCCESSx reserved for soft-fork upgrades"
)


def random_bytes(n):
    return bytes(random.getrandbits(8) for i in range(n))


def random_p2sh():
    return script_to_p2sh_script(random_bytes(20))


def create_transaction_to_script(node, wallet, txid, script, *, amount_sats):
    """Return signed transaction spending the first output of the
    input txid. Note that the node must be able to sign for the
    output that is being spent, and the node must not be running
    multiple wallets.
    """
    random_address = script_to_p2sh(CScript())
    output = wallet.get_utxo(txid=txid)
    rawtx = node.createrawtransaction(
        inputs=[{"txid": output["txid"], "vout": output["vout"]}],
        outputs={random_address: Decimal(amount_sats) / COIN},
    )
    tx = CTransaction()
    tx.deserialize(BytesIO(bytes.fromhex(rawtx)))
    # Replace with our script
    tx.vout[0].scriptPubKey = script
    # Sign
    wallet.sign_tx(tx)
    return tx


class CatTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [
            ["-par=1"]
        ]  # Use only one script thread to get the exact reject reason for testing
        self.setup_clean_chain = True
        self.rpc_timeout = 120

    def get_block(self, txs):
        self.tip = self.nodes[0].getbestblockhash()
        self.height = self.nodes[0].getblockcount()
        self.log.debug(self.height)
        block = create_block(
            int(self.tip, 16), create_coinbase(self.height + 1))
        block.vtx.extend(txs)
        add_witness_commitment(block)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        return block.serialize(True).hex(), block.hash

    def add_block(self, txs):
        block, h = self.get_block(txs)
        reason = self.nodes[0].submitblock(block)
        if reason:
            self.log.debug("Reject Reason: [%s]", reason)
        assert_equal(self.nodes[0].getbestblockhash(), h)
        return h

    def run_test(self):
        # The goal of this test suite is to rest OP_CAT is disabled by default in segwitv0 and p2sh script.
        # and discourage tapscript.

        wallet = MiniWallet(self.nodes[0], mode=MiniWalletMode.RAW_P2PK)
        self.nodes[0].add_p2p_connection(P2PInterface())

        BLOCKS = 200
        self.log.info("Mining %d blocks for mature coinbases", BLOCKS)
        # Drop the last 100 as they're unspendable!
        coinbase_txids = [
            self.nodes[0].getblock(b)["tx"][0]
            for b in self.generate(wallet, BLOCKS)[:-100]
        ]
        def get_coinbase(): return coinbase_txids.pop()
        self.log.info("Creating setup transactions")

        outputs = [CTxOut(i * 1000, random_p2sh()) for i in range(1, 11)]
        # Add some fee
        amount_sats = sum(out.nValue for out in outputs) + 200 * 500

        private_key = ECKey()
        # use simple deterministic private key (k=1)
        private_key.set((1).to_bytes(32, "big"), False)
        assert private_key.is_valid
        public_key, _ = compute_xonly_pubkey(private_key.get_bytes())

        op_cat_script = CScript([
            # Calling CAT on an empty stack
            # The content of the stack doesn't really matter for what we are testing
            # The interpreter should never get to the point where its executing this OP_CAT instruction
            OP_CAT,
        ])

        self.log.info("Creating a CAT tapscript funding tx")
        taproot_op_cat = taproot_construct(
            public_key, [("only-path", op_cat_script, 0xC0)])
        taproot_op_cat_funding_tx = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            taproot_op_cat.scriptPubKey,
            amount_sats=amount_sats,
        )

        self.log.info("Creating a CAT segwit funding tx")
        segwit_cat_funding_tx = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            CScript([0, sha256(op_cat_script)]),
            amount_sats=amount_sats,
        )

        self.log.info("Create p2sh OP_CAT funding tx")
        p2sh_cat_funding_tx = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            CScript([OP_HASH160, hash160(op_cat_script), OP_EQUAL]),
            amount_sats=amount_sats,
        )

        funding_txs = [
            taproot_op_cat_funding_tx,
            segwit_cat_funding_tx,
            p2sh_cat_funding_tx
        ]
        (
            taproot_op_cat_outpoint,
            segwit_op_cat_outpoint,
            bare_op_cat_outpoint,
        ) = [COutPoint(int(tx.rehash(), 16), 0) for tx in funding_txs]

        self.log.info("Funding all outputs")
        self.add_block(funding_txs)

        self.log.info("Testing tapscript OP_CAT usage is discouraged")
        taproot_op_cat_transaction = CTransaction()
        taproot_op_cat_transaction.vin = [
            CTxIn(taproot_op_cat_outpoint)]
        taproot_op_cat_transaction.vout = outputs
        taproot_op_cat_transaction.wit.vtxinwit += [
            CTxInWitness()]
        taproot_op_cat_transaction.wit.vtxinwit[0].scriptWitness.stack = [
            op_cat_script,
            bytes([0xC0 + taproot_op_cat.negflag]) +
            taproot_op_cat.internal_pubkey,
        ]

        assert_raises_rpc_error(
            -26,
            DISCOURAGED_CAT_ERROR,
            self.nodes[0].sendrawtransaction,
            taproot_op_cat_transaction.serialize().hex(),
        )
        self.log.info(
            "Tapscript OP_CAT spend not accepted by sendrawtransaction"
        )

        self.log.info("Testing Segwitv0 OP_CAT usage is disabled")
        segwitv0_op_cat_transaction = CTransaction()
        segwitv0_op_cat_transaction.vin = [
            CTxIn(segwit_op_cat_outpoint)]
        segwitv0_op_cat_transaction.vout = outputs
        segwitv0_op_cat_transaction.wit.vtxinwit += [
            CTxInWitness()]
        segwitv0_op_cat_transaction.wit.vtxinwit[0].scriptWitness.stack = [
            op_cat_script,
        ]

        assert_raises_rpc_error(
            -26,
            DISABLED_OP_CODE,
            self.nodes[0].sendrawtransaction,
            segwitv0_op_cat_transaction.serialize().hex(),
        )
        self.log.info("Segwitv0 OP_CAT spend failed with expected error")

        self.log.info("Testing p2sh script OP_CAT usage is disabled")
        p2sh_op_cat_transaction = CTransaction()
        p2sh_op_cat_transaction.vin = [
            CTxIn(bare_op_cat_outpoint)]
        p2sh_op_cat_transaction.vin[0].scriptSig = CScript(
            [op_cat_script])
        p2sh_op_cat_transaction.vout = outputs

        assert_raises_rpc_error(
            -26,
            DISABLED_OP_CODE,
            self.nodes[0].sendrawtransaction,
            p2sh_op_cat_transaction.serialize().hex(),
        )
        self.log.info("p2sh OP_CAT spend failed with expected error")


if __name__ == "__main__":
    CatTest(__file__).main()
