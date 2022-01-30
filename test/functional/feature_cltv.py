#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test BIP65 (CHECKLOCKTIMEVERIFY).

Test that the CHECKLOCKTIMEVERIFY soft-fork activates at (regtest) block height
1351.
"""

from test_framework.blocktools import create_coinbase, create_block, create_transaction
from test_framework.messages import CTransaction, msg_block, ToHex
from test_framework.p2p import P2PInterface
from test_framework.script import CScript, OP_1NEGATE, OP_CHECKLOCKTIMEVERIFY, OP_DROP, CScriptNum
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    hex_str_to_bytes,
)

from io import BytesIO

CLTV_HEIGHT = 1351
VB_TOP_BITS = 0x20000000

def cltv_invalidate(tx):
    '''Modify the signature in vin 0 of the tx to fail CLTV

    Prepends -1 CLTV DROP in the scriptSig itself.

    TODO: test more ways that transactions using CLTV could be invalid (eg
    locktime requirements fail, sequence time requirements fail, etc).
    '''
    tx.vin[0].scriptSig = CScript([OP_1NEGATE, OP_CHECKLOCKTIMEVERIFY, OP_DROP] +
                                  list(CScript(tx.vin[0].scriptSig)))

def cltv_validate(node, tx, height):
    '''Modify the signature in vin 0 of the tx to pass CLTV
    Prepends <height> CLTV DROP in the scriptSig, and sets
    the locktime to height'''
    tx.vin[0].nSequence = 0
    tx.nLockTime = height

    # Need to re-sign, since nSequence and nLockTime changed
    signed_result = node.signrawtransactionwithwallet(ToHex(tx))
    new_tx = CTransaction()
    new_tx.deserialize(BytesIO(hex_str_to_bytes(signed_result['hex'])))

    new_tx.vin[0].scriptSig = CScript([CScriptNum(height), OP_CHECKLOCKTIMEVERIFY, OP_DROP] +
                                  list(CScript(new_tx.vin[0].scriptSig)))
    return new_tx


class BIP65Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[
            '-whitelist=noban@127.0.0.1',
            '-par=1',  # Use only one script thread to get the exact reject reason for testing
            '-acceptnonstdtxn=1',  # cltv_invalidate is nonstandard
            '-vbparams=mweb:-2:0',
        ]]
        self.setup_clean_chain = True
        self.rpc_timeout = 480

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_cltv_info(self, *, is_active):
        assert_equal(self.nodes[0].getblockchaininfo()['softforks']['bip65'],
            {
                "active": is_active,
                "height": CLTV_HEIGHT,
                "type": "buried",
            },
        )

    def run_test(self):
        peer = self.nodes[0].add_p2p_connection(P2PInterface())

        self.test_cltv_info(is_active=False)

        self.log.info("Mining %d blocks", CLTV_HEIGHT - 2)
        self.coinbase_txids = [self.nodes[0].getblock(b)['tx'][0] for b in self.nodes[0].generate(CLTV_HEIGHT - 2)]
        self.nodeaddress = self.nodes[0].getnewaddress()

        self.log.info("Test that an invalid-according-to-CLTV transaction can still appear in a block")

        spendtx = create_transaction(self.nodes[0], self.coinbase_txids[0],
                self.nodeaddress, amount=1.0)
        cltv_invalidate(spendtx)
        spendtx.rehash()

        tip = self.nodes[0].getbestblockhash()
        block_time = self.nodes[0].getblockheader(tip)['mediantime'] + 1
        block = create_block(int(tip, 16), create_coinbase(CLTV_HEIGHT - 1), block_time)
        block.nVersion = VB_TOP_BITS
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()

        self.test_cltv_info(is_active=False)  # Not active as of current tip and next block does not need to obey rules
        peer.send_and_ping(msg_block(block))
        self.test_cltv_info(is_active=True)  # Not active as of current tip, but next block must obey rules
        assert_equal(self.nodes[0].getbestblockhash(), block.hash)

        self.log.info("Test that blocks must now be at least version VB_TOP_BITS")
        tip = block.sha256
        block_time += 1
        block = create_block(tip, create_coinbase(CLTV_HEIGHT), block_time)
        block.nVersion = 3
        block.solve()

        with self.nodes[0].assert_debug_log(expected_msgs=['{}, bad-version(0x00000003)'.format(block.hash)]):
            peer.send_and_ping(msg_block(block))
            assert_equal(int(self.nodes[0].getbestblockhash(), 16), tip)
            peer.sync_with_ping()

        self.log.info("Test that invalid-according-to-cltv transactions cannot appear in a block")
        block.nVersion = VB_TOP_BITS

        spendtx = create_transaction(self.nodes[0], self.coinbase_txids[1],
                self.nodeaddress, amount=1.0)
        cltv_invalidate(spendtx)
        spendtx.rehash()

        # First we show that this tx is valid except for CLTV by getting it
        # rejected from the mempool for exactly that reason.
        assert_equal(
            [{'txid': spendtx.hash, 'allowed': False, 'reject-reason': 'non-mandatory-script-verify-flag (Negative locktime)'}],
            self.nodes[0].testmempoolaccept(rawtxs=[spendtx.serialize().hex()], maxfeerate=0)
        )

        # Now we verify that a block with this transaction is also invalid.
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()

        with self.nodes[0].assert_debug_log(expected_msgs=['CheckInputScripts on {} failed with non-mandatory-script-verify-flag (Negative locktime)'.format(block.vtx[-1].hash)]):
            peer.send_and_ping(msg_block(block))
            assert_equal(int(self.nodes[0].getbestblockhash(), 16), tip)
            peer.sync_with_ping()

        self.log.info("Test that a version VB_TOP_BITS block with a valid-according-to-CLTV transaction is accepted")
        spendtx = cltv_validate(self.nodes[0], spendtx, CLTV_HEIGHT - 1)
        spendtx.rehash()

        block.vtx.pop(1)
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()

        self.test_cltv_info(is_active=True)  # Not active as of current tip, but next block must obey rules
        peer.send_and_ping(msg_block(block))
        self.test_cltv_info(is_active=True)  # Active as of current tip
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.sha256)


if __name__ == '__main__':
    BIP65Test().main()
