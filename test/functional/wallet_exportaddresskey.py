#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test address-bound key export, script membership, authorization and non-mutation.

All secrets are public synthetic fixtures. No private-descriptor export RPC is
used as an oracle.
"""
import hashlib
import hmac
import json
import os
import re
import sys

from test_framework.address import (
    base58_to_byte,
    byte_to_base58,
    create_deterministic_address_bcrt1_p2tr_op_true,
    key_to_p2pkh,
    key_to_p2sh_p2wpkh,
    key_to_p2wpkh,
    program_to_witness,
    script_to_p2wsh,
    script_to_p2sh,
)
from test_framework.descriptors import descsum_create
from test_framework.key import ECKey, ORDER
from test_framework.messages import COIN, COutPoint, CTransaction, CTxIn, CTxInWitness, CTxOut, tx_from_hex
from test_framework.script import (
    CScript, LegacySignatureHash, SegwitV0SignatureHash, SIGHASH_ALL,
    OP_CHECKSEQUENCEVERIFY, OP_CHECKSIG, OP_ELSE, OP_ENDIF, OP_EQUALVERIFY,
    OP_IF, OP_SHA256, OP_SIZE, OP_TRUE, OP_VERIFY,
)
from test_framework.script_util import (
    key_to_p2pk_script, key_to_p2pkh_script, key_to_p2wpkh_script,
    keys_to_multisig_script, script_to_p2sh_script, script_to_p2wsh_script,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_not_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import bytes_to_wif


# BIP32 vector 1, also used in src/test/bip32_tests.cpp. These anchor the
# independent stdlib HMAC + existing Python ECKey derivation oracle below.
ROOT_XPRV = 'xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi'
CHILD_XPRV = 'xprv9uHRZZhk6KAJC1avXpDAp4MDc3sQKNxDiPvvkX8Br5ngLNv1TxvUxt4cV1rGL5hj6KCesnDYUhd7oWgT11eZG7XnxHrnYeSvkzY7d2bhkJ7'


def public_key(secret, compressed=True):
    key = ECKey()
    key.set(secret, compressed)
    return key.get_pubkey().get_bytes()


def derive_leaf(secret, chain, path):
    """Independent BIP32 CKDpriv for the fixed, valid test paths only."""
    for index in path:
        data = b'\x00' + secret if index >= 0x80000000 else public_key(secret)
        digest = hmac.new(chain, data + index.to_bytes(4, 'big'), hashlib.sha512).digest()
        tweak = int.from_bytes(digest[:32], 'big')
        assert tweak < ORDER, 'invalid synthetic derivation tweak'
        value = (tweak + int.from_bytes(secret, 'big')) % ORDER
        assert_not_equal(value, 0)
        secret, chain = value.to_bytes(32, 'big'), digest[32:]
    return secret, chain


class ExportAddressKeyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.wallet_names = []
        self.extra_args = [['-keypool=10']]
        if self.is_external_signer_compiled():
            mock = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'mocks', 'signer.py')
            self.extra_args[0].append(f'-signer={sys.executable} {mock}')

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def import_descriptor(self, wallet, descriptor, **kwargs):
        result = wallet.importdescriptors([{'desc': descsum_create(descriptor), 'timestamp': 'now', **kwargs}])
        assert result[0]['success'], 'synthetic descriptor import failed'
        if any(material in descriptor for material in self.private_material):
            self.private_material.add(descriptor)

    def check_export(self, wallet, address, secret, compressed, *, witness=False):
        result = self.export_unchanged(wallet, address)
        assert_equal(set(result), {'keys', 'private_key_hex', 'compressed'})
        encoded = result['private_key_hex']
        assert isinstance(encoded, str) and re.fullmatch('[0-9a-f]{64}', encoded), 'invalid scalar encoding'
        assert_equal(encoded, secret.hex())
        assert_equal(type(result['compressed']), bool)
        assert_equal(result['compressed'], compressed)
        pubkey = public_key(bytes.fromhex(encoded), result['compressed'])
        assert_equal(result['keys'], [{'pubkey': pubkey.hex(), 'private_key_hex': encoded, 'compressed': compressed, 'roles': ['script']}])
        derived_address = key_to_p2wpkh(pubkey) if witness else key_to_p2pkh(pubkey)
        assert_equal(derived_address, address)
        self.private_material.add(encoded)

    def snapshot(self, wallet, addresses):
        info = wallet.getwalletinfo()
        return {
            'indices': wallet.listdescriptors(),  # public descriptors only
            'balances': wallet.getbalances(),
            'wallet': {k: info.get(k) for k in ['txcount', 'keypoolsize', 'keypoolsize_hd_internal']},
            'addresses': [wallet.getaddressinfo(address) for address in addresses],
            'labels': {label: wallet.getaddressesbylabel(label) for label in wallet.listlabels()},
            'transactions': wallet.listtransactions(count=1000),
            'locked_coins': wallet.listlockunspent(),
            'unlocked_until': info.get('unlocked_until'),
        }

    def export_unchanged(self, wallet, address):
        # These snapshots avoid getaddressinfo before the first export, since that
        # RPC may warm public solving-provider caches. Warm them explicitly afterward.
        before = self.snapshot(wallet, [])
        result = wallet.exportaddresskey(address)
        wallet.getaddressinfo(address)
        assert_equal(wallet.exportaddresskey(address), result)
        assert_equal(self.snapshot(wallet, []), before)
        return result

    def check_script_export(self, wallet, address, secrets):
        result = self.export_unchanged(wallet, address)
        assert_equal(set(result), {'keys'})
        expected = sorted([
            {'pubkey': public_key(secret, compressed).hex(), 'private_key_hex': secret.hex(),
             'compressed': compressed, 'roles': ['script']}
            for secret, compressed in set(secrets)
        ], key=lambda record: record['pubkey'])
        assert_equal(result['keys'], expected)
        self.private_material.update(secret.hex() for secret, _ in secrets)
        return result

    def check_script_constructions(self, wallet, root, root_secret, root_chain):
        self.log.info('Check nested wrappers and partial multisig ownership independently of threshold')
        secrets = [(i).to_bytes(32, 'big') for i in range(20, 23)]
        pubkeys = [public_key(secret) for secret in secrets]
        wifs = [bytes_to_wif(secret) for secret in secrets]
        self.private_material.update(wifs)
        for variant, (local, threshold, sorted_keys) in enumerate([(1, 1, False), (1, 3, False), (2, 2, True), (3, 2, False)]):
            name = f'partial_{variant}'
            self.nodes[0].createwallet(name, blank=True)
            partial = self.nodes[0].get_wallet_rpc(name)
            expressions = [wifs[i] if i < local else pubkeys[i].hex() for i in range(3)]
            kind = 'sortedmulti' if sorted_keys else 'multi'
            inner = f'{kind}({threshold},{",".join(expressions)})'
            script = keys_to_multisig_script(sorted(pubkeys) if sorted_keys else pubkeys, k=threshold)
            for descriptor, address in [(f'sh({inner})', script_to_p2sh(script)),
                                        (f'wsh({inner})', script_to_p2wsh(script)),
                                        (f'sh(wsh({inner}))', script_to_p2sh(CScript([0, hashlib.sha256(script).digest()])))]:
                self.import_descriptor(partial, descriptor)
                self.check_script_export(partial, address, [(secret, True) for secret in secrets[:local]])
        for compressed in [True, False]:
            secret = (30 + compressed).to_bytes(32, 'big')
            wif = bytes_to_wif(secret, compressed)
            self.private_material.add(wif)
            pubkey = public_key(secret, compressed)
            for kind, script in [('pk', key_to_p2pk_script(pubkey)), ('pkh', key_to_p2pkh_script(pubkey))]:
                self.import_descriptor(wallet, f'sh({kind}({wif}))')
                self.check_script_export(wallet, script_to_p2sh(script), [(secret, compressed)])
            if compressed:
                self.import_descriptor(wallet, f'sh(wpkh({wif}))')
                self.check_script_export(wallet, key_to_p2sh_p2wpkh(pubkey), [(secret, True)])
                self.import_descriptor(wallet, f'wsh(pk({wif}))')
                self.check_script_export(wallet, script_to_p2wsh(key_to_p2pk_script(pubkey)), [(secret, True)])

        self.log.info('Check ranged receive/change multisig and repeated keys')
        for branch, index in [(0, 7), (1, 11)]:
            desc = f'wsh(multi(2,{wifs[0]},{root}/48h/1h/0h/{branch}/*,{wifs[0]}))'
            self.import_descriptor(wallet, desc, active=True, internal=bool(branch), range=[0, 15], next_index=index)
            leaf, _ = derive_leaf(root_secret, root_chain, [0x80000030, 0x80000001, 0x80000000, branch, index])
            script = keys_to_multisig_script([pubkeys[0], public_key(leaf), pubkeys[0]], k=2)
            address = script_to_p2wsh(script)
            generated = wallet.getrawchangeaddress('bech32') if branch else wallet.getnewaddress('', 'bech32')
            assert_equal(generated, address)
            self.check_script_export(wallet, address, [(secrets[0], True), (leaf, True)])

        self.log.info('Check all Miniscript branches despite timelocks and unavailable hash preimages')
        for policy in [f'or_i(pk({wifs[0]}),and_v(v:older(1000),pk({wifs[1]})))',
                       f'or_i(pk({wifs[0]}),and_v(v:sha256({hashlib.sha256(b"unknown preimage").hexdigest()}),pk({wifs[1]})))']:
            descriptor = f'wsh({policy})'
            self.import_descriptor(wallet, descriptor)
            public_desc = wallet.getdescriptorinfo(descsum_create(descriptor))['descriptor']
            address = wallet.deriveaddresses(public_desc)[0]
            self.check_script_export(wallet, address, [(secrets[0], True), (secrets[1], True)])

    def check_cross_manager_resolution(self, root, root_secret, root_chain):
        self.log.info('Check cross-descriptor key union and independence from import order')
        node = self.nodes[0]
        secrets = [(i).to_bytes(32, 'big') for i in [50, 51]]
        pubkeys = [public_key(secret) for secret in secrets]
        wifs = [bytes_to_wif(secret) for secret in secrets]
        self.private_material.update(wifs)
        a = f'wsh(multi(2,{wifs[0]},{pubkeys[1].hex()}))'
        b = f'wsh(multi(2,[01020304]{pubkeys[0].hex()},{wifs[1]}))'
        address = script_to_p2wsh(keys_to_multisig_script(pubkeys, k=2))
        for name, descriptors in [('cross', [a, b]), ('cross_reverse', [b, a])]:
            node.createwallet(name, blank=True)
            wallet = node.get_wallet_rpc(name)
            self.import_descriptor(wallet, descriptors[0])
            first = secrets[0] if name == 'cross' else secrets[1]
            self.check_script_export(wallet, address, [(first, True)])
            self.import_descriptor(wallet, descriptors[1])
            self.check_script_export(wallet, address, [(secret, True) for secret in secrets])
        wallet = node.get_wallet_rpc('cross')

        self.log.info('Check a concrete candidate whose derived secret belongs to another descriptor')
        self.import_descriptor(wallet, f'wpkh({root}/5/*)', range=[0, 5])
        leaf, _ = derive_leaf(root_secret, root_chain, [5, 3])
        leaf_pubkey = public_key(leaf)
        target = f'wsh(multi(2,{wifs[0]},{leaf_pubkey.hex()}))'
        self.import_descriptor(wallet, target)
        address = script_to_p2wsh(keys_to_multisig_script([pubkeys[0], leaf_pubkey], k=2))
        self.check_script_export(wallet, address, [(secrets[0], True), (leaf, True)])
        self.check_export(wallet, key_to_p2pkh(leaf_pubkey), leaf, True)
        self.check_script_export(wallet, key_to_p2sh_p2wpkh(leaf_pubkey), [(leaf, True)])

        self.log.info('Check exact compression identity and selected-wallet isolation')
        uncompressed = public_key(secrets[1], compressed=False)
        assert_raises_rpc_error(-5, 'No private key for this address', wallet.exportaddresskey, key_to_p2pkh(uncompressed))
        wif = bytes_to_wif(secrets[1], compressed=False)
        self.private_material.add(wif)
        self.import_descriptor(wallet, f'unused({wif})')
        self.check_export(wallet, key_to_p2pkh(uncompressed), secrets[1], False)
        node.createwallet('isolated', blank=True)
        isolated = node.get_wallet_rpc('isolated')
        assert_raises_rpc_error(-5, 'No private key for this address', isolated.exportaddresskey, key_to_p2pkh(leaf_pubkey))
        assert_raises_rpc_error(-5, 'lacks the spending/script information', isolated.exportaddresskey, address)

    def check_spending_and_state(self, root_secret, root_chain):
        self.log.info('Fund independently reconstructed scripts and spend using exported scalars')
        node = self.nodes[0]
        node.createwallet('funding')
        funding = node.get_wallet_rpc('funding')
        mining_address = funding.getnewaddress()
        self.generatetoaddress(node, 101, mining_address)

        def spend(wallet_name, script_code, wrapper, expected, make_stack, *, external=(), required=1):
            wallet = node.get_wallet_rpc(wallet_name)
            if wrapper == 'sh':
                outer, address = script_to_p2sh_script(script_code), script_to_p2sh(script_code)
                redeem = script_code
            elif wrapper == 'sh_wpkh':
                pubkey = public_key(*expected[0])
                redeem = key_to_p2wpkh_script(pubkey)
                outer, address = script_to_p2sh_script(redeem), script_to_p2sh(redeem)
            else:
                redeem = script_to_p2wsh_script(script_code)
                outer = script_to_p2sh_script(redeem) if wrapper == 'sh_wsh' else redeem
                address = script_to_p2sh(redeem) if wrapper == 'sh_wsh' else script_to_p2wsh(script_code)
            wallet.setlabel(address, 'script export spend fixture')
            txid = funding.sendtoaddress(address, 1)
            self.generatetoaddress(node, 1, mining_address)
            previous = tx_from_hex(funding.gettransaction(txid)['hex'])
            matches = [i for i, output in enumerate(previous.vout) if output.scriptPubKey == outer]
            assert_equal(len(matches), 1)
            index = matches[0]
            assert_equal(previous.vout[index].nValue, COIN)
            coin = {'txid': txid, 'vout': index}
            assert wallet.lockunspent(False, [coin]), 'funded wallet output cannot be locked'
            assert coin in wallet.listlockunspent(), 'nonempty lock-state fixture missing'
            exported = self.check_script_export(wallet, address, expected)
            assert wallet.lockunspent(True, [coin]), 'funded wallet output cannot be unlocked'

            tx = CTransaction()
            tx.vin = [CTxIn(COutPoint(int(txid, 16), index), nSequence=0xfffffffd)]
            destination = funding.getnewaddress('', 'legacy')
            destination_info = node.validateaddress(destination)
            tx.vout = [CTxOut(COIN - 10000, bytes.fromhex(destination_info['scriptPubKey']))]
            witness = wrapper != 'sh'
            if witness:
                digest = SegwitV0SignatureHash(script_code, tx, inIdx=0, hashtype=SIGHASH_ALL, amount=COIN)
            else:
                digest, error = LegacySignatureHash(script_code, tx, inIdx=0, hashtype=SIGHASH_ALL)
                assert_equal(error, None)
            signatures = {}

            def sign(secret, compressed):
                key = ECKey()
                key.set(secret, compressed)
                signature = key.sign_ecdsa(digest)
                assert key.get_pubkey().verify_ecdsa(signature, digest), 'exported scalar signature does not verify'
                signatures[key.get_pubkey().get_bytes()] = signature + bytes([SIGHASH_ALL])

            for record in exported['keys']:
                sign(bytes.fromhex(record['private_key_hex']), record['compressed'])
            assert_equal(len(signatures), len(expected))

            def set_stack():
                stack = make_stack(signatures)
                if witness:
                    tx.vin[0].scriptSig = CScript([redeem]) if wrapper.startswith('sh_') else CScript()
                    tx.wit.vtxinwit = [CTxInWitness()]
                    tx.wit.vtxinwit[0].scriptWitness.stack = stack + ([] if wrapper == 'sh_wpkh' else [script_code])
                else:
                    tx.vin[0].scriptSig = CScript(stack + [script_code])

            if len(signatures) < required:
                set_stack()
                partial = node.testmempoolaccept([tx.serialize().hex()])[0]
                assert not partial['allowed'], 'incomplete exported set unexpectedly satisfied threshold'
                assert 'script' in partial['reject-reason'] or 'stack' in partial['reject-reason'], 'partial spend failed outside script verification'
            for secret, compressed in external:
                sign(secret, compressed)
            set_stack()
            assert node.testmempoolaccept([tx.serialize().hex()])[0]['allowed'], 'exported-key spend rejected by script validation'
            node.sendrawtransaction(tx.serialize().hex())
            self.generatetoaddress(node, 1, mining_address)
            assert_equal(node.gettxout(txid, index), None)

        secrets = [(i).to_bytes(32, 'big') for i in [20, 21, 22]]
        pubkeys = [public_key(secret) for secret in secrets]
        for name, threshold, wrapper, local, sorted_keys in [
                ('partial_0', 1, 'sh', 1, False),
                ('partial_1', 3, 'wsh', 1, False),
                ('partial_2', 2, 'sh_wsh', 2, True)]:
            ordered = sorted(pubkeys) if sorted_keys else pubkeys
            script = keys_to_multisig_script(ordered, k=threshold)
            spend(name, script, wrapper, [(secret, True) for secret in secrets[:local]],
                  lambda signatures, ordered=ordered, threshold=threshold: [b''] + [signatures[p] for p in ordered if p in signatures][:threshold],
                  external=[(secret, True) for secret in secrets[local:threshold]], required=threshold)

        uncompressed = (30).to_bytes(32, 'big')
        pubkey = public_key(uncompressed, False)
        spend('scripts', key_to_p2pk_script(pubkey), 'sh', [(uncompressed, False)], lambda signatures: [signatures[pubkey]])
        compressed = (31).to_bytes(32, 'big')
        pubkey = public_key(compressed)
        spend('scripts', key_to_p2pkh_script(pubkey), 'sh_wpkh', [(compressed, True)], lambda signatures: [signatures[pubkey], pubkey])

        for guard in [[1000, OP_CHECKSEQUENCEVERIFY, OP_VERIFY],
                      [OP_SIZE, 32, OP_EQUALVERIFY, OP_SHA256, hashlib.sha256(b'unknown preimage').digest(), OP_EQUALVERIFY]]:
            # Reconstruct both Miniscript branches directly, without asking Core for a script.
            script = CScript([OP_IF, pubkeys[0], OP_CHECKSIG, OP_ELSE, *guard, pubkeys[1], OP_CHECKSIG, OP_ENDIF])
            spend('scripts', script, 'wsh', [(secret, True) for secret in secrets[:2]],
                  lambda signatures: [signatures[pubkeys[0]], b'\x01'])

        leaf, _ = derive_leaf(root_secret, root_chain, [5, 3])
        fixed = (50).to_bytes(32, 'big')
        ordered = [public_key(fixed), public_key(leaf)]
        spend('cross', keys_to_multisig_script(ordered, k=2), 'wsh', [(fixed, True), (leaf, True)],
              lambda signatures: [b''] + [signatures[p] for p in ordered], required=2)

        self.log.info('Check encrypted script/cross-manager export preserves authorization and transaction state')
        wallet = node.get_wallet_rpc('cross')
        address = script_to_p2wsh(keys_to_multisig_script(ordered, k=2))
        wallet.encryptwallet('synthetic-script-passphrase')
        before = self.snapshot(wallet, [])
        assert_raises_rpc_error(-13, 'walletpassphrase', wallet.exportaddresskey, address)
        assert_equal(self.snapshot(wallet, []), before)
        wallet.walletpassphrase('synthetic-script-passphrase', 60)
        self.check_script_export(wallet, address, [(fixed, True), (leaf, True)])
        wallet.walletlock()
        before = self.snapshot(wallet, [])
        assert_raises_rpc_error(-13, 'walletpassphrase', wallet.exportaddresskey, address)
        assert_equal(self.snapshot(wallet, []), before)

    def run_test(self):
        node = self.nodes[0]
        self.private_material = set()
        node.createwallet('owner', blank=True)
        wallet = node.get_wallet_rpc('owner')
        # JSON null and type checks must use JSON-RPC even under --usecli.
        rpc_node = node.create_new_rpc_connection(mode='AUTHPROXY')
        rpc_wallet = rpc_node / 'wallet/owner'
        help_text = wallet.help('exportaddresskey')
        for field in ['key_type', 'wallet', 'taproot_output', 'keys', 'pubkey', 'private_key_hex', 'compressed', 'roles']:
            assert field in help_text, 'RPC help missing field'

        self.log.info('Check fixed-width scalar encoding and both P2PKH public-key serializations')
        fixtures = []
        for secret in [(1).to_bytes(32, 'big'), bytes.fromhex('00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f')]:
            for compressed in [True, False]:
                pubkey = public_key(secret, compressed)
                wif = bytes_to_wif(secret, compressed)
                self.private_material.add(wif)
                self.import_descriptor(wallet, f'pkh({wif})')
                fixtures.append((key_to_p2pkh(pubkey), secret, compressed, False))
                if compressed:
                    self.import_descriptor(wallet, f'wpkh({wif})')
                    fixtures.append((key_to_p2wpkh(pubkey), secret, True, True))

        self.log.info('Check nonzero receive/change indices against independent BIP32 leaf values')
        payload, version = base58_to_byte(ROOT_XPRV)
        assert_equal(version, 4)
        assert_equal(len(payload), 77)
        root_secret, root_chain = payload[-32:], payload[-65:-33]
        child_payload, _ = base58_to_byte(CHILD_XPRV)
        assert_equal(derive_leaf(root_secret, root_chain, [0x80000000]), (child_payload[-32:], child_payload[-65:-33]))
        root = byte_to_base58(bytes.fromhex('358394') + payload[3:], 4)
        self.private_material.update([root, root_secret.hex(), ROOT_XPRV])
        for branch, index in [(0, 7), (1, 11)]:
            self.import_descriptor(wallet, f'wpkh({root}/84h/1h/0h/{branch}/*)', active=True, internal=bool(branch), range=[0, 15], next_index=index)
            secret, _ = derive_leaf(root_secret, root_chain, [0x80000054, 0x80000001, 0x80000000, branch, index])
            address = key_to_p2wpkh(public_key(secret))
            generated = wallet.getrawchangeaddress('bech32') if branch else wallet.getnewaddress('receive', 'bech32')
            assert_equal(generated, address)
            fixtures.append((address, secret, True, True))

        addresses = [fixture[0] for fixture in fixtures]
        node.createwallet('scripts', blank=True)
        self.check_script_constructions(node.get_wallet_rpc('scripts'), root, root_secret, root_chain)
        self.check_cross_manager_resolution(root, root_secret, root_chain)
        self.check_spending_and_state(root_secret, root_chain)
        self.log.info('Check optional mode, named arguments, JSON-RPC versions and CLI strings')
        expected = wallet.exportaddresskey(addresses[0])
        for result in [rpc_wallet.exportaddresskey(addresses[0], None),
                       wallet.exportaddresskey(addresses[0], 'wallet'),
                       wallet.exportaddresskey(address=addresses[0]),
                       rpc_wallet.exportaddresskey(address=addresses[0], key_type=None),
                       wallet.exportaddresskey(key_type='wallet', address=addresses[0]),
                       node.cli('-rpcwallet=owner').exportaddresskey(addresses[0], 'wallet')]:
            assert_equal(result, expected)
        for version in ['1.0', '2.0']:
            for params in [[addresses[0]], [addresses[0], None], {'address': addresses[0], 'key_type': 'wallet'}]:
                response, status = rpc_node._request('POST', '/wallet/owner', json.dumps({'jsonrpc': version, 'id': 1, 'method': 'exportaddresskey', 'params': params}).encode())
                assert_equal(status, 200)
                assert_equal(response['result'], expected)
            response, _ = rpc_node._request('POST', '/wallet/owner', ('{"jsonrpc":"' + version + '","id":1,"method":"exportaddresskey","params":{"address":"' + addresses[0] + '","key_type":"wallet","key_type":"wallet"}}').encode())
            assert_equal(response['error']['code'], -8)
        wallet.setlabel(addresses[0], 'synthetic export fixture')
        self.generatetoaddress(node, 1, addresses[0])
        before = self.snapshot(wallet, addresses)
        assert_greater_than(before['balances']['mine']['immature'], 0)
        for _ in range(2):
            for address, secret, compressed, witness in fixtures:
                self.check_export(wallet, address, secret, compressed, witness=witness)
        assert_equal(self.snapshot(wallet, addresses), before)

        self.log.info('Check wallet selection, capability and missing-key errors')
        node.createwallet('other', blank=True)
        other = node.get_wallet_rpc('other')
        unavailable = 'No private key for this address is available in the selected wallet'
        capability = 'This wallet has private keys disabled'
        assert_raises_rpc_error(-19, 'Multiple wallets are loaded', node.exportaddresskey, addresses[0])
        assert_raises_rpc_error(-18, 'does not exist or is not loaded', node.get_wallet_rpc('missing').exportaddresskey, addresses[0])
        assert_raises_rpc_error(-5, unavailable, other.exportaddresskey, addresses[0])
        unknown_pubkey = public_key((3).to_bytes(32, 'big'))
        unknown = key_to_p2wpkh(unknown_pubkey)
        assert_raises_rpc_error(-5, unavailable, wallet.exportaddresskey, unknown)
        uncompressed_witness = key_to_p2wpkh(public_key((1).to_bytes(32, 'big'), compressed=False))
        assert_raises_rpc_error(-5, unavailable, wallet.exportaddresskey, uncompressed_witness)
        node.createwallet('watch', disable_private_keys=True, blank=True)
        watch = node.get_wallet_rpc('watch')
        assert_raises_rpc_error(-4, capability, watch.exportaddresskey, unknown)
        self.import_descriptor(watch, f'addr({unknown})')
        assert_raises_rpc_error(-4, capability, watch.exportaddresskey, unknown)
        raw_address = key_to_p2wpkh(public_key((4).to_bytes(32, 'big')))
        self.import_descriptor(watch, f'raw({key_to_p2wpkh_script(public_key((4).to_bytes(32, "big"))).hex()})')
        assert_raises_rpc_error(-4, capability, watch.exportaddresskey, raw_address)
        # Core requires public-only descriptors to reside in a private-key-disabled wallet.
        self.import_descriptor(watch, f'wpkh({unknown_pubkey.hex()})')
        assert_raises_rpc_error(-4, capability, watch.exportaddresskey, unknown)
        if self.is_external_signer_compiled():
            node.createwallet('signer', disable_private_keys=True, external_signer=True)
            signer = node.get_wallet_rpc('signer')
            assert signer.getwalletinfo()['external_signer'], 'external-signer fixture missing capability'
            assert_raises_rpc_error(-4, 'Private keys are held by an external signer', signer.exportaddresskey, signer.getnewaddress('', 'bech32'))
        else:
            self.log.info('External-signer capability coverage unavailable in this build')

        self.log.info('Check network validation, unsupported scripts and argument validation')
        assert_raises_rpc_error(-5, 'Invalid address', wallet.exportaddresskey, 'not-an-address')
        invalid_checksum = addresses[0][:-1] + ('1' if addresses[0][-1] != '1' else '2')
        assert_raises_rpc_error(-5, 'Invalid address', wallet.exportaddresskey, invalid_checksum)
        for address in [key_to_p2pkh(unknown_pubkey, main=True), key_to_p2wpkh(unknown_pubkey, main=True)]:
            assert_raises_rpc_error(-5, 'Invalid address', wallet.exportaddresskey, address)
        taproot, _ = create_deterministic_address_bcrt1_p2tr_op_true()
        for address in [key_to_p2sh_p2wpkh(unknown_pubkey), script_to_p2wsh(CScript([OP_TRUE]))]:
            assert_raises_rpc_error(-5, 'lacks the spending/script information', wallet.exportaddresskey, address)
        for address in [taproot, program_to_witness(2, bytes(32))]:
            assert_raises_rpc_error(-5, 'spending construction is not supported', wallet.exportaddresskey, address)
        assert_raises_rpc_error(-5, 'spending construction is not supported', wallet.exportaddresskey, taproot, 'taproot_output')
        assert_raises_rpc_error(-8, 'requires a Taproot address', wallet.exportaddresskey, addresses[0], 'taproot_output')
        for mode in ['', 'Wallet', 'output', 'null']:
            assert_raises_rpc_error(-8, "key_type must be 'wallet' or 'taproot_output'", wallet.exportaddresskey, addresses[0], mode)
        for mode in [1, True, [], {}]:
            assert_raises_rpc_error(-3, 'JSON value of type', rpc_wallet.exportaddresskey, addresses[0], mode)
        assert_raises_rpc_error(-8, 'Unknown named parameter', wallet.exportaddresskey, address=addresses[0], unknown='wallet')
        for argument in [1, True, [], {}]:
            assert_raises_rpc_error(-3, 'JSON value of type', rpc_wallet.exportaddresskey, argument)
        assert_raises_rpc_error(-1, 'exportaddresskey', wallet.exportaddresskey)
        assert_raises_rpc_error(-1, 'exportaddresskey', wallet.exportaddresskey, addresses[0], 'wallet', 'extra')
        assert_raises_rpc_error(-3, 'not of expected type string', rpc_wallet.exportaddresskey, None)

        self.log.info('Check encrypted locked, unlocked and relocked authorization')
        wallet.encryptwallet('synthetic-test-passphrase')
        assert_raises_rpc_error(-13, 'walletpassphrase', wallet.exportaddresskey, addresses[0])
        assert_raises_rpc_error(-13, 'walletpassphrase', wallet.exportaddresskey, unknown)
        assert_raises_rpc_error(-13, 'walletpassphrase', wallet.exportaddresskey, taproot, 'taproot_output')
        wallet.walletpassphrase('synthetic-test-passphrase', 60)
        for address, secret, compressed, witness in fixtures:
            self.check_export(wallet, address, secret, compressed, witness=witness)
        wallet.walletlock()
        assert_raises_rpc_error(-13, 'walletpassphrase', wallet.exportaddresskey, addresses[-1])

        # Normal daemon RPC logging must never include exported scalars, WIFs,
        # private descriptors or the synthetic HD root. Fixed diagnostics avoid
        # printing key material even if an assertion fails.
        debug_log = node.debug_log_path.read_text(encoding='utf-8')
        assert all(material not in debug_log for material in self.private_material), 'private material found in normal daemon log'
        assert not re.search(r'\b(?:tprv|xprv)[1-9A-HJ-NP-Za-km-z]+', debug_log), 'extended private key found in normal daemon log'
        self.log.info('Address-key export regression checks passed')


if __name__ == '__main__':
    ExportAddressKeyTest(__file__).main()
