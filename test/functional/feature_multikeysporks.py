#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time

from test_framework.test_framework import DashTestFramework
from test_framework.util import (
  wait_until_helper,
)
'''
multikeysporks.py

Test logic for several signer keys usage for spork broadcast.

We set 5 possible keys for sporks signing and set minimum
required signers to 3. We check 1 and 2 signers can't set the spork
value, any 3 signers can change spork value and other 3 signers
can change it again.
'''


class MultiKeySporkTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(5, 0)

    def setup_network(self):
        # secret(base58): cP97oFnFzebdETAUqSRjcCnX4SXPNpHNd9pKJNK2TtcXsvcg4Tud
        # address(base58): bcrt1qchfpkuacm49j8slktvlf59wzm0klm9v6rzpx2r

        # secret(base58): cR5XZHPB5GQ2Hc1puLJ71GYKpc1aruGSrEm98L8avFuF8TWVrG6U
        # address(base58): bcrt1qx4agkazzwk8edh92vvc744tljfjhgd66gqq443

        # secret(base58): cManVSyerUMhiAC3ZBM4uVsPYmYjRyX189K2yGhJdyQEJ5qcCqBh
        # address(base58): bcrt1q8p6r3sr3lyfe0jemrvwx7cgzdue80v2gzt6he8

        # secret(base58): cR1cvMjvmcafKf25JSnpckWcJCYui175oSDUmua4B1GwYm5fUXVh
        # address(base58): bcrt1qh784d4jq8yzccs6fu49m0d2f2aqwjxydttsyt6

        # secret(base58): cNsswxWq6uc9kXWVXhwdyMV2ANXswnc8CdPjQ59oDx9kfotAY1Rd
        # address(base58): bcrt1qg07pa269esevftkecygkvx20svyn7uptscwmd3

        self.add_nodes(5)

        spork_chain_params =   ["-sporkaddr=bcrt1qchfpkuacm49j8slktvlf59wzm0klm9v6rzpx2r",
                                "-sporkaddr=bcrt1qx4agkazzwk8edh92vvc744tljfjhgd66gqq443",
                                "-sporkaddr=bcrt1q8p6r3sr3lyfe0jemrvwx7cgzdue80v2gzt6he8",
                                "-sporkaddr=bcrt1qh784d4jq8yzccs6fu49m0d2f2aqwjxydttsyt6",
                                "-sporkaddr=bcrt1qg07pa269esevftkecygkvx20svyn7uptscwmd3",
                                "-minsporkkeys=3"]

        # Node0 extra args to use on normal node restarts
        self.node0_extra_args = ["-sporkkey=cP97oFnFzebdETAUqSRjcCnX4SXPNpHNd9pKJNK2TtcXsvcg4Tud"] + spork_chain_params

        self.start_node(0, self.node0_extra_args)
        self.start_node(1, ["-sporkkey=cR5XZHPB5GQ2Hc1puLJ71GYKpc1aruGSrEm98L8avFuF8TWVrG6U"] + spork_chain_params)
        self.start_node(2, ["-sporkkey=cManVSyerUMhiAC3ZBM4uVsPYmYjRyX189K2yGhJdyQEJ5qcCqBh"] + spork_chain_params)
        self.start_node(3, ["-sporkkey=cR1cvMjvmcafKf25JSnpckWcJCYui175oSDUmua4B1GwYm5fUXVh"] + spork_chain_params)
        self.start_node(4, ["-sporkkey=cNsswxWq6uc9kXWVXhwdyMV2ANXswnc8CdPjQ59oDx9kfotAY1Rd"] + spork_chain_params)

        # connect nodes at start
        for i in range(0, 5):
            for j in range(i, 5):
                self.connect_nodes(i, j)

    def get_test_spork_value(self, node, spork_name):
        time.sleep(0.1)
        self.bump_mocktime(6)  # advance ProcessTick
        info = node.spork('show')
        # use spork for tests
        return info[spork_name]

    def test_spork(self, spork_name, final_value):
        # check test spork default state
        for node in self.nodes:
            assert self.get_test_spork_value(node, spork_name) == 4070908800

        self.bump_mocktime(1)
        # first and second signers set spork value
        self.nodes[0].spork(spork_name, 1)
        self.nodes[1].spork(spork_name, 1)
        self.bump_mocktime(5)
        # spork change requires at least 3 signers
        time.sleep(10)
        for node in self.nodes:
            assert self.get_test_spork_value(node, spork_name) != 1

        # restart with no extra args to trigger CheckAndRemove
        self.restart_node(0)
        assert self.get_test_spork_value(self.nodes[0], spork_name) != 1

        # restart again with correct_params, should resync spork parts from other nodes
        self.restart_node(0, self.node0_extra_args)
        for i in range(1, 5):
            self.connect_nodes(0, i)

        # third signer set spork value
        self.nodes[2].spork(spork_name, 1)
        self.bump_mocktime(6)
        time.sleep(5)
        # now spork state is changed
        for node in self.nodes:
            wait_until_helper(lambda: self.get_test_spork_value(node, spork_name) == 1, sleep=0.1, timeout=10)

        # restart with no extra args to trigger CheckAndRemove, should reset the spork back to its default
        self.restart_node(0)
        assert self.get_test_spork_value(self.nodes[0], spork_name) == 4070908800

        # restart again with correct_params, should resync sporks from other nodes
        self.restart_node(0, self.node0_extra_args)
        time.sleep(5)
        self.bump_mocktime(6)
        for i in range(1, 5):
            self.connect_nodes(0, i)

        wait_until_helper(lambda: self.get_test_spork_value(self.nodes[0], spork_name) == 1, sleep=0.1, timeout=10)

        self.bump_mocktime(1)
        # now set the spork again with other signers to test
        # old and new spork messages interaction
        self.nodes[2].spork(spork_name, final_value)
        self.nodes[3].spork(spork_name, final_value)
        self.nodes[4].spork(spork_name, final_value)
        for node in self.nodes:
            wait_until_helper(lambda: self.get_test_spork_value(node, spork_name) == final_value, sleep=0.1, timeout=10)

    def run_test(self):
        self.test_spork('SPORK_TEST', 2)
        self.test_spork('SPORK_TEST1', 3)
        for node in self.nodes:
            assert self.get_test_spork_value(node, 'SPORK_TEST') == 2
            assert self.get_test_spork_value(node, 'SPORK_TEST1') == 3


if __name__ == '__main__':
    MultiKeySporkTest().main()
