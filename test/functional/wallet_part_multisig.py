#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose
from test_framework.util import *


class MultiSigTest(ParticlTestFramework):
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

        # stop staking
        for i in range(len(nodes)):
            nodes[i].reservebalance(True, 10000000)

        ro = nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        assert(nodes[0].getwalletinfo()['total_balance'] == 100000)

        addrs = []
        pubkeys = []

        ro = nodes[0].getnewaddress()
        addrs.append(ro)
        ro = nodes[0].getaddressinfo(ro)
        pubkeys.append(ro['pubkey'])

        ro = nodes[0].getnewaddress()
        addrs.append(ro)
        ro = nodes[0].getaddressinfo(ro)
        pubkeys.append(ro['pubkey'])

        ro = nodes[1].extkeyimportmaster('drip fog service village program equip minute dentist series hawk crop sphere olympic lazy garbage segment fox library good alley steak jazz force inmate')

        ro = nodes[1].getnewaddress()
        addrs.append(ro)
        ro = nodes[1].getaddressinfo(ro)
        pubkeys.append(ro['pubkey'])


        ro = nodes[2].extkeyimportmaster(nodes[2].mnemonic('new', '', 'french')['mnemonic'])

        ro = nodes[2].getnewaddress()
        addrs.append(ro)
        ro = nodes[2].getaddressinfo(ro)
        pubkeys.append(ro['pubkey'])

        v = [addrs[0],addrs[1],pubkeys[2]]
        ro = nodes[0].addmultisigaddress(2, v)
        msAddr = ro['address']

        ro = nodes[0].getaddressinfo(msAddr)
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


        addrTo = nodes[2].getnewaddress()

        inputs = [{
            "txid":mstxid,
            "vout":fundoutid,
            "scriptPubKey":fundscriptpubkey,
            "redeemScript":redeemScript,
            "amount":10.0,
            }]

        outputs = {addrTo:2, msAddr:7.99}

        hexRaw = nodes[0].createrawtransaction(inputs, outputs)

        vk0 = nodes[0].dumpprivkey(addrs[0])
        signkeys = [vk0,]
        ro = nodes[0].signrawtransactionwithkey(hexRaw, signkeys, inputs)
        hexRaw1 = ro['hex']

        vk1 = nodes[0].dumpprivkey(addrs[1])
        signkeys = [vk1,]
        ro = nodes[0].signrawtransactionwithkey(hexRaw1, signkeys, inputs)
        hexRaw2 = ro['hex']

        txnid_spendMultisig = nodes[0].sendrawtransaction(hexRaw2)


        self.stakeBlocks(1)
        block1_hash = nodes[0].getblockhash(1)
        ro = nodes[0].getblock(block1_hash)
        assert(txnid_spendMultisig in ro['tx'])


        msAddr256 = nodes[0].addmultisigaddress(2, v, "", False, True)
        ro = nodes[0].getaddressinfo(msAddr256)
        assert(ro['isscript'] == True)

        ro = nodes[0].addmultisigaddress(2, v, "", True, True)
        msAddr256 = ro
        assert(msAddr256 == "tpj1vtll9wnsd7dxzygrjp2j5jr5tgrjsjmj3vwjf7vf60f9p50g5ddqmasmut")

        ro = nodes[0].getaddressinfo(msAddr256)
        assert(ro['isscript'] == True)
        scriptPubKey = ro['scriptPubKey']
        redeemScript = ro['hex']

        mstxid2 = nodes[0].sendtoaddress(msAddr256, 9)

        ro = nodes[0].gettransaction(mstxid2)
        hexfund = ro['hex']
        ro = nodes[0].decoderawtransaction(hexfund)

        fundscriptpubkey = ''
        fundoutid = -1
        for vout in ro['vout']:
            if not isclose(vout['value'], 9.0):
                continue
            fundoutid = vout['n']
            fundscriptpubkey = vout['scriptPubKey']['hex']
            assert('OP_SHA256' in vout['scriptPubKey']['asm'])
        assert(fundoutid >= 0), "fund output not found"


        inputs = [{
            "txid":mstxid2,
            "vout":fundoutid,
            "scriptPubKey":fundscriptpubkey,
            "redeemScript":redeemScript,
            "amount":9.0, # Must specify amount
            }]

        addrTo = nodes[2].getnewaddress()
        outputs = {addrTo:2, msAddr256:6.99}

        hexRaw = nodes[0].createrawtransaction(inputs, outputs)

        vk0 = nodes[0].dumpprivkey(addrs[0])
        signkeys = [vk0,]
        ro = nodes[0].signrawtransactionwithkey(hexRaw, signkeys, inputs)
        hexRaw1 = ro['hex']

        ro = nodes[0].decoderawtransaction(hexRaw1)


        vk1 = nodes[0].dumpprivkey(addrs[1])
        signkeys = [vk1,]
        ro = nodes[0].signrawtransactionwithkey(hexRaw1, signkeys, inputs)
        hexRaw2 = ro['hex']

        txnid_spendMultisig2 = nodes[0].sendrawtransaction(hexRaw2)

        self.stakeBlocks(1)
        block2_hash = nodes[0].getblockhash(2)
        ro = nodes[0].getblock(block2_hash)
        assert(txnid_spendMultisig2 in ro['tx'])

        ro = nodes[0].getaddressinfo(msAddr)
        scriptPubKey = ro['scriptPubKey']
        redeemScript = ro['hex']

        opts = {"recipe":"abslocktime","time":946684800,"addr":msAddr}
        ro = nodes[0].buildscript(opts)
        scriptTo = ro['hex']

        outputs = [{'address':'script', 'amount':8, 'script':scriptTo},]
        mstxid3 = nodes[0].sendtypeto('part', 'part', outputs)

        ro = nodes[0].gettransaction(mstxid3)
        hexfund = ro['hex']
        ro = nodes[0].decoderawtransaction(hexfund)

        fundscriptpubkey = ''
        fundoutid = -1
        for vout in ro['vout']:
            if not isclose(vout['value'], 8.0):
                continue
            fundoutid = vout['n']
            fundscriptpubkey = vout['scriptPubKey']['hex']
            assert('OP_CHECKLOCKTIMEVERIFY' in vout['scriptPubKey']['asm'])
        assert(fundoutid >= 0), "fund output not found"


        inputs = [{
            "txid":mstxid3,
            "vout":fundoutid,
            "scriptPubKey":fundscriptpubkey,
            "redeemScript":redeemScript,
            "amount":8.0, # Must specify amount
            }]

        addrTo = nodes[2].getnewaddress()
        outputs = {addrTo:2, msAddr:5.99}
        locktime = 946684801

        hexRaw = nodes[0].createrawtransaction(inputs, outputs, locktime)

        vk0 = nodes[0].dumpprivkey(addrs[0])
        signkeys = [vk0,]
        ro = nodes[0].signrawtransactionwithkey(hexRaw, signkeys, inputs)
        hexRaw1 = ro['hex']

        ro = nodes[0].decoderawtransaction(hexRaw1)

        vk1 = nodes[0].dumpprivkey(addrs[1])
        signkeys = [vk1,]
        ro = nodes[0].signrawtransactionwithkey(hexRaw1, signkeys, inputs)
        hexRaw2 = ro['hex']

        txnid_spendMultisig3 = nodes[0].sendrawtransaction(hexRaw2)

        self.stakeBlocks(1)
        block3_hash = nodes[0].getblockhash(3)
        ro = nodes[0].getblock(block3_hash)
        assert(txnid_spendMultisig3 in ro['tx'])


        # Coldstake script

        stakeAddr = nodes[0].getnewaddress()
        addrTo = nodes[0].getnewaddress()

        opts = {"recipe":"ifcoinstake","addrstake":stakeAddr,"addrspend":msAddr}
        ro = nodes[0].buildscript(opts)
        scriptTo = ro['hex']

        outputs = [{ 'address':'script', 'amount':1, 'script':scriptTo }]
        txFundId = nodes[0].sendtypeto('part', 'part', outputs)
        hexfund = nodes[0].gettransaction(txFundId)['hex']
        ro = nodes[0].decoderawtransaction(hexfund)
        for vout in ro['vout']:
            if not isclose(vout['value'], 1.0):
                continue
            fundoutn = vout['n']
            fundscriptpubkey = vout['scriptPubKey']['hex']
            assert('OP_ISCOINSTAKE' in vout['scriptPubKey']['asm'])
        assert(fundoutn >= 0), "fund output not found"

        ro = nodes[0].getaddressinfo(msAddr)
        assert(ro['isscript'] == True)
        scriptPubKey = ro['scriptPubKey']
        redeemScript = ro['hex']

        inputs = [{
            "txid":txFundId,
            "vout":fundoutn,
            "scriptPubKey":fundscriptpubkey,
            "redeemScript":redeemScript,
            "amount":1.0,
            }]

        outputs = {addrTo:0.99}
        hexRaw = nodes[0].createrawtransaction(inputs, outputs)

        sig0 = nodes[0].createsignaturewithwallet(hexRaw, inputs[0], addrs[0])
        sig1 = nodes[0].createsignaturewithwallet(hexRaw, inputs[0], addrs[1])

        witnessStack = [
            "",
            sig0,
            sig1,
            redeemScript,
        ]
        hexRawSigned = nodes[0].tx([hexRaw,'witness=0:'+ ':'.join(witnessStack)])

        ro = nodes[0].verifyrawtransaction(hexRawSigned)
        assert(ro['complete'] == True)

        ro = nodes[0].signrawtransactionwithwallet(hexRaw)
        assert(ro['complete'] == True)
        assert(ro['hex'] == hexRawSigned)

        txid = nodes[0].sendrawtransaction(hexRawSigned)

        self.stakeBlocks(1)
        block4_hash = nodes[0].getblockhash(4)
        ro = nodes[0].getblock(block4_hash)
        assert(txid in ro['tx'])


if __name__ == '__main__':
    MultiSigTest().main()
