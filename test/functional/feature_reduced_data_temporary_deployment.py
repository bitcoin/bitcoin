#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test temporary BIP9 deployment with active_duration parameter.

This test verifies that a BIP9 deployment with active_duration properly expires
after the specified number of blocks. We use REDUCED_DATA as the test deployment
with active_duration=144 blocks.

The test uses two nodes:
- Node 0: BIP-110 enforcing (active_duration=144)
- Node 1: Non-BIP-110 (never active, simulates Bitcoin Core)

The test verifies:
1. BIP9 state transitions: DEFINED -> STARTED -> LOCKED_IN -> ACTIVE
2. Consensus rules ARE enforced during the active period (blocks 432-575)
3. Chain split: BIP-110 node rejects invalid blocks, non-BIP-110 accepts
4. Reorg: Longer valid chain wins when nodes reconnect
5. Consensus rules STOP being enforced after expiry (block 576+)
6. Post-expiry convergence: Both nodes accept the same blocks

Expected timeline:
- Period 0 (blocks 0-143): DEFINED
- Period 1 (blocks 144-287): STARTED (signaling happens here)
- Period 2 (blocks 288-431): LOCKED_IN
- Period 3 (blocks 432-575): ACTIVE (144 blocks, rules enforced on node0 only)
- Block 576+: EXPIRED (rules no longer enforced, nodes converge)
"""

from test_framework.blocktools import (
    create_block,
    create_coinbase,
    add_witness_commitment,
)
from test_framework.messages import (
    CTxOut,
)
from test_framework.script import (
    CScript,
    OP_RETURN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

REDUCED_DATA_BIT = 4
VERSIONBITS_TOP_BITS = 0x20000000


class TemporaryDeploymentTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        # Node 0: BIP-110 with active_duration=144 blocks
        # Node 1: BIP-110 never active (simulates Bitcoin Core)
        # NEVER_ACTIVE = -2 for start_time prevents deployment from ever leaving DEFINED state
        self.extra_args = [
            ['-vbparams=reduced_data:0:999999999999:0:2147483647:144', '-acceptnonstdtxn=1'],
            ['-vbparams=reduced_data:-2:-1', '-acceptnonstdtxn=1'],
        ]

    def setup_network(self):
        self.setup_nodes()
        self.connect_nodes(0, 1)

    def create_block_for_node(self, node, txs=None, signal=False, time_offset=0):
        """Create a block for a specific node."""
        if txs is None:
            txs = []
        tip = node.getbestblockhash()
        height = node.getblockcount() + 1
        tip_header = node.getblockheader(tip)
        block_time = tip_header['time'] + 1 + time_offset
        block = create_block(int(tip, 16), create_coinbase(height), ntime=block_time, txlist=txs)
        if signal:
            block.nVersion = VERSIONBITS_TOP_BITS | (1 << REDUCED_DATA_BIT)
        add_witness_commitment(block)
        block.solve()
        return block

    def mine_blocks_on_node(self, node, count, signal=False):
        """Mine count blocks on a specific node."""
        for _ in range(count):
            block = self.create_block_for_node(node, signal=signal)
            node.submitblock(block.serialize().hex())

    def create_tx_with_large_output(self, wallet):
        """Create a transaction with 84-byte OP_RETURN (violates BIP-110's 83-byte limit)."""
        tx_dict = wallet.create_self_transfer()
        tx = tx_dict['tx']
        # 81 bytes data = 84-byte script (OP_RETURN + OP_PUSHDATA1 + len + data)
        tx.vout.append(CTxOut(0, CScript([OP_RETURN, b'x' * 81])))
        return tx

    def get_deployment_status(self, node):
        """Get reduced_data deployment status."""
        info = node.getdeploymentinfo()
        rd = info['deployments']['reduced_data']
        if 'bip9' in rd:
            return rd['bip9']['status'], rd['bip9'].get('since', 'N/A')
        return rd.get('status'), rd.get('since', 'N/A')

    def run_test(self):
        node_bip110 = self.nodes[0]
        node_core = self.nodes[1]

        wallet = MiniWallet(node_bip110)

        # =====================================================================
        # Phase 1: Build common chain through BIP9 state transitions
        # =====================================================================
        self.log.info("Phase 1: Building common chain through BIP9 states")

        self.log.info("Mining initial blocks for spendable coins...")
        self.generate(wallet, 101)
        self.sync_all()

        status, _ = self.get_deployment_status(node_bip110)
        assert_equal(status, 'defined')

        # Mine to end of period 0
        self.log.info("Mining through period 0 (DEFINED)...")
        self.generate(node_bip110, 42)
        self.sync_all()
        assert_equal(node_bip110.getblockcount(), 143)

        # Period 1: Signal for activation
        self.log.info("Mining period 1 with signaling (STARTED)...")
        self.mine_blocks_on_node(node_bip110, 144, signal=True)
        self.sync_all()
        assert_equal(node_bip110.getblockcount(), 287)
        status, _ = self.get_deployment_status(node_bip110)
        assert_equal(status, 'started')

        # Period 2: Lock in
        self.log.info("Mining period 2 (LOCKED_IN)...")
        self.mine_blocks_on_node(node_bip110, 144, signal=True)
        self.sync_all()
        assert_equal(node_bip110.getblockcount(), 431)
        status, since = self.get_deployment_status(node_bip110)
        assert_equal(status, 'locked_in')
        assert_equal(since, 288)

        # =====================================================================
        # Phase 2: Test activation and chain split
        # =====================================================================
        self.log.info("Phase 2: Testing activation and chain split behavior")

        # Mine block 432 (activation)
        self.mine_blocks_on_node(node_bip110, 1)
        self.sync_all()
        assert_equal(node_bip110.getblockcount(), 432)
        status, since = self.get_deployment_status(node_bip110)
        self.log.info(f"Block 432 - Status: {status}, Since: {since}")
        assert_equal(status, 'active')
        assert_equal(since, 432)

        # Disconnect nodes BEFORE creating invalid block to prevent P2P relay
        # (Bitcoin Core relays blocks via compact blocks before full validation completes)
        self.log.info("Disconnecting nodes for chain split test...")
        self.disconnect_nodes(0, 1)

        # Create the invalid block (84-byte OP_RETURN violates BIP-110's 83-byte limit)
        self.log.info("Test: BIP-110 node rejects block with 84-byte OP_RETURN output")
        tx_invalid = self.create_tx_with_large_output(wallet)
        block_invalid = self.create_block_for_node(node_bip110, [tx_invalid])

        # Submit to BIP-110 node - should be rejected
        result_bip110 = node_bip110.submitblock(block_invalid.serialize().hex())
        assert_equal(result_bip110, 'bad-txns-vout-script-toolarge')
        assert_equal(node_bip110.getblockcount(), 432)

        # Submit to non-BIP-110 node - should be accepted
        self.log.info("Test: Non-BIP-110 node accepts the same block")
        result_core = node_core.submitblock(block_invalid.serialize().hex())
        assert_equal(result_core, None)
        assert_equal(node_core.getblockcount(), 433)

        # Chain split confirmed
        self.log.info(f"Chain split: BIP-110={node_bip110.getblockcount()}, Core={node_core.getblockcount()}")

        # =====================================================================
        # Phase 3: Test reorg behavior
        # =====================================================================
        self.log.info("Phase 3: Testing reorg behavior")

        # Non-BIP-110 extends its chain
        self.log.info("Non-BIP-110 node extends chain with 3 more blocks...")
        for i in range(3):
            block = self.create_block_for_node(node_core, time_offset=i)
            node_core.submitblock(block.serialize().hex())
        assert_equal(node_core.getblockcount(), 436)

        # BIP-110 node builds longer valid chain
        self.log.info("BIP-110 node builds longer valid chain (5 blocks)...")
        for i in range(5):
            block = self.create_block_for_node(node_bip110, time_offset=i+10)
            node_bip110.submitblock(block.serialize().hex())
        assert_equal(node_bip110.getblockcount(), 437)

        # Reconnect - non-BIP-110 should reorg to BIP-110's chain
        self.log.info("Reconnecting nodes - expecting reorg...")
        self.connect_nodes(0, 1)
        self.sync_blocks()

        assert_equal(node_core.getbestblockhash(), node_bip110.getbestblockhash())
        assert_equal(node_core.getblockcount(), 437)
        self.log.info(f"Reorg complete: both nodes at height {node_core.getblockcount()}")

        # =====================================================================
        # Phase 4: Test rules enforced until expiry
        # =====================================================================
        self.log.info("Phase 4: Testing rules enforced until expiry")

        # Mine to block 574 (one before last active block)
        # active_duration=144, activation at 432, so last active block is 432+144-1=575
        blocks_to_574 = 574 - node_bip110.getblockcount()
        self.log.info(f"Mining {blocks_to_574} blocks to reach block 574...")
        self.generate(node_bip110, blocks_to_574)
        self.sync_all()
        assert_equal(node_bip110.getblockcount(), 574)

        # Disconnect nodes to prevent compact block relay of invalid block
        self.disconnect_nodes(0, 1)

        # Verify rules still enforced at block 575 (last active block)
        self.log.info("Test: Rules still enforced at block 575 (last active block)")
        tx_invalid = self.create_tx_with_large_output(wallet)
        block_invalid = self.create_block_for_node(node_bip110, [tx_invalid])
        result = node_bip110.submitblock(block_invalid.serialize().hex())
        assert_equal(result, 'bad-txns-vout-script-toolarge')

        # Mine valid block 575 (last active block)
        block_valid = self.create_block_for_node(node_bip110)
        node_bip110.submitblock(block_valid.serialize().hex())
        assert_equal(node_bip110.getblockcount(), 575)

        # Reconnect and sync
        self.connect_nodes(0, 1)
        self.sync_all()

        # =====================================================================
        # Phase 5: Test expiry - rules no longer enforced
        # =====================================================================
        self.log.info("Phase 5: Testing expiry - rules no longer enforced")

        # At block 576, deployment has expired (first expired block = 432 + 144)
        self.log.info("Test: BIP-110 node accepts 'invalid' block at height 576 (expired)")
        tx_invalid = self.create_tx_with_large_output(wallet)
        block_after_expiry = self.create_block_for_node(node_bip110, [tx_invalid])
        result = node_bip110.submitblock(block_after_expiry.serialize().hex())
        assert_equal(result, None)
        self.sync_all()
        assert_equal(node_bip110.getblockcount(), 576)

        # Verify state machine reports EXPIRED
        status, since = self.get_deployment_status(node_bip110)
        self.log.info(f"Block 576: Status={status}, Since={since}")
        assert_equal(status, 'expired')
        assert_equal(since, 576)

        # =====================================================================
        # Phase 6: Test post-expiry convergence
        # =====================================================================
        self.log.info("Phase 6: Testing post-expiry convergence")

        # Both nodes should accept the same "invalid" blocks now
        self.log.info("Test: Both nodes accept 'invalid' blocks after expiry")
        for i in range(5):
            tx = self.create_tx_with_large_output(wallet)
            block = self.create_block_for_node(node_bip110, [tx], time_offset=i)
            result_bip110 = node_bip110.submitblock(block.serialize().hex())
            assert_equal(result_bip110, None)
            self.sync_all()
            assert_equal(node_core.getbestblockhash(), node_bip110.getbestblockhash())

        final_height = node_bip110.getblockcount()
        self.log.info(f"Final height: {final_height}, both nodes synced")

        # =====================================================================
        # Summary
        # =====================================================================
        self.log.info("All tests passed:")
        self.log.info("  - BIP9 state transitions (DEFINED -> STARTED -> LOCKED_IN -> ACTIVE -> EXPIRED)")
        self.log.info("  - Chain split at activation (BIP-110 rejects, Core accepts)")
        self.log.info("  - Reorg to longer valid chain on reconnect")
        self.log.info("  - Rules enforced during active period (432-575)")
        self.log.info("  - Rules not enforced after expiry (576+)")
        self.log.info("  - Post-expiry convergence (both nodes accept same blocks)")


if __name__ == '__main__':
    TemporaryDeploymentTest(__file__).main()
