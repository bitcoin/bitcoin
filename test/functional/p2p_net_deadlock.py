#!/usr/bin/env python3
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import threading
from test_framework.test_framework import BitcoinTestFramework
from random import randbytes


class NetDeadlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def run_test(self):
        node0 = self.nodes[0]
        node1 = self.nodes[1]

        self.log.info("Simultaneously send a large message on both sides")
        rand_msg = randbytes(4000000).hex()

        thread0 = threading.Thread(target=node0.sendmsgtopeer, args=(0, "unknown", rand_msg))
        thread1 = threading.Thread(target=node1.sendmsgtopeer, args=(0, "unknown", rand_msg))

        thread0.start()
        thread1.start()
        thread0.join()
        thread1.join()

        self.log.info("Check whether a deadlock happened")
        self.generate(node0, 1)
        self.sync_blocks()


if __name__ == '__main__':
    NetDeadlockTest(__file__).main()
