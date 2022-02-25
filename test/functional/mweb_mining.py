#!/usr/bin/env python3
# Copyright (c) 2014-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mining RPCs for MWEB blocks"""

from test_framework.blocktools import (create_coinbase, NORMAL_GBT_REQUEST_PARAMS)
from test_framework.messages import (CBlock, MWEBBlock)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.ltc_util import create_hogex, get_mweb_header_tip, setup_mweb_chain

class MWEBMiningTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.supports_cli = False

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Setup MWEB chain")
        setup_mweb_chain(node)

        # Call getblocktemplate
        node.generatetoaddress(1, node.get_deterministic_priv_key().address)
        gbt = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        next_height = int(gbt["height"])        

        # Build MWEB block
        mweb_header = get_mweb_header_tip(node)
        mweb_header.height = next_height
        mweb_header.rehash()
        mweb_block = MWEBBlock(mweb_header)
        
        # Build coinbase and HogEx txs
        coinbase_tx = create_coinbase(height=next_height)
        hogex_tx = create_hogex(node, mweb_header.hash)
        vtx = [coinbase_tx, hogex_tx]

        # Build block proposal
        block = CBlock()
        block.nVersion = gbt["version"]
        block.hashPrevBlock = int(gbt["previousblockhash"], 16)
        block.nTime = gbt["curtime"]
        block.nBits = int(gbt["bits"], 16)
        block.nNonce = 0
        block.vtx = vtx
        block.mweb_block = mweb_block.serialize().hex()
        block.hashMerkleRoot = block.calc_merkle_root()

        # Call getblocktemplate with the block proposal
        self.log.info("getblocktemplate: Test valid block")
        rsp = node.getblocktemplate(template_request={
            'data': block.serialize().hex(),
            'mode': 'proposal',
            'rules': ['mweb', 'segwit'],
        })
        assert_equal(rsp, None)


if __name__ == '__main__':
    MWEBMiningTest().main()
