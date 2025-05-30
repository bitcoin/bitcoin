#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet keypool and interaction with wallet encryption/locking."""

import time
from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_not_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import WalletUnlock

class KeyPoolTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        nodes = self.nodes
        addr_before_encrypting = nodes[0].getnewaddress()
        addr_before_encrypting_data = nodes[0].getaddressinfo(addr_before_encrypting)

        # Encrypt wallet and wait to terminate
        nodes[0].encryptwallet('test')
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
        # Keep creating keys
        addr = nodes[0].getnewaddress()
        addr_data = nodes[0].getaddressinfo(addr)
        assert_not_equal(addr_before_encrypting_data['hdmasterfingerprint'], addr_data['hdmasterfingerprint'])
        assert_raises_rpc_error(-12, "Error: Keypool ran out, please call keypoolrefill first", nodes[0].getnewaddress)

        # put six (plus 2) new keys in the keypool (100% external-, +100% internal-keys, 1 in min)
        with WalletUnlock(nodes[0], 'test'):
            nodes[0].keypoolrefill(6)
        wi = nodes[0].getwalletinfo()
        assert_equal(wi['keypoolsize_hd_internal'], 24)
        assert_equal(wi['keypoolsize'], 24)

        # drain the internal keys
        nodes[0].getrawchangeaddress()
        nodes[0].getrawchangeaddress()
        nodes[0].getrawchangeaddress()
        nodes[0].getrawchangeaddress()
        nodes[0].getrawchangeaddress()
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
        addr.add(nodes[0].getnewaddress(address_type="bech32"))
        addr.add(nodes[0].getnewaddress(address_type="bech32"))
        addr.add(nodes[0].getnewaddress(address_type="bech32"))
        addr.add(nodes[0].getnewaddress(address_type="bech32"))
        addr.add(nodes[0].getnewaddress(address_type="bech32"))
        addr.add(nodes[0].getnewaddress(address_type="bech32"))
        assert len(addr) == 6
        # remember keypool sizes
        wi = nodes[0].getwalletinfo()
        kp_size_before = [wi['keypoolsize_hd_internal'], wi['keypoolsize']]
        # the next one should fail
        assert_raises_rpc_error(-12, "Error: Keypool ran out, please call keypoolrefill first", nodes[0].getnewaddress)
        # check that keypool sizes did not change
        wi = nodes[0].getwalletinfo()
        kp_size_after = [wi['keypoolsize_hd_internal'], wi['keypoolsize']]
        assert_equal(kp_size_before, kp_size_after)

        # refill keypool with three new addresses
        nodes[0].walletpassphrase('test', 1)
        nodes[0].keypoolrefill(3)

        # test walletpassphrase timeout
        time.sleep(1.1)
        assert_equal(nodes[0].getwalletinfo()["unlocked_until"], 0)

        # drain the keypool
        for _ in range(3):
            nodes[0].getnewaddress()
        assert_raises_rpc_error(-12, "Keypool ran out", nodes[0].getnewaddress)

        with WalletUnlock(nodes[0], 'test'):
            nodes[0].keypoolrefill(100)
            wi = nodes[0].getwalletinfo()
            assert_equal(wi['keypoolsize_hd_internal'], 400)
            assert_equal(wi['keypoolsize'], 400)

        # create a blank wallet
        nodes[0].createwallet(wallet_name='w2', blank=True, disable_private_keys=True)
        w2 = nodes[0].get_wallet_rpc('w2')

        # refer to initial wallet as w1
        w1 = nodes[0].get_wallet_rpc(self.default_wallet_name)

        # import private key and fund it
        address = addr.pop()
        desc = w1.getaddressinfo(address)['desc']
        res = w2.importdescriptors([{'desc': desc, 'timestamp': 'now'}])
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

if __name__ == '__main__':
    KeyPoolTest(__file__).main()
