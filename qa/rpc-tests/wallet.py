#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Exercise the wallet.  Ported from wallet.sh.  
# Does the following:
#   a) creates 3 nodes, with an empty chain (no blocks).
#   b) node0 mines a block
#   c) node1 mines 101 blocks, so now nodes 0 and 1 have 50btc, node2 has none. 
#   d) node0 sends 21 btc to node2, in two transactions (11 btc, then 10 btc).
#   e) node0 mines a block, collects the fee on the second transaction
#   f) node1 mines 100 blocks, to mature node0's just-mined block
#   g) check that node0 has 100-21, node2 has 21
#   h) node0 should now have 2 unspent outputs;  send these to node2 via raw tx broadcast by node1
#   i) have node1 mine a block
#   j) check balances - node0 should have 0, node2 should have 100
#   k) test ResendWalletTransactions - create transactions, startup fourth node, make sure it syncs
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class WalletTest (BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 4)

    def setup_network(self, split=False):
        self.nodes = start_nodes(3, self.options.tmpdir)
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        self.is_network_split=False
        self.sync_all()

    def run_test (self):
        print "Mining blocks..."

        self.nodes[0].generate(1)

        walletinfo = self.nodes[0].getwalletinfo()
        assert_equal(walletinfo['immature_balance'], 50)
        assert_equal(walletinfo['balance'], 0)

        self.sync_all()
        self.nodes[1].generate(101)
        self.sync_all()

        assert_equal(self.nodes[0].getbalance(), 50)
        assert_equal(self.nodes[1].getbalance(), 50)
        assert_equal(self.nodes[2].getbalance(), 0)

        # Send 21 BTC from 0 to 2 using sendtoaddress call.
        # Second transaction will be child of first, and will require a fee
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 11)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 10)

        walletinfo = self.nodes[0].getwalletinfo()
        assert_equal(walletinfo['immature_balance'], 0)

        # Have node0 mine a block, thus it will collect its own fee.
        self.nodes[0].generate(1)
        self.sync_all()

        # Have node1 generate 100 blocks (so node0 can recover the fee)
        self.nodes[1].generate(100)
        self.sync_all()

        # node0 should end up with 100 btc in block rewards plus fees, but
        # minus the 21 plus fees sent to node2
        assert_equal(self.nodes[0].getbalance(), 100-21)
        assert_equal(self.nodes[2].getbalance(), 21)

        # Node0 should have two unspent outputs.
        # Create a couple of transactions to send them to node2, submit them through 
        # node1, and make sure both node0 and node2 pick them up properly: 
        node0utxos = self.nodes[0].listunspent(1)
        assert_equal(len(node0utxos), 2)

        # create both transactions
        txns_to_send = []
        for utxo in node0utxos: 
            inputs = []
            outputs = {}
            inputs.append({ "txid" : utxo["txid"], "vout" : utxo["vout"]})
            outputs[self.nodes[2].getnewaddress("from1")] = utxo["amount"]
            raw_tx = self.nodes[0].createrawtransaction(inputs, outputs)
            txns_to_send.append(self.nodes[0].signrawtransaction(raw_tx))

        # Have node 1 (miner) send the transactions
        self.nodes[1].sendrawtransaction(txns_to_send[0]["hex"], True)
        self.nodes[1].sendrawtransaction(txns_to_send[1]["hex"], True)

        # Have node1 mine a block to confirm transactions:
        self.nodes[1].generate(1)
        self.sync_all()

        assert_equal(self.nodes[0].getbalance(), 0)
        assert_equal(self.nodes[2].getbalance(), 100)
        assert_equal(self.nodes[2].getbalance("from1"), 100-21)

        # Send 10 BTC normal
        address = self.nodes[0].getnewaddress("test")
        self.nodes[2].settxfee(Decimal('0.001'))
        txid = self.nodes[2].sendtoaddress(address, 10, "", "", False)
        self.nodes[2].generate(1)
        self.sync_all()
        assert_equal(self.nodes[2].getbalance(), Decimal('89.99900000'))
        assert_equal(self.nodes[0].getbalance(), Decimal('10.00000000'))

        # Send 10 BTC with subtract fee from amount
        txid = self.nodes[2].sendtoaddress(address, 10, "", "", True)
        self.nodes[2].generate(1)
        self.sync_all()
        assert_equal(self.nodes[2].getbalance(), Decimal('79.99900000'))
        assert_equal(self.nodes[0].getbalance(), Decimal('19.99900000'))

        # Sendmany 10 BTC
        txid = self.nodes[2].sendmany('from1', {address: 10}, 0, "", [])
        self.nodes[2].generate(1)
        self.sync_all()
        assert_equal(self.nodes[2].getbalance(), Decimal('69.99800000'))
        assert_equal(self.nodes[0].getbalance(), Decimal('29.99900000'))

        # Sendmany 10 BTC with subtract fee from amount
        txid = self.nodes[2].sendmany('from1', {address: 10}, 0, "", [address])
        self.nodes[2].generate(1)
        self.sync_all()
        assert_equal(self.nodes[2].getbalance(), Decimal('59.99800000'))
        assert_equal(self.nodes[0].getbalance(), Decimal('39.99800000'))

        # Test ResendWalletTransactions:
        # Create a couple of transactions, then start up a fourth
        # node (nodes[3]) and ask nodes[0] to rebroadcast.
        # EXPECT: nodes[3] should have those transactions in its mempool.
        txid1 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        txid2 = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        sync_mempools(self.nodes)

        self.nodes.append(start_node(3, self.options.tmpdir))
        connect_nodes_bi(self.nodes, 0, 3)
        sync_blocks(self.nodes)

        relayed = self.nodes[0].resendwallettransactions()
        assert_equal(set(relayed), set([txid1, txid2]))
        sync_mempools(self.nodes)

        assert(txid1 in self.nodes[3].getrawmempool())
        
        #check if we can list zero value tx as available coins
        #1. create rawtx
        #2. hex-changed one output to 0.0 
        #3. sign and send
        #4. check if recipient (node0) can list the zero value tx
        usp = self.nodes[1].listunspent()
        inputs = [{"txid":usp[0]['txid'], "vout":usp[0]['vout']}]
        outputs = {self.nodes[1].getnewaddress(): 49.998, self.nodes[0].getnewaddress(): 11.11}
        
        rawTx = self.nodes[1].createrawtransaction(inputs, outputs).replace("c0833842", "00000000") #replace 11.11 with 0.0 (int32)
        decRawTx = self.nodes[1].decoderawtransaction(rawTx)
        signedRawTx = self.nodes[1].signrawtransaction(rawTx)
        decRawTx = self.nodes[1].decoderawtransaction(signedRawTx['hex'])
        zeroValueTxid= decRawTx['txid']
        sendResp = self.nodes[1].sendrawtransaction(signedRawTx['hex'])
        
        self.sync_all()
        self.nodes[1].generate(1) #mine a block
        self.sync_all()
        
        unspentTxs = self.nodes[0].listunspent() #zero value tx must be in listunspents output
        found = False
        for uTx in unspentTxs:
            if uTx['txid'] == zeroValueTxid:
                found = True
                assert_equal(uTx['amount'], Decimal('0.00000000'));
        assert(found)
        
        #do some -walletbroadcast tests
        stop_nodes(self.nodes)
        wait_bitcoinds()
        self.nodes = start_nodes(3, self.options.tmpdir, [["-walletbroadcast=0"],["-walletbroadcast=0"],["-walletbroadcast=0"]])
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        self.sync_all()

        txIdNotBroadcasted  = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 2);
        txObjNotBroadcasted = self.nodes[0].gettransaction(txIdNotBroadcasted)
        self.nodes[1].generate(1) #mine a block, tx should not be in there
        self.sync_all()
        assert_equal(self.nodes[2].getbalance(), Decimal('59.99800000')); #should not be changed because tx was not broadcasted
        
        #now broadcast from another node, mine a block, sync, and check the balance
        self.nodes[1].sendrawtransaction(txObjNotBroadcasted['hex'])
        self.nodes[1].generate(1)
        self.sync_all()
        txObjNotBroadcasted = self.nodes[0].gettransaction(txIdNotBroadcasted)
        assert_equal(self.nodes[2].getbalance(), Decimal('61.99800000')); #should not be
        
        #create another tx
        txIdNotBroadcasted  = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 2);
        
        #restart the nodes with -walletbroadcast=1
        stop_nodes(self.nodes)
        wait_bitcoinds()
        self.nodes = start_nodes(3, self.options.tmpdir)
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        sync_blocks(self.nodes)
        
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)
        
        #tx should be added to balance because after restarting the nodes tx should be broadcastet
        assert_equal(self.nodes[2].getbalance(), Decimal('63.99800000')); #should not be
        
if __name__ == '__main__':
    WalletTest ().main ()
