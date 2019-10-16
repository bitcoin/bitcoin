#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test OP_CHECKTEMPLATEVERIFY (BIP-119)
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
    sha256,
)
from test_framework.p2p import P2PInterface
from test_framework.script import (
    CScript,
    OP_TRUE,
    OP_DEPTH,
    OP_ENDIF,
    OP_IF,
    OP_CHECKTEMPLATEVERIFY,
    OP_FALSE,
    OP_DROP,
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

CHECKTEMPLATEVERIFY_ERROR = "mandatory-script-verify-flag-failed (Script failed an OP_CHECKTEMPLATEVERIFY operation)"
DISCOURAGED_ERROR = (
    "non-mandatory-script-verify-flag (NOPx reserved for soft-fork upgrades)"
)
STACK_TOO_SHORT_ERROR = (
    "mandatory-script-verify-flag-failed (Operation not valid with the current stack size)"
)


def random_bytes(n):
    return bytes(random.getrandbits(8) for i in range(n))


def template_hash_for_outputs(outputs, nIn=0, nVin=1, vin_override=None):
    c = CTransaction()
    c.version = 2
    c.vin = vin_override
    if vin_override is None:
        c.vin = [CTxIn()] * nVin
    c.vout = outputs
    return c.get_standard_template_hash(nIn)


def random_p2sh():
    return script_to_p2sh_script(random_bytes(20))


def random_real_outputs_and_script(n, nIn=0, nVin=1, vin_override=None):
    outputs = [CTxOut((x + 1) * 1000, random_p2sh()) for x in range(n)]
    script = CScript(
        [
            template_hash_for_outputs(outputs, nIn, nVin, vin_override),
            OP_CHECKTEMPLATEVERIFY,
        ]
    )
    return outputs, script


def random_secure_tree(depth):
    leaf_nodes = [
        CTxOut(nValue=100, scriptPubKey=CScript(bytes([0, 0x14]) + random_bytes(20)))
        for x in range(2 ** depth)
    ]
    outputs_tree = [[CTxOut()] * (2 ** i) for i in range(depth)] + [leaf_nodes]
    for d in range(1, depth + 2):
        idxs = zip(
            range(0, len(outputs_tree[-d]), 2), range(1, len(outputs_tree[-d]), 2)
        )
        for (idx, (a, b)) in enumerate(
            [(outputs_tree[-d][i], outputs_tree[-d][j]) for (i, j) in idxs]
        ):
            s = CScript(
                bytes([0x20])
                + template_hash_for_outputs([a, b])
                + bytes([OP_CHECKTEMPLATEVERIFY])
            )
            a = sum(o.nValue for o in [a, b])
            t = CTxOut(a + 1000, s)
            outputs_tree[-d - 1][idx] = t
    return outputs_tree


def create_transaction_to_script(node, wallet, txid, script, *, amount_sats):
    """
    Return signed transaction spending the first output of the
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


class CheckTemplateVerifyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # Use only one script thread to get the exact reject reason for testing
        self.extra_args = [
            ["-par=1"]
        ]
        self.setup_clean_chain = True
        self.rpc_timeout = 120

    def get_block(self, txs):
        self.tip = self.nodes[0].getbestblockhash()
        self.height = self.nodes[0].getblockcount()
        self.log.debug(self.height)
        block = create_block(int(self.tip, 16), create_coinbase(self.height + 1))
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

    def fail_block(self, txs, cause=CHECKTEMPLATEVERIFY_ERROR):
        block, h = self.get_block(txs)
        assert_equal(self.nodes[0].submitblock(block), cause)
        assert_equal(self.nodes[0].getbestblockhash(), self.tip)

    def run_test(self):
        """
        The goal is to test a number of circumstances and combinations of parameters. Roughly:

          - Taproot OP_CTV
          - SegWit OP_CTV
          - SegWit OP_CTV wrong size on stack
            - fails policy
            - passes consensus
          - SegWit OP_CTV no argument in stack from program
            - fails policy and consensus with empty stack
            - passes consensus and policy when argument is the correct hash
            - passes consensus when argument is non 32 bytes
            - fails policy when argument is non 32 bytes
          - P2SH OP_CTV (impossible to spend w/o hash cycle!)
          - Bare OP_CTV
          - OP_CTV at vin index 0
          - OP_CTV at vin index > 0
          - OP_CTV with scriptSigs set
          - OP_CTV without scriptSigs set
          - OP_CTV with multiple inputs
          - accepting correct parameters
          - rejecting incorrect parameters
          - OP_CTV in a tree

        A few tests may seem redundant, but it is because they are testing the cached computation of the hash
        at vin index 0
        """
        wallet = MiniWallet(self.nodes[0], mode=MiniWalletMode.RAW_P2PK)
        self.nodes[0].add_p2p_connection(P2PInterface())

        BLOCKS = 115
        self.log.info("Mining %d blocks for mature coinbases", BLOCKS)
        # Drop the last 100 as they're unspendable!
        coinbase_txids = [
            self.nodes[0].getblock(b)["tx"][0]
            for b in self.generate(wallet, BLOCKS)[:-100]
        ]
        get_coinbase = lambda: coinbase_txids.pop()

        self.log.info("Creating setup transactions")

        self.log.info("Creating script for 10 random outputs")
        outputs, script = random_real_outputs_and_script(10)
        # Add some fee satoshis
        amount_sats = sum(out.nValue for out in outputs) + 200 * 500

        self.log.info("Creating funding txn for 10 random outputs as a Taproot script")
        private_key = ECKey()
        # use simple deterministic private key (k=1)
        private_key.set((1).to_bytes(32, "big"), False)
        assert private_key.is_valid
        public_key, _ = compute_xonly_pubkey(private_key.get_bytes())
        taproot = taproot_construct(public_key, [("only-path", script, 0xC0)])
        taproot_ctv_funding_tx = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            taproot.scriptPubKey,
            amount_sats=amount_sats,
        )

        self.log.info("Creating funding txn for 10 random outputs as a segwit script")
        segwit_ctv_funding_tx = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            CScript([0, sha256(script)]),
            amount_sats=amount_sats,
        )

        self.log.info("Creating a CTV with a non 32 byte stack segwit script")
        segwit_ctv_funding_tx_wrongsize_stack = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            CScript([0, sha256(CScript([OP_TRUE, OP_CHECKTEMPLATEVERIFY]))]),
            amount_sats=amount_sats,
        )

        self.log.info("Creating a CTV with an empty stack segwit script")
        # allows either calling with empty witness stack or with a 32 byte hash (cleanstack rule)
        empty_stack_script = CScript(
            [OP_CHECKTEMPLATEVERIFY, OP_DEPTH, OP_IF, OP_DROP, OP_ENDIF, OP_TRUE]
        )
        segwit_ctv_funding_tx_empty_stack = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            CScript([0, sha256(empty_stack_script)]),
            amount_sats=amount_sats,
        )

        self.log.info(
            "Creating funding txn for 10 random outputs as a p2sh script (impossible to spend)"
        )
        p2sh_ctv_funding_tx = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            script_to_p2sh_script(script),
            amount_sats=amount_sats,
        )

        # Small tree size 4 for test speed.
        # Can be set to a large value like 16 (i.e., 65K txns).
        TREE_SIZE = 4
        self.log.info(f"Creating script for tree size depth {TREE_SIZE}")
        congestion_tree_txo = random_secure_tree(TREE_SIZE)

        self.log.info(f"Creating funding txn for tree size depth {TREE_SIZE}")
        bare_ctv_tree_funding_tx = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            congestion_tree_txo[0][0].scriptPubKey,
            amount_sats=congestion_tree_txo[0][0].nValue,
        )

        self.log.info("Creating script for spend at position 2")
        outputs_position_2, script_position_2 = random_real_outputs_and_script(10, 1, 2)
        # Add some fee satoshis
        amount_position_2 = sum(out.nValue for out in outputs_position_2) + 200 * 500

        self.log.info("Creating funding txn for spend at position 2")
        bare_ctv_position_2_funding_tx = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            script_position_2,
            amount_sats=amount_position_2,
        )

        self.log.info(
            "Creating script for spend at position 1 with 2 non-null scriptsigs"
        )
        (
            outputs_specific_scriptSigs,
            script_specific_scriptSigs,
        ) = random_real_outputs_and_script(
            10,
            0,
            2,
            [CTxIn(scriptSig=CScript([OP_TRUE])), CTxIn(scriptSig=CScript([OP_FALSE]))],
        )
        # Add some fee satoshis
        amount_specific_scriptSigs = (
            sum(out.nValue for out in outputs_specific_scriptSigs) + 200 * 500
        )

        self.log.info(
            "Creating funding txn for spend at position 1 with 2 non-null scriptsigs"
        )
        bare_ctv_specific_scriptSigs_funding_tx = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            script_specific_scriptSigs,
            amount_sats=amount_specific_scriptSigs,
        )

        self.log.info(
            "Creating script for spend at position 2 with 2 non-null scriptsigs"
        )
        (
            outputs_specific_scriptSigs_position_2,
            script_specific_scriptSigs_position_2,
        ) = random_real_outputs_and_script(
            10,
            1,
            2,
            [CTxIn(scriptSig=CScript([OP_TRUE])), CTxIn(scriptSig=CScript([OP_FALSE]))],
        )
        # Add some fee satoshis
        amount_specific_scriptSigs_position_2 = (
            sum(out.nValue for out in outputs_specific_scriptSigs_position_2)
            + 200 * 500
        )

        self.log.info(
            "Creating funding txn for spend at position 2 with 2 non-null scriptsigs"
        )
        bare_ctv_specific_scriptSigs_position_2_funding_tx = (
            create_transaction_to_script(
                self.nodes[0],
                wallet,
                get_coinbase(),
                script_specific_scriptSigs_position_2,
                amount_sats=amount_specific_scriptSigs_position_2,
            )
        )

        self.log.info("Creating funding txns for some anyone can spend outputs")
        anyone_can_spend_funding_tx = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            CScript([OP_TRUE]),
            amount_sats=amount_sats,
        )
        bare_anyone_can_spend_funding_tx = create_transaction_to_script(
            self.nodes[0],
            wallet,
            get_coinbase(),
            CScript([OP_TRUE]),
            amount_sats=amount_sats,
        )

        funding_txs = [
            taproot_ctv_funding_tx,
            segwit_ctv_funding_tx,
            segwit_ctv_funding_tx_wrongsize_stack,
            segwit_ctv_funding_tx_empty_stack,
            p2sh_ctv_funding_tx,
            anyone_can_spend_funding_tx,
            bare_ctv_tree_funding_tx,
            bare_ctv_position_2_funding_tx,
            bare_anyone_can_spend_funding_tx,
            bare_ctv_specific_scriptSigs_funding_tx,
            bare_ctv_specific_scriptSigs_position_2_funding_tx,
        ]

        self.log.info("Obtaining TXIDs")
        (
            taproot_ctv_outpoint,
            segwit_ctv_outpoint,
            segwit_ctv_wrongsize_stack_outpoint,
            segwit_ctv_empty_stack_outpoint,
            p2sh_ctv_outpoint,
            anyone_can_spend_outpoint,
            bare_ctv_tree_outpoint,
            bare_ctv_position_2_outpoint,
            bare_anyone_can_spend_outpoint,
            bare_ctv_specific_scriptSigs_outpoint,
            bare_ctv_specific_scriptSigs_position_2_outpoint,
        ) = [COutPoint(int(tx.txid_hex, 16), 0) for tx in funding_txs]

        self.log.info("Funding all outputs")
        self.add_block(funding_txs)

        self.log.info("Testing Taproot OP_CHECKTEMPLATEVERIFY spend")
        # Test sendrawtransaction
        taproot_check_template_verify_tx = CTransaction()
        taproot_check_template_verify_tx.version = 2
        taproot_check_template_verify_tx.vin = [CTxIn(taproot_ctv_outpoint)]
        taproot_check_template_verify_tx.vout = outputs
        taproot_check_template_verify_tx.wit.vtxinwit += [CTxInWitness()]
        taproot_check_template_verify_tx.wit.vtxinwit[0].scriptWitness.stack = [
            script,
            bytes([0xC0 + taproot.negflag]) + taproot.internal_pubkey,
        ]
        assert_equal(
            self.nodes[0].sendrawtransaction(
                taproot_check_template_verify_tx.serialize().hex()
            ),
            taproot_check_template_verify_tx.txid_hex,
        )
        self.log.info(
            "Taproot OP_CHECKTEMPLATEVERIFY spend accepted by sendrawtransaction"
        )

        # Now we verify that a block with this transaction is also valid
        blockhash = self.add_block([taproot_check_template_verify_tx])
        self.log.info("Taproot OP_CHECKTEMPLATEVERIFY spend accepted in a block")

        self.log.info("Rolling back the block")
        # Reset tip
        self.nodes[0].invalidateblock(blockhash)

        self.log.info("Testing Segwit OP_CHECKTEMPLATEVERIFY spend")
        # Test sendrawtransaction
        check_template_verify_tx = CTransaction()
        check_template_verify_tx.version = 2
        check_template_verify_tx.vin = [CTxIn(segwit_ctv_outpoint)]
        check_template_verify_tx.vout = outputs

        check_template_verify_tx.wit.vtxinwit += [CTxInWitness()]
        check_template_verify_tx.wit.vtxinwit[0].scriptWitness.stack = [script]
        assert_equal(
            self.nodes[0].sendrawtransaction(
                check_template_verify_tx.serialize().hex()
            ),
            check_template_verify_tx.txid_hex,
        )
        self.log.info(
            "Segwit OP_CHECKTEMPLATEVERIFY spend accepted by sendrawtransaction"
        )

        # Now we verify that a block with this transaction is also valid
        blockhash = self.add_block([check_template_verify_tx])
        self.log.info("Segwit OP_CHECKTEMPLATEVERIFY spend accepted in a block")

        self.log.info("Rolling back the block")
        # Reset tip
        self.nodes[0].invalidateblock(blockhash)

        # Show any modification will break the validity
        self.log.info(
            "Modifying Segwit OP_CHECKTEMPLATEVERIFY spend, block should fail"
        )
        check_template_verify_tx_mutated_amount = check_template_verify_tx
        check_template_verify_tx_mutated_amount.vout[0].nValue += 1
        check_template_verify_tx_mutated_amount.txid_hex
        self.fail_block([check_template_verify_tx_mutated_amount])
        self.log.info("Modified Segwit OP_CHECKTEMPLATEVERIFY spend failed to confirm")

        # Now show that only one input allowed
        self.log.info("Testing that multiple inputs are disallowed when specified")
        check_template_verify_two_inputs = check_template_verify_tx
        check_template_verify_two_inputs.vin += [CTxIn(anyone_can_spend_outpoint)]
        check_template_verify_two_inputs.txid_hex
        self.fail_block([check_template_verify_two_inputs])

        self.log.info(
            "Testing that the second input specified was actually spendable by itself"
        )

        # Second UTXO was actually spendable
        spendtx = CTransaction()
        spendtx.version = 2
        spendtx.vin = [CTxIn(anyone_can_spend_outpoint)]
        spendtx.vout += [CTxOut(int(amount_sats - 1000), random_p2sh())]
        spendtx.txid_hex
        blockhash = self.add_block([spendtx])
        # Reset tip
        self.nodes[0].invalidateblock(blockhash)

        self.log.info(
            "Testing Segwit OP_CHECKTEMPLATEVERIFY spend with a wrong size argument"
        )
        # Test sendrawtransaction
        check_template_verify_tx_wrongsize_stack = CTransaction()
        check_template_verify_tx_wrongsize_stack.version = 2
        check_template_verify_tx_wrongsize_stack.vin = [
            CTxIn(segwit_ctv_wrongsize_stack_outpoint)
        ]
        check_template_verify_tx_wrongsize_stack.vout = outputs

        check_template_verify_tx_wrongsize_stack.wit.vtxinwit += [CTxInWitness()]
        check_template_verify_tx_wrongsize_stack.wit.vtxinwit[0].scriptWitness.stack = [
            CScript([OP_TRUE, OP_CHECKTEMPLATEVERIFY])
        ]

        assert_raises_rpc_error(
            -26,
            DISCOURAGED_ERROR,
            self.nodes[0].sendrawtransaction,
            check_template_verify_tx_wrongsize_stack.serialize().hex(),
        )
        self.log.info(
            "OP_CHECKTEMPLATEVERIFY with wrong size stack rejected by sendrawtransaction as discouraged"
        )

        # Now we verify that a block with this transaction is valid
        blockhash = self.add_block([check_template_verify_tx_wrongsize_stack])
        self.log.info(
            "Segwit OP_CHECKTEMPLATEVERIFY with wrong size stack spend accepted in a block (soft fork upgradable)"
        )

        self.log.info(
            "Testing Segwit OP_CHECKTEMPLATEVERIFY spend with an empty stack argument"
        )
        # Test sendrawtransaction
        check_template_verify_tx_empty_stack = CTransaction()
        check_template_verify_tx_empty_stack.version = 2
        check_template_verify_tx_empty_stack.vin = [
            CTxIn(segwit_ctv_empty_stack_outpoint)
        ]
        check_template_verify_tx_empty_stack.vout = outputs

        check_template_verify_tx_empty_stack.wit.vtxinwit += [CTxInWitness()]
        check_template_verify_tx_empty_stack.wit.vtxinwit[0].scriptWitness.stack = [
            empty_stack_script
        ]

        assert_raises_rpc_error(
            -26,
            STACK_TOO_SHORT_ERROR,
            self.nodes[0].sendrawtransaction,
            check_template_verify_tx_empty_stack.serialize().hex(),
        )

        self.log.info(
            "OP_CHECKTEMPLATEVERIFY with wrong size stack rejected by sendrawtransaction as discouraged"
        )

        # Now we verify that a block with this transaction is invalid
        self.fail_block([check_template_verify_tx_empty_stack], STACK_TOO_SHORT_ERROR)
        self.log.info(
            "Segwit OP_CHECKTEMPLATEVERIFY with wrong size stack spend rejected from block"
        )

        # Show that putting some element on the stack makes it succeed for consensus but fail policy
        self.log.info(
            "Segwit OP_CHECKTEMPLATEVERIFY with CTV argument in the program should fail policy pass consensus if witness stack is not 32 bytes"
        )
        check_template_verify_tx_empty_stack.wit.vtxinwit[0].scriptWitness.stack = [
            CScript([OP_TRUE]),
            empty_stack_script,
        ]
        assert_raises_rpc_error(
            -26,
            DISCOURAGED_ERROR,
            self.nodes[0].sendrawtransaction,
            check_template_verify_tx_empty_stack.serialize().hex(),
        )
        self.log.info(
            "OP_CHECKTEMPLATEVERIFY with wrong size argument on the witness stack rejected by sendrawtransaction as discouraged"
        )

        # Now we verify that a block with this transaction is valid
        blockhash = self.add_block([check_template_verify_tx_empty_stack])
        self.log.info(
            "Segwit OP_CHECKTEMPLATEVERIFY with empty stack spend accepted in a block with something on the witness stack"
        )
        self.log.info("Rolling back the block")
        # Reset tip
        self.nodes[0].invalidateblock(blockhash)

        # Put the correct hash on
        self.log.info(
            "Testing OP_CHECKTEMPLATEVERIFY spend with template hash from the witness stack"
        )
        h = check_template_verify_tx_empty_stack.get_standard_template_hash(0)
        check_template_verify_tx_empty_stack.wit.vtxinwit[0].scriptWitness.stack = [
            h,
            empty_stack_script,
        ]
        assert_equal(
            self.nodes[0].sendrawtransaction(
                check_template_verify_tx_empty_stack.serialize().hex()
            ),
            check_template_verify_tx_empty_stack.txid_hex,
        )
        self.log.info(
            "Witness stack defined OP_CHECKTEMPLATEVERIFY spend accepted by sendrawtransaction"
        )
        # Now we verify that a block with this transaction is also valid
        blockhash = self.add_block([check_template_verify_tx_empty_stack])
        self.log.info(
            "Witness stack defined OP_CHECKTEMPLATEVERIFY spend accepted in a block"
        )
        self.log.info("Rolling back the block")
        # Reset tip
        self.nodes[0].invalidateblock(blockhash)

        self.log.info(
            "Testing that other 32 byte arguments passed from the witness stack fail"
        )
        h = h[::-1]
        check_template_verify_tx_empty_stack.wit.vtxinwit[0].scriptWitness.stack = [
            h,
            empty_stack_script,
        ]
        self.fail_block([check_template_verify_tx_empty_stack])

        # Test sendrawtransaction with P2SH
        # Mathematically, this test cannot succeed since P2SH requires that the
        # P2SH script is in the scriptsig (which contains H), and H much
        # contain a commitment to the scriptsig.
        #
        # Note that this relies on RIPEMD160 being hard to find a cycle in.
        # This *could* break one day (P2SH is no longer recommended), but
        # there's no capability that would be gained by doing so (such as a
        # recursive covenant), it'd just be a "party trick" (and you could
        # also possibly steal all existing P2SH addresses).
        p2sh_check_template_verify_tx = CTransaction()
        p2sh_check_template_verify_tx.version = 2
        p2sh_check_template_verify_tx.vin = [
            CTxIn(p2sh_ctv_outpoint, CScript([script]))
        ]
        p2sh_check_template_verify_tx.vout = outputs

        assert_raises_rpc_error(
            -26,
            CHECKTEMPLATEVERIFY_ERROR,
            self.nodes[0].sendrawtransaction,
            p2sh_check_template_verify_tx.serialize().hex(),
        )
        self.log.info(
            "P2SH OP_CHECKTEMPLATEVERIFY spend rejected by sendrawtransaction"
        )

        # Now we verify that a block with this transaction is also invalid
        self.fail_block([p2sh_check_template_verify_tx])
        self.log.info("P2SH OP_CHECKTEMPLATEVERIFY spend rejected in a block")

        self.log.info(
            "Testing a congestion control tree using bare OP_CHECKTEMPLATEVERIFY"
        )
        # Expand Congestion Control Tree to one specific input
        out = bare_ctv_tree_outpoint
        txs = []
        for level in congestion_tree_txo[1:]:
            spendtx = CTransaction()
            spendtx.version = 2
            spendtx.vin += [CTxIn(out)]
            spendtx.vout += level[:2]
            out = COutPoint(int(spendtx.txid_hex, 16), 0)
            txs.append(spendtx)
        self.add_block(txs)

        self.log.info("Testing bare OP_CHECKTEMPLATEVERIFY with CTV at position 2")
        check_template_verify_tx_pos_2 = CTransaction()
        check_template_verify_tx_pos_2.version = 2
        check_template_verify_tx_pos_2.vin = [CTxIn(bare_ctv_position_2_outpoint)]
        check_template_verify_tx_pos_2.vout = outputs_position_2
        self.log.info(
            "Testing that the transaction fails because we have too few inputs"
        )
        self.fail_block([check_template_verify_tx_pos_2])
        check_template_verify_tx_pos_2.vin += [CTxIn(bare_anyone_can_spend_outpoint)]
        check_template_verify_tx_pos_2.txid_hex
        self.log.info(
            "Testing that the transaction fails because the inputs are in the wrong order"
        )
        self.fail_block([check_template_verify_tx_pos_2])
        self.log.info(
            "Testing that the transaction succeeds when the inputs are in the correct order"
        )
        check_template_verify_tx_pos_2.vin.reverse()
        check_template_verify_tx_pos_2.txid_hex
        blockhash = self.add_block([check_template_verify_tx_pos_2])
        self.nodes[0].invalidateblock(blockhash)
        check_template_verify_tx_pos_2.vin[0].scriptSig = CScript([OP_TRUE])
        check_template_verify_tx_pos_2.txid_hex
        self.log.info(
            "Testing that the transaction fails because the scriptSig on the other input has been modified"
        )
        self.fail_block([check_template_verify_tx_pos_2])

        self.log.info(
            "Testing bare OP_CHECKTEMPLATEVERIFY with CTV at position 1 with specific scriptSigs"
        )
        check_template_verify_tx_specific_scriptSigs = CTransaction()
        check_template_verify_tx_specific_scriptSigs.version = 2
        check_template_verify_tx_specific_scriptSigs.vin = [
            CTxIn(bare_ctv_specific_scriptSigs_outpoint, CScript([OP_TRUE])),
            CTxIn(bare_anyone_can_spend_outpoint, CScript([OP_TRUE])),
        ]
        check_template_verify_tx_specific_scriptSigs.vout = outputs_specific_scriptSigs
        check_template_verify_tx_specific_scriptSigs.txid_hex
        self.log.info(
            "Testing bare OP_CHECKTEMPLATEVERIFY rejects incorrect scriptSigs"
        )
        self.fail_block([check_template_verify_tx_specific_scriptSigs])

        self.log.info("Testing bare OP_CHECKTEMPLATEVERIFY accepts correct scriptSigs")
        check_template_verify_tx_specific_scriptSigs.vin[1].scriptSig = CScript(
            [OP_FALSE]
        )
        check_template_verify_tx_specific_scriptSigs.txid_hex
        blockhash = self.add_block([check_template_verify_tx_specific_scriptSigs])
        self.nodes[0].invalidateblock(blockhash)

        self.log.info(
            "Testing bare OP_CHECKTEMPLATEVERIFY with CTV at position 2 with specific scriptSigs"
        )
        # This is only really to test that uncached values work correctly with scriptSig set
        check_template_verify_tx_specific_scriptSigs_position_2 = CTransaction()
        check_template_verify_tx_specific_scriptSigs_position_2.version = 2
        check_template_verify_tx_specific_scriptSigs_position_2.vin = [
            CTxIn(bare_anyone_can_spend_outpoint, CScript([OP_TRUE])),
            CTxIn(
                bare_ctv_specific_scriptSigs_position_2_outpoint, CScript([OP_FALSE])
            ),
        ]
        check_template_verify_tx_specific_scriptSigs_position_2.vout = (
            outputs_specific_scriptSigs_position_2
        )
        check_template_verify_tx_specific_scriptSigs_position_2.txid_hex
        self.add_block([check_template_verify_tx_specific_scriptSigs_position_2])


if __name__ == "__main__":
    CheckTemplateVerifyTest(__file__).main()
