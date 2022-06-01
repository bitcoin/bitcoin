#!/usr/bin/env python3
# Copyright (c) 2017-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listsinceblock RPC."""

from test_framework.address import key_to_p2wpkh
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.key import ECKey
from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import BIP125_SEQUENCE_NUMBER
from test_framework.util import (
    assert_array_result,
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import bytes_to_wif

from decimal import Decimal

class ListSinceBlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # All nodes are in IBD from genesis, so they'll need the miner (node2) to be an outbound connection, or have
        # only one connection. (See fPreferredDownload in net_processing)
        self.connect_nodes(1, 2)
        self.generate(self.nodes[2], COINBASE_MATURITY + 1)

        self.test_no_blockhash()
        self.test_invalid_blockhash()
        self.test_reorg()
        self.test_double_spend()
        self.test_double_send()
        self.double_spends_filtered()
        self.test_targetconfirmations()

    def test_no_blockhash(self):
        self.log.info("Test no blockhash")
        txid = self.nodes[2].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        self.sync_all()
        assert_array_result(self.nodes[0].listtransactions(), {"txid": txid}, {
            "category": "receive",
            "amount": 1,
            "confirmations": 0,
            "trusted": False,
        })

        blockhash, = self.generate(self.nodes[2], 1)
        blockheight = self.nodes[2].getblockheader(blockhash)['height']

        txs = self.nodes[0].listtransactions()
        assert_array_result(txs, {"txid": txid}, {
            "category": "receive",
            "amount": 1,
            "blockhash": blockhash,
            "blockheight": blockheight,
            "confirmations": 1,
        })
        assert_equal(len(txs), 1)
        assert "trusted" not in txs[0]

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
        self.log.info("Test invalid blockhash")
        assert_raises_rpc_error(-5, "Block not found", self.nodes[0].listsinceblock,
                                "42759cde25462784395a337460bde75f58e73d3f08bd31fdc3507cbac856a2c4")
        assert_raises_rpc_error(-5, "Block not found", self.nodes[0].listsinceblock,
                                "0000000000000000000000000000000000000000000000000000000000000000")
        assert_raises_rpc_error(-8, "blockhash must be of length 64 (not 11, for 'invalid-hex')", self.nodes[0].listsinceblock,
                                "invalid-hex")
        assert_raises_rpc_error(-8, "blockhash must be hexadecimal string (not 'Z000000000000000000000000000000000000000000000000000000000000000')", self.nodes[0].listsinceblock,
                                "Z000000000000000000000000000000000000000000000000000000000000000")

    def test_targetconfirmations(self):
        '''
        This tests when the value of target_confirmations exceeds the number of
        blocks in the main chain. In this case, the genesis block hash should be
        given for the `lastblock` property. If target_confirmations is < 1, then
        a -8 invalid parameter error is thrown.
        '''
        self.log.info("Test target_confirmations")
        blockhash, = self.generate(self.nodes[2], 1)
        blockheight = self.nodes[2].getblockheader(blockhash)['height']

        assert_equal(
            self.nodes[0].getblockhash(0),
            self.nodes[0].listsinceblock(blockhash, blockheight + 1)['lastblock'])
        assert_equal(
            self.nodes[0].getblockhash(0),
            self.nodes[0].listsinceblock(blockhash, blockheight + 1000)['lastblock'])
        assert_raises_rpc_error(-8, "Invalid parameter",
            self.nodes[0].listsinceblock, blockhash, 0)

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
        self.log.info("Test reorg")

        # Split network into two
        self.split_network()

        # send to nodes[0] from nodes[2]
        senttx = self.nodes[2].sendtoaddress(self.nodes[0].getnewaddress(), 1)

        # generate on both sides
        nodes1_last_blockhash = self.generate(self.nodes[1], 6, sync_fun=lambda: self.sync_all(self.nodes[:2]))[-1]
        nodes2_first_blockhash = self.generate(self.nodes[2], 7, sync_fun=lambda: self.sync_all(self.nodes[2:]))[0]
        self.log.debug("nodes[1] last blockhash = {}".format(nodes1_last_blockhash))
        self.log.debug("nodes[2] first blockhash = {}".format(nodes2_first_blockhash))

        self.join_network()

        # listsinceblock(nodes1_last_blockhash) should now include tx as seen from nodes[0]
        # and return the block height which listsinceblock now exposes since a5e7795.
        transactions = self.nodes[0].listsinceblock(nodes1_last_blockhash)['transactions']
        found = next(tx for tx in transactions if tx['txid'] == senttx)
        assert_equal(found['blockheight'], self.nodes[0].getblockheader(nodes2_first_blockhash)['height'])

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
        self.log.info("Test double spend")

        self.sync_all()

        # share utxo between nodes[1] and nodes[2]
        eckey = ECKey()
        eckey.generate()
        privkey = bytes_to_wif(eckey.get_bytes())
        address = key_to_p2wpkh(eckey.get_pubkey().get_bytes())
        self.nodes[2].sendtoaddress(address, 10)
        self.generate(self.nodes[2], 6)
        self.nodes[2].importprivkey(privkey)
        utxos = self.nodes[2].listunspent()
        utxo = [u for u in utxos if u["address"] == address][0]
        self.nodes[1].importprivkey(privkey)

        # Split network into two
        self.split_network()

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
        lastblockhash = self.generate(self.nodes[1], 3, sync_fun=self.no_op)[2]
        self.generate(self.nodes[2], 4, sync_fun=self.no_op)

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
        self.log.info("Test double send")

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
        self.generate(self.nodes[2], 2, sync_fun=self.no_op)

        # send from nodes[2]; this will end up in bb3
        txid2 = self.nodes[2].sendrawtransaction(signedtx)

        assert_equal(txid1, txid2)

        # generate on both sides
        lastblockhash = self.generate(self.nodes[1], 3, sync_fun=self.no_op)[2]
        self.generate(self.nodes[2], 2, sync_fun=self.no_op)

        self.join_network()

        self.sync_all()

        # gettransaction should work for txid1
        tx1 = self.nodes[0].gettransaction(txid1)
        assert_equal(tx1['blockheight'], self.nodes[0].getblockheader(tx1['blockhash'])['height'])

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

    def double_spends_filtered(self):
        '''
        `listsinceblock` was returning conflicted transactions even if they
        occurred before the specified cutoff blockhash
        '''
        self.log.info("Test spends filtered")
        spending_node = self.nodes[2]
        dest_address = spending_node.getnewaddress()

        tx_input = dict(
            sequence=BIP125_SEQUENCE_NUMBER, **next(u for u in spending_node.listunspent()))
        rawtx = spending_node.createrawtransaction(
            [tx_input], {dest_address: tx_input["amount"] - Decimal("0.00051000"),
                         spending_node.getrawchangeaddress(): Decimal("0.00050000")})
        signedtx = spending_node.signrawtransactionwithwallet(rawtx)
        orig_tx_id = spending_node.sendrawtransaction(signedtx["hex"])
        original_tx = spending_node.gettransaction(orig_tx_id)

        double_tx = spending_node.bumpfee(orig_tx_id)

        # check that both transactions exist
        block_hash = spending_node.listsinceblock(
            spending_node.getblockhash(spending_node.getblockcount()))
        original_found = False
        double_found = False
        for tx in block_hash['transactions']:
            if tx['txid'] == original_tx['txid']:
                original_found = True
            if tx['txid'] == double_tx['txid']:
                double_found = True
        assert_equal(original_found, True)
        assert_equal(double_found, True)

        lastblockhash = self.generate(spending_node, 1)[0]

        # check that neither transaction exists
        block_hash = spending_node.listsinceblock(lastblockhash)
        original_found = False
        double_found = False
        for tx in block_hash['transactions']:
            if tx['txid'] == original_tx['txid']:
                original_found = True
            if tx['txid'] == double_tx['txid']:
                double_found = True
        assert_equal(original_found, False)
        assert_equal(double_found, False)

if __name__ == '__main__':
    ListSinceBlockTest().main()
