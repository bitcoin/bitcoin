#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test max_activation_height for mandatory BIP9 activation.

This test verifies that BIP9 deployments with max_activation_height properly
activate at the specified height regardless of miner signaling, similar to BIP8.

The test verifies four critical scenarios:
1. Mandatory activation at max_height without signaling
2. Normal deployment without max_height (requires signaling)
3. Early activation via signaling before reaching max_height
4. Max_height overrides timeout

Expected behavior:
- When max_activation_height is set and reached while in STARTED state,
  the deployment transitions to LOCKED_IN (then ACTIVE) regardless of signaling
- Max_activation_height overrides timeout
- Once ACTIVE, the deployment remains ACTIVE permanently (terminal state)
- Without max_activation_height, activation requires sufficient signaling
"""

from test_framework.blocktools import (
    create_block,
    create_coinbase,
    add_witness_commitment,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

TESTDUMMY_BIT = 28
TESTDUMMY_MASK = 1 << TESTDUMMY_BIT
VERSIONBITS_TOP_BITS = 0x20000000


class MaxActivationHeightTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 6  # 6 nodes for tests 1-6 (test 0 validation is done separately)
        self.setup_clean_chain = True
        # NO_TIMEOUT = std::numeric_limits<int64_t>::max() = 9223372036854775807
        NO_TIMEOUT = '9223372036854775807'
        # INT_MAX = std::numeric_limits<int>::max() = 2147483647
        INT_MAX = '2147483647'
        self.extra_args = [
            [f'-vbparams=testdummy:0:{NO_TIMEOUT}:0:576'],      # Test 1: max_height=576 (shows full flow)
            ['-vbparams=testdummy:0:999999999999'],              # Test 2: no max_height (uses timeout)
            [f'-vbparams=testdummy:0:{NO_TIMEOUT}:0:576'],      # Test 3: max_height=576 (early activation)
            [f'-vbparams=testdummy:0:{NO_TIMEOUT}:0:432'],      # Test 4: verify permanent ACTIVE
            [f'-vbparams=testdummy:0:{NO_TIMEOUT}:0:432:144'],  # Test 5: max_height + active_duration
            [f'-vbparams=testdummy:0:999999999999:0:{INT_MAX}:{INT_MAX}:72'],  # Test 6: custom 50% threshold (72/144)
        ]

    def setup_network(self):
        """Keep nodes isolated - don't connect them to each other"""
        self.add_nodes(self.num_nodes)
        for i in range(self.num_nodes):
            self.start_node(i, extra_args=self.extra_args[i])
        # Nodes remain disconnected for independent blockchain testing

    def mine_blocks(self, node, count, signal=False):
        """Mine count blocks, optionally signaling for testdummy."""
        for i in range(count):
            tip = node.getbestblockhash()
            height = node.getblockcount() + 1
            tip_header = node.getblockheader(tip)
            block_time = tip_header['time'] + 1
            block = create_block(int(tip, 16), create_coinbase(height), ntime=block_time)
            if signal:
                block.nVersion = VERSIONBITS_TOP_BITS | (1 << TESTDUMMY_BIT)
            add_witness_commitment(block)
            block.solve()
            node.submitblock(block.serialize().hex())

            # Log every 20 blocks and at key heights for debugging
            if height % 20 == 0 or height == 143 or height == 144:
                mtp = node.getblockheader(node.getbestblockhash())['mediantime']
                self.log.info(f"  Block {height}: time={block_time}, MTP={mtp}")

    def get_status(self, node):
        """Get testdummy deployment status."""
        info = node.getdeploymentinfo()
        td = info['deployments']['testdummy']
        if 'bip9' in td:
            return td['bip9']['status'], td['bip9'].get('since', 0)
        return td.get('status', 'unknown'), 0

    def run_test(self):
        # Test 0: Verify validation rejects both timeout and max_activation_height
        self.log.info("=== TEST 0: Validation test - reject both timeout and max_activation_height ===")
        self.log.info("Attempting to start bitcoind with both timeout and max_activation_height...")

        # Run bitcoind directly with invalid config to test validation
        import subprocess

        # Get the bitcoind binary path from the test framework
        bitcoind_path = self.binary_paths.bitcoind

        # Create a temporary datadir for this test
        import tempfile
        with tempfile.TemporaryDirectory() as tmpdir:
            try:
                # Run bitcoind with invalid vbparams (both timeout and max_activation_height)
                result = subprocess.run(
                    [bitcoind_path, f'-datadir={tmpdir}', '-regtest',
                     '-vbparams=testdummy:0:1:0:432'],  # timeout=1, max_activation_height=432
                    capture_output=True,
                    text=True,
                    timeout=5
                )

                # If we get here with exit code 0, the validation failed
                if result.returncode == 0:
                    raise AssertionError("bitcoind should have failed to start with both timeout and max_activation_height")

                # Check that the error message contains the expected validation error
                error_output = result.stderr
                self.log.info(f"bitcoind correctly failed with error: {error_output[:200]}")

                assert "Cannot specify both timeout" in error_output and "max_activation_height" in error_output, \
                    f"Expected validation error about both parameters, got: {error_output}"

                self.log.info("SUCCESS: Validation correctly rejected invalid configuration")

            except subprocess.TimeoutExpired:
                raise AssertionError("bitcoind timed out (should have failed immediately with validation error)")

        self.log.info("\n=== Test: max_activation_height=576 (full flow with non-mandatory period) ===")
        node = self.nodes[0]

        # Check deployment info to verify max_activation_height is set
        info = node.getdeploymentinfo()
        self.log.info(f"Deployment info: {info['deployments']['testdummy']}")

        # Period 0 (0-143): DEFINED
        self.log.info("\n--- Period 0 (blocks 0-143): DEFINED ---")
        self.mine_blocks(node, 143, signal=False)
        assert_equal(node.getblockcount(), 143)
        status, since = self.get_status(node)
        self.log.info(f"Block 143: Status={status}")
        assert_equal(status, 'defined')

        # Block 144: Transition to STARTED
        self.log.info("\n--- Block 144: Transition to STARTED ---")
        self.mine_blocks(node, 1, signal=False)
        status, since = self.get_status(node)
        self.log.info(f"Block 144: Status={status}, Since={since}")
        assert_equal(status, 'started')
        assert_equal(since, 144)

        # GBT: vbrequired should NOT have TESTDUMMY bit (before enforcement window [288, 432))
        gbt = node.getblocktemplate({"rules": ["segwit"]})
        assert_equal(gbt['vbrequired'] & TESTDUMMY_MASK, 0)

        # Period 1 (144-287): STARTED
        self.log.info("\n--- Period 1 (blocks 145-287): STARTED ---")
        self.mine_blocks(node, 143, signal=False)
        assert_equal(node.getblockcount(), 287)
        status, since = self.get_status(node)
        self.log.info(f"Block 287: Status={status}")
        assert_equal(status, 'started')

        # GBT: vbrequired SHOULD have TESTDUMMY bit (next block 288 is in enforcement window [288, 432))
        gbt = node.getblocktemplate({"rules": ["segwit"]})
        assert_equal(gbt['vbrequired'] & TESTDUMMY_MASK, TESTDUMMY_MASK)

        # Period 2 (288-431): STARTED - forced lock-in will occur at end of this period
        self.log.info("\n--- Period 2 (blocks 288-431): STARTED ---")
        self.log.info("Forced lock-in will occur at block 432 (max_activation_height - nPeriod)")

        # Try to mine block 288 without signaling - should be REJECTED
        self.log.info("\nNEGATIVE TEST: Attempting to mine block 288 without signaling...")
        tip = node.getbestblockhash()
        height = node.getblockcount() + 1
        tip_header = node.getblockheader(tip)
        block_time = tip_header['time'] + 1
        block = create_block(int(tip, 16), create_coinbase(height), ntime=block_time)
        block.nVersion = VERSIONBITS_TOP_BITS  # No signaling bit
        add_witness_commitment(block)
        block.solve()
        result = node.submitblock(block.serialize().hex())
        self.log.info(f"Submitblock result (should be rejected): {result}")
        # Block should be rejected - check we're still at block 287
        assert_equal(node.getblockcount(), 287)
        self.log.info("SUCCESS: Block without signaling was correctly REJECTED during enforcement window")

        # Now mine Period 2 with proper signaling (blocks 288-430)
        self.log.info("\nMining blocks 288-430 with proper signaling...")
        self.mine_blocks(node, 143, signal=True)
        assert_equal(node.getblockcount(), 430)

        # GBT: vbrequired SHOULD have TESTDUMMY bit (next block 431 is in enforcement window [288, 432))
        gbt = node.getblocktemplate({"rules": ["segwit"]})
        assert_equal(gbt['vbrequired'] & TESTDUMMY_MASK, TESTDUMMY_MASK)

        # Try to mine block 431 without signaling - should be REJECTED (last block of enforcement window)
        self.log.info("\nNEGATIVE TEST: Attempting to mine block 431 without signaling (upper boundary)...")
        tip = node.getbestblockhash()
        height = node.getblockcount() + 1
        tip_header = node.getblockheader(tip)
        block_time = tip_header['time'] + 1
        block = create_block(int(tip, 16), create_coinbase(height), ntime=block_time)
        block.nVersion = VERSIONBITS_TOP_BITS  # No signaling bit
        add_witness_commitment(block)
        block.solve()
        result = node.submitblock(block.serialize().hex())
        self.log.info(f"Submitblock result (should be rejected): {result}")
        assert_equal(node.getblockcount(), 430)
        self.log.info("SUCCESS: Block without signaling was correctly REJECTED at upper boundary of enforcement window")

        # Mine block 431 with proper signaling
        self.mine_blocks(node, 1, signal=True)
        assert_equal(node.getblockcount(), 431)
        status, since = self.get_status(node)
        self.log.info(f"Block 431: Status={status}")
        assert_equal(status, 'started')

        # GBT: vbrequired should NOT have TESTDUMMY bit (next block 432 is outside enforcement window)
        gbt = node.getblocktemplate({"rules": ["segwit"]})
        assert_equal(gbt['vbrequired'] & TESTDUMMY_MASK, 0)

        # status_next should be locked_in (forced lock-in at next period boundary)
        info = node.getdeploymentinfo()
        assert_equal(info['deployments']['testdummy']['bip9']['status_next'], 'locked_in')

        # Period 3 (432-575): LOCKED_IN (forced by max_activation_height)
        self.log.info("\n--- Period 3 (blocks 432-575): LOCKED_IN ---")
        self.mine_blocks(node, 1, signal=False)  # Mine block 432
        assert_equal(node.getblockcount(), 432)
        status, since = self.get_status(node)
        self.log.info(f"Block 432: Status={status}, Since={since}")
        assert_equal(status, 'locked_in')
        assert_equal(since, 432)

        # Mine through period 3 to activate at block 576
        self.log.info("Mining blocks 433-575...")
        self.mine_blocks(node, 143, signal=False)
        assert_equal(node.getblockcount(), 575)
        status, since = self.get_status(node)
        assert_equal(status, 'locked_in')

        # status_next should be active (activation at next period boundary)
        info = node.getdeploymentinfo()
        assert_equal(info['deployments']['testdummy']['bip9']['status_next'], 'active')

        # Period 4 (576+): ACTIVE
        self.log.info("\n--- Period 4 (block 576+): ACTIVE ---")
        self.mine_blocks(node, 1, signal=False)
        assert_equal(node.getblockcount(), 576)
        status, since = self.get_status(node)
        self.log.info(f"Block 576: Status={status}, Since={since}")
        assert_equal(status, 'active')
        assert_equal(since, 576)

        # RPC field assertions: max_activation_height present, height set, no height_end (permanent)
        info = node.getdeploymentinfo()
        td = info['deployments']['testdummy']
        assert_equal(td['bip9']['max_activation_height'], 576)
        assert_equal(td['height'], 576)
        assert 'height_end' not in td

        self.log.info("\n=== TEST 1 COMPLETE ===")
        self.log.info("Summary: max_activation_height=576 test passed")
        self.log.info("- Deployment activated at height 576 via forced lock-in at 432")
        self.log.info("- Mandatory signaling enforced during blocks 288-431 (BIP148-style)")
        self.log.info("- Non-signaling blocks rejected during enforcement window")
        self.log.info("- Non-signaling blocks accepted outside enforcement window")

        # Test 2: Deployment without max_height requires signaling
        self.log.info("\n\n=== TEST 2: Deployment without max_height requires signaling ===")
        node = self.nodes[1]

        # Period 0 (0-143): DEFINED
        self.log.info("\n--- Period 0 (blocks 0-143): DEFINED ---")
        self.mine_blocks(node, 143, signal=False)
        assert_equal(node.getblockcount(), 143)
        status, since = self.get_status(node)
        self.log.info(f"Block 143: Status={status}")
        assert_equal(status, 'defined')

        # Mine period 1 (blocks 144-287) without signaling - should transition to STARTED
        self.log.info("Mining period 1 (blocks 144-287) without signaling...")
        self.mine_blocks(node, 144, signal=False)
        assert_equal(node.getblockcount(), 287)
        status, since = self.get_status(node)
        self.log.info(f"Block 287: Status={status}")
        assert_equal(status, 'started')

        # Mine period 2 (blocks 288-431) without signaling - should remain STARTED
        self.log.info("Mining period 2 (blocks 288-431) without signaling...")
        self.mine_blocks(node, 144, signal=False)
        status, since = self.get_status(node)
        self.log.info(f"Block 431: Status={status}")
        assert_equal(status, 'started')  # Should NOT lock in without signaling

        # Mine period 3 (blocks 432-575) without signaling - should remain STARTED
        self.log.info("Mining period 3 (blocks 432-575) without signaling...")
        self.mine_blocks(node, 144, signal=False)
        status, since = self.get_status(node)
        self.log.info(f"Block 575: Status={status}")
        assert_equal(status, 'started')  # Still STARTED without signaling

        # RPC field assertions: max_activation_height should be absent (no max_height)
        info = node.getdeploymentinfo()
        assert 'max_activation_height' not in info['deployments']['testdummy']['bip9']

        self.log.info("\n=== TEST 2 COMPLETE ===")
        self.log.info("SUCCESS: Deployment did NOT activate without signaling (no max_height)")

        # Test 3: Early activation via signaling before max_height
        self.log.info("\n\n=== TEST 3: Early activation via signaling before max_height ===")
        node = self.nodes[2]

        # Period 0 (0-143): DEFINED
        self.log.info("\n--- Period 0 (blocks 0-143): DEFINED ---")
        self.mine_blocks(node, 143, signal=False)
        assert_equal(node.getblockcount(), 143)
        status, since = self.get_status(node)
        self.log.info(f"Block 143: Status={status}")
        assert_equal(status, 'defined')

        # Mine period 1 (blocks 144-287) with 100% signaling
        self.log.info("Mining period 1 (blocks 144-287) with 100% signaling...")
        self.mine_blocks(node, 144, signal=True)
        assert_equal(node.getblockcount(), 287)
        status, since = self.get_status(node)
        self.log.info(f"Block 287: Status={status}")
        assert_equal(status, 'started')

        # Mine period 2 (blocks 288-431) with signaling - should lock in
        self.log.info("Mining period 2 (blocks 288-431) with signaling - should lock in...")
        self.mine_blocks(node, 144, signal=True)
        assert_equal(node.getblockcount(), 431)
        status, since = self.get_status(node)
        self.log.info(f"Block 431: Status={status}, Since={since}")
        assert_equal(status, 'locked_in')
        assert_equal(since, 288)  # Locked in at start of period 2 via signaling threshold

        # Mine block 432 - should activate via signaling (well before max_height 576)
        self.log.info("Mining block 432 - should activate via signaling (before max_height 576)...")
        self.mine_blocks(node, 1, signal=True)
        assert_equal(node.getblockcount(), 432)
        status, since = self.get_status(node)
        self.log.info(f"Block 432: Status={status}, Since={since}")
        assert_equal(status, 'active')
        assert_equal(since, 432)

        self.log.info("\n=== TEST 3 COMPLETE ===")
        self.log.info("SUCCESS: Deployment activated early via signaling (at 432, before max_height 576)")

        # Test 4: Verify ACTIVE state is permanent
        self.log.info("\n\n=== TEST 4: Verify ACTIVE state is permanent ===")
        node = self.nodes[3]

        # Activate via max_height (max_height=432)
        # Mine to block 143 (period 0) without signaling
        self.log.info("Mining period 0 (blocks 0-143) without signaling...")
        self.mine_blocks(node, 143, signal=False)
        assert_equal(node.getblockcount(), 143)

        # Mine through enforcement window (blocks 144-287) WITH signaling
        # Enforcement window for max_height=432 is [144, 288)
        self.log.info("Mining blocks 144-287 with signaling (enforcement window)...")
        self.mine_blocks(node, 144, signal=True)
        assert_equal(node.getblockcount(), 287)
        status, since = self.get_status(node)
        assert_equal(status, 'started')

        # Mine period 2 (blocks 288-431) - will force lock-in at 288 (432 - 144)
        self.log.info("Mining period 2 (blocks 288-431) - forced lock-in at end...")
        self.mine_blocks(node, 144, signal=False)
        assert_equal(node.getblockcount(), 431)
        status, since = self.get_status(node)
        assert_equal(status, 'locked_in')

        # Mine block 432 - should activate
        self.log.info("Mining block 432 - should activate via max_height...")
        self.mine_blocks(node, 1, signal=False)
        assert_equal(node.getblockcount(), 432)
        status, since = self.get_status(node)
        self.log.info(f"Block 432: Status={status}, Since={since}")
        assert_equal(status, 'active')
        assert_equal(since, 432)

        # Mine 300 more blocks to verify permanence
        self.log.info("Mining 300 more blocks to verify ACTIVE state persists...")
        self.mine_blocks(node, 300, signal=False)
        assert_equal(node.getblockcount(), 732)
        status, since = self.get_status(node)
        self.log.info(f"Block 732: Status={status}, Since={since}")
        assert_equal(status, 'active')
        assert_equal(since, 432)

        self.log.info("\n=== TEST 4 COMPLETE ===")
        self.log.info("SUCCESS: Deployment remains ACTIVE permanently")

        # Test 5: Combined temporary deployment with max_height
        self.log.info("\n\n=== TEST 5: Temporary deployment with max_height ===")
        node = self.nodes[4]

        # This node has max_activation_height=432 AND active_duration=144
        # Should activate at 432 via max_height, then expire at 432+144=576
        # Mine to block 143 (period 0) without signaling
        self.log.info("Mining period 0 (blocks 0-143) without signaling...")
        self.mine_blocks(node, 143, signal=False)
        assert_equal(node.getblockcount(), 143)

        # Mine through enforcement window (blocks 144-287) WITH signaling
        # Enforcement window for max_height=432 is [144, 288)
        self.log.info("Mining blocks 144-287 with signaling (enforcement window)...")
        self.mine_blocks(node, 144, signal=True)
        assert_equal(node.getblockcount(), 287)
        status, since = self.get_status(node)
        self.log.info(f"Block 287: Status={status}")
        assert_equal(status, 'started')

        # Mine period 2 (blocks 288-431) - will force lock-in at 288 (432 - 144)
        self.log.info("Mining period 2 (blocks 288-431) - forced lock-in at end...")
        self.mine_blocks(node, 144, signal=False)
        assert_equal(node.getblockcount(), 431)
        status, since = self.get_status(node)
        self.log.info(f"Block 431: Status={status}")
        assert_equal(status, 'locked_in')

        # Mine block 432 - should activate via max_height
        self.log.info("Mining block 432 - should activate via max_height...")
        self.mine_blocks(node, 1, signal=False)
        assert_equal(node.getblockcount(), 432)
        status, since = self.get_status(node)
        self.log.info(f"Block 432: Status={status}, Since={since}")
        assert_equal(status, 'active')
        assert_equal(since, 432)

        # RPC field assertions: max_activation_height, height, and height_end for temporary deployment
        info = node.getdeploymentinfo()
        td = info['deployments']['testdummy']
        assert_equal(td['bip9']['max_activation_height'], 432)
        assert_equal(td['height'], 432)
        assert_equal(td['height_end'], 575)  # 432 + 144 - 1

        # Mine through active period to block 575 (432+144-1)
        self.log.info("Mining through active period to block 575 (432+144-1)...")
        self.mine_blocks(node, 143, signal=False)
        assert_equal(node.getblockcount(), 575)
        status, since = self.get_status(node)
        self.log.info(f"Block 575: Status={status}")
        assert_equal(status, 'active')

        # status_next should be expired (expiry at next period boundary)
        info = node.getdeploymentinfo()
        assert_equal(info['deployments']['testdummy']['bip9']['status_next'], 'expired')

        # Mine block 576 (432+144) - deployment has expired
        # RPC status uses State(blockindex->pprev), so expired appears at tip 576
        self.log.info("Mining block 576 (432+144) - deployment should be expired...")
        self.mine_blocks(node, 1, signal=False)
        assert_equal(node.getblockcount(), 576)
        status, since = self.get_status(node)
        self.log.info(f"Block 576: Status={status}, Since={since}")
        assert_equal(status, 'expired')
        assert_equal(since, 576)

        # status_next should also be expired (terminal state)
        info = node.getdeploymentinfo()
        assert_equal(info['deployments']['testdummy']['bip9']['status_next'], 'expired')

        # Mine block 577 - verify EXPIRED is terminal
        self.log.info("Mining block 577 - verify EXPIRED is terminal...")
        self.mine_blocks(node, 1, signal=False)
        assert_equal(node.getblockcount(), 577)
        status, since = self.get_status(node)
        self.log.info(f"Block 577: Status={status}")
        assert_equal(status, 'expired')

        self.log.info("\n=== TEST 5 COMPLETE ===")
        self.log.info("SUCCESS: Temporary deployment with max_height activated and expired correctly")

        # Test 6: Custom per-deployment threshold
        self.log.info("\n\n=== TEST 6: Custom per-deployment threshold (50% = 72/144 blocks) ===")
        node = self.nodes[5]

        # This node has threshold=72 (50% of 144 blocks)
        # Default regtest threshold is 108 (75%), but this deployment should activate at 72

        # Period 0 (0-143): DEFINED
        self.log.info("Mining period 0 (blocks 0-143) without signaling...")
        self.mine_blocks(node, 143, signal=False)
        assert_equal(node.getblockcount(), 143)
        status, _ = self.get_status(node)
        assert_equal(status, 'defined')

        # Block 144: Transition to STARTED
        self.log.info("Mining block 144 to transition to STARTED...")
        self.mine_blocks(node, 1, signal=False)
        assert_equal(node.getblockcount(), 144)
        status, since = self.get_status(node)
        self.log.info(f"Block 144: Status={status}, Since={since}")
        assert_equal(status, 'started')
        assert_equal(since, 144)

        # Period 1 (144-287): NEGATIVE TEST - Mine only 71 signaling blocks (one below threshold)
        self.log.info("NEGATIVE TEST: Mining period 1 with only 71 signaling blocks (below threshold of 72)...")
        self.mine_blocks(node, 71, signal=True)   # 71 signaling blocks
        self.mine_blocks(node, 72, signal=False)  # 72 non-signaling blocks (+ 1 from block 144 = 73 total)
        assert_equal(node.getblockcount(), 287)

        # Block 288: Should remain STARTED (71 < 72 threshold)
        self.mine_blocks(node, 1, signal=False)
        assert_equal(node.getblockcount(), 288)
        status, since = self.get_status(node)
        self.log.info(f"Block 288: Status={status} (should remain started, 71 < threshold 72)")
        assert_equal(status, 'started')
        assert_equal(since, 144)

        # Period 2 (288-431): POSITIVE TEST - Mine exactly 72 signaling blocks (meets threshold)
        self.log.info("Mining period 2 with exactly 72 signaling blocks (meets threshold of 72)...")
        self.mine_blocks(node, 72, signal=True)   # 72 signaling blocks
        self.mine_blocks(node, 71, signal=False)  # 71 non-signaling blocks (+ 1 from block 288 = 72 total)
        assert_equal(node.getblockcount(), 431)

        # Block 432: Should transition to LOCKED_IN (threshold met in previous period)
        self.log.info("Mining block 432 to check lock-in...")
        self.mine_blocks(node, 1, signal=False)
        assert_equal(node.getblockcount(), 432)
        status, since = self.get_status(node)
        self.log.info(f"Block 432: Status={status}, Since={since}")
        assert_equal(status, 'locked_in')
        assert_equal(since, 432)

        # Mine through locked_in period to activate
        self.log.info("Mining through locked_in period (433-575)...")
        self.mine_blocks(node, 143, signal=False)
        assert_equal(node.getblockcount(), 575)
        status, since = self.get_status(node)
        assert_equal(status, 'locked_in')

        # Block 576: Should transition to ACTIVE
        self.log.info("Mining block 576 to activate...")
        self.mine_blocks(node, 1, signal=False)
        assert_equal(node.getblockcount(), 576)
        status, since = self.get_status(node)
        self.log.info(f"Block 576: Status={status}, Since={since}")
        assert_equal(status, 'active')
        assert_equal(since, 576)

        self.log.info("\n=== TEST 6 COMPLETE ===")
        self.log.info("SUCCESS: Deployment activated with custom 50% threshold (72/144 blocks)")
        self.log.info("- 71/144 signaling correctly failed to lock in (negative test)")
        self.log.info("- 72/144 signaling correctly locked in (positive test)")


if __name__ == '__main__':
    MaxActivationHeightTest(__file__).main()
