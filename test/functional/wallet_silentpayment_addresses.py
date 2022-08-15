#!/usr/bin/env python3
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)


class SilentAddressesTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-txindex=1"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def init_wallet(self, *, node):
        pass

    def run_test(self):
        self.nodes[0].createwallet(wallet_name=f'silent_wallet', descriptors=True, blank=True)

        sp_desc = "sp(cP8cbSymcBmE36ghaM1FFEUdCvphVeeYFvnyiHMSWVEa9i6jB2Th)#p39n508k"

        silent_wallet = self.nodes[0].get_wallet_rpc(f'silent_wallet')
        ret = silent_wallet.importdescriptors([{'desc': sp_desc, 'timestamp': 0, 'active': True}])
        assert ret[0]['success']

        sil_addr_01 = silent_wallet.getsilentaddress()['address']
        sil_addr_02 = silent_wallet.getsilentaddress('1')['address']
        sil_addr_03 = silent_wallet.getsilentaddress('2')['address']
        sil_addr_04 = silent_wallet.getsilentaddress('3')['address']
        sil_addr_05 = silent_wallet.getsilentaddress('4')['address']
        sil_addr_06 = silent_wallet.getsilentaddress('5')['address']

        assert_equal(sil_addr_01, "sprt168nnqa3fxvm0fqextvv3j8kzc2nx4u84u07hxtpgdkkswrrtqwl2thhzr9ze7cr4nlz7ragedagakt4u288vga50gjrp4vd23krsgtq67qrmt")
        assert_equal(sil_addr_02, "sprt168nnqa3fxvm0fqextvv3j8kzc2nx4u84u07hxtpgdkkswrrtqwltul72dwazf904zkytg29xnxwycfn2nwez45cscvsj3zgwdtgf4fglcq4ml")
        assert_equal(sil_addr_03, "sprt168nnqa3fxvm0fqextvv3j8kzc2nx4u84u07hxtpgdkkswrrtqwlp6q7ql2u6rxfv6kt8qre26rn3cpmc3jyw8d6zatvaqrw3hszp8kqvrwnt6")
        assert_equal(sil_addr_04, "sprt168nnqa3fxvm0fqextvv3j8kzc2nx4u84u07hxtpgdkkswrrtqwlrmcmqpqtlrhypawsd5wm0uzxgthaazkmz28zk28yl69zc8s38p3qangva0")
        assert_equal(sil_addr_05, "sprt168nnqa3fxvm0fqextvv3j8kzc2nx4u84u07hxtpgdkkswrrtqwlt8tjeraq5ygdemakam9eux8d9qrp3ex285z4nfvnx9kcfaq0l22qtltc29")
        assert_equal(sil_addr_06, "sprt168nnqa3fxvm0fqextvv3j8kzc2nx4u84u07hxtpgdkkswrrtqwlgcqk508uxj3ekneu9k9q4jnwalyds69lun7kclz2sy38musrdnycxaw9s3")

        ret = silent_wallet.decodesilentaddress(sil_addr_01)
        assert_equal(ret["scan_pubkey"], "d1e73076293336f483265b19191ec2c2a66af0f5e3fd732c286dad070c6b03be")
        assert_equal(ret["spend_pubkey"], "a5dee219459f60759fc5e1f5196f51db2ebc51cec4768f44861ab1aa8d87042c")

        ret = silent_wallet.decodesilentaddress(sil_addr_02)
        assert_equal(ret["scan_pubkey"], "d1e73076293336f483265b19191ec2c2a66af0f5e3fd732c286dad070c6b03be")
        assert_equal(ret["spend_pubkey"], "be7fca6bba2495f51588b428a6999c4c266a9bb22ad310c32128890e6ad09aa5")

        ret = silent_wallet.decodesilentaddress(sil_addr_03)
        assert_equal(ret["scan_pubkey"], "d1e73076293336f483265b19191ec2c2a66af0f5e3fd732c286dad070c6b03be")
        assert_equal(ret["spend_pubkey"], "1d03c0fab9a1992cd596700f2ad0e71c07788c88e3b742ead9d00dd1bc0413d8")

        ret = silent_wallet.decodesilentaddress(sil_addr_04)
        assert_equal(ret["scan_pubkey"], "d1e73076293336f483265b19191ec2c2a66af0f5e3fd732c286dad070c6b03be")
        assert_equal(ret["spend_pubkey"], "3de3600817f1dc81eba0da3b6fe08c85dfbd15b6251c5651c9fd14583c2270c4")

        ret = silent_wallet.decodesilentaddress(sil_addr_05)
        assert_equal(ret["scan_pubkey"], "d1e73076293336f483265b19191ec2c2a66af0f5e3fd732c286dad070c6b03be")
        assert_equal(ret["spend_pubkey"], "b3ae591f414221b9df6ddd973c31da500c31c9947a0ab34b2662db09e81ff528")

        ret = silent_wallet.decodesilentaddress(sil_addr_06)
        assert_equal(ret["scan_pubkey"], "d1e73076293336f483265b19191ec2c2a66af0f5e3fd732c286dad070c6b03be")
        assert_equal(ret["spend_pubkey"], "8c02d479f86947369e785b141594dddf91b0d17fc9fad8f8950244fbe406d993")


if __name__ == '__main__':
    SilentAddressesTest().main()
