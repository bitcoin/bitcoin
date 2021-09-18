#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test BIP 37
"""

from test_framework.messages import (
    CInv,
    MAX_BLOOM_FILTER_SIZE,
    MAX_BLOOM_HASH_FUNCS,
    MSG_BLOCK,
    MSG_FILTERED_BLOCK,
    msg_filteradd,
    msg_filterclear,
    msg_filterload,
    msg_getdata,
    msg_mempool,
    msg_version,
)
from test_framework.p2p import (
    P2PInterface,
    P2P_SERVICES,
    P2P_SUBVERSION,
    P2P_VERSION,
    p2p_lock,
)
from test_framework.script import MAX_SCRIPT_ELEMENT_SIZE
from test_framework.test_framework import BitcoinTestFramework


class P2PBloomFilter(P2PInterface):
    # This is a P2SH watch-only wallet
    watch_script_pubkey = 'a914ffffffffffffffffffffffffffffffffffffffff87'
    # The initial filter (n=10, fp=0.000001) with just the above scriptPubKey added
    watch_filter_init = msg_filterload(
        data=
        b'@\x00\x08\x00\x80\x00\x00 \x00\xc0\x00 \x04\x00\x08$\x00\x04\x80\x00\x00 \x00\x00\x00\x00\x80\x00\x00@\x00\x02@ \x00',
        nHashFuncs=19,
        nTweak=0,
        nFlags=1,
    )

    def __init__(self):
        super().__init__()
        self._tx_received = False
        self._merkleblock_received = False

    def on_inv(self, message):
        want = msg_getdata()
        for i in message.inv:
            # inv messages can only contain TX or BLOCK, so translate BLOCK to FILTERED_BLOCK
            if i.type == MSG_BLOCK:
                want.inv.append(CInv(MSG_FILTERED_BLOCK, i.hash))
            else:
                want.inv.append(i)
        if len(want.inv):
            self.send_message(want)

    def on_merkleblock(self, message):
        self._merkleblock_received = True

    def on_tx(self, message):
        self._tx_received = True

    @property
    def tx_received(self):
        with p2p_lock:
            return self._tx_received

    @tx_received.setter
    def tx_received(self, value):
        with p2p_lock:
            self._tx_received = value

    @property
    def merkleblock_received(self):
        with p2p_lock:
            return self._merkleblock_received

    @merkleblock_received.setter
    def merkleblock_received(self, value):
        with p2p_lock:
            self._merkleblock_received = value


class FilterTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[
            '-peerbloomfilters',
            '-whitelist=noban@127.0.0.1',  # immediate tx relay
        ]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_size_limits(self, filter_peer):
        self.log.info('Check that too large filter is rejected')
        with self.nodes[0].assert_debug_log(['Misbehaving']):
            filter_peer.send_and_ping(msg_filterload(data=b'\xbb'*(MAX_BLOOM_FILTER_SIZE+1)))

        self.log.info('Check that max size filter is accepted')
        with self.nodes[0].assert_debug_log([], unexpected_msgs=['Misbehaving']):
            filter_peer.send_and_ping(msg_filterload(data=b'\xbb'*(MAX_BLOOM_FILTER_SIZE)))
        filter_peer.send_and_ping(msg_filterclear())

        self.log.info('Check that filter with too many hash functions is rejected')
        with self.nodes[0].assert_debug_log(['Misbehaving']):
            filter_peer.send_and_ping(msg_filterload(data=b'\xaa', nHashFuncs=MAX_BLOOM_HASH_FUNCS+1))

        self.log.info('Check that filter with max hash functions is accepted')
        with self.nodes[0].assert_debug_log([], unexpected_msgs=['Misbehaving']):
            filter_peer.send_and_ping(msg_filterload(data=b'\xaa', nHashFuncs=MAX_BLOOM_HASH_FUNCS))
        # Don't send filterclear until next two filteradd checks are done

        self.log.info('Check that max size data element to add to the filter is accepted')
        with self.nodes[0].assert_debug_log([], unexpected_msgs=['Misbehaving']):
            filter_peer.send_and_ping(msg_filteradd(data=b'\xcc'*(MAX_SCRIPT_ELEMENT_SIZE)))

        self.log.info('Check that too large data element to add to the filter is rejected')
        with self.nodes[0].assert_debug_log(['Misbehaving']):
            filter_peer.send_and_ping(msg_filteradd(data=b'\xcc'*(MAX_SCRIPT_ELEMENT_SIZE+1)))

        filter_peer.send_and_ping(msg_filterclear())

    def test_msg_mempool(self):
        self.log.info("Check that a node with bloom filters enabled services p2p mempool messages")
        filter_peer = P2PBloomFilter()

        self.log.debug("Create a tx relevant to the peer before connecting")
        filter_address = self.nodes[0].decodescript(filter_peer.watch_script_pubkey)['address']
        txid = self.nodes[0].sendtoaddress(filter_address, 90)

        self.log.debug("Send a mempool msg after connecting and check that the tx is received")
        self.nodes[0].add_p2p_connection(filter_peer)
        filter_peer.send_and_ping(filter_peer.watch_filter_init)
        filter_peer.send_message(msg_mempool())
        filter_peer.wait_for_tx(txid)

    def test_frelay_false(self, filter_peer):
        self.log.info("Check that a node with fRelay set to false does not receive invs until the filter is set")
        filter_peer.tx_received = False
        filter_address = self.nodes[0].decodescript(filter_peer.watch_script_pubkey)['address']
        self.nodes[0].sendtoaddress(filter_address, 90)
        # Sync to make sure the reason filter_peer doesn't receive the tx is not p2p delays
        filter_peer.sync_with_ping()
        assert not filter_peer.tx_received

        # Clear the mempool so that this transaction does not impact subsequent tests
        self.generate(self.nodes[0], 1)

    def test_filter(self, filter_peer):
        # Set the bloomfilter using filterload
        filter_peer.send_and_ping(filter_peer.watch_filter_init)
        # If fRelay is not already True, sending filterload sets it to True
        assert self.nodes[0].getpeerinfo()[0]['relaytxes']
        filter_address = self.nodes[0].decodescript(filter_peer.watch_script_pubkey)['address']

        self.log.info('Check that we receive merkleblock and tx if the filter matches a tx in a block')
        block_hash = self.generatetoaddress(self.nodes[0], 1, filter_address)[0]
        txid = self.nodes[0].getblock(block_hash)['tx'][0]
        filter_peer.wait_for_merkleblock(block_hash)
        filter_peer.wait_for_tx(txid)

        self.log.info('Check that we only receive a merkleblock if the filter does not match a tx in a block')
        filter_peer.tx_received = False
        block_hash = self.generatetoaddress(self.nodes[0], 1, self.nodes[0].getnewaddress())[0]
        filter_peer.wait_for_merkleblock(block_hash)
        assert not filter_peer.tx_received

        self.log.info('Check that we not receive a tx if the filter does not match a mempool tx')
        filter_peer.merkleblock_received = False
        filter_peer.tx_received = False
        self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 90)
        filter_peer.sync_send_with_ping()
        assert not filter_peer.merkleblock_received
        assert not filter_peer.tx_received

        self.log.info('Check that we receive a tx if the filter matches a mempool tx')
        filter_peer.merkleblock_received = False
        txid = self.nodes[0].sendtoaddress(filter_address, 90)
        filter_peer.wait_for_tx(txid)
        assert not filter_peer.merkleblock_received

        self.log.info('Check that after deleting filter all txs get relayed again')
        filter_peer.send_and_ping(msg_filterclear())
        for _ in range(5):
            txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 7)
            filter_peer.wait_for_tx(txid)

        self.log.info('Check that request for filtered blocks is ignored if no filter is set')
        filter_peer.merkleblock_received = False
        filter_peer.tx_received = False
        with self.nodes[0].assert_debug_log(expected_msgs=['received getdata']):
            block_hash = self.generatetoaddress(self.nodes[0], 1, self.nodes[0].getnewaddress())[0]
            filter_peer.wait_for_inv([CInv(MSG_BLOCK, int(block_hash, 16))])
            filter_peer.sync_with_ping()
            assert not filter_peer.merkleblock_received
            assert not filter_peer.tx_received

        self.log.info('Check that sending "filteradd" if no filter is set is treated as misbehavior')
        with self.nodes[0].assert_debug_log(['Misbehaving']):
            filter_peer.send_and_ping(msg_filteradd(data=b'letsmisbehave'))

        self.log.info("Check that division-by-zero remote crash bug [CVE-2013-5700] is fixed")
        filter_peer.send_and_ping(msg_filterload(data=b'', nHashFuncs=1))
        filter_peer.send_and_ping(msg_filteradd(data=b'letstrytocrashthisnode'))
        self.nodes[0].disconnect_p2ps()

    def run_test(self):
        filter_peer = self.nodes[0].add_p2p_connection(P2PBloomFilter())
        self.log.info('Test filter size limits')
        self.test_size_limits(filter_peer)

        self.log.info('Test BIP 37 for a node with fRelay = True (default)')
        self.test_filter(filter_peer)
        self.nodes[0].disconnect_p2ps()

        self.log.info('Test BIP 37 for a node with fRelay = False')
        # Add peer but do not send version yet
        filter_peer_without_nrelay = self.nodes[0].add_p2p_connection(P2PBloomFilter(), send_version=False, wait_for_verack=False)
        # Send version with relay=False
        version_without_fRelay = msg_version()
        version_without_fRelay.nVersion = P2P_VERSION
        version_without_fRelay.strSubVer = P2P_SUBVERSION
        version_without_fRelay.nServices = P2P_SERVICES
        version_without_fRelay.relay = 0
        filter_peer_without_nrelay.send_message(version_without_fRelay)
        filter_peer_without_nrelay.wait_for_verack()
        assert not self.nodes[0].getpeerinfo()[0]['relaytxes']
        self.test_frelay_false(filter_peer_without_nrelay)
        self.test_filter(filter_peer_without_nrelay)

        self.test_msg_mempool()


if __name__ == '__main__':
    FilterTest().main()
