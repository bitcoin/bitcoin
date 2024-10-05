#!/usr/bin/env python3
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import threading
from test_framework.messages import MAX_PROTOCOL_MESSAGE_LENGTH
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import random_bytes

class NetDeadlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def run_test(self):
        node0 = self.nodes[0]
        node1 = self.nodes[1]

        self.log.info("Simultaneously send a large message on both sides")
        rand_msg = random_bytes(MAX_PROTOCOL_MESSAGE_LENGTH).hex()

        thread0 = threading.Thread(target=node0.sendmsgtopeer, args=(0, "unknown", rand_msg))
        thread1 = threading.Thread(target=node1.sendmsgtopeer, args=(0, "unknown", rand_msg))

        thread0.start()
        thread1.start()
        thread0.join()
        thread1.join()

        self.log.info("Check whether a deadlock happened")
        self.generate(self.nodes[0], 1)


if __name__ == '__main__':
    NetDeadlockTest().main()
