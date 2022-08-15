#!/usr/bin/env python3
from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.util import (
    assert_equal,
)

import random
import string


class SilentIdentifierSimple(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        # Test compatibility with PR #25957
        self.extra_args = [["-blockfilterindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def init_wallet(self, *, node):
        pass

    def random_string(self, n):
        return ''.join(random.choice(string.ascii_lowercase) for _ in range(n))

    def one_wallet(self, sender_wallet):
        self.nodes[0].createwallet(wallet_name=f'recipient_wallet_01', descriptors=True, silent_payment=True)
        recipient_wallet_01 = self.nodes[0].get_wallet_rpc(f'recipient_wallet_01')

        label01 = self.random_string(8)
        recv_addr_01 = recipient_wallet_01.getsilentaddress()['address']
        recv_addr_02 = recipient_wallet_01.getsilentaddress(label01)['address']

        outputs = [{recv_addr_01: 3}, {recv_addr_02: 2}]

        sender_wallet.send(outputs=outputs)

        self.generatetoaddress(self.nodes[0], 1, sender_wallet.getnewaddress())

        recipient_wallet_01_utxos = recipient_wallet_01.listunspent()
        assert_equal(len(recipient_wallet_01_utxos), 2)
        assert label01 in [utxo['label'] for utxo in recipient_wallet_01_utxos]

    def two_wallets(self, sender_wallet):
        self.nodes[0].createwallet(wallet_name=f'recipient_wallet_02', descriptors=True, silent_payment=True)
        recipient_wallet_02 = self.nodes[0].get_wallet_rpc(f'recipient_wallet_02')

        self.nodes[0].createwallet(wallet_name=f'recipient_wallet_03', descriptors=True, silent_payment=True)
        recipient_wallet_03 = self.nodes[0].get_wallet_rpc(f'recipient_wallet_03')

        label01 = self.random_string(8)
        recv_addr_01 = recipient_wallet_02.getsilentaddress()['address']
        recv_addr_02 = recipient_wallet_02.getsilentaddress(label01)['address']

        label02 = self.random_string(8)
        recv_addr_03 = recipient_wallet_03.getsilentaddress()['address']
        recv_addr_04 = recipient_wallet_03.getsilentaddress(label02)['address']

        outputs = [{recv_addr_01: 1}, {recv_addr_02: 2}, {recv_addr_03: 3}, {recv_addr_04: 4}]

        assert sender_wallet.send(outputs=outputs)['complete']

        self.generatetoaddress(self.nodes[0], 1, sender_wallet.getnewaddress())

        recipient_wallet_02_utxos = recipient_wallet_02.listunspent()
        assert_equal(len(recipient_wallet_02_utxos), 2)
        assert label01 in [utxo['label'] for utxo in recipient_wallet_02_utxos]

        recipient_wallet_03_utxos = recipient_wallet_03.listunspent()
        assert_equal(len(recipient_wallet_03_utxos), 2)
        assert label02 in [utxo['label'] for utxo in recipient_wallet_03_utxos]

    def test_block_filter(self, sender_wallet):
        self.nodes[0].createwallet(wallet_name=f'recipient_wallet_04', descriptors=True, silent_payment=True)
        recipient_wallet_04 = self.nodes[0].get_wallet_rpc(f'recipient_wallet_04')

        label01 = self.random_string(8)
        recv_addr_01 = recipient_wallet_04.getsilentaddress()['address']
        recv_addr_02 = recipient_wallet_04.getsilentaddress(label01)['address']

        self.nodes[0].unloadwallet(f'recipient_wallet_04')

        for i in range(50):
            outputs = [{recv_addr_01: 1}, {recv_addr_02: 2}]
            assert sender_wallet.send(outputs=outputs)['complete']
            self.generatetoaddress(self.nodes[0], 1, sender_wallet.getnewaddress())

        self.nodes[0].loadwallet(f'recipient_wallet_04')
        recipient_wallet_04 = self.nodes[0].get_wallet_rpc(f'recipient_wallet_04')

        assert_equal(recipient_wallet_04.getbalance(), 150)
        assert_equal(len(recipient_wallet_04.listunspent()), 100)

    def run_test(self):
        self.nodes[0].createwallet(wallet_name=f'sender_wallet', descriptors=True)
        sender_wallet = self.nodes[0].get_wallet_rpc(f'sender_wallet')

        self.generatetoaddress(self.nodes[0], 1, sender_wallet.getnewaddress('', 'bech32'))
        self.generatetoaddress(self.nodes[0], 1, sender_wallet.getnewaddress('', 'bech32'))
        self.generatetoaddress(self.nodes[0], 1, sender_wallet.getnewaddress('', 'bech32m'))
        self.generatetoaddress(self.nodes[0], 1, sender_wallet.getnewaddress('', 'legacy'))
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 10, "bcrt1qjqmxmkpmxt80xz4y3746zgt0q3u3ferr34acd5")

        self.one_wallet(sender_wallet)
        self.two_wallets(sender_wallet)
        self.test_block_filter(sender_wallet)


if __name__ == '__main__':
    SilentIdentifierSimple().main()
