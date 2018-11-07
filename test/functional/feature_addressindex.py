#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test addressindex generation and fetching
#

import binascii

from test_framework.messages import COIN, COutPoint, CTransaction, CTxIn, CTxOut
from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch
from test_framework.script import CScript, OP_CHECKSIG, OP_DUP, OP_EQUAL, OP_EQUALVERIFY, OP_HASH160
from test_framework.util import assert_equal, connect_nodes

class AddressIndexTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        # Nodes 0/1 are "wallet" nodes
        self.start_node(0, [])
        self.start_node(1, ["-addressindex"])
        # Nodes 2/3 are used for testing
        self.start_node(2, ["-addressindex"])
        self.start_node(3, ["-addressindex"])
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[0], 3)

        self.sync_all()

    def run_test(self):
        self.log.info("Test that settings can't be changed without -reindex...")
        self.stop_node(1)
        self.nodes[1].assert_start_raises_init_error(["-addressindex=0"], "You need to rebuild the database using -reindex to change -addressindex", match=ErrorMatch.PARTIAL_REGEX)
        self.start_node(1, ["-addressindex=0", "-reindex"])
        connect_nodes(self.nodes[0], 1)
        self.sync_all()
        self.stop_node(1)
        self.nodes[1].assert_start_raises_init_error(["-addressindex"], "You need to rebuild the database using -reindex to change -addressindex", match=ErrorMatch.PARTIAL_REGEX)
        self.start_node(1, ["-addressindex", "-reindex"])
        connect_nodes(self.nodes[0], 1)
        self.sync_all()

        self.log.info("Mining blocks...")
        mining_address = self.nodes[0].getnewaddress()
        self.nodes[0].generatetoaddress(105, mining_address)
        self.sync_all()

        chain_height = self.nodes[1].getblockcount()
        assert_equal(chain_height, 105)
        assert_equal(self.nodes[1].getbalance(), 0)
        assert_equal(self.nodes[2].getbalance(), 0)

        # Check that balances are correct
        balance0 = self.nodes[1].getaddressbalance("93bVhahvUKmQu8gu9g3QnPPa2cxFK98pMB")
        balance_mining = self.nodes[1].getaddressbalance(mining_address)
        assert_equal(balance0["balance"], 0)
        assert_equal(balance_mining["balance"], 105 * 500 * COIN)
        assert_equal(balance_mining["balance_immature"], 100 * 500 * COIN)
        assert_equal(balance_mining["balance_spendable"], 5 * 500 * COIN)

        # Check p2pkh and p2sh address indexes
        self.log.info("Testing p2pkh and p2sh address index...")

        txid0 = self.nodes[0].sendtoaddress("yMNJePdcKvXtWWQnFYHNeJ5u8TF2v1dfK4", 10)
        self.nodes[0].generate(1)

        txidb0 = self.nodes[0].sendtoaddress("93bVhahvUKmQu8gu9g3QnPPa2cxFK98pMB", 10)
        self.nodes[0].generate(1)

        txid1 = self.nodes[0].sendtoaddress("yMNJePdcKvXtWWQnFYHNeJ5u8TF2v1dfK4", 15)
        self.nodes[0].generate(1)

        txidb1 = self.nodes[0].sendtoaddress("93bVhahvUKmQu8gu9g3QnPPa2cxFK98pMB", 15)
        self.nodes[0].generate(1)

        txid2 = self.nodes[0].sendtoaddress("yMNJePdcKvXtWWQnFYHNeJ5u8TF2v1dfK4", 20)
        self.nodes[0].generate(1)

        txidb2 = self.nodes[0].sendtoaddress("93bVhahvUKmQu8gu9g3QnPPa2cxFK98pMB", 20)
        self.nodes[0].generate(1)

        self.sync_all()

        txids = self.nodes[1].getaddresstxids("yMNJePdcKvXtWWQnFYHNeJ5u8TF2v1dfK4")
        assert_equal(len(txids), 3)
        assert_equal(txids[0], txid0)
        assert_equal(txids[1], txid1)
        assert_equal(txids[2], txid2)

        txidsb = self.nodes[1].getaddresstxids("93bVhahvUKmQu8gu9g3QnPPa2cxFK98pMB")
        assert_equal(len(txidsb), 3)
        assert_equal(txidsb[0], txidb0)
        assert_equal(txidsb[1], txidb1)
        assert_equal(txidsb[2], txidb2)

        # Check that limiting by height works
        self.log.info("Testing querying txids by range of block heights..")
        height_txids = self.nodes[1].getaddresstxids({
            "addresses": ["93bVhahvUKmQu8gu9g3QnPPa2cxFK98pMB"],
            "start": 105,
            "end": 110
        })
        assert_equal(len(height_txids), 2)
        assert_equal(height_txids[0], txidb0)
        assert_equal(height_txids[1], txidb1)

        # Check that multiple addresses works
        multitxids = self.nodes[1].getaddresstxids({"addresses": ["93bVhahvUKmQu8gu9g3QnPPa2cxFK98pMB", "yMNJePdcKvXtWWQnFYHNeJ5u8TF2v1dfK4"]})
        assert_equal(len(multitxids), 6)
        assert_equal(multitxids[0], txid0)
        assert_equal(multitxids[1], txidb0)
        assert_equal(multitxids[2], txid1)
        assert_equal(multitxids[3], txidb1)
        assert_equal(multitxids[4], txid2)
        assert_equal(multitxids[5], txidb2)

        # Check that balances are correct
        balance0 = self.nodes[1].getaddressbalance("93bVhahvUKmQu8gu9g3QnPPa2cxFK98pMB")
        assert_equal(balance0["balance"], 45 * 100000000)

        # Check that outputs with the same address will only return one txid
        self.log.info("Testing for txid uniqueness...")
        addressHash = binascii.unhexlify("FE30B718DCF0BF8A2A686BF1820C073F8B2C3B37")
        scriptPubKey = CScript([OP_HASH160, addressHash, OP_EQUAL])
        unspent = self.nodes[0].listunspent()
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(unspent[0]["txid"], 16), unspent[0]["vout"]))]
        tx.vout = [CTxOut(10, scriptPubKey), CTxOut(11, scriptPubKey)]
        tx.rehash()

        signed_tx = self.nodes[0].signrawtransactionwithwallet(binascii.hexlify(tx.serialize()).decode("utf-8"))
        sent_txid = self.nodes[0].sendrawtransaction(signed_tx["hex"], True)

        self.nodes[0].generate(1)
        self.sync_all()

        txidsmany = self.nodes[1].getaddresstxids("93bVhahvUKmQu8gu9g3QnPPa2cxFK98pMB")
        assert_equal(len(txidsmany), 4)
        assert_equal(txidsmany[3], sent_txid)

        # Check that balances are correct
        self.log.info("Testing balances...")
        balance0 = self.nodes[1].getaddressbalance("93bVhahvUKmQu8gu9g3QnPPa2cxFK98pMB")
        assert_equal(balance0["balance"], 45 * 100000000 + 21)

        # Check that balances are correct after spending
        self.log.info("Testing balances after spending...")
        privkey2 = "cU4zhap7nPJAWeMFu4j6jLrfPmqakDAzy8zn8Fhb3oEevdm4e5Lc"
        address2 = "yeMpGzMj3rhtnz48XsfpB8itPHhHtgxLc3"
        addressHash2 = binascii.unhexlify("C5E4FB9171C22409809A3E8047A29C83886E325D")
        scriptPubKey2 = CScript([OP_DUP, OP_HASH160, addressHash2, OP_EQUALVERIFY, OP_CHECKSIG])
        self.nodes[0].importprivkey(privkey2)

        unspent = self.nodes[0].listunspent()
        tx = CTransaction()
        tx_fee_sat = 1000
        tx.vin = [CTxIn(COutPoint(int(unspent[0]["txid"], 16), unspent[0]["vout"]))]
        amount = int(unspent[0]["amount"] * 100000000) - tx_fee_sat
        tx.vout = [CTxOut(amount, scriptPubKey2)]
        tx.rehash()
        signed_tx = self.nodes[0].signrawtransactionwithwallet(binascii.hexlify(tx.serialize()).decode("utf-8"))
        spending_txid = self.nodes[0].sendrawtransaction(signed_tx["hex"], True)
        self.nodes[0].generate(1)
        self.sync_all()
        balance1 = self.nodes[1].getaddressbalance(address2)
        assert_equal(balance1["balance"], amount)

        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(spending_txid, 16), 0))]
        send_amount = 1 * 100000000 + 12840
        change_amount = amount - send_amount - 10000
        tx.vout = [CTxOut(change_amount, scriptPubKey2), CTxOut(send_amount, scriptPubKey)]
        tx.rehash()

        signed_tx = self.nodes[0].signrawtransactionwithwallet(binascii.hexlify(tx.serialize()).decode("utf-8"))
        sent_txid = self.nodes[0].sendrawtransaction(signed_tx["hex"], True)
        self.nodes[0].generate(1)
        self.sync_all()

        balance2 = self.nodes[1].getaddressbalance(address2)
        assert_equal(balance2["balance"], change_amount)

        # Check that deltas are returned correctly
        deltas = self.nodes[1].getaddressdeltas({"addresses": [address2], "start": 0, "end": 200})
        balance3 = 0
        for delta in deltas:
            balance3 += delta["satoshis"]
        assert_equal(balance3, change_amount)
        assert_equal(deltas[0]["address"], address2)
        assert_equal(deltas[0]["blockindex"], 1)

        # Check that entire range will be queried
        deltasAll = self.nodes[1].getaddressdeltas({"addresses": [address2]})
        assert_equal(len(deltasAll), len(deltas))

        # Check that deltas can be returned from range of block heights
        deltas = self.nodes[1].getaddressdeltas({"addresses": [address2], "start": 113, "end": 113})
        assert_equal(len(deltas), 1)

        # Check that unspent outputs can be queried
        self.log.info("Testing utxos...")
        utxos = self.nodes[1].getaddressutxos({"addresses": [address2]})
        assert_equal(len(utxos), 1)
        assert_equal(utxos[0]["satoshis"], change_amount)

        # Check that indexes will be updated with a reorg
        self.log.info("Testing reorg...")

        best_hash = self.nodes[0].getbestblockhash()
        self.nodes[0].invalidateblock(best_hash)
        self.nodes[1].invalidateblock(best_hash)
        self.nodes[2].invalidateblock(best_hash)
        self.nodes[3].invalidateblock(best_hash)
        # Allow some time for the reorg to start
        self.bump_mocktime(2)
        self.sync_all()

        balance4 = self.nodes[1].getaddressbalance(address2)
        assert_equal(balance4, balance1)

        utxos2 = self.nodes[1].getaddressutxos({"addresses": [address2]})
        assert_equal(len(utxos2), 1)
        assert_equal(utxos2[0]["satoshis"], amount)

        # Check sorting of utxos
        self.nodes[2].generate(150)

        self.nodes[2].sendtoaddress(address2, 50)
        self.nodes[2].generate(1)
        self.nodes[2].sendtoaddress(address2, 50)
        self.nodes[2].generate(1)
        self.sync_all()

        utxos3 = self.nodes[1].getaddressutxos({"addresses": [address2]})
        assert_equal(len(utxos3), 3)
        assert_equal(utxos3[0]["height"], 114)
        assert_equal(utxos3[1]["height"], 264)
        assert_equal(utxos3[2]["height"], 265)

        # Check mempool indexing
        self.log.info("Testing mempool indexing...")

        privKey3 = "cRyrMvvqi1dmpiCmjmmATqjAwo6Wu7QTjKu1ABMYW5aFG4VXW99K"
        address3 = "yWB15aAdpeKuSaQHFVJpBDPbNSLZJSnDLA"
        addressHash3 = binascii.unhexlify("6C186B3A308A77C779A9BB71C3B5A7EC28232A13")
        scriptPubKey3 = CScript([OP_DUP, OP_HASH160, addressHash3, OP_EQUALVERIFY, OP_CHECKSIG])
        # address4 = "2N8oFVB2vThAKury4vnLquW2zVjsYjjAkYQ"
        scriptPubKey4 = CScript([OP_HASH160, addressHash3, OP_EQUAL])
        unspent = self.nodes[2].listunspent()

        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(unspent[0]["txid"], 16), unspent[0]["vout"]))]
        amount = int(unspent[0]["amount"] * 100000000) - tx_fee_sat
        tx.vout = [CTxOut(amount, scriptPubKey3)]
        tx.rehash()
        signed_tx = self.nodes[2].signrawtransactionwithwallet(binascii.hexlify(tx.serialize()).decode("utf-8"))
        memtxid1 = self.nodes[2].sendrawtransaction(signed_tx["hex"], True)
        self.bump_mocktime(2)

        tx2 = CTransaction()
        tx2.vin = [CTxIn(COutPoint(int(unspent[1]["txid"], 16), unspent[1]["vout"]))]
        amount = int(unspent[1]["amount"] * 100000000) - tx_fee_sat
        tx2.vout = [
            CTxOut(int(amount / 4), scriptPubKey3),
            CTxOut(int(amount / 4), scriptPubKey3),
            CTxOut(int(amount / 4), scriptPubKey4),
            CTxOut(int(amount / 4), scriptPubKey4)
        ]
        tx2.rehash()
        signed_tx2 = self.nodes[2].signrawtransactionwithwallet(binascii.hexlify(tx2.serialize()).decode("utf-8"))
        memtxid2 = self.nodes[2].sendrawtransaction(signed_tx2["hex"], True)
        self.bump_mocktime(2)

        mempool = self.nodes[2].getaddressmempool({"addresses": [address3]})
        assert_equal(len(mempool), 3)
        assert_equal(mempool[0]["txid"], memtxid1)
        assert_equal(mempool[0]["address"], address3)
        assert_equal(mempool[0]["index"], 0)
        assert_equal(mempool[1]["txid"], memtxid2)
        assert_equal(mempool[1]["index"], 0)
        assert_equal(mempool[2]["txid"], memtxid2)
        assert_equal(mempool[2]["index"], 1)

        self.nodes[2].generate(1)
        self.sync_all()
        mempool2 = self.nodes[2].getaddressmempool({"addresses": [address3]})
        assert_equal(len(mempool2), 0)

        tx = CTransaction()
        tx.vin = [
            CTxIn(COutPoint(int(memtxid2, 16), 0)),
            CTxIn(COutPoint(int(memtxid2, 16), 1))
        ]
        tx.vout = [CTxOut(int(amount / 2 - 10000), scriptPubKey2)]
        tx.rehash()
        self.nodes[2].importprivkey(privKey3)
        signed_tx3 = self.nodes[2].signrawtransactionwithwallet(binascii.hexlify(tx.serialize()).decode("utf-8"))
        self.nodes[2].sendrawtransaction(signed_tx3["hex"], True)
        self.bump_mocktime(2)

        mempool3 = self.nodes[2].getaddressmempool({"addresses": [address3]})
        assert_equal(len(mempool3), 2)
        assert_equal(mempool3[0]["prevtxid"], memtxid2)
        assert_equal(mempool3[0]["prevout"], 0)
        assert_equal(mempool3[1]["prevtxid"], memtxid2)
        assert_equal(mempool3[1]["prevout"], 1)

        # sending and receiving to the same address
        privkey1 = "cMvZn1pVWntTEcsK36ZteGQXRAcZ8CoTbMXF1QasxBLdnTwyVQCc"
        address1 = "yM9Eed1bxjy7tYxD3yZDHxjcVT48WdRoB1"
        address1hash = binascii.unhexlify("0909C84A817651502E020AAD0FBCAE5F656E7D8A")
        address1script = CScript([OP_DUP, OP_HASH160, address1hash, OP_EQUALVERIFY, OP_CHECKSIG])

        self.nodes[0].sendtoaddress(address1, 10)
        self.nodes[0].generate(1)
        self.sync_all()

        utxos = self.nodes[1].getaddressutxos({"addresses": [address1]})
        assert_equal(len(utxos), 1)

        tx = CTransaction()
        tx.vin = [
            CTxIn(COutPoint(int(utxos[0]["txid"], 16), utxos[0]["outputIndex"]))
        ]
        amount = int(utxos[0]["satoshis"] - 10000)
        tx.vout = [CTxOut(amount, address1script)]
        tx.rehash()
        self.nodes[0].importprivkey(privkey1)
        signed_tx = self.nodes[0].signrawtransactionwithwallet(binascii.hexlify(tx.serialize()).decode("utf-8"))
        self.nodes[0].sendrawtransaction(signed_tx["hex"], True)

        self.sync_all()
        mempool_deltas = self.nodes[2].getaddressmempool({"addresses": [address1]})
        assert_equal(len(mempool_deltas), 2)

        self.log.info("Passed")


if __name__ == '__main__':
    AddressIndexTest().main()
