#!/usr/bin/env python
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Exercise the listtransactions API

# Add python-bitcoinrpc to module search path:
from util import *
from skeleton import Skeleton as baseclass

# 1) Change class name to your test
class MyTest(baseclass):
  
    # 2) Implement tests
    def run_test(self,nodes):
        ''' '''

# 3) Change 'MyTest' to class name
if __name__ == '__main__':
    MyTest().main()
