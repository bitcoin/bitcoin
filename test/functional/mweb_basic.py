#!/usr/bin/env python3
# Copyright (c) 2021 The Litecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Basic MWEB test"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class MWEBBasicTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.extra_args = [['-whitelist=noban@127.0.0.1'],[]]  # immediate tx relay
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Create all but one pre-MWEB blocks")
        self.nodes[0].generate(430)

        self.log.info("Pegin some coins")
        addr0 = self.nodes[0].getnewaddress(address_type='mweb')
        pegin1_txid = self.nodes[0].sendtoaddress(addr0, 10)

        self.log.info("Ensure pegin transaction was not accepted to mempool, and abandon it")
        assert_equal(set(self.nodes[0].getrawmempool()), set())
        self.nodes[0].abandontransaction(pegin1_txid)

        self.log.info("Generate final pre-MWEB block and pegin, ensuring tx is accepted to mempool")
        self.nodes[0].generate(1)
        pegin2_txid = self.nodes[0].sendtoaddress(addr0, 10)
        self.sync_all();
        assert_equal(set(self.nodes[0].getrawmempool()), {pegin2_txid})
        assert_equal(set(self.nodes[1].getrawmempool()), {pegin2_txid})

        self.log.info("Create some blocks - activate MWEB")
        self.nodes[0].generate(10)
        self.sync_all()

        self.log.info("Check for MWEB UTXOs")
        utxos = [x for x in self.nodes[0].listunspent() if x['address'].startswith('tmweb')]
        assert_equal(len(utxos), 2)
        utxos.sort(key=lambda x: x['amount'])

        utxo0 = utxos[0]
        utxo1 = utxos[1]
        if utxos[0]['address'] != addr0:
            utxo0 = utxos[1]
            utxo1 = utxos[0]
          
        assert utxo0['amount'] == 10 and utxo0['address'] == addr0
        assert 2 < utxo1['amount'] < 2.5 # change from single 12.5 LTC coinbase being spent

        self.log.info("Send MWEB coins to node 1")
        addr1 = self.nodes[1].getnewaddress(address_type='mweb')
        self.nodes[0].sendtoaddress(addr1, 5)
        self.nodes[0].generate(1)
        self.sync_all()

        self.log.info("Check MWEB coins are spent on node 0")
        utxos = [x for x in self.nodes[0].listunspent() if x['address'].startswith('tmweb')]
        assert_equal(len(utxos), 2)
        assert sum(x['amount'] for x in utxos) < 45

        self.log.info("Check for MWEB UTXO on node 1")
        utxos = [x for x in self.nodes[1].listunspent() if x['address'].startswith('tmweb')]
        assert_equal(len(utxos), 1)
        assert utxos[0]['amount'] == 5 and utxos[0]['address'] == addr1

        self.log.info("Send MWEB coins to node 0")
        self.nodes[1].sendtoaddress(addr0, 2)
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()

        self.log.info("Check MWEB coins are spent on node 1")
        utxos = [x for x in self.nodes[1].listunspent() if x['address'].startswith('tmweb')]
        assert_equal(len(utxos), 1)
        assert sum(x['amount'] for x in utxos) < 3
        self.log.info("UTXO amount: {}".format(utxos[0]['amount']))

        self.log.info("Check for MWEB UTXO on node 0")
        utxos = self.nodes[0].listunspent(addresses=[addr0])
        assert_equal(len(utxos), 1)
        assert utxos[0]['amount'] == 2 and utxos[0]['address'] == addr0

        self.log.info("Pegout coins on node 1")
        addr2 = self.nodes[1].getnewaddress()
        self.nodes[1].sendtoaddress(addr2, 2)
        self.sync_all()
        self.nodes[1].generate(1)
        self.sync_all()

        self.log.info("Check MWEB coins are spent on node 1")
        utxos = [x for x in self.nodes[1].listunspent() if x['address'].startswith('tmweb')]
        assert_equal(len(utxos), 1)
        assert sum(x['amount'] for x in utxos) < 1

        self.log.info("Mine 4 more blocks. Peg-out maturity is 6 blocks, so coins shouldn't be available yet.")
        self.nodes[1].generate(4)
        self.sync_all()

        self.log.info("Check for UTXO on node 1")
        utxos = self.nodes[1].listunspent(addresses=[addr2])
        assert_equal(len(utxos), 0)
        
        self.log.info("Mine 1 more block. Peg-out coins should mature.")
        self.nodes[1].generate(1)
        self.sync_all()

        self.log.info("Check for UTXO on node 1")
        utxos = self.nodes[1].listunspent(addresses=[addr2])
        assert_equal(len(utxos), 1)
        assert utxos[0]['amount'] == 2 and utxos[0]['address'] == addr2

if __name__ == '__main__':
    MWEBBasicTest().main()
