#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet keypool and interaction with wallet encryption/locking."""

import re
import time
from decimal import Decimal

from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet_util import WalletUnlock

TEST_KEYPOOL_SIZE = 10
TEST_NEW_KEYPOOL_SIZE = TEST_KEYPOOL_SIZE + 2

class KeyPoolTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[f"-keypool={TEST_KEYPOOL_SIZE}"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        nodes = self.nodes

        # Derive addresses from the wallet without removing them from keypool
        addrs = []
        if not self.options.descriptors:
            path = str(self.nodes[0].datadir_path / 'wallet.dump')
            nodes[0].dumpwallet(path)
            file = open(path, "r", encoding="utf8")
            m = re.search(r"masterkey: (\w+)", file.read())
            file.close()
            xpriv = m.group(1)
            desc = descsum_create(f"wpkh({xpriv}/0h/0h/*h)")
            addrs = nodes[0].deriveaddresses(descriptor=desc, range=[0, 9])
        else:
            list_descriptors = nodes[0].listdescriptors()
            for desc in list_descriptors["descriptors"]:
                if desc['active'] and not desc["internal"] and desc["desc"][:4] == "wpkh":
                    addrs = nodes[0].deriveaddresses(descriptor=desc["desc"], range=[0, 9])

        addr0 = addrs[0]
        addr9 = addrs[9] # arbitrary future address index

        # Address is mine and active before it is removed from keypool by getnewaddress
        addr0_before_getting_data = nodes[0].getaddressinfo(addr0)
        assert addr0_before_getting_data['ismine']
        assert addr0_before_getting_data['isactive']

        addr_before_encrypting = nodes[0].getnewaddress()
        addr_before_encrypting_data = nodes[0].getaddressinfo(addr_before_encrypting)
        assert addr0 == addr_before_encrypting
        # Address is still mine and active even after being removed from keypool
        assert addr_before_encrypting_data['ismine']
        assert addr_before_encrypting_data['isactive']

        wallet_info_old = nodes[0].getwalletinfo()
        if not self.options.descriptors:
            assert addr_before_encrypting_data['hdseedid'] == wallet_info_old['hdseedid']

        # Address is mine and active before wallet is encrypted (resetting keypool)
        addr9_before_encrypting_data = nodes[0].getaddressinfo(addr9)
        assert addr9_before_encrypting_data['ismine']
        assert addr9_before_encrypting_data['isactive']

        # Imported things are never considered active, no need to rescan
        # Imported public keys / addresses can't be mine because they are not spendable
        if self.options.descriptors:
            nodes[0].importdescriptors([{
                "desc": "addr(bcrt1q95gp4zeaah3qcerh35yhw02qeptlzasdtst55v)",
                "timestamp": "now"
            }])
        else:
            nodes[0].importaddress("bcrt1q95gp4zeaah3qcerh35yhw02qeptlzasdtst55v", "label", rescan=False)
        import_addr_data = nodes[0].getaddressinfo("bcrt1q95gp4zeaah3qcerh35yhw02qeptlzasdtst55v")
        assert import_addr_data["iswatchonly"] is not self.options.descriptors
        assert not import_addr_data["ismine"]
        assert not import_addr_data["isactive"]

        if self.options.descriptors:
            nodes[0].importdescriptors([{
                "desc": "pk(02f893ca95b0d55b4ce4e72ae94982eb679158cb2ebc120ff62c17fedfd1f0700e)",
                "timestamp": "now"
            }])
        else:
            nodes[0].importpubkey("02f893ca95b0d55b4ce4e72ae94982eb679158cb2ebc120ff62c17fedfd1f0700e", "label", rescan=False)
        import_pub_data = nodes[0].getaddressinfo("bcrt1q4v7a8wn5vqd6fk4026s5gzzxyu7cfzz23n576h")
        assert import_pub_data["iswatchonly"] is not self.options.descriptors
        assert not import_pub_data["ismine"]
        assert not import_pub_data["isactive"]

        nodes[0].importprivkey("cPMX7v5CNV1zCphFSq2hnR5rCjzAhA1GsBfD1qrJGdj4QEfu38Qx", "label", rescan=False)
        import_priv_data = nodes[0].getaddressinfo("bcrt1qa985v5d53qqtrfujmzq2zrw3r40j6zz4ns02kj")
        assert not import_priv_data["iswatchonly"]
        assert import_priv_data["ismine"]
        assert not import_priv_data["isactive"]

        # Encrypt wallet and wait to terminate
        nodes[0].encryptwallet('test')
        addr9_after_encrypting_data = nodes[0].getaddressinfo(addr9)
        # Key is from unencrypted seed, no longer considered active
        assert not addr9_after_encrypting_data['isactive']
        # ...however it *IS* still mine since we can spend with this key
        assert addr9_after_encrypting_data['ismine']

        if self.options.descriptors:
            # Import hardened derivation only descriptors
            nodes[0].walletpassphrase('test', 10)
            nodes[0].importdescriptors([
                {
                    "desc": "wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0h/*h)#y4dfsj7n",
                    "timestamp": "now",
                    "range": [0,0],
                    "active": True
                },
                {
                    "desc": "pkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1h/*h)#a0nyvl0k",
                    "timestamp": "now",
                    "range": [0,0],
                    "active": True
                },
                {
                    "desc": "sh(wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/2h/*h))#lmeu2axg",
                    "timestamp": "now",
                    "range": [0,0],
                    "active": True
                },
                {
                    "desc": "wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/3h/*h)#jkl636gm",
                    "timestamp": "now",
                    "range": [0,0],
                    "active": True,
                    "internal": True
                },
                {
                    "desc": "pkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/4h/*h)#l3crwaus",
                    "timestamp": "now",
                    "range": [0,0],
                    "active": True,
                    "internal": True
                },
                {
                    "desc": "sh(wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/5h/*h))#qg8wa75f",
                    "timestamp": "now",
                    "range": [0,0],
                    "active": True,
                    "internal": True
                }
            ])
            nodes[0].walletlock()
        # Keep creating keys until we run out
        for _ in range(TEST_KEYPOOL_SIZE - 1):
            nodes[0].getnewaddress()
        addr = nodes[0].getnewaddress()
        addr_data = nodes[0].getaddressinfo(addr)
        wallet_info = nodes[0].getwalletinfo()
        assert addr_before_encrypting_data['hdmasterfingerprint'] != addr_data['hdmasterfingerprint']
        if not self.options.descriptors:
            assert addr_data['hdseedid'] == wallet_info['hdseedid']
        assert_raises_rpc_error(-12, "Error: Keypool ran out, please call keypoolrefill first", nodes[0].getnewaddress)

        # put two new keys in the keypool
        with WalletUnlock(nodes[0], 'test'):
            nodes[0].keypoolrefill(TEST_NEW_KEYPOOL_SIZE)
        wi = nodes[0].getwalletinfo()
        if self.options.descriptors:
            # Descriptors wallet: keypool size applies to both internal and external
            # chains and there are four of each (legacy, nested, segwit, and taproot)
            assert_equal(wi['keypoolsize_hd_internal'], TEST_NEW_KEYPOOL_SIZE * 4)
            assert_equal(wi['keypoolsize'], TEST_NEW_KEYPOOL_SIZE * 4)
        else:
            # Legacy wallet: keypool size applies to both internal and external HD chains
            assert_equal(wi['keypoolsize_hd_internal'], TEST_NEW_KEYPOOL_SIZE)
            assert_equal(wi['keypoolsize'], TEST_NEW_KEYPOOL_SIZE)

        # drain the internal keys
        for _ in range(TEST_NEW_KEYPOOL_SIZE):
            nodes[0].getrawchangeaddress()
        # remember keypool sizes
        wi = nodes[0].getwalletinfo()
        kp_size_before = [wi['keypoolsize_hd_internal'], wi['keypoolsize']]
        # the next one should fail
        assert_raises_rpc_error(-12, "Keypool ran out", nodes[0].getrawchangeaddress)
        # check that keypool sizes did not change
        wi = nodes[0].getwalletinfo()
        kp_size_after = [wi['keypoolsize_hd_internal'], wi['keypoolsize']]
        assert_equal(kp_size_before, kp_size_after)

        # drain the external keys
        addr = set()
        for _ in range(TEST_NEW_KEYPOOL_SIZE):
            addr.add(nodes[0].getnewaddress(address_type="bech32"))
        # remember keypool sizes
        wi = nodes[0].getwalletinfo()
        kp_size_before = [wi['keypoolsize_hd_internal'], wi['keypoolsize']]
        # the next one should fail
        assert_raises_rpc_error(-12, "Error: Keypool ran out, please call keypoolrefill first", nodes[0].getnewaddress)
        # check that keypool sizes did not change
        wi = nodes[0].getwalletinfo()
        kp_size_after = [wi['keypoolsize_hd_internal'], wi['keypoolsize']]
        assert_equal(kp_size_before, kp_size_after)

        # refill keypool
        nodes[0].walletpassphrase('test', 1)
        # At this point the keypool has >45 keys in it
        # calling keypoolrefill with anything smaller than that is a noop
        nodes[0].keypoolrefill(50)

        # test walletpassphrase timeout
        time.sleep(1.1)
        assert_equal(nodes[0].getwalletinfo()["unlocked_until"], 0)

        # drain the keypool
        for _ in range(50):
            nodes[0].getnewaddress()
        assert_raises_rpc_error(-12, "Keypool ran out", nodes[0].getnewaddress)

        with WalletUnlock(nodes[0], 'test'):
            nodes[0].keypoolrefill(100)
            wi = nodes[0].getwalletinfo()
            if self.options.descriptors:
                assert_equal(wi['keypoolsize_hd_internal'], 400)
                assert_equal(wi['keypoolsize'], 400)
            else:
                assert_equal(wi['keypoolsize_hd_internal'], 100)
                assert_equal(wi['keypoolsize'], 100)

            if not self.options.descriptors:
                # Check that newkeypool entirely flushes the keypool
                start_keypath = nodes[0].getaddressinfo(nodes[0].getnewaddress())['hdkeypath']
                start_change_keypath = nodes[0].getaddressinfo(nodes[0].getrawchangeaddress())['hdkeypath']
                # flush keypool and get new addresses
                nodes[0].newkeypool()
                end_keypath = nodes[0].getaddressinfo(nodes[0].getnewaddress())['hdkeypath']
                end_change_keypath = nodes[0].getaddressinfo(nodes[0].getrawchangeaddress())['hdkeypath']
                # The new keypath index should be 100 more than the old one
                new_index = int(start_keypath.rsplit('/',  1)[1][:-1]) + 100
                new_change_index = int(start_change_keypath.rsplit('/',  1)[1][:-1]) + 100
                assert_equal(end_keypath, "m/0'/0'/" + str(new_index) + "'")
                assert_equal(end_change_keypath, "m/0'/1'/" + str(new_change_index) + "'")

        # create a blank wallet
        nodes[0].createwallet(wallet_name='w2', blank=True, disable_private_keys=True)
        w2 = nodes[0].get_wallet_rpc('w2')

        # refer to initial wallet as w1
        w1 = nodes[0].get_wallet_rpc(self.default_wallet_name)

        # import private key and fund it
        address = addr.pop()
        desc = w1.getaddressinfo(address)['desc']
        if self.options.descriptors:
            res = w2.importdescriptors([{'desc': desc, 'timestamp': 'now'}])
        else:
            res = w2.importmulti([{'desc': desc, 'timestamp': 'now'}])
        assert_equal(res[0]['success'], True)

        with WalletUnlock(w1, 'test'):
            res = w1.sendtoaddress(address=address, amount=0.00010000)
        self.generate(nodes[0], 1)
        destination = addr.pop()

        # Using a fee rate (10 sat / byte) well above the minimum relay rate
        # creating a 5,000 sat transaction with change should not be possible
        assert_raises_rpc_error(-4, "Transaction needs a change address, but we can't generate it.", w2.walletcreatefundedpsbt, inputs=[], outputs=[{addr.pop(): 0.00005000}], subtractFeeFromOutputs=[0], feeRate=0.00010)

        # creating a 10,000 sat transaction without change, with a manual input, should still be possible
        res = w2.walletcreatefundedpsbt(inputs=w2.listunspent(), outputs=[{destination: 0.00010000}], subtractFeeFromOutputs=[0], feeRate=0.00010)
        assert_equal("psbt" in res, True)

        # creating a 10,000 sat transaction without change should still be possible
        res = w2.walletcreatefundedpsbt(inputs=[], outputs=[{destination: 0.00010000}], subtractFeeFromOutputs=[0], feeRate=0.00010)
        assert_equal("psbt" in res, True)
        # should work without subtractFeeFromOutputs if the exact fee is subtracted from the amount
        res = w2.walletcreatefundedpsbt(inputs=[], outputs=[{destination: 0.00008900}], feeRate=0.00010)
        assert_equal("psbt" in res, True)

        # dust change should be removed
        res = w2.walletcreatefundedpsbt(inputs=[], outputs=[{destination: 0.00008800}], feeRate=0.00010)
        assert_equal("psbt" in res, True)

        # create a transaction without change at the maximum fee rate, such that the output is still spendable:
        res = w2.walletcreatefundedpsbt(inputs=[], outputs=[{destination: 0.00010000}], subtractFeeFromOutputs=[0], feeRate=0.0008823)
        assert_equal("psbt" in res, True)
        assert_equal(res["fee"], Decimal("0.00009706"))

        # creating a 10,000 sat transaction with a manual change address should be possible
        res = w2.walletcreatefundedpsbt(inputs=[], outputs=[{destination: 0.00010000}], subtractFeeFromOutputs=[0], feeRate=0.00010, changeAddress=addr.pop())
        assert_equal("psbt" in res, True)

        if not self.options.descriptors:
            msg = "Error: Private keys are disabled for this wallet"
            assert_raises_rpc_error(-4, msg, w2.keypoolrefill, 100)

if __name__ == '__main__':
    KeyPoolTest(__file__).main()
