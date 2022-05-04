#!/usr/bin/env python3
# Copyright (c) 2021 The Litecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Basic MWEB Wallet test"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.ltc_util import setup_mweb_chain
from test_framework.util import assert_equal

class MWEBWalletBasicTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [['-whitelist=noban@127.0.0.1'],['-whitelist=noban@127.0.0.1'],[]]  # immediate tx relay

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node0 = self.nodes[0]
        node1 = self.nodes[1]
        node2 = self.nodes[2]

        self.log.info("Setting up MWEB chain")
        setup_mweb_chain(node0)
        self.sync_all()
        
        #
        # Send to node1 mweb
        #
        self.log.info("Send to node1 mweb address")
        n1_addr = node1.getnewaddress(address_type='mweb')
        tx1_id = node0.sendtoaddress(n1_addr, 25)
        self.sync_mempools()

        self.log.info("Verify node0's wallet lists the transaction as spent")
        n0_tx1 = node0.gettransaction(txid=tx1_id)
        assert_equal(n0_tx1['confirmations'], 0)
        assert n0_tx1['amount'] == -25
        assert n0_tx1['fee'] < 0 and n0_tx1['fee'] > -0.1

        self.log.info("Verify node1's wallet receives the unconfirmed transaction")
        n1_addr_coins = node1.listreceivedbyaddress(minconf=0, address_filter=n1_addr)
        assert_equal(len(n1_addr_coins), 1)
        assert n1_addr_coins[0]['amount'] == 25
        assert n1_addr_coins[0]['confirmations'] == 0
        assert node1.getbalances()['mine']['untrusted_pending'] == 25
        assert node1.getbalances()['mine']['trusted'] == 0
        
        self.log.info("Mine the next block")
        node0.generate(1)
        self.sync_blocks()
        
        self.log.info("Verify node1's wallet lists the transaction as confirmed")
        n1_addr_coins = node1.listunspent(addresses=[n1_addr])
        assert_equal(len(n1_addr_coins), 1)
        assert n1_addr_coins[0]['amount'] == 25
        assert n1_addr_coins[0]['confirmations'] == 1
        assert node1.getbalances()['mine']['untrusted_pending'] == 0
        assert node1.getbalances()['mine']['trusted'] == 25

        self.log.info("Verify node0's wallet lists the transaction as confirmed")
        n0_tx1 = node0.gettransaction(txid=tx1_id)
        assert_equal(n0_tx1['confirmations'], 1)
        assert n0_tx1['amount'] == -25
        assert n0_tx1['fee'] < 0 and n0_tx1['fee'] > -0.1

        #
        # Pegout to node2
        #
        self.log.info("Send (pegout) to node2 bech32 address")
        n2_addr = node2.getnewaddress(address_type='bech32')
        tx2_id = node1.sendtoaddress(n2_addr, 15)
        self.sync_mempools()

        self.log.info("Verify node1's wallet lists the transactions as spent")
        n1_tx2 = node1.gettransaction(txid=tx2_id)
        assert_equal(n1_tx2['confirmations'], 0)
        assert_equal(n1_tx2['amount'], -15)
        assert n1_tx2['fee'] < 0 and n1_tx2['fee'] > -0.1

        self.log.info("Verify node2's wallet receives the first pegout transaction")
        n2_tx2 = node2.listwallettransactions(txid=tx2_id)[0]
        assert_equal(n2_tx2['amount'], 15)
        assert_equal(n2_tx2['confirmations'], 0)
        assert tx2_id in node1.getrawmempool()

        #
        # Pegout to node2 using subtract fee from amount
        #
        self.log.info("Send (pegout) to node2 bech32 address")
        n2_addr2 = node2.getnewaddress(address_type='bech32')
        tx3_id = node1.sendtoaddress(address=n2_addr2, amount=5, subtractfeefromamount=True)
        self.sync_mempools()
        
        n1_tx3 = node1.gettransaction(txid=tx3_id)
        assert_equal(n1_tx3['confirmations'], 0)
        assert n1_tx3['amount'] > -5 and n1_tx3['amount'] < -4.9
        assert n1_tx3['fee'] < 0 and n1_tx3['fee'] > -0.1

        self.log.info("Verify node2's wallet receives the second pegout transaction")
        n2_addr2_coins = node2.listreceivedbyaddress(minconf=0, address_filter=n2_addr2)
        assert_equal(len(n2_addr2_coins), 1)
        assert n2_addr2_coins[0]['amount'] < 5 and n2_addr2_coins[0]['amount'] > 4.9
        assert_equal(n2_addr2_coins[0]['confirmations'], 0)
        assert tx3_id in node1.getrawmempool()

        self.log.info("Mine next block to make sure the transactions confirm successfully")
        node0.generate(1)
        self.sync_all()
        assert tx2_id not in node1.getrawmempool()
        assert tx3_id not in node1.getrawmempool()        

        n2_addr2_coins = node2.listreceivedbyaddress(minconf=0, address_filter=n2_addr2)
        assert_equal(len(n2_addr2_coins), 1)
        assert n2_addr2_coins[0]['amount'] < 5 and n2_addr2_coins[0]['amount'] > 4.9
        assert_equal(n2_addr2_coins[0]['confirmations'], 1)

        n2_balances = node2.getbalances()['mine']
        assert n2_balances['immature'] > 19.9 and n2_balances['immature'] < 20
        assert_equal(n2_balances['untrusted_pending'], 0)
        assert_equal(n2_balances['trusted'], 0)

        # TODO: Conflicting txs
        # TODO: Duplicate hash

if __name__ == '__main__':
    MWEBWalletBasicTest().main()
