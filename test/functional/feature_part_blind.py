#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose
from test_framework.util import *

class BlindTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [ ['-debug','-noacceptnonstdtxn'] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 0, 2)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        nodes = self.nodes

        # Stop staking
        for i in range(len(nodes)):
            nodes[i].reservebalance(True, 10000000)

        ro = nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        assert(nodes[0].getwalletinfo()['total_balance'] == 100000)

        txnHashes = []

        ro = nodes[1].extkeyimportmaster('drip fog service village program equip minute dentist series hawk crop sphere olympic lazy garbage segment fox library good alley steak jazz force inmate')
        sxAddrTo1_1 = nodes[1].getnewstealthaddress('lblsx11')
        assert(sxAddrTo1_1 == 'TetbYTGv5LiqyFiUD3a5HHbpSinQ9KiRYDGAMvRzPfz4RnHMbKGAwDr1fjLGJ5Eqg1XDwpeGyqWMiwdK3qM3zKWjzHNpaatdoHVzzA')

        txnHash = nodes[0].sendparttoblind(sxAddrTo1_1, 3.4, '', '', False, 'node0 -> node1 p->b')
        txnHashes.append(txnHash)


        ro = nodes[0].listtransactions()
        assert(len(ro) == 10)

        ro = nodes[0].getwalletinfo()
        assert(isclose(ro['total_balance'], 99996.594196))
        assert(self.wait_for_mempool(nodes[1], txnHash))

        ro = nodes[1].getwalletinfo()
        assert(isclose(ro['unconfirmed_blind'], 3.4))

        ro = nodes[1].transactionblinds(txnHash)
        assert(len(ro) == 2)

        ro = nodes[1].listtransactions()
        assert(len(ro) == 2)

        self.stakeBlocks(2)

        nodes[2].extkeyimportmaster(nodes[2].mnemonic('new')['master'])
        sxAddrTo2_1 = nodes[2].getnewstealthaddress('lblsx21')

        txnHash3 = nodes[1].sendblindtoblind(sxAddrTo2_1, 0.2, '', '', False, 'node1 -> node2 b->b')

        ro = nodes[1].getwalletinfo()
        assert(ro['blind_balance'] < 3.2 and ro['blind_balance'] > 3.1)

        ro = nodes[1].listtransactions()
        assert(len(ro) == 3)
        fFound = False
        for e in ro:
            if e['category'] == 'send':
                assert(e['type'] == 'blind')
                assert(isclose(e['amount'], -0.2))
                fFound = True
        assert(fFound)

        assert(self.wait_for_mempool(nodes[2], txnHash3))


        ro = nodes[2].getwalletinfo()
        assert(isclose(ro['unconfirmed_blind'], 0.2))

        ro = nodes[2].listtransactions()
        assert(len(ro) == 1)
        e = ro[0]
        assert(e['category'] == 'receive')
        assert(e['type'] == 'blind')
        assert(isclose(e['amount'], 0.2))
        assert(e['stealth_address'] == sxAddrTo2_1)


        txnHash4 = nodes[1].sendblindtopart(sxAddrTo2_1, 0.5, '', '', False, 'node1 -> node2 b->p')

        ro = nodes[1].getwalletinfo()
        assert(ro['blind_balance'] < 2.7 and ro['blind_balance'] > 2.69)

        ro = nodes[1].listtransactions()
        assert(len(ro) == 4)
        fFound = False
        for e in ro:
            if e['category'] == 'send' and e['type'] == 'standard':
                assert(isclose(e['amount'], -0.5))
                fFound = True
        assert(fFound)

        assert(self.wait_for_mempool(nodes[2], txnHash4))

        ro = nodes[2].getwalletinfo()
        assert(isclose(ro['unconfirmed_balance'], 0.5))
        assert(isclose(ro['unconfirmed_blind'], 0.2))

        ro = nodes[2].listtransactions()
        assert(len(ro) == 2)



        sxAddrTo2_3 = nodes[2].getnewstealthaddress('n2 sx+prefix', '4', '0xaaaa')
        ro = nodes[2].validateaddress(sxAddrTo2_3)
        assert(ro['isvalid'] == True)
        assert(ro['isstealthaddress'] == True)
        assert(ro['prefix_num_bits'] == 4)
        assert(ro['prefix_bitfield'] == '0x000a')



        txnHash5 = nodes[0].sendparttoblind(sxAddrTo2_3, 0.5, '', '', False, 'node0 -> node2 p->b')

        assert(self.wait_for_mempool(nodes[2], txnHash5))

        ro = nodes[2].listtransactions()
        assert(ro[-1]['txid'] == txnHash5)


        ro = nodes[0].getwalletinfo()
        # Some of the balance will have staked
        assert(isclose(ro['balance'] + ro['staked_balance'], 99996.09498274))
        availableBalance = ro['balance']


        # Check node0 can spend remaining coin
        addrTo0_2 = nodes[0].getnewaddress()
        txnHash2 = nodes[0].sendtoaddress(addrTo0_2, availableBalance, '', '', True, 'node0 spend remaining')
        txnHashes.append(txnHash2)


        nodes[0].syncwithvalidationinterfacequeue()
        ro = nodes[0].getwalletinfo()
        assert(isclose(ro['total_balance'], 99996.09292874))

        ro = nodes[1].getwalletinfo()
        assert(isclose(ro['blind_balance'], 2.691068))

        unspent = nodes[2].listunspentblind(minconf=0)
        assert(len(unspent[0]['stealth_address']))
        assert(len(unspent[0]['label']))

        # Test lockunspent
        unspent = nodes[1].listunspentblind(minconf=0)
        assert(nodes[1].lockunspent(False, [unspent[0]]) == True)
        assert(len(nodes[1].listlockunspent()) == 1)
        unspentCheck = nodes[1].listunspentblind(minconf=0)
        assert(len(unspentCheck) < len(unspent))
        assert(nodes[1].lockunspent(True, [unspent[0]]) == True)
        unspentCheck = nodes[1].listunspentblind(minconf=0)
        assert(len(unspentCheck) == len(unspent))



        outputs = [{'address':sxAddrTo2_3, 'amount':2.691068, 'subfee':True},]
        ro = nodes[1].sendtypeto('blind', 'part', outputs, 'comment_to', 'comment_from', 4, 64, True)
        feePerKB = (1000.0 / ro['bytes']) * float(ro['fee'])
        assert(feePerKB > 0.001 and feePerKB < 0.004)

        ro = nodes[1].sendtypeto('blind', 'blind', outputs, 'comment_to', 'comment_from', 4, 64, True)
        feePerKB = (1000.0 / ro['bytes']) * float(ro['fee'])
        assert(feePerKB > 0.001 and feePerKB < 0.004)

        ro = nodes[1].sendtypeto('blind', 'part', outputs)

        try:
            ro = nodes[1].sendtypeto('blind', 'blind', outputs)
        except JSONRPCException as e:
            assert('Insufficient blinded funds' in e.error['message'])



if __name__ == '__main__':
    BlindTest().main()
