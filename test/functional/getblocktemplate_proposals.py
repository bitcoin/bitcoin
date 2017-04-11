#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test block proposals with getblocktemplate."""

from binascii import a2b_hex, b2a_hex
from hashlib import sha256
from struct import pack

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

def b2x(b):
    return b2a_hex(b).decode('ascii')

# NOTE: This does not work for signed numbers (set the high bit) or zero (use b'\0')
def encodeUNum(n):
    s = bytearray(b'\1')
    while n > 127:
        s[0] += 1
        s.append(n % 256)
        n //= 256
    s.append(n)
    return bytes(s)

def varlenEncode(n):
    if n < 0xfd:
        return pack('<B', n)
    if n <= 0xffff:
        return b'\xfd' + pack('<H', n)
    if n <= 0xffffffff:
        return b'\xfe' + pack('<L', n)
    return b'\xff' + pack('<Q', n)

def dblsha(b):
    return sha256(sha256(b).digest()).digest()

def genmrklroot(leaflist):
    cur = leaflist
    while len(cur) > 1:
        n = []
        if len(cur) & 1:
            cur.append(cur[-1])
        for i in range(0, len(cur), 2):
            n.append(dblsha(cur[i] + cur[i + 1]))
        cur = n
    return cur[0]

def template_to_bytearray(tmpl, txlist):
    blkver = pack('<L', tmpl['version'])
    mrklroot = genmrklroot(list(dblsha(a) for a in txlist))
    timestamp = pack('<L', tmpl['curtime'])
    nonce = b'\0\0\0\0'
    blk = blkver + a2b_hex(tmpl['previousblockhash'])[::-1] + mrklroot + timestamp + a2b_hex(tmpl['bits'])[::-1] + nonce
    blk += varlenEncode(len(txlist))
    for tx in txlist:
        blk += tx
    return bytearray(blk)

def template_to_hex(tmpl, txlist):
    return b2x(template_to_bytearray(tmpl, txlist))

def assert_template(node, tmpl, txlist, expect):
    rsp = node.getblocktemplate({'data': template_to_hex(tmpl, txlist), 'mode': 'proposal'})
    if rsp != expect:
        raise AssertionError('unexpected: %s' % (rsp,))

class GetBlockTemplateProposalTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = False

    def run_test(self):
        node = self.nodes[0]
        # Mine a block to leave initial block download
        node.generate(1)
        tmpl = node.getblocktemplate()
        if 'coinbasetxn' not in tmpl:
            rawcoinbase = encodeUNum(tmpl['height'])
            rawcoinbase += b'\x01-'
            hexcoinbase = b2x(rawcoinbase)
            hexoutval = b2x(pack('<Q', tmpl['coinbasevalue']))
            tmpl['coinbasetxn'] = {'data': '01000000' + '01' + '0000000000000000000000000000000000000000000000000000000000000000ffffffff' + ('%02x' % (len(rawcoinbase),)) + hexcoinbase + 'fffffffe' + '01' + hexoutval + '00' + '00000000'}
        txlist = list(bytearray(a2b_hex(a['data'])) for a in (tmpl['coinbasetxn'],) + tuple(tmpl['transactions']))

        self.log.info("getblocktemplate: Test capability advertised")
        assert('proposal' in tmpl['capabilities'])

        self.log.info("getblocktemplate: Test bad input hash for coinbase transaction")
        txlist[0][4 + 1] += 1
        assert_template(node, tmpl, txlist, 'bad-cb-missing')
        txlist[0][4 + 1] -= 1

        self.log.info("getblocktemplate: Test truncated final transaction")
        lastbyte = txlist[-1].pop()
        assert_raises_jsonrpc(-22, "Block decode failed", assert_template, node, tmpl, txlist, 'n/a')
        txlist[-1].append(lastbyte)

        self.log.info("getblocktemplate: Test duplicate transaction")
        txlist.append(txlist[0])
        assert_template(node, tmpl, txlist, 'bad-txns-duplicate')
        txlist.pop()

        self.log.info("getblocktemplate: Test invalid transaction")
        txlist.append(bytearray(txlist[0]))
        txlist[-1][4 + 1] = 0xff
        assert_template(node, tmpl, txlist, 'bad-txns-inputs-missingorspent')
        txlist.pop()

        self.log.info("getblocktemplate: Test nonfinal transaction")
        txlist[0][-4:] = b'\xff\xff\xff\xff'
        assert_template(node, tmpl, txlist, 'bad-txns-nonfinal')
        txlist[0][-4:] = b'\0\0\0\0'

        self.log.info("getblocktemplate: Test bad tx count")
        txlist.append(b'')
        assert_raises_jsonrpc(-22, 'Block decode failed', assert_template, node, tmpl, txlist, 'n/a')
        txlist.pop()

        self.log.info("getblocktemplate: Test bad bits")
        realbits = tmpl['bits']
        tmpl['bits'] = '1c0000ff'  # impossible in the real world
        assert_template(node, tmpl, txlist, 'bad-diffbits')
        tmpl['bits'] = realbits

        self.log.info("getblocktemplate: Test bad merkle root")
        rawtmpl = template_to_bytearray(tmpl, txlist)
        rawtmpl[4 + 32] = (rawtmpl[4 + 32] + 1) % 0x100
        rsp = node.getblocktemplate({'data': b2x(rawtmpl), 'mode': 'proposal'})
        if rsp != 'bad-txnmrklroot':
            raise AssertionError('unexpected: %s' % (rsp,))

        self.log.info("getblocktemplate: Test bad timestamps")
        realtime = tmpl['curtime']
        tmpl['curtime'] = 0x7fffffff
        assert_template(node, tmpl, txlist, 'time-too-new')
        tmpl['curtime'] = 0
        assert_template(node, tmpl, txlist, 'time-too-old')
        tmpl['curtime'] = realtime

        self.log.info("getblocktemplate: Test valid block")
        assert_template(node, tmpl, txlist, None)

        self.log.info("getblocktemplate: Test not best block")
        tmpl['previousblockhash'] = 'ff00' * 16
        assert_template(node, tmpl, txlist, 'inconclusive-not-best-prevblk')

if __name__ == '__main__':
    GetBlockTemplateProposalTest().main()
