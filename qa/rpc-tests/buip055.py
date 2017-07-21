#!/usr/bin/env python3
# Copyright (c) 2015-2017 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import time
import shutil
import random
from binascii import hexlify
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.util import *
from test_framework.script import *
from test_framework.blocktools import *
import test_framework.script as script
import pdb
import sys
if sys.version_info[0] < 3:
    raise "Use Python 3"
import logging
logging.basicConfig(format='%(asctime)s.%(levelname)s: %(message)s', level=logging.INFO)

NODE_BITCOIN_CASH = (1 << 5)
invalidOpReturn = hexlify(b'Bitcoin: A Peer-to-Peer Electronic Cash System')

def bitcoinAddress2bin(btcAddress):
    """convert a bitcoin address to binary data capable of being put in a CScript"""
    # chop the version and checksum out of the bytes of the address
    return decodeBase58(btcAddress)[1:-4]


B58_DIGITS = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz'


def decodeBase58(s):
    """Decode a base58-encoding string, returning bytes"""
    if not s:
        return b''

    # Convert the string to an integer
    n = 0
    for c in s:
        n *= 58
        if c not in B58_DIGITS:
            raise InvalidBase58Error('Character %r is not a valid base58 character' % c)
        digit = B58_DIGITS.index(c)
        n += digit

    # Convert the integer to bytes
    h = '%x' % n
    if len(h) % 2:
        h = '0' + h
    res = binascii.unhexlify(h.encode('utf8'))

    # Add padding back.
    pad = 0
    for c in s[:-1]:
        if c == B58_DIGITS[0]:
            pad += 1
        else:
            break
    return b'\x00' * pad + res


def wastefulOutput(btcAddress):
    """ Warning: Creates outputs that can't be spent by bitcoind"""
    data = b"""this is junk data. this is junk data. this is junk data. this is junk data. this is junk data.
this is junk data. this is junk data. this is junk data. this is junk data. this is junk data.
this is junk data. this is junk data. this is junk data. this is junk data. this is junk data."""
    ret = CScript([data, OP_DROP, OP_DUP, OP_HASH160, bitcoinAddress2bin(btcAddress), OP_EQUALVERIFY, OP_CHECKSIG])
    return ret


def p2pkh(btcAddress):
    """ create a pay-to-public-key-hash script"""
    ret = CScript([OP_DUP, OP_HASH160, bitcoinAddress2bin(btcAddress), OP_EQUALVERIFY, OP_CHECKSIG])
    return ret


def createrawtransaction(inputs, outputs, outScriptGenerator=p2pkh):
    """
    Create a transaction with the exact input and output syntax as the bitcoin-cli "createrawtransaction" command.
    If you use the default outScriptGenerator, this function will return a hex string that exactly matches the
    output of bitcoin-cli createrawtransaction.
    """
    if not type(inputs) is list:
        inputs = [inputs]

    tx = CTransaction()
    for i in inputs:
        tx.vin.append(CTxIn(COutPoint(i["txid"], i["vout"]), b"", 0xffffffff))
    for addr, amount in outputs.items():
        if addr == "data":
            tx.vout.append(CTxOut(0, CScript([OP_RETURN, unhexlify(amount)])))
        else:
            tx.vout.append(CTxOut(amount * BTC, outScriptGenerator(addr)))
    tx.rehash()
    return hexlify(tx.serialize()).decode("utf-8")


def mostly_sync_mempools(rpc_connections, difference=50, wait=1,verbose=1):
    """
    Wait until everybody has the most of the same transactions in their memory
    pools. There is no guarantee that mempools will ever sync due to the
    filterInventoryKnown bloom filter.
    """
    iterations = 0
    while True:
        iterations+=1
        pool = set(rpc_connections[0].getrawmempool())
        num_match = 1
        poolLen = [len(pool)]
        for i in range(1, len(rpc_connections)):
            tmp = set(rpc_connections[i].getrawmempool())
            if tmp == pool:
                num_match = num_match+1
            if iterations > 10 and len(tmp.symmetric_difference(pool)) < difference:
                num_match = num_match+1
            poolLen.append(len(tmp))
        if verbose:
            logging.info("sync mempool: " + str(poolLen))
        if num_match == len(rpc_connections):
            break
        time.sleep(wait)


class BUIP055Test (BitcoinTestFramework):
    def __init__(self, extended=False):
        self.extended = extended
        BitcoinTestFramework.__init__(self)

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-rpcservertimeout=0"], timewait=60 * 10))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-rpcservertimeout=0"], timewait=60 * 10))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-rpcservertimeout=0"], timewait=60 * 10))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-rpcservertimeout=0"], timewait=60 * 10))
        interconnect_nodes(self.nodes)
        self.is_network_split = False
        self.sync_all()

    def testDefaults(self):
        for n in self.nodes:
            nodeInfo = n.getnetworkinfo()
            t = n.get("mining.fork*")
            assert(t['mining.forkBlockSize'] == 2000000)  # REQ-4-2
            assert(t['mining.forkExcessiveBlock'] == 8000000)  # REQ-4-1

            if int(nodeInfo["localservices"],16)&NODE_BITCOIN_CASH:
                assert(t['mining.forkTime'] == 1501590000)  # Bitcoin Cash release REQ-2
            else:
                assert(t['mining.forkTime'] == 0)  # main release default

    def testCli(self):
        n = self.nodes[0]
        now = int(time.time())
        n.set("mining.forkTime=%d" % now)
        n.set("mining.forkExcessiveBlock=9000000")
        n.set("mining.forkBlockSize=3000000")
        n = self.nodes[1]
        n.set("mining.forkTime=%d" % now, "mining.forkExcessiveBlock=9000000", "mining.forkBlockSize=3000000")

        # Verify that the values were properly set
        for n in self.nodes[0:2]:
            t = n.get("mining.fork*")
            assert(t['mining.forkBlockSize'] == 3000000)
            assert(t['mining.forkExcessiveBlock'] == 9000000)
            assert(t['mining.forkTime'] == now)

        self.nodes[3].set("mining.forkTime=0")
        return now

    def createUtxos(self, node, addrs, amt):
        wallet = node.listunspent()
        wallet.sort(key=lambda x: x["amount"], reverse=True)

        # Create a LOT of UTXOs
        logging.info("Create lots of UTXOs...")
        n = 0
        group = min(100, amt)
        count = 0
        for w in wallet:
            count += group
            split_transaction(node, [w], addrs[n:group + n])
            n += group
            if n >= len(addrs):
                n = 0
            if count > amt:
                break
        logging.info("mine blocks")
        node.generate(1)  # mine all the created transactions
        logging.info("sync all blocks and mempools")
        self.sync_all()

    def generateTx(self, node, txBytes, addrs, data=None):
        wallet = node.listunspent()
        wallet.sort(key=lambda x: x["amount"], reverse=False)
        logging.info("Wallet length is %d" % len(wallet))

        size = 0
        count = 0
        decContext = decimal.getcontext().prec
        decimal.getcontext().prec = 8 + 8  # 8 digits to get to 21million, and each bitcoin is 100 million satoshis
        while size < txBytes:
            count += 1
            utxo = wallet.pop()
            outp = {}
            # Make the tx bigger by adding addtl outputs so it validates faster
            payamt = satoshi_round(utxo["amount"] / decimal.Decimal(8.0))
            for x in range(0, 8):
                # its test code, I don't care if rounding error is folded into the fee
                outp[addrs[(count + x) % len(addrs)]] = payamt
                #outscript = self.wastefulOutput(addrs[(count+x)%len(addrs)])
                #outscripthex = hexlify(outscript).decode("ascii")
                #outp[outscripthex] = payamt
            if data:
                outp["data"] = data
            txn = createrawtransaction([utxo], outp, wastefulOutput)
            # txn2 = node.createrawtransaction([utxo], outp)
            signedtxn = node.signrawtransaction(txn)
            size += len(binascii.unhexlify(signedtxn["hex"]))
            node.sendrawtransaction(signedtxn["hex"])
        logging.info("%d tx %d length" % (count, size))
        decimal.getcontext().prec = decContext
        return (count, size)

    def run_test(self):
        # Creating UTXOs needed for building tx for large blocks
        NUM_ADDRS = 50
        logging.info("Creating addresses...")
        self.nodes[0].keypoolrefill(NUM_ADDRS)
        self.nodes[1].keypoolrefill(NUM_ADDRS)
        addrs = [self.nodes[0].getnewaddress() for _ in range(NUM_ADDRS)]
        addrs1 = [self.nodes[1].getnewaddress() for _ in range(NUM_ADDRS)]
        addrs3 = [self.nodes[3].getnewaddress() for _ in range(5)]
        logging.info("creating utxos")

        self.nodes[1].generate(5)
        sync_blocks(self.nodes)

        self.createUtxos(self.nodes[0], addrs, 3000)
        for i in range(0,2):
            self.createUtxos(self.nodes[1], addrs1, 3000)
        self.createUtxos(self.nodes[3], addrs3, 3000)

        sync_blocks(self.nodes)

        self.testDefaults()
        time.sleep(1)  # by controlling the forkTime, we know exactly what the median block will be
        forkTime = self.testCli()  # also sets up parameters on nodes 0, 1 to to fork
        time.sleep(2)  # Wait to ensure that every newly generated block is after the fork time.

        base = [x.getblockcount() for x in self.nodes]
        assert_equal(base, [base[0]] * 4)

        # TEST REQ-3: that a <= 1 MB block is rejected by the fork nodes
        # the rejection happens for the block after the block whose median time of the prior
        # 11 blocks is >= the fork time.  This is tricky to test exactly because we cannot
        # control a block's nTime from this python code.

        # If I generate lots of blocks at once, via generate(15) it is possible that
        # the small block fork nodes will disconnect from the large block nodes before
        # fully syncing because they get the invalid fork block.
        # This won't happen in mainnet because blocks aren't solved instantly and because
        # other fork nodes will relay the blocks.
        for i in range(0,15):
          self.nodes[3].generate(1)
          time.sleep(2)

        sync_blocks(self.nodes[2:])
        sync_blocks(self.nodes[0:2])

        nMedianTimeSpan = 11  # from chain.h
        projectedForkHeight = int(base[0] + nMedianTimeSpan / 2 + 1)

        counts = [x.getblockcount() for x in self.nodes]
        logging.info("waiting for block: %d" % projectedForkHeight)
        while counts[0] != projectedForkHeight:
            counts = [x.getblockcount() for x in self.nodes]
            logging.info(counts)
            time.sleep(1)

        assert(counts[0] < counts[2])
        assert(counts[1] < counts[3])
        assert(counts[0] == counts[1])
        assert(counts[2] == counts[3])

        # TEST that the client refuses to make a < 1MB fork block
        node = self.nodes[0]

        try:
            ret = node.generate(1)
            logging.info(ret)
            assert(0)  # should have raised exception
        except JSONRPCException as e:
            assert("bad-blk-too-small" in e.error["message"])

        logging.info("Building > 1MB block...")

        # TEST that the client refuses to include invalid op return txns in the first block
        self.generateTx(node, 950000, addrs, data='54686973206973203830206279746573206f6620746573742064617461206372656174656420746f20757365207570207472616e73616374696f6e20737061636520666173746572202e2e2e2e2e2e2e')
        try:
            self.generateTx(node,100000, addrs,data=invalidOpReturn)
            assert(0)  # should have raised exception
        except JSONRPCException as e:
            assert("wrong-fork" in e.error["message"])

        # temporarily turn off forking so we can inject some bad tx into the mempool
        node.set("mining.forkTime=0")
        unspendableTx, unspendableTxSize = self.generateTx(node,100000, addrs,data=invalidOpReturn)
        node.set("mining.forkTime=%d" % forkTime)

        # node 3 is not forking so these tx are allowed
        self.generateTx(self.nodes[3],100000,addrs,data=invalidOpReturn)

        try:
            ret = node.generate(1)
            logging.info(ret)
            assert(0)  # should have raised exception
        except JSONRPCException as e:
            assert("bad-blk-too-small" in e.error["message"])
            logging.info("PASS: Invalid OP return transactions were not used when attempting to make the fork block")

        # TEST REQ-3: generate a large block
        self.generateTx(node, 100000, addrs)
        # if we don't sync mempools, when a block is created the system will be so busy syncing tx that it will time out
        # requesting the block, and so never receive it.
        # This only happens in testnet because there is only 1 node generating all the tx and with the block.
        # But node 0 has a lot of invalid transactions in it, so the sync needs to be pretty loose
        mostly_sync_mempools(self.nodes[0:2],difference=100, wait=2)

        commonAncestor = node.getbestblockhash()
        node.generate(1)
        forkHeight = node.getblockcount()
        print("forkHeight: %d" % forkHeight)
        # Test that the forked nodes accept this block as the fork block
        sync_blocks(self.nodes[0:2])
        # counts = [ x.getblockcount() for x in self.nodes[0:2] ]
        counts = [x.getblockcount() for x in self.nodes]
        logging.info(counts)

        # generate blocks and ensure that the other node syncs them
        self.nodes[1].generate(1)
        wallet = self.nodes[1].listunspent()
        utxo = wallet.pop()
        txn = createrawtransaction([utxo], {addrs1[0]:utxo["amount"]}, wastefulOutput)
        signedtxn = self.nodes[1].signrawtransaction(txn)
        signedtxn2 = self.nodes[1].signrawtransaction(txn,None,None,"ALL|FORKID")
        assert(signedtxn["hex"] != signedtxn2["hex"])  # they should use a different sighash method
        try:
            self.nodes[3].sendrawtransaction(signedtxn2["hex"])
        except JSONRPCException as e:
            assert("mandatory-script-verify-flag-failed" in e.error["message"])
            logging.info("PASS: New sighash rejected from 1MB chain")
        self.nodes[1].sendrawtransaction(signedtxn2["hex"])
        try:
           self.nodes[1].sendrawtransaction(signedtxn["hex"])
        except JSONRPCException as e:
            assert("txn-mempool-conflict" in e.error["message"])
            logging.info("PASS: submission of new and old sighash txn rejected")
        self.nodes[1].generate(1)

        # connect 1 to 3 to propagate these transactions
        connect_nodes(self.nodes[1],3)

        # Issue sendtoaddress commands using both the new sighash and the ond and ensure that they work.
        self.nodes[1].set("wallet.useNewSig=False")
        txhash2 = self.nodes[1].sendtoaddress(addrs[0], 2.345)
        self.nodes[1].set("wallet.useNewSig=True")
        # produce a new sighash transaction using the sendtoaddress API
        txhash = self.nodes[1].sendtoaddress(addrs[0], 1.234)
        rawtx = self.nodes[1].getrawtransaction(txhash)
        try:
            self.nodes[3].sendrawtransaction(rawtx)
            print("ERROR!") # error assert(0)
        except JSONRPCException as e:
            assert("mandatory-script-verify-flag-failed" in e.error["message"])
            # hitting this exception verifies that the new format was rejected by the unforked node and that the new format was generated

        rawtx = self.nodes[1].getrawtransaction(txhash2)
        self.nodes[3].sendrawtransaction(rawtx)  # should replay on the small block fork, since its an old sighash tx

        self.nodes[1].generate(1)
        txinfo = self.nodes[1].gettransaction(txhash)
        assert(txinfo["blockindex"] > 0) # ensure that the new-style tx was included in the block
        txinfo = self.nodes[1].gettransaction(txhash2)
        assert(txinfo["blockindex"] > 0) # ensure that the old-style tx was included in the block

        # small block node should have gotten this cross-chain replayable tx
        txsIn3 = self.nodes[3].getrawmempool()
        assert(txhash2 in txsIn3)
        self.nodes[3].generate(1)
        txsIn3 = self.nodes[3].getrawmempool()
        assert(txsIn3 == []) # all transactions were included in the block

        # Issue sendmany commands using both the new sighash and the ond and ensure that they work.
        self.nodes[1].set("wallet.useNewSig=False")
        txhash2 = self.nodes[1].sendmany("",{addrs[0]:2.345, addrs[1]:1.23})
        self.nodes[1].set("wallet.useNewSig=True")
        # produce a new sighash transaction using the sendtoaddress API
        txhash = self.nodes[1].sendmany("",{addrs[0]:0.345, addrs[1]:0.23})
        rawtx = self.nodes[1].getrawtransaction(txhash)
        try:
            self.nodes[3].sendrawtransaction(rawtx)
            print("ERROR!") # error assert(0)
        except JSONRPCException as e:
            assert("mandatory-script-verify-flag-failed" in e.error["message"])
            # hitting this exception verifies that the new format was rejected by the unforked node and that the new format was generated
        self.nodes[1].generate(1)

        sync_blocks(self.nodes[0:2])

        self.nodes[0].generate(2)
        sync_blocks(self.nodes[0:2])

        # generate blocks on the original side
        self.nodes[2].generate(2)
        sync_blocks(self.nodes[2:])
        counts = [x.getblockcount() for x in self.nodes]
        assert(counts == [forkHeight + 6, forkHeight + 6, base[3] + 15 + 3, base[3] + 15 + 3])
        forkBest = self.nodes[0].getbestblockhash()
        origBest = self.nodes[3].getbestblockhash()
        logging.info("Fork height: %d" % forkHeight)
        logging.info("Common ancestor: %s" % commonAncestor)
        logging.info("Fork tip: %s" % forkBest)
        logging.info("Small block tip: %s" % origBest)

        # Limitation: fork logic will not cause a re-org if the node is beyond it
        logging.info("Show that a re-org will not happen if the stopped/started node is already beyond the fork")
        stop_node(self.nodes[2], 2)
        self.nodes[2] = start_node(2, self.options.tmpdir, ["-debug", "-mining.forkTime=%d" %
                                                            forkTime, "-mining.forkExcessiveBlock=9000000", "-mining.forkBlockSize=3000000"], timewait=900)
        connect_nodes(self.nodes[2], 3)
        connect_nodes(self.nodes[2], 0)
        sync_blocks(self.nodes[0:2])
        assert(self.nodes[2].getbestblockhash() == origBest)

        # Now clean up the node to force a re-sync, but connect to the small block fork nodes
        logging.info("Resync but only connected to the small block nodes")
        stop_node(self.nodes[2], 2)
        shutil.rmtree(self.options.tmpdir + os.sep + "node2" + os.sep + "regtest")
        self.nodes[2] = start_node(2, self.options.tmpdir, ["-debug", "-mining.forkTime=%d" %
                                                            forkTime, "-mining.forkExcessiveBlock=9000000", "-mining.forkBlockSize=3000000"], timewait=900)
        connect_nodes(self.nodes[2], 3)

        # I'm not expecting the nodes to fully sync because the fork client is only connected to the small block clients
        waitCount = 0
        while self.nodes[2].getblockcount() < forkHeight - 1:
            time.sleep(5)
            waitCount += 5
            if waitCount > 30:
                # we can't be sure the node will fully sync.  It may be dropped early
                logging.info("Node did not sync up to the fork height")
                break
            logging.info(self.nodes[2].getblockcount())
            if self.nodes[2].getpeerinfo() == []:  # I'll drop as soon as peer 3 tries to give me the invalid fork block
                break
        time.sleep(10)  # wait longer to see if we continue to sync
        t = self.nodes[2].getinfo()

        # Cannot progress beyond the common ancestor, because we are looking for a big block
        # however, if we were processing multiple blocks at once, we might not get to exactly forkHeight - 1 so < is needed
        assert(t["blocks"] < forkHeight)

        # now connect to fork node to see it continue to sync
        logging.info("connect to a large block node and see that sync completes")
        self.nodes[0].clearbanned()  # node 2 can get banned in prior tests because it may have served the small block
        connect_nodes(self.nodes[2], 0)
        sync_blocks(self.nodes[0:3])
        assert(self.nodes[2].getbestblockhash() == forkBest)

        # test full sync if only connected to forked nodes
        stop_node(self.nodes[2], 2)
        logging.info("Resync to minority fork connected to minority fork nodes only")

        shutil.rmtree(self.options.tmpdir + os.sep + "node2" + os.sep + "regtest")
        self.nodes[2] = start_node(2, self.options.tmpdir, ["-debug", "-mining.forkTime=%d" %
                                                            forkTime, "-mining.forkExcessiveBlock=9000000", "-mining.forkBlockSize=3000000"], timewait=900)
        self.nodes[0].clearbanned()  # node 2 can get banned in prior tests because it may have served the small block
        connect_nodes(self.nodes[2], 0)
        sync_blocks(self.nodes[0:3])
        t = self.nodes[2].getinfo()
        assert(self.nodes[2].getbestblockhash() == forkBest)

        # Now clean up the node to force a re-sync, but connect to both forks to prove it follows the proper fork
        stop_node(self.nodes[2], 2)
        logging.info("Resync to minority fork in the presence of majority fork nodes")
        shutil.rmtree(self.options.tmpdir + os.sep + "node2" + os.sep + "regtest")
        self.nodes[2] = start_node(2, self.options.tmpdir, ["-debug", "-mining.forkTime=%d" %
                                                            forkTime, "-mining.forkExcessiveBlock=9000000", "-mining.forkBlockSize=3000000"], timewait=900)
        self.nodes[0].clearbanned()  # node 2 can get banned in prior tests because it may have served the small block
        self.nodes[3].clearbanned()  # node 2 can get banned in prior tests because it may have served the small block
        connect_nodes(self.nodes[2], 3)
        connect_nodes(self.nodes[2], 0)
        sync_blocks(self.nodes[0:3])

        assert(self.nodes[2].getbestblockhash() == forkBest)
        # pdb.set_trace()

        logging.info("Reindex across fork")
        # see if we reindex properly across the fork
        node = self.nodes[2]
        curCount = node.getblockcount()
        stop_node(node, 2)
        node = self.nodes[2] = start_node(2, self.options.tmpdir, ["-debug", "-reindex", "-checkblockindex=1", "-mining.forkTime=%d" %
                                                                   forkTime, "-mining.forkExcessiveBlock=9000000", "-mining.forkBlockSize=3000000"], timewait=900)
        time.sleep(10)
        assert_equal(node.getblockcount(), curCount)

        connect_nodes(self.nodes[2], 3)
        connect_nodes(self.nodes[2], 0)

        # Verify that the unspendable tx I created never got spent
        mempool = self.nodes[0].getmempoolinfo()
        print(mempool)
        assert(mempool["size"] == unspendableTx)
        assert(mempool["bytes"] == unspendableTxSize)

        # Now create some big blocks to ensure that we are properly creating them
        self.generateTx(self.nodes[1], 1005000, addrs, data='54686973206973203830206279746573206f6620746573742064617461206372656174656420746f20757365207570207472616e73616374696f6e20737061636520666173746572202e2e2e2e2e2e2e')
        blkHash = self.nodes[1].generate(1)[0]
        blkInfo = self.nodes[1].getblock(blkHash)
        assert(blkInfo["size"] > 1000000)
        self.generateTx(self.nodes[1], 4000000, addrs, data='54686973206973203830206279746573206f6620746573742064617461206372656174656420746f20757365207570207472616e73616374696f6e20737061636520666173746572202e2e2e2e2e2e2e')
        blkHash = self.nodes[1].generate(1)[0]
        blkInfo = self.nodes[1].getblock(blkHash)
        assert(blkInfo["size"] <= 3000000)  # this is the configured max block size for this test
        assert(blkInfo["size"] >= 2900000)  # block should have been filled up!

        self.nodes[1].generate(1) # should consume all the rest of the spendable tx

        # The unspendable tx I created on node 0 should not have been relayed to node 1
        mempool = self.nodes[1].getmempoolinfo()
        assert(mempool["size"] == 0)
        assert(mempool["bytes"] == 0)

        sync_blocks(self.nodes[0:3])
        # The unspendable tx I created on node 0 should still be there
        mempool = self.nodes[0].getmempoolinfo()
        assert(mempool["size"] == unspendableTx)
        assert(mempool["bytes"] == unspendableTxSize)



def info(type, value, tb):
    if hasattr(sys, 'ps1') or not sys.stderr.isatty():
        # we are in interactive mode or we don't have a tty-like
        # device, so we call the default hook
        sys.__excepthook__(type, value, tb)
    else:
        import traceback
        import pdb
        # we are NOT in interactive mode, print the exception...
        traceback.print_exception(type, value, tb)
        print
        # ...then start the debugger in post-mortem mode.
        pdb.pm()


sys.excepthook = info


def Test():
    t = BUIP055Test(True)
    bitcoinConf = {
        "debug": ["net", "blk", "thin", "mempool", "req", "bench", "evict"],  # "lck"
        "blockprioritysize": 2000000  # we don't want any transactions rejected due to insufficient fees...
    }
# "--tmpdir=/ramdisk/test", "--nocleanup","--noshutdown"
    t.main(["--tmpdir=/ramdisk/test"], bitcoinConf, None)  # , "--tracerpc"])
#  t.main([],bitcoinConf,None)


if __name__ == '__main__':
    BUIP055Test(True).main(sys.argv)
