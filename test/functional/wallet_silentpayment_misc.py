#!/usr/bin/env python3
from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.util import (
    assert_raises_rpc_error,
    assert_equal
)
from test_framework.descriptors import descsum_create
from test_framework.key import (
    ECKey,
    compute_xonly_pubkey,
)
from test_framework.wallet_util import bytes_to_wif


class SilentTransactioTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [[], ["-txindex=1"], []]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def init_wallet(self, *, node):
        pass

    def invalid_create_wallet(self):
        self.log.info("Testing the creation of invalid wallets")

        if self.is_bdb_compiled():
            assert_raises_rpc_error(-4, "Only descriptor wallets support silent payments.",
                self.nodes[1].createwallet, wallet_name='invalid_wallet', descriptors=False, silent_payment=True)

        assert_raises_rpc_error(-4, "Silent payments require the ability to store private keys.",
            self.nodes[1].createwallet, wallet_name='invalid_wallet', descriptors=True, disable_private_keys=True, silent_payment=True)

        if self.is_external_signer_compiled():
            assert_raises_rpc_error(-4, "Silent payments require the ability to store private keys.",
                self.nodes[1].createwallet, wallet_name='invalid_wallet',  descriptors=True, disable_private_keys=True, external_signer=True, silent_payment=True)

        assert_raises_rpc_error(-4, "Silent payment verification requires access to private keys. Cannot be used with encrypted wallets.",
            self.nodes[1].createwallet, wallet_name='invalid_wallet', descriptors=True, passphrase="passphrase", silent_payment=True)

    def watch_only_wallet_send(self):
        self.log.info("Testing if a watch only wallet is not able to send silent transaction")

        self.nodes[0].createwallet(wallet_name='watch_only_wallet', descriptors=True, disable_private_keys=True, blank=True)
        watch_only_wallet = self.nodes[0].get_wallet_rpc('watch_only_wallet')

        desc_import = [{
            "desc": descsum_create("wpkh(tpubD6NzVbkrYhZ4YNXVQbNhMK1WqguFsUXceaVJKbmno2aZ3B6QfbMeraaYvnBSGpV3vxLyTTK9DYT1yoEck4XUScMzXoQ2U2oSmE2JyMedq3H/0/*)"),
            "timestamp": "now",
            "internal": False,
            "active": True,
            "keypool": True,
            "range": [0, 100],
            "watchonly": True,
        }]

        watch_only_wallet.importdescriptors(desc_import)

        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 10, watch_only_wallet.getnewaddress())

        self.log.info("Watch-only wallets cannot send coins using silent_payment option")
        outputs = [{"sprt1nnspl0k3p5atkj88lsk703dv0nwczz440huq9dhzdj5un2aa76t2ptnk3pl2tg6v080uq2nvylqq8398zu6422n2lzaz403tpuq44egxa4t46": 15}]

        assert_raises_rpc_error(-4, "Silent payments require access to private keys to build transactions.",
            watch_only_wallet.send, outputs=outputs)

    def encrypted_wallet_send(self):
        self.log.info("Testing if encrypted SP wallet")

        self.nodes[0].createwallet(wallet_name='encrypted_wallet', descriptors=True, passphrase='passphrase')
        encrypted_wallet = self.nodes[0].get_wallet_rpc('encrypted_wallet')

        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 10, encrypted_wallet.getnewaddress())

        self.log.info("encrypted wallets must be able to send coins after decryption")
        outputs = [{"bcrt1pk0yzk76w2p55ykyjyfeq99td069c257se9nwugl7cl5geadq944spyc330": 15}]

        # send RPC can be run without decrypting the wallet and it must generate a incomplete PSBT
        tx = encrypted_wallet.send(outputs=outputs, options={"add_to_wallet": False})
        assert not tx['complete']

        # but when silent_payment option is enabled, wallet must be decrypted
        outputs = [{"sprt1nnspl0k3p5atkj88lsk703dv0nwczz440huq9dhzdj5un2aa76t9jt4nlxwucu9lzw75yxl74x9vq6jl7v33064hrx8kd0q5rgvjctq354m7t": 15}]
        assert_raises_rpc_error(-13, "Please enter the wallet passphrase with walletpassphrase first.",
            encrypted_wallet.send, outputs=outputs)

        encrypted_wallet.walletpassphrase('passphrase', 20)

        tx = encrypted_wallet.send(outputs=outputs)
        assert tx['complete']

    def test_addressinfo(self):
        self.log.info("Testing decodesilentaddress RPC")
        self.nodes[1].createwallet(wallet_name='sp_addr_info_01', descriptors=True, silent_payment=True)
        sp_addr_info_01 = self.nodes[1].get_wallet_rpc('sp_addr_info_01')

        addr01 = sp_addr_info_01.getsilentaddress()['address']

        assert_raises_rpc_error(-5, "Use `decodesilentaddress` RPC to get information about silent addresses.",
            sp_addr_info_01.getaddressinfo, addr01)

        addr01_info = sp_addr_info_01.decodesilentaddress(addr01)
        assert_equal(addr01_info['identifier'], 0)

        for i in range(20):
            sp_addr_info_01.getsilentaddress(str(i))['address']

        addr01 = sp_addr_info_01.getsilentaddress(str(21))['address']
        addr01_info = sp_addr_info_01.decodesilentaddress(addr01)
        assert_equal(addr01_info['identifier'], 21)

        for i in range(22,67):
            sp_addr_info_01.getsilentaddress(str(i))['address']

        addr01 = sp_addr_info_01.getsilentaddress(str(67))['address']
        addr01_info = sp_addr_info_01.decodesilentaddress(addr01)
        assert_equal(addr01_info['identifier'], 67)

    def test_sp_descriptor(self):
        self.log.info("Testing if SP descriptor works as expected")

        self.nodes[1].createwallet(wallet_name='sp_wallet_01', descriptors=True, silent_payment=True)
        sp_wallet_01 = self.nodes[1].get_wallet_rpc('sp_wallet_01')

        wallet_has_sp_desc = False

        sp_pub = ''

        for desc in sp_wallet_01.listdescriptors()['descriptors']:
            if "sp(" in desc['desc']:
                sp_pub = desc['desc']
                wallet_has_sp_desc = True

        assert wallet_has_sp_desc

        self.nodes[1].createwallet(wallet_name='sp_wallet_02', descriptors=True, blank=True)
        sp_wallet_02 = self.nodes[1].get_wallet_rpc('sp_wallet_02')

        result = sp_wallet_02.importdescriptors([{"desc": sp_pub, "timestamp": "now"}])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -4)
        assert_equal(result[0]['error']['message'], "Cannot import Silent Payment descriptor without private keys provided.")

        ranged_xpriv = "sp(tprv8ZgxMBicQKsPeKADhpai31FxDicMkXH29nNwuq7rK8m8w6cvwEyMCynLbXLZBYxrVdZLsopaMGwLUvNUwkx4GHcGNgpcUAGcfEVVEzjUYvb/42'/1'/0'/1/*)#l5qfzdxa"

        result = sp_wallet_02.importdescriptors([{"desc": ranged_xpriv, "timestamp": "now"}])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -5)
        assert_equal(result[0]['error']['message'], "Silent Payment descriptors cannot be ranged.")

        xpriv = "sp(cMcfPtLYsvQAUrP1Xw3uWYvtnNH1pSJ1jWbgTxL5yqwkgfbmaSkx)#vutc4cze"

        result = sp_wallet_02.importdescriptors([{"desc": xpriv, "timestamp": "now"}])
        assert_equal(result[0]['success'], True)

        # This will fail because "active = True" was not present in descriptor
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", sp_wallet_02.getsilentaddress)

        eckey = ECKey()
        eckey.generate()
        privkey = bytes_to_wif(eckey.get_bytes())
        pubkey = compute_xonly_pubkey(eckey.get_bytes())[0].hex()

        xpriv1 = "sp(cNSWZfhJWf1uYcsMKZxN94SgNiM8xQPhGyGaEkyCSR1GtSweLkdr)#vs4h975j"
        xpriv2 = "sp(cS1NMRiK87hdpzzNJgiYcdABjCPGxbBAdCrHoosqzjfeQY4z85rn)#dh4rsd8m"
        xpriv3 = descsum_create("sp(" + privkey + ")")

        result = sp_wallet_02.importdescriptors([
            {"desc": xpriv1, "timestamp": "now", "active": True},
            {"desc": xpriv2, "timestamp": "now", "active": True},
            {"desc": xpriv3, "timestamp": "now", "active": True}
        ])
        assert_equal(result[0]['success'], True)
        assert_equal(result[1]['success'], True)
        assert_equal(result[2]['success'], True)

        for desc in sp_wallet_02.listdescriptors(True)['descriptors']:
            if desc['desc'] == xpriv3:
                assert desc['active']
            else:
                assert not desc['active']

        sp_addr = sp_wallet_02.getsilentaddress()['address']
        assert_raises_rpc_error(-5, "Use `decodesilentaddress` RPC to get information about silent addresses.",
            sp_wallet_02.getaddressinfo, sp_addr)

        addr_info = sp_wallet_02.decodesilentaddress(sp_addr)

        assert_equal(addr_info['address'], sp_addr)
        assert_equal(addr_info['identifier'], 0)
        assert_equal(addr_info['label'], '')
        assert_equal(addr_info['spend_pubkey'], pubkey)

    def test_big_transaction_multiple_wallets(self):
        self.log.info("Testing a big transaction to SP wallet")

        self.nodes[0].createwallet(wallet_name='mw_01', descriptors=True)
        mw_01 = self.nodes[0].get_wallet_rpc('mw_01')

        self.nodes[1].createwallet(wallet_name='mw_02', descriptors=True, silent_payment=True)
        mw_02 = self.nodes[1].get_wallet_rpc('mw_02')

        self.nodes[1].createwallet(wallet_name='mw_03', descriptors=True, silent_payment=True)
        mw_03 = self.nodes[1].get_wallet_rpc('mw_03')

        self.nodes[1].createwallet(wallet_name='mw_04', descriptors=True)
        mw_04 = self.nodes[1].get_wallet_rpc('mw_04')

        self.nodes[1].createwallet(wallet_name='mw_05', descriptors=True, silent_payment=True)
        mw_05 = self.nodes[1].get_wallet_rpc('mw_05')

        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 15, mw_01.getnewaddress())

        recv_addr_01 = mw_02.getsilentaddress()['address']
        recv_addr_02 = mw_02.getnewaddress('', 'bech32m')
        recv_addr_03 = mw_02.getnewaddress('', 'bech32')

        recv_addr_04 = mw_03.getsilentaddress()['address']
        recv_addr_05 = mw_03.getnewaddress('', 'legacy')

        recv_addr_06 = mw_04.getnewaddress('', 'legacy')
        recv_addr_07 = mw_04.getnewaddress('', 'bech32m')
        recv_addr_08 = mw_04.getnewaddress('', 'p2sh-segwit')

        recv_addr_09 = mw_05.getsilentaddress()['address']
        recv_addr_10 = mw_05.getnewaddress('', 'p2sh-segwit')
        recv_addr_11 = mw_05.getnewaddress('', 'bech32m')
        recv_addr_12 = mw_05.getnewaddress('', 'bech32')
        recv_addr_13 = mw_05.getnewaddress('', 'legacy')

        outputs = [{recv_addr_01: 2}, {recv_addr_02: 3}, {recv_addr_03: 1}, {recv_addr_04: 2}, {recv_addr_05: 1},
            {recv_addr_06: 4}, {recv_addr_07: 3}, {recv_addr_08: 2}, {recv_addr_09: 5}, {recv_addr_10: 4},
            {recv_addr_11: 2}, {recv_addr_12: 1}, {recv_addr_13: 2}]

        ret = mw_01.send(outputs)

        assert ret['complete']

        self.sync_mempools()

        assert_equal(len(mw_02.listunspent(0)), 3)
        assert_equal(len(mw_03.listunspent(0)), 2)
        assert_equal(len(mw_04.listunspent(0)), 3)
        assert_equal(len(mw_05.listunspent(0)), 5)

    def test_backup_sp_descriptor(self):
        self.log.info("Testing SP descriptor backup")

        self.nodes[0].createwallet(wallet_name='sender_bk_01', descriptors=True)
        sender_bk_01 = self.nodes[0].get_wallet_rpc('sender_bk_01')

        self.nodes[1].createwallet(wallet_name='receiver_bk_01', descriptors=True, silent_payment=True)
        receiver_bk_01 = self.nodes[1].get_wallet_rpc('receiver_bk_01')

        assert receiver_bk_01.getwalletinfo()['silent_payment']

        receiver_sp_address = receiver_bk_01.getsilentaddress()['address']

        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY, sender_bk_01.getnewaddress())
        self.generate(self.nodes[0], COINBASE_MATURITY)

        ret1 = sender_bk_01.send({receiver_sp_address: 2})
        assert ret1['complete']

        ret2 = sender_bk_01.send({receiver_sp_address: 3})
        assert ret2['complete']

        ret3 = sender_bk_01.send({receiver_sp_address: 5})
        assert ret3['complete']

        self.generate(self.nodes[0], 7)

        self.sync_all()

        assert_equal(len(receiver_bk_01.listunspent()), 3)

        for utxo in receiver_bk_01.listunspent(0):
            assert utxo['desc'].startswith('rawtr(')

        sp_priv = ''

        for desc in receiver_bk_01.listdescriptors(True)['descriptors']:
            if "sp(" in desc['desc']:
                sp_priv = desc['desc']

        assert sp_priv != ''

        self.nodes[1].createwallet(wallet_name='receiver_bk_02', descriptors=True, blank=True)
        receiver_bk_02 = self.nodes[1].get_wallet_rpc('receiver_bk_02')

        ret = receiver_bk_02.importdescriptors([{'desc': sp_priv, 'timestamp': 0, 'active': True}])

        assert ret[0]['success']
        assert receiver_bk_02.getwalletinfo()['silent_payment']

        assert_equal(len(receiver_bk_02.listunspent()), 3)

    def test_load_silent_wallet(self):
        self.log.info("Testing wallet loading after receveing transactions")

        self.nodes[0].createwallet(wallet_name='load_sender_wallet_01', descriptors=True)
        load_sender_wallet_01 = self.nodes[0].get_wallet_rpc('load_sender_wallet_01')

        self.nodes[1].createwallet(wallet_name='load_recipient_wallet_01', descriptors=True, silent_payment=True)
        load_recipient_wallet_01 = self.nodes[1].get_wallet_rpc('load_recipient_wallet_01')

        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 25, load_sender_wallet_01.getnewaddress())

        sp_recv_addr = load_recipient_wallet_01.getsilentaddress()['address']

        self.nodes[1].unloadwallet('load_recipient_wallet_01')

        silent_tx_ids = []

        for _ in range(1,5):

            outputs = [{sp_recv_addr: 5}]

            silent_tx_ids.append(load_sender_wallet_01.send(outputs=outputs)['txid'])

            self.generate(self.nodes[0], 1)

        self.sync_all()

        self.nodes[1].loadwallet('load_recipient_wallet_01')

        listunspent = [u['txid'] for u in load_recipient_wallet_01.listunspent(0)]

        assert len(listunspent) == len(silent_tx_ids)

        for txid in silent_tx_ids:
            assert txid in listunspent

    # it makes no sense for users to send themselves silent payments, but that should work anyway.
    def test_self_sp_transfer(self):

        self.log.info("Testing self transfer")

        self.nodes[1].createwallet(wallet_name='self_sender_wallet_01', descriptors=True, silent_payment=True)
        self_sender_wallet_01 = self.nodes[1].get_wallet_rpc('self_sender_wallet_01')

        self.generatetoaddress(self.nodes[1], 1, self_sender_wallet_01.getnewaddress())

        self.generate(self.nodes[1], COINBASE_MATURITY)

        sp_recv_addr = self_sender_wallet_01.getsilentaddress()['address']

        ret = self_sender_wallet_01.send([{sp_recv_addr: 0.2}])

        assert ret['complete']

        assert_equal(len(self_sender_wallet_01.listunspent(0)), 2)

    def test_scantxoutset(self):
        self.log.info("Testing  SP scantxoutset")
        self.nodes[0].createwallet(wallet_name='scan_sender_wallet_02', descriptors=True)
        scan_sender_wallet_02 = self.nodes[0].get_wallet_rpc('scan_sender_wallet_02')

        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 10, scan_sender_wallet_02.getnewaddress())

        self.nodes[2].createwallet(wallet_name='recipient_wallet_03', descriptors=True, silent_payment=True, blank=True)
        recipient_wallet_03 = self.nodes[2].get_wallet_rpc('recipient_wallet_03')

        desc_import = [{
            "desc": "sp(cQq73sG9nQN1bUNneZHKiYV3jhnED7Y3oNUdLHoHrpZdJD51uaRD)#9llg6xjm",
            "timestamp": "now",
            "internal": False,
            "active": True,
        }]

        self.log.info("send multiple silent transactions to test scantxoutset")
        for amount in [2.75, 3.98, 2.30]:

            recipient_wallet_03.importdescriptors(desc_import)

            recv_addr_03 = recipient_wallet_03.getsilentaddress()['address']

            outputs = [{recv_addr_03: amount}]

            silent_tx_ret = scan_sender_wallet_02.send(outputs=outputs)

            self.sync_mempools()

            silent_utxo = [x for x in recipient_wallet_03.listunspent(0) if x['txid'] == silent_tx_ret['txid']][0]

            assert silent_utxo['address'] != recv_addr_03

            self.generate(self.nodes[0], 7)

            self.sync_all()

        self.wait_until(lambda: all(i["synced"] for i in self.nodes[0].getindexinfo().values()))

        utxos = recipient_wallet_03.listunspent()

        scan_result = self.nodes[1].scantxoutset("start", [desc_import[0]["desc"]])

        self.log.info("check if silent scantxoutset and listunspent have the same result for the same descriptor")
        assert len(scan_result["unspents"]) == len(utxos)

        for scan_tx in scan_result["unspents"]:
            assert len([x for x in utxos if x['txid'] == scan_tx['txid']]) == 1


    def run_test(self):
        self.invalid_create_wallet()
        self.watch_only_wallet_send()
        self.encrypted_wallet_send()
        self.test_self_sp_transfer()
        self.test_sp_descriptor()
        self.test_big_transaction_multiple_wallets()
        self.test_backup_sp_descriptor()
        self.test_load_silent_wallet()
        self.test_scantxoutset()
        self.test_addressinfo()


if __name__ == '__main__':
    SilentTransactioTest().main()
