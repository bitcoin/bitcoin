#!/usr/bin/env python3
#
# Copyright (C) 2015 Peter Todd
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# WARNING: Do not run this on a wallet with a non-trivial amount of BTC. This
# utility has had very little testing and is being published as a
# proof-of-concept only.

# Requires python-bitcoinlib w/ sendmany support:
#
# https://github.com/petertodd/python-bitcoinlib/commit/6a0a2b9429edea318bea7b65a68a950cae536790

import sys
if sys.version_info.major < 3:
    sys.stderr.write('Sorry, Python 3.x required by this example.\n')
    sys.exit(1)

import argparse
import hashlib
import logging
import sys
import os

import bitcoin.rpc
from bitcoin.core import *
from bitcoin.core.script import *
from bitcoin.wallet import *

parser = argparse.ArgumentParser(
        description="Publish text in the blockchain, suitably padded for easy recovery with strings",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)

parser.add_argument('-n', action='store_true',
                    dest='dryrun',
                    help="Dry-run; don't actually send the transactions")
parser.add_argument("-q","--quiet",action="count",default=0,
                    help="Be more quiet.")
parser.add_argument("-v","--verbose",action="count",default=0,
                    help="Be more verbose. Both -v and -q may be used multiple times.")
parser.add_argument("--min-len",action="store",type=int,default=20,
                    help="Minimum text length; shorter text is padded to this length")
parser.add_argument("-f","--fee-per-kb",action="store",type=float,default=0.0002,
                    help="Fee-per-KB")
parser.add_argument("-k","--privkey",action="store",type=str,default=None,
                    help="Specify private key")

net_parser = parser.add_mutually_exclusive_group()
net_parser.add_argument('-t','--testnet', action='store_true',
                        dest='testnet',
                        help='Use testnet')
net_parser.add_argument('-r','--regtest', action='store_true',
                        dest='regtest',
                        help='Use regtest')

parser.add_argument('fd', type=argparse.FileType('rb'), metavar='FILE',
                    help='Text file')

args = parser.parse_args()

# Setup logging
args.verbosity = args.verbose - args.quiet
if args.verbosity == 0:
    logging.root.setLevel(logging.INFO)
elif args.verbosity >= 1:
    logging.root.setLevel(logging.DEBUG)
elif args.verbosity == -1:
    logging.root.setLevel(logging.WARNING)
elif args.verbosity <= -2:
    logging.root.setLevel(logging.ERROR)

if args.testnet:
    bitcoin.SelectParams('testnet')
elif args.regtest:
    bitcoin.SelectParams('regtest')

proxy = bitcoin.rpc.Proxy()

if args.privkey is None:
    args.privkey = CBitcoinSecret.from_secret_bytes(os.urandom(32))

else:
    args.privkey = CBitcoinSecret(args.privkey)

logging.info('Using keypair %s %s' % (b2x(args.privkey.pub), args.privkey))

# Turn the text file into padded lines
if args.fd is sys.stdin:
    # work around a bug where even though we specified binary encoding we get
    # the sys.stdin instead.
    args.fd = sys.stdin.buffer
raw_padded_lines = [b'\x00' + line.rstrip().ljust(args.min_len) + b'\x00' for line in args.fd.readlines()]

# combine lines if < MAX_SCRIPT_ELEMENT_SIZE
padded_lines = []
prev_line = b'\x00'
for line in raw_padded_lines:
    if len(prev_line) + len(line) <= MAX_SCRIPT_ELEMENT_SIZE:
        prev_line = prev_line + line[1:]

    else:
        padded_lines.append(prev_line)
        prev_line = line

if prev_line:
    padded_lines.append(prev_line)

scripts = []
while padded_lines:
    def make_scripts(lines, n):
        # The n makes sure every p2sh addr is unique; the pubkey lets us
        # control the order the vin order vs. just using hashlocks.
        redeemScript = []
        for chunk in reversed(lines):
            if len(chunk) > MAX_SCRIPT_ELEMENT_SIZE:
                parser.exit('Lines must be less than %d characters; got %d characters' %\
                                    (MAX_SCRIPT_ELEMENT_SIZE, len(chunk)))
            redeemScript.extend([OP_HASH160, Hash160(chunk), OP_EQUALVERIFY])
        redeemScript = CScript(redeemScript +
                               [args.privkey.pub, OP_CHECKSIGVERIFY,
                                n, OP_DROP, # deduplicate push dropped to meet BIP62 rules
                                OP_DEPTH, 0, OP_EQUAL]) # prevent scriptSig malleability

        return CScript(lines) + redeemScript, redeemScript

    scriptSig = redeemScript = None
    for i in range(len(padded_lines)):
        next_scriptSig, next_redeemScript = make_scripts(padded_lines[0:i+1], len(scripts))

        # FIXME: magic numbers!
        if len(next_redeemScript) > 520 or len(next_scriptSig) > 1600-100:
            padded_lines = padded_lines[i:]
            break

        else:
            scriptSig = next_scriptSig
            redeemScript = next_redeemScript

    else:
        padded_lines = []

    scripts.append((scriptSig, redeemScript))

# pay to the redeemScripts to make them spendable

# the 41 accounts for the size of the CTxIn itself
payments = {P2SHBitcoinAddress.from_redeemScript(redeemScript):int(((len(scriptSig)+41)/1000 * args.fee_per_kb)*COIN)
                for scriptSig, redeemScript in scripts}

prevouts_by_scriptPubKey = None
if not args.dryrun:
    txid = proxy.sendmany('', payments, 0)

    logging.info('Sent pre-pub tx: %s' % b2lx(txid))

    tx = proxy.getrawtransaction(txid)

    prevouts_by_scriptPubKey = {txout.scriptPubKey:COutPoint(txid, i) for i, txout in enumerate(tx.vout)}

else:
    prevouts_by_scriptPubKey = {redeemScript.to_p2sh_scriptPubKey():COutPoint(b'\x00'*32, i)
                                    for i, (scriptSig, redeemScript) in enumerate(scripts)}
    logging.debug('Payments: %r' % payments)
    logging.info('Total cost: %s BTC' % str_money_value(sum(amount for addr, amount in payments.items())))

# Create unsigned tx for SignatureHash
vout = [CTxOut(0, CScript([OP_RETURN]))]

unsigned_vin = []
for scriptSig, redeemScript in scripts:
    scriptPubKey = redeemScript.to_p2sh_scriptPubKey()

    txin = CTxIn(prevouts_by_scriptPubKey[scriptPubKey])
    unsigned_vin.append(txin)
unsigned_tx = CTransaction(unsigned_vin, vout)

# Sign!
signed_vin = []
for i, (scriptSig, redeemScript) in enumerate(scripts):
    sighash = SignatureHash(redeemScript, unsigned_tx, i, SIGHASH_NONE)
    sig = args.privkey.sign(sighash) + bytes([SIGHASH_NONE])

    signed_scriptSig = CScript([sig] + list(scriptSig))

    txin = CTxIn(unsigned_vin[i].prevout, signed_scriptSig)

    signed_vin.append(txin)

signed_tx = CTransaction(signed_vin, vout)

if args.dryrun:
    serialized_tx = signed_tx.serialize()
    logging.info('tx size: %d bytes' % len(serialized_tx))
    logging.debug('hex: %s' % b2x(serialized_tx))

else:
    # FIXME: the tx could be too long here, but there's no way to get sendmany
    # to *not* broadcast the transaction first. This is a proof-of-concept, so
    # punting.

    logging.debug('Sending publish tx, hex: %s' % b2x(signed_tx.serialize()))
    txid = proxy.sendrawtransaction(signed_tx)
    logging.info('Sent publish tx: %s' % b2lx(txid))

