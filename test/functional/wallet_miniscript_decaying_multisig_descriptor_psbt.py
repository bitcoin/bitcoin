#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test a miniscript multisig that starts as 4-of-4 and "decays" to 3-of-4, 2-of-4, and finally 1-of-4 at each future halvening block height.

Spending policy: `thresh(4,pk(key_1),pk(key_2),pk(key_3),pk(key_4),after(t1),after(t2),after(t3))`
This is similar to `test/functional/wallet_multisig_descriptor_psbt.py`.
"""

import random
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_raises_rpc_error,
)


class WalletMiniscriptDecayingMultisigDescriptorPSBTTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, legacy=False)

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.wallet_names = []
        self.extra_args = [["-keypool=100"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    @staticmethod
    def _get_xpub(wallet, internal):
        """Extract the wallet's xpubs using `listdescriptors` and pick the one from the `pkh` descriptor since it's least likely to be accidentally reused (legacy addresses)."""
        pkh_descriptor = next(filter(lambda d: d["desc"].startswith("pkh(") and d["internal"] == internal, wallet.listdescriptors()["descriptors"]))
        # keep all key origin information (master key fingerprint and all derivation steps) for proper support of hardware devices
        # see section 'Key origin identification' in 'doc/descriptors.md' for more details...
        return pkh_descriptor["desc"].split("pkh(")[1].split(")")[0]

    def create_multisig(self, external_xpubs, internal_xpubs):
        """The multisig is created by importing the following descriptors. The resulting wallet is watch-only and every signer can do this."""
        self.node.createwallet(wallet_name=f"{self.name}", blank=True, descriptors=True, disable_private_keys=True)
        multisig = self.node.get_wallet_rpc(f"{self.name}")
        # spending policy: `thresh(4,pk(key_1),pk(key_2),pk(key_3),pk(key_4),after(t1),after(t2),after(t3))`
        # IMPORTANT: when backing up your descriptor, the order of key_1...key_4 must be correct!
        external = multisig.getdescriptorinfo(f"wsh(thresh({self.N},pk({f'),s:pk('.join(external_xpubs)}),sln:after({f'),sln:after('.join(map(str, self.locktimes))})))")
        internal = multisig.getdescriptorinfo(f"wsh(thresh({self.N},pk({f'),s:pk('.join(internal_xpubs)}),sln:after({f'),sln:after('.join(map(str, self.locktimes))})))")
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
        return multisig

    def run_test(self):
        self.node = self.nodes[0]
        self.M = 4  # starts as 4-of-4
        self.N = 4

        self.locktimes = [104, 106, 108]  # in real-world align with future halvenings, eg: [840000, 1050000, 1260000]
        assert_equal(len(self.locktimes), self.N - 1)

        self.name = f"{self.M}_of_{self.N}_decaying_multisig"
        self.log.info(f"Testing a miniscript multisig which starts as 4-of-4 and 'decays' to 3-of-4 at block height {self.locktimes[0]}, 2-of-4 at {self.locktimes[1]}, and finally 1-of-4 at {self.locktimes[2]}...")

        self.log.info("Create the signer wallets and get their xpubs...")
        signers = [self.node.get_wallet_rpc(self.node.createwallet(wallet_name=f"signer_{i}", descriptors=True)["name"]) for i in range(self.N)]
        external_xpubs, internal_xpubs = [[self._get_xpub(signer, internal) for signer in signers] for internal in [False, True]]

        self.log.info("Create the watch-only decaying multisig using signers' xpubs...")
        multisig = self.create_multisig(external_xpubs, internal_xpubs)

        self.log.info("Get a mature utxo to send to the multisig...")
        coordinator_wallet = self.node.get_wallet_rpc(self.node.createwallet(wallet_name=f"coordinator", descriptors=True)["name"])
        self.generatetoaddress(self.node, 101, coordinator_wallet.getnewaddress())

        self.log.info("Send funds to the multisig's receiving address...")
        deposit_amount = 6.15
        coordinator_wallet.sendtoaddress(multisig.getnewaddress(), deposit_amount)
        self.generate(self.node, 1)
        assert_approx(multisig.getbalance(), deposit_amount, vspan=0.001)

        self.log.info("Send transactions from the multisig as required signers decay...")
        sent = 0
        for locktime in [0] + self.locktimes:
            self.log.info(f"At block height >= {locktime} this multisig is {self.M}-of-{self.N}")
            current_height = self.node.getblock(self.node.getbestblockhash())['height']

            amount = 1.5
            receiver = signers[0]

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
                self.log.info("We can always spend with all signers!")

            multisig.sendrawtransaction(psbt["hex"])
            sent += amount

            self.log.info("Check that balances are correct after the transaction has been included in a block...")
            self.generate(self.node, 1)
            assert_approx(multisig.getbalance(), deposit_amount - sent, vspan=0.001)
            assert_equal(receiver.getbalance(), sent)

            self.M -= 1  # decay the number of required signers for the next locktime..


if __name__ == "__main__":
    WalletMiniscriptDecayingMultisigDescriptorPSBTTest().main()
