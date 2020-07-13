#!/usr/bin/env python3
# Copyright (c) 2015-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the ZMQ notification interface."""
import struct

from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import CTransaction, hash256
from test_framework.util import assert_equal, connect_nodes
from io import BytesIO
from time import sleep

def hash256_reversed(byte_str):
    return hash256(byte_str)[::-1]

class ZMQSubscriber:
    def __init__(self, socket, topic):
        self.sequence = 0
        self.socket = socket
        self.topic = topic

        import zmq
        self.socket.setsockopt(zmq.SUBSCRIBE, self.topic)

    def receive(self):
        topic, body, seq = self.socket.recv_multipart()
        # Topic should match the subscriber topic.
        assert_equal(topic, self.topic)
        # Sequence should be incremental.
        assert_equal(struct.unpack('<I', seq)[-1], self.sequence)
        self.sequence += 1
        return body


class ZMQTest (BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_py3_zmq()
        self.skip_if_no_bitcoind_zmq()

    def run_test(self):
        import zmq
        self.ctx = zmq.Context()
        try:
            self.test_basic()
            self.test_reorg()
        finally:
            # Destroy the ZMQ context.
            self.log.debug("Destroying ZMQ context")
            self.ctx.destroy(linger=None)

    def test_basic(self):
        import zmq

        # Invalid zmq arguments don't take down the node, see #17185.
        self.restart_node(0, ["-zmqpubrawtx=foo", "-zmqpubhashtx=bar"])

        address = 'tcp://127.0.0.1:28332'
        sockets = []
        subs = []
        services = [b"hashblock", b"hashtx", b"rawblock", b"rawtx"]
        for service in services:
            sockets.append(self.ctx.socket(zmq.SUB))
            sockets[-1].set(zmq.RCVTIMEO, 60000)
            subs.append(ZMQSubscriber(sockets[-1], service))

        # Subscribe to all available topics.
        hashblock = subs[0]
        hashtx = subs[1]
        rawblock = subs[2]
        rawtx = subs[3]

        self.restart_node(0, ["-zmqpub%s=%s" % (sub.topic.decode(), address) for sub in [hashblock, hashtx, rawblock, rawtx]])
        connect_nodes(self.nodes[0], 1)
        for socket in sockets:
            socket.connect(address)

        # Relax so that the subscriber is ready before publishing zmq messages
        sleep(0.2)

        num_blocks = 5
        self.log.info("Generate %(n)d blocks (and %(n)d coinbase txes)" % {"n": num_blocks})
        genhashes = self.nodes[0].generatetoaddress(num_blocks, ADDRESS_BCRT1_UNSPENDABLE)

        self.sync_all()

        for x in range(num_blocks):
            # Should receive the coinbase txid.
            txid = hashtx.receive()

            # Should receive the coinbase raw transaction.
            hex = rawtx.receive()
            tx = CTransaction()
            tx.deserialize(BytesIO(hex))
            tx.calc_sha256()
            assert_equal(tx.hash, txid.hex())

            # Should receive the generated raw block.
            block = rawblock.receive()
            assert_equal(genhashes[x], hash256_reversed(block[:80]).hex())

            # Should receive the generated block hash.
            hash = hashblock.receive().hex()
            assert_equal(genhashes[x], hash)
            # The block should only have the coinbase txid.
            assert_equal([txid.hex()], self.nodes[1].getblock(hash)["tx"])


        if self.is_wallet_compiled():
            self.log.info("Wait for tx from second node")
            payment_txid = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1.0)
            self.sync_all()

            # Should receive the broadcasted txid.
            txid = hashtx.receive()
            assert_equal(payment_txid, txid.hex())

            # Should receive the broadcasted raw transaction.
            hex = rawtx.receive()
            assert_equal(payment_txid, hash256_reversed(hex).hex())

            # Mining the block with this tx should result in second notification
            # after coinbase tx notification
            self.nodes[0].generatetoaddress(1, ADDRESS_BCRT1_UNSPENDABLE)
            hashtx.receive()
            txid = hashtx.receive()
            assert_equal(payment_txid, txid.hex())


        self.log.info("Test the getzmqnotifications RPC")
        assert_equal(self.nodes[0].getzmqnotifications(), [
            {"type": "pubhashblock", "address": address, "hwm": 1000},
            {"type": "pubhashtx", "address": address, "hwm": 1000},
            {"type": "pubrawblock", "address": address, "hwm": 1000},
            {"type": "pubrawtx", "address": address, "hwm": 1000},
        ])

        assert_equal(self.nodes[1].getzmqnotifications(), [])

    def test_reorg(self):
        if not self.is_wallet_compiled():
            self.log.info("Skipping reorg test because wallet is disabled")
            return

        import zmq
        address = 'tcp://127.0.0.1:28333'

        services = [b"hashblock", b"hashtx"]
        sockets = []
        subs = []
        for service in services:
            sockets.append(self.ctx.socket(zmq.SUB))
            # 2 second timeout to check end of notifications
            sockets[-1].set(zmq.RCVTIMEO, 2000)
            subs.append(ZMQSubscriber(sockets[-1], service))

        # Subscribe to all available topics.
        hashblock = subs[0]
        hashtx = subs[1]

        # Should only notify the tip if a reorg occurs
        self.restart_node(0, ["-zmqpub%s=%s" % (sub.topic.decode(), address) for sub in [hashblock, hashtx]])
        for socket in sockets:
            socket.connect(address)
        # Relax so that the subscriber is ready before publishing zmq messages
        sleep(0.2)

        # Generate 1 block in nodes[0] with 1 mempool tx and receive all notifications
        payment_txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1.0)
        disconnect_block = self.nodes[0].generatetoaddress(1, ADDRESS_BCRT1_UNSPENDABLE)[0]
        disconnect_cb = self.nodes[0].getblock(disconnect_block)["tx"][0]
        assert_equal(self.nodes[0].getbestblockhash(), hashblock.receive().hex())
        assert_equal(hashtx.receive().hex(), payment_txid)
        assert_equal(hashtx.receive().hex(), disconnect_cb)

        # Generate 2 blocks in nodes[1]
        connect_blocks = self.nodes[1].generatetoaddress(2, ADDRESS_BCRT1_UNSPENDABLE)

        # nodes[0] will reorg chain after connecting back nodes[1]
        connect_nodes(self.nodes[0], 1)
        self.sync_blocks() # tx in mempool valid but not advertised

        # Should receive nodes[1] tip
        assert_equal(self.nodes[1].getbestblockhash(), hashblock.receive().hex())

        # During reorg:
        # Get old payment transaction notification from disconnect and disconnected cb
        assert_equal(hashtx.receive().hex(), payment_txid)
        assert_equal(hashtx.receive().hex(), disconnect_cb)
        # And the payment transaction again due to mempool entry
        assert_equal(hashtx.receive().hex(), payment_txid)
        assert_equal(hashtx.receive().hex(), payment_txid)
        # And the new connected coinbases
        for i in [0, 1]:
            assert_equal(hashtx.receive().hex(), self.nodes[1].getblock(connect_blocks[i])["tx"][0])

        # If we do a simple invalidate we announce the disconnected coinbase
        self.nodes[0].invalidateblock(connect_blocks[1])
        assert_equal(hashtx.receive().hex(), self.nodes[1].getblock(connect_blocks[1])["tx"][0])
        # And the current tip
        assert_equal(hashtx.receive().hex(), self.nodes[1].getblock(connect_blocks[0])["tx"][0])

if __name__ == '__main__':
    ZMQTest().main()
