#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose
from test_framework.util import *

class StealthTest(ParticlTestFramework):
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
        node = self.nodes[0]
        node1 = self.nodes[1]
        node2 = self.nodes[2]

        # Stop staking
        ro = node.reservebalance(True, 10000000)
        ro = node1.reservebalance(True, 10000000)
        ro = node2.reservebalance(True, 10000000)

        ro = node.extkeyimportmaster("abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb")
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')

        #ro = node.extkey("list", "true")
        #print(json.dumps(ro, indent=4))

        ro = node.getinfo()
        assert(ro['total_balance'] == 100000)

        txnHashes = []


        ro = node1.extkeyimportmaster("drip fog service village program equip minute dentist series hawk crop sphere olympic lazy garbage segment fox library good alley steak jazz force inmate")

        sxAddrTo1 = node1.getnewstealthaddress()
        assert(sxAddrTo1 == 'TetbYTGv5LiqyFiUD3a5HHbpSinQ9KiRYDGAMvRzPfz4RnHMbKGAwDr1fjLGJ5Eqg1XDwpeGyqWMiwdK3qM3zKWjzHNpaatdoHVzzA')

        txnHash = node.sendtoaddress(sxAddrTo1, 1)
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(node1, txnHash))

        ro = node1.listtransactions()
        assert(len(ro) == 1)
        assert(ro[0]['stealth_address'] == sxAddrTo1)
        assert(isclose(ro[0]['amount'], 1))

        # Test imported sx address
        ro = node1.importstealthaddress("7pJLDnLxoYmkwpMNDX69dWGT7tuZ45LHgMajQDD8JrXb9LHmzfBA",
            "7uk8ELaUsop2r4vMg415wEGBfRd1MmY7JiXX7CRhwuwq5PaWXQ9N", "importedAddr", "5", "0xaaaa")
        sxAddrTo2 = '32eEcCuGkGjP82BTF3kquiCDjZWmZiyhqe7C6isbv6MJZSKAeWNx5g436QuhGNc6DNYpboDm3yNiqYmTmkg76wYr5JCKgdEUPqLCWaMW'
        assert(ro['stealth_address'] == sxAddrTo2)


        ro = node1.liststealthaddresses()
        sro = str(ro)
        assert(sxAddrTo1 in sro)
        assert(sxAddrTo2 in sro)

        txnHash = node.sendtoaddress(sxAddrTo2, 0.2)
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(node1, txnHash))

        ro = node1.listtransactions()

        sxAddrTo3 = node1.getnewstealthaddress()
        assert(sxAddrTo3 == 'TetcV5ZNzM6hT6Tz8Vc5t6FM74nojY8oFdeCnPr9Vyx6QNqrR7LKy87aZ1ytGGqBSAJ9CpWDj81pPwYPYHjg6Ks8GKXvGyLoBdTDYQ')

        txnHash = node.sendtoaddress(sxAddrTo3, 0.3, '', '', False, 'narration test')
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(node1, txnHash))

        ro = node1.listtransactions()
        print(json.dumps(ro, indent=4, default=self.jsonDecimal))
        assert(ro[-1]['stealth_address'] == sxAddrTo3)
        assert(isclose(ro[-1]['amount'], 0.3))
        assert('narration test' in str(ro[-1]))



        # - Test encrypted narrations on sent wtx (from sending node)
        ro = node.listtransactions()
        assert('narration test' in str(ro[-1]))


        oRoot2 = node2.mnemonic("new")
        assert('mnemonic' in oRoot2)

        ro = node2.extkeyimportmaster(oRoot2['mnemonic'])
        assert('Success.' in ro['result'])

        sxAddrTo2_1 = node2.getnewstealthaddress('lbl test 3 bits', "3")


        ro = node2.importstealthaddress("7uk8ELaUsop2r4vMg415wEGBfRd1MmY7JiXX7CRhwuwq5PaWXQ9N", # secrets are backwards
            "7pJLDnLxoYmkwpMNDX69dWGT7tuZ45LHgMajQDD8JrXb9LHmzfBA", "importedAddr", "9", "0xabcd")
        sxAddrTo2_2 = '9xFM875J9ApSymuT9Yuz6mqmy36JE1tNANGZmvS6hXFoQaV7ZLx8ZGjT2WULbuxwY4EsmNhHivLd4f8SRkxSjGjUED51SA4WqJRysUk9f'
        assert(ro['stealth_address'] == sxAddrTo2_2)


        ro = node2.encryptwallet("qwerty234")
        assert("wallet encrypted" in ro)

        self.nodes[2].wait_until_stopped() # wait until encryptwallet has shut down node
        # Restart node 2
        self.start_node(2, self.extra_args[2])
        node2 = self.nodes[2]
        ro = node2.walletpassphrase("qwerty234", 300)
        ro = node2.reservebalance(True, 10000000)
        ro = node2.walletlock()
        connect_nodes_bi(self.nodes, 0, 2)

        # Test send to locked wallet
        txnHash = node.sendtoaddress(sxAddrTo2_1, 0.4, '', '', False, 'narration test node2')
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(node2, txnHash))

        ro = node2.listtransactions()
        assert(isclose(ro[-1]['amount'], 0.4))
        assert('narration test node2' in str(ro[-1]))

        txnHash = node.sendtoaddress(sxAddrTo2_2, 0.5, '', '', False, 'test 5')
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(node2, txnHash))

        ro = node2.listtransactions()
        assert(isclose(ro[-1]['amount'], 0.5))
        assert('test 5' in str(ro[-1]))

        txnHash = node.sendtoaddress(sxAddrTo2_2, 0.6, '', '', False, 'test 6')
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(node2, txnHash))

        ro = node2.listtransactions()
        assert(isclose(ro[-1]['amount'], 0.6))
        assert('test 6' in str(ro[-1]))

        ro = node2.walletpassphrase("qwerty234", 300)



        # Start staking
        ro = node.walletsettings('stakelimit', {'height':1})
        ro = node.reservebalance(False)

        assert(self.wait_for_height(node, 1))

        block1_hash = node.getblockhash(1)
        ro = node.getblock(block1_hash)
        for txnHash in txnHashes:
            assert(txnHash in ro['tx'])

        self.sync_all()
        ro = node2.getwalletinfo()
        print("node2.getwalletinfo() ", ro)

        txnHash = node2.sendtoaddress(sxAddrTo3, 1.4, '', '', False, 'node2 -> node1')
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(node1, txnHash))
        ro = node1.listtransactions()
        assert(isclose(ro[-1]['amount'], 1.4))
        assert('node2 -> node1' in str(ro[-1]))


        #assert(False)
        #print(json.dumps(ro, indent=4, default=self.jsonDecimal))

if __name__ == '__main__':
    StealthTest().main()
