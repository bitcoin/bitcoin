#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test cisa() descriptors in the wallet: addresses, receiving and spending with BIP460 signature aggregation."""

import re

from test_framework.descriptors import descsum_create
from test_framework.psbt import PSBT, PSBT_IN_TAP_KEY_SIG
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

TPRV = "tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK"


class WalletCisaTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def create_wallet(self, name, desc):
        self.nodes[0].createwallet(name)
        wallet = self.nodes[0].get_wallet_rpc(name)
        assert_equal(wallet.importdescriptors([{"desc": descsum_create(desc), "active": True, "timestamp": "now"}])[0]["success"], True)
        return wallet

    def fund(self, wallet, count):
        """Fund count new witness v2 addresses of wallet and return their outpoints."""
        addrs = [wallet.getnewaddress("", "bech32m") for _ in range(count)]
        for addr in addrs:
            assert_equal(wallet.getaddressinfo(addr)["witness_version"], 2)
            self.funder.sendtoaddress(addr, 2)
        self.generate(self.nodes[0], 1)
        return [{"txid": u["txid"], "vout": u["vout"]} for u in wallet.listunspent() if u["address"] in addrs]

    def funded_psbt(self, wallet, mode, count=1):
        outputs = [{self.funder.getnewaddress(): 1}]
        return wallet.walletcreatefundedpsbt(inputs=self.fund(wallet, count), outputs=outputs, options={"add_inputs": False, "cisa_mode": mode}, psbt_version=0)["psbt"]

    def broadcast(self, hex_tx, witness_sizes):
        txid = self.nodes[0].sendrawtransaction(hex_tx)
        tx = self.nodes[0].getrawtransaction(txid, True)
        assert_equal([[len(elem) // 2 for elem in vin["txinwitness"]] for vin in tx["vin"]], witness_sizes)
        self.generate(self.nodes[0], 1)

    def test_addresses(self, wallet):
        self.log.info("An active cisa() descriptor provides the bech32m addresses")
        addr = wallet.getnewaddress("", "bech32m")
        info = wallet.getaddressinfo(addr)
        assert_equal(info["witness_version"], 2)
        assert_equal(info["ismine"], True)
        assert info["desc"].startswith("cisa([")
        assert_equal(wallet.getaddressinfo(wallet.getrawchangeaddress("bech32m"))["witness_version"], 2)

        txid = self.funder.sendtoaddress(addr, 1)
        self.generate(self.nodes[0], 1)
        utxo, = wallet.listunspent()
        assert_equal(utxo["txid"], txid)
        assert_equal(utxo["desc"], info["desc"])

        self.log.info("Without an aggregation mode a witness v2 input is spent with an opted-out signature")
        tx = self.nodes[0].getrawtransaction(wallet.sendtoaddress(self.funder.getnewaddress(), 0.5), True)
        assert_equal([len(elem) // 2 for elem in tx["vin"][0]["txinwitness"]], [64])
        self.generate(self.nodes[0], 1)

    def test_single_wallet(self, wallet):
        self.log.info("send aggregates the wallet's own inputs")
        inputs = self.fund(wallet, 2)
        res = wallet.send(outputs=[{self.funder.getnewaddress(): 3}], options={"inputs": inputs, "add_inputs": False, "cisa_mode": "halfagg"})
        assert_equal(res["complete"], True)
        tx = self.nodes[0].getrawtransaction(res["txid"], True)
        assert_equal([[len(elem) // 2 for elem in vin["txinwitness"]] for vin in tx["vin"]], [[32], [65]])
        self.generate(self.nodes[0], 1)

        self.log.info("Full aggregation signs in a second round after the nonces")
        inputs = self.fund(wallet, 2)
        res = wallet.send(outputs=[{self.funder.getnewaddress(): 3}], options={"inputs": inputs, "add_inputs": False, "cisa_mode": "fullagg"})
        assert_equal(res["complete"], False)
        for psbt_in in self.nodes[0].decodepsbt(res["psbt"])["inputs"]:
            assert_equal(psbt_in["cisa_mode"], "fullagg")
            assert_equal(len(psbt_in["cisa_fullagg_pubnonce"]), 132)
            assert "cisa_fullagg_partial_sig" not in psbt_in
        res = wallet.walletprocesspsbt(res["psbt"])
        assert_equal(res["complete"], True)
        self.broadcast(res["hex"], [[0], [65]])

        self.log.info("sendall aggregates the witness v2 inputs among the wallet's coins")
        self.fund(wallet, 2)
        res = wallet.sendall(recipients=[self.funder.getnewaddress()], options={"cisa_mode": "halfagg"})
        tx = self.nodes[0].getrawtransaction(res["txid"], True)
        sizes = [[len(elem) // 2 for elem in vin["txinwitness"]] for vin in tx["vin"]]
        assert_equal(sorted(size for size in sizes if len(size) == 1), [[32], [65]])
        self.generate(self.nodes[0], 1)

    def test_two_wallets(self, alice, bob):
        node = self.nodes[0]
        self.log.info("Two wallets form one half-aggregation group, each signing its own input")
        psbt = node.joinpsbts([self.funded_psbt(alice, "halfagg"), self.funded_psbt(bob, "halfagg")])
        assert_equal([psbt_in["cisa_mode"] for psbt_in in node.decodepsbt(psbt)["inputs"]], ["halfagg"] * 2)
        bob_signed = bob.walletprocesspsbt(psbt)
        assert_equal(bob_signed["complete"], False)
        assert_equal(sum("cisa_halfagg_sig" in psbt_in for psbt_in in node.decodepsbt(bob_signed["psbt"])["inputs"]), 1)
        assert_equal(sorted(psbt_in["next"] for psbt_in in node.analyzepsbt(bob_signed["psbt"])["inputs"]), ["finalizer", "updater"])
        alice_signed = alice.walletprocesspsbt(bob_signed["psbt"])
        assert_equal(alice_signed["complete"], True)
        self.broadcast(alice_signed["hex"], [[32], [65]])

        self.log.info("Full aggregation exchanges nonces through combinepsbt before signing")
        psbt = node.joinpsbts([self.funded_psbt(alice, "fullagg"), self.funded_psbt(bob, "fullagg")])
        nonces = [wallet.walletprocesspsbt(psbt)["psbt"] for wallet in (alice, bob)]
        assert_raises_rpc_error(-8, "PSBTs not compatible", node.combinepsbt, [nonces[0], alice.walletprocesspsbt(psbt)["psbt"]])
        psbt = node.combinepsbt(nonces)
        assert all("cisa_fullagg_pubnonce" in psbt_in for psbt_in in node.decodepsbt(psbt)["inputs"])
        assert_equal(node.finalizepsbt(psbt)["complete"], False)
        psbt = node.combinepsbt([wallet.walletprocesspsbt(psbt)["psbt"] for wallet in (alice, bob)])
        assert all("cisa_fullagg_partial_sig" in psbt_in for psbt_in in node.decodepsbt(psbt)["inputs"])
        final = node.finalizepsbt(psbt)
        assert_equal(final["complete"], True)
        self.broadcast(final["hex"], [[0], [65]])

    def test_finalizer(self, wallet):
        self.log.info("An aggregated input carrying an opted-out signature is not finalized")
        signed = wallet.walletprocesspsbt(self.funded_psbt(wallet, "halfagg"), finalize=False)["psbt"]
        psbt = PSBT.from_base64(signed)
        psbt.i[0].map[PSBT_IN_TAP_KEY_SIG] = bytes(64)
        assert_equal(self.nodes[0].finalizepsbt(psbt.to_base64())["complete"], False)
        assert_equal(self.nodes[0].finalizepsbt(signed)["complete"], True)

        self.log.info("descriptorprocesspsbt signs by aggregation mode")
        psbt = wallet.walletcreatefundedpsbt(inputs=self.fund(wallet, 2), outputs=[{self.funder.getnewaddress(): 3}], options={"add_inputs": False})["psbt"]
        desc = descsum_create(f"cisa({TPRV}/86h/1h/0h/<0;1>/*)")
        res = self.nodes[0].descriptorprocesspsbt(psbt=psbt, descriptors=[{"desc": desc, "range": 100}], cisa_mode="halfagg")
        assert_equal(res["complete"], True)
        self.broadcast(res["hex"], [[32], [65]])

    def test_musig(self):
        self.log.info("A MuSig2 aggregate key signs a half-aggregation input with the BIP373 workflow")
        node = self.nodes[0]
        keys = []
        for account in (2, 3):
            node.createwallet(f"key{account}", blank=True)
            key_wallet = node.get_wallet_rpc(f"key{account}")
            key_wallet.importdescriptors([{"desc": descsum_create(f"tr({TPRV}/86h/1h/{account}h/<0;1>/*)"), "timestamp": "now"}])
            pub = re.match(r"tr\((\[.+?\]\w+)/0/\*\)#", key_wallet.listdescriptors()["descriptors"][0]["desc"]).group(1)
            keys.append((f"{TPRV}/86h/1h/{account}h", pub))
        carol = self.create_wallet("carol", f"cisa(musig({keys[0][0]},{keys[1][1]})/<0;1>/*)")
        dave = self.create_wallet("dave", f"cisa(musig({keys[0][1]},{keys[1][0]})/<0;1>/*)")
        addr = carol.getnewaddress("", "bech32m")
        assert_equal(addr, dave.getnewaddress("", "bech32m"))
        self.funder.sendtoaddress(addr, 2)
        self.generate(node, 1)
        utxo, = carol.listunspent()
        psbt = carol.walletcreatefundedpsbt(inputs=[utxo], outputs=[{self.funder.getnewaddress(): 1}], options={"add_inputs": False, "cisa_mode": "halfagg"})["psbt"]
        for _ in range(2):  # nonces, then partial signatures
            psbt = node.combinepsbt([wallet.walletprocesspsbt(psbt)["psbt"] for wallet in (carol, dave)])
        final = node.finalizepsbt(psbt)
        assert_equal(final["complete"], True)
        self.broadcast(final["hex"], [[65]])

    def run_test(self):
        self.funder = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        alice = self.create_wallet("alice", f"cisa({TPRV}/86h/1h/0h/<0;1>/*)")
        bob = self.create_wallet("bob", f"cisa({TPRV}/86h/1h/1h/<0;1>/*)")

        self.test_addresses(alice)
        self.test_single_wallet(alice)
        self.test_two_wallets(alice, bob)
        self.test_finalizer(alice)
        self.test_musig()


if __name__ == "__main__":
    WalletCisaTest(__file__).main()
