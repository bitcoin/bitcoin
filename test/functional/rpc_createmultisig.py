#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test multisig RPCs"""
import decimal
import itertools
import json
import os

from test_framework.address import address_to_scriptpubkey
from test_framework.descriptors import descsum_create, drop_origins
from test_framework.key import ECPubKey
from test_framework.messages import COIN
from test_framework.script_util import keys_to_multisig_script
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
    assert_equal,
)
from test_framework.wallet_util import generate_keypair
from test_framework.wallet import (
    MiniWallet,
    getnewdestination,
)

class RpcCreateMultiSigTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.supports_cli = False
        self.enable_wallet_if_possible()

    def create_keys(self, num_keys):
        self.pub = []
        self.priv = []
        for _ in range(num_keys):
            privkey, pubkey = generate_keypair(wif=True)
            self.pub.append(pubkey.hex())
            self.priv.append(privkey)

    def create_wallet(self, node, wallet_name):
        node.createwallet(wallet_name=wallet_name, disable_private_keys=True)
        return node.get_wallet_rpc(wallet_name)

    def run_test(self):
        node0, node1, node2 = self.nodes
        self.wallet = MiniWallet(test_node=node0)

        if self.is_wallet_compiled():
            self.check_addmultisigaddress_errors()

        self.log.info('Generating blocks ...')
        self.generate(self.wallet, 149)

        wallet_multi = self.create_wallet(node1, 'wmulti') if self._requires_wallet else None
        self.create_keys(21)  # max number of allowed keys + 1
        m_of_n = [(2, 3), (3, 3), (2, 5), (3, 5), (10, 15), (15, 15)]
        for (sigs, keys) in m_of_n:
            for output_type in ["bech32", "p2sh-segwit", "legacy"]:
                self.do_multisig(keys, sigs, output_type, wallet_multi)

        self.test_multisig_script_limit(wallet_multi)
        self.test_mixing_uncompressed_and_compressed_keys(node0, wallet_multi)
        self.test_sortedmulti_descriptors_bip67()

        # Check that bech32m is currently not allowed
        assert_raises_rpc_error(-5, "createmultisig cannot create bech32m multisig addresses", self.nodes[0].createmultisig, 2, self.pub, "bech32m")

        self.log.info('Check correct encoding of multisig script for all n (1..20)')
        for nkeys in range(1, 20+1):
            keys = [self.pub[0]]*nkeys
            expected_ms_script = keys_to_multisig_script(keys, k=nkeys)  # simply use n-of-n
            # note that the 'legacy' address type fails for n values larger than 15
            # due to exceeding the P2SH size limit (520 bytes), so we use 'bech32' instead
            # (for the purpose of this encoding test, we don't care about the resulting address)
            res = self.nodes[0].createmultisig(nrequired=nkeys, keys=keys, address_type='bech32')
            assert_equal(res['redeemScript'], expected_ms_script.hex())

    def check_addmultisigaddress_errors(self):
        if self.options.descriptors:
            return
        self.log.info('Check that addmultisigaddress fails when the private keys are missing')
        addresses = [self.nodes[1].getnewaddress(address_type='legacy') for _ in range(2)]
        assert_raises_rpc_error(-5, 'no full public key for address', lambda: self.nodes[0].addmultisigaddress(nrequired=1, keys=addresses))
        for a in addresses:
            # Importing all addresses should not change the result
            self.nodes[0].importaddress(a)
        assert_raises_rpc_error(-5, 'no full public key for address', lambda: self.nodes[0].addmultisigaddress(nrequired=1, keys=addresses))

        # Bech32m address type is disallowed for legacy wallets
        pubs = [self.nodes[1].getaddressinfo(addr)["pubkey"] for addr in addresses]
        assert_raises_rpc_error(-5, "Bech32m multisig addresses cannot be created with legacy wallets", self.nodes[0].addmultisigaddress, 2, pubs, "", "bech32m")

    def test_multisig_script_limit(self, wallet_multi):
        node1 = self.nodes[1]
        pubkeys = self.pub[0:20]

        self.log.info('Test legacy redeem script max size limit')
        assert_raises_rpc_error(-8, "redeemScript exceeds size limit: 684 > 520", node1.createmultisig, 16, pubkeys, 'legacy')

        self.log.info('Test valid 16-20 multisig p2sh-legacy and bech32 (no wallet)')
        self.do_multisig(nkeys=20, nsigs=16, output_type="p2sh-segwit", wallet_multi=None)
        self.do_multisig(nkeys=20, nsigs=16, output_type="bech32", wallet_multi=None)

        self.log.info('Test invalid 16-21 multisig p2sh-legacy and bech32 (no wallet)')
        assert_raises_rpc_error(-8, "Number of keys involved in the multisignature address creation > 20", node1.createmultisig, 16, self.pub, 'p2sh-segwit')
        assert_raises_rpc_error(-8, "Number of keys involved in the multisignature address creation > 20", node1.createmultisig, 16, self.pub, 'bech32')

        # Check legacy wallet related command
        self.log.info('Test legacy redeem script max size limit (with wallet)')
        if wallet_multi is not None and not self.options.descriptors:
            assert_raises_rpc_error(-8, "redeemScript exceeds size limit: 684 > 520", wallet_multi.addmultisigaddress, 16, pubkeys, '', 'legacy')

            self.log.info('Test legacy wallet unsupported operation. 16-20 multisig p2sh-legacy and bech32 generation')
            # Due an internal limitation on legacy wallets, the redeem script limit also applies to p2sh-segwit and bech32 (even when the scripts are valid)
            # We take this as a "good thing" to tell users to upgrade to descriptors.
            assert_raises_rpc_error(-4, "Unsupported multisig script size for legacy wallet. Upgrade to descriptors to overcome this limitation for p2sh-segwit or bech32 scripts", wallet_multi.addmultisigaddress, 16, pubkeys, '', 'p2sh-segwit')
            assert_raises_rpc_error(-4, "Unsupported multisig script size for legacy wallet. Upgrade to descriptors to overcome this limitation for p2sh-segwit or bech32 scripts", wallet_multi.addmultisigaddress, 16, pubkeys, '', 'bech32')

    def do_multisig(self, nkeys, nsigs, output_type, wallet_multi):
        node0, node1, node2 = self.nodes
        pub_keys = self.pub[0: nkeys]
        priv_keys = self.priv[0: nkeys]

        # Construct the expected descriptor
        desc = 'multi({},{})'.format(nsigs, ','.join(pub_keys))
        if output_type == 'legacy':
            desc = 'sh({})'.format(desc)
        elif output_type == 'p2sh-segwit':
            desc = 'sh(wsh({}))'.format(desc)
        elif output_type == 'bech32':
            desc = 'wsh({})'.format(desc)
        desc = descsum_create(desc)

        msig = node2.createmultisig(nsigs, pub_keys, output_type)
        assert 'warnings' not in msig
        madd = msig["address"]
        mredeem = msig["redeemScript"]
        assert_equal(desc, msig['descriptor'])
        if output_type == 'bech32':
            assert madd[0:4] == "bcrt"  # actually a bech32 address

        if wallet_multi is not None:
            # compare against addmultisigaddress
            msigw = wallet_multi.addmultisigaddress(nsigs, pub_keys, None, output_type)
            maddw = msigw["address"]
            mredeemw = msigw["redeemScript"]
            assert_equal(desc, drop_origins(msigw['descriptor']))
            # addmultisigiaddress and createmultisig work the same
            assert maddw == madd
            assert mredeemw == mredeem

        spk = address_to_scriptpubkey(madd)
        value = decimal.Decimal("0.00004000")
        tx = self.wallet.send_to(from_node=self.nodes[0], scriptPubKey=spk, amount=int(value * COIN))
        prevtxs = [{"txid": tx["txid"], "vout": tx["sent_vout"], "scriptPubKey": spk.hex(), "redeemScript": mredeem, "amount": value}]

        self.generate(node0, 1)

        outval = value - decimal.Decimal("0.00002000")  # deduce fee (must be higher than the min relay fee)
        # send coins to node2 when wallet is enabled
        node2_balance = node2.getbalances()['mine']['trusted'] if self.is_wallet_compiled() else 0
        out_addr = node2.getnewaddress() if self.is_wallet_compiled() else getnewdestination('bech32')[2]
        rawtx = node2.createrawtransaction([{"txid": tx["txid"], "vout": tx["sent_vout"]}], [{out_addr: outval}])

        prevtx_err = dict(prevtxs[0])
        del prevtx_err["redeemScript"]

        assert_raises_rpc_error(-8, "Missing redeemScript/witnessScript", node2.signrawtransactionwithkey, rawtx, priv_keys[0:nsigs-1], [prevtx_err])

        # if witnessScript specified, all ok
        prevtx_err["witnessScript"] = prevtxs[0]["redeemScript"]
        node2.signrawtransactionwithkey(rawtx, priv_keys[0:nsigs-1], [prevtx_err])

        # both specified, also ok
        prevtx_err["redeemScript"] = prevtxs[0]["redeemScript"]
        node2.signrawtransactionwithkey(rawtx, priv_keys[0:nsigs-1], [prevtx_err])

        # redeemScript mismatch to witnessScript
        prevtx_err["redeemScript"] = "6a" # OP_RETURN
        assert_raises_rpc_error(-8, "redeemScript does not correspond to witnessScript", node2.signrawtransactionwithkey, rawtx, priv_keys[0:nsigs-1], [prevtx_err])

        # redeemScript does not match scriptPubKey
        del prevtx_err["witnessScript"]
        assert_raises_rpc_error(-8, "redeemScript/witnessScript does not match scriptPubKey", node2.signrawtransactionwithkey, rawtx, priv_keys[0:nsigs-1], [prevtx_err])

        # witnessScript does not match scriptPubKey
        prevtx_err["witnessScript"] = prevtx_err["redeemScript"]
        del prevtx_err["redeemScript"]
        assert_raises_rpc_error(-8, "redeemScript/witnessScript does not match scriptPubKey", node2.signrawtransactionwithkey, rawtx, priv_keys[0:nsigs-1], [prevtx_err])

        rawtx2 = node2.signrawtransactionwithkey(rawtx, priv_keys[0:nsigs - 1], prevtxs)
        rawtx3 = node2.signrawtransactionwithkey(rawtx2["hex"], [priv_keys[-1]], prevtxs)
        assert rawtx3['complete']

        tx = node0.sendrawtransaction(rawtx3["hex"], 0)
        blk = self.generate(node0, 1)[0]
        assert tx in node0.getblock(blk)["tx"]

        # When the wallet is enabled, assert node2 sees the incoming amount
        if self.is_wallet_compiled():
            assert_equal(node2.getbalances()['mine']['trusted'], node2_balance + outval)

        txinfo = node0.getrawtransaction(tx, True, blk)
        self.log.info("n/m=%d/%d %s size=%d vsize=%d weight=%d" % (nsigs, nkeys, output_type, txinfo["size"], txinfo["vsize"], txinfo["weight"]))

    def test_mixing_uncompressed_and_compressed_keys(self, node, wallet_multi):
        self.log.info('Mixed compressed and uncompressed multisigs are not allowed')
        pk0, pk1, pk2 = [getnewdestination('bech32')[0].hex() for _ in range(3)]

        # decompress pk2
        pk_obj = ECPubKey()
        pk_obj.set(bytes.fromhex(pk2))
        pk_obj.compressed = False
        pk2 = pk_obj.get_bytes().hex()

        # Check all permutations of keys because order matters apparently
        for keys in itertools.permutations([pk0, pk1, pk2]):
            # Results should be the same as this legacy one
            legacy_addr = node.createmultisig(2, keys, 'legacy')['address']

            if wallet_multi is not None:
                # 'addmultisigaddress' should return the same address
                result = wallet_multi.addmultisigaddress(2, keys, '', 'legacy')
                assert_equal(legacy_addr, result['address'])
                assert 'warnings' not in result

            # Generate addresses with the segwit types. These should all make legacy addresses
            err_msg = ["Unable to make chosen address type, please ensure no uncompressed public keys are present."]

            for addr_type in ['bech32', 'p2sh-segwit']:
                result = self.nodes[0].createmultisig(nrequired=2, keys=keys, address_type=addr_type)
                assert_equal(legacy_addr, result['address'])
                assert_equal(result['warnings'], err_msg)

                if wallet_multi is not None:
                    result = wallet_multi.addmultisigaddress(nrequired=2, keys=keys, address_type=addr_type)
                    assert_equal(legacy_addr, result['address'])
                    assert_equal(result['warnings'], err_msg)

    def test_sortedmulti_descriptors_bip67(self):
        self.log.info('Testing sortedmulti descriptors with BIP 67 test vectors')
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), 'data/rpc_bip67.json'), encoding='utf-8') as f:
            vectors = json.load(f)

        for t in vectors:
            key_str = ','.join(t['keys'])
            desc = descsum_create('sh(sortedmulti(2,{}))'.format(key_str))
            assert_equal(self.nodes[0].deriveaddresses(desc)[0], t['address'])
            sorted_key_str = ','.join(t['sorted_keys'])
            sorted_key_desc = descsum_create('sh(multi(2,{}))'.format(sorted_key_str))
            assert_equal(self.nodes[0].deriveaddresses(sorted_key_desc)[0], t['address'])


if __name__ == '__main__':
    RpcCreateMultiSigTest().main()
