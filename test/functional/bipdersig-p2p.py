#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test BIP66 (DER SIG).

Test that the DERSIG soft-fork activates at (regtest) height 1251.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.mininode import *
from test_framework.blocktools import create_coinbase, create_block
from test_framework.script import CScript
from io import BytesIO

DERSIG_HEIGHT = 1251

# Reject codes that we might receive in this test
REJECT_INVALID = 16
REJECT_OBSOLETE = 17
REJECT_NONSTANDARD = 64
VB_TOP_BITS = 0x20000000

# A canonical signature consists of:
# <30> <total len> <02> <len R> <R> <02> <len S> <S> <hashtype>
def unDERify(tx):
    """
    Make the signature in vin 0 of a tx non-DER-compliant,
    by adding padding after the S-value.
    """
    scriptSig = CScript(tx.vin[0].scriptSig)
    newscript = []
    for i in scriptSig:
        if (len(newscript) == 0):
            newscript.append(i[0:-1] + b'\0' + i[-1:])
        else:
            newscript.append(i)
    tx.vin[0].scriptSig = CScript(newscript)

def create_transaction(node, coinbase, to_address, amount):
    from_txid = node.getblock(coinbase)['tx'][0]
    inputs = [{ "txid" : from_txid, "vout" : 0}]
    outputs = { to_address : amount }
    rawtx = node.createrawtransaction(inputs, outputs)
    signresult = node.signrawtransaction(rawtx)
    tx = CTransaction()
    tx.deserialize(BytesIO(hex_str_to_bytes(signresult['hex'])))
    return tx

class BIP66Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['-promiscuousmempoolflags=1', '-whitelist=127.0.0.1']]
        self.setup_clean_chain = True

    def run_test(self):
        node0 = NodeConnCB()
        connections = []
        connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], node0))
        node0.add_connection(connections[0])
        NetworkThread().start() # Start up network handling in another thread

        # wait_for_verack ensures that the P2P connection is fully up.
        node0.wait_for_verack()

        self.log.info("Mining %d blocks", DERSIG_HEIGHT - 2)
        self.coinbase_blocks = self.nodes[0].generate(DERSIG_HEIGHT - 2)
        self.nodeaddress = self.nodes[0].getnewaddress()

        self.log.info("Test that a transaction with non-DER signature can still appear in a block")

        spendtx = create_transaction(self.nodes[0], self.coinbase_blocks[0],
                self.nodeaddress, 1.0)
        unDERify(spendtx)
        spendtx.rehash()

        tip = self.nodes[0].getbestblockhash()
        block_time = self.nodes[0].getblockheader(tip)['mediantime'] + 1
        block = create_block(int(tip, 16), create_coinbase(DERSIG_HEIGHT - 1), block_time)
        block.nVersion = VB_TOP_BITS
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()

        node0.send_and_ping(msg_block(block))
        assert_equal(self.nodes[0].getbestblockhash(), block.hash)

        self.log.info("Test that blocks must now be at least version VB_TOP_BITS")
        tip = block.sha256
        block_time += 1
        block = create_block(tip, create_coinbase(DERSIG_HEIGHT), block_time)
        block.nVersion = 2
        block.rehash()
        block.solve()
        node0.send_and_ping(msg_block(block))
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), tip)

        wait_until(lambda: "reject" in node0.last_message.keys(), lock=mininode_lock)
        with mininode_lock:
            assert_equal(node0.last_message["reject"].code, REJECT_OBSOLETE)
            assert_equal(node0.last_message["reject"].reason, b'bad-version(0x00000002)')
            assert_equal(node0.last_message["reject"].data, block.sha256)
            del node0.last_message["reject"]

        self.log.info("Test that transactions with non-DER signatures cannot appear in a block")
        block.nVersion = VB_TOP_BITS

        spendtx = create_transaction(self.nodes[0], self.coinbase_blocks[1],
                self.nodeaddress, 1.0)
        unDERify(spendtx)
        spendtx.rehash()

        # First we show that this tx is valid except for DERSIG by getting it
        # accepted to the mempool (which we can achieve with
        # -promiscuousmempoolflags).
        node0.send_and_ping(msg_tx(spendtx))
        assert spendtx.hash in self.nodes[0].getrawmempool()

        # Now we verify that a block with this transaction is invalid.
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()

        node0.send_and_ping(msg_block(block))
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), tip)

        wait_until(lambda: "reject" in node0.last_message.keys(), lock=mininode_lock)
        with mininode_lock:
            # We can receive different reject messages depending on whether
            # bitcoind is running with multiple script check threads. If script
            # check threads are not in use, then transaction script validation
            # happens sequentially, and bitcoind produces more specific reject
            # reasons.
            assert node0.last_message["reject"].code in [REJECT_INVALID, REJECT_NONSTANDARD]
            assert_equal(node0.last_message["reject"].data, block.sha256)
            if node0.last_message["reject"].code == REJECT_INVALID:
                # Generic rejection when a block is invalid
                assert_equal(node0.last_message["reject"].reason, b'block-validation-failed')
            else:
                assert b'Non-canonical DER signature' in node0.last_message["reject"].reason

        self.log.info("Test that a version VB_TOP_BITS block with a DERSIG-compliant transaction is accepted")
        block.vtx[1] = create_transaction(self.nodes[0],
                self.coinbase_blocks[1], self.nodeaddress, 1.0)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()

        node0.send_and_ping(msg_block(block))
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.sha256)

if __name__ == '__main__':
    BIP66Test().main()
