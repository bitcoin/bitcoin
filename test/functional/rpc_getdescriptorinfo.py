#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test getdescriptorinfo RPC.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.descriptors import descsum_create
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class DescriptorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-disablewallet"]]
        self.wallet_names = []

    def test_desc(self, desc, isrange, issolvable, hasprivatekeys):
        info = self.nodes[0].getdescriptorinfo(desc)
        assert_equal(info, self.nodes[0].getdescriptorinfo(descsum_create(desc)))
        assert_equal(info['descriptor'], descsum_create(desc))
        assert_equal(info['isrange'], isrange)
        assert_equal(info['issolvable'], issolvable)
        assert_equal(info['hasprivatekeys'], hasprivatekeys)

    def run_test(self):
        assert_raises_rpc_error(-1, 'getdescriptorinfo', self.nodes[0].getdescriptorinfo)
        assert_raises_rpc_error(-3, 'Expected type string', self.nodes[0].getdescriptorinfo, 1)
        assert_raises_rpc_error(-5, 'is not a valid descriptor function', self.nodes[0].getdescriptorinfo, '')

        # P2PK output with the specified public key.
        self.test_desc('pk(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)', isrange=False, issolvable=True, hasprivatekeys=False)
        # P2PKH output with the specified public key.
        self.test_desc('pkh(02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5)', isrange=False, issolvable=True, hasprivatekeys=False)
        # P2SH-P2PKH output with the specified public key.
        self.test_desc('sh(pkh(03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556))', isrange=False, issolvable=True, hasprivatekeys=False)
        # Any P2PK, P2PKH, or P2SH-P2PKH output with the specified public key.
        self.test_desc('combo(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)', isrange=False, issolvable=True, hasprivatekeys=False)
        # A bare *1-of-2* multisig output with keys in the specified order.
        self.test_desc('multi(1,022f8bde4d1a07209355b4a7250a5c5128e88b84bddc619ab7cba8d569b240efe4,025cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc)', isrange=False, issolvable=True, hasprivatekeys=False)
        # A P2SH *2-of-2* multisig output with keys in the specified order.
        self.test_desc('sh(multi(2,022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01,03acd484e2f0c7f65309ad178a9f559abde09796974c57e714c35f110dfc27ccbe))', isrange=False, issolvable=True, hasprivatekeys=False)
        # A P2WSH *2-of-3* multisig output with keys in the specified order.
        self.test_desc('sh(multi(2,03a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7,03774ae7f858a9411e5ef4246b70c65aac5649980be5c17891bbec17895da008cb,03d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a))', isrange=False, issolvable=True, hasprivatekeys=False)
        # A P2SH-P2SH *1-of-3* multisig output with keys in the specified order.
        self.test_desc('sh(multi(1,03f28773c2d975288bc7d1d205c3748651b075fbc6610e58cddeeddf8f19405aa8,03499fdf9e895e719cfd64e67f07d38e3226aa7b63678949e6e49b241a60e823e4,02d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e))', isrange=False, issolvable=True, hasprivatekeys=False)
        # A P2PK output with the public key of the specified xpub.
        self.test_desc('pk(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B)', isrange=False, issolvable=True, hasprivatekeys=False)
        # A P2PKH output with child key *1'/2* of the specified xpub.
        self.test_desc("pkh(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1'/2)", isrange=False, issolvable=True, hasprivatekeys=False)
        # A set of P2PKH outputs, but additionally specifies that the specified xpub is a child of a master with fingerprint `d34db33f`, and derived using path `44'/0'/0'`.
        self.test_desc("pkh([d34db33f/44'/0'/0']tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/*)", isrange=True, issolvable=True, hasprivatekeys=False)
        # A set of *1-of-2* P2SH multisig outputs where the first multisig key is the *1/0/`i`* child of the first specified xpub and the second multisig key is the *0/0/`i`* child of the second specified xpub, and `i` is any number in a configurable range (`0-1000` by default).
        self.test_desc("sh(multi(1,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/0/*,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/0/0/*))", isrange=True, issolvable=True, hasprivatekeys=False)


if __name__ == '__main__':
    DescriptorTest().main()
