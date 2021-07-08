#!/usr/bin/env python3
# Copyright (c) 2017-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool acceptance of raw transactions."""

from decimal import Decimal
import math

from test_framework.test_framework import SyscoinTestFramework
from test_framework.key import ECKey
from test_framework.messages import (
    BIP125_SEQUENCE_NUMBER,
    COIN,
    COutPoint,
    CTxIn,
    CTxOut,
    MAX_BLOCK_BASE_SIZE,
    MAX_MONEY,
    tx_from_hex,
)
from test_framework.script import (
    hash160,
    CScript,
    OP_0,
    OP_2,
    OP_3,
    OP_CHECKMULTISIG,
    OP_EQUAL,
    OP_HASH160,
    OP_RETURN,
)
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class MempoolAcceptanceTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[
            '-txindex','-permitbaremultisig=0',
        ]] * self.num_nodes
        self.supports_cli = False
        # SYSCOIN
        self.rpc_timeout = 480

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def check_mempool_result(self, result_expected, *args, **kwargs):
        """Wrapper to check result of testmempoolaccept on node_0's mempool"""
        result_test = self.nodes[0].testmempoolaccept(*args, **kwargs)
        for r in result_test:
            r.pop('wtxid')  # Skip check for now
        assert_equal(result_expected, result_test)
        assert_equal(self.nodes[0].getmempoolinfo()['size'], self.mempool_size)  # Must not change mempool state

    def run_test(self):
        node = self.nodes[0]

        self.log.info('Start with empty mempool, and 200 blocks')
        self.mempool_size = 0
        assert_equal(node.getblockcount(), 200)
        assert_equal(node.getmempoolinfo()['size'], self.mempool_size)
        coins = node.listunspent()

        self.log.info('Should not accept garbage to testmempoolaccept')
        assert_raises_rpc_error(-3, 'Expected type array, got string', lambda: node.testmempoolaccept(rawtxs='ff00baar'))
        assert_raises_rpc_error(-8, 'Array must contain between 1 and 25 transactions.', lambda: node.testmempoolaccept(rawtxs=['ff22']*26))
        assert_raises_rpc_error(-8, 'Array must contain between 1 and 25 transactions.', lambda: node.testmempoolaccept(rawtxs=[]))
        assert_raises_rpc_error(-22, 'TX decode failed', lambda: node.testmempoolaccept(rawtxs=['ff00baar']))

        self.log.info('A transaction already in the blockchain')
        coin = coins.pop()  # Pick a random coin(base) to spend
        raw_tx_in_block = node.signrawtransactionwithwallet(node.createrawtransaction(
            inputs=[{'txid': coin['txid'], 'vout': coin['vout']}],
            outputs=[{node.getnewaddress(): 0.3}, {node.getnewaddress(): 49}],
        ))['hex']
        txid_in_block = node.sendrawtransaction(hexstring=raw_tx_in_block, maxfeerate=0)
        node.generate(1)
        self.mempool_size = 0
        self.check_mempool_result(
            result_expected=[{'txid': txid_in_block, 'allowed': False, 'reject-reason': 'txn-already-known'}],
            rawtxs=[raw_tx_in_block],
        )

        self.log.info('A transaction not in the mempool')
        fee = Decimal('0.000007')
        raw_tx_0 = node.signrawtransactionwithwallet(node.createrawtransaction(
            inputs=[{"txid": txid_in_block, "vout": 0, "sequence": BIP125_SEQUENCE_NUMBER}],  # RBF is used later
            outputs=[{node.getnewaddress(): Decimal('0.3') - fee}],
        ))['hex']
        tx = tx_from_hex(raw_tx_0)
        txid_0 = tx.rehash()
        self.check_mempool_result(
            result_expected=[{'txid': txid_0, 'allowed': True, 'vsize': tx.get_vsize(), 'fees': {'base': fee}}],
            rawtxs=[raw_tx_0],
        )

        self.log.info('A final transaction not in the mempool')
        coin = coins.pop()  # Pick a random coin(base) to spend
        output_amount = Decimal('0.025')
        raw_tx_final = node.signrawtransactionwithwallet(node.createrawtransaction(
            inputs=[{'txid': coin['txid'], 'vout': coin['vout'], "sequence": 0xffffffff}],  # SEQUENCE_FINAL
            outputs=[{node.getnewaddress(): output_amount}],
            locktime=node.getblockcount() + 2000,  # Can be anything
        ))['hex']
        tx = tx_from_hex(raw_tx_final)
        fee_expected = coin['amount'] - output_amount
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': True, 'vsize': tx.get_vsize(), 'fees': {'base': fee_expected}}],
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
        tx.vin[0].nSequence = BIP125_SEQUENCE_NUMBER + 1  # Now, opt out of RBF
        raw_tx_0 = node.signrawtransactionwithwallet(tx.serialize().hex())['hex']
        tx = tx_from_hex(raw_tx_0)
        txid_0 = tx.rehash()
        self.check_mempool_result(
            result_expected=[{'txid': txid_0, 'allowed': True, 'vsize': tx.get_vsize(), 'fees': {'base': (2 * fee)}}],
            rawtxs=[raw_tx_0],
        )

        self.log.info('A transaction that conflicts with an unconfirmed tx')
        # Send the transaction that replaces the mempool transaction and opts out of replaceability
        node.sendrawtransaction(hexstring=tx.serialize().hex(), maxfeerate=0)
        # take original raw_tx_0
        tx = tx_from_hex(raw_tx_0)
        tx.vout[0].nValue -= int(4 * fee * COIN)  # Set more fee
        # skip re-signing the tx
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'txn-mempool-conflict'}],
            rawtxs=[tx.serialize().hex()],
            maxfeerate=0,
        )

        self.log.info('A transaction with missing inputs, that never existed')
        tx = tx_from_hex(raw_tx_0)
        tx.vin[0].prevout = COutPoint(hash=int('ff' * 32, 16), n=14)
        # skip re-signing the tx
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'missing-inputs'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction with missing inputs, that existed once in the past')
        tx = tx_from_hex(raw_tx_0)
        tx.vin[0].prevout.n = 1  # Set vout to 1, to spend the other outpoint (49 coins) of the in-chain-tx we want to double spend
        raw_tx_1 = node.signrawtransactionwithwallet(tx.serialize().hex())['hex']
        txid_1 = node.sendrawtransaction(hexstring=raw_tx_1, maxfeerate=0)
        # Now spend both to "clearly hide" the outputs, ie. remove the coins from the utxo set by spending them
        raw_tx_spend_both = node.signrawtransactionwithwallet(node.createrawtransaction(
            inputs=[
                {'txid': txid_0, 'vout': 0},
                {'txid': txid_1, 'vout': 0},
            ],
            outputs=[{node.getnewaddress(): 0.1}]
        ))['hex']
        txid_spend_both = node.sendrawtransaction(hexstring=raw_tx_spend_both, maxfeerate=0)
        node.generate(1)
        self.mempool_size = 0
        # Now see if we can add the coins back to the utxo set by sending the exact txs again
        self.check_mempool_result(
            result_expected=[{'txid': txid_0, 'allowed': False, 'reject-reason': 'missing-inputs'}],
            rawtxs=[raw_tx_0],
        )
        self.check_mempool_result(
            result_expected=[{'txid': txid_1, 'allowed': False, 'reject-reason': 'missing-inputs'}],
            rawtxs=[raw_tx_1],
        )

        self.log.info('Create a signed "reference" tx for later use')
        raw_tx_reference = node.signrawtransactionwithwallet(node.createrawtransaction(
            inputs=[{'txid': txid_spend_both, 'vout': 0}],
            outputs=[{node.getnewaddress(): 0.05}],
        ))['hex']
        tx = tx_from_hex(raw_tx_reference)
        # Reference tx should be valid on itself
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': True, 'vsize': tx.get_vsize(), 'fees': { 'base': Decimal('0.1') - Decimal('0.05')}}],
            rawtxs=[tx.serialize().hex()],
            maxfeerate=0,
        )

        self.log.info('A transaction with no outputs')
        tx = tx_from_hex(raw_tx_reference)
        tx.vout = []
        # Skip re-signing the transaction for context independent checks from now on
        # tx = tx_from_hex(node.signrawtransactionwithwallet(tx.serialize().hex())['hex'])
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'bad-txns-vout-empty'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A really large transaction')
        tx = tx_from_hex(raw_tx_reference)
        tx.vin = [tx.vin[0]] * math.ceil(MAX_BLOCK_BASE_SIZE / len(tx.vin[0].serialize()))
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

        # The following two validations prevent overflow of the output amounts (see CVE-2010-5139).
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
        # Pick the input of the first tx we signed, so it has to be a coinbase tx
        raw_tx_coinbase_spent = node.getrawtransaction(txid=node.decoderawtransaction(hexstring=raw_tx_in_block)['vin'][0]['txid'])
        tx = tx_from_hex(raw_tx_coinbase_spent)
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'coinbase'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('Some nonstandard transactions')
        tx = tx_from_hex(raw_tx_reference)
        tx.nVersion = 3  # A version currently non-standard
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
        key = ECKey()
        key.generate()
        pubkey = key.get_pubkey().get_bytes()
        tx.vout[0].scriptPubKey = CScript([OP_2, pubkey, pubkey, pubkey, OP_3, OP_CHECKMULTISIG])  # Some bare multisig script (2-of-3)
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'bare-multisig'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx = tx_from_hex(raw_tx_reference)
        tx.vin[0].scriptSig = CScript([OP_HASH160])  # Some not-pushonly scriptSig
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'scriptsig-not-pushonly'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx = tx_from_hex(raw_tx_reference)
        tx.vin[0].scriptSig = CScript([b'a' * 1648]) # Some too large scriptSig (>1650 bytes)
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'scriptsig-size'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx = tx_from_hex(raw_tx_reference)
        output_p2sh_burn = CTxOut(nValue=540, scriptPubKey=CScript([OP_HASH160, hash160(b'burn'), OP_EQUAL]))
        num_scripts = 100000 // len(output_p2sh_burn.serialize())  # Use enough outputs to make the tx too large for our policy
        tx.vout = [output_p2sh_burn] * num_scripts
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'tx-size'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx = tx_from_hex(raw_tx_reference)
        tx.vout[0] = output_p2sh_burn
        tx.vout[0].nValue -= 1  # Make output smaller, such that it is dust for our policy
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
        tx.vin[0].nSequence = 2  # We could include it in the second block mined from now, but not the very next one
        # Can skip re-signing the tx because of early rejection
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'non-BIP68-final'}],
            rawtxs=[tx.serialize().hex()],
            maxfeerate=0,
        )


if __name__ == '__main__':
    MempoolAcceptanceTest().main()
