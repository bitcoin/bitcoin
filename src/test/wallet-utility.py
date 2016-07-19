#!/usr/bin/python
# Copyright 2014 BitPay, Inc.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import subprocess
import os
import json
import sys
import buildenv
import shutil

def assert_equal(thing1, thing2):
    if thing1 != thing2:
        raise AssertionError("%s != %s"%(str(thing1),str(thing2)))

if __name__ == '__main__':
    datadir = os.environ["srcdir"] + "/test/data"
    execprog = './wallet-utility' + buildenv.exeext
    execargs = '-datadir=' + datadir
    execrun = execprog + ' ' + execargs

    proc = subprocess.Popen(execrun, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, shell=True)
    try:
        outs = proc.communicate()
    except OSError:
        print("OSError, Failed to execute " + execprog)
        sys.exit(1)

    output = json.loads(outs[0])

    assert_equal(output[0], "13EngsxkRi7SJPPqCyJsKf34U8FoX9E9Av")
    assert_equal(output[1], "1FKCLGTpPeYBUqfNxktck8k5nqxB8sjim8")
    assert_equal(output[2], "13cdtE9tnNeXCZJ8KQ5WELgEmLSBLnr48F")

    execargs = '-datadir=' + datadir + ' -dumppass'
    execrun = execprog + ' ' + execargs

    proc = subprocess.Popen(execrun, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, shell=True)
    try:
        outs = proc.communicate()
    except OSError:
        print("OSError, Failed to execute " + execprog)
        sys.exit(1)

    output = json.loads(outs[0])

    assert_equal(output[0]['addr'], "13EngsxkRi7SJPPqCyJsKf34U8FoX9E9Av")
    assert_equal(output[0]['pkey'], "5Jz5BWE2WQxp1hGqDZeisQFV1mRFR2AVBAgiXCbNcZyXNjD9aUd")
    assert_equal(output[1]['addr'], "1FKCLGTpPeYBUqfNxktck8k5nqxB8sjim8")
    assert_equal(output[1]['pkey'], "5HsX2b3v2GjngYQ5ZM4mLp2b2apw6aMNVaPELV1YmpiYR1S4jzc")
    assert_equal(output[2]['addr'], "13cdtE9tnNeXCZJ8KQ5WELgEmLSBLnr48F")
    assert_equal(output[2]['pkey'], "5KCWAs1wX2ESiL4PfDR8XYVSSETHFd2jaRGxt1QdanBFTit4XcH")

    if os.path.exists(datadir + '/database'):
        if os.path.isdir(datadir + '/database'):
            shutil.rmtree(datadir + '/database')

    if os.path.exists(datadir + '/db.log'):
        os.remove(datadir + '/db.log')
    sys.exit(0)
