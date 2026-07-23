#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license.

"""
Ensures that UTXO unserialization errors abort the node, and does not
cause a consensus divergence.

A valid UTXO is created and shared between two nodes. The raw database
entry is then deliberately modified on one node so it can no longer be
deserialized. When the other node spends that UTXO and mines a block,
the node with the unserializable entry must abort during block connection
rather than silently treating the coin as absent and marking the block
BLOCK_FAILED_VALID, which would cause it to permanently diverge from the
network's best chain.
"""

try:
    import plyvel  # type: ignore[import]
except ImportError:
    plyvel = None

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class UTXOAbortOnErrorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        if plyvel is None:
            raise SkipTest("plyvel not available (pip install plyvel)")

    def setup_network(self):
        self.setup_nodes() # Start with nodes disconnected

    def run_test(self):
        node0, node1 = self.nodes

        self.log.info("Mining mature coinbase on node0")
        wallet0 = MiniWallet(node0)
        self.generate(wallet0, COINBASE_MATURITY + 1, sync_fun=self.no_op)
        assert_equal(node0.getblockcount(), COINBASE_MATURITY + 1)

        # The coinbase of block 1 is now mature. This is the UTXO we will
        # make unserializable on node0 and spend on node1.
        coinbase_txid = node0.getblock(node0.getblockhash(1))['tx'][0]
        assert node0.gettxout(coinbase_txid, 0) is not None, f"Expected UTXO {coinbase_txid}:0 to exist in UTXO set"

        self.log.info("Preparing a spend of the mature coinbase UTXO (not broadcast)")
        utxo = wallet0.get_utxo(txid=coinbase_txid, vout=0, mark_as_spent=False)
        spend_tx_hex = wallet0.create_self_transfer(utxo_to_spend=utxo)['hex']

        self.log.info("Sync node1 up to the tip, then isolate nodes")
        self.connect_nodes(0, 1)
        self.sync_blocks()
        assert_equal(node1.getblockcount(), COINBASE_MATURITY + 1)
        self.disconnect_nodes(0, 1)

        self.log.info("Make UTXO unserializable in node0 database")
        self.stop_node(0)

        # LevelDB key for the CoinEntry serialization:
        # key = DB_COIN (0x43='C') || txid (32 bytes) || VARINT(vout=0)
        coin_key = b'\x43' + bytes.fromhex(coinbase_txid)[::-1] + b'\x00'
        chainstate_path = str(node0.chain_path / "chainstate")

        # Update entry to mimic an incompatible serialization format
        with plyvel.DB(chainstate_path, create_if_missing=False, compression=None) as db:
            existing_value = db.get(coin_key)
            assert existing_value is not None, f"UTXO {coinbase_txid}:0 not found in db"

            # Write a single-byte value. After XOR deobfuscation this is still just one
            # byte, far too short for a valid Coin (which needs height VARINT + amount
            # VARINT + script at minimum). Deserialization will throw "end of data" when
            # trying to read beyond the first field.
            db.put(coin_key, b'\x00')

        self.log.info("Restart node0 — the unserializable entry is only visible during block validation, not at startup")
        self.start_node(0)

        # Individual coin values are not read at startup; only block validation
        # touches them. The node starts cleanly regardless of the tampered entry.
        assert_equal(node0.getblockcount(), COINBASE_MATURITY + 1)

        # Now spend the corrupted UTXO
        self.log.info("node1 broadcasts the spend and mines a block")
        node1.sendrawtransaction(spend_tx_hex)
        spending_block_hash = self.generate(node1, 1, sync_fun=self.no_op)[0]
        assert_equal(node1.getblockcount(), COINBASE_MATURITY + 2)

        self.log.info("Connect node0 to node1 — node0 must abort when it tries to connect the spending block")
        # Verify that the unserializable entry triggers the expected error and is
        # never silently misreported as a missing input (bad-txns-inputs-missingorspent),
        # which would indicate the previous silent-divergence behaviour.
        with node0.assert_debug_log(expected_msgs=["Error reading from database: Coin deserialization failure"],
                                    unexpected_msgs=["bad-txns-inputs-missingorspent"], timeout=60):
            try:
                self.connect_nodes(0, 1)
            except Exception:
                pass # node0 may validly abort before connect_nodes returns

        # Confirm node0 aborted with SIGABRT
        self.wait_until(lambda: self.nodes[0].is_node_stopped(
            expected_ret_code=-6,
            expected_stderr="Error: Error reading from database, shutting down.",
        ))
        self.log.info("node0 aborted cleanly — no silent divergence occurred")

        self.log.info("Restart node0 and verify the spending block was not permanently marked invalid")
        self.start_node(0)
        assert_equal(node0.getblockcount(), COINBASE_MATURITY + 1)

        # If BLOCK_FAILED_VALID had been written to disk, the node would be
        # permanently stuck on a stale tip even after the tampered entry is resolved.
        tips = node0.getchaintips()
        permanently_invalid = [
            t for t in tips
            if t['hash'] == spending_block_hash and t['status'] == 'invalid'
        ]
        assert len(permanently_invalid) == 0, f"Spending block {spending_block_hash} must not be marked BLOCK_FAILED_VALID"


if __name__ == '__main__':
    UTXOAbortOnErrorTest(__file__).main()
