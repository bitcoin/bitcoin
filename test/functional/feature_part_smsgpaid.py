#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose, getIndexAtProperty
from test_framework.util import *
import binascii

class SmsgPaidTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [ ['-debug','-noacceptnonstdtxn'] for i in range(self.num_nodes) ]

    def setup_network(self, split=False):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()
        connect_nodes(self.nodes[0], 1)

        self.is_network_split = False
        self.sync_all()

    def run_test (self):
        tmpdir = self.options.tmpdir
        nodes = self.nodes

        # Stop staking
        for i in range(len(nodes)):
            nodes[i].reservebalance(True, 10000000)

        nodes[0].extkeyimportmaster(nodes[0].mnemonic('new')['master'])
        nodes[1].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')

        address0 = nodes[0].getnewaddress()  # Will be different each run
        address1 = nodes[1].getnewaddress()
        assert(address1 == 'pX9N6S76ZtA5BfsiJmqBbjaEgLMHpt58it')

        ro = nodes[0].smsglocalkeys()
        assert(len(ro['wallet_keys']) == 0)

        ro = nodes[0].smsgaddlocaladdress(address0)
        assert('Receiving messages enabled for address' in ro['result'])

        ro = nodes[0].smsglocalkeys()
        assert(len(ro['wallet_keys']) == 1)


        ro = nodes[1].smsgaddaddress(address0, ro['wallet_keys'][0]['public_key'])
        assert(ro['result'] == 'Public key added to db.')

        text_1 = "['data':'test','value':1]"
        ro = nodes[1].smsgsend(address1, address0, text_1, True, 4, True)
        assert(ro['result'] == 'Not Sent.')
        assert(isclose(ro['fee'], 0.00085800))


        ro = nodes[1].smsgsend(address1, address0, text_1, True, 4)
        assert(ro['result'] == 'Sent.')

        self.stakeBlocks(1, nStakeNode=1)
        self.waitForSmsgExchange(1, 1, 0)

        ro = nodes[0].smsginbox()
        assert(len(ro['messages']) == 1)
        assert(ro['messages'][0]['text'] == text_1)


        ro = nodes[0].smsgimportprivkey('7pHSJFY1tNwi6d68UttGzB8YnXq2wFWrBVoadLv4Y6ekJD3L1iKs', 'smsg test key')

        address0_1 = 'pasdoMwEn35xQUXFvsChWAQjuG8rEKJQW9'
        text_2 = "['data':'test','value':2]"
        ro = nodes[0].smsglocalkeys()
        assert(len(ro['smsg_keys']) == 1)
        assert(ro['smsg_keys'][0]['address'] == address0_1)

        ro = nodes[1].smsgaddaddress(address0_1, ro['smsg_keys'][0]['public_key'])
        assert(ro['result'] == 'Public key added to db.')

        ro = nodes[1].smsgsend(address1, address0_1, text_2, True, 4)
        assert(ro['result'] == 'Sent.')

        self.stakeBlocks(1, nStakeNode=1)
        self.waitForSmsgExchange(2, 1, 0)

        ro = nodes[0].smsginbox()
        assert(len(ro['messages']) == 1)
        assert(ro['messages'][0]['text'] == text_2)



        nodes[0].node_encrypt_wallet('qwerty234')
        self.start_node(0)

        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        ro = nodes[0].getwalletinfo()
        assert(ro['encryptionstatus'] == 'Locked')

        localkeys0 = nodes[0].smsglocalkeys()
        assert(len(localkeys0['smsg_keys']) == 1)
        assert(len(localkeys0['wallet_keys']) == 1)
        assert(localkeys0['smsg_keys'][0]['address'] == address0_1)
        assert(localkeys0['wallet_keys'][0]['address'] == address0)

        text_3 = "['data':'test','value':3]"
        ro = nodes[0].smsglocalkeys()
        assert(len(ro['smsg_keys']) == 1)
        assert(ro['smsg_keys'][0]['address'] == address0_1)

        ro = nodes[1].smsgsend(address1, address0, 'Non paid msg')
        assert(ro['result'] == 'Sent.')

        ro = nodes[1].smsgsend(address1, address0_1, text_3, True, 4)
        assert(ro['result'] == 'Sent.')
        assert(len(ro['txid']) == 64)

        self.sync_all()
        self.stakeBlocks(1, nStakeNode=1)
        self.waitForSmsgExchange(4, 1, 0)

        msgid = ro['msgid']
        for i in range(5):
            try:
                ro = nodes[1].smsg(msgid)
                assert(ro['location'] == 'outbox')
                break
            except Exception as e:
                time.sleep(1)
        assert(ro['text'] == text_3)
        assert(ro['addressfrom'] == address1)
        assert(ro['addressto'] == address0_1)

        ro = nodes[0].walletpassphrase("qwerty234", 300)
        ro = nodes[0].smsginbox()
        assert(len(ro['messages']) == 2)
        flat = json.dumps(ro, default=self.jsonDecimal)
        assert('Non paid msg' in flat)
        assert(text_3 in flat)

        ro = nodes[0].walletlock()

        ro = nodes[0].smsginbox("all")
        assert(len(ro['messages']) == 4)
        flat = json.dumps(ro, default=self.jsonDecimal)
        assert(flat.count('Wallet is locked') == 2)


        ro = nodes[0].smsg(msgid)
        assert(ro['read'] == True)

        ro = nodes[0].smsg(msgid, {'setread':False})
        assert(ro['read'] == False)

        ro = nodes[0].smsg(msgid, {'delete':True})
        assert(ro['operation'] == 'Deleted')

        try:
            ro = nodes[0].smsg(msgid)
            assert(False), 'Read deleted msg.'
        except:
            pass

        ro = nodes[0].smsggetpubkey(address0_1)
        assert(ro['publickey'] == 'h2UfzZxbhxQPcXDfYTBRGSC7GM77qrLjhtqcmfAnAia9')


        filepath = tmpdir+'/sendfile.txt'
        msg = b"msg in file\0after null sep"
        with open(filepath, 'wb', encoding=None) as fp:
            fp.write(msg)

        ro = nodes[1].smsgsend(address1, address0_1, filepath, True, 4, False, True)
        assert(ro['result'] == 'Sent.')
        msgid = ro['msgid']

        ro = nodes[1].smsgsend(address1, address0_1, binascii.hexlify(msg).decode("utf-8"), True, 4, False, False, True)
        msgid2 = ro['msgid']
        self.stakeBlocks(1, nStakeNode=1)

        for i in range(5):
            try:
                ro = nodes[1].smsg(msgid, {'encoding':'hex'})
                assert(ro['location'] == 'outbox')
                break
            except:
                time.sleep(1)
        assert(msg == bytes.fromhex(ro['hex'][:-2]))  # Extra null byte gets tacked on

        for i in range(5):
            try:
                ro = nodes[1].smsg(msgid2, {'encoding':'hex'})
                assert(ro['location'] == 'outbox')
                break
            except:
                time.sleep(1)
        assert(msg == bytes.fromhex(ro['hex'][:-2]))
        assert(ro['daysretention'] == 4)


        ro = nodes[0].smsgoptions('list', True)
        assert(len(ro['options']) == 3)
        assert(len(ro['options'][0]['description']) > 0)

        ro = nodes[0].smsgoptions('set', 'newAddressAnon', 'false')
        assert('newAddressAnon = false' in json.dumps(ro))


        addr = nodes[0].getnewaddress('smsg test')
        pubkey = nodes[0].getaddressinfo(addr)['pubkey']
        ro = nodes[1].smsgaddaddress(addr, pubkey)
        assert('Public key added to db' in json.dumps(ro))

        # Wait for sync
        i = 0
        for i in range(10):
            ro = nodes[0].smsginbox('all')
            if len(ro['messages']) >= 5:
                break
            time.sleep(1)
        assert(i < 10)


        # Test filtering
        ro = nodes[0].smsginbox('all', "'vAlue':2")
        assert(len(ro['messages']) == 1)

        ro = nodes[1].smsgoutbox('all', "'vAlue':2")
        assert(len(ro['messages']) == 1)


        # Test clear and rescan
        ro = nodes[0].smsginbox('clear')
        assert('Deleted 5 messages' in ro['result'])

        ro = nodes[0].walletpassphrase("qwerty234", 300)
        ro = nodes[0].smsgscanbuckets()
        assert('Scan Buckets Completed' in ro['result'])

        ro = nodes[0].smsginbox('all')
        # Recover 5 + 1 dropped msg
        assert(len(ro['messages']) == 6)


        # Test smsglocalkeys
        addr = nodes[0].getnewaddress()

        ro = nodes[0].smsglocalkeys('recv','+', addr)
        assert('Address not found' in ro['result'])
        ro = nodes[0].smsglocalkeys('anon', '+', addr)
        assert('Address not found' in ro['result'])

        ro = nodes[0].smsgaddlocaladdress(addr)
        assert('Receiving messages enabled for address' in ro['result'])

        ro = nodes[0].smsglocalkeys('recv',  '-',addr)
        assert('Receive off' in ro['key'])
        assert(addr in ro['key'])

        ro = nodes[0].smsglocalkeys('anon', '-',addr)
        assert('Anon off' in ro['key'])
        assert(addr in ro['key'])

        ro = nodes[0].smsglocalkeys('all')

        n = getIndexAtProperty(ro['wallet_keys'], 'address', addr)
        assert(ro['wallet_keys'][n]['receive'] == '0')
        assert(ro['wallet_keys'][n]['anon'] == '0')

        # Test smsgpurge
        ro = nodes[0].smsg(msgid, {'encoding':'hex'})
        assert(ro['msgid'] == msgid)

        nodes[0].smsgpurge(msgid)

        try:
            nodes[0].smsg(msgid, {'encoding':'hex'})
            assert(False), 'Purged message in inbox'
        except JSONRPCException as e:
            assert('Unknown message id' in e.error['message'])

        ro = nodes[0].smsgbuckets()
        assert(int(ro['total']['numpurged']) == 1)
        assert(int(ro['buckets'][0]['no. messages']) == int(ro['buckets'][0]['active messages']) + 1)







if __name__ == '__main__':
    SmsgPaidTest().main()
