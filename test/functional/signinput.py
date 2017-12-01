#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the RPC call related to the uptime command.

Test corresponds to code in rpc/server.cpp.
"""

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class SignInputTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        privatekey = "cPbVfYM4UUuxKeJUotfq791Z36QN8br9yyAi61JghziH6LDgiqWF"
        tx = "010000000243ec7a579f5561a42a7e9637ad4156672735a658be2752181801f723ba3316d200000000844730440220449a203d0062ea01022f565c94c4b62bf2cc9a05de20519bc18cded9e99aa5f702201a2b8361e2af179eb93e697d9fedd0bf1e036f0a5be39af4b1f791df803bdb6501210363e38e2e0e55cdebfb7e27d0cb53ded150c320564a4614c18738feb124c8efd21976a9141a5fdcb6201f7e4fd160f9dca81075bd8537526088acffffffff43ec7a579f5561a42a7e9637ad4156672735a658be2752181801f723ba3316d20100000085483045022100ff6e7edffa5e0758244af6af77edd14f1d80226d66f81ed58dba2def34778236022057d95177497758e467c194f0d897f175645d30e7ac2f9490247e13227ac4d2fc01210363e38e2e0e55cdebfb7e27d0cb53ded150c320564a4614c18738feb124c8efd21976a9141a5fdcb6201f7e4fd160f9dca81075bd8537526088acffffffff0100752b7d000000001976a9141a5fdcb6201f7e4fd160f9dca81075bd8537526088ac00000000"
        scriptcode = "76a9141a5fdcb6201f7e4fd160f9dca81075bd8537526088ac"
        amount = 100000000
        sigversion = "BASE"
        input_index = 0
        sighash = "ALL"
        expected_sig = "3045022100cc79eb5596db1e5cf48acf67dd67626110698d4c504995ae00dd421edc721f91022053b5f23c1808b4a22e6364deed5eceda8967d1d3af2d2c7edf4727f1d1a4df6501"
        assert_equal(expected_sig, self.nodes[0].signinput(
            tx,
            scriptcode,
            amount,
            sigversion,
            input_index,
            privatekey,
            sighash
        ))

        # optional parameter
        assert_equal(expected_sig, self.nodes[0].signinput(
            tx,
            scriptcode,
            amount,
            sigversion,
            input_index,
            privatekey
        ))

        # WITNESS_0
        sigversion = "WITNESS_V0"
        expected_sig = "304402207618e6c93582612b25024cab7a6b3a0ce0a4389f0ee81cc6a8efb9345a47df2402205a15ba66ea375e25fcd9758453c14d7596d59eae5702a339f2c3f793c68f1a9801"
        assert_equal(expected_sig, self.nodes[0].signinput(
            tx,
            scriptcode,
            amount,
            sigversion,
            input_index,
            privatekey
        ))

        # WITNESS_0/SINGLE
        sigversion = "WITNESS_V0"
        sighash = "SINGLE"
        expected_sig = "3044022053a587a195c93b4d0816e03aa7922e010d64f08fdadf1d69ec5589e0163ff30002207daf9b3ca2b5fb3c584ef8e91e1e5e42d3b16fbb87fcdbf59be6462421f4bac003"
        assert_equal(expected_sig, self.nodes[0].signinput(
            tx,
            scriptcode,
            amount,
            sigversion,
            input_index,
            privatekey,
            sighash
        ))

if __name__ == '__main__':
    SignInputTest().main()
