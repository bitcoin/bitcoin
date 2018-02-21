#!/usr/bin/env python3
# Copyright (c) 2018 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose
from test_framework.util import *

class USBDeviceTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [ ['-debug','-noacceptnonstdtxn'] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

        connect_nodes_bi(self.nodes, 0, 1)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        nodes = self.nodes

        # Stop staking
        for i in range(len(nodes)):
            nodes[i].reservebalance(True, 10000000)

        nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(nodes[0].getwalletinfo()['total_balance'] == 100000)

        ro = nodes[1].listdevices()
        assert(len(ro) == 1)
        assert(ro[0]['vendor'] == 'Debug')
        assert(ro[0]['product'] == 'Device')

        ro = nodes[1].getdeviceinfo()
        assert(ro['device'] == 'debug')

        ro = nodes[1].getdevicepublickey('0')
        assert(ro['address'] == 'praish9BVxVdhykpqBYEs6L65AQ7iKd9z1')
        assert(ro['path'] == "m/44'/1'/0'/0")

        ro = nodes[1].getdevicepublickey('0/1')
        assert(ro['address'] == 'peWvjy33QptC2Gz3ww7jTTLPjC2QJmifBR')
        assert(ro['path'] == "m/44'/1'/0'/0/1")

        ro = nodes[1].getdevicexpub("m/44'/1'/0'", "")
        assert(ro == 'pparszKXPyRegWYwPacdPduNPNEryRbZDCAiSyo8oZYSsbTjc6FLP4TCPEX58kAeCB6YW9cSdR6fsbpeWDBTgjbkYjXCoD9CNoFVefbkg3exzpQE')

        message = 'This is just a test message'
        sig = nodes[1].devicesignmessage('0/1', message)
        assert(True == nodes[1].verifymessage('peWvjy33QptC2Gz3ww7jTTLPjC2QJmifBR', sig, message))

        ro = nodes[1].initaccountfromdevice('test_acc')
        assert(ro['extkey'] == 'pparszKXPyRegWYwPacdPduNPNEryRbZDCAiSyo8oZYSsbTjc6FLP4TCPEX58kAeCB6YW9cSdR6fsbpeWDBTgjbkYjXCoD9CNoFVefbkg3exzpQE')
        assert(ro['path'] == "m/44'/1'/0'")

        ro = nodes[1].extkey('list', 'true')
        assert(len(ro) == 1)
        assert(ro[0]['path'] == "m/44h/1h/0h")
        assert(ro[0]['epkey'] == 'pparszKXPyRegWYwPacdPduNPNEryRbZDCAiSyo8oZYSsbTjc6FLP4TCPEX58kAeCB6YW9cSdR6fsbpeWDBTgjbkYjXCoD9CNoFVefbkg3exzpQE')
        assert(ro[0]['label'] == 'test_acc')
        assert(ro[0]['hardware_device'] == '0xffff 0x0001')


        addr1_0 = nodes[1].getnewaddress('lbl1_0')
        ro = nodes[1].filteraddresses()
        assert(len(ro) == 1)
        assert(ro[0]['path'] == 'm/0/0')
        assert(ro[0]['owned'] == 'true')
        assert(ro[0]['label'] == 'lbl1_0')

        va_addr1_0 = nodes[1].validateaddress(addr1_0)
        assert(va_addr1_0['ismine'] == True)
        assert(va_addr1_0['iswatchonly'] == False)
        assert(va_addr1_0['isondevice'] == True)
        assert(va_addr1_0['path'] == 'm/0/0')

        try:
            nodes[1].getnewstealthaddress()
        except JSONRPCException as e:
            pass
        else:
            assert(False)

        txnid0 = nodes[0].sendtoaddress(addr1_0, 10);

        self.stakeBlocks(1)

        ro = nodes[1].getwalletinfo()
        assert(isclose(ro['balance'], 10.0))

        addr0_0 = nodes[0].getnewaddress()
        hexRaw = nodes[1].createrawtransaction([], {addr0_0:1})
        hexFunded = nodes[1].fundrawtransaction(hexRaw)['hex']
        txDecoded = nodes[1].decoderawtransaction(hexFunded)

        ro = nodes[1].devicesignrawtransaction(hexFunded)
        assert(ro['complete'] == True)

        txnid1 = nodes[1].sendrawtransaction(ro['hex'])

        self.sync_all()
        self.stakeBlocks(1)

        ro = nodes[1].devicesignrawtransaction(hexFunded)
        assert(ro['errors'][0]['error'] == 'Input not found or already spent')

        prevtxns = [{'txid':txDecoded['vin'][0]['txid'],'vout':txDecoded['vin'][0]['vout'],'scriptPubKey':va_addr1_0['scriptPubKey'],'amount':10},]
        ro = nodes[1].devicesignrawtransaction(hexFunded, prevtxns, ['0/0',])
        assert(ro['complete'] == True)

        ro = nodes[1].listunspent()
        assert(ro[0]['ondevice'] == True)


        txnid2 = nodes[1].sendtoaddress(addr0_0, 0.1)

        self.sync_all()

        ro = nodes[0].filtertransactions()
        assert(ro[0]['txid'] == txnid2)



        #assert(False)
        #print(json.dumps(ro, indent=4, default=self.jsonDecimal))


if __name__ == '__main__':
    USBDeviceTest().main()
