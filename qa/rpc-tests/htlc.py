#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test usage of HTLC transactions with RPC
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.mininode import sha256, ripemd160

class HTLCTest(BitcoinTestFramework):
    BUYER = 0
    SELLER = 1

    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = False

    def setup_network(self):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        connect_nodes_bi(self.nodes, 0, 1)
        self.is_network_split = False
        self.sync_all()

    def activateCSV(self):
        # activation should happen at block height 432 (3 periods)
        min_activation_height = 432
        height = self.nodes[0].getblockcount()
        assert(height < 432)
        self.nodes[0].generate(432-height)
        assert(get_bip9_status(self.nodes[0], 'csv')['status'] == 'active')
        sync_blocks(self.nodes)

    def run_test(self):
        # Activate checksequenceverify
        self.activateCSV()

        # The buyer wishes to purchase the preimage of "254e38932fdb9fc27f82aac2a5cc6d789664832383e3cf3298f8c120812712db"
        image = "254e38932fdb9fc27f82aac2a5cc6d789664832383e3cf3298f8c120812712db"
        # The seller wishes to sell the preimage
        preimage = "696c6c756d696e617469"

        assert_equal(image, bytes_to_hex_str(sha256(hex_str_to_bytes(preimage))))

        self.run_tests_with_preimage_and_image(preimage, image)

        image = "5bb50b07a120dba7f1aae9623825071bc1fe4b40"
        preimage = "ffffffffff"

        assert_equal(image, bytes_to_hex_str(ripemd160(hex_str_to_bytes(preimage))))

        self.run_tests_with_preimage_and_image(preimage, image)

    def run_tests_with_preimage_and_image(self, preimage, image):
        self.test_refund(image, 10)
        self.test_refund(image, 100)

        assert_equal(False, self.test_sell(image, 10))
        self.nodes[self.SELLER].importpreimage(preimage)
        assert(self.test_sell(image, 10))

    def test_refund(self, image, num_blocks):
        (seller_spending_tx, buyer_refund_tx) = self.fund_htlc(image, num_blocks)

        # The buyer signs the refund transaction
        buyer_sign = self.nodes[self.BUYER].signrawtransaction(buyer_refund_tx)
        assert_equal(buyer_sign["complete"], True)

        # The buyer should not be able to spend the funds yet
        assert(self.expect_cannot_send(self.BUYER, buyer_sign["hex"]))

        # After appearing in num_blocks number of blocks, the buyer can.
        self.nodes[self.BUYER].generate(num_blocks-1)
        sync_blocks(self.nodes)

        self.nodes[self.BUYER].sendrawtransaction(buyer_sign["hex"])

        self.nodes[self.BUYER].generate(1)
        sync_blocks(self.nodes)

    def test_sell(self, image, num_blocks):
        (seller_spending_tx, buyer_refund_tx) = self.fund_htlc(image, num_blocks)

        # The seller signs the spending transaction
        seller_sign = self.nodes[self.SELLER].signrawtransaction(seller_spending_tx)

        if seller_sign["complete"] == False:
            return False

        self.nodes[self.SELLER].sendrawtransaction(seller_sign["hex"])

        return True

    def fund_htlc(self, image, num_blocks):
        buyer_addr = self.nodes[self.BUYER].getnewaddress("")
        buyer_pubkey = self.nodes[self.BUYER].validateaddress(buyer_addr)["pubkey"]
        seller_addr = self.nodes[self.SELLER].getnewaddress("")
        seller_pubkey = self.nodes[self.SELLER].validateaddress(seller_addr)["pubkey"]

        # Create the HTLC transaction
        htlc = self.nodes[self.BUYER].createhtlc(seller_pubkey, buyer_pubkey, image, str(num_blocks))

        # Import into wallets
        self.nodes[self.BUYER].importaddress(htlc["redeemScript"], "", False, True)
        self.nodes[self.SELLER].importaddress(htlc["redeemScript"], "", False, True)

        # Buyer sends the funds
        self.nodes[self.BUYER].sendtoaddress(htlc["address"], 10)
        self.nodes[self.BUYER].generate(1)
        sync_blocks(self.nodes)

        funding_tx = False

        for tx in self.nodes[self.SELLER].listtransactions("*", 500, 0, True):
            if tx["address"] == htlc["address"]:
                funding_tx = tx

        assert(funding_tx != False)

        seller_spending_tx = self.nodes[self.SELLER].createrawtransaction(
            [{"txid": funding_tx["txid"], "vout": funding_tx["vout"]}],
            {seller_addr: 9.99}
        )
        buyer_refund_tx = self.nodes[self.BUYER].createrawtransaction(
            [{"txid": funding_tx["txid"], "vout": funding_tx["vout"], "sequence": num_blocks}],
            {buyer_addr: 9.99}
        )

        # TODO: why isn't this already version 2? or shouldn't the signer figure
        # out that it's necessary?
        seller_spending_tx = "02" + seller_spending_tx[2:]
        buyer_refund_tx = "02" + buyer_refund_tx[2:]

        return (seller_spending_tx, buyer_refund_tx)

    def expect_cannot_send(self, i, tx):
        exception_triggered = False

        try:
            self.nodes[i].sendrawtransaction(tx)
        except JSONRPCException:
            exception_triggered = True

        return exception_triggered

if __name__ == '__main__':
    HTLCTest().main()

