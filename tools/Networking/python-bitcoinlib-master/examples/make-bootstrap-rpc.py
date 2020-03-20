#!/usr/bin/env python3

# Copyright (C) 2013-2014 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.

"""Make a boostrap.dat file by getting the blocks from the RPC interface."""

import sys
if sys.version_info.major < 3:
    sys.stderr.write('Sorry, Python 3.x required by this example.\n')
    sys.exit(1)

import bitcoin
import bitcoin.rpc

import struct
import sys
import time

try:
    if len(sys.argv) not in (2, 3):
        raise Exception

    n = int(sys.argv[1])

    if len(sys.argv) == 3:
        bitcoin.SelectParams(sys.argv[2])
except Exception as ex:
    print('Usage: %s <block-height> [network=(mainnet|testnet|regtest)] > bootstrap.dat' % sys.argv[0], file=sys.stderr)
    sys.exit(1)


proxy = bitcoin.rpc.Proxy()

total_bytes = 0
start_time = time.time()

fd = sys.stdout.buffer
for i in range(n + 1):
    block = proxy.getblock(proxy.getblockhash(i))

    block_bytes = block.serialize()

    total_bytes += len(block_bytes)
    print('%.2f KB/s, height %d, %d bytes' %
            ((total_bytes / 1000) / (time.time() - start_time),
             i, len(block_bytes)),
          file=sys.stderr)

    fd.write(bitcoin.params.MESSAGE_START)
    fd.write(struct.pack('<i', len(block_bytes)))
    fd.write(block_bytes)
