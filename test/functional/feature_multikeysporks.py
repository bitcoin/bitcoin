#!/usr/bin/env python3
# Copyright (c) 2018-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import connect_nodes, wait_until

'''
feature_multikeysporks.py

Test logic for several signer keys usage for spork broadcast.

We set 5 possible keys for sporks signing and set minimum
required signers to 3. We check 1 and 2 signers can't set the spork
value, any 3 signers can change spork value and other 3 signers
can change it again.
'''


class MultiKeySporkTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 5
        self.setup_clean_chain = True

    def setup_network(self):
        # secret(base58): 931wyuRNVYvhg18Uu9bky5Qg1z4QbxaJ7fefNBzjBPiLRqcd33F
        # keyid(hex): 60f0f57f71f0081f1aacdd8432340a33a526f91b
        # address(base58): yNsMZhEhYqv14TgdYb1NS2UmNZjE8FSJxa

        # secret(base58): 91vbXGMSWKGHom62986XtL1q2mQDA12ngcuUNNe5NfMSj44j7g3
        # keyid(hex): 43dff2b09de2f904f688ec14ee6899087b889ad0
        # address(base58): yfLSXFfipnkgYioD6L8aUNyfRgEBuJv48h

        # secret(base58): 92bxUjPT5AhgXuXJwfGGXqhomY2SdQ55MYjXyx9DZNxCABCSsRH
        # keyid(hex): d9aa5fa00cce99101a4044e65dc544d1579890de
        # address(base58): ygcG5S2pQz2U1UAaHvU6EznKZW7yapKMA7

        # secret(base58): 934yPXiVGf4RCY2qTs2Bt5k3TEtAiAg12sMxCt8yVWbSU7p3fuD
        # keyid(hex): 0b23935ce0bea3b997a334f6fa276c9fa17687b2
        # address(base58): ycbRQWbovrhQMTuxg9p4LAuW5SCMAKqPrn

        # secret(base58): 92Cxwia363Wg2qGF1fE5z4GKi8u7r1nrWQXdtsj2ACZqaDPSihD
        # keyid(hex): 1d1098b2b1f759b678a0a7a098637a9b898adcac
        # address(base58): yc5TGfcHYoLCrcbVy4umsiDjsYUn39vLui

        self.add_nodes(5)

        spork_chain_params =   ["-sporkaddr=ygcG5S2pQz2U1UAaHvU6EznKZW7yapKMA7",
                                "-sporkaddr=yfLSXFfipnkgYioD6L8aUNyfRgEBuJv48h",
                                "-sporkaddr=yNsMZhEhYqv14TgdYb1NS2UmNZjE8FSJxa",
                                "-sporkaddr=ycbRQWbovrhQMTuxg9p4LAuW5SCMAKqPrn",
                                "-sporkaddr=yc5TGfcHYoLCrcbVy4umsiDjsYUn39vLui",
                                "-minsporkkeys=3"]

        # Node0 extra args to use on normal node restarts
        self.node0_extra_args = ["-sporkkey=931wyuRNVYvhg18Uu9bky5Qg1z4QbxaJ7fefNBzjBPiLRqcd33F"] + spork_chain_params

        self.start_node(0, self.node0_extra_args)
        self.start_node(1, ["-sporkkey=91vbXGMSWKGHom62986XtL1q2mQDA12ngcuUNNe5NfMSj44j7g3"] + spork_chain_params)
        self.start_node(2, ["-sporkkey=92bxUjPT5AhgXuXJwfGGXqhomY2SdQ55MYjXyx9DZNxCABCSsRH"] + spork_chain_params)
        self.start_node(3, ["-sporkkey=934yPXiVGf4RCY2qTs2Bt5k3TEtAiAg12sMxCt8yVWbSU7p3fuD"] + spork_chain_params)
        self.start_node(4, ["-sporkkey=92Cxwia363Wg2qGF1fE5z4GKi8u7r1nrWQXdtsj2ACZqaDPSihD"] + spork_chain_params)

        # connect nodes at start
        for i in range(0, 5):
            for j in range(i, 5):
                connect_nodes(self.nodes[i], j)

    def get_test_spork_value(self, node, spork_name):
        self.bump_mocktime(5)  # advance ProcessTick
        info = node.spork('show')
        # use InstantSend spork for tests
        return info[spork_name]

    def test_spork(self, spork_name, final_value):
        # check test spork default state
        for node in self.nodes:
            assert(self.get_test_spork_value(node, spork_name) == 4070908800)

        self.bump_mocktime(1)
        # first and second signers set spork value
        self.nodes[0].spork(spork_name, 1)
        self.nodes[1].spork(spork_name, 1)
        # spork change requires at least 3 signers
        time.sleep(10)
        for node in self.nodes:
            assert(self.get_test_spork_value(node, spork_name) != 1)

        # restart with no extra args to trigger CheckAndRemove
        self.restart_node(0)
        assert(self.get_test_spork_value(self.nodes[0], spork_name) != 1)

        # restart again with corect_params, should resync spork parts from other nodes
        self.restart_node(0, self.node0_extra_args)
        for i in range(1, 5):
            connect_nodes(self.nodes[0], i)

        # third signer set spork value
        self.nodes[2].spork(spork_name, 1)
        # now spork state is changed
        for node in self.nodes:
            wait_until(lambda: self.get_test_spork_value(node, spork_name) == 1, sleep=0.1, timeout=10)

        # restart with no extra args to trigger CheckAndRemove, should reset the spork back to its default
        self.restart_node(0)
        assert(self.get_test_spork_value(self.nodes[0], spork_name) == 4070908800)

        # restart again with corect_params, should resync sporks from other nodes
        self.restart_node(0, self.node0_extra_args)
        for i in range(1, 5):
            connect_nodes(self.nodes[0], i)

        wait_until(lambda: self.get_test_spork_value(self.nodes[0], spork_name) == 1, sleep=0.1, timeout=10)

        self.bump_mocktime(1)
        # now set the spork again with other signers to test
        # old and new spork messages interaction
        self.nodes[2].spork(spork_name, final_value)
        self.nodes[3].spork(spork_name, final_value)
        self.nodes[4].spork(spork_name, final_value)
        for node in self.nodes:
            wait_until(lambda: self.get_test_spork_value(node, spork_name) == final_value, sleep=0.1, timeout=10)

    def run_test(self):
        self.test_spork('SPORK_2_INSTANTSEND_ENABLED', 2)
        self.test_spork('SPORK_3_INSTANTSEND_BLOCK_FILTERING', 3)
        for node in self.nodes:
            assert(self.get_test_spork_value(node, 'SPORK_2_INSTANTSEND_ENABLED') == 2)
            assert(self.get_test_spork_value(node, 'SPORK_3_INSTANTSEND_BLOCK_FILTERING') == 3)


if __name__ == '__main__':
    MultiKeySporkTest().main()
