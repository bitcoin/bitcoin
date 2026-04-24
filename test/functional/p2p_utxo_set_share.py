#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test P2P UTXO set sharing."""

import hashlib
import os
import shutil

from test_framework.messages import (
    hash256,
    msg_getutxoset,
    msg_getutxostinf,
    ser_uint256,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet

# Using height 299 because it has a valid assumeutxo param
START_HEIGHT = 199
SNAPSHOT_HEIGHT = 299
CHUNK_SIZE = 3_900_000


def verify_merkle_proof(leaf_hash, proof, chunk_index, total_chunks, merkle_root):
    computed = leaf_hash
    index = chunk_index
    for sibling in proof:
        if index & 1:
            computed = hash256(ser_uint256(sibling) + ser_uint256(computed))
        else:
            computed = hash256(ser_uint256(computed) + ser_uint256(sibling))
        computed = int.from_bytes(computed, 'little')
        index >>= 1
    return computed == merkle_root


class UTXOSetShareTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [[], []]

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        self.start_nodes(extra_args=self.extra_args)

    def run_test(self):
        n0 = self.nodes[0]
        n1 = self.nodes[1]
        wallet = MiniWallet(n0)

        # Generate blocks to SNAPSHOT_HEIGHT so blockhash matches the hardcoded
        # assumeutxo parameters for height 299.
        for n in self.nodes:
            n.setmocktime(n.getblockheader(n.getbestblockhash())['time'])

        assert_equal(n0.getblockcount(), START_HEIGHT)
        for i in range(100):
            if i % 3 == 0:
                wallet.send_self_transfer(from_node=n0)
            self.generate(n0, nblocks=1, sync_fun=self.no_op)
            if i == 4:
                temp_invalid = n0.getbestblockhash()
                n0.invalidateblock(temp_invalid)
                self.generateblock(n0, output="raw(aaaa)", transactions=[], sync_fun=self.no_op)
                stale_hash = n0.getbestblockhash()
                n0.invalidateblock(stale_hash)
                n0.reconsiderblock(temp_invalid)
        assert_equal(n0.getblockcount(), SNAPSHOT_HEIGHT)

        # Create UTXO snapshot and place in share directory
        dump_output = n0.dumptxoutset("utxos.dat", "latest")
        snapshot_path = dump_output['path']

        share_dir = os.path.join(n0.datadir_path, "regtest", "share")
        os.makedirs(share_dir, exist_ok=True)
        shutil.copy(snapshot_path, os.path.join(share_dir, "utxos.dat"))

        # Restart serving node to pick up the snapshot. We also start the
        # node in prune mode to have it signal NODE_NETWORK_LIMITED. This
        # means the downloading node won't sync any blocks from the sharing
        # nodes after activating the downloaded snapshot. This avoids a
        # potential race between downloading and validating the (very short
        # chain) and activating the downloaded snapshot.
        self.restart_node(0, extra_args=["-prune=1"])
        n0.setmocktime(n0.getblockheader(n0.getbestblockhash())['time'])

        self.log.info("Test NODE_UTXO_SET service bit is advertised")
        services = n0.getnetworkinfo()['localservicesnames']
        assert 'UTXO_SET' in services

        # Connect peer and request utxosetinfo
        peer = n0.add_p2p_connection(P2PInterface())
        peer.send_and_ping(msg_getutxostinf())

        self.log.info("Test utxosetinfo response is valid")
        assert 'utxosetinfo' in peer.last_message
        info_msg = peer.last_message['utxosetinfo']
        assert_greater_than(len(info_msg.entries), 0)

        entry = info_msg.entries[0]
        assert_equal(entry.height, SNAPSHOT_HEIGHT)
        assert_greater_than(entry.data_length, 0)

        self.log.info("Test requesting all chunks and verifying Merkle proofs")
        total_chunks = (entry.data_length + CHUNK_SIZE - 1) // CHUNK_SIZE
        reassembled = b""

        for i in range(total_chunks):
            req = msg_getutxoset(height=entry.height, block_hash=entry.block_hash, chunk_index=i)
            peer.send_and_ping(req)
            assert 'utxoset' in peer.last_message
            chunk_msg = peer.last_message['utxoset']

            assert_equal(chunk_msg.height, entry.height)
            assert_equal(chunk_msg.block_hash, entry.block_hash)
            assert_equal(chunk_msg.chunk_index, i)
            assert_greater_than(len(chunk_msg.data), 0)

            if i < total_chunks - 1:
                assert_equal(len(chunk_msg.data), CHUNK_SIZE)

            leaf_hash = int.from_bytes(hash256(chunk_msg.data), 'little')
            assert verify_merkle_proof(leaf_hash, chunk_msg.proof_hashes, i, total_chunks, entry.merkle_root)

            reassembled += chunk_msg.data

        self.log.info("Test reassembled data matches original snapshot")
        with open(snapshot_path, 'rb') as f:
            original = f.read()
        assert_equal(len(reassembled), len(original))
        assert_equal(hashlib.sha256(reassembled).hexdigest(), hashlib.sha256(original).hexdigest())

        self.log.info("Test invalid chunk index causes disconnect")
        peer2 = n0.add_p2p_connection(P2PInterface())
        bad_req = msg_getutxoset(height=entry.height, block_hash=entry.block_hash, chunk_index=total_chunks)
        peer2.send_without_ping(bad_req)
        peer2.wait_for_disconnect()

        self.log.info("Test wrong block hash causes disconnect")
        peer3 = n0.add_p2p_connection(P2PInterface())
        bad_req = msg_getutxoset(height=entry.height, block_hash=0xdeadbeef, chunk_index=0)
        peer3.send_without_ping(bad_req)
        peer3.wait_for_disconnect()

        self.log.info("Test wrong height causes disconnect")
        peer4 = n0.add_p2p_connection(P2PInterface())
        bad_req = msg_getutxoset(height=entry.height + 1, block_hash=entry.block_hash, chunk_index=0)
        peer4.send_without_ping(bad_req)
        peer4.wait_for_disconnect()

        self.log.info("Test node without snapshot does not advertise NODE_UTXO_SET")
        n1_services = n1.getnetworkinfo()['localservicesnames']
        assert 'UTXO_SET' not in n1_services

        self.log.info("Test getutxostinf to non-serving node gets no response")
        peer5 = n1.add_p2p_connection(P2PInterface())
        peer5.send_and_ping(msg_getutxostinf())
        assert 'utxosetinfo' not in peer5.last_message

        self.log.info("Test downloadutxoset with invalid height")
        assert_raises_rpc_error(-8, "No assumeutxo data for height", n1.downloadutxoset, 12345)

        # Transfer headers to downloading node so it can activate snapshot
        for i in range(1, SNAPSHOT_HEIGHT + 1):
            block = n0.getblock(n0.getblockhash(i), 0)
            n1.submitheader(block)
        assert_equal(n1.getblockchaininfo()["headers"], SNAPSHOT_HEIGHT)

        self.log.info("Test full P2P download via downloadutxoset RPC")
        result = n1.downloadutxoset(SNAPSHOT_HEIGHT)
        assert_equal(result['height'], SNAPSHOT_HEIGHT)

        self.log.info("Test duplicate downloadutxoset is rejected")
        assert_raises_rpc_error(-1, "Failed to start download. A download may already be in progress.", n1.downloadutxoset, SNAPSHOT_HEIGHT)

        # Connect nodes so download can proceed
        self.connect_nodes(0, 1)
        self.wait_until(lambda: len(n1.getchainstates()['chainstates']) == 2, timeout=30)


if __name__ == '__main__':
    UTXOSetShareTest(__file__).main()
