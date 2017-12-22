#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Test descendant package tracking code

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.mininode import COIN

MAX_ANCESTORS = 25
MAX_DESCENDANTS = 25

class MempoolPackagesTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = False

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-maxorphantx=1000", "-relaypriority=0", "-debug"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-maxorphantx=1000", "-relaypriority=0", "-limitancestorcount=5", "-debug"]))
        connect_nodes(self.nodes[0], 1)
        self.is_network_split = False
        self.sync_all()

    # Build a transaction that spends parent_txid:vout
    # Return amount sent
    def chain_transaction(self, node, parent_txid, vout, value, fee, num_outputs):
        send_value = satoshi_round((value - fee)/num_outputs)
        inputs = [ {'txid' : parent_txid, 'vout' : vout} ]
        outputs = {}
        for i in range(num_outputs):
            outputs[node.getnewaddress()] = send_value
        rawtx = node.createrawtransaction(inputs, outputs)
        signedtx = node.signrawtransaction(rawtx)
        txid = node.sendrawtransaction(signedtx['hex'])
        fulltx = node.getrawtransaction(txid, 1)
        assert(len(fulltx['vout']) == num_outputs) # make sure we didn't generate a change output
        return (txid, send_value)

    def run_test(self):
        ''' Mine some blocks and have them mature. '''
        self.nodes[0].generate(101)
        utxo = self.nodes[0].listunspent(10)
        txid = utxo[0]['txid']
        vout = utxo[0]['vout']
        value = utxo[0]['amount']

        fee = Decimal("0.0001")
        # MAX_ANCESTORS transactions off a confirmed tx should be fine
        chain = []
        for i in range(MAX_ANCESTORS):
            (txid, sent_value) = self.chain_transaction(self.nodes[0], txid, 0, value, fee, 1)
            value = sent_value
            chain.append(txid)

        # Check mempool has MAX_ANCESTORS transactions in it, and descendant
        # count and fees should look correct
        mempool = self.nodes[0].getrawmempool(True)
        assert_equal(len(mempool), MAX_ANCESTORS)
        descendant_count = 1
        descendant_fees = 0
        descendant_size = 0

        for x in reversed(chain):
            assert_equal(mempool[x]['descendantcount'], descendant_count)
            descendant_fees += mempool[x]['fee']
            assert_equal(mempool[x]['modifiedfee'], mempool[x]['fee'])
            assert_equal(mempool[x]['descendantfees'], descendant_fees * COIN)
            descendant_size += mempool[x]['size']
            assert_equal(mempool[x]['descendantsize'], descendant_size)
            descendant_count += 1

        # Check that descendant modified fees includes fee deltas from
        # prioritisetransaction
        self.nodes[0].prioritisetransaction(chain[-1], 0, 1000)
        mempool = self.nodes[0].getrawmempool(True)

        descendant_fees = 0
        for x in reversed(chain):
            descendant_fees += mempool[x]['fee']
            assert_equal(mempool[x]['descendantfees'], descendant_fees * COIN + 1000)

        # Adding one more transaction on to the chain should fail.
        try:
            self.chain_transaction(self.nodes[0], txid, vout, value, fee, 1)
        except JSONRPCException as e:
            print("too-long-ancestor-chain successfully rejected")

        # Check that prioritising a tx before it's added to the mempool works
        # First clear the mempool by mining a block.
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        # Prioritise a transaction that has been mined, then add it back to the
        # mempool by using invalidateblock.
        self.nodes[0].prioritisetransaction(chain[-1], 0, 2000)
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
        # Keep node1's tip synced with node0
        self.nodes[1].invalidateblock(self.nodes[1].getbestblockhash())

        # Now check that the transaction is in the mempool, with the right modified fee
        mempool = self.nodes[0].getrawmempool(True)

        descendant_fees = 0
        for x in reversed(chain):
            descendant_fees += mempool[x]['fee']
            if (x == chain[-1]):
                assert_equal(mempool[x]['modifiedfee'], mempool[x]['fee']+satoshi_round(0.00002))
            assert_equal(mempool[x]['descendantfees'], descendant_fees * COIN + 2000)

        # TODO: check that node1's mempool is as expected

        # TODO: test ancestor size limits

        # Now test descendant chain limits
        txid = utxo[1]['txid']
        value = utxo[1]['amount']
        vout = utxo[1]['vout']

        transaction_package = []
        # First create one parent tx with 10 children
        (txid, sent_value) = self.chain_transaction(self.nodes[0], txid, vout, value, fee, 10)
        parent_transaction = txid
        for i in range(10):
            transaction_package.append({'txid': txid, 'vout': i, 'amount': sent_value})

        for i in range(MAX_DESCENDANTS):
            utxo = transaction_package.pop(0)
            try:
                (txid, sent_value) = self.chain_transaction(self.nodes[0], utxo['txid'], utxo['vout'], utxo['amount'], fee, 10)
                for j in range(10):
                    transaction_package.append({'txid': txid, 'vout': j, 'amount': sent_value})
                if i == MAX_DESCENDANTS - 2:
                    mempool = self.nodes[0].getrawmempool(True)
                    assert_equal(mempool[parent_transaction]['descendantcount'], MAX_DESCENDANTS)
            except JSONRPCException as e:
                print(e.error['message'])
                assert_equal(i, MAX_DESCENDANTS - 1)
                print("tx that would create too large descendant package successfully rejected")

        # TODO: check that node1's mempool is as expected

        # TODO: test descendant size limits

        # Test reorg handling
        # First, the basics:
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)
        self.nodes[1].invalidateblock(self.nodes[0].getbestblockhash())
        self.nodes[1].reconsiderblock(self.nodes[0].getbestblockhash())

        # Now test the case where node1 has a transaction T in its mempool that
        # depends on transactions A and B which are in a mined block, and the
        # block containing A and B is disconnected, AND B is not accepted back
        # into node1's mempool because its ancestor count is too high.

        # Create 8 transactions, like so:
        # Tx0 -> Tx1 (vout0)
        #   \--> Tx2 (vout1) -> Tx3 -> Tx4 -> Tx5 -> Tx6 -> Tx7
        #
        # Mine them in the next block, then generate a new tx8 that spends
        # Tx1 and Tx7, and add to node1's mempool, then disconnect the
        # last block.

        # Create tx0 with 2 outputs
        utxo = self.nodes[0].listunspent()
        txid = utxo[0]['txid']
        value = utxo[0]['amount']
        vout = utxo[0]['vout']

        send_value = satoshi_round((value - fee)/2)
        inputs = [ {'txid' : txid, 'vout' : vout} ]
        outputs = {}
        for i in range(2):
            outputs[self.nodes[0].getnewaddress()] = send_value
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        signedtx = self.nodes[0].signrawtransaction(rawtx)
        txid = self.nodes[0].sendrawtransaction(signedtx['hex'])
        tx0_id = txid
        value = send_value

        # Create tx1
        (tx1_id, tx1_value) = self.chain_transaction(self.nodes[0], tx0_id, 0, value, fee, 1)

        # Create tx2-7
        vout = 1
        txid = tx0_id
        for i in range(6):
            (txid, sent_value) = self.chain_transaction(self.nodes[0], txid, vout, value, fee, 1)
            vout = 0
            value = sent_value

        # Mine these in a block
        self.nodes[0].generate(1)
        self.sync_all()

        # Now generate tx8, with a big fee
        inputs = [ {'txid' : tx1_id, 'vout': 0}, {'txid' : txid, 'vout': 0} ]
        outputs = { self.nodes[0].getnewaddress() : send_value + value - 4*fee }
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        signedtx = self.nodes[0].signrawtransaction(rawtx)
        txid = self.nodes[0].sendrawtransaction(signedtx['hex'])
        sync_mempools(self.nodes)
        
        # Now try to disconnect the tip on each node...
        self.nodes[1].invalidateblock(self.nodes[1].getbestblockhash())
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
        sync_blocks(self.nodes)

if __name__ == '__main__':
    MempoolPackagesTest().main()
