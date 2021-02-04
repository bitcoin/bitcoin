#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test deprecation of RPC getaddressinfo `labels` returning an array
containing a JSON object of `name` and purpose` key-value pairs. It now
returns an array containing only the label name.

"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

LABELS_TO_TEST = frozenset({"" , "New 𝅘𝅥𝅯 $<#>&!рыба Label"})

class GetAddressInfoLabelsPurposeDeprecationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        # Start node[0] with -deprecatedrpc=labelspurpose and node[1] without.
        self.extra_args = [["-deprecatedrpc=labelspurpose"], []]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_labels(self, node_num, label_name, expected_value):
        node = self.nodes[node_num]
        address = node.getnewaddress()
        if label_name != "":
            node.setlabel(address, label_name)
            self.log.info("  set label to {}".format(label_name))
        labels = node.getaddressinfo(address)["labels"]
        self.log.info("  labels = {}".format(labels))
        assert_equal(labels, expected_value)

    def run_test(self):
        """Test getaddressinfo labels with and without -deprecatedrpc flag."""
        self.log.info("Test getaddressinfo labels with -deprecatedrpc flag")
        for label in LABELS_TO_TEST:
            self.test_labels(node_num=0, label_name=label, expected_value=[{"name": label, "purpose": "receive"}])

        self.log.info("Test getaddressinfo labels without -deprecatedrpc flag")
        for label in LABELS_TO_TEST:
            self.test_labels(node_num=1, label_name=label, expected_value=[label])


if __name__ == '__main__':
    GetAddressInfoLabelsPurposeDeprecationTest().main()
