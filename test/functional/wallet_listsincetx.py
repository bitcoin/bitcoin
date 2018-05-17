#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listsincetx RPC including longpoll."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

import threading
import time

class LongpollThread(threading.Thread):
    def __init__(self, node, txid):
        threading.Thread.__init__(self)
        self.txid = txid
        self.node = node
        self.txlist = []

    def run(self):
        self.txlist = self.node.listsincetx(self.txid, 10)


class ListSinceTxTest (BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def run_test(self):
        self.nodes[0].generate(101)
        self.sync_all()
        newest_txid         = self.nodes[0].listtransactions()[-1]['txid']
        second_newest_txid  = self.nodes[0].listtransactions()[-2]['txid']
        listsinceoldest     = self.nodes[0].listsincetx(self.nodes[0].listtransactions("*", 101)[0]['txid'])
        self.log.info("Testing synchronous listtxsince behaviour")
        assert_equal(len(listsinceoldest), 100) #all follow up transactions
        assert_equal(listsinceoldest[-1]['txid'], newest_txid)
        assert_equal(listsinceoldest[-2]['txid'], second_newest_txid)
        assert_equal(self.nodes[0].listsincetx(second_newest_txid)[0]['txid'], newest_txid)
        assert_equal(len(self.nodes[0].listsincetx(newest_txid)), 0) #no transaction will follow the newest one
        assert_equal(len(self.nodes[0].listsincetx(newest_txid, 1)), 0) #poll with no result
        txid_n1_1 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        txid_n1_2 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 2)
        self.sync_all()
        assert_equal(self.nodes[1].listsincetx(txid_n1_1)[0]['txid'], txid_n1_2)
        assert_equal(len(self.nodes[1].listsincetx(txid_n1_2, 1)), 0) #poll with no result

        self.log.info("Start long polling thread")
        new_addr_n1 = self.nodes[1].getnewaddress()
        thr = LongpollThread(self.nodes[1], txid_n1_2)
        thr.start()
        self.log.info("Sending transaction from other node on main thread")
        txid_n1_3 = self.nodes[0].sendtoaddress(new_addr_n1, 3)
        cnt = 0
        while True: #need to loop since sync_all won't work with an active RPC connection
            if len(thr.txlist) > 0:
                self.log.info("Lonpoll thread received transaction")
                break
            time.sleep(1)
            cnt+=1
            assert(cnt < 10) #timeout of 10 seconds
        thr.join(5)
        assert_equal(thr.txlist[0]['txid'], txid_n1_3)

if __name__ == '__main__':
    ListSinceTxTest().main()
