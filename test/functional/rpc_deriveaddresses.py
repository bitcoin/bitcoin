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
        address = "bcrt1qjqmxmkpmxt80xz4y3746zgt0q3u3ferr34acd5"
        assert_equal(self.nodes[0].deriveaddresses(descriptor), [address])

        descriptor = descriptor[:-9]
        assert_raises_rpc_error(-5, "Missing checksum", self.nodes[0].deriveaddresses, descriptor)

        descriptor_pubkey = "wpkh(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/0)#s9ga3alw"
        address = "bcrt1qjqmxmkpmxt80xz4y3746zgt0q3u3ferr34acd5"
        assert_equal(self.nodes[0].deriveaddresses(descriptor_pubkey), [address])

        res = self.nodes[0].deriveaddresses(descriptor=descriptor_pubkey, options={"details": True})
        assert_equal(res[0]['address'], address)
        assert_equal(res[0]['output_script'], '001490366dd83b32cef30aa48faba1216f047914e463')
        assert_equal(len(res[0]['public_keys']), 1)
        assert_equal(res[0]['public_keys'][0], '03e1c5b6e650966971d7e71ef2674f80222752740fc1dfd63bbbd220d2da9bd0fb')

        res = self.nodes[0].deriveaddresses(descriptor=descriptor + "#t6wfjs64", options={"details": True})
        assert_equal(res[0]['address'], address)
        assert_equal(res[0]['output_script'], '001490366dd83b32cef30aa48faba1216f047914e463')
        assert_equal(len(res[0]['public_keys']), 1)
        assert_equal(res[0]['public_keys'][0], '03e1c5b6e650966971d7e71ef2674f80222752740fc1dfd63bbbd220d2da9bd0fb')

        ranged_descriptor = "wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)#kft60nuy"
        assert_equal(self.nodes[0].deriveaddresses(ranged_descriptor, [1, 2]), ["bcrt1qhku5rq7jz8ulufe2y6fkcpnlvpsta7rq4442dy", "bcrt1qpgptk2gvshyl0s9lqshsmx932l9ccsv265tvaq"])
        assert_equal(self.nodes[0].deriveaddresses(ranged_descriptor, 2), [address, "bcrt1qhku5rq7jz8ulufe2y6fkcpnlvpsta7rq4442dy", "bcrt1qpgptk2gvshyl0s9lqshsmx932l9ccsv265tvaq"])

        res = self.nodes[0].deriveaddresses(ranged_descriptor, [1,2], {"details": True})
        assert_equal(res[0]['index'], 1)
        assert_equal(res[0]['address'], 'bcrt1qhku5rq7jz8ulufe2y6fkcpnlvpsta7rq4442dy')
        assert_equal(res[0]['output_script'], '0014bdb94183d211f9fe272a26936c067f6060bef860')
        assert_equal(len(res[0]['public_keys']), 1)
        assert_equal(res[0]['public_keys'][0], '030d820fc9e8211c4169be8530efbc632775d8286167afd178caaf1089b77daba7')
        assert_equal(res[1]['index'], 2)
        assert_equal(res[1]['address'], 'bcrt1qpgptk2gvshyl0s9lqshsmx932l9ccsv265tvaq')
        assert_equal(res[1]['output_script'], '00140a02bb290c85c9f7c0bf042f0d98b157cb8c418a')
        assert_equal(len(res[1]['public_keys']), 1)
        assert_equal(res[1]['public_keys'][0], '030d01adfad886db234d143b4d5d7f2abec365ae393407cb425e1f39fa3da694c0')

        ranged_descriptor = "wsh(multi(2,tprv8ZgxMBicQKsPePmENhT9N9yiSfTtDoC1f39P7nNmgEyCB6Nm4Qiv1muq4CykB9jtnQg2VitBrWh8PJU8LHzoGMHTrS2VKBSgAz7Ssjf9S3P/86'/1'/0'/0/*,tprv8ZgxMBicQKsPfMzHFsP1xhYbMGowTt7PPwkfsuBDhCN7rSnCTSConREDsnroDrtNtCCxzhGvhLYVCULPDtrzTV7w1wNWMPsBz2tbX3tKFRy/86'/1'/0'/0/*))#kjq88ke0"
        res = self.nodes[0].deriveaddresses(ranged_descriptor, 0, {"details": True})
        assert_equal(res[0]['index'], 0)
        assert_equal(res[0]['address'], 'bcrt1qupzqr3kdgn6zdjkemlfy5kytr42z96wsutjjylvj4qvwm2argvtshnh8gs')
        assert_equal(res[0]['output_script'], '0020e04401c6cd44f426cad9dfd24a588b1d5422e9d0e2e5227d92a818edaba34317')
        assert_equal(len(res[0]['public_keys']), 2)
        assert_equal(res[0]['public_keys'][0], '02557828307dcf41e15753ec4c7e1f7af149dd85f8e2a10b81bc81174f9129997b')
        assert_equal(res[0]['public_keys'][1], '03f1f8587257e595dd5a1699da22b749cb9bea5f5d0b9bccfb43f119092b1297de')

        ranged_descriptor = "wsh(multi(2,tprv8ZgxMBicQKsPePmENhT9N9yiSfTtDoC1f39P7nNmgEyCB6Nm4Qiv1muq4CykB9jtnQg2VitBrWh8PJU8LHzoGMHTrS2VKBSgAz7Ssjf9S3P/0/*,tpubDBYDcH8P2PedrEN3HxWYJJJMZEdgnrqMsjeKpPNzwe7jmGwk5M3HRdSf5vudAXwrJPfUsfvUPFooKWmz79Lh111U51RNotagXiGNeJe3i6t/1/*))#szn2yxkq"
        res = self.nodes[0].deriveaddresses(ranged_descriptor, 0, {"details": True})
        assert_equal(res[0]['index'], 0)
        assert_equal(res[0]['address'], 'bcrt1qqsat6c82fvdy73rfzye8f7nwxcz3xny7t56azl73g95mt3tmzvgsgyd282')
        assert_equal(res[0]['output_script'], '0020043abd60ea4b1a4f4469113274fa6e3605134c9e5d35d17fd14169b5c57b1311')
        assert_equal(len(res[0]['public_keys']), 2)
        assert_equal(res[0]['public_keys'][0], '0281c133371848d11ac195189bdfd32c1bbf39ebc1ee1b627897afd2ff8770e77b')
        assert_equal(res[0]['public_keys'][1], '03db7c6fb51e4386137e2a320c25a8681ebdc5130a902c7abb9fb94d7358f9ca70')

        assert_raises_rpc_error(-8, "Range should not be specified for an un-ranged descriptor", self.nodes[0].deriveaddresses, descsum_create("wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/0)"), [0, 2])

        assert_raises_rpc_error(-8, "Range must be specified for a ranged descriptor", self.nodes[0].deriveaddresses, descsum_create("wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)"))

        assert_raises_rpc_error(-8, "End of range is too high", self.nodes[0].deriveaddresses, descsum_create("wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)"), 10000000000)

        assert_raises_rpc_error(-8, "Range is too large", self.nodes[0].deriveaddresses, descsum_create("wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)"), [1000000000, 2000000000])

        assert_raises_rpc_error(-8, "Range specified as [begin,end] must not have begin after end", self.nodes[0].deriveaddresses, descsum_create("wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)"), [2, 0])

        assert_raises_rpc_error(-8, "Range should be greater or equal than 0", self.nodes[0].deriveaddresses, descsum_create("wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)"), [-1, 0])

        combo_descriptor = descsum_create("combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/0)")
        assert_equal(self.nodes[0].deriveaddresses(combo_descriptor), ["mtfUoUax9L4tzXARpw1oTGxWyoogp52KhJ", "mtfUoUax9L4tzXARpw1oTGxWyoogp52KhJ", address, "2NDvEwGfpEqJWfybzpKPHF2XH3jwoQV3D7x"])

        hardened_without_privkey_descriptor = descsum_create("wpkh(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1'/1/0)")
        assert_raises_rpc_error(-5, "Cannot derive script without private keys", self.nodes[0].deriveaddresses, hardened_without_privkey_descriptor)

        bare_multisig_descriptor = descsum_create("multi(1,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/0,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/1)")
        assert_raises_rpc_error(-5, "Descriptor does not have a corresponding address", self.nodes[0].deriveaddresses, bare_multisig_descriptor)

if __name__ == '__main__':
    DeriveaddressesTest().main()
