#!/usr/bin/env python

#
# Test double-spend-relay and notification code
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from decimal import Decimal

class DoubleSpendRelay(BitcoinTestFramework):

    #
    # Create a 4-node network; roles for the nodes are:
    # [0] : transaction creator
    # [1] : respend sender
    # [2] : relay node
    # [3] : receiver, should detect/notify of double-spends
    #
    # Node connectivity is:
    # [0,1] <--> [2] <--> [3]
    #
    def setup_network(self):
        self.is_network_split = False
        self.nodes = []
        for i in range(0,4):
            self.nodes.append(start_node(i, self.options.tmpdir))
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[1], 2)
        connect_nodes(self.nodes[3], 2)
        return self.nodes

    def run_test(self):
        fee = Decimal("0.01")

        nodes = self.nodes
        # Test 1: First spend
        # shutdown nodes[1] so it is not aware of the first spend
        # and will be willing to broadcast a respend
        stop_node(nodes[1], 1)
        # First spend: nodes[0] -> nodes[3]
        amount = Decimal("144") # We rely on this requiring 3 50-BTC inputs
        (total_in, tx1_inputs) = gather_inputs(nodes[0], amount+fee)
        change_outputs = make_change(nodes[0], total_in, amount, fee)
        outputs = dict(change_outputs)
        outputs[nodes[3].getnewaddress()] = amount
        signed = nodes[0].signrawtransaction(nodes[0].createrawtransaction(tx1_inputs, outputs))
        txid1 = nodes[0].sendrawtransaction(signed["hex"], True)
        sync_mempools([nodes[0], nodes[3]])
        
        txid1_info = nodes[3].gettransaction(txid1)
        assert_equal(txid1_info["respendsobserved"], [])

        # Test 2: Is double-spend of tx1_inputs[0] relayed?
        # Restart nodes[1]
        nodes[1] = start_node(1, self.options.tmpdir)
        connect_nodes(nodes[1], 2)
        # Second spend: nodes[0] -> nodes[0]
        amount = Decimal("40")
        total_in = Decimal("50")
        inputs2 = [tx1_inputs[0]]
        change_outputs = make_change(nodes[0], total_in, amount, fee)
        outputs = dict(change_outputs)
        outputs[nodes[0].getnewaddress()] = amount
        signed = nodes[0].signrawtransaction(nodes[0].createrawtransaction(inputs2, outputs))
        txid2 = nodes[1].sendrawtransaction(signed["hex"], True)
        # Wait until txid2 is relayed to nodes[3] (but don't wait forever):
        # Note we can't use sync_mempools, because the respend isn't added to
        # the mempool.
        for i in range(1,7):
            txid1_info = nodes[3].gettransaction(txid1)
            if txid1_info["respendsobserved"] != []:
                break
            time.sleep(0.1 * i**2) # geometric back-off
        assert_equal(txid1_info["respendsobserved"], [txid2])

        # Test 3: Is triple-spend of tx1_inputs[0] not relayed?
        # Clear node1 mempool
        stop_node(nodes[1], 1)
        nodes[1] = start_node(1, self.options.tmpdir)
        connect_nodes(nodes[1], 2)
        # Third spend: nodes[0] -> nodes[0]
        outputs = dict(change_outputs)
        outputs[nodes[0].getnewaddress()] = amount
        signed = nodes[0].signrawtransaction(nodes[0].createrawtransaction(inputs2, outputs))
        txid3 = nodes[1].sendrawtransaction(signed["hex"], True)
        # Ensure txid3 not relayed to nodes[3]:
        time.sleep(9.1)
        txid1_info = nodes[3].gettransaction(txid1)
        assert_equal(txid1_info["respendsobserved"], [txid2])

        # Test 4: Is double-spend of tx1_inputs[1] relayed when triple-spend of tx1_inputs[0] precedes it?
        # Clear node1 mempool
        stop_node(nodes[1], 1)
        nodes[1] = start_node(1, self.options.tmpdir)
        connect_nodes(nodes[1], 2)
        # Inputs are third spend, second spend
        amount = Decimal("89")
        total_in = Decimal("100")
        inputs4 = [tx1_inputs[0],tx1_inputs[1]]
        change_outputs = make_change(nodes[0], total_in, amount, fee)
        outputs = dict(change_outputs)
        outputs[nodes[0].getnewaddress()] = amount
        signed = nodes[0].signrawtransaction(nodes[0].createrawtransaction(inputs4, outputs))
        txid4 = nodes[1].sendrawtransaction(signed["hex"], True)
        # Wait until txid4 is relayed to nodes[3] (but don't wait forever):
        for i in range(1,7):
            txid1_info = nodes[3].gettransaction(txid1)
            if txid1_info["respendsobserved"] != [txid2]:
                break
            time.sleep(0.1 * i**2) # geometric back-off
        assert_equal(sorted(txid1_info["respendsobserved"]), sorted([txid2,txid4]))

        # Test 5: Is double-spend of tx1_inputs[2] relayed when triple-spend of tx1_inputs[0] follows it?
        # Clear node1 mempool
        stop_node(nodes[1], 1)
        nodes[1] = start_node(1, self.options.tmpdir)
        connect_nodes(nodes[1], 2)
        # Inputs are second spend, third spend
        amount = Decimal("88")
        total_in = Decimal("100")
        inputs5 = [tx1_inputs[2],tx1_inputs[0]]
        change_outputs = make_change(nodes[0], total_in, amount, fee)
        outputs = dict(change_outputs)
        outputs[nodes[0].getnewaddress()] = amount
        signed = nodes[0].signrawtransaction(nodes[0].createrawtransaction(inputs5, outputs))
        txid5 = nodes[1].sendrawtransaction(signed["hex"], True)
        # Wait until txid5 is relayed to nodes[3] (but don't wait forever):
        for i in range(1,7):
            txid1_info = nodes[3].gettransaction(txid1)
            if sorted(txid1_info["respendsobserved"]) != sorted([txid2,txid4]):
                break
            time.sleep(0.1 * i**2) # geometric back-off
        assert_equal(sorted(txid1_info["respendsobserved"]), sorted([txid2,txid4,txid5]))

if __name__ == '__main__':
    DoubleSpendRelay().main()

