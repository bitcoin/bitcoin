#!/usr/bin/env python3
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the deriveaddresses rpc call."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.descriptors import descsum_create
from test_framework.util import assert_equal, assert_raises_rpc_error

class DeriveaddressesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        assert_raises_rpc_error(-5, "Missing checksum", self.nodes[0].deriveaddresses, "a")

        descriptor = "wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/0)#t6wfjs64"
        address = "rltc1qjqmxmkpmxt80xz4y3746zgt0q3u3ferr0c8362"
        assert_equal(self.nodes[0].deriveaddresses(descriptor), [address])

        descriptor = descriptor[:-9]
        assert_raises_rpc_error(-5, "Missing checksum", self.nodes[0].deriveaddresses, descriptor)

        descriptor_pubkey = "wpkh(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/0)#s9ga3alw"
        address = "rltc1qjqmxmkpmxt80xz4y3746zgt0q3u3ferr0c8362"
        assert_equal(self.nodes[0].deriveaddresses(descriptor_pubkey), [address])

        ranged_descriptor = "wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)#kft60nuy"
        assert_equal(self.nodes[0].deriveaddresses(ranged_descriptor, [1, 2]), ["rltc1qhku5rq7jz8ulufe2y6fkcpnlvpsta7rqtc0r66", "rltc1qpgptk2gvshyl0s9lqshsmx932l9ccsv2ye3927"])
        assert_equal(self.nodes[0].deriveaddresses(ranged_descriptor, 2), [address, "rltc1qhku5rq7jz8ulufe2y6fkcpnlvpsta7rqtc0r66", "rltc1qpgptk2gvshyl0s9lqshsmx932l9ccsv2ye3927"])

        assert_raises_rpc_error(-8, "Range should not be specified for an un-ranged descriptor", self.nodes[0].deriveaddresses, descsum_create("wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/0)"), [0, 2])

        assert_raises_rpc_error(-8, "Range must be specified for a ranged descriptor", self.nodes[0].deriveaddresses, descsum_create("wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)"))

        assert_raises_rpc_error(-8, "End of range is too high", self.nodes[0].deriveaddresses, descsum_create("wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)"), 10000000000)

        assert_raises_rpc_error(-8, "Range is too large", self.nodes[0].deriveaddresses, descsum_create("wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)"), [1000000000, 2000000000])

        assert_raises_rpc_error(-8, "Range specified as [begin,end] must not have begin after end", self.nodes[0].deriveaddresses, descsum_create("wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)"), [2, 0])

        assert_raises_rpc_error(-8, "Range should be greater or equal than 0", self.nodes[0].deriveaddresses, descsum_create("wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)"), [-1, 0])

        combo_descriptor = descsum_create("combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/0)")
        assert_equal(self.nodes[0].deriveaddresses(combo_descriptor), ["mtfUoUax9L4tzXARpw1oTGxWyoogp52KhJ", "mtfUoUax9L4tzXARpw1oTGxWyoogp52KhJ", address, "QhH14HY4FwMbpAN3SRRJKixiC8PdFg8qpP"])

        hardened_without_privkey_descriptor = descsum_create("wpkh(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1'/1/0)")
        assert_raises_rpc_error(-5, "Cannot derive script without private keys", self.nodes[0].deriveaddresses, hardened_without_privkey_descriptor)

        bare_multisig_descriptor = descsum_create("multi(1,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/0,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/1)")
        assert_raises_rpc_error(-5, "Descriptor does not have a corresponding address", self.nodes[0].deriveaddresses, bare_multisig_descriptor)

if __name__ == '__main__':
    DeriveaddressesTest().main()
