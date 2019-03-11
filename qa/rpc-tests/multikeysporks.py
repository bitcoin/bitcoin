#!/usr/bin/env python3
# Copyright (c) 2018 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from time import *

'''
multikeysporks.py

Test logic for several signer keys usage for spork broadcast.

We set 5 possible keys for sporks signing and set minimum 
required signers to 3. We check 1 and 2 signers can't set the spork 
value, any 3 signers can change spork value and other 3 signers
can change it again.
'''


class MultiKeySporkTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 5
        self.setup_clean_chain = True
        self.is_network_split = False

    def setup_network(self):
        self.nodes = []

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

        self.nodes.append(start_node(0, self.options.tmpdir,
                                     ["-sporkkey=931wyuRNVYvhg18Uu9bky5Qg1z4QbxaJ7fefNBzjBPiLRqcd33F",
                                      "-sporkaddr=ygcG5S2pQz2U1UAaHvU6EznKZW7yapKMA7",
                                      "-sporkaddr=yfLSXFfipnkgYioD6L8aUNyfRgEBuJv48h",
                                      "-sporkaddr=yNsMZhEhYqv14TgdYb1NS2UmNZjE8FSJxa",
                                      "-sporkaddr=ycbRQWbovrhQMTuxg9p4LAuW5SCMAKqPrn",
                                      "-sporkaddr=yc5TGfcHYoLCrcbVy4umsiDjsYUn39vLui",
                                      "-minsporkkeys=3"]))
        self.nodes.append(start_node(1, self.options.tmpdir,
                                     ["-sporkkey=91vbXGMSWKGHom62986XtL1q2mQDA12ngcuUNNe5NfMSj44j7g3",
                                      "-sporkaddr=ygcG5S2pQz2U1UAaHvU6EznKZW7yapKMA7",
                                      "-sporkaddr=yfLSXFfipnkgYioD6L8aUNyfRgEBuJv48h",
                                      "-sporkaddr=yNsMZhEhYqv14TgdYb1NS2UmNZjE8FSJxa",
                                      "-sporkaddr=ycbRQWbovrhQMTuxg9p4LAuW5SCMAKqPrn",
                                      "-sporkaddr=yc5TGfcHYoLCrcbVy4umsiDjsYUn39vLui",
                                      "-minsporkkeys=3"]))
        self.nodes.append(start_node(2, self.options.tmpdir,
                                     ["-sporkkey=92bxUjPT5AhgXuXJwfGGXqhomY2SdQ55MYjXyx9DZNxCABCSsRH",
                                      "-sporkaddr=ygcG5S2pQz2U1UAaHvU6EznKZW7yapKMA7",
                                      "-sporkaddr=yfLSXFfipnkgYioD6L8aUNyfRgEBuJv48h",
                                      "-sporkaddr=yNsMZhEhYqv14TgdYb1NS2UmNZjE8FSJxa",
                                      "-sporkaddr=ycbRQWbovrhQMTuxg9p4LAuW5SCMAKqPrn",
                                      "-sporkaddr=yc5TGfcHYoLCrcbVy4umsiDjsYUn39vLui",
                                      "-minsporkkeys=3"]))
        self.nodes.append(start_node(3, self.options.tmpdir,
                                     ["-sporkkey=934yPXiVGf4RCY2qTs2Bt5k3TEtAiAg12sMxCt8yVWbSU7p3fuD",
                                      "-sporkaddr=ygcG5S2pQz2U1UAaHvU6EznKZW7yapKMA7",
                                      "-sporkaddr=yfLSXFfipnkgYioD6L8aUNyfRgEBuJv48h",
                                      "-sporkaddr=yNsMZhEhYqv14TgdYb1NS2UmNZjE8FSJxa",
                                      "-sporkaddr=ycbRQWbovrhQMTuxg9p4LAuW5SCMAKqPrn",
                                      "-sporkaddr=yc5TGfcHYoLCrcbVy4umsiDjsYUn39vLui",
                                      "-minsporkkeys=3"]))
        self.nodes.append(start_node(4, self.options.tmpdir,
                                     ["-sporkkey=92Cxwia363Wg2qGF1fE5z4GKi8u7r1nrWQXdtsj2ACZqaDPSihD",
                                      "-sporkaddr=ygcG5S2pQz2U1UAaHvU6EznKZW7yapKMA7",
                                      "-sporkaddr=yfLSXFfipnkgYioD6L8aUNyfRgEBuJv48h",
                                      "-sporkaddr=yNsMZhEhYqv14TgdYb1NS2UmNZjE8FSJxa",
                                      "-sporkaddr=ycbRQWbovrhQMTuxg9p4LAuW5SCMAKqPrn",
                                      "-sporkaddr=yc5TGfcHYoLCrcbVy4umsiDjsYUn39vLui",
                                      "-minsporkkeys=3"]))
        # connect nodes at start
        for i in range(0, 5):
            for j in range(i, 5):
                connect_nodes(self.nodes[i], j)

    def get_test_spork_state(self, node):
        info = node.spork('show')
        # use InstantSend spork for tests
        return info['SPORK_2_INSTANTSEND_ENABLED']

    def set_test_spork_state(self, node, value):
        # use InstantSend spork for tests
        node.spork('SPORK_2_INSTANTSEND_ENABLED', value)

    def wait_for_test_spork_state(self, node, value):
        start = time()
        got_state = False
        while True:
            if self.get_test_spork_state(node) == value:
                got_state = True
                break
            if time() > start + 10:
                break
            sleep(0.1)
        return got_state

    def run_test(self):
        # check test spork default state
        for node in self.nodes:
            assert(self.get_test_spork_state(node) == 0)

        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        # first and second signers set spork value
        self.set_test_spork_state(self.nodes[0], 1)
        self.set_test_spork_state(self.nodes[1], 1)
        # spork change requires at least 3 signers
        for node in self.nodes:
            assert(not self.wait_for_test_spork_state(node, 1))

        # third signer set spork value
        self.set_test_spork_state(self.nodes[2], 1)
        # now spork state is changed
        for node in self.nodes:
            assert(self.wait_for_test_spork_state(node, 1))

        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        # now set the spork again with other signers to test
        # old and new spork messages interaction
        self.set_test_spork_state(self.nodes[2], 2)
        self.set_test_spork_state(self.nodes[3], 2)
        self.set_test_spork_state(self.nodes[4], 2)
        for node in self.nodes:
            assert(self.wait_for_test_spork_state(node, 2))


if __name__ == '__main__':
    MultiKeySporkTest().main()
