#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test BIP66 (DER SIG).

Test the DERSIG soft-fork activation on regtest.
"""

from test_framework.blocktools import (
    DERSIG_HEIGHT,
    create_block,
    create_coinbase,
)
from test_framework.messages import msg_block
from test_framework.p2p import P2PInterface
from test_framework.script import CScript
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)


# A canonical signature consists of:
# <30> <total len> <02> <len R> <R> <02> <len S> <S> <hashtype>
def unDERify(tx):
    """
    Make the signature in vin 0 of a tx non-DER-compliant,
    by adding padding after the S-value.
    """
    scriptSig = CScript(tx.vin[0].scriptSig)
    newscript = []
    for i in scriptSig:
        if (len(newscript) == 0):
            newscript.append(i[0:-1] + b'\0' + i[-1:])
        else:
            newscript.append(i)
    tx.vin[0].scriptSig = CScript(newscript)


class BIP66Test(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[
            '-whitelist=noban@127.0.0.1',
            '-par=1',  # Use only one script thread to get the exact log msg for testing
        ]]
        self.setup_clean_chain = True
        self.rpc_timeout = 240

    def create_tx(self, input_txid):
        utxo_to_spend = self.miniwallet.get_utxo(txid=input_txid, mark_as_spent=False)
        return self.miniwallet.create_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo_to_spend)['tx']

    def test_dersig_info(self, *, is_active):
        assert_equal(self.nodes[0].getblockchaininfo()['softforks']['bip66'],
            {
                "active": is_active,
                "height": DERSIG_HEIGHT,
                "type": "buried",
            },
        )

    def run_test(self):
        peer = self.nodes[0].add_p2p_connection(P2PInterface())
        self.miniwallet = MiniWallet(self.nodes[0], mode=MiniWalletMode.RAW_P2PK)

        self.test_dersig_info(is_active=False)

        self.log.info("Mining %d blocks", DERSIG_HEIGHT - 2)
        self.coinbase_txids = [self.nodes[0].getblock(b)['tx'][0] for b in self.miniwallet.generate(DERSIG_HEIGHT - 2)]

        self.log.info("Test that a transaction with non-DER signature can still appear in a block")

        spendtx = self.create_tx(self.coinbase_txids[0])
        unDERify(spendtx)
        spendtx.rehash()

        tip = self.nodes[0].getbestblockhash()
        block_time = self.nodes[0].getblockheader(tip)['mediantime'] + 1

        block = create_block(int(tip, 16), create_coinbase(DERSIG_HEIGHT - 1), block_time)
        block.nVersion = 2
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()

        assert_equal(self.nodes[0].getblockcount(), DERSIG_HEIGHT - 2)
        self.test_dersig_info(is_active=False)  # Not active as of current tip and next block does not need to obey rules
        peer.send_and_ping(msg_block(block))
        assert_equal(self.nodes[0].getblockcount(), DERSIG_HEIGHT - 1)
        self.test_dersig_info(is_active=True)  # Not active as of current tip, but next block must obey rules
        assert_equal(self.nodes[0].getbestblockhash(), block.hash)

        self.log.info("Test that blocks must now be at least version 3")
        tip = block.sha256
        block_time += 1
        block = create_block(tip, create_coinbase(DERSIG_HEIGHT), block_time)
        block.nVersion = 2
        block.rehash()
        block.solve()
        with self.nodes[0].assert_debug_log(expected_msgs=['{}, bad-version(0x00000002)'.format(block.hash)]):
            peer.send_and_ping(msg_block(block))
            assert_equal(int(self.nodes[0].getbestblockhash(), 16), tip)
            peer.sync_with_ping()

        self.log.info("Test that transactions with non-DER signatures cannot appear in a block")
        block.nVersion = 3

        spendtx = self.create_tx(self.coinbase_txids[1])
        unDERify(spendtx)
        spendtx.rehash()

        # First we show that this tx is valid except for DERSIG by getting it
        # rejected from the mempool for exactly that reason.
        assert_equal(
            [{
                'txid': spendtx.hash,
                'wtxid': spendtx.getwtxid(),
                'allowed': False,
                'reject-reason': 'non-mandatory-script-verify-flag (Non-canonical DER signature)',
            }],
            self.nodes[0].testmempoolaccept(rawtxs=[spendtx.serialize().hex()], maxfeerate=0),
        )

        # Now we verify that a block with this transaction is also invalid.
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()

        with self.nodes[0].assert_debug_log(expected_msgs=['CheckInputScripts on {} failed with non-mandatory-script-verify-flag (Non-canonical DER signature)'.format(block.vtx[-1].hash)]):
            peer.send_and_ping(msg_block(block))
            assert_equal(int(self.nodes[0].getbestblockhash(), 16), tip)
            peer.sync_with_ping()

        self.log.info("Test that a version 3 block with a DERSIG-compliant transaction is accepted")
        block.vtx[1] = self.create_tx(self.coinbase_txids[1])
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()

        self.test_dersig_info(is_active=True)  # Not active as of current tip, but next block must obey rules
        peer.send_and_ping(msg_block(block))
        self.test_dersig_info(is_active=True)  # Active as of current tip
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.sha256)


if __name__ == '__main__':
    BIP66Test().main()
