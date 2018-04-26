#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose
from test_framework.util import *

class ExtKeyTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [ ['-debug',] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 0, 2)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        node = self.nodes[0]
        node1 = self.nodes[1]

        # stop staking
        node.reservebalance(True, 10000000)

        ro = node.extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        assert(node.getwalletinfo()['total_balance'] == 100000)

        # Start staking
        node.walletsettings('stakelimit', {'height':1})
        node.reservebalance(False)

        assert(self.wait_for_height(node, 1))

        # stop staking
        node.reservebalance(True, 10000000)
        node1.reservebalance(True, 10000000)

        ro = node1.extkeyimportmaster('drip fog service village program equip minute dentist series hawk crop sphere olympic lazy garbage segment fox library good alley steak jazz force inmate')
        assert(ro['account_id'] == 'ahL1QdHhzNCtZWJzv36ScfPipJP1cUzAD8')

        extAddrTo = node1.getnewextaddress('test label')
        assert(extAddrTo == 'pparszNYZ1cpWxnNieFqHCV2rtXmG74a4WAXHHhXaRATzzU6kMixjy1rXDM1UM4LVgkXRpLNM1rQNvkgLf7kUeMXiyaBMK8aSR3td4b4cX4epnHF')

        ro = node1.filteraddresses()
        assert(len(ro) == 1)
        assert(ro[0]['label'] == 'test label')


        ro = node1.getaddressinfo(extAddrTo)
        assert(ro['ismine'] == True)
        assert(ro['isextkey'] == True)

        ro = node1.dumpprivkey(extAddrTo)
        assert(ro == 'xparFnnG7xJkEekTjWGumcEY1BKgryY4txW5Ce56KQPBJG7u3cNsUHxGgjVwHGEaxUGDAjT4SXv7fkWkp4TFaFHjaoZVh8Zricnwz3DjAxtqtmi')

        txnHash = node.sendtoaddress(extAddrTo, 10)

        ro = node.getmempoolentry(txnHash)
        assert(ro['height'] == 1)

        # start staking
        node.walletsettings('stakelimit', {'height':2})
        node.reservebalance(False)

        assert(self.wait_for_height(node, 2))

        # stop staking
        ro = node.reservebalance(True, 10000000)


        ro = node1.listtransactions()
        assert(len(ro) == 1)
        assert(ro[0]['address'] == 'pkGv5xgviEAEjwpRPeEt8c9cvraw2umKYo')
        assert(ro[0]['amount'] == 10)

        ro = node1.getwalletinfo()
        assert(ro['total_balance'] == 10)


        block2_hash = node.getblockhash(2)

        ro = node.getblock(block2_hash)
        assert(txnHash in ro['tx'])


        txnHash2 = node.sendtoaddress(extAddrTo, 20, '', '', False, 'narration test')

        assert(self.wait_for_mempool(node1, txnHash2))

        ro = node1.listtransactions()
        assert(len(ro) == 2)
        assert(ro[1]['address'] == 'pbo5e7tsLJBdUcCWteTTkGBxjW8Xy12o1V')
        assert(ro[1]['amount'] == 20)
        assert('narration test' in ro[1].values())


        ro = node.listtransactions()
        assert('narration test' in ro[-1].values())


        extAddrTo0 = node.getnewextaddress()

        txnHashes = []
        for k in range(24):
            v = round(0.01 * float(k+1), 5)
            node1.syncwithvalidationinterfacequeue()
            txnHash = node1.sendtoaddress(extAddrTo0, v, '', '', False)
            txnHashes.append(txnHash)

        for txnHash in txnHashes:
            assert(self.wait_for_mempool(node, txnHash))

        ro = node.listtransactions('*', 24)
        assert(len(ro) == 24)
        assert[isclose(ro[0]['amount'], 0.01)]
        assert[isclose(ro[23]['amount'], 0.24)]
        assert[ro[23]['address'] == 'pm23xKs3gy6AhZZ7JZe61Rn1m8VB83P49d']


        # start staking
        ro = node.walletsettings('stakelimit', {'height':3})
        ro = node.reservebalance(False)

        assert(self.wait_for_height(node, 3))

        block3_hash = node.getblockhash(3)
        ro = node.getblock(block3_hash)

        for txnHash in txnHashes:
            assert(txnHash in ro['tx'])


        # Test bech32 encoding
        ek_b32 = 'tpep1q3ehtcetqqqqqqesj04mypkmhnly5rktqmcpmjuq09lyevcsjxrgra6x8trd52vp2vpsk6kf86v3npg6x66ymrn5yrqnclxtqrlfdlw3j4f0309dhxct8kc68paxt'
        assert(node.getnewextaddress('lbl_b32', '', True) == ek_b32)
        assert(ek_b32 in json.dumps(node.filteraddresses()))


if __name__ == '__main__':
    ExtKeyTest().main()
