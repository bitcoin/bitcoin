#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test witness v2 cross-input signature aggregation (BIP460).

Covers witness v2 output creation and spending: opted-out keypath spends,
half-agg (BIP458) and full-agg (BIP459) groups, script path spends, the
annex commitment and invalid witness structures.

Node 0 validates blocks with parallel script checks, node 1 with -par=1,
covering both the queued and the sequential CISA verification paths.
"""

from dataclasses import dataclass

from test_framework.address import program_to_witness
from test_framework.blocktools import (
    COINBASE_MATURITY,
    add_witness_commitment,
    create_block,
    create_coinbase,
)
from test_framework.cisa import (
    CISA_MARKER_FULLAGG,
    CISA_MARKER_HALFAGG,
    cisa_signature_hash,
    cisa_witness_element,
    fullagg_sign,
    halfagg_aggregate,
)
from test_framework.key import (
    compute_xonly_pubkey,
    generate_privkey,
    sign_schnorr,
    TaggedHash,
    tweak_add_privkey,
    tweak_add_pubkey,
)
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    SEQUENCE_FINAL,
)
from test_framework.script import (
    CScript,
    OP_1,
    OP_2,
    OP_CHECKSIG,
    taproot_construct,
    TaprootSignatureHash,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet

AMOUNT = 100_000
FEE = 20_000


@dataclass
class V2Utxo:
    txid: str
    vout: int
    amount: int
    privkey: bytes  # tweaked private key
    pubkey: bytes   # witness program (tweaked x-only pubkey)
    spk: CScript


def make_v2_key():
    """Generate a keypair tweaked for witness v2 keypath spending (no script tree).

    Returns (tweaked_privkey, tweaked_pubkey, scriptPubKey)."""
    internal_privkey = generate_privkey()
    internal_pubkey, _ = compute_xonly_pubkey(internal_privkey)
    tweak = TaggedHash("TapTweak", internal_pubkey)
    tweaked_privkey = tweak_add_privkey(internal_privkey, tweak)
    tweaked_pubkey, _ = tweak_add_pubkey(internal_pubkey, tweak)
    spk = CScript([OP_2, tweaked_pubkey])
    assert_equal(len(spk), 34)
    return tweaked_privkey, tweaked_pubkey, spk


class CISATest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        # Node 1 validates sequentially to cover the non-queued CISA path.
        self.extra_args = [[], ["-par=1"]]

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        self.generate(self.nodes[0], COINBASE_MATURITY + 20)

        self.test_activation_status()
        self.test_v2_output_creation()
        self.test_optout_keypath_spend()
        self.test_halfagg_group()
        self.test_fullagg_group()
        self.test_single_member_groups()
        self.test_mixed_transaction()
        self.test_annex_commitment()
        self.test_script_path_spend()
        self.test_cross_mode_protection()
        self.test_invalid_structures()
        self.test_invalid_block()

        # Both validation modes must agree on the final chain.
        self.sync_blocks()
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())

    # Helpers

    def fund_output(self, spk, privkey=b"", pubkey=b"", amount=AMOUNT):
        """Create and confirm an output paying to spk."""
        res = self.wallet.send_to(from_node=self.nodes[0], scriptPubKey=spk, amount=amount)
        self.generate(self.nodes[0], 1)
        return V2Utxo(res["txid"], res["sent_vout"], amount, privkey, pubkey, spk)

    def fund_v2(self, amount=AMOUNT):
        """Create and confirm a witness v2 keypath output."""
        privkey, pubkey, spk = make_v2_key()
        return self.fund_output(spk, privkey, pubkey, amount)

    def build_spend(self, utxos):
        """Build an unsigned transaction spending the given v2 utxos."""
        tx = CTransaction()
        tx.version = 2
        tx.vin = [CTxIn(outpoint=COutPoint(int(u.txid, 16), u.vout), nSequence=SEQUENCE_FINAL) for u in utxos]
        tx.vout = [CTxOut(sum(u.amount for u in utxos) - FEE, CScript([OP_1, bytes(32)]))]
        tx.wit.vtxinwit = [CTxInWitness() for _ in utxos]
        spent = [CTxOut(u.amount, u.spk) for u in utxos]
        return tx, spent

    def sign_optout(self, tx, spent, idx, utxo, hash_type=0, annex=None):
        """Set an opted-out keypath witness on input idx."""
        sighash = TaprootSignatureHash(tx, spent, hash_type, input_index=idx, annex=annex)
        sig = sign_schnorr(utxo.privkey, sighash)
        if hash_type != 0:
            sig += bytes([hash_type])
        stack = [sig]
        if annex is not None:
            stack.append(annex)
        tx.wit.vtxinwit[idx].scriptWitness.stack = stack

    def set_group_witness(self, tx, members, marker, sig_data, annexes):
        """Set the witnesses of an aggregation group from the per-input
        signature data, the final input being the last member."""
        for i, (idx, _, hash_type) in enumerate(members):
            final = i == len(members) - 1
            stack = [cisa_witness_element(sig_data[i], hash_type, marker if final else None)]
            if idx in annexes:
                stack.append(annexes[idx])
            tx.wit.vtxinwit[idx].scriptWitness.stack = stack

    def sign_halfagg_group(self, tx, spent, members, annexes=None):
        """Sign a half-aggregation group. members is a list of
        (input_index, utxo, hash_type) with the final member last."""
        annexes = annexes or {}
        pms = []
        for idx, utxo, hash_type in members:
            msg = cisa_signature_hash(tx, spent, idx, hash_type, CISA_MARKER_HALFAGG, annex=annexes.get(idx))
            sig = sign_schnorr(utxo.privkey, msg)
            pms.append((utxo.pubkey, msg, sig))
        aggsig = halfagg_aggregate(pms)
        sig_data = [sig[:32] for _, _, sig in pms[:-1]] + [aggsig[-64:]]
        self.set_group_witness(tx, members, CISA_MARKER_HALFAGG, sig_data, annexes)
        return aggsig

    def sign_fullagg_group(self, tx, spent, members):
        """Sign a full-aggregation group by running the DahLIAS protocol
        locally, members as in sign_halfagg_group."""
        seckeys = [utxo.privkey for _, utxo, _ in members]
        msgs = [cisa_signature_hash(tx, spent, idx, hash_type, CISA_MARKER_FULLAGG) for idx, _, hash_type in members]
        aggsig = fullagg_sign(seckeys, msgs)
        sig_data = [b""] * (len(members) - 1) + [aggsig]
        self.set_group_witness(tx, members, CISA_MARKER_FULLAGG, sig_data, annexes={})
        return aggsig

    def assert_accept(self, tx):
        """Check mempool acceptance, then mine and verify confirmation."""
        node = self.nodes[0]
        res = node.testmempoolaccept([tx.serialize().hex()])[0]
        assert res["allowed"], res
        txid = node.sendrawtransaction(tx.serialize().hex(), 0)
        blockhash = self.generate(node, 1)[0]
        assert txid in node.getblock(blockhash)["tx"]

    def assert_reject(self, tx, reason):
        node = self.nodes[0]
        res = node.testmempoolaccept([tx.serialize().hex()])[0]
        assert_equal(res["allowed"], False)
        assert reason in res["reject-reason"], f"{reason} not in {res['reject-reason']}"
        assert_raises_rpc_error(-26, reason, node.sendrawtransaction, tx.serialize().hex(), 0)

    # Subtests

    def test_activation_status(self):
        self.log.info("CISA deployment is active on regtest")
        for node in self.nodes:
            info = node.getdeploymentinfo()["deployments"]["cisa"]
            assert_equal(info["active"], True)

    def test_v2_output_creation(self):
        self.log.info("Witness v2 outputs are standard and recognized")
        utxo = self.fund_v2()
        out = self.nodes[0].gettxout(utxo.txid, utxo.vout)
        assert_equal(out["scriptPubKey"]["type"], "witness_v2_cisa")

        address = program_to_witness(2, utxo.pubkey)
        assert_equal(out["scriptPubKey"]["address"], address)
        info = self.nodes[0].validateaddress(address)
        assert_equal(info["isvalid"], True)
        assert_equal(info["witness_version"], 2)
        assert_equal(info["witness_program"], utxo.pubkey.hex())

    def test_optout_keypath_spend(self):
        self.log.info("Opted-out keypath spends with default and explicit sighash types")
        utxo1, utxo2 = self.fund_v2(), self.fund_v2()
        tx, spent = self.build_spend([utxo1, utxo2])
        self.sign_optout(tx, spent, 0, utxo1)  # SIGHASH_DEFAULT
        self.sign_optout(tx, spent, 1, utxo2, hash_type=0x81)  # ALL|ANYONECANPAY
        self.assert_accept(tx)

        self.log.info("Explicitly encoded SIGHASH_DEFAULT is invalid")
        utxo = self.fund_v2()
        tx, spent = self.build_spend([utxo])
        sighash = TaprootSignatureHash(tx, spent, 0, input_index=0)
        sig = sign_schnorr(utxo.privkey, sighash)
        tx.wit.vtxinwit[0].scriptWitness.stack = [sig + bytes([0x00])]
        self.assert_reject(tx, "Invalid Schnorr signature hash type")

        self.log.info("A 64-byte opted-out signature whose own last byte equals a marker value is valid")
        utxo = self.fund_v2()
        tx, spent = self.build_spend([utxo])
        sighash = TaprootSignatureHash(tx, spent, 0, input_index=0)
        aux = 0
        while True:
            sig = sign_schnorr(utxo.privkey, sighash, aux=aux.to_bytes(32, "big"))
            if sig[-1] in (CISA_MARKER_HALFAGG, CISA_MARKER_FULLAGG):
                break
            aux += 1
        tx.wit.vtxinwit[0].scriptWitness.stack = [sig]
        self.assert_accept(tx)

    def test_halfagg_group(self):
        self.log.info("Half-aggregation groups of two and three inputs")
        utxos = [self.fund_v2() for _ in range(2)]
        tx, spent = self.build_spend(utxos)
        self.sign_halfagg_group(tx, spent, [(0, utxos[0], 0), (1, utxos[1], 0)])
        self.assert_accept(tx)

        # Three inputs with mixed sighash types (SIGHASH_SINGLE on input 0).
        utxos = [self.fund_v2() for _ in range(3)]
        tx, spent = self.build_spend(utxos)
        self.sign_halfagg_group(tx, spent, [(0, utxos[0], 0x03), (1, utxos[1], 0), (2, utxos[2], 0x01)])
        self.assert_accept(tx)

    def test_fullagg_group(self):
        self.log.info("Full-aggregation groups of two and three inputs")
        utxos = [self.fund_v2() for _ in range(2)]
        tx, spent = self.build_spend(utxos)
        self.sign_fullagg_group(tx, spent, [(0, utxos[0], 0), (1, utxos[1], 0)])
        self.assert_accept(tx)

        utxos = [self.fund_v2() for _ in range(3)]
        tx, spent = self.build_spend(utxos)
        self.sign_fullagg_group(tx, spent, [(0, utxos[0], 0x01), (1, utxos[1], 0x01), (2, utxos[2], 0x01)])
        self.assert_accept(tx)

    def test_single_member_groups(self):
        self.log.info("Single-member aggregation groups carry a 64-byte signature")
        utxo = self.fund_v2()
        tx, spent = self.build_spend([utxo])
        aggsig = self.sign_halfagg_group(tx, spent, [(0, utxo, 0)])
        assert_equal(len(aggsig), 64)
        self.assert_accept(tx)

        utxo = self.fund_v2()
        tx, spent = self.build_spend([utxo])
        aggsig = self.sign_fullagg_group(tx, spent, [(0, utxo, 0)])
        assert_equal(len(aggsig), 64)
        self.assert_accept(tx)

    def test_mixed_transaction(self):
        self.log.info("Mixed transaction: opt-out, half-agg group, full-agg group and a v1 input")
        # Interleave the groups like example 4 of the BIP.
        v2 = [self.fund_v2() for _ in range(6)]

        # Also spend a taproot output alongside the v2 inputs.
        # The key construction is the same as for v2, only the output version differs.
        v1_privkey, v1_pubkey, _ = make_v2_key()
        v1_utxo = self.fund_output(CScript([OP_1, v1_pubkey]), v1_privkey, v1_pubkey)

        # Inputs: 0 opt-out, 1 half-agg, 2 full-agg, 3 full-agg,
        # 4 half-agg (final), 5 full-agg (final), 6 taproot v1.
        all_utxos = v2 + [v1_utxo]
        tx, spent = self.build_spend(all_utxos)
        self.sign_halfagg_group(tx, spent, [(1, v2[1], 0x01), (4, v2[4], 0)])
        self.sign_fullagg_group(tx, spent, [(2, v2[2], 0), (3, v2[3], 0), (5, v2[5], 0)])
        self.sign_optout(tx, spent, 0, v2[0])
        # Taproot input signed under plain BIP341 rules.
        v1_sighash = TaprootSignatureHash(tx, spent, 0, input_index=6)
        tx.wit.vtxinwit[6].scriptWitness.stack = [sign_schnorr(v1_utxo.privkey, v1_sighash)]
        self.assert_accept(tx)

    def test_annex_commitment(self):
        self.log.info("Annexes are committed by the aggregate signature")
        utxos = [self.fund_v2() for _ in range(2)]
        annex = bytes([0x50]) + b"CISA annex test"
        tx, spent = self.build_spend(utxos)
        self.sign_halfagg_group(tx, spent, [(0, utxos[0], 0), (1, utxos[1], 0)], annexes={0: annex})

        # Stripping the annex invalidates the aggregate signature.
        tx_stripped = CTransaction(tx)
        tx_stripped.wit.vtxinwit[0].scriptWitness.stack = tx.wit.vtxinwit[0].scriptWitness.stack[:1]
        self.assert_reject(tx_stripped, "CISA aggregate signature verification failed")

        # The intact transaction is valid by consensus but nonstandard,
        # annexes are not relayed for witness v2 just like for taproot.
        self.assert_reject(tx, "bad-witness-nonstandard")
        self.generate_block_with_tx(tx, expect_valid=True)

    def test_script_path_spend(self):
        self.log.info("Witness v2 script path spends follow BIP341/342 rules")
        leaf_privkey = generate_privkey()
        leaf_pubkey, _ = compute_xonly_pubkey(leaf_privkey)
        leaf_script = CScript([leaf_pubkey, OP_CHECKSIG])

        internal_privkey = generate_privkey()
        internal_pubkey, _ = compute_xonly_pubkey(internal_privkey)
        info = taproot_construct(internal_pubkey, [("leaf", leaf_script)])
        spk = CScript([OP_2, info.output_pubkey])

        utxo = self.fund_output(spk, pubkey=info.output_pubkey)

        tx, spent = self.build_spend([utxo])
        sighash = TaprootSignatureHash(tx, spent, 0, input_index=0, scriptpath=True, leaf_script=leaf_script, codeseparator_pos=0xffffffff)
        sig = sign_schnorr(leaf_privkey, sighash)
        leaf = info.leaves["leaf"]
        control = bytes([leaf.version + info.negflag]) + info.internal_pubkey + leaf.merklebranch
        tx.wit.vtxinwit[0].scriptWitness.stack = [sig, bytes(leaf_script), control]
        self.assert_accept(tx)

    def test_cross_mode_protection(self):
        self.log.info("Signatures cannot be moved between aggregation modes")
        # Fold two opted-out signatures into a half-aggregation group,
        # emulating a third-party aggregator. The aggregated signature
        # message commits to the mode, so this must fail.
        utxos = [self.fund_v2() for _ in range(2)]
        tx, spent = self.build_spend(utxos)
        pms = []
        for idx, utxo in enumerate(utxos):
            msg_optout = TaprootSignatureHash(tx, spent, 0, input_index=idx)
            sig = sign_schnorr(utxo.privkey, msg_optout)
            # The aggregator uses the half-agg flavored messages.
            msg_halfagg = cisa_signature_hash(tx, spent, idx, 0, CISA_MARKER_HALFAGG)
            pms.append((utxo.pubkey, msg_halfagg, sig))
        aggsig = halfagg_aggregate(pms)
        tx.wit.vtxinwit[0].scriptWitness.stack = [pms[0][2][:32]]
        tx.wit.vtxinwit[1].scriptWitness.stack = [cisa_witness_element(aggsig[-64:], 0, CISA_MARKER_HALFAGG)]
        self.assert_reject(tx, "CISA aggregate signature verification failed")

        # A signature over the half-agg flavored message is also not valid
        # as a bare opted-out signature.
        utxo = self.fund_v2()
        tx, spent = self.build_spend([utxo])
        msg_halfagg = cisa_signature_hash(tx, spent, 0, 0, CISA_MARKER_HALFAGG)
        sig = sign_schnorr(utxo.privkey, msg_halfagg)
        tx.wit.vtxinwit[0].scriptWitness.stack = [sig]
        self.assert_reject(tx, "Invalid Schnorr signature")

    def test_invalid_structures(self):
        self.log.info("Invalid witness v2 structures are rejected")
        utxo = self.fund_v2()

        # Empty witness
        tx, spent = self.build_spend([utxo])
        tx.wit.vtxinwit[0].scriptWitness.stack = []
        self.assert_reject(tx, "Witness program was passed an empty witness")

        # 1-byte element: full-agg member with the invalid sighash byte 0xbb
        tx, spent = self.build_spend([utxo])
        tx.wit.vtxinwit[0].scriptWitness.stack = [bytes([0xbb])]
        self.assert_reject(tx, "Invalid witness v2 sighash type")

        # 2-byte element ending in a marker
        tx, spent = self.build_spend([utxo])
        tx.wit.vtxinwit[0].scriptWitness.stack = [bytes([0x01, CISA_MARKER_HALFAGG])]
        self.assert_reject(tx, "Invalid witness v2 aggregation marker")

        # 66-byte element ending in a sighash byte instead of a marker
        tx, spent = self.build_spend([utxo])
        tx.wit.vtxinwit[0].scriptWitness.stack = [bytes(65) + bytes([0x01])]
        self.assert_reject(tx, "Invalid witness v2 aggregation marker")

        # Full-agg member with an empty element and no final input
        tx, spent = self.build_spend([utxo])
        tx.wit.vtxinwit[0].scriptWitness.stack = [b""]
        self.assert_reject(tx, "Invalid CISA aggregation group structure")

        # Half-agg members without a final input
        utxos = [self.fund_v2() for _ in range(2)]
        tx, spent = self.build_spend(utxos)
        tx.wit.vtxinwit[0].scriptWitness.stack = [bytes(32)]
        tx.wit.vtxinwit[1].scriptWitness.stack = [bytes(32)]
        self.assert_reject(tx, "Invalid CISA aggregation group structure")

        # Member after the group's final input
        utxos = [self.fund_v2() for _ in range(2)]
        tx, spent = self.build_spend(utxos)
        self.sign_halfagg_group(tx, spent, [(1, utxos[1], 0), (0, utxos[0], 0)])  # final on input 0
        self.assert_reject(tx, "Invalid CISA aggregation group structure")

        # Final input carrying the whole aggregate signature instead of its 64-byte share
        utxos = [self.fund_v2() for _ in range(2)]
        tx, spent = self.build_spend(utxos)
        aggsig = self.sign_halfagg_group(tx, spent, [(0, utxos[0], 0), (1, utxos[1], 0)])
        tx.wit.vtxinwit[1].scriptWitness.stack = [cisa_witness_element(aggsig, 0, CISA_MARKER_HALFAGG)]
        self.assert_reject(tx, "Invalid witness v2 aggregation marker")

        # Two final inputs in one group
        utxos = [self.fund_v2() for _ in range(2)]
        tx, spent = self.build_spend(utxos)
        aggsig = self.sign_halfagg_group(tx, spent, [(0, utxos[0], 0), (1, utxos[1], 0)])
        tx.wit.vtxinwit[0].scriptWitness.stack = [cisa_witness_element(aggsig[-64:], 0, CISA_MARKER_HALFAGG)]
        self.assert_reject(tx, "Invalid CISA aggregation group structure")

        # Nonce shares swapped between two members
        utxos = [self.fund_v2() for _ in range(3)]
        tx, spent = self.build_spend(utxos)
        self.sign_halfagg_group(tx, spent, [(0, utxos[0], 0), (1, utxos[1], 0), (2, utxos[2], 0)])
        wit0, wit1 = tx.wit.vtxinwit[0].scriptWitness.stack, tx.wit.vtxinwit[1].scriptWitness.stack
        tx.wit.vtxinwit[0].scriptWitness.stack, tx.wit.vtxinwit[1].scriptWitness.stack = wit1, wit0
        self.assert_reject(tx, "CISA aggregate signature verification failed")

        # Wrong aggregate signature (bit flipped in s)
        utxos = [self.fund_v2() for _ in range(2)]
        tx, spent = self.build_spend(utxos)
        aggsig = self.sign_halfagg_group(tx, spent, [(0, utxos[0], 0), (1, utxos[1], 0)])
        bad = bytearray(aggsig[-64:])
        bad[-1] ^= 0x01
        tx.wit.vtxinwit[1].scriptWitness.stack = [cisa_witness_element(bytes(bad), 0, CISA_MARKER_HALFAGG)]
        self.assert_reject(tx, "CISA aggregate signature verification failed")

    def generate_block_with_tx(self, tx, expect_valid):
        """Submit a block containing tx to both nodes and check the result."""
        self.sync_blocks()
        tip = self.nodes[0].getbestblockhash()
        height = self.nodes[0].getblockcount() + 1
        block_time = self.nodes[0].getblockheader(tip)["mediantime"] + 1
        block = create_block(int(tip, 16), create_coinbase(height), ntime=block_time, txlist=[tx])
        add_witness_commitment(block)
        block.solve()
        for node in self.nodes:
            result = node.submitblock(block.serialize().hex())
            if expect_valid:
                # The block may already have propagated from the other node.
                assert result in (None, "duplicate"), result
                assert_equal(node.getbestblockhash(), block.hash_hex)
            else:
                assert result is not None
                assert_equal(node.getbestblockhash(), tip)

    def test_invalid_block(self):
        self.log.info("Blocks with invalid CISA transactions are rejected by both validation modes")
        utxos = [self.fund_v2() for _ in range(2)]
        tx, spent = self.build_spend(utxos)
        aggsig = self.sign_halfagg_group(tx, spent, [(0, utxos[0], 0), (1, utxos[1], 0)])
        bad = bytearray(aggsig[-64:])
        bad[-1] ^= 0x01
        tx.wit.vtxinwit[1].scriptWitness.stack = [cisa_witness_element(bytes(bad), 0, CISA_MARKER_HALFAGG)]
        self.generate_block_with_tx(tx, expect_valid=False)

        # The same transaction with the correct aggregate signature is valid.
        tx.wit.vtxinwit[1].scriptWitness.stack = [cisa_witness_element(aggsig[-64:], 0, CISA_MARKER_HALFAGG)]
        self.generate_block_with_tx(tx, expect_valid=True)


if __name__ == '__main__':
    CISATest(__file__).main()
