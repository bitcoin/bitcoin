#!/usr/bin/env python3

# Copyright (C) 2014 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.

"""Example of timestamping a file via OP_RETURN"""

import sys
if sys.version_info.major < 3:
    sys.stderr.write('Sorry, Python 3.x required by this example.\n')
    sys.exit(1)

import hashlib
import bitcoin.rpc
import sys

from bitcoin import params
from bitcoin.core import *
from bitcoin.core.script import *

proxy = bitcoin.rpc.Proxy()

assert len(sys.argv) > 1

digests = []
for f in sys.argv[1:]:
    try:
        with open(f, 'rb') as fd:
            digests.append(Hash(fd.read()))
    except FileNotFoundError as exp:
        if len(f)/2 in (20, 32):
            digests.append(x(f))
        else:
            raise exp
    except IOError as exp:
        print(exp, file=sys.stderr)
        continue

for digest in digests:
    txouts = []

    unspent = sorted(proxy.listunspent(0), key=lambda x: hash(x['amount']))

    txins = [CTxIn(unspent[-1]['outpoint'])]
    value_in = unspent[-1]['amount']

    change_addr = proxy.getnewaddress()
    change_pubkey = proxy.validateaddress(change_addr)['pubkey']
    change_out = CMutableTxOut(params.MAX_MONEY, CScript([change_pubkey, OP_CHECKSIG]))

    digest_outs = [CMutableTxOut(0, CScript([OP_RETURN, digest]))]

    txouts = [change_out] + digest_outs

    tx = CMutableTransaction(txins, txouts)


    FEE_PER_BYTE = 0.00025*COIN/1000
    while True:
        tx.vout[0].nValue = int(value_in - max(len(tx.serialize()) * FEE_PER_BYTE, 0.00011*COIN))

        r = proxy.signrawtransaction(tx)
        assert r['complete']
        tx = r['tx']

        if value_in - tx.vout[0].nValue >= len(tx.serialize()) * FEE_PER_BYTE:
            print(b2x(tx.serialize()))
            print(len(tx.serialize()), 'bytes', file=sys.stderr)
            print(b2lx(proxy.sendrawtransaction(tx)))
            break
