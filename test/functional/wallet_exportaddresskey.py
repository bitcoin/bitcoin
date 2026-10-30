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
from test_framework.authproxy import JSONRPCException
from test_framework.crypto.secp256k1 import GE, G
from test_framework.key import ECKey, H_POINT, ORDER, TaggedHash, sign_schnorr, verify_schnorr
from test_framework.messages import COIN, COutPoint, CTransaction, CTxIn, CTxInWitness, CTxOut, tx_from_hex
from test_framework.script import (
    CScript, LegacySignatureHash, SegwitV0SignatureHash, SIGHASH_ALL,
    OP_CHECKLOCKTIMEVERIFY, OP_CHECKSEQUENCEVERIFY, OP_CHECKSIG, OP_CHECKSIGADD, OP_ELSE, OP_ENDIF, OP_EQUALVERIFY,
    OP_IF, OP_NUMEQUAL, OP_SHA256, OP_SIZE, OP_TRUE, OP_VERIFY, SIGHASH_DEFAULT, TaprootSignatureHash, taproot_construct,
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


def musig_public_key(pubkeys, path=()):
    """Independent BIP327 public KeyAgg and BIP328 public derivation, for synthetic fixtures."""
    pubkeys = sorted(pubkeys)
    second = next((pubkey for pubkey in pubkeys if pubkey != pubkeys[0]), None)
    key_list = TaggedHash('KeyAgg list', b''.join(pubkeys))
    terms = [(1 if pubkey == second else int.from_bytes(TaggedHash('KeyAgg coefficient', key_list + pubkey), 'big') % ORDER,
              GE.from_bytes(pubkey)) for pubkey in pubkeys]
    point = GE.mul(*terms)
    chain = bytes.fromhex('868087ca02a6f974c4598924c36b57762d32cb45717167e300622c7167e38965')
    for index in path:
        assert index < 0x80000000, 'test aggregate derivation must be public'
        digest = hmac.new(chain, point.to_bytes_compressed() + index.to_bytes(4, 'big'), hashlib.sha512).digest()
        tweak = int.from_bytes(digest[:32], 'big')
        assert tweak < ORDER, 'invalid test public derivation'
        point = point + tweak * G
        chain = digest[32:]
    assert not point.infinity, 'invalid synthetic aggregate'
    return point.to_bytes_compressed()


def taproot_output_secret(secret, merkle_root=b''):
    """Independent BIP341 test oracle, preserving the actual tweaked scalar's output parity."""
    pubkey = public_key(secret)
    value = int.from_bytes(secret, 'big')
    if pubkey[0] == 3:
        value = ORDER - value
    tweak = int.from_bytes(TaggedHash('TapTweak', pubkey[1:] + merkle_root), 'big')
    assert tweak < ORDER, 'invalid synthetic tweak'
    value = (value + tweak) % ORDER
    assert value, 'invalid synthetic output scalar'
    return value.to_bytes(32, 'big')


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

    def check_error(self, code, message, function, *args, **kwargs):
        """Check exact failure shape without exposing synthetic secrets on failure."""
        try:
            function(*args, **kwargs)
        except JSONRPCException as error:
            assert_equal(error.error['code'], code)
            assert message in error.error['message'], 'incorrect export error message'
            assert_equal(set(error.error), {'code', 'message'})
            assert all(secret not in error.error['message'] for secret in self.private_material), 'secret in export error'
        else:
            raise AssertionError('export unexpectedly succeeded instead of returning an error')

    def check_failure_matrix(self, wallet, fixtures):
        rpc_wallet = self.nodes[0].create_new_rpc_connection(mode='AUTHPROXY') / 'wallet/owner'
        self.log.info('Check both-mode error matrix and validation/authorization precedence')
        node = self.nodes[0]
        address = fixtures[0][0]
        secret = (140).to_bytes(32, 'big')
        self.private_material.update([secret.hex(), bytes_to_wif(secret)])
        self.import_descriptor(wallet, f'tr({bytes_to_wif(secret)})')
        taproot = program_to_witness(1, taproot_construct(public_key(secret)[1:]).output_pubkey)
        for mode in ['wallet', 'taproot_output']:
            expected = wallet.exportaddresskey(taproot, mode)
            assert_equal(len(expected['keys']), 1)
            assert_equal(wallet.exportaddresskey(address=taproot, key_type=mode), expected)
            for endpoint, code, message in [(node, -19, 'Multiple wallets are loaded'),
                                            (node.get_wallet_rpc('missing'), -18, 'does not exist or is not loaded')]:
                self.check_error(code, message, endpoint.exportaddresskey, taproot, mode)
                self.check_error(code, message, endpoint.exportaddresskey, 'invalid', mode)
            capabilities = [(node.get_wallet_rpc('watch'), 'This wallet has private keys disabled')]
            if self.is_external_signer_compiled():
                capabilities.append((node.get_wallet_rpc('signer'), 'Private keys are held by an external signer'))
            for endpoint, message in capabilities:
                self.check_error(-4, message, endpoint.exportaddresskey, taproot, mode)
                self.check_error(-5, 'Invalid address', endpoint.exportaddresskey, 'invalid', mode)
                self.check_error(-8, "key_type must be 'wallet' or 'taproot_output'", endpoint.exportaddresskey, taproot, 'unknown')
            for invalid in ['invalid', taproot[:-1] + ('q' if taproot[-1] != 'q' else 'p'),
                            key_to_p2pkh(public_key(secret), main=True), key_to_p2wpkh(public_key(secret), main=True)]:
                self.check_error(-5, 'Invalid address', wallet.exportaddresskey, invalid, mode)
            for invalid in [None, 1, True, [], {}]:
                self.check_error(-3, 'Wrong type passed', rpc_wallet.exportaddresskey, invalid, mode)
            self.check_error(-1, 'exportaddresskey', wallet.exportaddresskey, taproot, mode, 'extra')
            self.check_error(-8, 'Unknown named parameter', wallet.exportaddresskey, address=taproot, key_type=mode, extra=True)
            self.check_error(-3, 'Wrong type passed', rpc_wallet.exportaddresskey, key_type=mode, address=None)
            self.check_error(-3, 'Wrong type passed', rpc_wallet.exportaddresskey, key_type=mode)
            self.check_error(-5, 'spending construction is not supported', wallet.exportaddresskey, program_to_witness(1, bytes(32)), mode)
            self.check_error(-5, 'Invalid address', wallet.exportaddresskey, bytes_to_wif(secret), mode)
        self.check_error(-8, "key_type must be 'wallet' or 'taproot_output'", wallet.exportaddresskey, taproot, ROOT_XPRV)
        # An empty selected wallet has no secret; a directly known output key has one.
        node.createwallet('empty_matrix', blank=True)
        empty = node.get_wallet_rpc('empty_matrix')
        direct = program_to_witness(1, public_key(secret)[1:])
        for mode in ['wallet', 'taproot_output']:
            self.check_error(-5, 'lacks the spending/script information', empty.exportaddresskey, direct, mode)
        self.import_descriptor(empty, f'rawtr({bytes_to_wif(secret)})')
        for mode in ['wallet', 'taproot_output']:
            assert_equal(len(empty.exportaddresskey(direct, mode)['keys']), 1)
        wallet.encryptwallet('synthetic-test-passphrase')
        for mode in ['wallet', 'taproot_output']:
            self.check_error(-13, 'walletpassphrase', wallet.exportaddresskey, taproot, mode)
            self.check_error(-13, 'walletpassphrase', wallet.exportaddresskey, direct, mode)
            self.check_error(-5, 'Invalid address', wallet.exportaddresskey, 'invalid', mode)
            self.check_error(-8, "key_type must be 'wallet' or 'taproot_output'", wallet.exportaddresskey, taproot, 'invalid')
        self.check_error(-8, 'requires a Taproot address', wallet.exportaddresskey, address, 'taproot_output')
        wallet.walletpassphrase('synthetic-test-passphrase', 60)
        for fixture_address, fixture_secret, compressed, witness in fixtures:
            self.check_export(wallet, fixture_address, fixture_secret, compressed, witness=witness)
        for mode in ['wallet', 'taproot_output']:
            assert_equal(len(wallet.exportaddresskey(taproot, mode)['keys']), 1)
        wallet.walletlock()
        for mode in ['wallet', 'taproot_output']:
            self.check_error(-13, 'walletpassphrase', wallet.exportaddresskey, taproot, mode)

    def import_descriptor(self, wallet, descriptor, **kwargs):
        result = wallet.importdescriptors([{'desc': descsum_create(descriptor), 'timestamp': 'now', **kwargs}])
        for item in result:
            message = item.get('error', {}).get('message', '')
            for material in self.private_material:
                message = message.replace(material, '<redacted>')
            assert item['success'], f'synthetic descriptor import failed: {message}'
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

    def export_unchanged(self, wallet, address, key_type=None):
        # These snapshots avoid getaddressinfo before the first export, since that
        # RPC may warm public solving-provider caches. Warm them explicitly afterward.
        before = self.snapshot(wallet, [])
        arguments = [address] if key_type is None else [address, key_type]
        result = wallet.exportaddresskey(*arguments)
        wallet.getaddressinfo(address)
        assert_equal(wallet.exportaddresskey(*arguments), result)
        assert_equal(self.snapshot(wallet, []), before)
        for record in result['keys']:
            self.private_material.add(record['private_key_hex'])
            self.private_material.add(bytes_to_wif(bytes.fromhex(record['private_key_hex']), compressed=record['compressed']))
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

    def check_taproot_roles(self):
        self.log.info('Check Taproot internal, direct output and script roles with independent public commitments')
        node = self.nodes[0]
        secrets = [(i).to_bytes(32, 'big') for i in [70, 71, 72]]
        pubkeys = [public_key(secret) for secret in secrets]
        xonly = [pubkey[1:] for pubkey in pubkeys]
        wifs = [bytes_to_wif(secret) for secret in secrets]
        self.private_material.update(wifs)

        def check(wallet, address, expected):
            result = self.export_unchanged(wallet, address)
            assert_equal(set(result), {'keys'})
            records = sorted([
                {'pubkey': public_key(secret).hex(), 'private_key_hex': secret.hex(),
                 'compressed': True, 'roles': sorted(roles)}
                for secret, roles in expected
            ], key=lambda record: record['pubkey'])
            assert_equal(result['keys'], records)
            self.private_material.update(secret.hex() for secret, _ in expected)

        # Each wallet has different internal/script ownership. The unknown internal
        # NUMS point has no known secret; script-only success cannot rely on it.
        for local_internal, local_leaves in [(True, 0), (False, 1), (True, 2)]:
            node.createwallet(f'taproot_{local_internal}_{local_leaves}', blank=True)
            wallet = node.get_wallet_rpc(f'taproot_{local_internal}_{local_leaves}')
            internal = wifs[0] if local_internal else H_POINT
            internal_x = xonly[0] if local_internal else bytes.fromhex(H_POINT)
            expressions = [wifs[i] if i <= local_leaves else pubkeys[i].hex() for i in [1, 2]]
            expected = [(secrets[0], ['taproot_internal'])] if local_internal else []
            expected += [(secrets[i], ['taproot_script']) for i in range(1, local_leaves + 1)]
            for sorted_keys in [False, True]:
                kind = 'sortedmulti_a' if sorted_keys else 'multi_a'
                self.import_descriptor(wallet, f'tr({internal},{kind}(2,{",".join(expressions)}))')
                keys = sorted(xonly[1:]) if sorted_keys else xonly[1:]
                script = CScript([keys[0], OP_CHECKSIG, keys[1], OP_CHECKSIGADD, 2, OP_NUMEQUAL])
                tree = taproot_construct(internal_x, [('multi', script)])
                address = program_to_witness(1, tree.output_pubkey)
                check(wallet, address, expected)
            self.import_descriptor(wallet, f'tr({internal},or_i(pk({expressions[0]}),and_v(v:older(1000),pk({expressions[1]}))))')
            branch = CScript([OP_IF, xonly[1], OP_CHECKSIG, OP_ELSE, 1000, OP_CHECKSEQUENCEVERIFY,
                              OP_VERIFY, xonly[2], OP_CHECKSIG, OP_ENDIF])
            tree = taproot_construct(internal_x, [('miniscript', branch)])
            check(wallet, program_to_witness(1, tree.output_pubkey), expected)
            leaf_a = CScript([xonly[1], OP_CHECKSIG])
            leaf_b = CScript([xonly[2], OP_CHECKSIG])
            self.import_descriptor(wallet, f'tr({internal},{{pk({expressions[0]}),{{pk({expressions[1]}),pk({expressions[0]})}}}})')
            tree = taproot_construct(internal_x, [('a', leaf_a), [('b', leaf_b), ('repeat', leaf_a)]])
            check(wallet, program_to_witness(1, tree.output_pubkey), expected)

        node.createwallet('taproot_parity', blank=True)
        wallet = node.get_wallet_rpc('taproot_parity')
        representatives = [(1).to_bytes(32, 'big'), (ORDER - 1).to_bytes(32, 'big')]
        self.private_material.update(bytes_to_wif(secret) for secret in representatives)
        # A directly held output key is exportable even before importing rawtr().
        for secret in representatives:
            self.import_descriptor(wallet, f'wpkh({bytes_to_wif(secret)})')
        x = public_key(representatives[0])[1:]
        direct = program_to_witness(1, x)
        check(wallet, direct, [(secret, ['taproot_output']) for secret in representatives])
        self.import_descriptor(wallet, f'rawtr({bytes_to_wif(representatives[0])})')
        check(wallet, direct, [(secret, ['taproot_output']) for secret in representatives])
        self.import_descriptor(wallet, f'tr({bytes_to_wif(representatives[0])})')
        check(wallet, program_to_witness(1, taproot_construct(x).output_pubkey),
              [(secret, ['taproot_internal']) for secret in representatives])
        self.import_descriptor(wallet, f'tr({bytes_to_wif(representatives[0])},{{pk({x.hex()}),pk({x.hex()})}})')
        leaf = CScript([x, OP_CHECKSIG])
        tree = taproot_construct(x, [('a', leaf), ('b', leaf)])
        check(wallet, program_to_witness(1, tree.output_pubkey),
              [(secret, ['taproot_internal', 'taproot_script']) for secret in representatives])
        uncompressed_wif = bytes_to_wif(representatives[0], compressed=False)
        self.private_material.add(uncompressed_wif)
        self.import_descriptor(wallet, f'pkh({uncompressed_wif})')
        records = self.export_unchanged(wallet, direct)['keys']
        assert_equal(len(records), 3)
        assert_equal(records[-1], {'pubkey': public_key(representatives[0], compressed=False).hex(),
                                   'private_key_hex': representatives[0].hex(), 'compressed': False,
                                   'roles': ['taproot_output']})
        # Knowing other keys cannot disclose absent tree information.
        missing = program_to_witness(1, taproot_construct(xonly[0]).output_pubkey)
        assert_raises_rpc_error(-5, 'lacks the spending/script information', wallet.exportaddresskey, missing)

    def check_musig_participants(self, root, root_secret, root_chain):
        self.log.info('Check original MuSig participants, independently aggregated public outputs and reused roles')
        node = self.nodes[0]
        secrets = [(i).to_bytes(32, 'big') for i in [90, 91, 92]]
        pubkeys = [public_key(secret) for secret in secrets]
        wifs = [bytes_to_wif(secret) for secret in secrets]
        self.private_material.update(wifs)

        def check(wallet, address, expected):
            result = self.export_unchanged(wallet, address)
            assert_equal(set(result), {'keys'})
            records = sorted([
                {'pubkey': public_key(secret).hex(), 'private_key_hex': secret.hex(),
                 'compressed': True, 'roles': sorted(roles)} for secret, roles in expected
            ], key=lambda record: record['pubkey'])
            assert_equal(result['keys'], records)
            self.private_material.update(secret.hex() for secret, _ in expected)

        for local in [1, 2, 3]:
            for repeated in [False, True]:
                name = f'musig_export_{local}_{repeated}'
                node.createwallet(name, blank=True)
                wallet = node.get_wallet_rpc(name)
                expressions = [wifs[i] if i < local else pubkeys[i].hex() for i in range(3)]
                participants = list(pubkeys)
                if repeated:
                    expressions.append(pubkeys[0].hex())
                    participants.append(pubkeys[0])
                expression = f'musig({",".join(expressions)})'
                aggregate = musig_public_key(participants)
                expected = [(secret, ['musig2_participant']) for secret in secrets[:local]]
                for context in ['tr', 'rawtr', 'leaf']:
                    descriptor = f'{context}({expression})' if context != 'leaf' else f'tr({H_POINT},pk({expression}))'
                    self.import_descriptor(wallet, descriptor)
                    if context == 'tr':
                        output = taproot_construct(aggregate[1:]).output_pubkey
                    elif context == 'rawtr':
                        output = aggregate[1:]
                    else:
                        output = taproot_construct(bytes.fromhex(H_POINT), [('mu', CScript([aggregate[1:], OP_CHECKSIG]))]).output_pubkey
                    check(wallet, program_to_witness(1, output), expected)
                descriptor = f'tr({H_POINT},or_i(pk({expression}),pk({wifs[0]})))'
                self.import_descriptor(wallet, descriptor)
                script = CScript([OP_IF, aggregate[1:], OP_CHECKSIG, OP_ELSE, pubkeys[0][1:], OP_CHECKSIG, OP_ENDIF])
                output = taproot_construct(bytes.fromhex(H_POINT), [('mu', script)]).output_pubkey
                reused = [(secret, ['musig2_participant', 'taproot_script'] if i == 0 else ['musig2_participant'])
                          for i, secret in enumerate(secrets[:local])]
                check(wallet, program_to_witness(1, output), reused)

        first_payload, _ = base58_to_byte(ROOT_XPRV)
        second_payload, _ = base58_to_byte(CHILD_XPRV)
        second_secret, second_chain = second_payload[-32:], second_payload[-65:-33]
        second_root = byte_to_base58(bytes.fromhex('358394') + second_payload[3:], 4)
        roots = [root, second_root]
        materials = [(root_secret, root_chain), (second_secret, second_chain)]
        self.private_material.update([second_root, second_secret.hex()])
        pubs = [byte_to_base58(bytes.fromhex('3587cf') + payload[3:-33] + public_key(secret), 4)
                for payload, (secret, _) in zip([first_payload, second_payload], materials)]
        node.createwallet('musig_export_ranges', blank=True)
        wallet = node.get_wallet_rpc('musig_export_ranges')
        for aggregate_derivation in [False, True]:
            for multipath in [False, True]:
                self.log.info(f'Check MuSig derivation: aggregate={aggregate_derivation}, multipath={multipath}')
                participant_suffix = '/5' if aggregate_derivation else ('/<5;6>/*' if multipath else '/5/*')
                aggregate_suffix = ('/<7;8>/*' if multipath else '/7/*') if aggregate_derivation else ''
                expression = f'musig({roots[0]}{participant_suffix},{pubs[1]}{participant_suffix}){aggregate_suffix}'
                self.import_descriptor(wallet, f'rawtr({expression})', range=[0, 15])
                for branch in ([0, 1] if multipath else [0]):
                    for index in [0, 2]:
                        path = [5] if aggregate_derivation else [5 + branch, index]
                        participants = [derive_leaf(secret, chain, path)[0] for secret, chain in materials]
                        aggregate_path = [7 + branch, index] if aggregate_derivation else []
                        output = musig_public_key([public_key(secret) for secret in participants], aggregate_path)[1:]
                        check(wallet, program_to_witness(1, output), [(participants[0], ['musig2_participant'])])

        self.import_descriptor(wallet, f'rawtr(musig({root}/5/*h,{pubkeys[1].hex()}))', range=[0, 15])
        hardened = derive_leaf(root_secret, root_chain, [5, 0x80000002])[0]
        aggregate = musig_public_key([public_key(hardened), pubkeys[1]])
        check(wallet, program_to_witness(1, aggregate[1:]), [(hardened, ['musig2_participant'])])

        # The original matching descriptor has no participant secrets. Resolve its
        # concrete /5/2 participant in a different ranged descriptor, without roots in results.
        node.createwallet('musig_export_cross', blank=True)
        wallet = node.get_wallet_rpc('musig_export_cross')
        self.import_descriptor(wallet, f'wpkh({roots[0]}/5/*)', range=[0, 3])
        local = derive_leaf(root_secret, root_chain, [5, 2])[0]
        external = derive_leaf(second_secret, second_chain, [5, 2])[0]
        expression = f'musig({pubs[0]}/5/*,{pubs[1]}/5/*)'
        self.import_descriptor(wallet, f'tr({expression},pk({wifs[2]}))', range=[0, 3])
        aggregate = musig_public_key([public_key(local), public_key(external)])
        script = CScript([pubkeys[2][1:], OP_CHECKSIG])
        output = taproot_construct(aggregate[1:], [('normal', script)]).output_pubkey
        check(wallet, program_to_witness(1, output), [(local, ['musig2_participant']), (secrets[2], ['taproot_script'])])

        self.log.info('Check held MuSig SEC identities and exclude the opposite participant point')
        node.createwallet('musig_export_sec', blank=True)
        wallet = node.get_wallet_rpc('musig_export_sec')
        positive = (1).to_bytes(32, 'big')
        negative = (ORDER - 1).to_bytes(32, 'big')
        participant = public_key(positive)
        opposite = public_key(negative)
        for compressed in [False, True]:
            wif = bytes_to_wif(negative, compressed=compressed)
            self.private_material.add(wif)
            self.import_descriptor(wallet, f'pkh({wif})')

        def point_address(pubkey):
            self.import_descriptor(wallet, f'tr(musig({pubkey.hex()},{pubkeys[1].hex()}),pk({wifs[2]}))')
            aggregate = musig_public_key([pubkey, pubkeys[1]])
            output = taproot_construct(aggregate[1:], [('normal', CScript([pubkeys[2][1:], OP_CHECKSIG]))]).output_pubkey
            return program_to_witness(1, output)

        def check_held(address, held):
            result = self.export_unchanged(wallet, address)
            assert_equal(set(result), {'keys'})
            expected = [{'pubkey': pubkeys[2].hex(), 'private_key_hex': secrets[2].hex(),
                         'compressed': True, 'roles': ['taproot_script']}]
            expected += [{'pubkey': public_key(secret, compressed).hex(), 'private_key_hex': secret.hex(),
                          'compressed': compressed, 'roles': ['musig2_participant']} for secret, compressed in held]
            assert_equal(result['keys'], sorted(expected, key=lambda record: record['pubkey']))
            self.private_material.update(secret.hex() for secret, _ in held)

        address = point_address(participant)
        check_held(address, [])
        uncompressed_wif = bytes_to_wif(positive, compressed=False)
        self.private_material.add(uncompressed_wif)
        self.import_descriptor(wallet, f'pkh({uncompressed_wif})')
        check_held(address, [(positive, False)])
        compressed_wif = bytes_to_wif(positive)
        self.private_material.add(compressed_wif)
        self.import_descriptor(wallet, f'wpkh({compressed_wif})')
        check_held(address, [(positive, True), (positive, False)])
        check_held(point_address(opposite), [(negative, True), (negative, False)])

    def check_taproot_key_reload(self):
        self.log.info('Check stored Taproot key identities across wallet reload and encryption')
        node = self.nodes[0]
        for kind in ['tr', 'rawtr']:
            for value in [1, ORDER - 1]:
                name = f'identity_{kind}_{value == 1}'
                node.createwallet(name, blank=True)
                wallet = node.get_wallet_rpc(name)
                secret = value.to_bytes(32, 'big')
                pubkey = public_key(secret)
                opposite = public_key((ORDER - value).to_bytes(32, 'big'))
                self.import_descriptor(wallet, f'{kind}({bytes_to_wif(secret)})')
                addresses = [key_to_p2pkh(pubkey), key_to_p2wpkh(pubkey), key_to_p2sh_p2wpkh(pubkey)]
                for phase in ['imported', 'reloaded', 'encrypted']:
                    self.log.info(f'Check {kind} WIF with public-key prefix {pubkey[0]:02x}, {phase}')
                    if phase != 'imported':
                        if phase == 'encrypted':
                            wallet.encryptwallet('test')
                        node.unloadwallet(name)
                        node.loadwallet(name)
                        wallet = node.get_wallet_rpc(name)
                        if phase == 'encrypted':
                            wallet.walletpassphrase('test', 60)
                    self.check_export(wallet, addresses[0], secret, True)
                    self.check_export(wallet, addresses[1], secret, True, witness=True)
                    self.check_script_export(wallet, addresses[2], [(secret, True)])
                    for address_for_key in [key_to_p2pkh, key_to_p2wpkh]:
                        self.check_error(-5, 'No private key for this address', wallet.exportaddresskey, address_for_key(opposite))

    def check_xonly_hd_root(self, root, root_secret, root_chain):
        self.log.info('Check an odd-Y HD root reused as an x-only internal or leaf key')
        node = self.nodes[0]
        assert_equal(public_key(root_secret)[0], 3)
        child, _ = derive_leaf(root_secret, root_chain, [0])
        for root_internal in [False, True]:
            name = f'identity_hd_{root_internal}'
            node.createwallet(name, blank=True)
            wallet = node.get_wallet_rpc(name)
            internal, leaf = (root_secret, child) if root_internal else (child, root_secret)
            root_xonly = public_key(root_secret)[1:].hex()
            internal_expression, leaf_expression = (root_xonly, f'{root}/0') if root_internal else (f'{root}/0', root_xonly)
            self.import_descriptor(wallet, f'tr({internal_expression},pk({leaf_expression}))')
            tree = taproot_construct(public_key(internal)[1:], [('leaf', CScript([public_key(leaf)[1:], OP_CHECKSIG]))])
            address = program_to_witness(1, tree.output_pubkey)
            expected = sorted([
                {'pubkey': public_key(secret).hex(), 'private_key_hex': secret.hex(), 'compressed': True, 'roles': [role]}
                for secret, role in [(internal, 'taproot_internal'), (leaf, 'taproot_script')]
            ], key=lambda record: record['pubkey'])
            for reload in [False, True]:
                if reload:
                    node.unloadwallet(name)
                    node.loadwallet(name)
                    wallet = node.get_wallet_rpc(name)
                assert_equal(self.export_unchanged(wallet, address)['keys'], expected)
                output = self.export_unchanged(wallet, address, 'taproot_output')['keys']
                assert_equal(len(output), 1)
                assert_equal(output[0]['private_key_hex'], taproot_output_secret(internal, tree.merkle_root).hex())
                assert_equal(output[0]['roles'], ['taproot_output'])

    def check_taproot_output_keys(self):
        self.log.info('Check BIP341 output scalars and actual key-path spends')
        vector_path = os.path.join(self.config['environment']['SRCDIR'], 'src', 'test', 'data', 'bip341_wallet_vectors.json')
        with open(vector_path, encoding='utf8') as vector_file:
            vectors = json.load(vector_file)
        for spending in vectors['keyPathSpending']:
            for vector in spending['inputSpending']:
                given = vector['given']
                root = bytes.fromhex(given['merkleRoot']) if given['merkleRoot'] else b''
                assert_equal(taproot_output_secret(bytes.fromhex(given['internalPrivkey']), root).hex(), vector['intermediary']['tweakedPrivkey'])

        node = self.nodes[0]
        node.createwallet('output_keys', blank=True)
        wallet = node.get_wallet_rpc('output_keys')
        funding = node.get_wallet_rpc('funding')
        mining = funding.getnewaddress()
        spend_count = 0

        def check(address, expected):
            result = self.export_unchanged(wallet, address, 'taproot_output')
            records = sorted([{'pubkey': public_key(secret, compressed).hex(), 'private_key_hex': secret.hex(),
                               'compressed': compressed, 'roles': ['taproot_output']}
                              for secret, compressed in expected], key=lambda record: record['pubkey'])
            assert_equal(result, {'keys': records})
            self.private_material.update(secret.hex() for secret, _ in expected)
            return result['keys']

        def spend(address, output_key):
            nonlocal spend_count
            txid = funding.sendtoaddress(address, 1)
            self.generatetoaddress(node, 1, mining)
            previous = tx_from_hex(funding.gettransaction(txid)['hex'])
            script = bytes.fromhex(node.validateaddress(address)['scriptPubKey'])
            index = next(i for i, output in enumerate(previous.vout) if output.scriptPubKey == script)
            tx = CTransaction()
            tx.vin = [CTxIn(COutPoint(int(txid, 16), index), nSequence=0xfffffffd)]
            destination = funding.getnewaddress('', 'legacy')
            tx.vout = [CTxOut(COIN - 10000, bytes.fromhex(node.validateaddress(destination)['scriptPubKey']))]
            digest = TaprootSignatureHash(tx, [previous.vout[index]], SIGHASH_DEFAULT)
            scalar = bytes.fromhex(output_key['private_key_hex'])
            signature = sign_schnorr(scalar, digest)
            assert verify_schnorr(public_key(scalar)[1:], signature, digest), 'exported output signature does not verify'
            tx.wit.vtxinwit = [CTxInWitness()]
            tx.wit.vtxinwit[0].scriptWitness.stack = [signature]
            assert node.testmempoolaccept([tx.serialize().hex()])[0]['allowed'], 'exported scalar cannot spend requested output'
            node.sendrawtransaction(tx.serialize().hex())
            self.generatetoaddress(node, 1, mining)
            spend_count += 1

        parities = set()
        for value in list(range(1, 13)) + [ORDER - 1]:
            internal = value.to_bytes(32, 'big')
            wif = bytes_to_wif(internal)
            self.private_material.add(wif)
            for has_tree in [False, True]:
                leaf_pubkey = public_key((99).to_bytes(32, 'big'))
                descriptor = f'tr({wif},pk({leaf_pubkey.hex()}))' if has_tree else f'tr({wif})'
                self.import_descriptor(wallet, descriptor)
                scripts = [('leaf', CScript([leaf_pubkey[1:], OP_CHECKSIG]))] if has_tree else []
                tree = taproot_construct(public_key(internal)[1:], scripts)
                address = program_to_witness(1, tree.output_pubkey)
                scalar = taproot_output_secret(internal, tree.merkle_root)
                parities.add((public_key(internal)[0] & 1, public_key(scalar)[0] & 1))
                ordinary = self.export_unchanged(wallet, address)
                assert any(record['private_key_hex'] == internal.hex() and 'taproot_internal' in record['roles'] for record in ordinary['keys']), 'wallet mode lost raw internal scalar'
                records = check(address, [(scalar, True)])
                assert_not_equal(scalar, internal)
                assert_equal(public_key(scalar)[1:], tree.output_pubkey)
                if value in [1, 2, 3, 10, ORDER - 1]:
                    spend(address, records[0])
                if value == 10 and not has_tree:
                    assert records[0]['private_key_hex'].startswith('00'), 'leading-zero output fixture lost padding'
        assert_equal(len(parities), 4)

        direct = (1).to_bytes(32, 'big')
        self.import_descriptor(wallet, f'rawtr({bytes_to_wif(direct)})')
        address = program_to_witness(1, public_key(direct)[1:])
        negative = (ORDER - 1).to_bytes(32, 'big')
        records = check(address, [(direct, True), (negative, True)])
        spend(address, records[0])
        uncompressed_wif = bytes_to_wif(direct, compressed=False)
        self.private_material.add(uncompressed_wif)
        self.import_descriptor(wallet, f'pkh({uncompressed_wif})')
        check(address, [(direct, True), (negative, True), (direct, False)])
        assert_equal(spend_count, 11)
        self.log.info(f'Verified {spend_count} exported-scalar key-path spends and {len(parities)} internal/output parity combinations')

        for name, descriptor in [('script_only_output', f'tr({H_POINT},pk({bytes_to_wif((110).to_bytes(32, "big"))}))'),
                                 ('musig_only_output', f'tr(musig({bytes_to_wif((111).to_bytes(32, "big"))},{bytes_to_wif((112).to_bytes(32, "big"))}))')]:
            node.createwallet(name, blank=True)
            partial = node.get_wallet_rpc(name)
            for value in [110, 111, 112]:
                self.private_material.add(bytes_to_wif(value.to_bytes(32, 'big')))
            self.import_descriptor(partial, descriptor)
            public_descriptor = partial.listdescriptors()['descriptors'][0]['desc']
            target = partial.deriveaddresses(public_descriptor)[0]
            assert partial.exportaddresskey(target)['keys'], 'wallet-only fixture has no local keys'
            assert_raises_rpc_error(-5, 'No local Taproot output private key', partial.exportaddresskey, target, 'taproot_output')

    def check_security_sentinels(self, root, root_secret, root_chain):
        self.log.info('Check all export roles and modes against unrelated, ancestor, index and other-wallet sentinels')
        node = self.nodes[0]
        node.createwallet('security_sentinels', blank=True)
        wallet = node.get_wallet_rpc('security_sentinels')
        node.createwallet('security_foreign', blank=True)
        foreign_wallet = node.get_wallet_rpc('security_foreign')
        fixed, foreign, unrelated = [value.to_bytes(32, 'big') for value in [151, 152, 153]]
        fixed_wif, foreign_wif, unrelated_wif = [bytes_to_wif(secret) for secret in [fixed, foreign, unrelated]]
        child = derive_leaf(root_secret, root_chain, [5, 2])[0]
        participant = derive_leaf(root_secret, root_chain, [5])[0]
        foreign_payload, _ = base58_to_byte(CHILD_XPRV)
        foreign_root_secret, foreign_chain = foreign_payload[-32:], foreign_payload[-65:-33]
        foreign_root = byte_to_base58(bytes.fromhex('358394') + foreign_payload[3:], 4)
        foreign_xpub = byte_to_base58(bytes.fromhex('3587cf') + foreign_payload[3:-33] + public_key(foreign_root_secret), 4)
        foreign_participant = derive_leaf(foreign_root_secret, foreign_chain, [5])[0]
        foreign_pub = public_key(foreign)
        child_pub = public_key(child)
        fixed_pub = public_key(fixed)
        forbidden = [root_secret, foreign, unrelated, foreign_root_secret, foreign_participant]
        forbidden += [derive_leaf(root_secret, root_chain, path)[0]
                      for path in [[5, 0], [5, 1], [5, 3]] + [[9, index] for index in range(4)]]
        self.private_material.update(secret.hex() for secret in forbidden + [fixed, child, participant])
        self.private_material.update(bytes_to_wif(secret) for secret in forbidden + [fixed, child, participant])
        self.private_material.update([fixed_wif, foreign_wif, unrelated_wif, foreign_root])
        self.import_descriptor(foreign_wallet, f'wpkh({foreign_wif})')
        self.import_descriptor(foreign_wallet, f'wpkh({foreign_root}/5)')
        self.import_descriptor(wallet, f'wpkh({unrelated_wif})')
        self.import_descriptor(wallet, f'wpkh({root}/9/*)', range=[0, 3])
        # The local descriptor root derives exactly index 2. Its ancestor and other
        # cached indices are not candidates merely because the provider holds them.
        expression = f'{root}/5/*'
        scripts = keys_to_multisig_script([child_pub, foreign_pub, fixed_pub], k=2)
        leaf = CScript([fixed_pub[1:], OP_CHECKSIG, foreign_pub[1:], OP_CHECKSIGADD, 2, OP_NUMEQUAL])
        tree = taproot_construct(child_pub[1:], [('leaf', leaf)])
        aggregate = musig_public_key([child_pub, foreign_pub])
        musig_tree = taproot_construct(aggregate[1:], [('leaf', CScript([fixed_pub[1:], OP_CHECKSIG]))])
        derived_aggregate = musig_public_key([public_key(participant), public_key(foreign_participant)], [7, 2])
        cases = [
            ('script', f'wsh(multi(2,{expression},{foreign_pub.hex()},{fixed_wif}))',
             script_to_p2wsh(scripts), [(child, ['script']), (fixed, ['script'])], None),
            ('tree', f'tr({expression},multi_a(2,{fixed_wif},{foreign_pub.hex()}))',
             program_to_witness(1, tree.output_pubkey), [(child, ['taproot_internal']), (fixed, ['taproot_script'])],
             taproot_output_secret(child, tree.merkle_root)),
            ('direct', f'rawtr({expression})', program_to_witness(1, child_pub[1:]),
             [(child, ['taproot_output'])], child),
            ('participant', f'tr(musig({expression},{foreign_pub.hex()}),pk({fixed_wif}))',
             program_to_witness(1, musig_tree.output_pubkey), [(child, ['musig2_participant']), (fixed, ['taproot_script'])], None),
            ('aggregate_child', f'rawtr(musig({root}/5,{foreign_xpub}/5)/7/*)',
             program_to_witness(1, derived_aggregate[1:]), [(participant, ['musig2_participant'])], None),
        ]
        funding = node.get_wallet_rpc('funding')
        for name, descriptor, address, _, _ in cases:
            self.import_descriptor(wallet, descriptor, range=[0, 3])
            wallet.setlabel(address, f'sentinel_{name}')
            funding.sendtoaddress(address, 1)
        self.generatetoaddress(node, 1, funding.getnewaddress())
        coins = wallet.listunspent()
        assert coins, 'sentinel fixture has no tracked coins'
        wallet.lockunspent(False, [{'txid': coins[0]['txid'], 'vout': coins[0]['vout']}], True)
        node.unloadwallet('security_sentinels')
        node.loadwallet('security_sentinels')
        wallet = node.get_wallet_rpc('security_sentinels')
        before = self.snapshot(wallet, [])
        foreign_before = self.snapshot(foreign_wallet, [])
        assert_greater_than(before['wallet']['txcount'], 0)
        assert before['labels'], 'sentinel state preconditions missing'
        assert before['locked_coins'], 'sentinel state preconditions missing'
        seen_roles = set()
        positive = denied = 0
        for name, _, address, held, output in cases:
            for mode in ['wallet', 'taproot_output']:
                if mode == 'taproot_output' and output is None:
                    code, message = (-8, 'requires a Taproot address') if name == 'script' else (-5, 'No local Taproot output private key')
                    for _ in range(2):
                        self.check_error(code, message, wallet.exportaddresskey, address, mode)
                    denied += 1
                    continue
                expected = held if mode == 'wallet' else [(output, ['taproot_output'])]
                self.private_material.update(secret.hex() for secret, _ in expected)
                records = sorted([{'pubkey': public_key(secret).hex(), 'private_key_hex': secret.hex(),
                                   'compressed': True, 'roles': sorted(roles)} for secret, roles in expected],
                                 key=lambda record: record['pubkey'])
                result = self.export_unchanged(wallet, address, mode)
                assert_equal(result, {'keys': records})
                serialized = json.dumps(result)
                assert all(secret.hex() not in serialized for secret in forbidden), 'unrelated/ancestor/index/other-wallet secret exported'
                seen_roles.update(role for record in result['keys'] for role in record['roles'])
                positive += 1
        assert_equal(seen_roles, {'script', 'taproot_internal', 'taproot_script', 'taproot_output', 'musig2_participant'})
        assert_equal(positive, 7)
        assert_equal(denied, 3)
        assert_equal(self.snapshot(wallet, []), before)
        assert_equal(self.snapshot(foreign_wallet, []), foreign_before)
        self.log.info('Verified seven positive and three denied sentinel role/mode cases with repeated calls and unchanged funded wallet state')

    def check_taproot_script_spending(self):
        self.log.info('Spend script-only Taproot outputs using exported default-mode scalars')
        node = self.nodes[0]
        funding = node.get_wallet_rpc('funding')
        mining = funding.getnewaddress()
        secrets = [value.to_bytes(32, 'big') for value in [120, 121, 122]]
        pubkeys = [public_key(secret) for secret in secrets]
        xonly = [pubkey[1:] for pubkey in pubkeys]
        wifs = [bytes_to_wif(secret) for secret in secrets]
        self.private_material.update(wifs)
        preimage = b'\x11' * 32
        digest = hashlib.sha256(preimage).digest()
        cases = [('pk', f'pk({wifs[0]})', CScript([xonly[0], OP_CHECKSIG]), [0], [None])]
        for sorted_keys in [False, True]:
            indices = [0, 1, 2] if sorted_keys else [0, 1]
            kind = 'sortedmulti_a' if sorted_keys else 'multi_a'
            expression = f'{kind}(2,{",".join([wifs[0]] + [pubkeys[i].hex() for i in indices[1:]])})'
            ordered = sorted(indices, key=lambda i: xonly[i]) if sorted_keys else indices
            script = CScript([xonly[ordered[0]], OP_CHECKSIG] +
                             [item for i in ordered[1:] for item in [xonly[i], OP_CHECKSIGADD]] + [2, OP_NUMEQUAL])
            cases.append((kind, expression, script, [0], [ordered]))
        after_script = CScript([OP_IF, xonly[0], OP_CHECKSIG, OP_ELSE, 1, OP_CHECKLOCKTIMEVERIFY,
                               OP_VERIFY, xonly[1], OP_CHECKSIG, OP_ENDIF])
        cases.append(('miniscript_after', f'or_i(pk({wifs[0]}),and_v(v:after(1),pk({wifs[1]})))', after_script, [0, 1], [True, False]))
        hash_script = CScript([OP_IF, xonly[0], OP_CHECKSIG, OP_ELSE, OP_SIZE, 32, OP_EQUALVERIFY,
                              OP_SHA256, digest, OP_EQUALVERIFY, xonly[1], OP_CHECKSIG, OP_ENDIF])
        cases.append(('miniscript_hash', f'or_i(pk({wifs[0]}),and_v(v:sha256({digest.hex()}),pk({wifs[1]})))', hash_script, [0, 1], [True, False]))
        spent = 0
        partial_rejections = 0
        for name, expression, script, local_indices, branches in cases:
            wallet_name = f'taproot_script_spend_{name}'
            node.createwallet(wallet_name, blank=True)
            wallet = node.get_wallet_rpc(wallet_name)
            self.import_descriptor(wallet, f'tr({H_POINT},{expression})')
            tree = taproot_construct(bytes.fromhex(H_POINT), [('leaf', script)])
            address = program_to_witness(1, tree.output_pubkey)
            leaf = tree.leaves['leaf']
            control = bytes([leaf.version | tree.negflag]) + tree.internal_pubkey + leaf.merklebranch
            for branch in branches:
                txid = funding.sendtoaddress(address, 1)
                self.generatetoaddress(node, 1, mining)
                previous = tx_from_hex(funding.gettransaction(txid)['hex'])
                index = next(i for i, output in enumerate(previous.vout) if output.scriptPubKey == tree.scriptPubKey)
                exported = self.export_unchanged(wallet, address)
                expected = sorted([{'pubkey': pubkeys[i].hex(), 'private_key_hex': secrets[i].hex(),
                                    'compressed': True, 'roles': ['taproot_script']} for i in local_indices],
                                  key=lambda record: record['pubkey'])
                assert_equal(exported, {'keys': expected})
                self.private_material.update(record['private_key_hex'] for record in exported['keys'])
                assert_raises_rpc_error(-5, 'No local Taproot output private key', wallet.exportaddresskey, address, 'taproot_output')
                tx = CTransaction()
                tx.nLockTime = 1
                tx.vin = [CTxIn(COutPoint(int(txid, 16), index), nSequence=0xfffffffd)]
                destination = funding.getnewaddress('', 'legacy')
                tx.vout = [CTxOut(COIN - 10000, bytes.fromhex(node.validateaddress(destination)['scriptPubKey']))]
                sighash = TaprootSignatureHash(tx, [previous.vout[index]], SIGHASH_DEFAULT, scriptpath=True,
                                              leaf_script=script, codeseparator_pos=0xffffffff)
                signatures = {}
                for record in exported['keys']:
                    scalar = bytes.fromhex(record['private_key_hex'])
                    public = public_key(scalar)[1:]
                    signature = sign_schnorr(scalar, sighash)
                    assert verify_schnorr(public, signature, sighash), 'exported leaf signature does not verify'
                    signatures[public] = signature

                def set_witness():
                    if isinstance(branch, list):
                        stack = [signatures.get(xonly[i], b'') for i in reversed(branch)]
                    elif branch is None:
                        stack = [signatures[xonly[0]]]
                    elif branch:
                        stack = [signatures[xonly[0]], b'\x01']
                    else:
                        stack = [signatures[xonly[1]]] + ([preimage] if name == 'miniscript_hash' else []) + [b'']
                    tx.wit.vtxinwit = [CTxInWitness()]
                    tx.wit.vtxinwit[0].scriptWitness.stack = stack + [script, control]

                set_witness()
                if isinstance(branch, list):
                    partial = node.testmempoolaccept([tx.serialize().hex()])[0]
                    assert not partial['allowed'] and 'script' in partial['reject-reason'], 'partial local set unexpectedly satisfied threshold'
                    partial_rejections += 1
                    signature = sign_schnorr(secrets[1], sighash)
                    assert verify_schnorr(xonly[1], signature, sighash), 'synthetic cosigner signature does not verify'
                    signatures[xonly[1]] = signature
                    set_witness()
                assert node.testmempoolaccept([tx.serialize().hex()])[0]['allowed'], 'exported leaf keys do not spend the committed script'
                node.sendrawtransaction(tx.serialize().hex())
                self.generatetoaddress(node, 1, mining)
                spent += 1
        assert_equal(spent, 7)
        assert_equal(partial_rejections, 2)
        self.log.info(f'Verified {spent} exported-scalar Taproot script-path spends and {partial_rejections} incomplete local threshold rejections')

    def run_test(self):
        node = self.nodes[0]
        self.private_material = set()
        for mode in ['wallet', 'taproot_output']:
            self.check_error(-18, 'No wallet is loaded', node.exportaddresskey, 'invalid', mode)
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
        assert_equal(node.exportaddresskey(addresses[0]), wallet.exportaddresskey(addresses[0]))
        direct = program_to_witness(1, public_key(fixtures[0][1])[1:])
        for mode in ['wallet', 'taproot_output']:
            assert_equal(node.exportaddresskey(direct, mode), wallet.exportaddresskey(direct, mode))
        assert_equal(rpc_wallet.exportaddresskey(address=direct, key_type=None), wallet.exportaddresskey(direct))
        node.createwallet('scripts', blank=True)
        self.check_script_constructions(node.get_wallet_rpc('scripts'), root, root_secret, root_chain)
        self.check_cross_manager_resolution(root, root_secret, root_chain)
        self.check_spending_and_state(root_secret, root_chain)
        self.check_taproot_roles()
        self.check_musig_participants(root, root_secret, root_chain)
        self.check_taproot_key_reload()
        self.check_xonly_hd_root(root, root_secret, root_chain)
        self.check_taproot_output_keys()
        self.check_taproot_script_spending()
        self.check_security_sentinels(root, root_secret, root_chain)
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
        assert_raises_rpc_error(-5, 'lacks the spending/script information', wallet.exportaddresskey, taproot)
        assert_raises_rpc_error(-5, 'spending construction is not supported', wallet.exportaddresskey, program_to_witness(1, bytes(32)))
        assert_raises_rpc_error(-5, 'spending construction is not supported', wallet.exportaddresskey, program_to_witness(2, bytes(32)))
        assert_raises_rpc_error(-5, 'lacks the spending/script information', wallet.exportaddresskey, taproot, 'taproot_output')
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

        self.check_failure_matrix(wallet, fixtures)

        # Normal daemon RPC logging must never include exported scalars, WIFs,
        # private descriptors or the synthetic HD root. Fixed diagnostics avoid
        # printing key material even if an assertion fails.
        debug_log = node.debug_log_path.read_text(encoding='utf-8')
        assert all(material not in debug_log for material in self.private_material), 'private material found in normal daemon log'
        assert not re.search(r'\b(?:tprv|xprv)[1-9A-HJ-NP-Za-km-z]+', debug_log), 'extended private key found in normal daemon log'
        self.log.info('Address-key export regression checks passed')


if __name__ == '__main__':
    ExportAddressKeyTest(__file__).main()
