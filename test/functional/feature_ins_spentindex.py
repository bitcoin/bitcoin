#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test addressindex generation and fetching
#

from test_framework.test_particl import ParticlTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *

class SpentIndexTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.extra_args = [ ['-debug',] for i in range(self.num_nodes)]
        self.extra_args = [
            # Nodes 0/1 are "wallet" nodes
            ['-debug',],
            ['-debug','-spentindex'],
            # Nodes 2/3 are used for testing
            ['-debug','-spentindex'],
            ['-debug','-spentindex', '-txindex'],]

    def setup_network(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[0], 3)

        self.is_network_split = False
        self.sync_all()

    def run_test(self):

        nodes = self.nodes

        # Stop staking
        nodes[0].reservebalance(True, 10000000)
        nodes[1].reservebalance(True, 10000000)
        nodes[2].reservebalance(True, 10000000)
        nodes[3].reservebalance(True, 10000000)


        ro = nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        assert(nodes[0].getwalletinfo()['total_balance'] == 100000)

        ro = nodes[1].extkeyimportmaster('graine article givre hublot encadrer admirer stipuler capsule acajou paisible soutirer organe')
        ro = nodes[2].extkeyimportmaster('sección grito médula hecho pauta posada nueve ebrio bruto buceo baúl mitad')
        ro = nodes[3].extkeyimportmaster('けっこん　ゆそう　へいねつ　しあわせ　ちまた　きつね　たんたい　むかし　たかい　のいず　こわもて　けんこう')

        addrs = []
        addrs.append(nodes[1].getnewaddress())
        addrs.append(nodes[1].getnewaddress())


        # Check that
        print('Testing spent index...')

        unspent = nodes[0].listunspent()


        #{\"txid\":\"id\",\"vout\":n}
        inputs = [{'txid':unspent[0]['txid'],'vout':unspent[0]['vout']},]
        outputs = {addrs[0]:1}
        tx = nodes[0].createrawtransaction(inputs,outputs)

        # Add change output
        txfunded = nodes[0].fundrawtransaction(tx)

        txsigned = nodes[0].signrawtransaction(txfunded['hex'])

        txd = nodes[0].decoderawtransaction(txsigned['hex'])

        sent_txid = nodes[0].sendrawtransaction(txsigned['hex'], True)


        self.stakeBlocks(1)

        print("Testing getspentinfo method...")

        # Check that the spentinfo works standalone
        info = self.nodes[1].getspentinfo({"txid": unspent[0]["txid"], "index": unspent[0]["vout"]})
        assert_equal(info["txid"], sent_txid)
        assert_equal(info["index"], 0)
        assert_equal(info["height"], 1)

        print("Testing getrawtransaction method...")

        # Check that verbose raw transaction includes spent info
        txVerbose = self.nodes[3].getrawtransaction(unspent[0]["txid"], 1)
        assert_equal(txVerbose["vout"][unspent[0]["vout"]]["spentTxId"], sent_txid)
        assert_equal(txVerbose["vout"][unspent[0]["vout"]]["spentIndex"], 0)
        assert_equal(txVerbose["vout"][unspent[0]["vout"]]["spentHeight"], 1)

        # Check that verbose raw transaction includes address values and input values
        txVerbose2 = self.nodes[3].getrawtransaction(sent_txid, 1)
        assert_equal(txVerbose2["vin"][0]["address"], 'pcwP4hTtaMb7n4urszBTsgxWLdNLU4yNGz')
        assert(float(txVerbose2["vin"][0]["value"]) > 0)
        assert(txVerbose2["vin"][0]["valueSat"] > 0)


        # Check the mempool index
        txid2 = nodes[0].sendtoaddress(addrs[1], 5)
        self.sync_all()
        txVerbose3 = self.nodes[1].getrawtransaction(txid2, 1)
        assert(len(txVerbose3["vin"][0]["address"]) > 0)
        assert(float(txVerbose3["vin"][0]["value"]) > 0)
        assert(txVerbose3["vin"][0]["valueSat"] > 0)


        # Check the database index
        self.stakeBlocks(1)

        block1_hash = nodes[1].getblockhash(nodes[1].getblockcount())
        ro = nodes[1].getblock(block1_hash)
        assert(txid2 in ro['tx'])


        txVerbose4 = self.nodes[3].getrawtransaction(txid2, 1)
        assert(len(txVerbose4["vin"][0]["address"]) > 0)
        assert(float(txVerbose4["vin"][0]["value"]) > 0)
        assert(txVerbose4["vin"][0]["valueSat"] > 0)


        # Check block deltas
        print("Testing getblockdeltas...")

        block = nodes[3].getblockdeltas(block1_hash)
        assert_equal(len(block["deltas"]), 2)

        assert_equal(block["deltas"][0]["index"], 0)
        assert_equal(len(block["deltas"][0]["inputs"]), 1)
        assert_equal(len(block["deltas"][0]["outputs"]), 2)

        assert_equal(block["deltas"][1]["index"], 1)
        assert_equal(block["deltas"][1]["txid"], txid2)
        assert_equal(len(block["deltas"][1]["inputs"]), 1)
        assert_equal(len(block["deltas"][1]["outputs"]), 2)

        fFound = False
        for out in block["deltas"][1]["outputs"]:
            if out["satoshis"] == 500000000 and out["address"] == addrs[1]:
                fFound = True
                break
        assert(fFound)

        print("Passed\n")


if __name__ == '__main__':
    SpentIndexTest().main()
