#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test BIP65 (CHECKLOCKTIMEVERIFY).

Test that the CHECKLOCKTIMEVERIFY soft-fork activates at (regtest) block height
1351.
"""

from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.messages import (
    CTransaction,
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
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)

CLTV_HEIGHT = 1351


# Helper function to modify a transaction by
# 1) prepending a given script to the scriptSig of vin 0 and
# 2) (optionally) modify the nSequence of vin 0 and the tx's nLockTime
def cltv_modify_tx(tx, prepend_scriptsig, nsequence=None, nlocktime=None):
    assert_equal(len(tx.vin), 1)
    if nsequence is not None:
        tx.vin[0].nSequence = nsequence
        tx.nLockTime = nlocktime

    tx.vin[0].scriptSig = CScript(prepend_scriptsig + list(CScript(tx.vin[0].scriptSig)))
    tx.rehash()


def cltv_invalidate(tx, failure_reason):
    # Modify the signature in vin 0 and nSequence/nLockTime of the tx to fail CLTV
    #
    # According to BIP65, OP_CHECKLOCKTIMEVERIFY can fail due the following reasons:
    # 1) the stack is empty
    # 2) the top item on the stack is less than 0
    # 3) the lock-time type (height vs. timestamp) of the top stack item and the
    #    nLockTime field are not the same
    # 4) the top stack item is greater than the transaction's nLockTime field
    # 5) the nSequence field of the txin is 0xffffffff
    assert failure_reason in range(5)
    scheme = [
        # | Script to prepend to scriptSig                  | nSequence  | nLockTime    |
        # +-------------------------------------------------+------------+--------------+
        [[OP_CHECKLOCKTIMEVERIFY],                            None,       None],
        [[OP_1NEGATE, OP_CHECKLOCKTIMEVERIFY, OP_DROP],       None,       None],
        [[CScriptNum(1000), OP_CHECKLOCKTIMEVERIFY, OP_DROP], 0,          1296688602],  # timestamp of genesis block
        [[CScriptNum(1000), OP_CHECKLOCKTIMEVERIFY, OP_DROP], 0,          500],
        [[CScriptNum(500),  OP_CHECKLOCKTIMEVERIFY, OP_DROP], 0xffffffff, 500],
    ][failure_reason]

    cltv_modify_tx(tx, prepend_scriptsig=scheme[0], nsequence=scheme[1], nlocktime=scheme[2])


def cltv_validate(tx, height):
    # Modify the signature in vin 0 and nSequence/nLockTime of the tx to pass CLTV
    scheme = [[CScriptNum(height), OP_CHECKLOCKTIMEVERIFY, OP_DROP], 0, height]

    cltv_modify_tx(tx, prepend_scriptsig=scheme[0], nsequence=scheme[1], nlocktime=scheme[2])


class BIP65Test(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[
            '-whitelist=noban@127.0.0.1',
            '-par=1',  # Use only one script thread to get the exact reject reason for testing
            '-acceptnonstdtxn=1',  # cltv_invalidate is nonstandard
        ]]
        self.setup_clean_chain = True
        self.rpc_timeout = 480

    def test_cltv_info(self, *, is_active):
        assert_equal(self.nodes[0].getblockchaininfo()['softforks']['bip65'], {
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
        wallet.generate(10)
        self.nodes[0].generate(CLTV_HEIGHT - 2 - 10)

        self.log.info("Test that invalid-according-to-CLTV transactions can still appear in a block")

        # create one invalid tx per CLTV failure reason (5 in total) and collect them
        invalid_cltv_txs = []
        for i in range(5):
            spendtx = wallet.create_self_transfer(from_node=self.nodes[0])['tx']
            cltv_invalidate(spendtx, i)
            invalid_cltv_txs.append(spendtx)

        tip = self.nodes[0].getbestblockhash()
        block_time = self.nodes[0].getblockheader(tip)['mediantime'] + 1

        block = create_block(int(tip, 16), create_coinbase(CLTV_HEIGHT - 1), block_time)
        block.nVersion = 3
        block.vtx.extend(invalid_cltv_txs)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()

        self.test_cltv_info(is_active=False)  # Not active as of current tip and next block does not need to obey rules
        peer.send_and_ping(msg_block(block))
        self.test_cltv_info(is_active=True)  # Not active as of current tip, but next block must obey rules
        assert_equal(self.nodes[0].getbestblockhash(), block.hash)

        self.log.info("Test that blocks must now be at least version 4")
        tip = block.sha256
        block_time += 1
        block = create_block(tip, create_coinbase(CLTV_HEIGHT), block_time)
        block.nVersion = 3
        block.solve()
        with self.nodes[0].assert_debug_log(expected_msgs=['{}, bad-version(0x00000003)'.format(block.hash)]):
            peer.send_and_ping(msg_block(block))
            assert_equal(int(self.nodes[0].getbestblockhash(), 16), tip)
            peer.sync_with_ping()

        self.log.info("Test that invalid-according-to-CLTV transactions cannot appear in a block")
        block.nVersion = 4
        block.vtx.append(CTransaction()) # dummy tx after coinbase that will be replaced later

        # create and test one invalid tx per CLTV failure reason (5 in total)
        for i in range(5):
            spendtx = wallet.create_self_transfer(from_node=self.nodes[0])['tx']
            cltv_invalidate(spendtx, i)

            expected_cltv_reject_reason = [
                "non-mandatory-script-verify-flag (Operation not valid with the current stack size)",
                "non-mandatory-script-verify-flag (Negative locktime)",
                "non-mandatory-script-verify-flag (Locktime requirement not satisfied)",
                "non-mandatory-script-verify-flag (Locktime requirement not satisfied)",
                "non-mandatory-script-verify-flag (Locktime requirement not satisfied)",
            ][i]
            # First we show that this tx is valid except for CLTV by getting it
            # rejected from the mempool for exactly that reason.
            assert_equal(
                [{
                    'txid': spendtx.hash,
                    'wtxid': spendtx.getwtxid(),
                    'allowed': False,
                    'reject-reason': expected_cltv_reject_reason,
                }],
                self.nodes[0].testmempoolaccept(rawtxs=[spendtx.serialize().hex()], maxfeerate=0),
            )

            # Now we verify that a block with this transaction is also invalid.
            block.vtx[1] = spendtx
            block.hashMerkleRoot = block.calc_merkle_root()
            block.solve()

            with self.nodes[0].assert_debug_log(expected_msgs=['CheckInputScripts on {} failed with {}'.format(
                                                block.vtx[-1].hash, expected_cltv_reject_reason)]):
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
    BIP65Test().main()
