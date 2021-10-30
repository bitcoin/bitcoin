#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test BIP 37
"""

from test_framework.messages import (
    MSG_BLOCK,
    MSG_FILTERED_BLOCK,
    msg_getdata,
    msg_filterload,
    msg_filteradd,
    msg_filterclear,
)
from test_framework.mininode import (
    P2PInterface,
    mininode_lock,
)
from test_framework.test_framework import BitcoinTestFramework


class FilterNode(P2PInterface):
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

    def on_inv(self, message):
        want = msg_getdata()
        for i in message.inv:
            # inv messages can only contain TX or BLOCK, so translate BLOCK to FILTERED_BLOCK
            if i.type == MSG_BLOCK:
                i.type = MSG_FILTERED_BLOCK
            want.inv.append(i)
        if len(want.inv):
            self.send_message(want)

    def on_merkleblock(self, message):
        self.merkleblock_received = True

    def on_tx(self, message):
        self.tx_received = True


class FilterTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.extra_args = [[
            '-peerbloomfilters',
            '-whitelist=noban@127.0.0.1',  # immediate tx relay
        ]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info('Add filtered P2P connection to the node')
        filter_node = self.nodes[0].add_p2p_connection(FilterNode())
        filter_node.send_and_ping(filter_node.watch_filter_init)
        filter_address = self.nodes[0].decodescript(filter_node.watch_script_pubkey)['addresses'][0]

        self.log.info('Check that we receive merkleblock and tx if the filter matches a tx in a block')
        filter_node.merkleblock_received = False
        block_hash = self.nodes[0].generatetoaddress(1, filter_address)[0]
        txid = self.nodes[0].getblock(block_hash)['tx'][0]
        filter_node.wait_for_tx(txid)
        assert filter_node.merkleblock_received

        self.log.info('Check that we only receive a merkleblock if the filter does not match a tx in a block')
        with mininode_lock:
            filter_node.last_message.pop("merkleblock", None)
        filter_node.tx_received = False
        self.nodes[0].generatetoaddress(1, self.nodes[0].getnewaddress())
        filter_node.wait_for_merkleblock()
        assert not filter_node.tx_received

        self.log.info('Check that we not receive a tx if the filter does not match a mempool tx')
        filter_node.merkleblock_received = False
        filter_node.tx_received = False
        self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 90)
        filter_node.sync_with_ping()
        filter_node.sync_with_ping()
        assert not filter_node.merkleblock_received
        assert not filter_node.tx_received

        self.log.info('Check that we receive a tx in reply to a mempool msg if the filter matches a mempool tx')
        filter_node.merkleblock_received = False
        txid = self.nodes[0].sendtoaddress(filter_address, 90)
        filter_node.wait_for_tx(txid)
        assert not filter_node.merkleblock_received

        self.log.info('Check that after deleting filter all txs get relayed again')
        filter_node.send_and_ping(msg_filterclear())
        for _ in range(5):
            txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 7)
            filter_node.wait_for_tx(txid)

        self.log.info("Check that division-by-zero remote crash bug [CVE-2013-5700] is fixed")
        filter_node.send_and_ping(msg_filterload(data=b'', nHashFuncs=1))
        filter_node.send_and_ping(msg_filteradd(data=b'letstrytocrashthisnode'))


if __name__ == '__main__':
    FilterTest().main()
