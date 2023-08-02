#!/usr/bin/env python3
from test_framework.test_framework import BitcoinTestFramework
from test_framework.descriptors import descsum_create


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

    def create_silent_payments_wallet(self):

        self.nodes[0].createwallet(wallet_name='sp', descriptors=True, silent_payment=True, blank=True)
        sp_wallet = self.nodes[0].get_wallet_rpc('sp')
        desc_str = "sp(cN6fC62XuB4gv3tu4tFnwtd72jTfT7Mezzhn7b8GSZKHTHZiBegX,cSrhaUv9F9pKyW812V4M2mcf5awrwFYX8RWUfcd9xqk2Co6Rr2bh)"
        desc = [{"desc": descsum_create(desc_str), "active": True, "timestamp": "now", "internal": False}]
        sp_wallet.importdescriptors(desc)
        assert sp_wallet.getsilentaddress()["address"] == "sprt1qqfwvnptdd7ph2dgwzguh3k4vyqxzvr94kkhgxyrv4wgysnwd3l8nvq3qhnavtwv7qjk35pkalvqkacf4sfsf6c9k9y0f35q6n0y6zmyk6smx9ad8"

    def run_test(self):
        self.create_silent_payments_wallet()


if __name__ == '__main__':
    SilentTransactioTest().main()
