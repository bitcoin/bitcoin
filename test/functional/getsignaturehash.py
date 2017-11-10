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

class GetSignatureHashTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        tx = "010000000243ec7a579f5561a42a7e9637ad4156672735a658be2752181801f723ba3316d200000000844730440220449a203d0062ea01022f565c94c4b62bf2cc9a05de20519bc18cded9e99aa5f702201a2b8361e2af179eb93e697d9fedd0bf1e036f0a5be39af4b1f791df803bdb6501210363e38e2e0e55cdebfb7e27d0cb53ded150c320564a4614c18738feb124c8efd21976a9141a5fdcb6201f7e4fd160f9dca81075bd8537526088acffffffff43ec7a579f5561a42a7e9637ad4156672735a658be2752181801f723ba3316d20100000085483045022100ff6e7edffa5e0758244af6af77edd14f1d80226d66f81ed58dba2def34778236022057d95177497758e467c194f0d897f175645d30e7ac2f9490247e13227ac4d2fc01210363e38e2e0e55cdebfb7e27d0cb53ded150c320564a4614c18738feb124c8efd21976a9141a5fdcb6201f7e4fd160f9dca81075bd8537526088acffffffff0100752b7d000000001976a9141a5fdcb6201f7e4fd160f9dca81075bd8537526088ac00000000"
        scriptcode = "76a9141a5fdcb6201f7e4fd160f9dca81075bd8537526088ac"
        amount = 100000000
        sigversion = "BASE"
        input_index = 0
        sighash = "ALL"
        expected_hash = "64e049981a10cf597406a175d7443c8499d308168915377637e3f706ce186137" 
        assert_equal(expected_hash, self.nodes[0].getsignaturehash(
            tx,
            scriptcode,
            amount,
            sigversion,
            input_index,
            sighash
        ))

        # optional parameter
        assert_equal(expected_hash, self.nodes[0].getsignaturehash(
            tx,
            scriptcode,
            amount,
            sigversion,
            input_index
        ))




if __name__ == '__main__':
    GetSignatureHashTest().main()
