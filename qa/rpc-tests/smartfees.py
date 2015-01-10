#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test fee estimation code
#

from test_framework import BitcoinTestFramework
from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *

class EstimateFeeTest(BitcoinTestFramework):

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir,
                            ["-debug=mempool", "-debug=estimatefee", "-relaypriority=0"]))
        # Node1 mines small-but-not-tiny blocks, and allows free transactions.
        # NOTE: the CreateNewBlock code starts counting block size at 1,000 bytes,
        # so blockmaxsize of 2,000 is really just 1,000 bytes (room enough for
        # 6 or 7 transactions)
        self.nodes.append(start_node(1, self.options.tmpdir,
                                ["-blockprioritysize=1500", "-blockmaxsize=2000",
                                 "-debug=mempool", "-debug=estimatefee", "-relaypriority=0"]))
        connect_nodes(self.nodes[1], 0)

        # Node2 is a stingy miner, that
        # produces very small blocks (room for only 3 or so transactions)
        node2args = [ "-blockprioritysize=0", "-blockmaxsize=1500",
                      "-debug=mempool", "-debug=estimatefee", "-relaypriority=0"]
        self.nodes.append(start_node(2, self.options.tmpdir, node2args))
        connect_nodes(self.nodes[2], 0)

        self.is_network_split = False
        self.sync_all()
        

    def run_test(self):
        # Prime the memory pool with pairs of transactions
        # (high-priority, random fee and zero-priority, random fee)
        min_fee = Decimal("0.001")
        fees_per_kb = [];
        for i in range(12):
            (txid, txhex, fee) = random_zeropri_transaction(self.nodes, Decimal("1.1"),
                                                            min_fee, min_fee, 20)
            tx_kbytes = (len(txhex)/2)/1000.0
            fees_per_kb.append(float(fee)/tx_kbytes)

        # Mine blocks with node2 until the memory pool clears:
        count_start = self.nodes[2].getblockcount()
        while len(self.nodes[2].getrawmempool()) > 0:
            self.nodes[2].setgenerate(True, 1)
            self.sync_all()

        all_estimates = [ self.nodes[0].estimatefee(i) for i in range(1,20) ]
        print("Fee estimates, super-stingy miner: "+str([str(e) for e in all_estimates]))

        # Estimates should be within the bounds of what transactions fees actually were:
        delta = 1.0e-6 # account for rounding error
        for e in filter(lambda x: x >= 0, all_estimates):
            if float(e)+delta < min(fees_per_kb) or float(e)-delta > max(fees_per_kb):
                raise AssertionError("Estimated fee (%f) out of range (%f,%f)"%(float(e), min_fee_kb, max_fee_kb))

        # Generate transactions while mining 30 more blocks, this time with node1:
        for i in range(30):
            for j in range(random.randrange(6-4,6+4)):
                (txid, txhex, fee) = random_transaction(self.nodes, Decimal("1.1"),
                                                        Decimal("0.0"), min_fee, 20)
                tx_kbytes = (len(txhex)/2)/1000.0
                fees_per_kb.append(float(fee)/tx_kbytes)
            self.nodes[1].setgenerate(True, 1)
            self.sync_all()

        all_estimates = [ self.nodes[0].estimatefee(i) for i in range(1,20) ]
        print("Fee estimates, more generous miner: "+str([ str(e) for e in all_estimates]))
        for e in filter(lambda x: x >= 0, all_estimates):
            if float(e)+delta < min(fees_per_kb) or float(e)-delta > max(fees_per_kb):
                raise AssertionError("Estimated fee (%f) out of range (%f,%f)"%(float(e), min_fee_kb, max_fee_kb))

        # Finish by mining a normal-sized block:
        while len(self.nodes[0].getrawmempool()) > 0:
            self.nodes[0].setgenerate(True, 1)
            self.sync_all()

        final_estimates = [ self.nodes[0].estimatefee(i) for i in range(1,20) ]
        print("Final fee estimates: "+str([ str(e) for e in final_estimates]))


if __name__ == '__main__':
    EstimateFeeTest().main()
