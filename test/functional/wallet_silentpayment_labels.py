#!/usr/bin/env python3
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
    assert_equal,
)

import random
import string


class SilentLabelTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def init_wallet(self, *, node):
        pass

    def random_string(self, n):
        return ''.join(random.choice(string.ascii_lowercase) for _ in range(n))

    def test_address_label(self):
        self.nodes[0].createwallet(wallet_name='sp_wallet_01', descriptors=True, silent_payment=True)
        sp_wallet_01 = self.nodes[0].get_wallet_rpc('sp_wallet_01')

        existing_labels = []

        ret = sp_wallet_01.listsilentaddresses()
        assert_equal(ret['wallet_name'], 'sp_wallet_01')
        assert_equal(ret['silent_addresses'], [])

        assert_raises_rpc_error(-4, "Error: Use `getsilentaddress` RPC to generate silent payment address",
                sp_wallet_01.getnewaddress, address_type='silent-payment')

        self.log.info("Confirm that calling getsilentaddress without the label parameter will generate same address.")
        assert_equal(sp_wallet_01.getsilentaddress()['address'], sp_wallet_01.getsilentaddress()['address'])

        self.log.info("Confirm that calling getsilentaddress with different labels will generate different address.")
        label01 = self.random_string(8)
        label02 = self.random_string(8)

        assert_equal(sp_wallet_01.getsilentaddress(label01)['address'], sp_wallet_01.getsilentaddress(label01)['address'])
        assert sp_wallet_01.getsilentaddress(label01)['address'] != sp_wallet_01.getsilentaddress(label02)['address']

        labels = sp_wallet_01.listlabels()
        assert label01 in labels
        assert label02 in labels

        assert_equal(sp_wallet_01.listlabels(), sp_wallet_01.listlabels('silent-payment'))

        assert_equal(len(sp_wallet_01.getaddressesbylabel()), 3)
        assert_equal(len(sp_wallet_01.getaddressesbylabel(label01)), 1)
        assert_equal(len(sp_wallet_01.getaddressesbylabel(label02)), 1)

        identifier = 3

        existing_labels.append('')
        existing_labels.append(label01)
        existing_labels.append(label02)

        for _ in range(96):
            label = self.random_string(8)
            existing_labels.append(label)
            addr = sp_wallet_01.getsilentaddress(label)['address']

            ret = sp_wallet_01.decodesilentaddress(addr)
            assert_equal(identifier,ret['identifier'])

            identifier = identifier + 1

        label = self.random_string(8)
        assert_raises_rpc_error(-4, "No addresses available", sp_wallet_01.getsilentaddress, label=label)

        ret = sp_wallet_01.listsilentaddresses()
        assert_equal(ret['wallet_name'], 'sp_wallet_01')

        identifier = 0

        for addr in ret['silent_addresses']:
            assert_equal(addr['label'], existing_labels[identifier])
            ret = sp_wallet_01.decodesilentaddress(addr['address'])
            assert_equal(identifier,ret['identifier'])

            identifier = identifier + 1

    def run_test(self):
        self.test_address_label()


if __name__ == '__main__':
    SilentLabelTest().main()

