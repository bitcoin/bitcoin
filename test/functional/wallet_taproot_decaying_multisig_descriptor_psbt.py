#!/usr/bin/env python3
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test a Taproot multisig that starts as 4-of-4 and "decays" to 3-of-4, 2-of-4, and finally 1-of-4 at each future halvening block height.

Spending policy: `tr(musig(key_1,key_2,key_3,key_4),thresh(4,pk(key_1),pk(key_2),pk(key_3),pk(key_4),after(t1),after(t2),after(t3)))`

This is the Taproot analogue of `test/functional/wallet_miniscript_decaying_multisig_descriptor_psbt.py`.
The 4-of-4 "everyone signs" case is spent through the Taproot key path using a MuSig2 aggregate key
(for the best privacy and lowest fees), while the "decayed" thresholds are spent through a Miniscript
script path. As with the wsh version the signers are plain single-key wallets: everything they need
to sign, including the MuSig2 participant pubkeys, is carried in the PSBT, so they never import the
multisig descriptor.
"""

import random
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_raises_rpc_error,
)


class WalletTaprootDecayingMultisigDescriptorPSBTTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.wallet_names = []
        self.extra_args = [["-keypool=100"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    @staticmethod
    def _get_xpub(wallet):
        """Extract the wallet's xpubs using `listdescriptors` and pick the one from the `pkh` descriptor since it's least likely to be accidentally reused (legacy addresses)."""
        pkh_descriptor = next(filter(lambda d: d["desc"].startswith("pkh(") and not d["internal"], wallet.listdescriptors()["descriptors"]))
        # keep all key origin information (master key fingerprint and all derivation steps) for proper support of hardware devices
        # see section 'Key origin identification' in 'doc/descriptors.md' for more details...
        # Replace the change index with the multipath convention
        return pkh_descriptor["desc"].split("pkh(")[1].split(")")[0].replace("/0/*", "/<0;1>/*")

    def create_multisig(self, xpubs):
        """The multisig is created by importing a single multipath descriptor. The resulting wallet is watch-only and every signer can do this."""
        self.node.createwallet(wallet_name=f"{self.name}", blank=True, disable_private_keys=True)
        multisig = self.node.get_wallet_rpc(f"{self.name}")
        # key path: a MuSig2 4-of-4 aggregate of all participants (the cheapest, most private spend)
        # script path: `thresh(4,pk(key_1),pk(key_2),pk(key_3),pk(key_4),after(t1),after(t2),after(t3))`
        # IMPORTANT: when backing up your descriptor, the order of key_1...key_4 must be correct!
        multisig_desc = f"tr(musig({','.join(xpubs)}),thresh({self.N},pk({'),s:pk('.join(xpubs)}),sln:after({'),sln:after('.join(map(str, self.locktimes))})))"
        checksum = multisig.getdescriptorinfo(multisig_desc)["checksum"]
        result = multisig.importdescriptors([
            {  # Multipath descriptor expands to receive and change
                "desc": f"{multisig_desc}#{checksum}",
                "active": True,
                "timestamp": "now",
            },
        ])
        assert all(r["success"] for r in result)
        return multisig

    def _spend_keypath(self, signers, psbt):
        """Spend the 4-of-4 via the MuSig2 key path: every participant first contributes a public
        nonce, then a partial signature once all nonces are known. The participant pubkeys travel in
        the PSBT, so the plain signer wallets take part without importing the multisig descriptor."""
        # `finalize=False`: every signer can also satisfy the `thresh` script path, which would
        # otherwise finalize the (larger) script path before the MuSig2 partial sigs are collected.
        psbt = self.node.combinepsbt([signer.walletprocesspsbt(psbt["psbt"], finalize=False)["psbt"] for signer in signers])
        assert_equal(len(self.node.decodepsbt(psbt)["inputs"][0]["musig2_pubnonces"]), self.N)
        psbt = self.node.combinepsbt([signer.walletprocesspsbt(psbt, finalize=False)["psbt"] for signer in signers])
        assert_equal(len(self.node.decodepsbt(psbt)["inputs"][0]["musig2_partial_sigs"]), self.N)
        finalized = self.node.finalizepsbt(psbt)
        assert finalized["complete"]
        # The aggregate is a single 64-byte (SIGHASH_DEFAULT) Schnorr signature: a 1-element witness, like a single-key spend.
        witness = self.node.decoderawtransaction(finalized["hex"])["vin"][0]["txinwitness"]
        assert_equal(len(witness), 1)
        assert_equal(len(witness[0]) // 2, 64)
        return finalized

    def run_test(self):
        self.node = self.nodes[0]
        self.M = 4  # starts as 4-of-4
        self.N = 4

        self.locktimes = [104, 106, 108]
        assert_equal(len(self.locktimes), self.N - 1)

        self.name = f"{self.M}_of_{self.N}_decaying_taproot_multisig"
        self.log.info(f"Testing a Taproot multisig which spends its 4-of-4 via the MuSig2 key path and 'decays' to a miniscript 3-of-4 at block height {self.locktimes[0]}, 2-of-4 at {self.locktimes[1]}, and finally 1-of-4 at {self.locktimes[2]}...")

        self.log.info("Create the signer wallets and get their xpubs...")
        signers = [self.node.get_wallet_rpc(self.node.createwallet(wallet_name=f"signer_{i}")["name"]) for i in range(self.N)]
        xpubs = [self._get_xpub(signer) for signer in signers]

        self.log.info("Create the watch-only decaying Taproot multisig using signers' xpubs...")
        multisig = self.create_multisig(xpubs)

        self.log.info("Get a mature utxo to send to the multisig...")
        coordinator_wallet = self.node.get_wallet_rpc(self.node.createwallet(wallet_name="coordinator")["name"])
        self.generatetoaddress(self.node, 101, coordinator_wallet.getnewaddress())

        self.log.info("Send funds to the multisig's receiving address...")
        deposit_amount = 7.65
        coordinator_wallet.sendtoaddress(multisig.getnewaddress(address_type="bech32m"), deposit_amount)
        self.generate(self.node, 1)
        assert_approx(multisig.getbalance(), deposit_amount, vspan=0.001)

        amount = 1.5
        receiver = signers[0]
        sent = 0

        self.log.info("Check that fewer than 4 signers cannot move the funds before the multisig decays...")
        # Both MuSig2 rounds with only 3 of the 4 signers: the key path cannot aggregate without all
        # participants, and the script path needs 4 signatures until a locktime elapses, so neither
        # path completes. This is the core "everyone must sign" guarantee before the wallet decays.
        under_signed = multisig.walletcreatefundedpsbt(inputs=[], outputs={receiver.getnewaddress(): amount}, feeRate=0.00010, locktime=0)["psbt"]
        for _ in range(2):
            under_signed = self.node.combinepsbt([signers[m].walletprocesspsbt(under_signed, finalize=False)["psbt"] for m in range(self.N - 1)])
        assert_equal(self.node.finalizepsbt(under_signed)["complete"], False)

        self.log.info("Send transactions from the multisig as required signers decay...")
        for locktime in [0] + self.locktimes:
            self.log.info(f"At block height >= {locktime} this multisig is {self.M}-of-{self.N}")
            current_height = self.node.getblock(self.node.getbestblockhash())['height']

            # in this test each signer signs the same psbt "in series" one after the other.
            # Another option is for each signer to sign the original psbt, and then combine
            # and finalize these. In some cases this may be more optimal for coordination.
            psbt = multisig.walletcreatefundedpsbt(inputs=[], outputs={receiver.getnewaddress(): amount}, feeRate=0.00010, locktime=locktime)
            # the random sample asserts that any of the signing keys can sign for the 3-of-4,
            # 2-of-4, and 1-of-4. While this is basic behavior of the miniscript thresh primitive,
            # it is a critical property of this wallet.
            for i, m in enumerate(random.sample(range(self.M), self.M)):
                psbt = signers[m].walletprocesspsbt(psbt["psbt"])
                assert_equal(psbt["complete"], i == self.M - 1)

            if self.M < self.N:
                self.log.info(f"Check that the time-locked transaction is too immature to spend with {self.M}-of-{self.N} at block height {current_height}...")
                assert_equal(current_height >= locktime, False)
                assert_raises_rpc_error(-26, "non-final", multisig.sendrawtransaction, psbt["hex"])

                self.log.info(f"Generate blocks to reach the time-lock block height {locktime} and broadcast the transaction...")
                self.generate(self.node, locktime - current_height)
            else:
                self.log.info("All the signers are required to spend before the first locktime, so spend the cheaper, more private MuSig2 key path instead of the script path signed above")
                # The script path psbt above is already finalized, so fund a fresh one for the key-path
                # nonce/partial-signature rounds (and broadcast that transaction below).
                psbt = multisig.walletcreatefundedpsbt(inputs=[], outputs={receiver.getnewaddress(): amount}, feeRate=0.00010, locktime=locktime)
                psbt = self._spend_keypath(signers, psbt)

            multisig.sendrawtransaction(psbt["hex"])
            sent += amount

            self.log.info("Check that balances are correct after the transaction has been included in a block...")
            self.generate(self.node, 1)
            assert_approx(multisig.getbalance(), deposit_amount - sent, vspan=0.001)
            assert_equal(receiver.getbalance(), sent)

            self.M -= 1  # decay the number of required signers for the next locktime..

        self.log.info("Even after fully decaying, the 4-of-4 key path is still spendable as long as every key is accessible (this also spends a change output via the key path)...")
        psbt = multisig.walletcreatefundedpsbt(inputs=[], outputs={receiver.getnewaddress(): amount}, feeRate=0.00010, locktime=0)
        psbt = self._spend_keypath(signers, psbt)
        multisig.sendrawtransaction(psbt["hex"])
        sent += amount

        self.log.info("Check that balances are correct after the transaction has been included in a block...")
        self.generate(self.node, 1)
        assert_approx(multisig.getbalance(), deposit_amount - sent, vspan=0.001)
        assert_equal(receiver.getbalance(), sent)


if __name__ == "__main__":
    WalletTaprootDecayingMultisigDescriptorPSBTTest(__file__).main()
