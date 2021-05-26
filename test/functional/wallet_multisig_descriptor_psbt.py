#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test a basic M-of-N multisig setup between multiple people using descriptor wallets and PSBTs, as well as a signing flow.

This is meant to be documentation as much as functional tests, so it is kept as simple and readable as possible.
"""

from test_framework.address import base58_to_byte
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
)


class WalletMultisigDescriptorPSBTTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True
        self.wallet_names = []
        self.extra_args = [["-keypool=100"]] * self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def _get_xpub(self, wallet):
        """Extract the wallet's xpubs using `listdescriptors` and pick the one from the `pkh` descriptor since it's least likely to be accidentally reused (legacy addresses)."""
        descriptor = next(filter(lambda d: d["desc"].startswith("pkh"), wallet.listdescriptors()["descriptors"]))
        return descriptor["desc"].split("]")[-1].split("/")[0]

    def _check_psbt(self, psbt, to, value, multisig):
        """Helper method for any of the N participants to check the psbt with decodepsbt and verify it is OK before signing."""
        tx = multisig.decodepsbt(psbt)["tx"]
        amount = 0
        for vout in tx["vout"]:
            address = vout["scriptPubKey"]["address"]
            assert_equal(multisig.getaddressinfo(address)["ischange"], address != to)
            if address == to:
                amount += vout["value"]
        assert_approx(amount, float(value), vspan=0.001)

    def generate_and_exchange_xpubs(self, participants):
        """Every participant generates an xpub. The most straightforward way is to create a new descriptor wallet. Avoid reusing this wallet for any other purpose.."""
        for i, node in enumerate(participants):
            node.createwallet(wallet_name=f"participant_{i}", descriptors=True)
            yield self._get_xpub(node.get_wallet_rpc(f"participant_{i}"))

    def participants_import_descriptors(self, participants, xpubs):
        """The multisig is created by importing the following descriptors. The resulting wallet is watch-only and every participant can do this."""
        # some simple validation
        assert_equal(len(xpubs), self.N)
        # a sanity-check/assertion, this will throw if the base58 checksum of any of the provided xpubs are invalid
        for xpub in xpubs:
            base58_to_byte(xpub)

        for i, node in enumerate(participants):
            node.createwallet(wallet_name=f"{self.name}_{i}", blank=True, descriptors=True, disable_private_keys=True)
            multisig = node.get_wallet_rpc(f"{self.name}_{i}")
            external = multisig.getdescriptorinfo(f"wsh(sortedmulti({self.M},{f'/{0}/*,'.join(xpubs)}/{0}/*))")
            internal = multisig.getdescriptorinfo(f"wsh(sortedmulti({self.M},{f'/{1}/*,'.join(xpubs)}/{1}/*))")
            result = multisig.importdescriptors([
                {  # receiving addresses (internal: False)
                    "desc": external["descriptor"],
                    "active": True,
                    "internal": False,
                    "timestamp": "now",
                },
                {  # change addresses (internal: True)
                    "desc": internal["descriptor"],
                    "active": True,
                    "internal": True,
                    "timestamp": "now",
                },
            ])
            assert all(r["success"] for r in result)

    def get_multisig_receiving_address(self):
        """We will send funds to the resulting address (every participant should get the same addresses)."""
        multisig = self.nodes[0].get_wallet_rpc(f"{self.name}_{0}")
        receiving_address = multisig.getnewaddress()
        for i in range(1, self.N):
            assert_equal(receiving_address, self.nodes[i].get_wallet_rpc(f"{self.name}_{i}").getnewaddress())
        return receiving_address

    def make_sending_transaction(self, to, value):
        """Make a sending transaction, created using walletcreatefundedpsbt (anyone can initiate this)."""
        return self.nodes[0].get_wallet_rpc(f"{self.name}_{0}").walletcreatefundedpsbt(inputs=[], outputs={to: value}, options={"feeRate": 0.00010})

    def run_test(self):
        self.M = 2
        self.N = self.num_nodes
        self.name = f"{self.M}_of_{self.N}_multisig"
        self.log.info(f"Testing {self.name}...")

        self.log.info("Generate and exchange xpubs...")
        xpubs = list(self.generate_and_exchange_xpubs(self.nodes))

        self.log.info("Every participant imports the following descriptors to create the watch-only multisig...")
        self.participants_import_descriptors(self.nodes, xpubs)

        self.log.info("Get a mature utxo to send to the multisig...")
        coordinator_wallet = self.nodes[0].get_wallet_rpc(f"participant_{0}")
        coordinator_wallet.generatetoaddress(101, coordinator_wallet.getnewaddress())

        deposit_amount = 6.15
        multisig_receiving_address = self.get_multisig_receiving_address()
        self.log.info("Send funds to the resulting multisig receiving address...")
        coordinator_wallet.sendtoaddress(multisig_receiving_address, deposit_amount)
        self.nodes[0].generate(1)
        self.sync_all()
        for n in range(self.N):
            assert_approx(self.nodes[n].get_wallet_rpc(f"{self.name}_{n}").getbalance(), deposit_amount, vspan=0.001)

        self.log.info("Send a transaction from the multisig!")
        to = self.nodes[self.N - 1].get_wallet_rpc(f"participant_{self.N - 1}").getnewaddress()
        value = 1
        psbt = self.make_sending_transaction(to, value)

        psbts = []
        self.log.info("At least M users check the psbt with decodepsbt and (if OK) signs it with walletprocesspsbt...")
        for m in range(self.M):
            signers_multisig = self.nodes[m].get_wallet_rpc(f"{self.name}_{m}")
            self._check_psbt(psbt["psbt"], to, value, signers_multisig)
            signing_wallet = self.nodes[m].get_wallet_rpc(f"participant_{m}")
            partially_signed_psbt = signing_wallet.walletprocesspsbt(psbt["psbt"])
            psbts.append(partially_signed_psbt["psbt"])

        self.log.info("Collect the signed PSBTs with combinepsbt, finalizepsbt, then broadcast the resulting transaction...")
        combined = coordinator_wallet.combinepsbt(psbts)
        finalized = coordinator_wallet.finalizepsbt(combined)
        coordinator_wallet.sendrawtransaction(finalized["hex"])

        self.log.info("Check that balances are correct after the transaction has been included in a block.")
        self.nodes[0].generate(1)
        self.sync_all()
        assert_approx(self.nodes[0].get_wallet_rpc(f"{self.name}_{0}").getbalance(), deposit_amount - value, vspan=0.001)
        assert_equal(self.nodes[self.N - 1].get_wallet_rpc(f"participant_{self.N - 1}").getbalance(), value)

if __name__ == "__main__":
    WalletMultisigDescriptorPSBTTest().main()
