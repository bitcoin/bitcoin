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
        nodes = self.nodes
        txnHashes = []

        # Stop staking
        for i in range(len(nodes)):
            nodes[i].reservebalance(True, 10000000)

        ro = nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        assert(nodes[0].getwalletinfo()['total_balance'] == 100000)


        ro = nodes[1].extkeyimportmaster('drip fog service village program equip minute dentist series hawk crop sphere olympic lazy garbage segment fox library good alley steak jazz force inmate')
        sxAddrTo1 = nodes[1].getnewstealthaddress()
        assert(sxAddrTo1 == 'TetbYTGv5LiqyFiUD3a5HHbpSinQ9KiRYDGAMvRzPfz4RnHMbKGAwDr1fjLGJ5Eqg1XDwpeGyqWMiwdK3qM3zKWjzHNpaatdoHVzzA')

        ro = nodes[1].getaddressinfo(sxAddrTo1)
        assert(ro['ismine'] == True)
        assert(ro['isstealthaddress'] == True)

        txnHash = nodes[0].sendtoaddress(sxAddrTo1, 1)
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(nodes[1], txnHash))

        ro = nodes[1].listtransactions()
        assert(len(ro) == 1)
        assert(ro[0]['stealth_address'] == sxAddrTo1)
        assert(isclose(ro[0]['amount'], 1))

        ro = nodes[1].getaddressinfo(ro[0]['address'])
        assert(ro['ismine'] == True)
        assert(ro['from_stealth_address'] == sxAddrTo1)

        # Test imported sx address
        ro = nodes[1].importstealthaddress('7pJLDnLxoYmkwpMNDX69dWGT7tuZ45LHgMajQDD8JrXb9LHmzfBA',
            '7uk8ELaUsop2r4vMg415wEGBfRd1MmY7JiXX7CRhwuwq5PaWXQ9N', 'importedAddr', '5', '0xaaaa')
        sxAddrTo2 = '32eEcCuGkGjP82BTF3kquiCDjZWmZiyhqe7C6isbv6MJZSKAeWNx5g436QuhGNc6DNYpboDm3yNiqYmTmkg76wYr5JCKgdEUPqLCWaMW'
        assert(ro['stealth_address'] == sxAddrTo2)


        ro = nodes[1].liststealthaddresses()
        sro = str(ro)
        assert(sxAddrTo1 in sro)
        assert(sxAddrTo2 in sro)

        txnHash = nodes[0].sendtoaddress(sxAddrTo2, 0.2)
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(nodes[1], txnHash))

        ro = nodes[1].listtransactions()

        sxAddrTo3 = nodes[1].getnewstealthaddress()
        assert(sxAddrTo3 == 'TetcV5ZNzM6hT6Tz8Vc5t6FM74nojY8oFdeCnPr9Vyx6QNqrR7LKy87aZ1ytGGqBSAJ9CpWDj81pPwYPYHjg6Ks8GKXvGyLoBdTDYQ')

        txnHash = nodes[0].sendtoaddress(sxAddrTo3, 0.3, '', '', False, 'narration test')
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(nodes[1], txnHash))

        ro = nodes[1].listtransactions()
        assert(ro[-1]['stealth_address'] == sxAddrTo3)
        assert(isclose(ro[-1]['amount'], 0.3))
        assert('narration test' in str(ro[-1]))



        # - Test encrypted narrations on sent wtx (from sending nodes[0])
        ro = nodes[0].listtransactions()
        assert('narration test' in str(ro[-1]))


        oRoot2 = nodes[2].mnemonic('new')
        assert('mnemonic' in oRoot2)

        ro = nodes[2].extkeyimportmaster(oRoot2['mnemonic'])
        assert('Success.' in ro['result'])

        sxAddrTo2_1 = nodes[2].getnewstealthaddress('lbl test 3 bits', '3')


        ro = nodes[2].importstealthaddress('7uk8ELaUsop2r4vMg415wEGBfRd1MmY7JiXX7CRhwuwq5PaWXQ9N', # secrets are backwards
            '7pJLDnLxoYmkwpMNDX69dWGT7tuZ45LHgMajQDD8JrXb9LHmzfBA', 'importedAddr', '9', '0xabcd')
        sxAddrTo2_2 = '9xFM875J9ApSymuT9Yuz6mqmy36JE1tNANGZmvS6hXFoQaV7ZLx8ZGjT2WULbuxwY4EsmNhHivLd4f8SRkxSjGjUED51SA4WqJRysUk9f'
        assert(ro['stealth_address'] == sxAddrTo2_2)


        nodes[2].node_encrypt_wallet('qwerty234')
        self.start_node(2)

        ro = nodes[2].walletpassphrase('qwerty234', 300)
        ro = nodes[2].reservebalance(True, 10000000)
        ro = nodes[2].walletlock()
        connect_nodes_bi(self.nodes, 0, 2)

        # Test send to locked wallet
        txnHash = nodes[0].sendtoaddress(sxAddrTo2_1, 0.4, '', '', False, 'narration test node2')
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(nodes[2], txnHash))

        ro = nodes[2].listtransactions()
        assert(isclose(ro[-1]['amount'], 0.4))
        assert('narration test node2' in str(ro[-1]))

        txnHash = nodes[0].sendtoaddress(sxAddrTo2_2, 0.5, '', '', False, 'test 5')
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(nodes[2], txnHash))

        ro = nodes[2].listtransactions()
        assert(isclose(ro[-1]['amount'], 0.5))
        assert('test 5' in str(ro[-1]))

        txnHash = nodes[0].sendtoaddress(sxAddrTo2_2, 0.6, '', '', False, 'test 6')
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(nodes[2], txnHash))

        ro = nodes[2].listtransactions()
        assert(isclose(ro[-1]['amount'], 0.6))
        assert('test 6' in str(ro[-1]))

        ro = nodes[2].walletpassphrase('qwerty234', 400)



        # Start staking
        ro = nodes[0].walletsettings('stakelimit', {'height':1})
        ro = nodes[0].reservebalance(False)

        assert(self.wait_for_height(nodes[0], 1))

        block1_hash = nodes[0].getblockhash(1)
        ro = nodes[0].getblock(block1_hash)
        for txnHash in txnHashes:
            assert(txnHash in ro['tx'])

        self.sync_all()
        ro = nodes[2].getwalletinfo()

        txnHash = nodes[2].sendtoaddress(sxAddrTo3, 1.4, '', '', False, 'node2 -> node1')
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(nodes[1], txnHash))
        ro = nodes[1].listtransactions()
        assert(isclose(ro[-1]['amount'], 1.4))
        assert('node2 -> node1' in str(ro[-1]))


        assert_raises_rpc_error(-8, 'Spend secret must be different to scan secret.',
            nodes[0].importstealthaddress, '57F59BA2F9D9146635380FA453E4EED7E44290F0AAAE657C300AB0E51184151E',
            '57F59BA2F9D9146635380FA453E4EED7E44290F0AAAE657C300AB0E51184151E', '', '9', '0xabcd', True)

        # Test bech32 encoding
        ro = nodes[0].importstealthaddress('57F59BA2F9D9146635380FA453E4EED7E44290F0AAAE657C300AB0E51184151E',
            'F4708BD9B3D367F02EA6581AC446F12AA24350F1E0DEE309FACB561C2D81877A', 'importedAddr1b32', '9', '0xabcd', True)
        sx1_b32 = 'tps1qqpt67v3g652e5mvqv3hl60zkj3424l0chhvsc0ervrltujd7dgl6uspqfd2hf5dglxt285yp9d9h3nym39cdzdpr5ndcm940lhmg6z3tjuvzqqfe5qs2gep0t'
        assert(ro['stealth_address'] == sx1_b32)

        ro = nodes[0].getnewstealthaddress('importedAddr2b32', '', '', True)
        sx2_b32 = 'tps1qqpxdqs29p7sadu3fqpc808aw93a25k9zjy6f5hzgyf3vluy3n4f4ygpqwkkfhujgzgq83y55enleldy9lh7tkv6dmkaxjrjtyaswjcx3mjyyqqqhefkp7'
        assert(ro == sx2_b32)
        flat = json.dumps(nodes[0].filteraddresses())
        assert(sx1_b32 in flat and sx2_b32 in flat)

        ro = nodes[1].createrawtransaction([], {sx1_b32:0.1})
        ro = nodes[1].fundrawtransaction(ro)
        ro = nodes[1].signrawtransactionwithwallet(ro['hex'])
        assert(ro['complete'] == True)
        txnHash = nodes[1].sendrawtransaction(ro['hex'])
        txnHashes.append(txnHash)

        assert(self.wait_for_mempool(nodes[0], txnHash))

        ro = nodes[0].filtertransactions({ 'use_bech32': True })
        assert(ro[0]['outputs'][0]['stealth_address'] == sx1_b32)

        ro = nodes[0].derivefromstealthaddress(sx2_b32)
        assert(len(ro) == 4)
        assert(len(ro['pubkey']) == 66)
        assert(len(ro['ephemeral_pubkey']) == 66)

        recover = nodes[0].derivefromstealthaddress(sx2_b32, ro['ephemeral_pubkey'])
        assert(recover['pubkey'] == ro['pubkey'])
        assert(recover['ephemeral_pubkey'] == ro['ephemeral_pubkey'])
        assert(len(recover['privatekey']) > 0)

        replay = nodes[0].derivefromstealthaddress(sx2_b32, ro['ephemeral_privatekey'])
        assert(replay['pubkey'] == ro['pubkey'])
        assert(replay['ephemeral_pubkey'] == ro['ephemeral_pubkey'])


if __name__ == '__main__':
    StealthTest().main()
