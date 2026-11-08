#!/usr/bin/env python3
# Copyright (c) 2016-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Wallet commands for signing and verifying messages."""

import base64
from io import BytesIO
import json
from pathlib import Path

from test_framework.address import (
    base58_to_byte,
    byte_to_base58,
)
from test_framework.descriptors import descsum_create
from test_framework.key import (
    ECKey,
    H_POINT,
)
from test_framework.messages import deser_string_vector
from test_framework.script import (
    CScript,
    OP_CHECKSIG,
)
from test_framework.segwit_addr import decode_segwit_address
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class SignMessagesWithAddressTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.wallet_names = [self.default_wallet_name]
        self.extra_args = [["-addresstype=legacy"], ["-disablewallet"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()

    def import_descriptor(self, wallet, descriptor):
        descriptor = descsum_create(descriptor)
        assert wallet.importdescriptors([{'desc': descriptor, 'timestamp': 'now'}])[0]['success']
        return wallet.deriveaddresses(descriptor)[0]

    def sign_and_verify(self, wallet, address, message):
        proof = wallet.signmessage(address, message)
        assert proof.startswith('smp')
        verifier = self.nodes[1]
        for signature in [proof, proof[3:]]:
            assert verifier.verifymessage(address, signature, message)
            assert not verifier.verifymessage(address, signature, message + '!')
        return deser_string_vector(BytesIO(base64.b64decode(proof[3:], validate=True)))

    def test_descriptors(self):
        node = self.nodes[0]
        wallet = node.get_wallet_rpc(self.default_wallet_name)
        verifier = self.nodes[1]
        fixture = Path(self.config['environment']['SRCDIR']) / 'src/test/data/bip322_vectors.json'
        vectors = json.loads(fixture.read_text(encoding='utf-8'))
        messages = ['', vectors['basic']['tx_hashes'][2]['message']]
        addresses = []

        self.log.info('Import published keys with only the WIF network prefix changed')
        for row in vectors['basic']['simple']:
            if row['message'] == 'Hello World':
                continue  # Same descriptor as the empty-message row.
            keys = []
            pubkeys = []
            for wif in row['private_keys']:
                key_bytes, version = base58_to_byte(wif)
                assert_equal(version, 128)
                key = byte_to_base58(key_bytes, 239)
                assert_equal(base58_to_byte(key), (key_bytes, 239))
                keys.append(key)
                secret = ECKey()
                assert_equal(key_bytes[-1], 1)
                secret.set(key_bytes[:-1], compressed=True)
                pubkeys.append(secret.get_pubkey().get_bytes())
            if row['type'] == 'p2wpkh':
                descriptor = f'wpkh({keys[0]})'
            elif row['type'] == 'p2tr':
                descriptor = f'tr({keys[0]})'
            else:
                script = CScript(bytes.fromhex(row['witness_script']))
                ordered_keys = [keys[pubkeys.index(op)] for op in script if isinstance(op, bytes)]
                descriptor = f'wsh(multi(3,{",".join(ordered_keys)}))'
            address = self.import_descriptor(wallet, descriptor)
            version, program = decode_segwit_address('bc', row['address'])
            assert_equal(decode_segwit_address('bcrt', address), (version, program))
            for proof in row['bip322_signatures']:
                assert verifier.verifymessage(address, proof, row['message'])
            for message in dict.fromkeys([row['message'], *messages]):
                witness = self.sign_and_verify(wallet, address, message)
                if row['type'].startswith('p2wsh'):
                    assert_equal(witness[-1].hex(), row['witness_script'])
                else:
                    assert_equal(len(witness), 2 if row['type'] == 'p2wpkh' else 1)
            addresses.append(address)

        self.log.info('Sign P2WSH 2-of-3 with exactly two private keys; fail below the threshold')
        secrets = vectors['basic']['simple'][2]['private_keys']
        keys = [byte_to_base58(base58_to_byte(key)[0], 239) for key in secrets]
        pubkeys = []
        for wif in secrets:
            secret = ECKey()
            secret.set(base58_to_byte(wif)[0][:-1], compressed=True)
            pubkeys.append(secret.get_pubkey().get_bytes().hex())
        node.createwallet(wallet_name='threshold', blank=True)
        threshold = node.get_wallet_rpc('threshold')
        address = self.import_descriptor(threshold, f'wsh(multi(2,{keys[0]},{keys[1]},{pubkeys[2]}))')
        witness = self.sign_and_verify(threshold, address, messages[1])
        assert_equal(len(witness), 4)
        assert_equal(witness[0], b'')
        node.createwallet(wallet_name='incomplete', blank=True)
        incomplete = node.get_wallet_rpc('incomplete')
        assert_equal(self.import_descriptor(incomplete, f'wsh(multi(2,{keys[0]},{pubkeys[1]},{pubkeys[2]}))'), address)
        assert_raises_rpc_error(-4, 'Private key not available', incomplete.signmessage, address, messages[1])

        self.log.info('Use a Taproot key path with a committed tree and no available leaf key')
        tree_address = self.import_descriptor(threshold, f'tr({keys[1]},pk({H_POINT}))')
        assert_equal(len(self.sign_and_verify(threshold, tree_address, messages[1])), 1)

        self.log.info('Force a Taproot script path using an internal key with no known private key')
        descriptor = f'tr({H_POINT},pk({keys[0]}))'
        script_address = self.import_descriptor(threshold, descriptor)
        for message in messages:
            witness = self.sign_and_verify(threshold, script_address, message)
            assert_equal(len(witness), 3)
            assert_equal(witness[1], CScript([bytes.fromhex(pubkeys[0])[1:], OP_CHECKSIG]))
            assert_equal(len(witness[2]), 33)
            assert_equal(witness[2][0] & 0xfe, 0xc0)
            assert_equal(witness[2][1:].hex(), H_POINT)

        self.log.info('Missing private keys, witness scripts and unknown wallet destinations fail')
        node.createwallet(wallet_name='watch', disable_private_keys=True, blank=True)
        watch = node.get_wallet_rpc('watch')
        watch_address = self.import_descriptor(watch, f'wpkh({pubkeys[0]})')
        assert_raises_rpc_error(-4, 'Private key not available', watch.signmessage, watch_address, '')
        self.import_descriptor(watch, f'addr({address})')
        assert_raises_rpc_error(-4, 'Private key not available', watch.signmessage, address, '')
        assert_raises_rpc_error(-4, 'Private key not available', wallet.signmessage, script_address, '')

        self.log.info('Encrypted wallet signing fails while locked and succeeds after unlock for all three types')
        wallet.encryptwallet('message passphrase')
        for address in addresses:
            assert_raises_rpc_error(-13, 'Please enter the wallet passphrase', wallet.signmessage, address, '')
        wallet.walletpassphrase('message passphrase', 60)
        for address in addresses:
            self.sign_and_verify(wallet, address, messages[1])
        wallet.walletlock()
        for address in addresses:
            assert_raises_rpc_error(-13, 'Please enter the wallet passphrase', wallet.signmessage, address, '')
        for tested_wallet in [wallet, threshold, incomplete, watch]:
            assert_equal(tested_wallet.getwalletinfo()['txcount'], 0)
            assert_equal(tested_wallet.getbalances()['mine']['trusted'], 0)
        assert_equal(verifier.getblockcount(), 0)

    def run_test(self):
        message = 'This is just a test message'

        self.log.info('test signing with an address with wallet')
        address = self.nodes[0].getnewaddress()
        signature = self.nodes[0].signmessage(address, message)
        assert self.nodes[1].verifymessage(address, signature, message)

        self.log.info('test verifying with another address should not work')
        other_address = self.nodes[0].getnewaddress()
        other_signature = self.nodes[0].signmessage(other_address, message)
        assert not self.nodes[1].verifymessage(other_address, signature, message)
        assert not self.nodes[1].verifymessage(address, other_signature, message)

        self.log.info('test native witness signing dispatch')
        for address_type in ['bech32', 'bech32m']:
            witness_address = self.nodes[0].getnewaddress(address_type=address_type)
            proof = self.nodes[0].signmessage(witness_address, message)
            assert proof.startswith('smp')
            assert self.nodes[1].verifymessage(witness_address, proof, message)
            assert not self.nodes[1].verifymessage(witness_address, proof, message + '!')
            assert not self.nodes[1].verifymessage(self.nodes[0].getnewaddress(address_type=address_type), proof, message)

        key = self.nodes[0].get_deterministic_priv_key().key
        descriptor = descsum_create(f'wsh(pk({key}))')
        assert self.nodes[0].importdescriptors([{'desc': descriptor, 'timestamp': 'now'}])[0]['success']
        witness_address = self.nodes[0].deriveaddresses(descriptor)[0]
        proof = self.nodes[0].signmessage(witness_address, message)
        assert proof.startswith('smp')
        assert self.nodes[1].verifymessage(witness_address, proof, message)

        self.log.info('test nested SegWit signing remains unsupported')
        nested_address = self.nodes[0].getnewaddress(address_type='p2sh-segwit')
        assert_raises_rpc_error(-3, "Address does not refer to key", self.nodes[0].signmessage, nested_address, message)

        self.log.info('test parameter validity and error codes')
        # signmessage has two required parameters
        for num_params in [0, 1, 3, 4, 5]:
            param_list = ["dummy"]*num_params
            assert_raises_rpc_error(-1, "signmessage", self.nodes[0].signmessage, *param_list)
        # invalid key or address provided
        assert_raises_rpc_error(-5, "Invalid address", self.nodes[0].signmessage, "invalid_addr", message)

        self.test_descriptors()


if __name__ == '__main__':
    SignMessagesWithAddressTest(__file__).main()
