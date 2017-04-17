#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.util import *

import decimal

def isclose(a, b, rel_tol=1e-09, abs_tol=0.0):
    a = decimal.Decimal(a)
    b = decimal.Decimal(b)
    return abs(a-b) <= max(decimal.Decimal(rel_tol) * decimal.Decimal(max(abs(a), abs(b))), abs_tol)

class MultiSigTest(ParticlTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [ ['-debug',] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, self.extra_args, genfirstkey=False)
        
        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 0, 2)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        nodes = self.nodes
        
        ro = nodes[0].extkeyimportmaster("abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb")
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        
        ro = nodes[0].getinfo()
        assert(ro['balance'] == 100000)
        
        #txnHashes = []
        #assert(self.wait_for_height(node, 1))
        
        # stop staking
        ro = nodes[0].reservebalance(True, 10000000)
        ro = nodes[1].reservebalance(True, 10000000)
        
        addrs = []
        pubkeys = []
        
        ro = nodes[0].getnewaddress();
        addrs.append(ro)
        ro = nodes[0].validateaddress(ro);
        pubkeys.append(ro['pubkey'])
        
        ro = nodes[0].getnewaddress();
        addrs.append(ro)
        ro = nodes[0].validateaddress(ro);
        pubkeys.append(ro['pubkey'])
        
        ro = nodes[1].extkeyimportmaster("drip fog service village program equip minute dentist series hawk crop sphere olympic lazy garbage segment fox library good alley steak jazz force inmate")
        
        ro = nodes[1].getnewaddress();
        addrs.append(ro)
        ro = nodes[1].validateaddress(ro);
        pubkeys.append(ro['pubkey'])
        
        
        
        mn2 = nodes[2].mnemonic("new", "", "french")
        
        ro = nodes[2].extkeyimportmaster(mn2['mnemonic'])
        
        ro = nodes[2].getnewaddress();
        addrs.append(ro)
        ro = nodes[2].validateaddress(ro);
        pubkeys.append(ro['pubkey'])
        
        #v = '["'+addrs[0]+'","'+addrs[1]+'","'+pubkeys[2]+'","'+pubkeys[3]+'"]'
        #v = [addrs[0],addrs[1],pubkeys[2],pubkeys[3]]
        v = [addrs[0],addrs[1],pubkeys[2]]
        #print("v ", v)
        ro = nodes[0].addmultisigaddress(2, v)
        msAddr = ro
        #print("msAddr ", msAddr)
        
        ro = nodes[0].validateaddress(msAddr);
        #print("ro ", ro)
        assert(ro['isscript'] == True)
        scriptPubKey = ro['scriptPubKey']
        redeemScript = ro['hex']
        
        
        
        ro = nodes[0].sendtoaddress(msAddr, 10)
        mstxid = ro
        #print("mstxid ", mstxid)
        
        ro = nodes[0].gettransaction(mstxid)
        #print("gettransaction ", ro)
        hexfund = ro['hex']
        
        ro = nodes[0].decoderawtransaction(hexfund)
        #print("decoderawtransaction hexfund ", ro)
        
        fundscriptpubkey = ''
        fundoutid = -1
        for vout in ro['vout']:
            if not isclose(vout['value'], 10.0):
                continue
            
            fundoutid = vout['n']
            fundscriptpubkey = vout['scriptPubKey']['hex']
            
        
        assert(fundoutid >= 0), "fund output not found"
        #print("\nfundscriptpubkey", fundscriptpubkey)
        #print("fundoutid", fundoutid)
        
        
        
        addrTo = nodes[2].getnewaddress();
        #print("addrTo ", addrTo)
        
        inputs = [{ \
            "txid":mstxid,\
            "vout":fundoutid, \
            "scriptPubKey":fundscriptpubkey, \
            "redeemScript":redeemScript,
            "amount":10.0,
            }]
        
        outputs = {addrTo:2, msAddr:7.99}
        
        ro = nodes[0].createrawtransaction(inputs, outputs)
        hexRaw = ro
        #print("createrawtransaction ", hexRaw)
        
        ro = nodes[0].decoderawtransaction(hexRaw)
        #print("decoderawtransaction hexRaw ", ro)
        
        
        ro = nodes[0].dumpprivkey(addrs[0])
        #print("dumpprivkey ", ro)
        vk0 = ro
        
        signkeys = [vk0,]
        ro = nodes[0].signrawtransaction(hexRaw, inputs, signkeys)
        #print("signrawtransaction ", ro)
        hexRaw1 = ro['hex']
        
        
        vk1 = nodes[0].dumpprivkey(addrs[1])
        signkeys = [vk1,]
        ro = nodes[0].signrawtransaction(hexRaw1, inputs, signkeys)
        #print("signrawtransaction ", ro)
        hexRaw2 = ro['hex']
        
        """
        ro = nodes[0].decoderawtransaction(hexRaw2)
        print("decoderawtransaction ", ro)
        txspendid = ro['txid']
        """
        
        txnid_spendMultisig = nodes[0].sendrawtransaction(hexRaw2)
        #print("sendrawtransaction ", txnid_spendMultisig)
        
        
        # start staking
        ro = nodes[0].reservebalance(False)
        assert(self.wait_for_height(nodes[0], 1))
        block1_hash = nodes[0].getblockhash(1)
        ro = nodes[0].getblock(block1_hash)
        
        assert(txnid_spendMultisig in ro['tx'])
        
        #assert(False)
        #print(json.dumps(ro, indent=4))
        

if __name__ == '__main__':
    MultiSigTest().main()
