#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test blockreconstructionextratxn option with compact blocks."""

from test_framework.blocktools import (
    COINBASE_MATURITY,
    NORMAL_GBT_REQUEST_PARAMS,
    create_block,
    add_witness_commitment,
)
from test_framework.messages import (
    CTxOut,
    HeaderAndShortIDs,
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
    OP_TRUE,
)
from test_framework.script_util import (
    keys_to_multisig_script,
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
        self.last_message["cmpctblock"].header_and_shortids.header.calc_sha256()
        self.announced_blockhashes.add(self.last_message["cmpctblock"].header_and_shortids.header.sha256)

    def on_headers(self, message):
        self.block_announced = True
        for x in self.last_message["headers"].headers:
            x.calc_sha256()
            self.announced_blockhashes.add(x.sha256)

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
            "-acceptnonstdtxn=0",
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
        self.generate(self.wallet, COINBASE_MATURITY + 420)

    def restart_node_with_limit(self, count=None):
        """Restart node with specific count limit."""
        extra_args = ["-acceptnonstdtxn=0", "-debug=net"]

        if count is not None:
            self.log.info(f"Setting transaction count limit: {count}")
            extra_args.append(f"-blockreconstructionextratxn={count}")

        self.log.info(f"Restarting node with args: {extra_args}")
        self.restart_node(0, extra_args=extra_args)
        self.segwit_node = self.nodes[0].add_p2p_connection(TestP2PConn())
        self.segwit_node.send_and_ping(msg_sendcmpct(announce=True, version=2))

    def create_policy_rejected_tx(self, rejection_type="dust"):
        """Create a transaction that will be rejected for policy reasons but added to extra pool."""

        if rejection_type == "dust":
            tx_info = self.wallet.create_self_transfer()
            dust_amount = 100
            dust_script = CScript([OP_TRUE])
            tx_info['tx'].vout.append(CTxOut(dust_amount, dust_script))
            tx_info['tx'].vout[0].nValue -= dust_amount

        elif rejection_type == "low_fee":
            tx_info = self.wallet.create_self_transfer(fee_rate=Decimal('0.00000100'))

        elif rejection_type == "nonstandard_script":
            tx_info = self.wallet.create_self_transfer()
            pubkeys = []
            for _ in range(4):
                pubkeys.append(bytes([0x02] + [0x00] * 32))
            multisig_script = keys_to_multisig_script(pubkeys, k=4)
            tx_info['tx'].vout.append(CTxOut(10000, multisig_script))
            tx_info['tx'].vout[0].nValue -= 10000

        else:
            raise ValueError(f"Unknown rejection type: {rejection_type}")

        tx_info['hex'] = tx_info['tx'].serialize().hex()
        return tx_info

    def populate_extra_pool(self, num_txs, rejection_type="dust"):
        """Populate the extra transaction pool using policy-rejected transactions."""
        rejected_txs = []

        for i in range(num_txs):
            tx_info = self.create_policy_rejected_tx(rejection_type)
            tx_obj = tx_from_hex(tx_info['hex'])
            self.segwit_node.send_message(msg_tx(tx_obj))
            rejected_txs.append(tx_info)

        self.segwit_node.sync_with_ping()

        return rejected_txs

    def send_compact_block(self, transactions, indices):
        """Send a compact block and check which transactions are requested for reconstruction."""
        node = self.nodes[0]

        # Build block
        block = self.build_block_on_tip(node)

        for i in indices:
            tx_obj = tx_from_hex(transactions[i]['hex'])
            block.vtx.append(tx_obj)

        # Add witness commitment for blocks with witness transactions
        add_witness_commitment(block)
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

    # TEST: policy-rejected transactions

    def test_policy_rejection_types(self):
        """Test that each policy rejection type adds transactions to extra pool."""
        self.log.info("Testing policy rejection types for extra pool...")

        self.restart_node_with_limit(count=100)

        rejection_types = ["dust", "low_fee", "nonstandard_script"]
        rejected_txs = []

        for rejection_type in rejection_types:
            self.log.info(f"Testing {rejection_type} rejection...")
            tx_info = self.create_policy_rejected_tx(rejection_type)

            tx_obj = tx_from_hex(tx_info['hex'])
            self.segwit_node.send_message(msg_tx(tx_obj))

            rejected_txs.append({
                'type': rejection_type,
                'tx_info': tx_info,
                'txid': tx_info['tx'].hash
            })

        self.segwit_node.sync_with_ping()

        mempool = self.nodes[0].getrawmempool()
        for rejected in rejected_txs:
            assert_equal(rejected['txid'] in mempool, False)
            self.log.info(f"✓ {rejected['type']} transaction rejected from mempool")

        indices = list(range(len(rejected_txs)))
        tx_list = [r['tx_info'] for r in rejected_txs]

        result = self.send_compact_block(tx_list, indices)

        assert_equal(result["missing_indices"], [])
        self.log.info("✓ All rejected transactions are available in extra pool")

    # TEST: blockreconstructionextratxn

    def test_extratxnpool_disabled(self):
        """Test that setting count to 0 disables the extra transaction pool."""
        self.log.info("Testing disabled extra transaction pool (0 capacity)...")

        self.restart_node_with_limit(count=0)
        buffersize = 5
        rejected_txs = self.populate_extra_pool(buffersize)

        indices = list(range(buffersize))
        result = self.send_compact_block(rejected_txs, indices)
        assert_equal(result["missing_indices"], indices)
        self.log.info(f"✓ All {buffersize} transactions are missing (extra txn pool disabled)")

    def test_extratxnpool_capacity_and_wraparound(self):
        """Test extra transaction pool capacity and wraparound behavior."""
        self.log.info("Testing extra transaction pool capacity (400 transactions)...")

        buffersize = 400
        self.restart_node_with_limit(count=buffersize)

        rejected_txs = self.populate_extra_pool(buffersize)

        indices = list(range(buffersize))
        result = self.send_compact_block(rejected_txs, indices)

        assert_equal(result["missing_indices"], [])
        self.log.info("✓ All rejected transactions are in the extra txn pool")

        # Test that adding a 401st transaction causes eviction
        self.log.info("Adding transaction to test eviction...")
        self.populate_extra_pool(1)

        # Check original transactions again - first one should be evicted
        result2 = self.send_compact_block(rejected_txs, indices)
        assert_equal(result2["missing_indices"], [0])
        self.log.info("✓ Transaction 0 was evicted as expected (wraparound)")

    def test_single_extratxnpool_capacity(self):
        """Test edge case of single capacity extra transaction pool."""
        self.log.info("Testing single capacity extra transaction pool...")

        self.restart_node_with_limit(count=1)
        tx_count = 5

        rejected_txs = self.populate_extra_pool(tx_count)

        indices = list(range(tx_count))
        result = self.send_compact_block(rejected_txs, indices)

        expected_missing = list(range(tx_count - 1))
        assert_equal(result["missing_indices"], expected_missing)

    def test_extratxn_invalid_parameters(self):
        """Test handling of invalid blockreconstructionextratxn values."""
        self.log.info("Testing invalid parameter values...")

        # Test negative value - should be clamped to 0 (disabled)
        self.log.info("Testing negative value (-1)...")
        self.restart_node_with_limit(count=-1)

        # Add a transaction and verify pool is disabled
        rejected_txs = self.populate_extra_pool(1)
        result = self.send_compact_block(rejected_txs, [0])
        assert_equal(result["missing_indices"], [0])
        self.log.info("✓ Negative value correctly treated as disabled")


    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        # Setup the p2p connection
        self.segwit_node = self.nodes[0].add_p2p_connection(TestP2PConn())

        # Create UTXOs for testing
        self.make_utxos()

        # Ensure segwit is active
        assert softfork_active(self.nodes[0], "segwit")

        # Test policy rejection types first
        self.test_policy_rejection_types()

        # Extra Txn capacity tests
        self.test_extratxnpool_disabled()
        self.test_extratxnpool_capacity_and_wraparound()
        self.test_single_extratxnpool_capacity()
        self.test_extratxn_invalid_parameters()



if __name__ == '__main__':
    CompactBlocksBlockReconstructionLimitTest(__file__).main()
