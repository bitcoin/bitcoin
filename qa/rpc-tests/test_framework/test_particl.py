#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from .test_framework import BitcoinTestFramework
from .util import *

import decimal

def isclose(a, b, rel_tol=1e-09, abs_tol=0.0):
    a = decimal.Decimal(a)
    b = decimal.Decimal(b)
    return abs(a-b) <= max(decimal.Decimal(rel_tol) * decimal.Decimal(max(abs(a), abs(b))), abs_tol)

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
    
    def jsonDecimal(self, obj):
        if isinstance(obj, decimal.Decimal):
            return str(obj)
        raise TypeError
