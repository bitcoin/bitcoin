#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Exercise the rpc signals API

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import threading
import uuid

class pollThread (threading.Thread):
    def __init__(self, threadID, node, clientUUID):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.node = node
        self.uuid = clientUUID
    def run(self):
        self.node.pollnotifications(self.uuid, 5)

class RpcSignalsTest(BitcoinTestFramework):

    def pollthread(self, node):
        print("thread")

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test (self):
        self.nodes[0].generate(100)
        client0UUID = str(uuid.uuid1())
        client1UUID = str(uuid.uuid1())
        client2UUID = str(uuid.uuid1())
        assert_raises_jsonrpc(-8, "Client UUID not found", self.nodes[0].getregisterednotifications, client0UUID)

        self.nodes[0].setregisterednotifications(client0UUID, ["hashblock", "hashtx"])
        data = self.nodes[0].getregisterednotifications(client0UUID)
        assert("hashblock" in data)
        assert("hashtx" in data)

        self.nodes[0].setregisterednotifications(client1UUID, ["hashtx"])
        data = self.nodes[0].getregisterednotifications(client1UUID)
        assert("hashblock" not in data)
        assert("hashtx" in data)

        self.nodes[0].setregisterednotifications(client2UUID, ["hashblock"])

        minedblockhash = self.nodes[0].generate(1)[0]
        data = self.nodes[0].pollnotifications(client0UUID, 10)
        assert(len(data) == 2)

        # poll via thread (5 sec poll)
        myThread = pollThread(1, self.nodes[0], client0UUID)
        myThread.start()
        time.sleep(3)
        # thread must still be polling
        assert(myThread.isAlive() == True)
        time.sleep(5)
        # thread poll time must be expired
        assert(myThread.isAlive() == False)
        myThread.join()

        # test mempool transactions
        txid0 = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        txid1 = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        data = self.nodes[0].pollnotifications(client0UUID, 10)
        assert(len(data) == 2)
        assert(data[0]['type'] == "hashtx")
        assert(data[0]['obj'] == txid0)
        assert(data[0]['seq'] == 2)
        assert(data[1]['type'] == "hashtx")
        assert(data[1]['obj'] == txid1)
        assert(data[1]['seq'] == 3)

        # test second queue with tx only
        data = self.nodes[0].pollnotifications(client1UUID, 10)
        assert(len(data) == 3)
        assert(data[0]['type'] == "hashtx")
        assert(data[1]['type'] == "hashtx")
        assert(data[2]['type'] == "hashtx")
        assert(data[2]['seq'] == 3)

        # test if only hashblock reqistered queue has the right content
        data = self.nodes[0].pollnotifications(client2UUID, 10)
        assert(len(data) == 1)
        assert(data[0]['type'] == "hashblock")
        assert(data[0]['obj'] == minedblockhash)
        assert(data[0]['seq'] == 1)

        # test unregister
        self.nodes[0].setregisterednotifications(client0UUID, [])
        data = self.nodes[0].getregisterednotifications(client0UUID)
        assert(len(data) == 0)
        self.nodes[0].generate(1)
        data = self.nodes[0].pollnotifications(client0UUID, 5)
        assert(len(data) == 0)

if __name__ == '__main__':
    RpcSignalsTest().main()

