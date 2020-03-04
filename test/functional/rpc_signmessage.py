#!/usr/bin/env python3
# Copyright (c) 2016-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC commands for signing and verifying messages."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class SignMessagesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-addresstype=bech32"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        message = 'This is just a test message'

        self.log.info('test signing with priv_key')
        self.log.info('test signing with priv_key with bech32')
        priv_key = 'cVbpmtsSXkBJhSZakwBSUY7jxhUgXZeT4hCAGyKixccRZwMG4jrf'
        address = 'bcrt1qf2yvxk355rc6pqgprrn7q4d3lctr22ta3ne08w'
        signature = self.nodes[0].signmessagewithprivkey(priv_key, message, 'bech32')
        assert self.nodes[0].verifymessage(address, signature, message)
        assert_equal("OK", self.nodes[0].verifymessage(address, signature, message, 1))

        self.log.info('test legacy key signing')
        priv_key = 'cUeKHd5orzT3mz8P9pxyREHfsWtVfgsfDjiZZBcjUBAaGk1BTj7N'
        address = 'mpLQjfK79b7CCV4VMJWEWAj5Mpx8Up5zxB'
        expected_signature = 'INbVnW4e6PeRmsv2Qgu8NuopvrVjkcxob+sX8OcZG0SALhWybUjzMLPdAsXI46YZGb0KQTRii+wWIQzRpG/U+S0='
        signature = self.nodes[0].signmessagewithprivkey(priv_key, message)
        assert_equal(expected_signature, signature)
        assert self.nodes[0].verifymessage(address, signature, message)
        assert_equal("OK", self.nodes[0].verifymessage(address, signature, message, 1))

        self.log.info('test signing with an address with wallet')
        address = self.nodes[0].getnewaddress()
        signature = self.nodes[0].signmessage(address, message)
        assert self.nodes[0].verifymessage(address, signature, message)
        assert_equal("OK", self.nodes[0].verifymessage(address, signature, message, 1))

        self.log.info('test verifying with another address should not work')
        other_address = self.nodes[0].getnewaddress()
        legacy_address = self.nodes[0].getnewaddress(address_type='legacy')
        other_signature = self.nodes[0].signmessage(other_address, message)
        legacy_signature = self.nodes[0].signmessage(legacy_address, message)
        assert not self.nodes[0].verifymessage(other_address, signature, message)
        assert not self.nodes[0].verifymessage(address, other_signature, message)
        assert not self.nodes[0].verifymessage(legacy_address, signature, message)
        assert not self.nodes[0].verifymessage(address, legacy_signature, message)
        assert_equal("ERR_MALFORMED_SIGNATURE", self.nodes[0].verifymessage(other_address, signature, message, 1))
        assert_equal("ERR_MALFORMED_SIGNATURE", self.nodes[0].verifymessage(address, other_signature, message, 1))
        assert_equal("ERR_MALFORMED_SIGNATURE", self.nodes[0].verifymessage(legacy_address, signature, message, 1))
        # We get an error here, because the verifier believes the proof should be a BIP-322 proof, due to
        # the address not being P2PKH, but the proof is a legacy proof
        assert_equal("ERR_MALFORMED_SIGNATURE", self.nodes[0].verifymessage(address, legacy_signature, message, 1))

        self.log.info('test signing with a legacy address with wallet')
        address = self.nodes[0].getnewaddress(address_type='legacy')
        signature = self.nodes[0].signmessage(address, message)
        assert self.nodes[0].verifymessage(address, signature, message)
        assert_equal("OK", self.nodes[0].verifymessage(address, signature, message, 1))

        self.log.info('test verifying legacy proof with another address should not work')
        other_address = self.nodes[0].getnewaddress()
        legacy_address = self.nodes[0].getnewaddress(address_type='legacy')
        other_signature = self.nodes[0].signmessage(other_address, message)
        legacy_signature = self.nodes[0].signmessage(legacy_address, message)
        assert not self.nodes[0].verifymessage(other_address, signature, message)
        assert not self.nodes[0].verifymessage(address, other_signature, message)
        assert not self.nodes[0].verifymessage(legacy_address, signature, message)
        assert not self.nodes[0].verifymessage(address, legacy_signature, message)
        # We get an error here, because the verifier believes the proof should be a BIP-322 proof, due to
        # the address not being P2PKH, but the proof is a legacy proof
        assert_equal("ERR_MALFORMED_SIGNATURE", self.nodes[0].verifymessage(other_address, signature, message, 1))
        assert_equal("ERR_MALFORMED_SIGNATURE", self.nodes[0].verifymessage(address, other_signature, message, 1))
        assert_equal("ERR_MALFORMED_SIGNATURE", self.nodes[0].verifymessage(legacy_address, signature, message, 1))
        assert_equal("ERR_MALFORMED_SIGNATURE", self.nodes[0].verifymessage(address, legacy_signature, message, 1))

if __name__ == '__main__':
    SignMessagesTest().main()
