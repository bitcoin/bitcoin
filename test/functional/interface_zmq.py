#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the ZMQ notification interface."""
import os
import struct
import tempfile
from time import sleep
from io import BytesIO

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
    CBlock,
    hash256,
    tx_from_hex,
)
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    p2p_port,
)
from test_framework.wallet import (
    MiniWallet,
)
from test_framework.netutil import test_ipv6_local, test_unix_socket


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
        self.block_hash = test_framework.generate(node, 1, sync_fun=test_framework.no_op)[0]
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
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True
        self.zmq_port_base = p2p_port(self.num_nodes + 1)

    def skip_test_if_missing_module(self):
        self.skip_if_no_py3_zmq()
        self.skip_if_no_bitcoind_zmq()

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        self.ctx = zmq.Context()
        try:
            self.test_basic()
            if test_unix_socket():
                self.test_basic(unix=True)
            else:
                self.log.info("Skipping ipc test, because UNIX sockets are not supported.")
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

        self.restart_node(0, [f"-zmqpub{topic}={address.replace('ipc://', 'unix:')}" for topic, address in services])

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

    def test_basic(self, unix = False):
        self.log.info(f"Running basic test with {'ipc' if unix else 'tcp'} protocol")

        # Invalid zmq arguments don't take down the node, see #17185.
        self.restart_node(0, ["-zmqpubrawtx=foo", "-zmqpubhashtx=bar"])

        address = f"tcp://127.0.0.1:{self.zmq_port_base}"

        if unix:
            # Use the shortest temp path possible since paths may have as little as 92-char limit
            socket_path = tempfile.NamedTemporaryFile().name
            address = f"ipc://{socket_path}"

        subs = self.setup_zmq_test([(topic, address) for topic in ["hashblock", "hashtx", "rawblock", "rawtx"]])

        hashblock = subs[0]
        hashtx = subs[1]
        rawblock = subs[2]
        rawtx = subs[3]

        num_blocks = 5
        self.log.info(f"Generate {num_blocks} blocks (and {num_blocks} coinbase txes)")
        genhashes = self.generatetoaddress(self.nodes[0], num_blocks, ADDRESS_BCRT1_UNSPENDABLE)

        for x in range(num_blocks):
            # Should receive the coinbase txid.
            txid = hashtx.receive()

            # Should receive the coinbase raw transaction.
            tx = tx_from_hex(rawtx.receive().hex())
            tx.calc_sha256()
            assert_equal(tx.hash, txid.hex())

            # Should receive the generated raw block.
            hex = rawblock.receive()
            block = CBlock()
            block.deserialize(BytesIO(hex))
            assert block.is_valid()
            assert_equal(block.vtx[0].hash, tx.hash)
            assert_equal(len(block.vtx), 1)
            assert_equal(genhashes[x], hash256_reversed(hex[:80]).hex())

            # Should receive the generated block hash.
            hash = hashblock.receive().hex()
            assert_equal(genhashes[x], hash)
            # The block should only have the coinbase txid.
            assert_equal([txid.hex()], self.nodes[1].getblock(hash)["tx"])


        self.log.info("Wait for tx from second node")
        payment_tx = self.wallet.send_self_transfer(from_node=self.nodes[1])
        payment_txid = payment_tx['txid']
        self.sync_all()
        # Should receive the broadcasted txid.
        txid = hashtx.receive()
        assert_equal(payment_txid, txid.hex())

        # Should receive the broadcasted raw transaction.
        hex = rawtx.receive()
        assert_equal(payment_tx['wtxid'], hash256_reversed(hex).hex())

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
        if unix:
            os.unlink(socket_path)

    def test_reorg(self):

        address = f"tcp://127.0.0.1:{self.zmq_port_base}"

        # Should only notify the tip if a reorg occurs
        hashblock, hashtx = self.setup_zmq_test(
            [(topic, address) for topic in ["hashblock", "hashtx"]],
            recv_timeout=2)  # 2 second timeout to check end of notifications
        self.disconnect_nodes(0, 1)

        # Generate 1 block in nodes[0] with 1 mempool tx and receive all notifications
        payment_txid = self.wallet.send_self_transfer(from_node=self.nodes[0])['txid']
        disconnect_block = self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE, sync_fun=self.no_op)[0]
        disconnect_cb = self.nodes[0].getblock(disconnect_block)["tx"][0]
        assert_equal(self.nodes[0].getbestblockhash(), hashblock.receive().hex())
        assert_equal(hashtx.receive().hex(), payment_txid)
        assert_equal(hashtx.receive().hex(), disconnect_cb)

        # Generate 2 blocks in nodes[1] to a different address to ensure split
        connect_blocks = self.generatetoaddress(self.nodes[1], 2, ADDRESS_BCRT1_P2WSH_OP_TRUE, sync_fun=self.no_op)

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
        [seq] = self.setup_zmq_test([("sequence", f"tcp://127.0.0.1:{self.zmq_port_base}")])
        self.disconnect_nodes(0, 1)

        # Mempool sequence number starts at 1
        seq_num = 1

        # Generate 1 block in nodes[0] and receive all notifications
        dc_block = self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE, sync_fun=self.no_op)[0]

        # Note: We are not notified of any block transactions, coinbase or mined
        assert_equal((self.nodes[0].getbestblockhash(), "C", None), seq.receive_sequence())

        # Generate 2 blocks in nodes[1] to a different address to ensure a chain split
        self.generatetoaddress(self.nodes[1], 2, ADDRESS_BCRT1_P2WSH_OP_TRUE, sync_fun=self.no_op)

        # nodes[0] will reorg chain after connecting back nodes[1]
        self.connect_nodes(0, 1)

        # Then we receive all block (dis)connect notifications for the 2 block reorg
        assert_equal((dc_block, "D", None), seq.receive_sequence())
        block_count = self.nodes[1].getblockcount()
        assert_equal((self.nodes[1].getblockhash(block_count-1), "C", None), seq.receive_sequence())
        assert_equal((self.nodes[1].getblockhash(block_count), "C", None), seq.receive_sequence())

        self.log.info("Wait for tx from second node")
        payment_tx = self.wallet.send_self_transfer(from_node=self.nodes[1])
        payment_txid = payment_tx['txid']
        self.sync_all()
        self.log.info("Testing sequence notifications with mempool sequence values")

        # Should receive the broadcasted txid.
        assert_equal((payment_txid, "A", seq_num), seq.receive_sequence())
        seq_num += 1

        self.log.info("Testing RBF notification")
        # Replace it to test eviction/addition notification
        payment_tx['tx'].vout[0].nValue -= 1000
        rbf_txid = self.nodes[1].sendrawtransaction(payment_tx['tx'].serialize().hex())
        self.sync_all()
        assert_equal((payment_txid, "R", seq_num), seq.receive_sequence())
        seq_num += 1
        assert_equal((rbf_txid, "A", seq_num), seq.receive_sequence())
        seq_num += 1

        # Doesn't get published when mined, make a block and tx to "flush" the possibility
        # though the mempool sequence number does go up by the number of transactions
        # removed from the mempool by the block mining it.
        mempool_size = len(self.nodes[0].getrawmempool())
        c_block = self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)[0]
        # Make sure the number of mined transactions matches the number of txs out of mempool
        mempool_size_delta = mempool_size - len(self.nodes[0].getrawmempool())
        assert_equal(len(self.nodes[0].getblock(c_block)["tx"])-1, mempool_size_delta)
        seq_num += mempool_size_delta
        payment_txid_2 = self.wallet.send_self_transfer(from_node=self.nodes[1])['txid']
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
        assert_equal((rbf_txid, "A", seq_num), seq.receive_sequence())
        seq_num += 1

        # Other things may happen but aren't wallet-deterministic so we don't test for them currently
        self.nodes[0].reconsiderblock(best_hash)
        self.generatetoaddress(self.nodes[1], 1, ADDRESS_BCRT1_UNSPENDABLE)

        self.log.info("Evict mempool transaction by block conflict")
        orig_tx = self.wallet.send_self_transfer(from_node=self.nodes[0])
        orig_txid = orig_tx['txid']

        # More to be simply mined
        more_tx = []
        for _ in range(5):
            more_tx.append(self.wallet.send_self_transfer(from_node=self.nodes[0]))

        orig_tx['tx'].vout[0].nValue -= 1000
        bump_txid = self.nodes[0].sendrawtransaction(orig_tx['tx'].serialize().hex())
        # Mine the pre-bump tx
        txs_to_add = [orig_tx['hex']] + [tx['hex'] for tx in more_tx]
        block = create_block(int(self.nodes[0].getbestblockhash(), 16), create_coinbase(self.nodes[0].getblockcount()+1), txlist=txs_to_add)
        add_witness_commitment(block)
        block.solve()
        assert_equal(self.nodes[0].submitblock(block.serialize().hex()), None)
        tip = self.nodes[0].getbestblockhash()
        assert_equal(int(tip, 16), block.sha256)
        orig_txid_2 = self.wallet.send_self_transfer(from_node=self.nodes[0])['txid']

        # Flush old notifications until evicted tx original entry
        (hash_str, label, mempool_seq) = seq.receive_sequence()
        while hash_str != orig_txid:
            (hash_str, label, mempool_seq) = seq.receive_sequence()
        mempool_seq += 1

        # Added original tx
        assert_equal(label, "A")
        # More transactions to be simply mined
        for i in range(len(more_tx)):
            assert_equal((more_tx[i]['txid'], "A", mempool_seq), seq.receive_sequence())
            mempool_seq += 1
        # Bumped by rbf
        assert_equal((orig_txid, "R", mempool_seq), seq.receive_sequence())
        mempool_seq += 1
        assert_equal((bump_txid, "A", mempool_seq), seq.receive_sequence())
        mempool_seq += 1
        # Conflict announced first, then block
        assert_equal((bump_txid, "R", mempool_seq), seq.receive_sequence())
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

        self.log.info("Testing 'mempool sync' usage of sequence notifier")
        [seq] = self.setup_zmq_test([("sequence", f"tcp://127.0.0.1:{self.zmq_port_base}")])

        # In-memory counter, should always start at 1
        next_mempool_seq = self.nodes[0].getrawmempool(mempool_sequence=True)["mempool_sequence"]
        assert_equal(next_mempool_seq, 1)

        # Some transactions have been happening but we aren't consuming zmq notifications yet
        # or we lost a ZMQ message somehow and want to start over
        txs = []
        num_txs = 5
        for _ in range(num_txs):
            txs.append(self.wallet.send_self_transfer(from_node=self.nodes[1]))
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
        assert_equal(get_raw_seq, num_txs + 1)
        assert zmq_mem_seq < get_raw_seq

        # Things continue to happen in the "interim" while waiting for snapshot results
        # We have node 0 do all these to avoid p2p races with RBF announcements
        for _ in range(num_txs):
            txs.append(self.wallet.send_self_transfer(from_node=self.nodes[0]))
        txs[-1]['tx'].vout[0].nValue -= 1000
        self.nodes[0].sendrawtransaction(txs[-1]['tx'].serialize().hex())
        self.sync_all()
        self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)
        final_txid = self.wallet.send_self_transfer(from_node=self.nodes[0])['txid']

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
            ("hashblock", f"tcp://127.0.0.1:{self.zmq_port_base + 1}"),
            ("hashblock", f"tcp://127.0.0.1:{self.zmq_port_base + 2}"),
        ], sync_blocks=False)

        # Generate 1 block in nodes[0] and receive all notifications
        self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE, sync_fun=self.no_op)

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
            ("hashblock", f"tcp://[::1]:{self.zmq_port_base}")
        ], ipv6=True)

        # Generate 1 block in nodes[0]
        self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)

        # Should receive the same block hash
        assert_equal(self.nodes[0].getbestblockhash(), subscribers[0].receive().hex())


if __name__ == '__main__':
    ZMQTest(__file__).main()
