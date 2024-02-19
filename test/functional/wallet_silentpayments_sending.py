#!/usr/bin/env python3
from test_framework.address import address_to_scriptpubkey
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.descriptors import descsum_create
from test_framework.messages import COIN
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error
from test_framework.wallet import MiniWallet

SILENT_PAYMENT_ADDRESS="sprt1qqtzwfsu4f34wejks0nxwzed3zq6vh53cg2rnxj9w6ncyrmy95dxx7qnvd47fskn470t9tl4z8a8nul5k3fztquqp4fjrarl7d5lphu7rk52s4hsp"


class SilentTransactioTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [[], ["-txindex=1"], []]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def init_wallet(self, *, node):
        pass

    def watch_only_wallet_send(self):
        self.log.info("Testing if a watch only wallet is not able to send silent transaction")
        self.nodes[0].createwallet(wallet_name='watch_only_wallet', descriptors=True, disable_private_keys=True, blank=True)
        watch_only_wallet = self.nodes[0].get_wallet_rpc('watch_only_wallet')
        desc_import = [{
            "desc": descsum_create("wpkh(tpubD6NzVbkrYhZ4YNXVQbNhMK1WqguFsUXceaVJKbmno2aZ3B6QfbMeraaYvnBSGpV3vxLyTTK9DYT1yoEck4XUScMzXoQ2U2oSmE2JyMedq3H/0/*)"),
            "timestamp": "now",
            "internal": False,
            "active": True,
            "keypool": True,
            "range": [0, 100],
            "watchonly": True,
        }]
        watch_only_wallet.importdescriptors(desc_import)
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 10, watch_only_wallet.getnewaddress())
        self.log.info("Watch-only wallets cannot send coins using silent_payment option")
        assert_raises_rpc_error(
                -4,
                "Silent payments require access to private keys to build transactions.",
                watch_only_wallet.sendtoaddress,
                SILENT_PAYMENT_ADDRESS,
                21,
        )

    def encrypted_wallet_send(self):
        self.log.info("Testing if encrypted SP wallet")
        self.nodes[0].createwallet(wallet_name='encrypted_wallet', descriptors=True, passphrase='passphrase')
        encrypted_wallet = self.nodes[0].get_wallet_rpc('encrypted_wallet')
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 10, encrypted_wallet.getnewaddress())
        self.log.info("encrypted wallets must be able to send coins after decryption")
        outputs = [{"bcrt1pk0yzk76w2p55ykyjyfeq99td069c257se9nwugl7cl5geadq944spyc330": 15}]

        # send RPC can be run without decrypting the wallet and it must generate a incomplete PSBT
        tx = encrypted_wallet.send(outputs=outputs, options={"add_to_wallet": False})
        assert not tx['complete']

        # but when silent_payment option is enabled, wallet must be decrypted
        assert_raises_rpc_error(
                -13,
                "Please enter the wallet passphrase with walletpassphrase first.",
                encrypted_wallet.sendtoaddress,
                SILENT_PAYMENT_ADDRESS,
                21,
        )

        encrypted_wallet.walletpassphrase('passphrase', 20)
        txid = encrypted_wallet.sendtoaddress(
            address=SILENT_PAYMENT_ADDRESS,
            amount=21,
        )
        assert txid

    def test_simple_send(self):
        self.log.info("Testing a simple send")
        self.nodes[0].createwallet(wallet_name="simple_send")
        simple_send = self.nodes[0].get_wallet_rpc("simple_send")
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 10, simple_send.getnewaddress())
        txid = simple_send.sendtoaddress(
            address=SILENT_PAYMENT_ADDRESS,
            amount=21,
        )
        assert txid

    def test_deterministic_send(self):

        # set up and fund the funder wallet. we will use this wallet to create UTXOs
        # at specific addresses in the sending wallet
        # using miniwallet for determinism
        funder = MiniWallet(self.nodes[0])

        # set up the sender wallet
        xprv1 = 'tprv8ZgxMBicQKsPevADjDCWsa6DfhkVXicu8NQUzfibwX2MexVwW4tCec5mXdCW8kJwkzBRRmAay1KZya4WsehVvjTGVW6JLqiqd8DdZ4xSg52'
        descriptor = {"desc": descsum_create("wpkh(" + xprv1 + "/0h/0h/*h)"),
                              "timestamp": "now",
                              "active": True,
                      }
        self.nodes[1].createwallet("sending_wallet", descriptors=True)
        sender = self.nodes[1].get_wallet_rpc("sending_wallet")
        sender.importdescriptors([descriptor])

        # fund the sender wallet with two utxos and mine a block
        address_one = sender.getnewaddress()
        address_two = sender.getnewaddress()
        funder.send_to(from_node=self.nodes[0], scriptPubKey=address_to_scriptpubkey(address_one), amount=10 * COIN)
        funder.send_to(from_node=self.nodes[0], scriptPubKey=address_to_scriptpubkey(address_two), amount=10 * COIN)
        self.generate(self.nodes[0], 1)

        # recipient address: generated with wallet seed hex `f00dbabe`
        silent_payment_address = "sprt1qqgste7k9hx0qftg6qmwlkqtwuy6cycyavzmzj85c6qdfhjdpdjtdgqjuexzk6murw56suy3e0rd2cgqvycxttddwsvgxe2usfpxumr70xcrdz399"
        amount = 15.0
        txid = sender.sendtoaddress(address=silent_payment_address, amount=amount)
        tx = sender.getrawtransaction(txid, True)
        for output in tx["vout"]:
            if output["value"] == amount:
                assert output["scriptPubKey"]["address"] == "bcrt1pfjcwk0este8fn8c77p78djswa2hram3m2v4nwg2jjzdkmv8299ys5sv99h"
                break
        else:
            assert False

    def test_address_reuse(self):
        self.nodes[0].createwallet(wallet_name=f'miner_wallet', descriptors=True)
        miner_wallet = self.nodes[0].get_wallet_rpc(f'miner_wallet')
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 10, miner_wallet.getnewaddress())

        self.nodes[0].createwallet(wallet_name=f'sender_wallet', descriptors=True)
        sender_wallet = self.nodes[0].get_wallet_rpc(f'sender_wallet')
        sender_address = sender_wallet.getnewaddress()

        miner_wallet.send(outputs=[{sender_address: 5}])
        miner_wallet.send(outputs=[{sender_address: 7}])

        self.generate(self.nodes[0], 8, sync_fun=self.no_op)

        silent_payment_address = "sprt1qqgste7k9hx0qftg6qmwlkqtwuy6cycyavzmzj85c6qdfhjdpdjtdgqjuexzk6murw56suy3e0rd2cgqvycxttddwsvgxe2usfpxumr70xcrdz399"
        txid1 = sender_wallet.sendtoaddress(address=silent_payment_address, amount=4)
        txid2 = sender_wallet.sendtoaddress(address=silent_payment_address, amount=6)

        output1 = ""
        output2 = ""
        for output in sender_wallet.getrawtransaction(txid1, True)["vout"]:
            if output["value"] == 4.0:
                output1 = output["scriptPubKey"]["address"]

        for output in sender_wallet.getrawtransaction(txid2, True)["vout"]:
            if output["value"] == 6.0:
                output2 = output["scriptPubKey"]["address"]

        assert output1 != output2

    def run_test(self):
        self.watch_only_wallet_send()
        self.encrypted_wallet_send()
        self.test_simple_send()
        self.test_deterministic_send()
        self.test_address_reuse()


if __name__ == '__main__':
    SilentTransactioTest().main()
