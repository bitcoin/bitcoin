#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose
from test_framework.util import *

class BlindTest(ParticlTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [ ['-debug','-noacceptnonstdtxn'] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.nodes = self.start_nodes(self.num_nodes, self.options.tmpdir, self.extra_args)

        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 0, 2)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        nodes = self.nodes

        # Stop staking
        ro = nodes[0].reservebalance(True, 10000000)
        ro = nodes[1].reservebalance(True, 10000000)
        ro = nodes[2].reservebalance(True, 10000000)

        ro = nodes[0].extkeyimportmaster("abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb")
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')

        ro = nodes[0].getinfo()
        assert(ro['total_balance'] == 100000)

        txnHashes = []

        #assert(self.wait_for_height(node, 1))

        ro = nodes[1].extkeyimportmaster("drip fog service village program equip minute dentist series hawk crop sphere olympic lazy garbage segment fox library good alley steak jazz force inmate")
        sxAddrTo1_1 = nodes[1].getnewstealthaddress()
        assert(sxAddrTo1_1 == 'TetbYTGv5LiqyFiUD3a5HHbpSinQ9KiRYDGAMvRzPfz4RnHMbKGAwDr1fjLGJ5Eqg1XDwpeGyqWMiwdK3qM3zKWjzHNpaatdoHVzzA')

        txnHash = nodes[0].sendparttoblind(sxAddrTo1_1, 3.4, '', '', False, 'node0 -> node1 p->b')
        print("txnHash ", txnHash)
        txnHashes.append(txnHash)



        ro = nodes[0].listtransactions()
        print("0 listtransactions " + json.dumps(ro, indent=4, default=self.jsonDecimal))

        ro = nodes[0].getwalletinfo()
        print("0 getwalletinfo " + json.dumps(ro, indent=4, default=self.jsonDecimal))
        assert(isclose(ro['total_balance'], 99996.594196))


        assert(self.wait_for_mempool(nodes[1], txnHash))

        ro = nodes[1].getwalletinfo()
        print("1 getwalletinfo ", json.dumps(ro, indent=4, default=self.jsonDecimal))

        ro = nodes[1].listtransactions()
        print("1 listtransactions ", json.dumps(ro, indent=4, default=self.jsonDecimal))



        self.stakeBlocks(2)


        mnemonic2 = nodes[2].mnemonic("new");
        ro = nodes[2].extkeyimportmaster(mnemonic2["master"])
        sxAddrTo2_1 = nodes[2].getnewstealthaddress()

        txnHash3 = nodes[1].sendblindtoblind(sxAddrTo2_1, 0.2, '', '', False, 'node1 -> node2 b->b')
        print("txnHash3 ", txnHash3)

        ro = nodes[1].getwalletinfo()
        print("1 getwalletinfo ", json.dumps(ro, indent=4, default=self.jsonDecimal))

        ro = nodes[1].listtransactions()
        print("1 listtransactions ", json.dumps(ro, indent=4, default=self.jsonDecimal))

        assert(self.wait_for_mempool(nodes[2], txnHash3))


        ro = nodes[2].getwalletinfo()
        print("2 getwalletinfo ", json.dumps(ro, indent=4, default=self.jsonDecimal))

        ro = nodes[2].listtransactions()
        print("2 listtransactions ", json.dumps(ro, indent=4, default=self.jsonDecimal))


        # TODO: Depending on the random split there may not be enough funds in the unspent output here
        sxAddrTo2_2 = nodes[2].getnewextaddress();
        txnHash4 = nodes[1].sendblindtopart(sxAddrTo2_1, 0.5, '', '', False, 'node1 -> node2 b->p')
        #txnHash4 = nodes[1].sendblindtopart(sxAddrTo2_2, 0.5, '', '', False, 'node1 -> node2 b->p')
        print("txnHash4 ", txnHash4)


        ro = nodes[1].getwalletinfo()
        print("1 getwalletinfo ", json.dumps(ro, indent=4, default=self.jsonDecimal))

        ro = nodes[1].listtransactions()
        print("1 listtransactions ", json.dumps(ro, indent=4, default=self.jsonDecimal))


        assert(self.wait_for_mempool(nodes[2], txnHash4))

        ro = nodes[2].getwalletinfo()
        print("2 getwalletinfo ", json.dumps(ro, indent=4, default=self.jsonDecimal))

        ro = nodes[2].listtransactions()
        print("2 listtransactions ", json.dumps(ro, indent=4, default=self.jsonDecimal))



        sxAddrTo2_3 = nodes[2].getnewstealthaddress("n2 sx+prefix", "4", "0xaaaa");
        print("sxAddrTo2_3 ", sxAddrTo2_3)
        #assert(sxAddrTo2_3 == "32eETczfqSdXkByymvCGrwtkUUkwVfiSosd82tu1K4H7MKzFze6L5pyQL6R29qGzBxuUhLaeg1pEaAhvqicVeWScsdN19kH83JYPC1Tn")

        txnHash5 = nodes[0].sendparttoblind(sxAddrTo2_3, 0.5, '', '', False, 'node0 -> node2 p->b')
        print("txnHash5 ", txnHash5)

        assert(self.wait_for_mempool(nodes[2], txnHash5))

        ro = nodes[2].listtransactions()
        #print("2 listtransactions ", json.dumps(ro, indent=4, default=self.jsonDecimal))
        assert(ro[-1]["txid"] == txnHash5)


        ro = nodes[0].getwalletinfo()
        #print("0 getwalletinfo " + json.dumps(ro, indent=4, default=self.jsonDecimal))
        # Some of the balance will have staked
        assert(isclose(ro['balance'] + ro['staked_balance'], 99996.09498274))
        availableBalance = ro['balance'];


        # Check node0 can spend remaining coin
        addrTo0_2 = nodes[0].getnewaddress()
        txnHash2 = nodes[0].sendtoaddress(addrTo0_2, availableBalance, '', '', True, 'node0 spend remaining')
        #print("txnHash2 ", txnHash2)
        txnHashes.append(txnHash2)

        ro = nodes[0].getwalletinfo()
        #print("0 getwalletinfo " + json.dumps(ro, indent=4, default=self.jsonDecimal))
        assert(isclose(ro['total_balance'], 99996.09294874))


        #assert(False)
        #print(json.dumps(ro, indent=4, default=self.jsonDecimal))


if __name__ == '__main__':
    BlindTest().main()
