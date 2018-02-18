#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.util import *
from test_framework.address import *


class ColdStakingTest(ParticlTestFramework):
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
        nodes = self.nodes

        # Stop staking
        for i in range(len(nodes)):
            nodes[i].reservebalance(True, 10000000)

        ro = nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        assert(nodes[0].getwalletinfo()['total_balance'] == 100000)

        txnHashes = []

        nodes[1].extkeyimportmaster('drip fog service village program equip minute dentist series hawk crop sphere olympic lazy garbage segment fox library good alley steak jazz force inmate')
        nodes[2].extkeyimportmaster('sección grito médula hecho pauta posada nueve ebrio bruto buceo baúl mitad')

        addr2_1 = nodes[2].getnewaddress()


        ro = nodes[1].extkey('account')
        coldstakingaddr = ''
        for c in ro['chains']:
            if c['function'] != 'active_external':
                continue
            coldstakingaddr = c['chain']
            break
        assert(coldstakingaddr == 'pparszNYZ1cpWxnNiYLgR193XoZMaJBXDkwyeQeQvThTJKjz3sgbR4NjJT3bqAiHBk7Bd5PBRzEqMiHvma9BG6i9qH2iEf4BgYvfr5v3DaXEayNE')

        changeaddress = {'coldstakingaddress':coldstakingaddr}
        ro = nodes[0].walletsettings('changeaddress', changeaddress)
        assert(ro['changeaddress']['coldstakingaddress'] == coldstakingaddr)

        ro = nodes[0].walletsettings('changeaddress')
        assert(ro['changeaddress']['coldstakingaddress'] == coldstakingaddr)

        ro = nodes[0].walletsettings('changeaddress', {})
        assert(ro['changeaddress'] == 'cleared')

        ro = nodes[0].walletsettings('changeaddress')
        assert(ro['changeaddress'] == 'default')

        ro = nodes[0].walletsettings('changeaddress', changeaddress)
        assert(ro['changeaddress']['coldstakingaddress'] == coldstakingaddr)


        # Trying to set a coldstakingchangeaddress known to the wallet should fail
        ro = nodes[0].extkey('account')
        externalChain0 = ''
        for c in ro['chains']:
            if c['function'] != 'active_external':
                continue
            externalChain0 = c['chain']
            break
        assert(externalChain0 == 'pparszMzzW1247AwkKCH1MqneucXJfDoR3M5KoLsJZJpHkcjayf1xUMwPoTcTfUoQ32ahnkHhjvD2vNiHN5dHL6zmx8vR799JxgCw95APdkwuGm1')

        changeaddress = {'coldstakingaddress':externalChain0}
        try:
            ro = nodes[0].walletsettings('changeaddress', changeaddress)
            assert(False), 'Added known address as cold-staking-change-address.'
        except JSONRPCException as e:
            assert('is spendable from this wallet' in e.error['message'])



        txid1 = nodes[0].sendtoaddress(addr2_1, 100)

        tx = nodes[0].getrawtransaction(txid1, True)

        hashCoinstake = ''
        hashOther = ''
        found = False
        for out in tx['vout']:
            asm = out['scriptPubKey']['asm']
            asm = asm.split()
            if asm[0] != 'OP_ISCOINSTAKE':
                continue
            hashCoinstake = asm[4]
            hashOther = asm[10]

        assert(hashCoinstake == '65674e752b3a336337510bf5b57794c71c45cd4f')
        #print('keyhash_to_p2pkh', keyhash_to_p2pkh(hex_str_to_bytes(hashCoinstake)))

        ro = nodes[0].deriverangekeys(0, 0, coldstakingaddr)
        assert(ro[0] == keyhash_to_p2pkh(hex_str_to_bytes(hashCoinstake)))



        ro = nodes[0].extkey('list', 'true')

        fFound = False
        for ek in ro:
            if ek['id'] == 'xBDBWFLeYrbBhPRSKHzVwN61rwUGwCXvUB':
                fFound = True
                assert(ek['evkey'] == 'Unknown')
                assert(ek['num_derives'] == '1')
        assert(fFound)

        assert(self.wait_for_mempool(nodes[1], txid1))

        ro = nodes[1].extkey('key', 'xBDBWFLeYrbBhPRSKHzVwN61rwUGwCXvUB', 'true')
        assert(ro['num_derives'] == '1')

        ro = nodes[1].listtransactions('*', 999999, 0, True)
        assert(len(ro) == 1)

        ro = nodes[1].getwalletinfo()
        last_balance = ro['watchonly_unconfirmed_balance']
        assert(last_balance > 0)





        ekChange = nodes[0].getnewextaddress()
        assert(ekChange == 'pparszMzzW1247AwkR61QFUH6L8zSJDnRvsS8a2FLwfSsgbeusiLNdBkLRXjFb3E5AXVoR6PJTj9nSEF1feCsCyBdGw165XqVcaWs5HiDmcZrLAX')


        changeaddress = {'coldstakingaddress':coldstakingaddr, 'address_standard':ekChange}
        ro = nodes[0].walletsettings('changeaddress', changeaddress)
        assert(ro['changeaddress']['coldstakingaddress'] == coldstakingaddr)
        assert(ro['changeaddress']['address_standard'] == ekChange)



        txid2 = nodes[0].sendtoaddress(addr2_1, 100)

        tx = nodes[0].getrawtransaction(txid2, True)

        hashCoinstake = ''
        hashSpend = ''
        found = False
        for out in tx['vout']:
            asm = out['scriptPubKey']['asm']
            asm = asm.split()
            if asm[0] != 'OP_ISCOINSTAKE':
                continue
            hashCoinstake = asm[4]
            hashSpend = asm[10]

        assert(hashCoinstake == '1ac277619e43a7e0558c612f86b918104742f65c')
        assert(hashSpend == '55e9e9b1aebf76f2a2ce9d7af6267be996bc235e3a65fa0f87a345267f9b3895')


        ro = nodes[0].deriverangekeys(1, 1, coldstakingaddr)
        assert(ro[0] == keyhash_to_p2pkh(hex_str_to_bytes(hashCoinstake)))

        ro = nodes[0].deriverangekeys(0, 0, ekChange, 'false', 'false', 'false', 'true')
        assert(ro[0] == keyhash_to_p2pkh(hex_str_to_bytes(hashSpend)))



        ro = nodes[0].extkey('list', 'true')
        fFound = False
        for ek in ro:
            if ek['id'] == 'xBDBWFLeYrbBhPRSKHzVwN61rwUGwCXvUB':
                fFound = True
                assert(ek['evkey'] == 'Unknown')
                assert(ek['num_derives'] == '2')
        assert(fFound)

        ro = nodes[0].extkey('account')
        fFound = False
        for chain in ro['chains']:
            if chain['id'] == 'xXZRLYvJgbJyrqJhgNzMjEvVGViCdGmVAt':
                fFound = True
                assert(chain['num_derives'] == '1')
                assert(chain['path'] == 'm/0h/2')
        assert(fFound)


        assert(self.wait_for_mempool(nodes[1], txid2))

        ro = nodes[1].extkey('key', 'xBDBWFLeYrbBhPRSKHzVwN61rwUGwCXvUB', 'true')
        assert(ro['num_derives'] == '2')

        ro = nodes[1].listtransactions('*', 999999, 0, True)
        assert(len(ro) == 2)

        ro = nodes[1].getwalletinfo()
        assert(ro['watchonly_unconfirmed_balance'] > last_balance)



        txid3 = nodes[0].sendtoaddress(addr2_1, 100)

        tx = nodes[0].getrawtransaction(txid3, True)

        hashCoinstake = ''
        hashSpend = ''
        found = False
        for out in tx['vout']:
            asm = out['scriptPubKey']['asm']
            asm = asm.split()
            if asm[0] != 'OP_ISCOINSTAKE':
                continue
            hashCoinstake = asm[4]
            hashSpend = asm[10]

        ro = nodes[0].deriverangekeys(2, 2, coldstakingaddr)
        assert(ro[0] == keyhash_to_p2pkh(hex_str_to_bytes(hashCoinstake)))

        ro = nodes[0].deriverangekeys(1, 1, ekChange, 'false', 'false', 'false', 'true')
        assert(ro[0] == keyhash_to_p2pkh(hex_str_to_bytes(hashSpend)))

        ro = nodes[0].extkey('list', 'true')
        fFound = False
        for ek in ro:
            if ek['id'] == 'xBDBWFLeYrbBhPRSKHzVwN61rwUGwCXvUB':
                fFound = True
                assert(ek['evkey'] == 'Unknown')
                assert(ek['num_derives'] == '3')
        assert(fFound)

        ro = nodes[0].extkey('account')
        fFound = False
        for chain in ro['chains']:
            if chain['id'] == 'xXZRLYvJgbJyrqJhgNzMjEvVGViCdGmVAt':
                fFound = True
                assert(chain['num_derives'] == '2')
                assert(chain['path'] == 'm/0h/2')
        assert(fFound)

        assert(self.wait_for_mempool(nodes[1], txid3))

        ro = nodes[1].extkey('key', 'xBDBWFLeYrbBhPRSKHzVwN61rwUGwCXvUB', 'true')
        assert(ro['num_derives'] == '3')


        # Test stake to coldstakingchangeaddress
        ro = nodes[0].walletsettings('stakelimit', {'height':2})
        ro = nodes[0].reservebalance(False)

        assert(self.wait_for_height(nodes[0], 2))
        self.sync_all()

        ro = nodes[1].getwalletinfo()
        assert(ro['watchonly_staked_balance'] > 0)

        ro = nodes[0].extkey('list', 'true')
        fFound = False
        for ek in ro:
            if ek['id'] == 'xBDBWFLeYrbBhPRSKHzVwN61rwUGwCXvUB':
                fFound = True
                assert(ek['evkey'] == 'Unknown')
                assert(ek['num_derives'] == '5')
        assert(fFound)

        # Test mapRecord watchonly
        ro = nodes[1].getwalletinfo()
        wotBefore = ro['watchonly_total_balance']

        n1unspent = nodes[1].listunspent()
        addr2_1s = nodes[2].getnewstealthaddress()

        coincontrol = {'inputs':[{'tx':n1unspent[0]['txid'],'n':n1unspent[0]['vout']}]}
        outputs = [{'address':addr2_1s, 'amount':1, 'narr':'p2b,0->2'},]
        txid = nodes[0].sendtypeto('part', 'blind', outputs, 'comment', 'comment-to', 4, 64, False, coincontrol)

        self.sync_all()

        ro = nodes[1].getwalletinfo()
        wotAfter = ro['watchonly_total_balance']
        assert(wotAfter > wotBefore-Decimal(2.0))

        ro = nodes[1].listtransactions('*', 10, 0)
        assert(len(ro) == 0)

        ro = nodes[1].listtransactions('*', 10, 0, True)

        fFound = False
        for e in ro:
            if e['txid'] == txid:
                fFound = True
                assert(e['involvesWatchonly'] == True)
        assert(fFound)



        #assert(False)
        #print(json.dumps(ro, indent=4, default=self.jsonDecimal))


if __name__ == '__main__':
    ColdStakingTest().main()
