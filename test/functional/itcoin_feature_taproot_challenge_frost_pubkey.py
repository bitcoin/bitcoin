#!/usr/bin/env python3
"""Test the network configuration where the signet challenge is a taproot
challenge based on a FROST pubkey.
"""

# Import the test primitives
from test_framework.test_framework_itcoin_frost import BaseFrostTest

# import itcoin's miner
from test_framework.itcoin_abs_import import import_miner

miner = import_miner()


class SignetChallengeTaprootFrostPubkey(BaseFrostTest):
    def set_test_params(self):
        self.num_nodes = 4
        self.signet_num_signers = 4
        self.signet_num_signatures = 3
        super().set_test_params(tweak_public_key=False)

    def run_test(self):
        super().mine_block()


if __name__ == '__main__':
    SignetChallengeTaprootFrostPubkey().main()
