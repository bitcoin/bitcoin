#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test blockreconstructionextratxn option with compact blocks."""

import random

from test_framework.blocktools import (
    COINBASE_MATURITY,
    NORMAL_GBT_REQUEST_PARAMS,
    create_block,
)
from test_framework.messages import (
    CTxOut,
    HeaderAndShortIDs,
    MAX_BIP125_RBF_SEQUENCE,
    MSG_BLOCK,
    msg_cmpctblock,
    msg_sendcmpct,
    msg_tx,
    tx_from_hex,
)
from test_framework.p2p import (
    P2PInterface,
    p2p_lock,
)
from test_framework.script import (
    CScript,
    OP_DROP,
    OP_TRUE,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    softfork_active,
)
from decimal import Decimal
from test_framework.wallet import MiniWallet


# TestP2PConn: A peer we use to send messages to bitcoind, and store responses.
class TestP2PConn(P2PInterface):
    def __init__(self):
        super().__init__()
        self.last_sendcmpct = []
        self.block_announced = False
        # Store the hashes of blocks we've seen announced.
        # This is for synchronizing the p2p message traffic,
        # so we can eg wait until a particular block is announced.
        self.announced_blockhashes = set()

    def on_sendcmpct(self, message):
        self.last_sendcmpct.append(message)

    def on_cmpctblock(self, message):
        self.block_announced = True
        self.announced_blockhashes.add(self.last_message["cmpctblock"].header_and_shortids.header.hash_int)

    def on_headers(self, message):
        self.block_announced = True
        for x in self.last_message["headers"].headers:
            self.announced_blockhashes.add(x.hash_int)

    def on_inv(self, message):
        for x in self.last_message["inv"].inv:
            if x.type == MSG_BLOCK:
                self.block_announced = True
                self.announced_blockhashes.add(x.hash)

    # Requires caller to hold p2p_lock
    def received_block_announcement(self):
        return self.block_announced

    def clear_block_announcement(self):
        with p2p_lock:
            self.block_announced = False
            self.last_message.pop("inv", None)
            self.last_message.pop("headers", None)
            self.last_message.pop("cmpctblock", None)

    def clear_getblocktxn(self):
        with p2p_lock:
            self.last_message.pop("getblocktxn", None)


class CompactBlocksBlockReconstructionLimitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[
            "-acceptnonstdtxn=1",
            "-debug=net",
        ]]
        self.utxos = []

    def build_block_on_tip(self, node):
        """Build a block on top of the current tip."""
        block = create_block(tmpl=node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS))
        block.solve()
        return block

    def make_utxos(self):
        """Generate blocks to create UTXOs for the wallet."""
        self.generate(self.wallet, COINBASE_MATURITY + 800)

    def restart_node_with_limit(self, count=None):
        """Restart node with specific count limit."""
        extra_args = ["-acceptnonstdtxn=1", "-debug=net"]

        if count is not None:
            self.log.info(f"Setting transaction count limit: {count}")
            extra_args.append(f"-blockreconstructionextratxn={count}")

        self.log.info(f"Restarting node with args: {extra_args}")
        self.restart_node(0, extra_args=extra_args)
        self.segwit_node = self.nodes[0].add_p2p_connection(TestP2PConn())
        self.segwit_node.send_and_ping(msg_sendcmpct(announce=True, version=2))

    def create_extra_pool_transactions(self, num_txs):
        """Create pairs of original and replacement RBF transactions."""
        original_txs = []
        replacement_txs = []

        for i in range(num_txs):
            utxo = self.wallet.get_utxo()

            # Create normal sized transactions
            # from: rbf_extra_pool_explanation.md
            # Create original transaction with low fee
            original = self.wallet.create_self_transfer(
                utxo_to_spend=utxo,
                sequence=MAX_BIP125_RBF_SEQUENCE,  # 0xfffffffd - RBF enabled
                fee_rate=Decimal('0.001')
            )
            original_txs.append(original)

            # Create rbf transaction, higher fee
            replacement = self.wallet.create_self_transfer(
                utxo_to_spend=utxo,
                sequence=MAX_BIP125_RBF_SEQUENCE - 1,  
                fee_rate=Decimal('0.01')
            )
            replacement_txs.append(replacement)

        return original_txs, replacement_txs

    def populate_extra_pool(self, num_txs):
        """Populate the extra transaction pool by sending RBF transaction pairs."""
        node = self.nodes[0]

        original_txs, replacement_txs = self.create_extra_pool_transactions(num_txs)

        for i, original in enumerate(original_txs):
            tx_obj = tx_from_hex(original['hex'])
            self.segwit_node.send_message(msg_tx(tx_obj))

        for i, replacement in enumerate(replacement_txs):
            tx_obj = tx_from_hex(replacement['hex'])
            self.segwit_node.send_message(msg_tx(tx_obj))

        self.segwit_node.sync_with_ping()

        return original_txs

    def send_compact_block(self, transactions, indices):
        """Send a compact block and check which transactions are requested for reconstruction."""
        node = self.nodes[0]

        # Build block 
        block = self.build_block_on_tip(node)

        for i in indices:
            tx_obj = tx_from_hex(transactions[i]['hex'])
            block.vtx.append(tx_obj)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()

        # Send as compact block
        cmpct_block = HeaderAndShortIDs()
        cmpct_block.initialize_from_block(block, use_witness=True)
        self.segwit_node.send_and_ping(msg_cmpctblock(cmpct_block.to_p2p()))

        # Check if node requested missing transactions
        with p2p_lock:
            getblocktxn = self.segwit_node.last_message.get("getblocktxn")

        num_tx_requested = len(getblocktxn.block_txn_request.indexes) if getblocktxn else 0
        self.segwit_node.clear_getblocktxn()

        # Convert differential encoding to absolute indices (from BlockTransactionRequest)
        missing_indices = []
        if getblocktxn:
            absolute_block_indices = getblocktxn.block_txn_request.to_absolute()
            # Convert from block positions to transaction indices (subtract 1 for coinbase)
            missing_indices = [idx - 1 for idx in absolute_block_indices]

        return {
            "block": block,
            "getblocktxn": getblocktxn,
            "num_tx_requested": num_tx_requested,
            "missing_indices": missing_indices
        }

    def test_extratxnpool_disabled(self):
        """Test that setting count to 0 disables the extra transaction pool."""
        self.log.info("Testing disabled extra transaction pool (0 capacity)...")

        self.restart_node_with_limit(count=0)
        buffersize = 5
        original_txs = self.populate_extra_pool(buffersize)

        indices = list(range(buffersize))
        result = self.send_compact_block(original_txs, indices)
        assert_equal(result["missing_indices"], indices)
        self.log.info(f"✓ All {buffersize} transactions are missing (extra txn pool disabled)")

    def test_extratxnpool_capacity(self):
        """Test extra transaction pool capacity transactions."""
        self.log.info("Testing extra transaction pool capacity (50 transactions)...")

        buffersize = 50
        self.restart_node_with_limit(count=buffersize)

        original_txs = self.populate_extra_pool(buffersize)

        indices = list(range(buffersize))
        result = self.send_compact_block(original_txs, indices)

        assert_equal(result["missing_indices"], [])
        self.log.info("✓ All original transactions are in the extra txn pool")

        # Test that adding a 51st transaction causes eviction
        self.log.info("Adding transaction to test eviction...")
        new_txs = self.populate_extra_pool(1)

        # Check original transactions again - first one should be evicted
        result2 = self.send_compact_block(original_txs, indices)
        assert_equal(result2["missing_indices"], [0])
        self.log.info("✓ Transaction 0 was evicted as expected")

    def test_single_extratxnpool_capacity(self):
        """Test edge case of single capacity extra transaction pool."""
        self.log.info("Testing single capacity extra transaction pool...")

        self.restart_node_with_limit(count=1)
        tx_count = 5  # number of transactions to test

        original_txs = self.populate_extra_pool(tx_count)

        indices = list(range(tx_count))
        result = self.send_compact_block(original_txs, indices)

        expected_missing = list(range(4))
        assert_equal(result["missing_indices"], expected_missing)

    def test_extratxn_large_capacity(self):
        """Test extra transaction pool with very large count parameter."""
        self.log.info("Testing with count=400 to ensure large buffers work correctly...")

        buffersize = 400
        self.restart_node_with_limit(count=buffersize)

        original_txs = self.populate_extra_pool(buffersize)

        indices = list(range(buffersize))
        result = self.send_compact_block(original_txs, indices)

        assert_equal(result["missing_indices"], [])
        self.log.info("✓ All original transactions are in the extra txn pool")

        # Test that adding a transaction causes eviction
        self.log.info("Adding another transaction to test eviction...")
        new_txs = self.populate_extra_pool(1)

        # Check original transactions again - first one should be evicted
        result2 = self.send_compact_block(original_txs, indices)
        assert_equal(result2["missing_indices"], [0])
        self.log.info("✓ Transaction 0 was evicted as expected")

    def test_extratxn_buffer_wraparound(self):
        """Test that adding transactions to a full buffer evicts oldest slots."""
        self.log.info("Testing extratxn buffer wraparound - fill buffer then add more...")

        buffersize = 20  # buffer size, total txns size

        # number of new transactions to add
        new_tx_count = 17
        self.restart_node_with_limit(count=buffersize)

        # Step 1: Fill the buffer with tx pairs (original + replacement)
        original_txs = self.populate_extra_pool(buffersize)

        # Verify all original transactions are in the extra pool
        indices = list(range(buffersize))
        result = self.send_compact_block(original_txs, indices)
        assert_equal(result["missing_indices"], [])
        self.log.info("✓ All original transactions are in the extra pool")

        # Step 2: Add more transaction pairs
        self.log.info(f"Step 2: Adding {new_tx_count} more transaction pairs (should wrap and evict slots 0-{new_tx_count-1})")
        new_txs = self.populate_extra_pool(new_tx_count)

        result2 = self.send_compact_block(original_txs, indices)

        # Verify wraparound worked correctly - first new_tx_count should be evicted
        expected_missing = list(range(new_tx_count))  # First 16 should be evicted
        assert_equal(result2['missing_indices'], expected_missing)
        self.log.info(f"✓ Wraparound worked correctly! Transactions {expected_missing} were evicted as expected")

    def test_extratxn_minimal_capacity_eviction(self):
        """Test frequent eviction with minimal capacity."""
        self.log.info("Testing minimal capacity eviction with count=2...")
        
        self.restart_node_with_limit(2)
        
        # Add 10 transaction pairs to stress eviction
        num_txs = 10
        original_txs = self.populate_extra_pool(num_txs)
        
        # Only the last 2 should remain
        # Try to reconstruct with all 10
        indices = list(range(num_txs))
        result = self.send_compact_block(original_txs, indices)
        
        # First 8 should be missing (evicted)
        expected_missing = list(range(8))  # 0-7 evicted
        assert_equal(result["missing_indices"], expected_missing)

    def test_extratxn_invalid_parameters(self):
        """Test handling of invalid blockreconstructionextratxn values."""
        self.log.info("Testing invalid parameter values...")
        
        # Test negative value - should be clamped to 0 (disabled)
        self.log.info("Testing negative value (-1)...")
        self.restart_node_with_limit(count=-1)
        
        # Add a transaction and verify pool is disabled
        original_txs = self.populate_extra_pool(1)
        result = self.send_compact_block(original_txs, [0])
        assert_equal(result["missing_indices"], [0])
        
        # Test extremely large value
        self.log.info("Testing extremely large value (5000000000)...")
        self.restart_node_with_limit(count=5000000000)  # > uint32_t max
        
        # Add some transactions - should work but be clamped
        original_txs = self.populate_extra_pool(10)
        result = self.send_compact_block(original_txs, list(range(10)))
        assert_equal(result["missing_indices"], [])
 

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        # Setup the p2p connection
        self.segwit_node = self.nodes[0].add_p2p_connection(TestP2PConn())

        # Create UTXOs for testing
        self.make_utxos()

        # Ensure segwit is active
        assert softfork_active(self.nodes[0], "segwit")
        
        # Extra Txn capacity tests
        self.test_extratxnpool_disabled()
        self.test_extratxnpool_capacity()
        self.test_single_extratxnpool_capacity()
        self.test_extratxn_large_capacity()
        self.test_extratxn_minimal_capacity_eviction()
        self.test_extratxn_invalid_parameters()

        # Extra Txn wraparound tests
        self.test_extratxn_buffer_wraparound()



if __name__ == '__main__':
    CompactBlocksBlockReconstructionLimitTest(__file__).main()
