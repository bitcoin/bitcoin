#!/usr/bin/env python3
# Copyright (c) 2021-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test a basic M-of-N multisig setup between multiple people using descriptor wallets and PSBTs, as well as a signing flow.

This is meant to be documentation as much as functional tests, so it is kept as simple and readable as possible.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
)

from test_framework.descriptors import descsum_create


class WalletMultisigDescriptorPSBTTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-keypool=100"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    @staticmethod
    def _get_xpub(wallet):
        """Derive an xpub at m/44h/1h/0h using `derivehdkey`. This derivation matches the `pkh` descriptor since it's least likely to be accidentally reused (legacy addresses)."""
        # Ideally we would use m/87h/1h/0h but the wallet currently can't sign
        # for a derivation path that's not used in one of its descriptors.
        hdkey_info = wallet.derivehdkey("m/44h/1h/0h")
        # Keep all key origin information (master key fingerprint and all derivation steps) for proper support of hardware devices
        # See section 'Key origin identification' in 'doc/descriptors.md' for more details...
        return f"{hdkey_info['origin']}{hdkey_info['xpub']}/<0;1>/*"

    @staticmethod
    def _split_key_expression(xpub_expr):
        """Split "[fingerprint/path]xpub/<0;1>/*" into the fingerprint, the path and the xpub."""
        origin, _, rest = xpub_expr.partition("]")
        fingerprint, _, path = origin.lstrip("[").partition("/")
        return fingerprint, f"m/{path}", rest.split("/")[0]

    def _check_global_xpubs(self, psbt, xpub_exprs, multisig):
        """Every co-signer's extended key belongs in the PSBT, with the origin that leads
        to it, so that a signer holding only the file can tell which key in the script is
        its own."""
        got = multisig.decodepsbt(psbt)["global_xpubs"]
        expected = [self._split_key_expression(x) for x in xpub_exprs]
        assert_equal(sorted((g["master_fingerprint"], g["path"], g["xpub"]) for g in got),
                     sorted(expected))

    @staticmethod
    def _check_psbt(psbt, to, value, multisig):
        """Helper function for any of the N participants to check the psbt with decodepsbt and verify it is OK before signing."""
        decoded = multisig.decodepsbt(psbt)
        amount = 0
        for psbt_out in decoded["outputs"]:
            address = psbt_out["script"]["address"]
            assert_equal(multisig.getaddressinfo(address)["ischange"], address != to)
            if address == to:
                amount += psbt_out["amount"]
        assert_approx(amount, float(value), vspan=0.001)

    def participants_create_multisigs(self, xpubs):
        """The multisig is created by importing the following descriptors. The resulting wallet is watch-only and every participant can do this."""
        for i in range(self.N):
            self.node.createwallet(wallet_name=f"{self.name}_{i}", blank=True, disable_private_keys=True)
            multisig = self.node.get_wallet_rpc(f"{self.name}_{i}")
            desc = descsum_create(f"wsh(sortedmulti({self.M},{','.join(xpubs)}))")
            self.log.debug(desc)
            result = multisig.importdescriptors([
                {
                    "desc": desc,
                    "active": True,
                    "timestamp": "now",
                },
            ])
            assert all(r["success"] for r in result)
            yield multisig

    def test_change_descriptor_publishes(self, signers, coordinator):
        """The internal descriptor publishes on its own, reaching the PSBT through the
        change output while every input belongs to the external one. A wallet whose two
        descriptors share their keys cannot show this, since both then contribute the
        same entries."""
        def account(signer, path):
            info = signer.derivehdkey(path)
            return f"{info['origin']}{info['xpub']}/0/*"

        receiving = [account(signer, "m/44h/1h/0h") for signer in signers]
        change = [account(signer, "m/44h/1h/1h") for signer in signers]
        wallet = self.node.get_wallet_rpc(self.node.createwallet(
            wallet_name="split_change_multisig", blank=True, disable_private_keys=True)["name"])
        result = wallet.importdescriptors([
            {"desc": descsum_create(f"wsh(sortedmulti({self.M},{','.join(receiving)}))"),
             "active": True, "timestamp": "now"},
            {"desc": descsum_create(f"wsh(sortedmulti({self.M},{','.join(change)}))"),
             "active": True, "internal": True, "timestamp": "now"},
        ])
        assert all(r["success"] for r in result)

        coordinator.sendtoaddress(wallet.getnewaddress(), 1)
        self.generate(self.node, 1)
        funded = wallet.walletcreatefundedpsbt(inputs=[], outputs={coordinator.getnewaddress(): 0.5}, feeRate=0.00010)
        assert funded["changepos"] != -1, "the case needs a change output"
        self._check_global_xpubs(funded["psbt"], receiving + change, wallet)

    def run_test(self):
        self.M = 2
        self.N = 3
        self.node = self.nodes[0]
        self.name = f"{self.M}_of_{self.N}_multisig"
        self.log.info(f"Testing {self.name}...")

        participants = {
            # Every participant generates an xpub. The most straightforward way is to create a new descriptor wallet.
            # This wallet will be the participant's `signer` for the resulting multisig. Avoid reusing this wallet for any other purpose (for privacy reasons).
            "signers": [self.node.get_wallet_rpc(self.node.createwallet(wallet_name=f"participant_{i}")["name"]) for i in range(self.N)],
            # After participants generate and exchange their xpubs they will each create their own watch-only multisig.
            # Note: these multisigs are all the same, this just highlights that each participant can independently verify everything on their own node.
            "multisigs": []
        }

        self.log.info("Generate and exchange xpubs...")
        xpubs = [self._get_xpub(signer) for signer in participants["signers"]]

        self.log.info("Every participant imports the following descriptors to create the watch-only multisig...")
        participants["multisigs"] = list(self.participants_create_multisigs(xpubs))

        self.log.info("Check that every participant's multisig generates the same addresses...")
        for _ in range(10):  # we check that the first 10 generated addresses are the same for all participant's multisigs
            receive_addresses = [multisig.getnewaddress() for multisig in participants["multisigs"]]
            for address in receive_addresses:
                assert_equal(address, receive_addresses[0])
            change_addresses = [multisig.getrawchangeaddress() for multisig in participants["multisigs"]]
            for address in change_addresses:
                assert_equal(address, change_addresses[0])

        self.log.info("Get a mature utxo to send to the multisig...")
        coordinator_wallet = participants["signers"][0]
        self.generatetoaddress(self.node, 101, coordinator_wallet.getnewaddress())

        deposit_amount = 6.15
        multisig_receiving_address = participants["multisigs"][0].getnewaddress()
        self.log.info("Send funds to the resulting multisig receiving address...")
        coordinator_wallet.sendtoaddress(multisig_receiving_address, deposit_amount)
        self.generate(self.node, 1)
        for participant in participants["multisigs"]:
            assert_approx(participant.getbalance(), deposit_amount, vspan=0.001)

        self.log.info("Send a transaction from the multisig!")
        to = participants["signers"][self.N - 1].getnewaddress()
        value = 1
        self.log.info("First, make a sending transaction, created using `walletcreatefundedpsbt` (anyone can initiate this)...")
        psbt = participants["multisigs"][0].walletcreatefundedpsbt(inputs=[], outputs={to: value}, feeRate=0.00010)

        self.log.info("The psbt carries every co-signer's xpub, and does not when bip32derivs is off...")
        self._check_global_xpubs(psbt["psbt"], xpubs, participants["multisigs"][0])
        no_derivs = participants["multisigs"][0].walletcreatefundedpsbt(inputs=[], outputs={to: value}, feeRate=0.00010, bip32derivs=False)
        assert_equal(participants["multisigs"][0].decodepsbt(no_derivs["psbt"])["global_xpubs"], [])

        self.log.info("A single key wallet has no co-signer to tell apart, so it publishes nothing...")
        single_sig = participants["signers"][0].walletcreatefundedpsbt(inputs=[], outputs={to: value}, feeRate=0.00010)
        assert_equal(participants["signers"][0].decodepsbt(single_sig["psbt"])["global_xpubs"], [])

        psbts = []
        self.log.info("Now at least M users check the psbt with decodepsbt and (if OK) signs it with walletprocesspsbt...")
        for m in range(self.M):
            signers_multisig = participants["multisigs"][m]
            self._check_psbt(psbt["psbt"], to, value, signers_multisig)
            signing_wallet = participants["signers"][m]
            partially_signed_psbt = signing_wallet.walletprocesspsbt(psbt["psbt"])
            psbts.append(partially_signed_psbt["psbt"])

        self.log.info("Finally, collect the signed PSBTs with combinepsbt, finalizepsbt, then broadcast the resulting transaction...")
        combined = coordinator_wallet.combinepsbt(psbts)
        self.log.debug(coordinator_wallet.analyzepsbt(combined))
        finalized = coordinator_wallet.finalizepsbt(combined)
        coordinator_wallet.sendrawtransaction(finalized["hex"])

        self.log.info("Check that balances are correct after the transaction has been included in a block.")
        self.generate(self.node, 1)
        assert_approx(participants["multisigs"][0].getbalance(), deposit_amount - value, vspan=0.001)
        assert_equal(participants["signers"][self.N - 1].getbalance(), value)

        self.log.info("Send another transaction from the multisig, this time with a daisy chained signing flow (one after another in series)!")
        psbt = participants["multisigs"][0].walletcreatefundedpsbt(inputs=[], outputs={to: value}, feeRate=0.00010)
        for m in range(self.M):
            signers_multisig = participants["multisigs"][m]
            self._check_psbt(psbt["psbt"], to, value, signers_multisig)
            signing_wallet = participants["signers"][m]
            psbt = signing_wallet.walletprocesspsbt(psbt["psbt"])
            assert_equal(psbt["complete"], m == self.M - 1)
        coordinator_wallet.sendrawtransaction(psbt["hex"])

        self.log.info("Check that balances are correct after the transaction has been included in a block.")
        self.generate(self.node, 1)
        assert_approx(participants["multisigs"][0].getbalance(), deposit_amount - (value * 2), vspan=0.001)
        assert_equal(participants["signers"][self.N - 1].getbalance(), value * 2)

        self.log.info("The change descriptor publishes its own keys, even with no input of its own...")
        self.test_change_descriptor_publishes(participants["signers"], coordinator_wallet)


if __name__ == "__main__":
    WalletMultisigDescriptorPSBTTest(__file__).main()
