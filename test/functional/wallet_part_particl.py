#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.util import *

def read_dump(file_name):
    nLines = 0
    sJson = ''
    isJson = False
    with open(file_name, encoding='utf8') as inputfile:
        for line in inputfile:
            # only read non comment lines
            if line.startswith('# --- Begin JSON ---'):
                isJson = True
            elif line.startswith('# --- End JSON ---'):
                isJson = False
            if line[0] == "#":
                continue

            if isJson:
                sJson += line
            elif len(line) > 10:
                nLines+=1

    return sJson, nLines


class WalletParticlTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [ ['-debug',] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

    def run_test (self):
        tmpdir = self.options.tmpdir
        nodes = self.nodes

        # Wallet must initially contain no keys at all
        nodes[0].dumpwallet(tmpdir + '/node0/wallet.unencrypted.dump')
        sJson, nLines = read_dump(tmpdir + '/node0/wallet.unencrypted.dump')
        assert(nLines == 0)
        o = json.loads(sJson)
        assert(len(o['loose_extkeys']) == 0)
        assert(len(o['accounts']) == 0)

        # Try get a new address without a master key:
        try:
            addr = nodes[0].getnewaddress()
            raise AssertionError('Generated address from empty wallet.')
        except JSONRPCException as e:
            assert('Wallet has no active master key' in e.error['message'])

        ro = nodes[0].extkey()
        assert('No keys to list' in ro['result'])


        oRoot0 = nodes[0].mnemonic('new')

        ro = nodes[0].extkey('info', oRoot0['master'])
        root0_id = ro['key_info']['id']

        ro = nodes[0].extkey('info', oRoot0['master'], 'm/44h/1h')
        expectedMasterKey0 = ro['key_info']['result']


        # 'extkey import <key> [label] [bip44] [save_bip44_key]\n'
        ro = nodes[0].extkey('import', oRoot0['master'], 'import save key', 'true')

        # 'extkey list [show_secrets] - default\n'
        ro = nodes[0].extkey('list')
        assert(len(ro) == 1)
        assert(ro[0]['path'] == 'm/44h/1h')
        assert(not 'evkey' in ro[0])

        # 'extkey list [show_secrets] - default\n'
        ro = nodes[0].extkey('list', 'true')
        assert(ro[0]['evkey'] == expectedMasterKey0)
        assert(ro[0]['root_key_id'] == root0_id)
        master0_id = ro[0]['id']

        ro = nodes[0].extkey('setMaster', master0_id)
        assert(ro['result'] == 'Success.')

        ro = nodes[0].extkey('deriveAccount', '1st account')
        # NOTE: must pass blank path for num_derives_hardened to update on the master key

        assert(ro['result'] == 'Success.')
        account0_id = ro['account']

        ro = nodes[0].extkey('key', master0_id)
        assert(ro['num_derives'] == '0')
        assert(ro['num_derives_hardened'] == '1')

        ro = nodes[0].extkey('setDefaultAccount', account0_id)
        assert(ro['result'] == 'Success.')

        address0 = nodes[0].getnewaddress()

        ro = nodes[0].extkey('account', account0_id)

        fFound = False
        for c in ro['chains']:
            if c['function'] != 'active_external' or c['num_derives'] != '1':
                continue
            assert(c['use_type'] == 'external')
            fFound = True
            break
        assert(fFound)



        ro = nodes[0].extkey('deriveAccount', '2nd account')
        assert(ro['result'] == 'Success.')
        assert(account0_id != ro['account'])

        ro = nodes[0].extkey('key', master0_id)
        assert(ro['num_derives'] == '0')
        assert(ro['num_derives_hardened'] == '2')

        # re-derive 1st account
        ro = nodes[0].extkey('deriveAccount', '1st account', "0'")
        assert(ro['result'] == 'Failed.')
        assert(ro['reason'] == 'Account already exists in db.')


        # make sure info hasn't been forgotten/overwritten
        ro = nodes[0].extkey('account', account0_id)
        fFound = False
        for c in ro['chains']:
            if c['function'] != 'active_external' or c['num_derives'] != '1':
                continue
            fFound = True
            break
        assert(fFound)


        ro = nodes[0].extkey('deriveAccount', 'Should fail', 'abcd')
        assert(ro['result'] == 'Failed.')

        try:
            ro = nodes[0].extkey('deriveAccount', 'Should fail', "0'", 'abcd')
            raise AssertionError('Should have failed.')
        except JSONRPCException as e:
            assert('Unknown parameter' in e.error['message'])


        ro = nodes[0].getwalletinfo()
        assert(ro['encryptionstatus'] == 'Unencrypted')

        nodes[0].node_encrypt_wallet('qwerty123')
        self.start_node(0)


        ro = nodes[0].getwalletinfo()
        assert(ro['encryptionstatus'] == 'Locked')

        try:
            ro = nodes[0].extkey('list')
            raise AssertionError('Should have failed.')
        except JSONRPCException as e:
            assert('Wallet locked' in e.error['message'])

        ro = nodes[0].walletpassphrase('qwerty123', 300)

        ro = nodes[0].extkey('list')
        assert(len(ro) == 3) # 1 loose key (master) + 2 accounts

        ro = nodes[0].extkey('key', master0_id)
        assert(ro['key_type'] == 'Master')
        assert(ro['num_derives'] == '0')
        assert(ro['num_derives_hardened'] == '2')
        assert(ro['encrypted'] == 'true')
        assert(ro['id'] == master0_id)
        assert(ro['root_key_id'] == root0_id)
        assert(ro['current_master'] == 'true')


        ro = nodes[0].extkey('account', account0_id)
        fFound = False
        for c in ro['chains']:
            if c['function'] != 'active_external' or c['num_derives'] != '1':
                continue
            fFound = True
            break
        assert(fFound)

        address1 = nodes[0].getnewaddress()
        ro = nodes[0].extkey('account', account0_id)
        fFound = False
        for c in ro['chains']:
            if c['function'] != 'active_external' or c['num_derives'] != '2':
                continue
            fFound = True
            break
        assert(fFound)


        # test encrypting empty wallet
        nodes[1].dumpwallet(tmpdir + '/node1/wallet.unencrypted.dump')
        sJson, nLines = read_dump(tmpdir + '/node1/wallet.unencrypted.dump')
        assert(nLines == 0)
        o = json.loads(sJson)
        assert(len(o['loose_extkeys']) == 0)
        assert(len(o['accounts']) == 0)

        nodes[1].node_encrypt_wallet('qwerty234')
        self.start_node(1)

        try:
            ro = nodes[1].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        except JSONRPCException as e:
            assert('Wallet locked' in e.error['message'])

        try:
            ro = nodes[1].walletpassphrase('qwerty123', 300)
        except JSONRPCException as e:
            assert('passphrase entered was incorrect' in e.error['message'])
        assert(nodes[1].getwalletinfo()['encryptionstatus'] == 'Locked')

        nodes[1].walletpassphrase('qwerty234', 300)
        ro = nodes[1].getwalletinfo()
        assert(ro['encryptionstatus'] == 'Unlocked')
        assert(ro['unlocked_until'] > int(time.time()) and ro['unlocked_until'] < int(time.time()) + 500)

        ro = nodes[1].mnemonic('decode', '', 'abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb', 'false')
        decodedRoot = ro['master']

        roImport1 = nodes[1].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(roImport1['master_id'] == 'xHRmcdjD2kssaM5ZY8Cyzj8XWJsBweydyP')
        assert(roImport1['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')

        ro = nodes[1].extkey('list', 'true')
        assert(len(ro) == 2)
        assert(ro[0]['evkey'] == 'xparFhz8oNupLZBVCLZCDpXnaLf2H9uWxvZEmPQm2Hdsv5YZympmJZYXjbuvE1rRK4o8TMsbbpCWrbQbNvt7CZCeDULrgeQMi536vTuxvuXpWqN')
        assert(ro[0]['current_master'] == 'true')
        assert(ro[0]['root_key_id'] == 'xDALBKsFzxtkDPm6yoatAgNfRwavZTEheC')
        assert(ro[0]['encrypted'] == 'true')
        assert(ro[0]['path'] == 'm/44h/1h')
        assert(ro[0]['num_derives_hardened'] == '1')
        assert(ro[0]['num_derives'] == '0')
        assert(ro[0]['id'] == roImport1['master_id'])

        assert(ro[1]['type'] == 'Account')
        assert(ro[1]['id'] == roImport1['account_id'])
        assert(ro[1]['evkey'] == 'xparFkF1G6Dd5EW9vm2QFXMwseHFDx2hkXqqPExy6E1Pmf3XX1AyBy97PjVq2SAENoG8uVtmX1S9Hm6BE4aEPNxydc8cb3MsRPrcw4rq7V3dGRM')
        assert(ro[1]['root_key_id'] == roImport1['master_id'])
        assert(ro[1]['internal_chain'] == 'pparszMzzW1247AwkMQ1ou7KnV2o9HDHQJdycBVVAkkWeKHVwKwAqcTNVT2jpsxeXfsgvfHJYhyUuiT93r8iqmLTd52nzFeRfTggtVGuHPAAsb7f')
        assert(ro[1]['external_chain'] == 'pparszMzzW1247AwkKCH1MqneucXJfDoR3M5KoLsJZJpHkcjayf1xUMwPoTcTfUoQ32ahnkHhjvD2vNiHN5dHL6zmx8vR799JxgCw95APdkwuGm1')

        epkeytest = ro[1]['epkey']
        assert(epkeytest == 'pparszL1J9kjPoN8QqWamgQAGkzipU34hMjs3UJRTBz6DTs2Nq7kqgh3MvM1N6Fe11oPwnFjf5K2tR2CYTb93BXTfAZkeovjNR7A28Dxzik58kPH')

        ro = nodes[1].extkey('xparFkF1G6Dd5EW9vm2QFXMwseHFDx2hkXqqPExy6E1Pmf3XX1AyBy97PjVq2SAENoG8uVtmX1S9Hm6BE4aEPNxydc8cb3MsRPrcw4rq7V3dGRM')
        assert(ro['key_info']['ext_public_key'] == epkeytest)

        address1 = nodes[1].getnewaddress()

        try:
            ro = nodes[1].extkeyimportmaster(decodedRoot)
            assert(False), 'Imported same root key twice.'
        except JSONRPCException as e:
            assert('ExtKeyImportLoose failed, Derived key already exists in wallet' in e.error['message'])

        try:
            ro = nodes[1].walletpassphrasechange('fail', 'changedPass')
            assert(False), 'Changed password with incorrect old password.'
        except JSONRPCException as e:
            assert('passphrase entered was incorrect' in e.error['message'])


        ro = nodes[1].walletpassphrasechange('qwerty234', 'changedPass')

        # Restart node
        self.stop_node(1)
        self.start_node(1, self.extra_args[1])

        ro = nodes[1].walletpassphrase('changedPass', 300)

        ro = nodes[1].extkey('list', 'true')
        assert(len(ro) == 2)
        assert(ro[0]['id'] == roImport1['master_id'])
        assert(ro[1]['id'] == roImport1['account_id'])

        address1 = nodes[1].getnewaddress()


        """
        https://iancoleman.github.io/bip39/#english
        pact mammal barrel matrix local final lecture chunk wasp survey bid various book strong spread fall ozone daring like topple door fatigue limb olympic

        Bitcoin Testnet
        Purpose: 44
        Coin: 1 (Particl testnet)
        Account: 0
        Ext/Internal: 0

        path key0: m/44'/1'/0'/0/0
        """
        testPubkeys = [
            '02250e74b8ef97412bc9e5664df28ec0f67dd2952c75b055a87f030b96edeb939b',
            '025aed9dc12d41ecd3977245c90744df87c625fe00aeb5957972f50d11e149c6c4',
            '02c889478e0eb53dc819bb744bd81d59dfe83e3705a7fba7d14da2a5a7bf104681',
            '02fe68bafd05bb9d4ee22e50f73b2c9ceb80b4ed98c026d6de93f445fab14a1d12',
            '0334ba8f8e3ee869c61d20d4d2c3d4424b73d4f81fc98a469c35abb9088f23ed79']


        roImport2 = nodes[2].extkeyimportmaster('pact mammal barrel matrix local final lecture chunk wasp survey bid various book strong spread fall ozone daring like topple door fatigue limb olympic', '', 'true')
        assert(roImport2['master_id'] == 'xML8N7vPoKoSYFCymj3UreYoGWty55YfA6')
        assert(roImport2['account_id'] == 'aigXuhTg7sDkGhKhQhnP8aWcymma6ENGZE')

        ro = nodes[2].extkey('list', 'true')
        assert(len(ro) == 3)

        assert(ro[0]['key_type'] == 'BIP44 Root Key')
        assert(ro[0]['id'] == 'x9tEdnk3HNVN5ihfHpxicdatAXJCpDZdNu')


        assert(ro[1]['key_type'] == 'Master')
        assert(ro[1]['id'] == 'xML8N7vPoKoSYFCymj3UreYoGWty55YfA6')
        assert(ro[1]['path'] == 'm/44h/1h')
        assert(ro[1]['root_key_id'] == 'x9tEdnk3HNVN5ihfHpxicdatAXJCpDZdNu')

        assert(ro[2]['path'] == 'm/0h')
        assert(ro[2]['id'] == 'aigXuhTg7sDkGhKhQhnP8aWcymma6ENGZE')

        # Change account label
        assert(nodes[2].extkey('account')['label'] == 'Default Account')
        nodes[2].extkey('options', 'aigXuhTg7sDkGhKhQhnP8aWcymma6ENGZE', 'label', 'New label')
        assert(nodes[2].extkey('account')['label'] == 'New label')


        for i in range(0, 5):
            addr = nodes[2].getnewaddress()
            ro = nodes[2].getaddressinfo(addr)
            assert(ro['pubkey'] == testPubkeys[i])


        ro = nodes[2].getnewstealthaddress('testLabel')
        assert(ro == 'TetbX4FkzdAFp1jSgo8jBr7DbRbA8fevy2Xhobhzr8iH171UPjYjmxBKdztpTPspccEKiAyK4u5vXcAtSFWbYb98RTGUxTVjF5qAZt')


        ro = nodes[2].liststealthaddresses()
        assert(len(ro) == 1)
        assert(len(ro[0]['Stealth Addresses']) == 1)
        assert(ro[0]['Stealth Addresses'][0]['Label'] == 'testLabel')
        assert(ro[0]['Stealth Addresses'][0]['Address'] == 'TetbX4FkzdAFp1jSgo8jBr7DbRbA8fevy2Xhobhzr8iH171UPjYjmxBKdztpTPspccEKiAyK4u5vXcAtSFWbYb98RTGUxTVjF5qAZt')


        ro = nodes[2].importstealthaddress('0189cd6197d570740e88974ebeb8bd48484c1ca2361159cccc6c36ca99372e5b', 'b9e7fa5f7d6fdd3ea4b1c9bc339292e2af6dc183c7ab2734ad5bf41485e87efe', 'testImported')
        assert('Success' in ro['result'])

        ro = nodes[2].liststealthaddresses('true')
        assert(len(ro) == 2)
        assert(len(ro[1]['Stealth Addresses']) == 1)
        assert(ro[1]['Stealth Addresses'][0]['Label'] == 'testImported')

        assert(ro[1]['Stealth Addresses'][0]['Scan Secret'] == '7oizjaJXPSuaXRsBKP3NXyazwp6LDE5NHLuw9qT2uM7kgGHCWJ5z')
        assert(ro[1]['Stealth Addresses'][0]['Spend Secret'] == '7uuPBdNM8GiUT16uV2BGMGTSmJ8inJCbVGMcuToHH1fWPZ8u3apk')

        ro = nodes[2].getnewstealthaddress('testLabel2')
        assert(ro == 'TetbJKQcmWW74aRDDfNnALb7jaAw8KQ2ubWYsSprpqSQvbSTJZMaednJWN4EKGGaBFBzkaL1iQMtiAvKKZMj7rVusYWL76iRwx87cy')

        ro = nodes[2].liststealthaddresses('true')
        assert(len(ro) == 2)
        assert(len(ro[0]['Stealth Addresses']) == 2)

        ro = nodes[2].getnewextaddress('lblExtTest')
        # Must start with ppar (show public address only)
        assert(ro == 'pparszNetDqyrvZksLHJkwJGwJ1r9JCcEyLeHatLjerxRuD3qhdTdrdo2mE6e1ewfd25EtiwzsECooU5YwhAzRN63iFid6v5AQn9N5oE9wfBYehn')

        ro = nodes[2].extkey('pparszNetDqyrvZksLHJkwJGwJ1r9JCcEyLeHatLjerxRuD3qhdTdrdo2mE6e1ewfd25EtiwzsECooU5YwhAzRN63iFid6v5AQn9N5oE9wfBYehn', 'info', 'true')

        # extkey key should return the same object if fed keyid/pk
        ro = nodes[2].extkey('key', 'xQj7nV8vdN43rNCh5vJ8UG5ZFcLGDga9js', 'true')
        assert(ro['evkey'] == 'xparFntbLBU6CS8cRXkPWRUcQfQa47aFN8d5VptFZ6scD14zPWsmMutnEcb6wonnk86zxAn6PumC64JnSf6k51kYjtEWEvfngDdgZCd9ES4rLaU')
        ro2 = nodes[2].extkey('key', 'pparszNetDqyrvZksLHJkwJGwJ1r9JCcEyLeHatLjerxRuD3qhdTdrdo2mE6e1ewfd25EtiwzsECooU5YwhAzRN63iFid6v5AQn9N5oE9wfBYehn', 'true')
        assert(ro == ro2)

        ro = nodes[2].getaddressinfo('pparszNetDqyrvZksLHJkwJGwJ1r9JCcEyLeHatLjerxRuD3qhdTdrdo2mE6e1ewfd25EtiwzsECooU5YwhAzRN63iFid6v5AQn9N5oE9wfBYehn')
        assert(ro['ismine'] == True)

        ro = nodes[2].getwalletinfo()
        assert(ro['total_balance'] == 0)

        ro = nodes[2].rescanblockchain()
        assert(ro['start_height'] == 0)
        assert(nodes[2].getwalletinfo()['total_balance'] == 25000)


        # Test extkeyaltversion
        epkPart = 'pparszNetDqyrvZksLHJkwJGwJ1r9JCcEyLeHatLjerxRuD3qhdTdrdo2mE6e1ewfd25EtiwzsECooU5YwhAzRN63iFid6v5AQn9N5oE9wfBYehn'
        epkBtc = nodes[2].extkeyaltversion(epkPart)
        epkPartOut = nodes[2].extkeyaltversion(epkBtc)
        assert(epkPartOut == epkPart)

        evkPart = 'xparFntbLBU6CS8cRXkPWRUcQfQa47aFN8d5VptFZ6scD14zPWsmMutnEcb6wonnk86zxAn6PumC64JnSf6k51kYjtEWEvfngDdgZCd9ES4rLaU'
        evkBtc = nodes[2].extkeyaltversion(evkPart)
        evkPartOut = nodes[2].extkeyaltversion(evkBtc)
        assert(evkPartOut == evkPart)


        nodes[1].walletpassphrase('changedPass', 100, True)
        ro = nodes[1].getwalletinfo()
        assert(ro['encryptionstatus'] == 'Unlocked, staking only')
        assert(ro['unlocked_until'] != 0)

        try:
            nodes[1].sendtoaddress(address1, 0.01)
            assert(False), 'Should be unlocked for staking only.'
        except JSONRPCException as e:
            assert('Wallet is unlocked for staking only' in e.error['message'])

        try:
            nodes[1].walletpassphrase('wrongPass', 0, True)
            assert(False), 'Should notify wrong password used.'
        except JSONRPCException as e:
            assert('The wallet passphrase entered was incorrect' in e.error['message'])

        nodes[1].walletpassphrase('changedPass', 0, True)
        ro = nodes[1].getwalletinfo()
        assert(ro['encryptionstatus'] == 'Unlocked, staking only')
        assert(ro['unlocked_until'] == 0)


        ro = nodes[0].filteraddresses(0, 100, '0', '', '2')
        assert(len(ro) == 0)


        # Test manageaddressbook
        try:
            ro = nodes[0].manageaddressbook('edit', 'piNdRiuL2BqUA8hh2A6AtEbBkKqKxK13LT', 'lblEdited')
            assert(False), 'Edited non existing address.'
        except JSONRPCException as e:
            assert('is not in the address book' in e.error['message'])

        try:
            ro = nodes[0].manageaddressbook('del', 'piNdRiuL2BqUA8hh2A6AtEbBkKqKxK13LT', 'lblEdited')
            assert(False), 'Deleted non existing address.'
        except JSONRPCException as e:
            assert('is not in the address book' in e.error['message'])


        ro = nodes[0].manageaddressbook('add', 'piNdRiuL2BqUA8hh2A6AtEbBkKqKxK13LT', 'lblTest', 'lblPurposeTest')
        assert(ro['result'] == 'success')

        #filteraddresses [offset] [count] [sort_code] [match_str] [match_owned] [show_path]
        ro = nodes[0].filteraddresses(0, 100, '0', '', '2')
        assert(len(ro) == 1)
        assert(ro[0]['address'] == 'piNdRiuL2BqUA8hh2A6AtEbBkKqKxK13LT')
        assert(ro[0]['label'] == 'lblTest')


        ro = nodes[0].manageaddressbook('edit', 'piNdRiuL2BqUA8hh2A6AtEbBkKqKxK13LT', 'lblEdited')

        ro = nodes[0].filteraddresses(0, 100, '0', '', '2')
        assert(len(ro) == 1)
        assert(ro[0]['address'] == 'piNdRiuL2BqUA8hh2A6AtEbBkKqKxK13LT')
        assert(ro[0]['label'] == 'lblEdited')

        ro = nodes[0].filteraddresses(0, 100)
        nOrigLen = len(ro)

        sTestAddress = ''
        for r in ro:
            try:
                if r['path'] == 'm/0/0':
                    sTestAddress = r['address']
                    break
            except:
                pass

        assert(len(sTestAddress) > 0), 'Could not find 1st owned address.'

        ro = nodes[0].manageaddressbook('edit', sTestAddress, 'lblEditedOwned')
        assert(ro['result'] == 'success')

        ro = nodes[0].filteraddresses(0, 100)
        assert(nOrigLen == len(ro))

        fPass = False
        for r in ro:
            if r['address'] == sTestAddress:
                assert(r['path'] == 'm/0/0')
                assert(r['label'] == 'lblEditedOwned')
                fPass = True
        assert(fPass)

        ro = nodes[0].manageaddressbook('del', 'piNdRiuL2BqUA8hh2A6AtEbBkKqKxK13LT')

        ro = nodes[0].filteraddresses(0, 100)
        assert(nOrigLen-1 == len(ro))

        # Restart node
        self.stop_node(0)
        self.start_node(0, self.extra_args[0])

        ro = nodes[0].filteraddresses(0, 100)
        assert(nOrigLen-1 == len(ro))


        fPass = False
        for r in ro:
            if r['address'] == sTestAddress:
                assert(r['path'] == 'm/0/0')
                assert(r['label'] == 'lblEditedOwned')
                fPass = True
        assert(fPass)

        ro = nodes[0].manageaddressbook('info', sTestAddress)


        ro = nodes[1].walletlock()
        time.sleep(1)
        ro = nodes[1].getwalletinfo()
        assert(ro['encryptionstatus'] == 'Locked')

        ro = nodes[1].walletpassphrase('changedPass', 2)
        ro = nodes[1].getwalletinfo()
        assert(ro['encryptionstatus'] == 'Unlocked')

        time.sleep(4)
        ro = nodes[1].getwalletinfo()
        assert(ro['encryptionstatus'] == 'Locked')

        ro = nodes[1].walletpassphrase('changedPass', 2)


        nodes[0].walletpassphrase('qwerty123', 3000)

        ro = nodes[0].extkey('account')
        sExternalChainId = ''
        for chain in ro['chains']:
            if chain['function'] == 'active_external':
                sExternalChainId = chain['id']
                nHardened = chain['num_derives_h']
                break
        assert(len(sExternalChainId) > 0)
        assert(nHardened == '0')


        ro = nodes[0].deriverangekeys(0, 0, sExternalChainId, 'true')
        sCheckAddr = ro[0]

        ro = nodes[0].getnewaddress('h1','false','true')
        sHardenedAddr = ro
        assert(sHardenedAddr == sCheckAddr)

        ro = nodes[0].extkey('account')

        nHardened = 0
        for chain in ro['chains']:
            if chain['function'] == 'active_external':
                nHardened = chain['num_derives_h']
                break
        assert(nHardened == '1')


        ro = nodes[0].filteraddresses()

        sPath = ''
        for entry in ro:
            if entry['address'] == sHardenedAddr:
                sPath = entry['path']
                break
        assert(sPath == "m/0/0'")

        addr = nodes[0].getnewaddress('', 'false', 'false', 'true')
        ro = nodes[0].validateaddress(addr)
        assert(ro['isvalid'] == True)
        ro = nodes[0].getaddressinfo(addr)
        assert(ro['ismine'] == True)

        addr = nodes[0].getnewaddress('', 'true', 'false', 'true')
        ro = nodes[0].validateaddress(addr)
        assert(ro['isvalid'] == True)
        ro = nodes[0].getaddressinfo(addr)
        assert(ro['address'].startswith('tpl1'))
        assert(ro['ismine'] == True)

        #sAddrStake = sHardenedAddr
        sAddrStake = 'pomrQeo26xVLV5pnuDYkTUYuABFuP13HHE'
        sAddrSpend = 'tpl1vj4wplpq9ct7zmms3tvpr5dah84txlffz9sp5s5w4c7dhh6hvqus29mjpy'
        jsonInput = {'recipe':'ifcoinstake', 'addrstake':sAddrStake, 'addrspend':sAddrSpend}

        ro = nodes[0].buildscript(jsonInput)
        scriptHex = ro['hex']
        assert(scriptHex == 'b86376a914cf3837ef2e493d5b485c7f4536f27415c5cd3b6088ac6776a82064aae0fc202e17e16f708ad811d1bdb9eab37d2911601a428eae3cdbdf57603988ac68')


        coincontrol = {'changeaddress':scriptHex,'debug':True}
        outputs = [{'address':sAddrSpend, 'amount':1, 'narr':'not change'},]
        ro = nodes[2].sendtypeto('part', 'part', outputs, 'comment', 'comment-to', 4, 32, True, coincontrol)

        ro = nodes[2].decoderawtransaction(ro['hex'])
        fFound = False
        for output in ro['vout']:
            if output['type'] != 'standard':
                continue
            if output['scriptPubKey']['asm'].startswith('OP_ISCOINSTAKE'):
                fFound = True
                break
        assert(fFound)

        ro = nodes[1].walletpassphrasechange('changedPass', 'changedPass2')


        ro = nodes[2].listaddressgroupings()
        assert(len(ro) == 5)


        # Test lockunspent
        unspent = nodes[2].listunspent()
        assert(nodes[2].lockunspent(False, [unspent[0]]) == True)
        assert(len(nodes[2].listlockunspent()) == 1)
        unspentCheck = nodes[2].listunspent()
        assert(len(unspentCheck) < len(unspent))
        assert(nodes[2].lockunspent(True, [unspent[0]]) == True)
        unspentCheck = nodes[2].listunspent()
        assert(len(unspentCheck) == len(unspent))

        # Test bumpfee
        txnid = nodes[1].sendtoaddress(address1, 0.01, "", "", False, "", True)
        ro = nodes[1].bumpfee(txnid)
        assert(len(ro['errors']) == 0)

if __name__ == '__main__':
    WalletParticlTest().main()

