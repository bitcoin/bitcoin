#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test freeze."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

def rollback_chain(node, address):
        # Rolling back the chain to test reversing the last FREEZE tx
        blockcount = node.getblockcount()
        blockhash = node.getblockhash(blockcount)
        node.invalidateblock(blockhash)
        node.clearmempool()
        node.generatetoaddress(1, address)
        new_blockcount = node.getblockcount()
        new_blockhash = node.getblockhash(new_blockcount)

        # Checking the block count is the same as before the rollback...
        assert_equal(blockcount, new_blockcount)

        # Checking the block hash is different from before the rollback...
        if blockhash == new_blockhash:
            raise AssertionError("Block hashes should differ after reorg")

class OmniDelegation(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [['-omniactivationallowsender=any']]

    def run_test(self):
        self.log.info("Test delegation of token issuance")

        node = self.nodes[0]

        # Obtaining addresses to work with
        issuer_address = node.getnewaddress()
        delegate_address = node.getnewaddress()
        sink_address = node.getnewaddress()
        coinbase_address = node.getnewaddress()

        # Preparing some mature Bitcoins
        node.generatetoaddress(105, coinbase_address)

        # Funding the addresses with some testnet BTC for fees
        node.sendmany("", {issuer_address: 5, delegate_address: 3, sink_address: 4})
        node.sendtoaddress(issuer_address, 6)
        node.sendtoaddress(issuer_address, 7)
        node.sendtoaddress(issuer_address, 8)
        node.sendtoaddress(issuer_address, 9)
        node.generatetoaddress(1, coinbase_address)

        # Creating a test (managed) property and granting 1000 tokens to the test address\
        node.omni_sendissuancemanaged(issuer_address, 1, 1, 0, "TestCat", "TestSubCat", "TestProperty", "TestURL", "TestData")
        node.generatetoaddress(1, coinbase_address)
        node.omni_sendgrant(issuer_address, sink_address, 3, "1000")
        node.generatetoaddress(1, coinbase_address)

        # Try to issue new tokens from the delegate address without delegating access to it
        txid = node.omni_sendgrant(delegate_address, sink_address, 3, "55")
        node.generatetoaddress(1, coinbase_address)
        tx = node.omni_gettransaction(txid)
        assert_equal(tx['valid'], False)

        # Name delegate
        txid = node.omni_sendadddelegate(issuer_address, 3, delegate_address)
        node.generatetoaddress(1, coinbase_address)
        tx = node.omni_gettransaction(txid)
        info = node.omni_getproperty(3)
        assert_equal(tx['valid'], True)
        assert_equal(info['delegate'], delegate_address)

        # Try again to issue new tokens from the delegate address
        txid = node.omni_sendgrant(delegate_address, sink_address, 3, "77")
        node.generatetoaddress(1, coinbase_address)
        tx = node.omni_gettransaction(txid)
        assert_equal(tx['valid'], True)

        # Remove delegate status
        txid = node.omni_sendremovedelegate(issuer_address, 3, delegate_address)
        node.generatetoaddress(1, coinbase_address)
        tx = node.omni_gettransaction(txid)
        info = node.omni_getproperty(3)
        assert_equal(tx['valid'], True)
        assert_equal(info['delegate'], "")

        # Try again to issue new tokens from the delegate address
        txid = node.omni_sendgrant(delegate_address, sink_address, 3, "33")
        node.generatetoaddress(1, coinbase_address)
        tx = node.omni_gettransaction(txid)
        assert_equal(tx['valid'], False)

        # Check final balance
        sink_balance = node.omni_getbalance(sink_address, 3)
        assert_equal(sink_balance['balance'], "1077")


if __name__ == '__main__':
    OmniDelegation().main()
