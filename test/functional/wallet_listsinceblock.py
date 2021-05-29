#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listsincelast RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_array_result, assert_raises_rpc_error

class ListSinceBlockTest (BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.setup_clean_chain = True

    def run_test(self):
        self.nodes[2].generate(101)
        self.sync_all()

        self.test_no_blockhash()
        self.test_invalid_blockhash()
        self.test_reorg()
        self.test_double_spend()
        self.test_double_send()

    def test_no_blockhash(self):
        txid = self.nodes[2].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        blockhash, = self.nodes[2].generate(1)
        self.sync_all()

        txs = self.nodes[0].listtransactions()
        assert_array_result(txs, {"txid": txid}, {
            "category": "receive",
            "amount": 1,
            "blockhash": blockhash,
            "confirmations": 1,
        })
        assert_equal(
            self.nodes[0].listsinceblock(),
            {"lastblock": blockhash,
             "removed": [],
             "transactions": txs})
        assert_equal(
            self.nodes[0].listsinceblock(""),
            {"lastblock": blockhash,
             "removed": [],
             "transactions": txs})

    def test_invalid_blockhash(self):
        assert_raises_rpc_error(-5, "Block not found", self.nodes[0].listsinceblock,
                                "42759cde25462784395a337460bde75f58e73d3f08bd31fdc3507cbac856a2c4")
        assert_raises_rpc_error(-5, "Block not found", self.nodes[0].listsinceblock,
                                "0000000000000000000000000000000000000000000000000000000000000000")
        assert_raises_rpc_error(-5, "Block not found", self.nodes[0].listsinceblock,
                                "invalid-hex")

    def test_reorg(self):
        '''
        `listsinceblock` did not behave correctly when handed a block that was
        no longer in the main chain:

             ab0
          /       \
        aa1 [tx0]   bb1
         |           |
        aa2         bb2
         |           |
        aa3         bb3
                     |
                    bb4

        Consider a client that has only seen block `aa3` above. It asks the node
        to `listsinceblock aa3`. But at some point prior the main chain switched
        to the bb chain.

        Previously: listsinceblock would find height=4 for block aa3 and compare
        this to height=5 for the tip of the chain (bb4). It would then return
        results restricted to bb3-bb4.

        Now: listsinceblock finds the fork at ab0 and returns results in the
        range bb1-bb4.

        This test only checks that [tx0] is present.
        '''

        # Split network into two
        self.split_network()

        # send to nodes[0] from nodes[2]
        senttx = self.nodes[2].sendtoaddress(self.nodes[0].getnewaddress(), 1)

        # generate on both sides
        lastblockhash = self.nodes[1].generate(6)[5]
        self.nodes[2].generate(7)
        self.log.info('lastblockhash=%s' % (lastblockhash))

        self.sync_all(self.nodes[:2])
        self.sync_all(self.nodes[2:])

        self.join_network()

        # listsinceblock(lastblockhash) should now include tx, as seen from nodes[0]
        lsbres = self.nodes[0].listsinceblock(lastblockhash)
        found = False
        for tx in lsbres['transactions']:
            if tx['txid'] == senttx:
                found = True
                break
        assert found

    def test_double_spend(self):
        '''
        This tests the case where the same UTXO is spent twice on two separate
        blocks as part of a reorg.

             ab0
          /       \
        aa1 [tx1]   bb1 [tx2]
         |           |
        aa2         bb2
         |           |
        aa3         bb3
                     |
                    bb4

        Problematic case:

        1. User 1 receives BTC in tx1 from utxo1 in block aa1.
        2. User 2 receives BTC in tx2 from utxo1 (same) in block bb1
        3. User 1 sees 2 confirmations at block aa3.
        4. Reorg into bb chain.
        5. User 1 asks `listsinceblock aa3` and does not see that tx1 is now
           invalidated.

        Currently the solution to this is to detect that a reorg'd block is
        asked for in listsinceblock, and to iterate back over existing blocks up
        until the fork point, and to include all transactions that relate to the
        node wallet.
        '''

        self.sync_all()

        # Split network into two
        self.split_network()

        # share utxo between nodes[1] and nodes[2]
        utxos = self.nodes[2].listunspent()
        utxo = utxos[0]
        privkey = self.nodes[2].dumpprivkey(utxo['address'])
        self.nodes[1].importprivkey(privkey)

        # send from nodes[1] using utxo to nodes[0]
        change = '%.8f' % (float(utxo['amount']) - 1.0003)
        recipient_dict = {
            self.nodes[0].getnewaddress(): 1,
            self.nodes[1].getnewaddress(): change,
        }
        utxo_dicts = [{
            'txid': utxo['txid'],
            'vout': utxo['vout'],
        }]
        txid1 = self.nodes[1].sendrawtransaction(
            self.nodes[1].signrawtransactionwithwallet(
                self.nodes[1].createrawtransaction(utxo_dicts, recipient_dict))['hex'])

        # send from nodes[2] using utxo to nodes[3]
        recipient_dict2 = {
            self.nodes[3].getnewaddress(): 1,
            self.nodes[2].getnewaddress(): change,
        }
        self.nodes[2].sendrawtransaction(
            self.nodes[2].signrawtransactionwithwallet(
                self.nodes[2].createrawtransaction(utxo_dicts, recipient_dict2))['hex'])

        # generate on both sides
        lastblockhash = self.nodes[1].generate(3)[2]
        self.nodes[2].generate(4)

        self.join_network()

        self.sync_all()

        # gettransaction should work for txid1
        assert self.nodes[0].gettransaction(txid1)['txid'] == txid1, "gettransaction failed to find txid1"

        # listsinceblock(lastblockhash) should now include txid1, as seen from nodes[0]
        lsbres = self.nodes[0].listsinceblock(lastblockhash)
        assert any(tx['txid'] == txid1 for tx in lsbres['removed'])

        # but it should not include 'removed' if include_removed=false
        lsbres2 = self.nodes[0].listsinceblock(blockhash=lastblockhash, include_removed=False)
        assert 'removed' not in lsbres2

    def test_double_send(self):
        '''
        This tests the case where the same transaction is submitted twice on two
        separate blocks as part of a reorg. The former will vanish and the
        latter will appear as the true transaction (with confirmations dropping
        as a result).

             ab0
          /       \
        aa1 [tx1]   bb1
         |           |
        aa2         bb2
         |           |
        aa3         bb3 [tx1]
                     |
                    bb4

        Asserted:

        1. tx1 is listed in listsinceblock.
        2. It is included in 'removed' as it was removed, even though it is now
           present in a different block.
        3. It is listed with a confirmation count of 2 (bb3, bb4), not
           3 (aa1, aa2, aa3).
        '''

        self.sync_all()

        # Split network into two
        self.split_network()

        # create and sign a transaction
        utxos = self.nodes[2].listunspent()
        utxo = utxos[0]
        change = '%.8f' % (float(utxo['amount']) - 1.0003)
        recipient_dict = {
            self.nodes[0].getnewaddress(): 1,
            self.nodes[2].getnewaddress(): change,
        }
        utxo_dicts = [{
            'txid': utxo['txid'],
            'vout': utxo['vout'],
        }]
        signedtxres = self.nodes[2].signrawtransactionwithwallet(
            self.nodes[2].createrawtransaction(utxo_dicts, recipient_dict))
        assert signedtxres['complete']

        signedtx = signedtxres['hex']

        # send from nodes[1]; this will end up in aa1
        txid1 = self.nodes[1].sendrawtransaction(signedtx)

        # generate bb1-bb2 on right side
        self.nodes[2].generate(2)

        # send from nodes[2]; this will end up in bb3
        txid2 = self.nodes[2].sendrawtransaction(signedtx)

        assert_equal(txid1, txid2)

        # generate on both sides
        lastblockhash = self.nodes[1].generate(3)[2]
        self.nodes[2].generate(2)

        self.join_network()

        self.sync_all()

        # gettransaction should work for txid1
        self.nodes[0].gettransaction(txid1)

        # listsinceblock(lastblockhash) should now include txid1 in transactions
        # as well as in removed
        lsbres = self.nodes[0].listsinceblock(lastblockhash)
        assert any(tx['txid'] == txid1 for tx in lsbres['transactions'])
        assert any(tx['txid'] == txid1 for tx in lsbres['removed'])

        # find transaction and ensure confirmations is valid
        for tx in lsbres['transactions']:
            if tx['txid'] == txid1:
                assert_equal(tx['confirmations'], 2)

        # the same check for the removed array; confirmations should STILL be 2
        for tx in lsbres['removed']:
            if tx['txid'] == txid1:
                assert_equal(tx['confirmations'], 2)

if __name__ == '__main__':
    ListSinceBlockTest().main()
