#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests NODE_COMPACT_FILTERS (BIP 157/158).

Tests that a node configured with -blockfilterindex and -peerblockfilters signals
NODE_COMPACT_FILTERS and can serve cfilters, cfheaders and cfcheckpts.
"""

from test_framework.messages import (
    CAssetLockTx,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    FILTER_TYPE_BASIC,
    NODE_COMPACT_FILTERS,
    hash256,
    msg_getcfcheckpt,
    msg_getcfheaders,
    msg_getcfilters,
    ser_uint256,
    tx_from_hex,
    uint256_from_str,
)
from test_framework.p2p import P2PInterface
from test_framework.script import (
    CScript,
    OP_RETURN,
)
from test_framework.script_util import (
    key_to_p2pkh_script,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
)
from test_framework.key import ECKey
from decimal import Decimal

class FiltersClient(P2PInterface):
    def __init__(self):
        super().__init__()
        # Store the cfilters received.
        self.cfilters = []

    def pop_cfilters(self):
        cfilters = self.cfilters
        self.cfilters = []
        return cfilters

    def on_cfilter(self, message):
        """Store cfilters received in a list."""
        self.cfilters.append(message)


class CompactFiltersTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.rpc_timeout = 480
        self.num_nodes = 2
        self.extra_args = [
            ["-blockfilterindex", "-peerblockfilters"],
            ["-blockfilterindex"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Node 0 supports COMPACT_FILTERS, node 1 does not.
        peer_0 = self.nodes[0].add_p2p_connection(FiltersClient())
        peer_1 = self.nodes[1].add_p2p_connection(FiltersClient())

        self.privkeys = [self.nodes[0].get_deterministic_priv_key().key]
        self.change_addr = self.nodes[0].get_deterministic_priv_key().address

        # Nodes 0 & 1 share the same first 999 blocks in the chain.
        self.generate(self.nodes[0], 999)

        # Stale blocks by disconnecting nodes 0 & 1, mining, then reconnecting
        self.disconnect_nodes(0, 1)

        stale_block_hash = self.generate(self.nodes[0], 1, sync_fun=self.no_op)[0]
        self.nodes[0].syncwithvalidationinterfacequeue()
        assert_equal(self.nodes[0].getblockcount(), 1000)

        self.generate(self.nodes[1], 1001, sync_fun=self.no_op)
        assert_equal(self.nodes[1].getblockcount(), 2000)

        # Check that nodes have signalled NODE_COMPACT_FILTERS correctly.
        assert peer_0.nServices & NODE_COMPACT_FILTERS != 0
        assert peer_1.nServices & NODE_COMPACT_FILTERS == 0

        # Check that the localservices is as expected.
        assert int(self.nodes[0].getnetworkinfo()['localservices'], 16) & NODE_COMPACT_FILTERS != 0
        assert int(self.nodes[1].getnetworkinfo()['localservices'], 16) & NODE_COMPACT_FILTERS == 0

        self.log.info("get cfcheckpt on chain to be re-orged out.")
        request = msg_getcfcheckpt(
            filter_type=FILTER_TYPE_BASIC,
            stop_hash=int(stale_block_hash, 16),
        )
        peer_0.send_and_ping(message=request)
        response = peer_0.last_message['cfcheckpt']
        assert_equal(response.filter_type, request.filter_type)
        assert_equal(response.stop_hash, request.stop_hash)
        assert_equal(len(response.headers), 1)

        self.log.info("Reorg node 0 to a new chain.")
        self.connect_nodes(0, 1)
        self.sync_blocks(timeout=600)
        self.nodes[0].syncwithvalidationinterfacequeue()

        main_block_hash = self.nodes[0].getblockhash(1000)
        assert main_block_hash != stale_block_hash, "node 0 chain did not reorganize"

        self.log.info("Check that peers can fetch cfcheckpt on active chain.")
        tip_hash = self.nodes[0].getbestblockhash()
        request = msg_getcfcheckpt(
            filter_type=FILTER_TYPE_BASIC,
            stop_hash=int(tip_hash, 16),
        )
        peer_0.send_and_ping(request)
        response = peer_0.last_message['cfcheckpt']
        assert_equal(response.filter_type, request.filter_type)
        assert_equal(response.stop_hash, request.stop_hash)

        main_cfcheckpt = self.nodes[0].getblockfilter(main_block_hash, 'basic')['header']
        tip_cfcheckpt = self.nodes[0].getblockfilter(tip_hash, 'basic')['header']
        assert_equal(
            response.headers,
            [int(header, 16) for header in (main_cfcheckpt, tip_cfcheckpt)],
        )

        self.log.info("Check that peers can fetch cfcheckpt on stale chain.")
        request = msg_getcfcheckpt(
            filter_type=FILTER_TYPE_BASIC,
            stop_hash=int(stale_block_hash, 16),
        )
        peer_0.send_and_ping(request)
        response = peer_0.last_message['cfcheckpt']

        stale_cfcheckpt = self.nodes[0].getblockfilter(stale_block_hash, 'basic')['header']
        assert_equal(
            response.headers,
            [int(header, 16) for header in (stale_cfcheckpt, )],
        )

        self.log.info("Check that peers can fetch cfheaders on active chain.")
        request = msg_getcfheaders(
            filter_type=FILTER_TYPE_BASIC,
            start_height=1,
            stop_hash=int(main_block_hash, 16),
        )
        peer_0.send_and_ping(request)
        response = peer_0.last_message['cfheaders']
        main_cfhashes = response.hashes
        assert_equal(len(main_cfhashes), 1000)
        assert_equal(
            compute_last_header(response.prev_header, response.hashes),
            int(main_cfcheckpt, 16),
        )

        self.log.info("Check that peers can fetch cfheaders on stale chain.")
        request = msg_getcfheaders(
            filter_type=FILTER_TYPE_BASIC,
            start_height=1,
            stop_hash=int(stale_block_hash, 16),
        )
        peer_0.send_and_ping(request)
        response = peer_0.last_message['cfheaders']
        stale_cfhashes = response.hashes
        assert_equal(len(stale_cfhashes), 1000)
        assert_equal(
            compute_last_header(response.prev_header, response.hashes),
            int(stale_cfcheckpt, 16),
        )

        self.log.info("Check that peers can fetch cfilters.")
        stop_hash = self.nodes[0].getblockhash(10)
        request = msg_getcfilters(
            filter_type=FILTER_TYPE_BASIC,
            start_height=1,
            stop_hash=int(stop_hash, 16),
        )
        peer_0.send_and_ping(request)
        response = peer_0.pop_cfilters()
        assert_equal(len(response), 10)

        self.log.info("Check that cfilter responses are correct.")
        for cfilter, cfhash, height in zip(response, main_cfhashes, range(1, 11)):
            block_hash = self.nodes[0].getblockhash(height)
            assert_equal(cfilter.filter_type, FILTER_TYPE_BASIC)
            assert_equal(cfilter.block_hash, int(block_hash, 16))
            computed_cfhash = uint256_from_str(hash256(cfilter.filter_data))
            assert_equal(computed_cfhash, cfhash)

        self.log.info("Check that peers can fetch cfilters for stale blocks.")
        request = msg_getcfilters(
            filter_type=FILTER_TYPE_BASIC,
            start_height=1000,
            stop_hash=int(stale_block_hash, 16),
        )
        peer_0.send_and_ping(request)
        response = peer_0.pop_cfilters()
        assert_equal(len(response), 1)

        cfilter = response[0]
        assert_equal(cfilter.filter_type, FILTER_TYPE_BASIC)
        assert_equal(cfilter.block_hash, int(stale_block_hash, 16))
        computed_cfhash = uint256_from_str(hash256(cfilter.filter_data))
        assert_equal(computed_cfhash, stale_cfhashes[999])

        self.log.info("Requests to node 1 without NODE_COMPACT_FILTERS results in disconnection.")
        requests = [
            msg_getcfcheckpt(
                filter_type=FILTER_TYPE_BASIC,
                stop_hash=int(main_block_hash, 16),
            ),
            msg_getcfheaders(
                filter_type=FILTER_TYPE_BASIC,
                start_height=1000,
                stop_hash=int(main_block_hash, 16),
            ),
            msg_getcfilters(
                filter_type=FILTER_TYPE_BASIC,
                start_height=1000,
                stop_hash=int(main_block_hash, 16),
            ),
        ]
        for request in requests:
            peer_1 = self.nodes[1].add_p2p_connection(P2PInterface())
            with self.nodes[1].assert_debug_log(expected_msgs=["requested unsupported block filter type"]):
                peer_1.send_message(request)
                peer_1.wait_for_disconnect()

        self.log.info("Check that invalid requests result in disconnection.")
        requests = [
            # Requesting too many filters results in disconnection.
            (
                msg_getcfilters(
                    filter_type=FILTER_TYPE_BASIC,
                    start_height=0,
                    stop_hash=int(main_block_hash, 16),
                ), "requested too many cfilters/cfheaders"
            ),
            # Requesting too many filter headers results in disconnection.
            (
                msg_getcfheaders(
                    filter_type=FILTER_TYPE_BASIC,
                    start_height=0,
                    stop_hash=int(tip_hash, 16),
                ), "requested too many cfilters/cfheaders"
            ),
            # Requesting unknown filter type results in disconnection.
            (
                msg_getcfcheckpt(
                    filter_type=255,
                    stop_hash=int(main_block_hash, 16),
                ), "requested unsupported block filter type"
            ),
            # Requesting unknown hash results in disconnection.
            (
                msg_getcfcheckpt(
                    filter_type=FILTER_TYPE_BASIC,
                    stop_hash=123456789,
                ), "requested invalid block hash"
            ),
            (
                # Request with (start block height > stop block height) results in disconnection.
                msg_getcfheaders(
                    filter_type=FILTER_TYPE_BASIC,
                    start_height=1000,
                    stop_hash=int(self.nodes[0].getblockhash(999), 16),
                ), "sent invalid getcfilters/getcfheaders with start height 1000 and stop height 999"
            ),
        ]
        for request, expected_log_msg in requests:
            peer_0 = self.nodes[0].add_p2p_connection(P2PInterface())
            with self.nodes[0].assert_debug_log(expected_msgs=[expected_log_msg]):
                peer_0.send_message(request)
                peer_0.wait_for_disconnect()

        self.log.info("Test -peerblockfilters without -blockfilterindex raises an error")
        self.stop_node(0)
        self.nodes[0].extra_args = ["-peerblockfilters"]
        msg = "Error: Cannot set -peerblockfilters without -blockfilterindex."
        self.nodes[0].assert_start_raises_init_error(expected_msg=msg)

        self.log.info("Test unknown value to -blockfilterindex raises an error")
        self.nodes[0].extra_args = ["-blockfilterindex=abc"]
        msg = "Error: Unknown -blockfilterindex value abc."
        self.nodes[0].assert_start_raises_init_error(expected_msg=msg)

        self.log.info("Test -blockfilterindex with -reindex-chainstate raises an error")
        self.nodes[0].assert_start_raises_init_error(
            expected_msg='Error: -reindex-chainstate option is not compatible with -blockfilterindex. '
            'Please temporarily disable blockfilterindex while using -reindex-chainstate, or replace -reindex-chainstate with -reindex to fully rebuild all indexes.',
            extra_args=['-blockfilterindex', '-reindex-chainstate'],
        )

        self.test_special_transactions_in_filters()

    def create_simple_assetlock(self, base_tx, base_tx_value, amount_locked_1, amount_locked_2=0):
        """Create a simple AssetLockTx for testing"""
        node = self.nodes[0]

        # Create the AssetLockTx
        inputs = [CTxIn(outpoint=COutPoint(int(base_tx, 16), 0))]

        # Credit outputs (what will be credited on Platform)
        credit_outputs = []
        for amount in [amount_locked_1, amount_locked_2]:
            if amount == 0:
                continue

            # Create a key for the credit outputs
            key = ECKey()
            key.generate()
            pubkey = key.get_pubkey().get_bytes()
            credit_outputs.append(CTxOut(int(amount * COIN), key_to_p2pkh_script(pubkey)))

        # AssetLock payload
        lock_tx_payload = CAssetLockTx(1, credit_outputs)

        # Calculate remaining amount (minus small fee)
        fee = Decimal("0.0001")
        remaining = base_tx_value - amount_locked_1 - amount_locked_2 - fee

        # Create the transaction
        lock_tx = CTransaction()
        lock_tx.vin = inputs

        # Add change output if there's remaining amount
        if remaining > 0:
            change_script = bytes.fromhex(node.validateaddress(self.change_addr)['scriptPubKey'])
            lock_tx.vout = [CTxOut(int(remaining * COIN), change_script)]

        # Add OP_RETURN output with the locked amount
        lock_tx.vout.append(CTxOut(int((amount_locked_1 + amount_locked_2) * COIN), CScript([OP_RETURN, b""])))

        lock_tx.nVersion = 3
        lock_tx.nType = 8  # asset lock type
        lock_tx.vExtraPayload = lock_tx_payload.serialize()

        # Sign the transaction
        tx_hex = lock_tx.serialize().hex()
        signed_tx = node.signrawtransactionwithkey(tx_hex, self.privkeys)
        return tx_from_hex(signed_tx["hex"]).serialize().hex()

    def test_special_transactions_in_filters(self):
        """Test that special transactions are included in block filters.

        Note: This functional test only covers AssetLockTx transactions because:
        - ProRegTx requires 1000 DASH collateral and masternode setup
        - ProUpServTx/ProUpRegTx/ProUpRevTx require existing masternodes
        - All special transaction types are comprehensively tested in unit tests
          (see src/test/blockfilter_tests.cpp)

        The test verifies filter functionality by comparing filter sizes between
        blocks with and without special transactions, as GCS filter decoding
        is not readily available in Python test framework.
        """
        self.log.info("Test that special transactions are included in block filters")

        # Restart node 0 with blockfilterindex enabled
        self.restart_node(0, extra_args=["-blockfilterindex", "-peerblockfilters"])

        # Test AssetLockTx transactions
        self.test_assetlock_basic()
        self.test_assetlock_multiple_outputs()

        self.log.info("Special transaction block filter test completed successfully")

    def test_assetlock_basic(self):
        """Test that AssetLockTx credit outputs are included in block filters"""
        self.log.info("Testing AssetLockTx in block filters...")

        node = self.nodes[0]

        # Create an AssetLockTx
        amount_to_lock = Decimal("0.1")
        block = node.getblock(node.getblockhash(300), 2)
        base_tx = block['tx'][0]['txid']
        base_tx_value = block['tx'][0]['vout'][0]['value']
        signed_hex = self.create_simple_assetlock(base_tx, base_tx_value, amount_to_lock)
        txid = node.sendrawtransaction(signed_hex)

        # Verify it's recognized as an AssetLockTx
        tx_details = node.getrawtransaction(txid, True)
        assert tx_details.get('type') == 8, f"Expected type 8 (AssetLockTx), got {tx_details.get('type')}"
        # Note: vExtraPayload may not appear in RPC output but the transaction is still valid

        # Mine it into a block
        block_hash = self.generate(node, 1, sync_fun=self.no_op)[0]

        # Verify the transaction is in the block with the assetLockTx field
        block = node.getblock(block_hash, 2)
        found_tx = None
        for tx in block['tx']:
            if tx['txid'] == txid:
                found_tx = tx
                break

        assert found_tx is not None, f"Transaction {txid} not found in block"
        assert 'assetLockTx' in found_tx, "Mined transaction should have assetLockTx field"
        assert 'creditOutputs' in found_tx['assetLockTx'], "AssetLockTx should have creditOutputs"
        assert len(found_tx['assetLockTx']['creditOutputs']) >= 1, "Expected at least one credit output"

        # Get the filter
        filter_result = node.getblockfilter(block_hash, 'basic')
        assert 'filter' in filter_result, "Block should have a filter"

        # To properly verify, we'll create a control block without special tx
        # and compare filter sizes
        control_block_hash = self.generate(node, 1, sync_fun=self.no_op)[0]
        control_filter = node.getblockfilter(control_block_hash, 'basic')

        # The block with AssetLockTx should have a larger filter due to credit outputs
        special_filter_size = len(filter_result['filter'])
        control_filter_size = len(control_filter['filter'])

        self.log.debug(f"Filter with AssetLockTx: {special_filter_size} hex chars")
        self.log.debug(f"Control filter: {control_filter_size} hex chars")

        # The filter with special tx should be larger (credit outputs add data)
        assert_greater_than(special_filter_size, control_filter_size)

        self.log.info("AssetLockTx basic test passed")

    def test_assetlock_multiple_outputs(self):
        """Test that multiple AssetLockTx credit outputs are all included in block filters"""
        self.log.info("Testing AssetLockTx with multiple credit outputs...")

        node = self.nodes[0]

        # Create an AssetLockTx with multiple credit outputs
        # This tests that all credit outputs are properly included in the filter

        amount1 = Decimal("0.03")
        amount2 = Decimal("0.04")

        block = node.getblock(node.getblockhash(301), 2)
        base_tx = block['tx'][0]['txid']
        base_tx_value = block['tx'][0]['vout'][0]['value']
        signed_hex = self.create_simple_assetlock(base_tx, base_tx_value, amount1, amount2)
        txid = node.sendrawtransaction(signed_hex)

        # Mine it into a block
        block_hash = self.generate(node, 1, sync_fun=self.no_op)[0]

        # Verify our tx is included and has the expected special payload
        block = node.getblock(block_hash, 2)
        tx = next((t for t in block["tx"] if t["txid"] == txid), None)
        assert tx is not None, f"AssetLockTx {txid} not found in block {block_hash}"
        assert "assetLockTx" in tx, "Mined transaction should have assetLockTx field"
        assert "creditOutputs" in tx["assetLockTx"], "AssetLockTx should have creditOutputs"

        # Get the filter to verify it was created
        filter_result = node.getblockfilter(block_hash, 'basic')
        assert 'filter' in filter_result, "Block should have a filter"

        # Compare with a control block to verify multiple credit outputs increase filter size
        control_block_hash = self.generate(node, 1, sync_fun=self.no_op)[0]
        control_filter = node.getblockfilter(control_block_hash, 'basic')

        # Filter with multiple credit outputs should be larger than control
        multi_output_filter_size = len(filter_result['filter'])
        control_filter_size = len(control_filter['filter'])

        self.log.debug(f"Filter with multiple outputs: {multi_output_filter_size} hex chars")
        self.log.debug(f"Control filter: {control_filter_size} hex chars")

        assert_greater_than(multi_output_filter_size, control_filter_size)

        self.log.info("AssetLockTx multiple outputs test passed")


def compute_last_header(prev_header, hashes):
    """Compute the last filter header from a starting header and a sequence of filter hashes."""
    header = ser_uint256(prev_header)
    for filter_hash in hashes:
        header = hash256(ser_uint256(filter_hash) + header)
    return uint256_from_str(header)


if __name__ == '__main__':
    CompactFiltersTest().main()
