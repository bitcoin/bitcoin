#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time

from test_framework.test_framework import DashTestFramework

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
        # secret(base58): cP8CHyqJpBot9W1eVZAfH5jhxACUJbTJrZo5ugyZTudpwtNYStWZ
        # address(base58): mjsQJdCwq2vhZrqLYpfvY2HXM8Gsc5s74v

        # secret(base58): cTDfsTGAjDyPEEhSP555H1R7T4NxAgm23vt8nrJpF9pK4fkohadX
        # address(base58): mwVjo3D1KJ3AfzekUCQwqJvgm9NJi1MiZT

        # secret(base58): cQadsocTQLUdBc7k8FsS7QNJB5vGe7K4w4RoHPi8jHAfCCn7dtXn
        # address(base58): mkV6NH5NrxZU5mpNzMCTLPqekUqtK9ApHm

        # secret(base58): cQCNNUK9L5NkqrGvghpYdDdjwW4tFg7WPp9WQEGvcdJW85MCr19B
        # address(base58): mkPTsmKU99N7QGHTpKZ39LVPSiyHE9PdGe

        # secret(base58): cNUqCKzN5UR8wPQscQvufcuysBo9mU3TYUsZuNUZFox2rtw5dkC4
        # address(base58): mfwRkVL3upN5Dm69WwG8fuPQMKqUAY2JC8

        self.add_nodes(5)

        self.start_node(0, ["-sporkkey=cP8CHyqJpBot9W1eVZAfH5jhxACUJbTJrZo5ugyZTudpwtNYStWZ",
                            "-sporkaddr=mkV6NH5NrxZU5mpNzMCTLPqekUqtK9ApHm",
                            "-sporkaddr=mwVjo3D1KJ3AfzekUCQwqJvgm9NJi1MiZT",
                            "-sporkaddr=mkPTsmKU99N7QGHTpKZ39LVPSiyHE9PdGe",
                            "-sporkaddr=mjsQJdCwq2vhZrqLYpfvY2HXM8Gsc5s74v",
                            "-sporkaddr=mfwRkVL3upN5Dm69WwG8fuPQMKqUAY2JC8",
                            "-minsporkkeys=3"])
        self.start_node(1, ["-sporkkey=cTDfsTGAjDyPEEhSP555H1R7T4NxAgm23vt8nrJpF9pK4fkohadX",
                            "-sporkaddr=mkV6NH5NrxZU5mpNzMCTLPqekUqtK9ApHm",
                            "-sporkaddr=mwVjo3D1KJ3AfzekUCQwqJvgm9NJi1MiZT",
                            "-sporkaddr=mkPTsmKU99N7QGHTpKZ39LVPSiyHE9PdGe",
                            "-sporkaddr=mjsQJdCwq2vhZrqLYpfvY2HXM8Gsc5s74v",
                            "-sporkaddr=mfwRkVL3upN5Dm69WwG8fuPQMKqUAY2JC8",
                            "-minsporkkeys=3"])
        self.start_node(2, ["-sporkkey=cQadsocTQLUdBc7k8FsS7QNJB5vGe7K4w4RoHPi8jHAfCCn7dtXn",
                            "-sporkaddr=mkV6NH5NrxZU5mpNzMCTLPqekUqtK9ApHm",
                            "-sporkaddr=mwVjo3D1KJ3AfzekUCQwqJvgm9NJi1MiZT",
                            "-sporkaddr=mkPTsmKU99N7QGHTpKZ39LVPSiyHE9PdGe",
                            "-sporkaddr=mjsQJdCwq2vhZrqLYpfvY2HXM8Gsc5s74v",
                            "-sporkaddr=mfwRkVL3upN5Dm69WwG8fuPQMKqUAY2JC8",
                            "-minsporkkeys=3"])
        self.start_node(3, ["-sporkkey=cQCNNUK9L5NkqrGvghpYdDdjwW4tFg7WPp9WQEGvcdJW85MCr19B",
                            "-sporkaddr=mkV6NH5NrxZU5mpNzMCTLPqekUqtK9ApHm",
                            "-sporkaddr=mwVjo3D1KJ3AfzekUCQwqJvgm9NJi1MiZT",
                            "-sporkaddr=mkPTsmKU99N7QGHTpKZ39LVPSiyHE9PdGe",
                            "-sporkaddr=mjsQJdCwq2vhZrqLYpfvY2HXM8Gsc5s74v",
                            "-sporkaddr=mfwRkVL3upN5Dm69WwG8fuPQMKqUAY2JC8",
                            "-minsporkkeys=3"])
        self.start_node(4, ["-sporkkey=cNUqCKzN5UR8wPQscQvufcuysBo9mU3TYUsZuNUZFox2rtw5dkC4",
                            "-sporkaddr=mkV6NH5NrxZU5mpNzMCTLPqekUqtK9ApHm",
                            "-sporkaddr=mwVjo3D1KJ3AfzekUCQwqJvgm9NJi1MiZT",
                            "-sporkaddr=mkPTsmKU99N7QGHTpKZ39LVPSiyHE9PdGe",
                            "-sporkaddr=mjsQJdCwq2vhZrqLYpfvY2HXM8Gsc5s74v",
                            "-sporkaddr=mfwRkVL3upN5Dm69WwG8fuPQMKqUAY2JC8",
                            "-minsporkkeys=3"])
        # connect nodes at start
        for i in range(0, 5):
            for j in range(i, 5):
                self.connect_nodes(i, j)

    def get_test_spork_value(self, node):
        info = node.spork('show')
        self.bump_mocktime(5)
        # use SB spork for tests
        return info['SPORK_TEST']

    def set_test_spork_value(self, node, value):
        # use SB spork for tests
        node.spork('SPORK_TEST', value)

    def run_test(self):
        # check test spork default state
        for node in self.nodes:
            assert(self.get_test_spork_value(node) == 4070908800)

        self.bump_mocktime(1)
        # first and second signers set spork value
        self.set_test_spork_value(self.nodes[0], 1)
        self.set_test_spork_value(self.nodes[1], 1)
        # spork change requires at least 3 signers
        time.sleep(10)
        for node in self.nodes:
            assert(self.get_test_spork_value(node) != 1)

        # third signer set spork value
        self.set_test_spork_value(self.nodes[2], 1)
        # now spork state is changed
        for node in self.nodes:
            time.sleep(0.1)
            self.wait_until(lambda: self.get_test_spork_value(node) == 1, timeout=10)

        self.bump_mocktime(1)
        # now set the spork again with other signers to test
        # old and new spork messages interaction
        self.set_test_spork_value(self.nodes[2], 2)
        self.set_test_spork_value(self.nodes[3], 2)
        self.set_test_spork_value(self.nodes[4], 2)
        time.sleep(0.1)
        for node in self.nodes:
            self.wait_until(lambda: self.get_test_spork_value(node) == 2, timeout=10)


if __name__ == '__main__':
    MultiKeySporkTest().main()
