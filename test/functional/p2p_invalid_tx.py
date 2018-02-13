#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid transactions.

In this test we connect to one node over p2p, and test tx requests."""
from test_framework.blocktools import create_block, create_coinbase, create_tx_with_script
from test_framework.messages import (
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
)
from test_framework.mininode import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    wait_until,
)


class InvalidTxRequestTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def bootstrap_p2p(self, *, num_connections=1):
        """Add a P2P connection to the node.

        Helper to connect and wait for version handshake."""
        for _ in range(num_connections):
            self.nodes[0].add_p2p_connection(P2PDataStore())

    def reconnect_p2p(self, **kwargs):
        """Tear down and bootstrap the P2P connection to the node.

        The node gets disconnected several times in this test. This helper
        method reconnects the p2p and restarts the network thread."""
        self.nodes[0].disconnect_p2ps()
        self.bootstrap_p2p(**kwargs)

    def run_test(self):
        node = self.nodes[0]  # convenience reference to the node

        self.bootstrap_p2p()  # Add one p2p connection to the node

        best_block = self.nodes[0].getbestblockhash()
        tip = int(best_block, 16)
        best_block_time = self.nodes[0].getblock(best_block)['time']
        block_time = best_block_time + 1

        self.log.info("Create a new block with an anyone-can-spend coinbase.")
        height = 1
        block = create_block(tip, create_coinbase(height), block_time)
        block.nVersion = 0x20000000
        block.solve()
        # Save the coinbase for later
        block1 = block
        tip = block.sha256
        node.p2p.send_blocks_and_test([block], node, success=True)

        self.log.info("Mature the block.")
        self.nodes[0].generate(100)

        # b'\x64' is OP_NOTIF
        # Transaction will be rejected with code 16 (REJECT_INVALID)
        # and we get disconnected immediately
        self.log.info('Test a transaction that is rejected')
        tx1 = create_tx_with_script(block1.vtx[0], 0, script_sig=b'\x64' * 35, amount=50 * COIN - 12000)
        node.p2p.send_txs_and_test([tx1], node, success=False, expect_disconnect=True)

        # Make two p2p connections to provide the node with orphans
        # * p2ps[0] will send valid orphan txs (one with low fee)
        # * p2ps[1] will send an invalid orphan tx (and is later disconnected for that)
        self.reconnect_p2p(num_connections=2)

        self.log.info('Test orphan transaction handling ... ')
        # Create a root transaction that we withhold until all dependend transactions
        # are sent out and in the orphan cache
        SCRIPT_PUB_KEY_OP_TRUE = b'\x51\x75' * 15 + b'\x51'
        tx_withhold = CTransaction()
        tx_withhold.vin.append(CTxIn(outpoint=COutPoint(block1.vtx[0].sha256, 0)))
        tx_withhold.vout.append(CTxOut(nValue=50 * COIN - 12000, scriptPubKey=SCRIPT_PUB_KEY_OP_TRUE))
        tx_withhold.calc_sha256()

        # Our first orphan tx with some outputs to create further orphan txs
        tx_orphan_1 = CTransaction()
        tx_orphan_1.vin.append(CTxIn(outpoint=COutPoint(tx_withhold.sha256, 0)))
        tx_orphan_1.vout = [CTxOut(nValue=10 * COIN, scriptPubKey=SCRIPT_PUB_KEY_OP_TRUE)] * 3
        tx_orphan_1.calc_sha256()

        # A valid transaction with low fee
        tx_orphan_2_no_fee = CTransaction()
        tx_orphan_2_no_fee.vin.append(CTxIn(outpoint=COutPoint(tx_orphan_1.sha256, 0)))
        tx_orphan_2_no_fee.vout.append(CTxOut(nValue=10 * COIN, scriptPubKey=SCRIPT_PUB_KEY_OP_TRUE))

        # A valid transaction with sufficient fee
        tx_orphan_2_valid = CTransaction()
        tx_orphan_2_valid.vin.append(CTxIn(outpoint=COutPoint(tx_orphan_1.sha256, 1)))
        tx_orphan_2_valid.vout.append(CTxOut(nValue=10 * COIN - 12000, scriptPubKey=SCRIPT_PUB_KEY_OP_TRUE))
        tx_orphan_2_valid.calc_sha256()

        # An invalid transaction with negative fee
        tx_orphan_2_invalid = CTransaction()
        tx_orphan_2_invalid.vin.append(CTxIn(outpoint=COutPoint(tx_orphan_1.sha256, 2)))
        tx_orphan_2_invalid.vout.append(CTxOut(nValue=11 * COIN, scriptPubKey=SCRIPT_PUB_KEY_OP_TRUE))

        self.log.info('Send the orphans ... ')
        # Send valid orphan txs from p2ps[0]
        node.p2p.send_txs_and_test([tx_orphan_1, tx_orphan_2_no_fee, tx_orphan_2_valid], node, success=False)
        # Send invalid tx from p2ps[1]
        node.p2ps[1].send_txs_and_test([tx_orphan_2_invalid], node, success=False)

        assert_equal(0, node.getmempoolinfo()['size'])  # Mempool should be empty
        assert_equal(2, len(node.getpeerinfo()))  # p2ps[1] is still connected

        self.log.info('Send the withhold tx ... ')
        node.p2p.send_txs_and_test([tx_withhold], node, success=True)

        # Transactions that should end up in the mempool
        expected_mempool = {
            t.hash
            for t in [
                tx_withhold,  # The transaction that is the root for all orphans
                tx_orphan_1,  # The orphan transaction that splits the coins
                tx_orphan_2_valid,  # The valid transaction (with sufficient fee)
            ]
        }
        # Transactions that do not end up in the mempool
        # tx_orphan_no_fee, because it has too low fee (p2ps[0] is not disconnected for relaying that tx)
        # tx_orphan_invaid, because it has negative fee (p2ps[1] is disconnected for relaying that tx)

        wait_until(lambda: 1 == len(node.getpeerinfo()), timeout=12)  # p2ps[1] is no longer connected
        assert_equal(expected_mempool, set(node.getrawmempool()))

        # restart node with sending BIP61 messages disabled, check that it disconnects without sending the reject message
        self.log.info('Test a transaction that is rejected, with BIP61 disabled')
        self.restart_node(0, ['-enablebip61=0', '-persistmempool=0'])
        self.reconnect_p2p(num_connections=1)
        with node.assert_debug_log(expected_msgs=[
                "{} from peer=0 was not accepted: mandatory-script-verify-flag-failed (Invalid OP_IF construction) (code 16)".format(tx1.hash),
                "disconnecting peer=0",
        ]):
            node.p2p.send_txs_and_test([tx1], node, success=False, expect_disconnect=True)
        # send_txs_and_test will have waited for disconnect, so we can safely check that no reject has been received
        assert_equal(node.p2p.reject_code_received, None)


if __name__ == '__main__':
    InvalidTxRequestTest().main()
