#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool acceptance of raw transactions."""

from copy import deepcopy
from decimal import Decimal
import math

from test_framework.messages import (
    COIN,
    MAX_BIP125_RBF_SEQUENCE,
    MAX_BLOCK_WEIGHT,
    MAX_MONEY,
    SEQUENCE_FINAL,
    WITNESS_SCALE_FACTOR,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    tx_from_hex,
)
from test_framework.script import (
    CScript,
    OP_0,
    OP_HASH160,
    OP_RETURN,
    OP_TRUE,
    SIGHASH_ALL,
    sign_input_legacy,
)
from test_framework.script_util import (
    DUMMY_MIN_OP_RETURN_SCRIPT,
    MIN_PADDING,
    MIN_STANDARD_TX_NONWITNESS_SIZE,
    PAY_TO_ANCHOR,
    keys_to_multisig_script,
    script_to_p2sh_script,
    script_to_p2wsh_script,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than, assert_raises_rpc_error
from test_framework.wallet import MiniWallet
from test_framework.wallet_util import generate_keypair


class MempoolAcceptanceTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [
            [
                '-txindex',
                '-permitbaremultisig=0',
                '-mempoolfullrbf=0',
            ]
        ] * self.num_nodes
        self.supports_cli = False

    def check_mempool_result(self, result_expected, *args, **kwargs):
        """Check the result of testmempoolaccept on node_0's mempool."""
        test_result = self.nodes[0].testmempoolaccept(*args, **kwargs)
        for r in test_result:
            r.pop('wtxid', None)
            if "fees" in r:
                r["fees"].pop("effective-feerate", None)
                r["fees"].pop("effective-includes", None)
        assert_equal(result_expected, test_result)
        # Must not change mempool state
        assert_equal(self.nodes[0].getmempoolinfo()['size'], self.mempool_size)

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)

        self.log.info('Start with empty mempool, and 200 blocks')
        self.mempool_size = 0
        assert_equal(node.getblockcount(), 200)
        assert_equal(node.getmempoolinfo()['size'], self.mempool_size)

        self.log.info('Should not accept garbage to testmempoolaccept')
        assert_raises_rpc_error(
            -3,
            'JSON value of type string is not of expected type array',
            lambda: node.testmempoolaccept(rawtxs='ff00baar')
        )
        assert_raises_rpc_error(
            -8,
            'Array must contain between 1 and 25 transactions.',
            lambda: node.testmempoolaccept(rawtxs=['ff22'] * 26)
        )
        assert_raises_rpc_error(
            -8,
            'Array must contain between 1 and 25 transactions.',
            lambda: node.testmempoolaccept(rawtxs=[])
        )
        assert_raises_rpc_error(
            -22,
            'TX decode failed',
            lambda: node.testmempoolaccept(rawtxs=['ff00baar'])
        )

        self.log.info('A transaction already in the blockchain')
        tx = self.wallet.create_self_transfer()['tx']  # Pick a random coinbase to spend
        tx.vout.append(deepcopy(tx.vout[0]))
        tx.vout[0].nValue = int(0.3 * COIN)
        tx.vout[1].nValue = int(49 * COIN)
        raw_tx_in_block = tx.serialize().hex()
        txid_in_block = self.wallet.sendrawtransaction(from_node=node, tx_hex=raw_tx_in_block)
        self.generate(node, 1)
        self.mempool_size = 0

        # Also check feerate. 1BTC/kvB fails
        assert_raises_rpc_error(
            -8,
            "Fee rates larger than or equal to 1BTC/kvB are not accepted",
            lambda: self.check_mempool_result(
                result_expected=None,
                rawtxs=[raw_tx_in_block],
                maxfeerate=1,
            )
        )
        # Check negative feerate
        assert_raises_rpc_error(
            -3,
            "Amount out of range",
            lambda: self.check_mempool_result(
                result_expected=None,
                rawtxs=[raw_tx_in_block],
                maxfeerate=-0.01,
            )
        )
        # ... 0.99 passes
        self.check_mempool_result(
            result_expected=[{'txid': txid_in_block, 'allowed': False, 'reject-reason': 'txn-already-known'}],
            rawtxs=[raw_tx_in_block],
            maxfeerate=0.99,
        )

        self.log.info('A transaction not in the mempool')
        fee = Decimal('0.000007')
        utxo_to_spend = self.wallet.get_utxo(txid=txid_in_block)  # use 0.3 BTC UTXO
        tx = self.wallet.create_self_transfer(utxo_to_spend=utxo_to_spend, sequence=MAX_BIP125_RBF_SEQUENCE)['tx']
        tx.vout[0].nValue = int((Decimal('0.3') - fee) * COIN)
        raw_tx_0 = tx.serialize().hex()
        txid_0 = tx.rehash()
        self.check_mempool_result(
            result_expected=[{'txid': txid_0, 'allowed': True, 'vsize': tx.get_vsize(), 'fees': {'base': fee}}],
            rawtxs=[raw_tx_0],
        )

        self.log.info('A final transaction not in the mempool')
        output_amount = Decimal('0.025')
        tx = self.wallet.create_self_transfer(
            sequence=SEQUENCE_FINAL,
            locktime=node.getblockcount() + 2000,  # Can be anything
        )['tx']
        tx.vout[0].nValue = int(output_amount * COIN)
        raw_tx_final = tx.serialize().hex()
        tx = tx_from_hex(raw_tx_final)
        fee_expected = Decimal('50.0') - output_amount
        self.check_mempool_result(
            result_expected=[{
                'txid': tx.rehash(),
                'allowed': True,
                'vsize': tx.get_vsize(),
                'fees': {'base': fee_expected}
            }],
            rawtxs=[tx.serialize().hex()],
            maxfeerate=0,
        )
        node.sendrawtransaction(hexstring=raw_tx_final, maxfeerate=0)
        self.mempool_size += 1

        self.log.info('A transaction in the mempool')
        node.sendrawtransaction(hexstring=raw_tx_0)
        self.mempool_size += 1
        self.check_mempool_result(
            result_expected=[{'txid': txid_0, 'allowed': False, 'reject-reason': 'txn-already-in-mempool'}],
            rawtxs=[raw_tx_0],
        )

        self.log.info('A transaction that replaces a mempool transaction')
        tx = tx_from_hex(raw_tx_0)
        tx.vout[0].nValue -= int(fee * COIN)  # Double the fee
        tx.vin[0].nSequence = MAX_BIP125_RBF_SEQUENCE + 1  # Now, opt out of RBF
        raw_tx_0 = tx.serialize().hex()
        txid_0 = tx.rehash()
        self.check_mempool_result(
            result_expected=[{'txid': txid_0, 'allowed': True, 'vsize': tx.get_vsize(), 'fees': {'base': (2 * fee)}}],
            rawtxs=[raw_tx_0],
        )

        self.log.info('A transaction that conflicts with an unconfirmed tx')
        # Send the transaction that replaces the mempool transaction and opts out of replaceability
        node.sendrawtransaction(hexstring=tx.serialize().hex(), maxfeerate=0)
        # Take original raw_tx_0
        tx = tx_from_hex(raw_tx_0)
        tx.vout[0].nValue -= int(4 * fee * COIN)  # Set more fee
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'txn-mempool-conflict'}],
            rawtxs=[tx.serialize().hex()],
            maxfeerate=0,
        )

        self.log.info('A transaction with missing inputs, that never existed')
        tx = tx_from_hex(raw_tx_0)
        tx.vin[0].prevout = COutPoint(hash=int('ff' * 32, 16), n=14)
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'missing-inputs'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction with missing inputs, that existed once in the past')
        tx = tx_from_hex(raw_tx_0)
        tx.vin[0].prevout.n = 1  # Set vout to 1 to spend the other outpoint
        raw_tx_1 = tx.serialize().hex()
        txid_1 = node.sendrawtransaction(hexstring=raw_tx_1, maxfeerate=0)
        # Now spend both to remove the coins from the UTXO set
        tx = self.wallet.create_self_transfer()['tx']
        tx.vin.append(deepcopy(tx.vin[0]))
        tx.wit.vtxinwit.append(deepcopy(tx.wit.vtxinwit[0]))
        tx.vin[0].prevout = COutPoint(hash=int(txid_0, 16), n=0)
        tx.vin[1].prevout = COutPoint(hash=int(txid_1, 16), n=0)
        tx.vout[0].nValue = int(0.1 * COIN)
        raw_tx_spend_both = tx.serialize().hex()
        txid_spend_both = self.wallet.sendrawtransaction(from_node=node, tx_hex=raw_tx_spend_both)
        self.generate(node, 1)
        self.mempool_size = 0
        # Now see if we can add the coins back to the UTXO set by sending the exact txs again
        self.check_mempool_result(
            result_expected=[{'txid': txid_0, 'allowed': False, 'reject-reason': 'missing-inputs'}],
            rawtxs=[raw_tx_0],
        )
        self.check_mempool_result(
            result_expected=[{'txid': txid_1, 'allowed': False, 'reject-reason': 'missing-inputs'}],
            rawtxs=[raw_tx_1],
        )

        self.log.info('Create a "reference" tx for later use')
        utxo_to_spend = self.wallet.get_utxo(txid=txid_spend_both)
        tx = self.wallet.create_self_transfer(utxo_to_spend=utxo_to_spend, sequence=SEQUENCE_FINAL)['tx']
        tx.vout[0].nValue = int(0.05 * COIN)
        raw_tx_reference = tx.serialize().hex()
        # Reference tx should be valid on itself
        self.check_mempool_result(
            result_expected=[{
                'txid': tx.rehash(),
                'allowed': True,
                'vsize': tx.get_vsize(),
                'fees': {'base': Decimal('0.1') - Decimal('0.05')}
            }],
            rawtxs=[tx.serialize().hex()],
            maxfeerate=0,
        )

        self.log.info('A transaction with no outputs')
        tx = tx_from_hex(raw_tx_reference)
        tx.vout = []
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'bad-txns-vout-empty'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A really large transaction')
        tx = tx_from_hex(raw_tx_reference)
        max_vin = math.ceil((MAX_BLOCK_WEIGHT // WITNESS_SCALE_FACTOR) / len(tx.vin[0].serialize()))
        tx.vin = [tx.vin[0]] * max_vin
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'bad-txns-oversize'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction with negative output value')
        tx = tx_from_hex(raw_tx_reference)
        tx.vout[0].nValue *= -1
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'bad-txns-vout-negative'}],
            rawtxs=[tx.serialize().hex()],
        )

        # Prevent overflow of the output amounts
        self.log.info('A transaction with too large output value')
        tx = tx_from_hex(raw_tx_reference)
        tx.vout[0].nValue = MAX_MONEY + 1
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'bad-txns-vout-toolarge'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction with too large sum of output values')
        tx = tx_from_hex(raw_tx_reference)
        tx.vout = [tx.vout[0]] * 2
        tx.vout[0].nValue = MAX_MONEY
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'bad-txns-txouttotal-toolarge'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction with duplicate inputs')
        tx = tx_from_hex(raw_tx_reference)
        tx.vin = [tx.vin[0]] * 2
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'bad-txns-inputs-duplicate'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A non-coinbase transaction with coinbase-like outpoint')
        tx = tx_from_hex(raw_tx_reference)
        tx.vin.append(CTxIn(COutPoint(hash=0, n=0xffffffff)))
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'bad-txns-prevout-null'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A coinbase transaction')
        raw_tx_coinbase_spent = node.getrawtransaction(
            txid=node.decoderawtransaction(hexstring=raw_tx_in_block)['vin'][0]['txid']
        )
        tx = tx_from_hex(raw_tx_coinbase_spent)
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'coinbase'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('Some nonstandard transactions')
        tx = tx_from_hex(raw_tx_reference)
        tx.version = 4  # A version currently non-standard
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'version'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx = tx_from_hex(raw_tx_reference)
        tx.vout[0].scriptPubKey = CScript([OP_0])  # Some non-standard script
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'scriptpubkey'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx = tx_from_hex(raw_tx_reference)
        _, pubkey = generate_keypair()
        tx.vout[0].scriptPubKey = keys_to_multisig_script([pubkey] * 3, k=2)  # Bare multisig script (2-of-3)
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'bare-multisig'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx = tx_from_hex(raw_tx_reference)
        tx.vin[0].scriptSig = CScript([OP_HASH160])  # Not push-only scriptSig
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'scriptsig-not-pushonly'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx = tx_from_hex(raw_tx_reference)
        tx.vin[0].scriptSig = CScript([b'a' * 1648])  # Too large scriptSig (>1650 bytes)
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'scriptsig-size'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx = tx_from_hex(raw_tx_reference)
        output_p2sh_burn = CTxOut(nValue=540, scriptPubKey=script_to_p2sh_script(b'burn'))
        num_scripts = 100000 // len(output_p2sh_burn.serialize())  # Enough outputs to make tx too large
        tx.vout = [output_p2sh_burn] * num_scripts
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'tx-size'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx = tx_from_hex(raw_tx_reference)
        tx.vout[0] = output_p2sh_burn
        tx.vout[0].nValue -= 1  # Make output smaller, such that it is dust
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'dust'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx = tx_from_hex(raw_tx_reference)
        tx.vout[0].scriptPubKey = CScript([OP_RETURN, b'\xff'])
        tx.vout = [tx.vout[0]] * 2
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'multi-op-return'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A timelocked transaction')
        tx = tx_from_hex(raw_tx_reference)
        tx.vin[0].nSequence -= 1  # Should be non-max, so locktime is not ignored
        tx.nLockTime = node.getblockcount() + 1
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'non-final'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction that is locked by BIP68 sequence logic')
        tx = tx_from_hex(raw_tx_reference)
        tx.vin[0].nSequence = 2  # Can be included in the second block mined from now
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'non-BIP68-final'}],
            rawtxs=[tx.serialize().hex()],
            maxfeerate=0,
        )

        # Prep for tiny-tx tests with wsh(OP_TRUE) output
        seed_tx = self.wallet.send_to(from_node=node, scriptPubKey=script_to_p2wsh_script(CScript([OP_TRUE])), amount=COIN)
        self.generate(node, 1)

        self.log.info('A tiny transaction (in non-witness bytes) that is disallowed')
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(int(seed_tx["txid"], 16), seed_tx["sent_vout"]), b"", SEQUENCE_FINAL))
        tx.wit.vtxinwit = [CTxInWitness()]
        tx.wit.vtxinwit[0].scriptWitness.stack = [CScript([OP_TRUE])]
        tx.vout.append(CTxOut(0, CScript([OP_RETURN] + ([OP_0] * (MIN_PADDING - 2)))))
        # Only non-witness size matters
        assert_equal(len(tx.serialize_without_witness()), 64)
        assert_equal(MIN_STANDARD_TX_NONWITNESS_SIZE - 1, 64)
        assert_greater_than(len(tx.serialize()), 64)

        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'tx-size-small'}],
            rawtxs=[tx.serialize().hex()],
            maxfeerate=0,
        )

        self.log.info('Minimally-small transaction (in non-witness bytes) that is allowed')
        tx.vout[0] = CTxOut(COIN - 1000, DUMMY_MIN_OP_RETURN_SCRIPT)
        assert_equal(len(tx.serialize_without_witness()), MIN_STANDARD_TX_NONWITNESS_SIZE)
        self.check_mempool_result(
            result_expected=[{
                'txid': tx.rehash(),
                'allowed': True,
                'vsize': tx.get_vsize(),
                'fees': {'base': Decimal('0.00001000')}
            }],
            rawtxs=[tx.serialize().hex()],
            maxfeerate=0,
        )

        self.log.info('OP_1 <0x4e73> is able to be created and spent')
        anchor_value = 10000
        create_anchor_tx = self.wallet.send_to(from_node=node, scriptPubKey=PAY_TO_ANCHOR, amount=anchor_value)
        self.generate(node, 1)

        # First spend has non-empty witness, will be rejected to prevent third-party wtxid malleability
        anchor_nonempty_wit_spend = CTransaction()
        anchor_nonempty_wit_spend.vin.append(
            CTxIn(COutPoint(int(create_anchor_tx["txid"], 16), create_anchor_tx["sent_vout"]), b"")
        )
        anchor_nonempty_wit_spend.vout.append(
            CTxOut(anchor_value - int(fee * COIN), script_to_p2wsh_script(CScript([OP_TRUE])))
        )
        anchor_nonempty_wit_spend.wit.vtxinwit.append(CTxInWitness())
        anchor_nonempty_wit_spend.wit.vtxinwit[0].scriptWitness.stack.append(b"f")
        anchor_nonempty_wit_spend.rehash()

        self.check_mempool_result(
            result_expected=[{
                'txid': anchor_nonempty_wit_spend.rehash(),
                'allowed': False,
                'reject-reason': 'bad-witness-nonstandard'
            }],
            rawtxs=[anchor_nonempty_wit_spend.serialize().hex()],
            maxfeerate=0,
        )

        # But is consensus-legal
        self.generateblock(node, self.wallet.get_address(), [anchor_nonempty_wit_spend.serialize().hex()])

        # Without witness elements it is standard
        create_anchor_tx = self.wallet.send_to(from_node=node, scriptPubKey=PAY_TO_ANCHOR, amount=anchor_value)
        self.generate(node, 1)

        anchor_spend = CTransaction()
        anchor_spend.vin.append(
            CTxIn(COutPoint(int(create_anchor_tx["txid"], 16), create_anchor_tx["sent_vout"]), b"")
        )
        anchor_spend.vout.append(
            CTxOut(anchor_value - int(fee * COIN), script_to_p2wsh_script(CScript([OP_TRUE])))
        )
        anchor_spend.wit.vtxinwit.append(CTxInWitness())
        # It's "segwit" but txid == wtxid since there is no witness data
        assert_equal(anchor_spend.rehash(), anchor_spend.getwtxid())

        self.check_mempool_result(
            result_expected=[{
                'txid': anchor_spend.rehash(),
                'allowed': True,
                'vsize': anchor_spend.get_vsize(),
                'fees': {'base': Decimal('0.00000700')}
            }],
            rawtxs=[anchor_spend.serialize().hex()],
            maxfeerate=0,
        )

        self.log.info('But cannot be spent if nested sh()')
        nested_anchor_tx = self.wallet.create_self_transfer(sequence=SEQUENCE_FINAL)['tx']
        nested_anchor_tx.vout[0].scriptPubKey = script_to_p2sh_script(PAY_TO_ANCHOR)
        nested_anchor_tx.rehash()
        self.generateblock(node, self.wallet.get_address(), [nested_anchor_tx.serialize().hex()])

        nested_anchor_spend = CTransaction()
        nested_anchor_spend.vin.append(CTxIn(COutPoint(nested_anchor_tx.sha256, 0), b""))
        nested_anchor_spend.vin[0].scriptSig = CScript([bytes(PAY_TO_ANCHOR)])
        nested_anchor_spend.vout.append(
            CTxOut(nested_anchor_tx.vout[0].nValue - int(fee * COIN), script_to_p2wsh_script(CScript([OP_TRUE])))
        )
        nested_anchor_spend.rehash()

        self.check_mempool_result(
            result_expected=[{
                'txid': nested_anchor_spend.rehash(),
                'allowed': False,
                'reject-reason': 'non-mandatory-script-verify-flag (Witness version reserved for soft-fork upgrades)'
            }],
            rawtxs=[nested_anchor_spend.serialize().hex()],
            maxfeerate=0,
        )
        # But is consensus-legal
        self.generateblock(node, self.wallet.get_address(), [nested_anchor_spend.serialize().hex()])

        self.log.info('Spending a confirmed bare multisig is okay')
        address = self.wallet.get_address()
        tx = tx_from_hex(raw_tx_reference)
        privkey, pubkey = generate_keypair()
        tx.vout[0].scriptPubKey = keys_to_multisig_script([pubkey] * 3, k=1)  # Bare multisig script (1-of-3)
        tx.rehash()
        self.generateblock(node, address, [tx.serialize().hex()])
        tx_spend = CTransaction()
        tx_spend.vin.append(CTxIn(COutPoint(tx.sha256, 0), b""))
        tx_spend.vout.append(
            CTxOut(tx.vout[0].nValue - int(fee * COIN), script_to_p2wsh_script(CScript([OP_TRUE])))
        )
        tx_spend.rehash()
        sign_input_legacy(tx_spend, 0, tx.vout[0].scriptPubKey, privkey, sighash_type=SIGHASH_ALL)
        tx_spend.vin[0].scriptSig = bytes(CScript([OP_0])) + tx_spend.vin[0].scriptSig
        self.check_mempool_result(
            result_expected=[{
                'txid': tx_spend.rehash(),
                'allowed': True,
                'vsize': tx_spend.get_vsize(),
                'fees': {'base': Decimal('0.00000700')}
            }],
            rawtxs=[tx_spend.serialize().hex()],
            maxfeerate=0,
        )


if __name__ == '__main__':
    MempoolAcceptanceTest().main()
