#!/usr/bin/python
# Copyright 2014 BitPay, Inc.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from subprocess import check_output
import json

import os

def assert_equal(thing1, thing2):
    if thing1 != thing2:
        raise AssertionError("%s != %s"%(str(thing1),str(thing2)))

if __name__ == '__main__':
    datadir = os.environ["srcdir"] + "/test/data"
    command = os.environ["srcdir"] + "/wallet-utility"

    output = json.loads(check_output([command, "-datadir=" + datadir]))

    assert_equal(output[0], "13EngsxkRi7SJPPqCyJsKf34U8FoX9E9Av");
    assert_equal(output[1], "1FKCLGTpPeYBUqfNxktck8k5nqxB8sjim8");
    assert_equal(output[2], "13cdtE9tnNeXCZJ8KQ5WELgEmLSBLnr48F");



