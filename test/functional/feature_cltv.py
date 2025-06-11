#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test BIP65 (CHECKLOCKTIMEVERIFY).

Test that the CHECKLOCKTIMEVERIFY soft-fork activates.
"""

from test_framework.blocktools import (
    TIME_GENESIS_BLOCK,
    create_block,
    create_coinbase,
)
from test_framework.messages import (
    CTransaction,
    SEQUENCE_FINAL,
    msg_block,
)
from test_framework.p2p import P2PInterface
from test_framework.script import (
    CScript,
    CScriptNum,
    OP_1NEGATE,
    OP_CHECKLOCKTIMEVERIFY,
    OP_DROP,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)


# Helper function to modify a transaction by
# 1) prepending a given script to the scriptSig of vin 0 and
# 2) (optionally) modify the nSequence of vin 0 and the tx's nLockTime
def cltv_modify_tx(tx, prepend_scriptsig, nsequence=None, nlocktime=None):
    assert_equal(len(tx.vin), 1)
    if nsequence is not None:
        tx.vin[0].nSequence = nsequence
        tx.nLockTime = nlocktime

    tx.vin[0].scriptSig = CScript(prepend_scriptsig + list(CScript(tx.vin[0].scriptSig)))


def cltv_invalidate(tx, failure_reason):
    # Modify the signature in vin 0 and nSequence/nLockTime of the tx to fail CLTV
    #
    # According to BIP65, OP_CHECKLOCKTIMEVERIFY can fail due the following reasons:
    # 1) the stack is empty
    # 2) the top item on the stack is less than 0
    # 3) the lock-time type (height vs. timestamp) of the top stack item and the
    #    nLockTime field are not the same
    # 4) the top stack item is greater than the transaction's nLockTime field
    # 5) the nSequence field of the txin is 0xffffffff (SEQUENCE_FINAL)
    assert failure_reason in range(5)
    scheme = [
        # | Script to prepend to scriptSig                  | nSequence  | nLockTime    |
        # +-------------------------------------------------+------------+--------------+
        [[OP_CHECKLOCKTIMEVERIFY],                            None,       None],
        [[OP_1NEGATE, OP_CHECKLOCKTIMEVERIFY, OP_DROP],       None,       None],
        [[CScriptNum(100), OP_CHECKLOCKTIMEVERIFY, OP_DROP],  0,          TIME_GENESIS_BLOCK],
        [[CScriptNum(100), OP_CHECKLOCKTIMEVERIFY, OP_DROP],  0,          50],
        [[CScriptNum(50),  OP_CHECKLOCKTIMEVERIFY, OP_DROP],  SEQUENCE_FINAL, 50],
    ][failure_reason]

    cltv_modify_tx(tx, prepend_scriptsig=scheme[0], nsequence=scheme[1], nlocktime=scheme[2])


def cltv_validate(tx, height):
    # Modify the signature in vin 0 and nSequence/nLockTime of the tx to pass CLTV
    scheme = [[CScriptNum(height), OP_CHECKLOCKTIMEVERIFY, OP_DROP], 0, height]

    cltv_modify_tx(tx, prepend_scriptsig=scheme[0], nsequence=scheme[1], nlocktime=scheme[2])


CLTV_HEIGHT = 111


class BIP65Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True
        self.extra_args = [[
            f'-testactivationheight=cltv@{CLTV_HEIGHT}',
            '-acceptnonstdtxn=1',  # cltv_invalidate is nonstandard
        ]]
        self.setup_clean_chain = True
        self.rpc_timeout = 480

    def test_cltv_info(self, *, is_active):
        assert_equal(self.nodes[0].getdeploymentinfo()['deployments']['bip65'], {
                "active": is_active,
                "height": CLTV_HEIGHT,
                "type": "buried",
            },
        )

    def run_test(self):
        peer = self.nodes[0].add_p2p_connection(P2PInterface())
        wallet = MiniWallet(self.nodes[0], mode=MiniWalletMode.RAW_OP_TRUE)

        self.test_cltv_info(is_active=False)

        self.log.info("Mining %d blocks", CLTV_HEIGHT - 2)
        self.generate(wallet, 10)
        self.generate(self.nodes[0], CLTV_HEIGHT - 2 - 10)
        assert_equal(self.nodes[0].getblockcount(), CLTV_HEIGHT - 2)

        self.log.info("Test that invalid-according-to-CLTV transactions can still appear in a block")

        # create one invalid tx per CLTV failure reason (5 in total) and collect them
        invalid_cltv_txs = []
        for i in range(5):
            spendtx = wallet.create_self_transfer()['tx']
            cltv_invalidate(spendtx, i)
            invalid_cltv_txs.append(spendtx)

        tip = self.nodes[0].getbestblockhash()
        block_time = self.nodes[0].getblockheader(tip)['mediantime'] + 1
        block = create_block(int(tip, 16), create_coinbase(CLTV_HEIGHT - 1), block_time, version=3, txlist=invalid_cltv_txs)
        block.solve()

        self.test_cltv_info(is_active=False)  # Not active as of current tip and next block does not need to obey rules
        peer.send_and_ping(msg_block(block))
        self.test_cltv_info(is_active=True)  # Not active as of current tip, but next block must obey rules
        assert_equal(self.nodes[0].getbestblockhash(), block.hash)

        self.log.info("Test that blocks must now be at least version 4")
        tip = block.sha256
        block_time += 1
        block = create_block(tip, create_coinbase(CLTV_HEIGHT), block_time, version=3)
        block.solve()

        with self.nodes[0].assert_debug_log(expected_msgs=[f'{block.hash}, bad-version(0x00000003)']):
            peer.send_and_ping(msg_block(block))
            assert_equal(int(self.nodes[0].getbestblockhash(), 16), tip)
            peer.sync_with_ping()

        self.log.info("Test that invalid-according-to-CLTV transactions cannot appear in a block")
        block.nVersion = 4
        block.vtx.append(CTransaction()) # dummy tx after coinbase that will be replaced later

        # create and test one invalid tx per CLTV failure reason (5 in total)
        for i in range(5):
            spendtx = wallet.create_self_transfer()['tx']
            assert_equal(len(spendtx.vin), 1)
            coin = spendtx.vin[0]
            coin_txid = format(coin.prevout.hash, '064x')
            coin_vout = coin.prevout.n
            cltv_invalidate(spendtx, i)

            expected_cltv_reject_reason = [
                "mandatory-script-verify-flag-failed (Operation not valid with the current stack size)",
                "mandatory-script-verify-flag-failed (Negative locktime)",
                "mandatory-script-verify-flag-failed (Locktime requirement not satisfied)",
                "mandatory-script-verify-flag-failed (Locktime requirement not satisfied)",
                "mandatory-script-verify-flag-failed (Locktime requirement not satisfied)",
            ][i]
            # First we show that this tx is valid except for CLTV by getting it
            # rejected from the mempool for exactly that reason.
            spendtx_txid = spendtx.txid_hex
            spendtx_wtxid = spendtx.wtxid_hex
            assert_equal(
                [{
                    'txid': spendtx_txid,
                    'wtxid': spendtx_wtxid,
                    'allowed': False,
                    'reject-reason': expected_cltv_reject_reason,
                    'reject-details': expected_cltv_reject_reason + f", input 0 of {spendtx_txid} (wtxid {spendtx_wtxid}), spending {coin_txid}:{coin_vout}"
                }],
                self.nodes[0].testmempoolaccept(rawtxs=[spendtx.serialize().hex()], maxfeerate=0),
            )

            # Now we verify that a block with this transaction is also invalid.
            block.vtx[1] = spendtx
            block.hashMerkleRoot = block.calc_merkle_root()
            block.solve()

            with self.nodes[0].assert_debug_log(expected_msgs=[f'Block validation error: {expected_cltv_reject_reason}']):
                peer.send_and_ping(msg_block(block))
                assert_equal(int(self.nodes[0].getbestblockhash(), 16), tip)
                peer.sync_with_ping()

        self.log.info("Test that a version 4 block with a valid-according-to-CLTV transaction is accepted")
        cltv_validate(spendtx, CLTV_HEIGHT - 1)

        block.vtx.pop(1)
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()

        self.test_cltv_info(is_active=True)  # Not active as of current tip, but next block must obey rules
        peer.send_and_ping(msg_block(block))
        self.test_cltv_info(is_active=True)  # Active as of current tip
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.sha256)


if __name__ == '__main__':
    BIP65Test(__file__).main()
