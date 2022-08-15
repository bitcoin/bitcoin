#!/usr/bin/env python3
from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import COINBASE_MATURITY


class SilentAddressReuseTest(BitcoinTestFramework):
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

    def run_test(self):
        self.nodes[0].createwallet(wallet_name=f'miner_wallet', descriptors=True)
        miner_wallet = self.nodes[0].get_wallet_rpc(f'miner_wallet')
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 10, miner_wallet.getnewaddress())

        print(miner_wallet.getbalances())

        self.nodes[0].createwallet(wallet_name=f'sender_wallet', descriptors=True)
        sender_wallet = self.nodes[0].get_wallet_rpc(f'sender_wallet')
        sender_address = sender_wallet.getnewaddress()

        miner_wallet.send(outputs=[{sender_address: 5}])
        miner_wallet.send(outputs=[{sender_address: 7}])

        self.generate(self.nodes[0], 8, sync_fun=self.no_op)

        print(sender_wallet.getbalances())

        self.nodes[0].createwallet(wallet_name=f'silent_wallet', descriptors=True, silent_payment=True)
        silent_wallet = self.nodes[0].get_wallet_rpc(f'silent_wallet')

        silent_address = silent_wallet.getsilentaddress()['address']

        sender_wallet.send(outputs=[{silent_address: 4}])
        sender_wallet.send(outputs=[{silent_address: 6}])

        self.generate(self.nodes[0], 8, sync_fun=self.no_op)

        utxos = silent_wallet.listunspent()

        assert len(utxos) == 2

        assert utxos[0]['address'] != utxos[1]['address']
        assert utxos[0]['scriptPubKey'] != utxos[1]['scriptPubKey']


if __name__ == '__main__':
    SilentAddressReuseTest().main()

