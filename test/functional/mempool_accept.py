#!/usr/bin/env python3
# Copyright (c) 2017-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool acceptance of raw transactions."""

from io import BytesIO
import math

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import (
    BIP125_SEQUENCE_NUMBER,
    COIN,
    COutPoint,
    CTransaction,
    CTxOut,
    MAX_BLOCK_BASE_SIZE,
)
from test_framework.script import (
    hash160,
    CScript,
    OP_0,
    OP_EQUAL,
    OP_HASH160,
    OP_RETURN,
)
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    hex_str_to_bytes,
)


class MempoolAcceptanceTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[
            '-txindex',
            '-acceptnonstdtxn=0',  # Try to mimic main-net
        ]] * self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def check_mempool_result(self, result_expected, *args, **kwargs):
        """Wrapper to check result of testmempoolaccept on node_0's mempool"""
        result_test = self.nodes[0].testmempoolaccept(*args, **kwargs)
        assert_equal(result_expected, result_test)
        assert_equal(self.nodes[0].getmempoolinfo()['size'], self.mempool_size)  # Must not change mempool state

    def run_test(self):
        self.test_single()
        self.test_multiple()

    def test_multiple(self):
        node = self.nodes[0]
        coins = node.listunspent(query_options={'minimumAmount': 50})

        def create_sign(*, prevtxs=None, **kwargs):
            signed = node.signrawtransactionwithwallet(
                hexstring=node.createrawtransaction(
                    **kwargs,
                    replaceable=True,
                ),
                prevtxs=prevtxs,
            )
            assert signed['complete']
            tx = CTransaction()
            tx.deserialize(BytesIO(hex_str_to_bytes(signed['hex'])))
            tx.rehash()
            return tx

        def send(tx):
            return node.sendrawtransaction(
                hexstring=tx.serialize().hex(),
                maxfeerate=0,
            )

        self.log.info('Populate mempool with tx graph')
        coin_a = coins.pop()
        txid_a = send(
            create_sign(
                inputs=[{
                    'txid': coin_a['txid'],
                    'vout': coin_a['vout'],
                }],
                outputs={
                    node.getnewaddress(): 20,
                    node.getnewaddress(): 29,
                },
            ))
        coin_b = coins.pop()
        txid_b = send(
            create_sign(
                inputs=[{
                    'txid': coin_b['txid'],
                    'vout': coin_b['vout'],
                }],
                outputs={node.getnewaddress(): 49},
            ))
        tx_c = create_sign(
            inputs=[{
                'txid': txid_a,
                'vout': 0
            }, {
                'txid': txid_b,
                'vout': 0,
            }],
            outputs={node.getnewaddress(): 68},
        )
        txid_c = send(tx_c)

        tx_d = create_sign(
            inputs=[{
                'txid': txid_a,
                'vout': 1
            }],
            outputs={node.getnewaddress(): 19},
        )
        txid_d = send(tx_d)

        self.log.info('Create replacement transactions')
        tx_d_1 = create_sign(
            inputs=[{
                'txid': txid_a,
                'vout': 1
            }],
            outputs={node.getnewaddress(): 18},
        )
        tx_d_2 = create_sign(
            inputs=[{
                'txid': txid_a,
                'vout': 1
            }],
            outputs={node.getnewaddress(): 17},
        )
        tx_e = create_sign(
            inputs=[{
                'txid': txid_d,
                'vout': 0
            }, {
                'txid': txid_c,
                'vout': 0
            }],
            outputs={node.getnewaddress(): 76},
        )
        tx_e_2 = create_sign(
            inputs=[{
                'txid': tx_d_2.hash,
                'vout': 0
            }, {
                'txid': txid_c,
                'vout': 0
            }],
            outputs={node.getnewaddress(): 75},
            prevtxs=[{
                'txid': tx_d_2.hash,
                'vout': 0,
                'scriptPubKey': tx_d_2.vout[0].scriptPubKey.hex(),
                'amount': tx_d_2.vout[0].nValue / COIN,
            }],
        )
        tx_a_1 = create_sign(
            inputs=[{
                'txid': coin_a['txid'],
                'vout': coin_a['vout'],
            }],
            outputs={
                node.getnewaddress(): 1,
            },
        )
        tx_c_1 = create_sign(
            inputs=[{
                'txid': tx_a_1.hash,
                'vout': 0
            }, {
                'txid': txid_b,
                'vout': 0,
            }],
            outputs={node.getnewaddress(): 1},
            prevtxs=[{
                'txid': tx_a_1.hash,
                'vout': 0,
                'scriptPubKey': tx_a_1.vout[0].scriptPubKey.hex(),
                'amount': tx_a_1.vout[0].nValue / COIN,
            }],
        )

        assert_equal(
            node.testmempoolaccept(
                rawtxs=[
                    tx.serialize().hex() for tx in [
                        tx_d_1,
                        tx_d_2,
                        tx_e,
                        tx_e_2,
                        tx_e_2,
                        tx_d,
                        tx_d_1,
                        tx_a_1,
                        tx_c,
                        tx_c_1,
                    ]
                ],
                maxfeerate=0), [
                    {
                        'txid': tx_d_1.hash,
                        'allowed': True,
                    },
                    {
                        'txid': tx_d_2.hash,
                        'allowed': True,
                    },
                    {
                        'txid': tx_e.hash,
                        'allowed': False,
                        'reject-reason': 'missing-inputs',
                    },
                    {
                        'txid': tx_e_2.hash,
                        'allowed': True,
                    },
                    {
                        'txid': tx_e_2.hash,
                        'allowed': False,
                        'reject-reason': '18: txn-already-in-mempool',
                    },
                    {
                        'txid': txid_d,
                        'allowed': False,
                        'reject-reason': '66: insufficient fee',
                    },
                    {
                        'txid': tx_d_1.hash,
                        'allowed': False,
                        'reject-reason': '66: insufficient fee',
                    },
                    {
                        'txid': tx_a_1.hash,
                        'allowed': True,
                    },
                    {
                        'txid': txid_c,
                        'allowed': False,
                        'reject-reason': 'missing-inputs',
                    },
                    {
                        'txid': tx_c_1.hash,
                        'allowed': True,
                    },
                ])

    def test_single(self):
        node = self.nodes[0]

        self.log.info('Start with empty mempool, and 200 blocks')
        self.mempool_size = 0
        assert_equal(node.getblockcount(), 200)
        assert_equal(node.getmempoolinfo()['size'], self.mempool_size)
        coins = node.listunspent()

        self.log.info('Should not accept garbage to testmempoolaccept')
        assert_raises_rpc_error(-3, 'Expected type array, got string', lambda: node.testmempoolaccept(rawtxs='ff00'))
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
            result_expected=[{'txid': txid_in_block, 'allowed': False, 'reject-reason': '18: txn-already-known'}],
            rawtxs=[raw_tx_in_block],
        )

        self.log.info('A transaction not in the mempool')
        fee = 0.00000700
        raw_tx_0 = node.signrawtransactionwithwallet(node.createrawtransaction(
            inputs=[{"txid": txid_in_block, "vout": 0, "sequence": BIP125_SEQUENCE_NUMBER}],  # RBF is used later
            outputs=[{node.getnewaddress(): 0.3 - fee}],
        ))['hex']
        tx = CTransaction()
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_0)))
        txid_0 = tx.rehash()
        self.check_mempool_result(
            result_expected=[{'txid': txid_0, 'allowed': True}],
            rawtxs=[raw_tx_0],
        )

        self.log.info('A final transaction not in the mempool')
        coin = coins.pop()  # Pick a random coin(base) to spend
        raw_tx_final = node.signrawtransactionwithwallet(node.createrawtransaction(
            inputs=[{'txid': coin['txid'], 'vout': coin['vout'], "sequence": 0xffffffff}],  # SEQUENCE_FINAL
            outputs=[{node.getnewaddress(): 0.025}],
            locktime=node.getblockcount() + 2000,  # Can be anything
        ))['hex']
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_final)))
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': True}],
            rawtxs=[tx.serialize().hex()],
            maxfeerate=0,
        )
        node.sendrawtransaction(hexstring=raw_tx_final, maxfeerate=0)
        self.mempool_size += 1

        self.log.info('A transaction in the mempool')
        node.sendrawtransaction(hexstring=raw_tx_0)
        self.mempool_size += 1
        self.check_mempool_result(
            result_expected=[{'txid': txid_0, 'allowed': False, 'reject-reason': '18: txn-already-in-mempool'}],
            rawtxs=[raw_tx_0],
        )

        self.log.info('A transaction that replaces a mempool transaction')
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_0)))
        tx.vout[0].nValue -= int(fee * COIN)  # Double the fee
        tx.vin[0].nSequence = BIP125_SEQUENCE_NUMBER + 1  # Now, opt out of RBF
        raw_tx_0 = node.signrawtransactionwithwallet(tx.serialize().hex())['hex']
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_0)))
        txid_0 = tx.rehash()
        self.check_mempool_result(
            result_expected=[{'txid': txid_0, 'allowed': True}],
            rawtxs=[raw_tx_0],
        )

        self.log.info('A transaction that conflicts with an unconfirmed tx')
        # Send the transaction that replaces the mempool transaction and opts out of replaceability
        node.sendrawtransaction(hexstring=tx.serialize().hex(), maxfeerate=0)
        # take original raw_tx_0
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_0)))
        tx.vout[0].nValue -= int(4 * fee * COIN)  # Set more fee
        # skip re-signing the tx
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '18: txn-mempool-conflict'}],
            rawtxs=[tx.serialize().hex()],
            maxfeerate=0,
        )

        self.log.info('A transaction with missing inputs, that never existed')
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_0)))
        tx.vin[0].prevout = COutPoint(hash=int('ff' * 32, 16), n=14)
        # skip re-signing the tx
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': 'missing-inputs'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction with missing inputs, that existed once in the past')
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_0)))
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
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        # Reference tx should be valid on itself
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': True}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction with no outputs')
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.vout = []
        # Skip re-signing the transaction for context independent checks from now on
        # tx.deserialize(BytesIO(hex_str_to_bytes(node.signrawtransactionwithwallet(tx.serialize().hex())['hex'])))
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '16: bad-txns-vout-empty'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A really large transaction')
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.vin = [tx.vin[0]] * math.ceil(MAX_BLOCK_BASE_SIZE / len(tx.vin[0].serialize()))
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '16: bad-txns-oversize'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction with negative output value')
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.vout[0].nValue *= -1
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '16: bad-txns-vout-negative'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction with too large output value')
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.vout[0].nValue = 21000000 * COIN + 1
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '16: bad-txns-vout-toolarge'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction with too large sum of output values')
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.vout = [tx.vout[0]] * 2
        tx.vout[0].nValue = 21000000 * COIN
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '16: bad-txns-txouttotal-toolarge'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction with duplicate inputs')
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.vin = [tx.vin[0]] * 2
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '16: bad-txns-inputs-duplicate'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A coinbase transaction')
        # Pick the input of the first tx we signed, so it has to be a coinbase tx
        raw_tx_coinbase_spent = node.getrawtransaction(txid=node.decoderawtransaction(hexstring=raw_tx_in_block)['vin'][0]['txid'])
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_coinbase_spent)))
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '16: coinbase'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('Some nonstandard transactions')
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.nVersion = 3  # A version currently non-standard
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '64: version'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.vout[0].scriptPubKey = CScript([OP_0])  # Some non-standard script
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '64: scriptpubkey'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.vin[0].scriptSig = CScript([OP_HASH160])  # Some not-pushonly scriptSig
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '64: scriptsig-not-pushonly'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        output_p2sh_burn = CTxOut(nValue=540, scriptPubKey=CScript([OP_HASH160, hash160(b'burn'), OP_EQUAL]))
        num_scripts = 100000 // len(output_p2sh_burn.serialize())  # Use enough outputs to make the tx too large for our policy
        tx.vout = [output_p2sh_burn] * num_scripts
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '64: tx-size'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.vout[0] = output_p2sh_burn
        tx.vout[0].nValue -= 1  # Make output smaller, such that it is dust for our policy
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '64: dust'}],
            rawtxs=[tx.serialize().hex()],
        )
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.vout[0].scriptPubKey = CScript([OP_RETURN, b'\xff'])
        tx.vout = [tx.vout[0]] * 2
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '64: multi-op-return'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A timelocked transaction')
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.vin[0].nSequence -= 1  # Should be non-max, so locktime is not ignored
        tx.nLockTime = node.getblockcount() + 1
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '64: non-final'}],
            rawtxs=[tx.serialize().hex()],
        )

        self.log.info('A transaction that is locked by BIP68 sequence logic')
        tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx_reference)))
        tx.vin[0].nSequence = 2  # We could include it in the second block mined from now, but not the very next one
        # Can skip re-signing the tx because of early rejection
        self.check_mempool_result(
            result_expected=[{'txid': tx.rehash(), 'allowed': False, 'reject-reason': '64: non-BIP68-final'}],
            rawtxs=[tx.serialize().hex()],
            maxfeerate=0,
        )


if __name__ == '__main__':
    MempoolAcceptanceTest().main()
