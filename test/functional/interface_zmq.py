#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the ZMQ notification interface."""
import struct

from test_framework.address import (
    ADDRESS_BCRT1_P2WSH_OP_TRUE,
    ADDRESS_BCRT1_UNSPENDABLE,
)
from test_framework.blocktools import (
    add_witness_commitment,
    create_block,
    create_coinbase,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import (
    CTransaction,
    hash256,
    tx_from_hex,
)
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.netutil import test_ipv6_local
from io import BytesIO
from time import sleep

# Test may be skipped and not have zmq installed
try:
    import zmq
except ImportError:
    pass

def hash256_reversed(byte_str):
    return hash256(byte_str)[::-1]

class ZMQSubscriber:
    def __init__(self, socket, topic):
        self.sequence = None  # no sequence number received yet
        self.socket = socket
        self.topic = topic

        self.socket.setsockopt(zmq.SUBSCRIBE, self.topic)

    # Receive message from publisher and verify that topic and sequence match
    def _receive_from_publisher_and_check(self):
        topic, body, seq = self.socket.recv_multipart()
        # Topic should match the subscriber topic.
        assert_equal(topic, self.topic)
        # Sequence should be incremental.
        received_seq = struct.unpack('<I', seq)[-1]
        if self.sequence is None:
            self.sequence = received_seq
        else:
            assert_equal(received_seq, self.sequence)
        self.sequence += 1
        return body

    def receive(self):
        return self._receive_from_publisher_and_check()

    def receive_sequence(self):
        body = self._receive_from_publisher_and_check()
        hash = body[:32].hex()
        label = chr(body[32])
        mempool_sequence = None if len(body) != 32+1+8 else struct.unpack("<Q", body[32+1:])[0]
        if mempool_sequence is not None:
            assert label == "A" or label == "R"
        else:
            assert label == "D" or label == "C"
        return (hash, label, mempool_sequence)


class ZMQTestSetupBlock:
    """Helper class for setting up a ZMQ test via the "sync up" procedure.
    Generates a block on the specified node on instantiation and provides a
    method to check whether a ZMQ notification matches, i.e. the event was
    caused by this generated block.  Assumes that a notification either contains
    the generated block's hash, it's (coinbase) transaction id, the raw block or
    raw transaction data.
    """

    def __init__(self, test_framework, node):
        self.block_hash = test_framework.generate(node, 1)[0]
        coinbase = node.getblock(self.block_hash, 2)['tx'][0]
        self.tx_hash = coinbase['txid']
        self.raw_tx = coinbase['hex']
        self.raw_block = node.getblock(self.block_hash, 0)

    def caused_notification(self, notification):
        return (
            self.block_hash in notification
            or self.tx_hash in notification
            or self.raw_block in notification
            or self.raw_tx in notification
        )


class ZMQTest (BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        if self.is_wallet_compiled():
            self.requires_wallet = True
        # This test isn't testing txn relay/timing, so set whitelist on the
        # peers for instant txn relay. This speeds up the test run time 2-3x.
        self.extra_args = [["-whitelist=noban@127.0.0.1"]] * self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_py3_zmq()
        self.skip_if_no_bitcoind_zmq()

    def run_test(self):
        self.ctx = zmq.Context()
        try:
            self.test_basic()
            self.test_sequence()
            self.test_mempool_sync()
            self.test_reorg()
            self.test_multiple_interfaces()
            self.test_ipv6()
        finally:
            # Destroy the ZMQ context.
            self.log.debug("Destroying ZMQ context")
            self.ctx.destroy(linger=None)

    # Restart node with the specified zmq notifications enabled, subscribe to
    # all of them and return the corresponding ZMQSubscriber objects.
    def setup_zmq_test(self, services, *, recv_timeout=60, sync_blocks=True, ipv6=False):
        subscribers = []
        for topic, address in services:
            socket = self.ctx.socket(zmq.SUB)
            if ipv6:
                socket.setsockopt(zmq.IPV6, 1)
            subscribers.append(ZMQSubscriber(socket, topic.encode()))

        self.restart_node(0, [f"-zmqpub{topic}={address}" for topic, address in services] +
                             self.extra_args[0])

        for i, sub in enumerate(subscribers):
            sub.socket.connect(services[i][1])

        # Ensure that all zmq publisher notification interfaces are ready by
        # running the following "sync up" procedure:
        #   1. Generate a block on the node
        #   2. Try to receive the corresponding notification on all subscribers
        #   3. If all subscribers get the message within the timeout (1 second),
        #      we are done, otherwise repeat starting from step 1
        for sub in subscribers:
            sub.socket.set(zmq.RCVTIMEO, 1000)
        while True:
            test_block = ZMQTestSetupBlock(self, self.nodes[0])
            recv_failed = False
            for sub in subscribers:
                try:
                    while not test_block.caused_notification(sub.receive().hex()):
                        self.log.debug("Ignoring sync-up notification for previously generated block.")
                except zmq.error.Again:
                    self.log.debug("Didn't receive sync-up notification, trying again.")
                    recv_failed = True
            if not recv_failed:
                self.log.debug("ZMQ sync-up completed, all subscribers are ready.")
                break

        # set subscriber's desired timeout for the test
        for sub in subscribers:
            sub.socket.set(zmq.RCVTIMEO, recv_timeout*1000)

        self.connect_nodes(0, 1)
        if sync_blocks:
            self.sync_blocks()

        return subscribers

    def test_basic(self):

        # Invalid zmq arguments don't take down the node, see #17185.
        self.restart_node(0, ["-zmqpubrawtx=foo", "-zmqpubhashtx=bar"])

        address = 'tcp://127.0.0.1:28332'
        subs = self.setup_zmq_test([(topic, address) for topic in ["hashblock", "hashtx", "rawblock", "rawtx"]])

        hashblock = subs[0]
        hashtx = subs[1]
        rawblock = subs[2]
        rawtx = subs[3]

        num_blocks = 5
        self.log.info(f"Generate {num_blocks} blocks (and {num_blocks} coinbase txes)")
        genhashes = self.generatetoaddress(self.nodes[0], num_blocks, ADDRESS_BCRT1_UNSPENDABLE)

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
            self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)
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

        address = 'tcp://127.0.0.1:28333'

        # Should only notify the tip if a reorg occurs
        hashblock, hashtx = self.setup_zmq_test(
            [(topic, address) for topic in ["hashblock", "hashtx"]],
            recv_timeout=2)  # 2 second timeout to check end of notifications
        self.disconnect_nodes(0, 1)

        # Generate 1 block in nodes[0] with 1 mempool tx and receive all notifications
        payment_txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1.0)
        disconnect_block = self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)[0]
        disconnect_cb = self.nodes[0].getblock(disconnect_block)["tx"][0]
        assert_equal(self.nodes[0].getbestblockhash(), hashblock.receive().hex())
        assert_equal(hashtx.receive().hex(), payment_txid)
        assert_equal(hashtx.receive().hex(), disconnect_cb)

        # Generate 2 blocks in nodes[1] to a different address to ensure split
        connect_blocks = self.generatetoaddress(self.nodes[1], 2, ADDRESS_BCRT1_P2WSH_OP_TRUE)

        # nodes[0] will reorg chain after connecting back nodes[1]
        self.connect_nodes(0, 1)
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

    def test_sequence(self):
        """
        Sequence zmq notifications give every blockhash and txhash in order
        of processing, regardless of IBD, re-orgs, etc.
        Format of messages:
        <32-byte hash>C :                 Blockhash connected
        <32-byte hash>D :                 Blockhash disconnected
        <32-byte hash>R<8-byte LE uint> : Transactionhash removed from mempool for non-block inclusion reason
        <32-byte hash>A<8-byte LE uint> : Transactionhash added mempool
        """
        self.log.info("Testing 'sequence' publisher")
        [seq] = self.setup_zmq_test([("sequence", "tcp://127.0.0.1:28333")])
        self.disconnect_nodes(0, 1)

        # Mempool sequence number starts at 1
        seq_num = 1

        # Generate 1 block in nodes[0] and receive all notifications
        dc_block = self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)[0]

        # Note: We are not notified of any block transactions, coinbase or mined
        assert_equal((self.nodes[0].getbestblockhash(), "C", None), seq.receive_sequence())

        # Generate 2 blocks in nodes[1] to a different address to ensure a chain split
        self.generatetoaddress(self.nodes[1], 2, ADDRESS_BCRT1_P2WSH_OP_TRUE)

        # nodes[0] will reorg chain after connecting back nodes[1]
        self.connect_nodes(0, 1)

        # Then we receive all block (dis)connect notifications for the 2 block reorg
        assert_equal((dc_block, "D", None), seq.receive_sequence())
        block_count = self.nodes[1].getblockcount()
        assert_equal((self.nodes[1].getblockhash(block_count-1), "C", None), seq.receive_sequence())
        assert_equal((self.nodes[1].getblockhash(block_count), "C", None), seq.receive_sequence())

        # Rest of test requires wallet functionality
        if self.is_wallet_compiled():
            self.log.info("Wait for tx from second node")
            payment_txid = self.nodes[1].sendtoaddress(address=self.nodes[0].getnewaddress(), amount=5.0, replaceable=True)
            self.sync_all()
            self.log.info("Testing sequence notifications with mempool sequence values")

            # Should receive the broadcasted txid.
            assert_equal((payment_txid, "A", seq_num), seq.receive_sequence())
            seq_num += 1

            self.log.info("Testing RBF notification")
            # Replace it to test eviction/addition notification
            rbf_info = self.nodes[1].bumpfee(payment_txid)
            self.sync_all()
            assert_equal((payment_txid, "R", seq_num), seq.receive_sequence())
            seq_num += 1
            assert_equal((rbf_info["txid"], "A", seq_num), seq.receive_sequence())
            seq_num += 1

            # Doesn't get published when mined, make a block and tx to "flush" the possibility
            # though the mempool sequence number does go up by the number of transactions
            # removed from the mempool by the block mining it.
            mempool_size = len(self.nodes[0].getrawmempool())
            c_block = self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)[0]
            self.sync_all()
            # Make sure the number of mined transactions matches the number of txs out of mempool
            mempool_size_delta = mempool_size - len(self.nodes[0].getrawmempool())
            assert_equal(len(self.nodes[0].getblock(c_block)["tx"])-1, mempool_size_delta)
            seq_num += mempool_size_delta
            payment_txid_2 = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1.0)
            self.sync_all()
            assert_equal((c_block, "C", None), seq.receive_sequence())
            assert_equal((payment_txid_2, "A", seq_num), seq.receive_sequence())
            seq_num += 1

            # Spot check getrawmempool results that they only show up when asked for
            assert type(self.nodes[0].getrawmempool()) is list
            assert type(self.nodes[0].getrawmempool(mempool_sequence=False)) is list
            assert "mempool_sequence" not in self.nodes[0].getrawmempool(verbose=True)
            assert_raises_rpc_error(-8, "Verbose results cannot contain mempool sequence values.", self.nodes[0].getrawmempool, True, True)
            assert_equal(self.nodes[0].getrawmempool(mempool_sequence=True)["mempool_sequence"], seq_num)

            self.log.info("Testing reorg notifications")
            # Manually invalidate the last block to test mempool re-entry
            # N.B. This part could be made more lenient in exact ordering
            # since it greatly depends on inner-workings of blocks/mempool
            # during "deep" re-orgs. Probably should "re-construct"
            # blockchain/mempool state from notifications instead.
            block_count = self.nodes[0].getblockcount()
            best_hash = self.nodes[0].getbestblockhash()
            self.nodes[0].invalidateblock(best_hash)
            sleep(2)  # Bit of room to make sure transaction things happened

            # Make sure getrawmempool mempool_sequence results aren't "queued" but immediately reflective
            # of the time they were gathered.
            assert self.nodes[0].getrawmempool(mempool_sequence=True)["mempool_sequence"] > seq_num

            assert_equal((best_hash, "D", None), seq.receive_sequence())
            assert_equal((rbf_info["txid"], "A", seq_num), seq.receive_sequence())
            seq_num += 1

            # Other things may happen but aren't wallet-deterministic so we don't test for them currently
            self.nodes[0].reconsiderblock(best_hash)
            self.generatetoaddress(self.nodes[1], 1, ADDRESS_BCRT1_UNSPENDABLE)
            self.sync_all()

            self.log.info("Evict mempool transaction by block conflict")
            orig_txid = self.nodes[0].sendtoaddress(address=self.nodes[0].getnewaddress(), amount=1.0, replaceable=True)

            # More to be simply mined
            more_tx = []
            for _ in range(5):
                more_tx.append(self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.1))

            raw_tx = self.nodes[0].getrawtransaction(orig_txid)
            bump_info = self.nodes[0].bumpfee(orig_txid)
            # Mine the pre-bump tx
            block = create_block(int(self.nodes[0].getbestblockhash(), 16), create_coinbase(self.nodes[0].getblockcount()+1))
            tx = tx_from_hex(raw_tx)
            block.vtx.append(tx)
            for txid in more_tx:
                tx = tx_from_hex(self.nodes[0].getrawtransaction(txid))
                block.vtx.append(tx)
            add_witness_commitment(block)
            block.solve()
            assert_equal(self.nodes[0].submitblock(block.serialize().hex()), None)
            tip = self.nodes[0].getbestblockhash()
            assert_equal(int(tip, 16), block.sha256)
            orig_txid_2 = self.nodes[0].sendtoaddress(address=self.nodes[0].getnewaddress(), amount=1.0, replaceable=True)

            # Flush old notifications until evicted tx original entry
            (hash_str, label, mempool_seq) = seq.receive_sequence()
            while hash_str != orig_txid:
                (hash_str, label, mempool_seq) = seq.receive_sequence()
            mempool_seq += 1

            # Added original tx
            assert_equal(label, "A")
            # More transactions to be simply mined
            for i in range(len(more_tx)):
                assert_equal((more_tx[i], "A", mempool_seq), seq.receive_sequence())
                mempool_seq += 1
            # Bumped by rbf
            assert_equal((orig_txid, "R", mempool_seq), seq.receive_sequence())
            mempool_seq += 1
            assert_equal((bump_info["txid"], "A", mempool_seq), seq.receive_sequence())
            mempool_seq += 1
            # Conflict announced first, then block
            assert_equal((bump_info["txid"], "R", mempool_seq), seq.receive_sequence())
            mempool_seq += 1
            assert_equal((tip, "C", None), seq.receive_sequence())
            mempool_seq += len(more_tx)
            # Last tx
            assert_equal((orig_txid_2, "A", mempool_seq), seq.receive_sequence())
            mempool_seq += 1
            self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)
            self.sync_all()  # want to make sure we didn't break "consensus" for other tests

    def test_mempool_sync(self):
        """
        Use sequence notification plus getrawmempool sequence results to "sync mempool"
        """
        if not self.is_wallet_compiled():
            self.log.info("Skipping mempool sync test")
            return

        self.log.info("Testing 'mempool sync' usage of sequence notifier")
        [seq] = self.setup_zmq_test([("sequence", "tcp://127.0.0.1:28333")])

        # In-memory counter, should always start at 1
        next_mempool_seq = self.nodes[0].getrawmempool(mempool_sequence=True)["mempool_sequence"]
        assert_equal(next_mempool_seq, 1)

        # Some transactions have been happening but we aren't consuming zmq notifications yet
        # or we lost a ZMQ message somehow and want to start over
        txids = []
        num_txs = 5
        for _ in range(num_txs):
            txids.append(self.nodes[1].sendtoaddress(address=self.nodes[0].getnewaddress(), amount=1.0, replaceable=True))
        self.sync_all()

        # 1) Consume backlog until we get a mempool sequence number
        (hash_str, label, zmq_mem_seq) = seq.receive_sequence()
        while zmq_mem_seq is None:
                (hash_str, label, zmq_mem_seq) = seq.receive_sequence()

        assert label == "A" or label == "R"
        assert hash_str is not None

        # 2) We need to "seed" our view of the mempool
        mempool_snapshot = self.nodes[0].getrawmempool(mempool_sequence=True)
        mempool_view = set(mempool_snapshot["txids"])
        get_raw_seq = mempool_snapshot["mempool_sequence"]
        assert_equal(get_raw_seq, 6)
        # Snapshot may be too old compared to zmq message we read off latest
        while zmq_mem_seq >= get_raw_seq:
            sleep(2)
            mempool_snapshot = self.nodes[0].getrawmempool(mempool_sequence=True)
            mempool_view = set(mempool_snapshot["txids"])
            get_raw_seq = mempool_snapshot["mempool_sequence"]

        # Things continue to happen in the "interim" while waiting for snapshot results
        # We have node 0 do all these to avoid p2p races with RBF announcements
        for _ in range(num_txs):
            txids.append(self.nodes[0].sendtoaddress(address=self.nodes[0].getnewaddress(), amount=0.1, replaceable=True))
        self.nodes[0].bumpfee(txids[-1])
        self.sync_all()
        self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)
        final_txid = self.nodes[0].sendtoaddress(address=self.nodes[0].getnewaddress(), amount=0.1, replaceable=True)

        # 3) Consume ZMQ backlog until we get to "now" for the mempool snapshot
        while True:
            if zmq_mem_seq == get_raw_seq - 1:
                break
            (hash_str, label, mempool_sequence) = seq.receive_sequence()
            if mempool_sequence is not None:
                zmq_mem_seq = mempool_sequence
                if zmq_mem_seq > get_raw_seq:
                    raise Exception(f"We somehow jumped mempool sequence numbers! zmq_mem_seq: {zmq_mem_seq} > get_raw_seq: {get_raw_seq}")

        # 4) Moving forward, we apply the delta to our local view
        #    remaining txs(5) + 1 rbf(A+R) + 1 block connect + 1 final tx
        expected_sequence = get_raw_seq
        r_gap = 0
        for _ in range(num_txs + 2 + 1 + 1):
            (hash_str, label, mempool_sequence) = seq.receive_sequence()
            if mempool_sequence is not None:
                if mempool_sequence != expected_sequence:
                    # Detected "R" gap, means this a conflict eviction, and mempool tx are being evicted before its
                    # position in the incoming block message "C"
                    if label == "R":
                        assert mempool_sequence > expected_sequence
                        r_gap += mempool_sequence - expected_sequence
                    else:
                        raise Exception(f"WARNING: txhash has unexpected mempool sequence value: {mempool_sequence} vs expected {expected_sequence}")
            if label == "A":
                assert hash_str not in mempool_view
                mempool_view.add(hash_str)
                expected_sequence = mempool_sequence + 1
            elif label == "R":
                assert hash_str in mempool_view
                mempool_view.remove(hash_str)
                expected_sequence = mempool_sequence + 1
            elif label == "C":
                # (Attempt to) remove all txids from known block connects
                block_txids = self.nodes[0].getblock(hash_str)["tx"][1:]
                for txid in block_txids:
                    if txid in mempool_view:
                        expected_sequence += 1
                        mempool_view.remove(txid)
                expected_sequence -= r_gap
                r_gap = 0
            elif label == "D":
                # Not useful for mempool tracking per se
                continue
            else:
                raise Exception("Unexpected ZMQ sequence label!")

        assert_equal(self.nodes[0].getrawmempool(), [final_txid])
        assert_equal(self.nodes[0].getrawmempool(mempool_sequence=True)["mempool_sequence"], expected_sequence)

        # 5) If you miss a zmq/mempool sequence number, go back to step (2)

        self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)

    def test_multiple_interfaces(self):
        # Set up two subscribers with different addresses
        # (note that after the reorg test, syncing would fail due to different
        # chain lengths on node0 and node1; for this test we only need node0, so
        # we can disable syncing blocks on the setup)
        subscribers = self.setup_zmq_test([
            ("hashblock", "tcp://127.0.0.1:28334"),
            ("hashblock", "tcp://127.0.0.1:28335"),
        ], sync_blocks=False)

        # Generate 1 block in nodes[0] and receive all notifications
        self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)

        # Should receive the same block hash on both subscribers
        assert_equal(self.nodes[0].getbestblockhash(), subscribers[0].receive().hex())
        assert_equal(self.nodes[0].getbestblockhash(), subscribers[1].receive().hex())

    def test_ipv6(self):
        if not test_ipv6_local():
            self.log.info("Skipping IPv6 test, because IPv6 is not supported.")
            return
        self.log.info("Testing IPv6")
        # Set up subscriber using IPv6 loopback address
        subscribers = self.setup_zmq_test([
            ("hashblock", "tcp://[::1]:28332")
        ], ipv6=True)

        # Generate 1 block in nodes[0]
        self.nodes[0].generatetoaddress(1, ADDRESS_BCRT1_UNSPENDABLE)

        # Should receive the same block hash
        assert_equal(self.nodes[0].getbestblockhash(), subscribers[0].receive().hex())


if __name__ == '__main__':
    ZMQTest().main()
