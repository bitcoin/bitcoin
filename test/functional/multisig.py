#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose
from test_framework.util import *

class MultiSigTest(ParticlTestFramework):

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

        # stop staking
        ro = nodes[0].reservebalance(True, 10000000)
        ro = nodes[1].reservebalance(True, 10000000)

        ro = nodes[0].extkeyimportmaster("abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb")
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')

        ro = nodes[0].getinfo()
        assert(ro['total_balance'] == 100000)

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

        v = [addrs[0],addrs[1],pubkeys[2]]
        ro = nodes[0].addmultisigaddress(2, v)
        msAddr = ro

        ro = nodes[0].validateaddress(msAddr);
        assert(ro['isscript'] == True)
        scriptPubKey = ro['scriptPubKey']
        redeemScript = ro['hex']


        ro = nodes[0].sendtoaddress(msAddr, 10)
        mstxid = ro

        ro = nodes[0].gettransaction(mstxid)
        hexfund = ro['hex']

        ro = nodes[0].decoderawtransaction(hexfund)

        fundscriptpubkey = ''
        fundoutid = -1
        for vout in ro['vout']:
            if not isclose(vout['value'], 10.0):
                continue

            fundoutid = vout['n']
            fundscriptpubkey = vout['scriptPubKey']['hex']
        assert(fundoutid >= 0), "fund output not found"


        addrTo = nodes[2].getnewaddress();

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

        ro = nodes[0].decoderawtransaction(hexRaw)

        ro = nodes[0].dumpprivkey(addrs[0])
        vk0 = ro

        signkeys = [vk0,]
        ro = nodes[0].signrawtransaction(hexRaw, inputs, signkeys)
        hexRaw1 = ro['hex']


        vk1 = nodes[0].dumpprivkey(addrs[1])
        signkeys = [vk1,]
        ro = nodes[0].signrawtransaction(hexRaw1, inputs, signkeys)
        hexRaw2 = ro['hex']

        txnid_spendMultisig = nodes[0].sendrawtransaction(hexRaw2)


        # start staking
        ro = nodes[0].walletsettings('stakelimit', {'height':1})
        ro = nodes[0].reservebalance(False)
        assert(self.wait_for_height(nodes[0], 1))
        block1_hash = nodes[0].getblockhash(1)
        ro = nodes[0].getblock(block1_hash)

        assert(txnid_spendMultisig in ro['tx'])

        #assert(False)
        #print(json.dumps(ro, indent=4, default=self.jsonDecimal))


if __name__ == '__main__':
    MultiSigTest().main()
