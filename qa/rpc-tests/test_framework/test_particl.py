#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from .test_framework import BitcoinTestFramework
from .util import *

class ParticlTestFramework(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
    
    def wait_for_height(self, node, nHeight, nTries=500):
        for i in range(nTries):
            time.sleep(1)
            ro = node.getinfo()
            if ro['blocks'] >= nHeight:
                return True
        return False
    
    def wait_for_mempool(self, node, txnHash, nTries=50):
        for i in range(50):
            time.sleep(0.5)
            try:
                ro = node.getmempoolentry(txnHash)
                
                if ro['size'] >= 100 and ro['height'] >= 0:
                    return True
            except:
                continue
        return False
