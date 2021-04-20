#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import DashTestFramework
from test_framework.util import set_node_times, isolate_node, reconnect_isolated_node

'''
feature_llmq_is_retroactive.py

Tests retroactive signing

We have 6 nodes where node 0 is the control node, nodes 1-5 are masternodes.
Mempool inconsistencies are simulated via disconnecting/reconnecting node 3
and by having a higher relay fee on nodes 4 and 5.
'''

class LLMQ_IS_RetroactiveSigning(DashTestFramework):
    def set_test_params(self):
        # -whitelist is needed to avoid the trickling logic on node0
        self.set_dash_test_params(6, 5, [["-whitelist=127.0.0.1"], [], [], [], ["-minrelaytxfee=0.001"], ["-minrelaytxfee=0.001"]], fast_dip3_enforcement=True)
        self.set_dash_llmq_test_params(5, 3)

    def run_test(self):
        self.activate_dip8()

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        # Turn mempool IS signing off
        self.nodes[0].spork("SPORK_2_INSTANTSEND_ENABLED", 1)
        self.wait_for_sporks_same()

        self.mine_quorum()
        self.mine_quorum()

        # Make sure that all nodes are chainlocked at the same height before starting actual tests
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash(), timeout=30)

        self.log.info("trying normal IS lock w/ signing spork off. Shouldn't be islocked before block is created.")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        # 3 nodes should be enough to create an IS lock even if nodes 4 and 5 (which have no tx itself)
        # are the only "neighbours" in intra-quorum connections for one of them.
        self.wait_for_instantlock(txid, self.nodes[0], False, 5)
        # Have to disable ChainLocks to avoid signing a block with a "safe" tx too early
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 4000000000)
        self.wait_for_sporks_same()
        # We have to wait in order to include tx in block
        self.bump_mocktime(10 * 60 + 1)
        block = self.nodes[0].generate(1)[0]
        self.wait_for_instantlock(txid, self.nodes[0])
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()
        self.wait_for_chainlocked_block_all_nodes(block)

        self.log.info("Enable mempool IS signing")
        self.nodes[0].spork("SPORK_2_INSTANTSEND_ENABLED", 0)
        self.wait_for_sporks_same()

        self.log.info("trying normal IS lock")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        # 3 nodes should be enough to create an IS lock even if nodes 4 and 5 (which have no tx itself)
        # are the only "neighbours" in intra-quorum connections for one of them.
        self.wait_for_instantlock(txid, self.nodes[0])
        self.bump_mocktime(1)
        block = self.nodes[0].generate(1)[0]
        self.wait_for_chainlocked_block_all_nodes(block)

        self.log.info("testing normal signing with partially known TX")
        isolate_node(self.nodes[3])
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        # Make sure nodes 1 and 2 received the TX before we continue,
        # otherwise it might announce the TX to node 3 when reconnecting
        self.wait_for_tx(txid, self.nodes[1])
        self.wait_for_tx(txid, self.nodes[2])
        reconnect_isolated_node(self.nodes[3], 0)
        # Make sure nodes actually try re-connecting quorum connections
        self.bump_mocktime(30)
        self.wait_for_mnauth(self.nodes[3], 2)
        # node 3 fully reconnected but the TX wasn't relayed to it, so there should be no IS lock
        self.wait_for_instantlock(txid, self.nodes[0], False, 5)
        # push the tx directly via rpc
        self.nodes[3].sendrawtransaction(self.nodes[0].getrawtransaction(txid))
        # node 3 should vote on a tx now since it became aware of it via sendrawtransaction
        # and this should be enough to complete an IS lock
        self.wait_for_instantlock(txid, self.nodes[0])

        self.log.info("testing retroactive signing with unknown TX")
        isolate_node(self.nodes[3])
        rawtx = self.nodes[0].createrawtransaction([], {self.nodes[0].getnewaddress(): 1})
        rawtx = self.nodes[0].fundrawtransaction(rawtx)['hex']
        rawtx = self.nodes[0].signrawtransactionwithwallet(rawtx)['hex']
        txid = self.nodes[3].sendrawtransaction(rawtx)
        # Make node 3 consider the TX as safe
        self.bump_mocktime(10 * 60 + 1)
        block = self.nodes[3].generatetoaddress(1, self.nodes[0].getnewaddress())[0]
        reconnect_isolated_node(self.nodes[3], 0)
        self.wait_for_chainlocked_block_all_nodes(block)
        self.nodes[0].setmocktime(self.mocktime)

        self.log.info("testing retroactive signing with partially known TX")
        isolate_node(self.nodes[3])
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        # Make sure nodes 1 and 2 received the TX before we continue,
        # otherwise it might announce the TX to node 3 when reconnecting
        self.wait_for_tx(txid, self.nodes[1])
        self.wait_for_tx(txid, self.nodes[2])
        reconnect_isolated_node(self.nodes[3], 0)
        # Make sure nodes actually try re-connecting quorum connections
        self.bump_mocktime(30)
        self.wait_for_mnauth(self.nodes[3], 2)
        # node 3 fully reconnected but the TX wasn't relayed to it, so there should be no IS lock
        self.wait_for_instantlock(txid, self.nodes[0], False, 5)
        # Make node0 consider the TX as safe
        self.bump_mocktime(10 * 60 + 1)
        block = self.nodes[0].generate(1)[0]
        assert(txid in self.nodes[0].getblock(block, 1)['tx'])
        self.wait_for_chainlocked_block_all_nodes(block)

        self.log.info("testing retroactive signing with partially known TX and all nodes session timeout")
        self.test_all_nodes_session_timeout(False)
        self.log.info("repeating test, but with cycled LLMQs")
        self.test_all_nodes_session_timeout(True)

        self.log.info("testing retroactive signing with partially known TX and single node session timeout")
        self.test_single_node_session_timeout(False)
        self.log.info("repeating test, but with cycled LLMQs")
        self.test_single_node_session_timeout(True)

    def cycle_llmqs(self):
        self.mine_quorum()
        self.mine_quorum()
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash(), timeout=30)

    def test_all_nodes_session_timeout(self, do_cycle_llmqs):
        set_node_times(self.nodes, self.mocktime)
        isolate_node(self.nodes[3])
        rawtx = self.nodes[0].createrawtransaction([], {self.nodes[0].getnewaddress(): 1})
        rawtx = self.nodes[0].fundrawtransaction(rawtx)['hex']
        rawtx = self.nodes[0].signrawtransactionwithwallet(rawtx)['hex']
        txid = self.nodes[0].sendrawtransaction(rawtx)
        txid = self.nodes[3].sendrawtransaction(rawtx)
        # Make sure nodes 1 and 2 received the TX before we continue
        self.wait_for_tx(txid, self.nodes[1])
        self.wait_for_tx(txid, self.nodes[2])
        # Make sure signing is done on nodes 1 and 2 (it's async)
        time.sleep(5)
        # Make the signing session for the IS lock timeout on nodes 1-3
        self.bump_mocktime(61)
        time.sleep(2) # make sure Cleanup() is called
        reconnect_isolated_node(self.nodes[3], 0)
        # Make sure nodes actually try re-connecting quorum connections
        self.bump_mocktime(30)
        self.wait_for_mnauth(self.nodes[3], 2)
        # node 3 fully reconnected but the signing session is already timed out on all nodes, so no IS lock
        self.wait_for_instantlock(txid, self.nodes[0], False, 5)
        if do_cycle_llmqs:
            self.cycle_llmqs()
            self.wait_for_instantlock(txid, self.nodes[0], False, 5)
        # Make node 0 consider the TX as safe
        self.bump_mocktime(10 * 60 + 1)
        block = self.nodes[0].generate(1)[0]
        assert(txid in self.nodes[0].getblock(block, 1)['tx'])
        self.wait_for_chainlocked_block_all_nodes(block)

    def test_single_node_session_timeout(self, do_cycle_llmqs):
        set_node_times(self.nodes, self.mocktime)
        isolate_node(self.nodes[3])
        rawtx = self.nodes[0].createrawtransaction([], {self.nodes[0].getnewaddress(): 1})
        rawtx = self.nodes[0].fundrawtransaction(rawtx)['hex']
        rawtx = self.nodes[0].signrawtransactionwithwallet(rawtx)['hex']
        txid = self.nodes[3].sendrawtransaction(rawtx)
        time.sleep(2) # make sure signing is done on node 2 (it's async)
        # Make the signing session for the IS lock timeout on node 3
        self.bump_mocktime(61)
        time.sleep(2) # make sure Cleanup() is called
        reconnect_isolated_node(self.nodes[3], 0)
        # Make sure nodes actually try re-connecting quorum connections
        self.bump_mocktime(30)
        self.wait_for_mnauth(self.nodes[3], 2)
        self.nodes[0].sendrawtransaction(rawtx)
        # Make sure nodes 1 and 2 received the TX
        self.wait_for_tx(txid, self.nodes[1])
        self.wait_for_tx(txid, self.nodes[2])
        # Make sure signing is done on nodes 1 and 2 (it's async)
        time.sleep(5)
        # node 3 fully reconnected but the signing session is already timed out on it, so no IS lock
        self.wait_for_instantlock(txid, self.nodes[0], False, 1)
        if do_cycle_llmqs:
            self.cycle_llmqs()
            self.wait_for_instantlock(txid, self.nodes[0], False, 5)
        # Make node 0 consider the TX as safe
        self.bump_mocktime(10 * 60 + 1)
        block = self.nodes[0].generate(1)[0]
        assert(txid in self.nodes[0].getblock(block, 1)['tx'])
        self.wait_for_chainlocked_block_all_nodes(block)

if __name__ == '__main__':
    LLMQ_IS_RetroactiveSigning().main()
