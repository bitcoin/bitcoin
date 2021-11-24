#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test behavior of -maxuploadtarget with byte unit M

* Same as MaxUploadTest in test/functional/feature_maxuploadtarget.py
"""
from feature_maxuploadtarget import MaxUploadTest

class MaxUploadTestByteUnit(MaxUploadTest):
    # same as MaxUploadTest but with an byte unit suffix M
    uploadtarget = "800M"
    restart_uploadtarget = "1M"

    def set_test_params(self):
        super().set_test_params()


    def run_test(self):
        super().run_test()


if __name__ == '__main__':
    MaxUploadTestByteUnit().main()
