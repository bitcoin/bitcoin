#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test addressindex generation and fetching
#

from test_framework.test_particl import ParticlTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *

class AddressIndexTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.extra_args = [
            # Nodes 0/1 are "wallet" nodes
            ['-debug','-relaypriority=0'],
            ['-debug','-addressindex'],
            # Nodes 2/3 are used for testing
            ['-debug','-addressindex','-relaypriority=0'],
            ['-debug','-addressindex'],]

    def setup_network(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[0], 3)

        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        nodes = self.nodes

        # Stop staking
        for i in range(len(nodes)):
            nodes[i].reservebalance(True, 10000000)

        ro = nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        assert(nodes[0].getwalletinfo()['total_balance'] == 100000)

        nodes[1].extkeyimportmaster('graine article givre hublot encadrer admirer stipuler capsule acajou paisible soutirer organe')
        nodes[2].extkeyimportmaster('sección grito médula hecho pauta posada nueve ebrio bruto buceo baúl mitad')
        nodes[3].extkeyimportmaster('けっこん　ゆそう　へいねつ　しあわせ　ちまた　きつね　たんたい　むかし　たかい　のいず　こわもて　けんこう')

        addrs = []
        addrs.append(nodes[1].getnewaddress())
        addrs.append(nodes[1].getnewaddress())
        addrs.append(nodes[1].getnewaddress())

        ms1 = nodes[1].addmultisigaddress(2, addrs)['address']
        assert(ms1 == 'r8L81gLiWg46j5EGfZSp2JHmA9hBgLbHuf') # rFHaEuXkYpNUYpMMY3kMkDdayQxpc7ozti

        addr1 = nodes[2].getnewaddress()
        assert(addr1 == 'pqZDE7YNWv5PJWidiaEG8tqfebkd6PNZDV') # pcX1WHotKuQwFypDf1ZkJrh81J1DS7DfXd
        addr2 = nodes[3].getnewaddress()
        assert(addr2 == 'pqavEUgLCZeGh8o9sTcCfYVAsrTgnQTUsK')


        self.sync_all()
        chain_height = self.nodes[1].getblockcount()
        assert_equal(chain_height, 0)

        assert_equal(self.nodes[1].getbalance(), 0)
        assert_equal(self.nodes[2].getbalance(), 0)

        balance0 = self.nodes[1].getaddressbalance("r8L81gLiWg46j5EGfZSp2JHmA9hBgLbHuf")
        assert_equal(balance0["balance"], 0)



        # Check p2pkh and p2sh address indexes
        print("Testing p2pkh and p2sh address index...")

        txid0 = self.nodes[0].sendtoaddress("pqZDE7YNWv5PJWidiaEG8tqfebkd6PNZDV", 10)
        self.stakeToHeight(1, fSync=False)
        txidb0 = self.nodes[0].sendtoaddress("r8L81gLiWg46j5EGfZSp2JHmA9hBgLbHuf", 10)
        self.stakeToHeight(2, fSync=False)
        txid1 = self.nodes[0].sendtoaddress("pqZDE7YNWv5PJWidiaEG8tqfebkd6PNZDV", 15)
        self.stakeToHeight(3, fSync=False)
        txidb1 = self.nodes[0].sendtoaddress("r8L81gLiWg46j5EGfZSp2JHmA9hBgLbHuf", 15)
        self.stakeToHeight(4, fSync=False)
        txid2 = self.nodes[0].sendtoaddress("pqZDE7YNWv5PJWidiaEG8tqfebkd6PNZDV", 20)
        self.stakeToHeight(5, fSync=False)
        txidb2 = self.nodes[0].sendtoaddress("r8L81gLiWg46j5EGfZSp2JHmA9hBgLbHuf", 20)
        self.stakeToHeight(6)

        txids = self.nodes[1].getaddresstxids("pqZDE7YNWv5PJWidiaEG8tqfebkd6PNZDV")
        assert_equal(len(txids), 3)
        assert_equal(txids[0], txid0)
        assert_equal(txids[1], txid1)
        assert_equal(txids[2], txid2)

        txidsb = self.nodes[1].getaddresstxids("r8L81gLiWg46j5EGfZSp2JHmA9hBgLbHuf")
        assert_equal(len(txidsb), 3)
        assert_equal(txidsb[0], txidb0)
        assert_equal(txidsb[1], txidb1)
        assert_equal(txidsb[2], txidb2)


        # Check that limiting by height works
        print("Testing querying txids by range of block heights..")
        # Note start and end parameters must be > 0 to apply
        height_txids = self.nodes[1].getaddresstxids({
            "addresses": ["r8L81gLiWg46j5EGfZSp2JHmA9hBgLbHuf"],
            "start": 3,
            "end": 4
        })
        assert_equal(len(height_txids), 1)
        #assert_equal(height_txids[0], txidb0)
        assert_equal(height_txids[0], txidb1)

        # Check that multiple addresses works
        multitxids = self.nodes[1].getaddresstxids({"addresses": ["r8L81gLiWg46j5EGfZSp2JHmA9hBgLbHuf", "pqZDE7YNWv5PJWidiaEG8tqfebkd6PNZDV"]})
        assert_equal(len(multitxids), 6)
        assert_equal(multitxids[0], txid0)
        assert_equal(multitxids[1], txidb0)
        assert_equal(multitxids[2], txid1)
        assert_equal(multitxids[3], txidb1)
        assert_equal(multitxids[4], txid2)
        assert_equal(multitxids[5], txidb2)

        # Check that balances are correct
        balance0 = self.nodes[1].getaddressbalance("r8L81gLiWg46j5EGfZSp2JHmA9hBgLbHuf")
        assert_equal(balance0["balance"], 45 * 100000000)


        # Check that outputs with the same address will only return one txid
        print("Testing for txid uniqueness...")

        inputs = []
        outputs = {'pqZDE7YNWv5PJWidiaEG8tqfebkd6PNZDV':1,'pqavEUgLCZeGh8o9sTcCfYVAsrTgnQTUsK':1}
        tx = self.nodes[0].createrawtransaction(inputs,outputs)

        # modified outputs to go to the same address
        #tx = 'a0000000000000020100e1f505000000001976a914e2c470d8005a3d40c6d79eb1907c479c24173ede88ac0100e1f505000000001976a914e2c470d8005a3d40c6d79eb1907c479c24173ede88ac'
        tx = 'a0000000000000020100e1f505000000001976a914e317164ad324e5ec2f8b5de080f0cb614042982d88ac0100e1f505000000001976a914e317164ad324e5ec2f8b5de080f0cb614042982d88ac'

        txfunded = self.nodes[0].fundrawtransaction(tx)

        #ro = self.nodes[0].decoderawtransaction(txfunded['hex'])
        #print(json.dumps(ro, indent=4, default=self.jsonDecimal))

        txsigned = self.nodes[0].signrawtransaction(txfunded['hex'])

        sent_txid = self.nodes[0].sendrawtransaction(txsigned['hex'], True)

        self.stakeBlocks(1)

        txidsmany = self.nodes[1].getaddresstxids("pqavEUgLCZeGh8o9sTcCfYVAsrTgnQTUsK")
        assert_equal(len(txidsmany), 1)
        assert_equal(txidsmany[0], sent_txid)


        # Check that balances are correct
        print("Testing balances...")
        balance0 = self.nodes[1].getaddressbalance("pqavEUgLCZeGh8o9sTcCfYVAsrTgnQTUsK")
        assert_equal(balance0["balance"], 2 * 100000000)

        unspent2 = self.nodes[2].listunspent()



        balance0 = self.nodes[1].getaddressbalance("pqZDE7YNWv5PJWidiaEG8tqfebkd6PNZDV")
        assert_equal(balance0["balance"], 45 * 100000000)


        inputs = []
        outputs = {'pqavEUgLCZeGh8o9sTcCfYVAsrTgnQTUsK':1}
        tx = self.nodes[2].createrawtransaction(inputs,outputs)
        txfunded = self.nodes[2].fundrawtransaction(tx)

        txsigned = self.nodes[2].signrawtransaction(txfunded['hex'])
        sent_txid = self.nodes[2].sendrawtransaction(txsigned['hex'], True)
        print("sent_txid", sent_txid)
        ro = self.nodes[0].decoderawtransaction(txsigned['hex'])

        self.sync_all()
        self.stakeBlocks(1)

        txidsmany = self.nodes[1].getaddresstxids("pqavEUgLCZeGh8o9sTcCfYVAsrTgnQTUsK")
        assert_equal(len(txidsmany), 2)
        assert_equal(txidsmany[1], sent_txid)


        balance0 = self.nodes[1].getaddressbalance('pqZDE7YNWv5PJWidiaEG8tqfebkd6PNZDV')
        assert(balance0["balance"] < 45 * 100000000)


         # Check that deltas are returned correctly
        deltas = self.nodes[1].getaddressdeltas({"addresses": ['pqavEUgLCZeGh8o9sTcCfYVAsrTgnQTUsK'], "start": 1, "end": 200})
        balance3 = 0
        for delta in deltas:
            balance3 += delta["satoshis"]
        assert_equal(balance3, 300000000)
        assert_equal(deltas[0]["address"], 'pqavEUgLCZeGh8o9sTcCfYVAsrTgnQTUsK')
        #assert_equal(deltas[0]["blockindex"], 1)


        address2 = 'pqZDE7YNWv5PJWidiaEG8tqfebkd6PNZDV'
        # Check that entire range will be queried
        deltasAll = self.nodes[1].getaddressdeltas({"addresses": [address2]})
        assert_equal(len(deltasAll), 4)

        # Check that deltas can be returned from range of block heights
        deltas = self.nodes[1].getaddressdeltas({"addresses": [address2], "start": 3, "end": 3})
        assert_equal(len(deltas), 1)

        # Check that unspent outputs can be queried
        print("Testing utxos...")
        utxos = self.nodes[1].getaddressutxos({"addresses": [address2]})
        assert_equal(len(utxos), 2)
        assert_equal(utxos[0]["satoshis"], 1500000000)



        # Check that indexes will be updated with a reorg
        print("Testing reorg...")
        height_before = self.nodes[1].getblockcount()
        best_hash = self.nodes[0].getbestblockhash()
        self.nodes[0].invalidateblock(best_hash)
        self.nodes[1].invalidateblock(best_hash)
        self.nodes[2].invalidateblock(best_hash)
        self.nodes[3].invalidateblock(best_hash)
        self.sync_all()
        assert(self.nodes[1].getblockcount() == height_before - 1)


        balance4 = self.nodes[1].getaddressbalance(address2)
        assert_equal(balance4['balance'], 4500000000)

        utxos2 = self.nodes[1].getaddressutxos({"addresses": [address2]})
        assert_equal(len(utxos2), 3)
        assert_equal(utxos2[0]["satoshis"], 1000000000)


        # Check sorting of utxos
        self.stakeBlocks(1)

        txidsort1 = self.nodes[0].sendtoaddress(address2, 50)
        self.stakeBlocks(1)
        txidsort2 = self.nodes[0].sendtoaddress(address2, 50)
        self.stakeBlocks(1)

        utxos3 = self.nodes[1].getaddressutxos({"addresses": [address2]})
        assert_equal(len(utxos3), 4)
        assert_equal(utxos3[0]["height"], 3)
        assert_equal(utxos3[1]["height"], 5)
        assert_equal(utxos3[2]["height"], 9)
        assert_equal(utxos3[3]["height"], 10)

        # Check mempool indexing
        print("Testing mempool indexing...")

        address3 = nodes[3].getnewaddress()


        txidsort1 = self.nodes[2].sendtoaddress(address3, 1)
        txidsort2 = self.nodes[2].sendtoaddress(address3, 1)
        txidsort3 = self.nodes[2].sendtoaddress(address3, 1)

        mempool = self.nodes[2].getaddressmempool({"addresses": [address3]})
        assert_equal(len(mempool), 3)


        addr256 = nodes[3].getnewaddress("", "false", "false", "true")

        txid = self.nodes[3].sendtoaddress(addr256, 2.56)
        mempool = self.nodes[3].getaddressmempool({"addresses": [addr256]})
        print(json.dumps(mempool, indent=4, default=self.jsonDecimal))
        assert_equal(len(mempool), 1)

        self.sync_all()
        self.stakeBlocks(1)

        ro = self.nodes[3].getaddresstxids(addr256)
        assert_equal(len(ro), 1)

        utxos = self.nodes[3].getaddressutxos({"addresses": [addr256]})
        assert_equal(len(utxos), 1)

        mempool = self.nodes[3].getaddressmempool({"addresses": [addr256]})
        assert_equal(len(mempool), 0)



        """

        # Check sorting of utxos
        self.nodes[2].generate(150)

        txidsort1 = self.nodes[2].sendtoaddress(address2, 50)
        self.nodes[2].generate(1)
        txidsort2 = self.nodes[2].sendtoaddress(address2, 50)
        self.nodes[2].generate(1)
        self.sync_all()

        utxos3 = self.nodes[1].getaddressutxos({"addresses": [address2]})
        assert_equal(len(utxos3), 3)
        assert_equal(utxos3[0]["height"], 114)
        assert_equal(utxos3[1]["height"], 264)
        assert_equal(utxos3[2]["height"], 265)

        # Check mempool indexing
        print("Testing mempool indexing...")

        privKey3 = "7wknRvW5NvcK9NM6vLoK2dZZMqUDD5CX1sFQfG1hgs8YLHniKYdm"
        address3 = "pkSCghnT5d1z846K24Mz5iodQPmyFyCWee"
        addressHash3 = bytes([170,152,114,181,187,205,181,17,216,158,14,17,170,39,218,115,253,44,63,80])
        scriptPubKey3 = CScript([OP_DUP, OP_HASH160, addressHash3, OP_EQUALVERIFY, OP_CHECKSIG])
        address4 = "rMncd8ybvLsVPnef7jhG2DtmvQok3EGTKw"
        scriptPubKey4 = CScript([OP_HASH160, addressHash3, OP_EQUAL])
        unspent = self.nodes[2].listunspent()

        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(unspent[0]["txid"], 16), unspent[0]["vout"]))]
        amount = int(unspent[0]["amount"] * 100000000 - 100000)
        tx.vout = [CTxOut(amount, scriptPubKey3)]
        tx.rehash()
        signed_tx = self.nodes[2].signrawtransaction(binascii.hexlify(tx.serialize()).decode("utf-8"))
        memtxid1 = self.nodes[2].sendrawtransaction(signed_tx["hex"], True)
        time.sleep(2)

        tx2 = CTransaction()
        tx2.vin = [CTxIn(COutPoint(int(unspent[1]["txid"], 16), unspent[1]["vout"]))]
        amount = int(unspent[1]["amount"] * 100000000 - 100000)
        tx2.vout = [
            CTxOut(int(amount / 4), scriptPubKey3),
            CTxOut(int(amount / 4), scriptPubKey3),
            CTxOut(int(amount / 4), scriptPubKey4),
            CTxOut(int(amount / 4), scriptPubKey4)
        ]
        tx2.rehash()
        signed_tx2 = self.nodes[2].signrawtransaction(binascii.hexlify(tx2.serialize()).decode("utf-8"))
        memtxid2 = self.nodes[2].sendrawtransaction(signed_tx2["hex"], True)
        time.sleep(2)

        mempool = self.nodes[2].getaddressmempool({"addresses": [address3]})
        assert_equal(len(mempool), 3)
        assert_equal(mempool[0]["txid"], memtxid1)
        assert_equal(mempool[0]["address"], address3)
        assert_equal(mempool[0]["index"], 0)
        assert_equal(mempool[1]["txid"], memtxid2)
        assert_equal(mempool[1]["index"], 0)
        assert_equal(mempool[2]["txid"], memtxid2)
        assert_equal(mempool[2]["index"], 1)

        blk_hashes = self.nodes[2].generate(1);
        self.sync_all();
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
        signed_tx3 = self.nodes[2].signrawtransaction(binascii.hexlify(tx.serialize()).decode("utf-8"))
        memtxid3 = self.nodes[2].sendrawtransaction(signed_tx3["hex"], True)
        time.sleep(2)

        mempool3 = self.nodes[2].getaddressmempool({"addresses": [address3]})
        assert_equal(len(mempool3), 2)
        assert_equal(mempool3[0]["prevtxid"], memtxid2)
        assert_equal(mempool3[0]["prevout"], 0)
        assert_equal(mempool3[1]["prevtxid"], memtxid2)
        assert_equal(mempool3[1]["prevout"], 1)


        # sending and receiving to the same address
        privkey1 = "7rdLWvaivKefhMy7Q7hpAQytkry3zND88nrwwPrUkf2h8w2yY5ww"
        address1 = "pnXhQCNov8ezRwL85zX5VHmiCLyexYC5yf"
        address1hash = bytes([193,146,191,247,81,175,142,254,193,81,53,212,43,254,237,249,26,111,62,52])
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
        amount = int(utxos[0]["satoshis"] - 1000)
        tx.vout = [CTxOut(amount, address1script)]
        tx.rehash()
        self.nodes[0].importprivkey(privkey1)
        signed_tx = self.nodes[0].signrawtransaction(binascii.hexlify(tx.serialize()).decode("utf-8"))
        mem_txid = self.nodes[0].sendrawtransaction(signed_tx["hex"], True)

        self.sync_all()
        mempool_deltas = self.nodes[2].getaddressmempool({"addresses": [address1]})
        assert_equal(len(mempool_deltas), 2)

        # Include chaininfo in results
        print("Testing results with chain info...")

        deltas_with_info = self.nodes[1].getaddressdeltas({
            "addresses": [address2],
            "start": 1,
            "end": 200,
            "chainInfo": True
        })
        start_block_hash = self.nodes[1].getblockhash(1);
        end_block_hash = self.nodes[1].getblockhash(200);
        assert_equal(deltas_with_info["start"]["height"], 1)
        assert_equal(deltas_with_info["start"]["hash"], start_block_hash)
        assert_equal(deltas_with_info["end"]["height"], 200)
        assert_equal(deltas_with_info["end"]["hash"], end_block_hash)

        utxos_with_info = self.nodes[1].getaddressutxos({"addresses": [address2], "chainInfo": True})
        expected_tip_block_hash = self.nodes[1].getblockhash(267);
        assert_equal(utxos_with_info["height"], 267)
        assert_equal(utxos_with_info["hash"], expected_tip_block_hash)
        """

        print("Passed\n")


if __name__ == '__main__':
    AddressIndexTest().main()
