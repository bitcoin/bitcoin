#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test signet mining RPCs

- getmininginfo
- getblocktemplate proposal mode
- submitblock"""

from decimal import Decimal

from test_framework.blocktools import (
    create_coinbase,
)
from test_framework.messages import (
    CBlock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

private_key  = "ZFNp4W4HSatHDc4zsheMsDWgUg2uiB5PtnY2Y2hUCNCkQiF81KyV"
pubkey       = "02c72b24ae2ee333f2da24aea66ce4338c01db8f26d0cc96f586d77edcb5238a4f"
address      = "sb1qghzjzc0jvkjvvvymxnadmjjpu2tywmvagduwfj"
blockscript  = "5121" + pubkey + "51ae" # 1-of-1 multisig

def assert_template(node, block, expect, rehash=True):
    if rehash:
        block.hashMerkleRoot = block.calc_merkle_root()
    rsp = node.getblocktemplate(template_request={'data': block.serialize().hex(), 'mode': 'proposal', 'rules': ['segwit']})
    assert_equal(rsp, expect)

def generate(node, count):
    for _ in range(count):
        addr = node.getnewaddress()
        block = node.getnewblockhex(addr)
        signed = node.signblock(block)
        node.grindblock(signed)


class SigMiningTest(BitcoinTestFramework):
    def set_test_params(self):
        self.chain = "signet"
        self.num_nodes = 2
        self.setup_clean_chain = True
        shared_args = ["-signet_blockscript=" + blockscript, "-signet_seednode=localhost:1234"]
        self.extra_args = [shared_args, shared_args]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]

        # give the privkey to node 1 so it can sign
        node.importprivkey(private_key)
        self.log.info('Imported network private key')
        self.log.info('address: %s, privkey: %s', address, node.dumpprivkey(address))

        self.log.info('getmininginfo')
        mining_info = node.getmininginfo()
        assert_equal(mining_info['blocks'], 0)
        assert_equal(mining_info['chain'], 'signet')
        assert 'currentblocktx' not in mining_info
        assert 'currentblockweight' not in mining_info
        assert_equal(mining_info['networkhashps'], Decimal('0'))
        assert_equal(mining_info['pooledtx'], 0)

        # Mine a block to leave initial block download
        # Actually we mine 20 cause there's a bug in the coinbase height serializers

        generate(node, 20)
        tmpl = node.getblocktemplate({'rules': ['segwit']})
        self.log.info("getblocktemplate: Test capability advertised")
        assert 'proposal' in tmpl['capabilities']
        assert 'coinbasetxn' not in tmpl

        coinbase_tx = create_coinbase(height=int(tmpl["height"]))
        # sequence numbers must not be max for nLockTime to have effect
        coinbase_tx.vin[0].nSequence = 2 ** 32 - 2
        coinbase_tx.rehash()

        block = CBlock()
        block.nVersion = tmpl["version"]
        block.hashPrevBlock = int(tmpl["previousblockhash"], 16)
        block.nTime = tmpl["curtime"]
        block.nBits = int(tmpl["bits"], 16)
        block.nNonce = 0
        block.vtx = [coinbase_tx]

if __name__ == '__main__':
    SigMiningTest().main()
