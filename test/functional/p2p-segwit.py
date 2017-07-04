#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test segwit transactions and blocks on P2P network."""

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.blocktools import create_block, create_coinbase, add_witness_commitment, get_witness_script, WITNESS_COMMITMENT_HEADER
from test_framework.key import CECKey, CPubKey
import time
import random
from binascii import hexlify

# The versionbit bit used to signal activation of SegWit
VB_WITNESS_BIT = 1
VB_PERIOD = 144
VB_ACTIVATION_THRESHOLD = 108
VB_TOP_BITS = 0x20000000

MAX_SIGOP_COST = 80000


# Calculate the virtual size of a witness block:
# (base + witness/4)
def get_virtual_size(witness_block):
    base_size = len(witness_block.serialize())
    total_size = len(witness_block.serialize(with_witness=True))
    # the "+3" is so we round up
    vsize = int((3*base_size + total_size + 3)/4)
    return vsize

class TestNode(NodeConnCB):
    def __init__(self):
        super().__init__()
        self.getdataset = set()

    def on_getdata(self, conn, message):
        for inv in message.inv:
            self.getdataset.add(inv.hash)

    def announce_tx_and_wait_for_getdata(self, tx, timeout=60):
        with mininode_lock:
            self.last_message.pop("getdata", None)
        self.send_message(msg_inv(inv=[CInv(1, tx.sha256)]))
        self.wait_for_getdata(timeout)

    def announce_block_and_wait_for_getdata(self, block, use_header, timeout=60):
        with mininode_lock:
            self.last_message.pop("getdata", None)
            self.last_message.pop("getheaders", None)
        msg = msg_headers()
        msg.headers = [ CBlockHeader(block) ]
        if use_header:
            self.send_message(msg)
        else:
            self.send_message(msg_inv(inv=[CInv(2, block.sha256)]))
            self.wait_for_getheaders()
            self.send_message(msg)
        self.wait_for_getdata()

    def request_block(self, blockhash, inv_type, timeout=60):
        with mininode_lock:
            self.last_message.pop("block", None)
        self.send_message(msg_getdata(inv=[CInv(inv_type, blockhash)]))
        self.wait_for_block(blockhash, timeout)
        return self.last_message["block"].block

    def test_transaction_acceptance(self, tx, with_witness, accepted, reason=None):
        tx_message = msg_tx(tx)
        if with_witness:
            tx_message = msg_witness_tx(tx)
        self.send_message(tx_message)
        self.sync_with_ping()
        assert_equal(tx.hash in self.connection.rpc.getrawmempool(), accepted)
        if (reason != None and not accepted):
            # Check the rejection reason as well.
            with mininode_lock:
                assert_equal(self.last_message["reject"].reason, reason)

    # Test whether a witness block had the correct effect on the tip
    def test_witness_block(self, block, accepted, with_witness=True):
        if with_witness:
            self.send_message(msg_witness_block(block))
        else:
            self.send_message(msg_block(block))
        self.sync_with_ping()
        assert_equal(self.connection.rpc.getbestblockhash() == block.hash, accepted)

# Used to keep track of anyone-can-spend outputs that we can use in the tests
class UTXO(object):
    def __init__(self, sha256, n, nValue):
        self.sha256 = sha256
        self.n = n
        self.nValue = nValue

# Helper for getting the script associated with a P2PKH
def GetP2PKHScript(pubkeyhash):
    return CScript([CScriptOp(OP_DUP), CScriptOp(OP_HASH160), pubkeyhash, CScriptOp(OP_EQUALVERIFY), CScriptOp(OP_CHECKSIG)])

# Add signature for a P2PK witness program.
def sign_P2PK_witness_input(script, txTo, inIdx, hashtype, value, key):
    tx_hash = SegwitVersion1SignatureHash(script, txTo, inIdx, hashtype, value)
    signature = key.sign(tx_hash) + chr(hashtype).encode('latin-1')
    txTo.wit.vtxinwit[inIdx].scriptWitness.stack = [signature, script]
    txTo.rehash()


class SegWitTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [["-whitelist=127.0.0.1"], ["-whitelist=127.0.0.1", "-acceptnonstdtxn=0"], ["-whitelist=127.0.0.1", "-vbparams=segwit:0:0"]]

    def setup_network(self):
        self.setup_nodes()
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        self.sync_all()

    ''' Helpers '''
    # Build a block on top of node0's tip.
    def build_next_block(self, nVersion=4):
        tip = self.nodes[0].getbestblockhash()
        height = self.nodes[0].getblockcount() + 1
        block_time = self.nodes[0].getblockheader(tip)["mediantime"] + 1
        block = create_block(int(tip, 16), create_coinbase(height), block_time)
        block.nVersion = nVersion
        block.rehash()
        return block

    # Adds list of transactions to block, adds witness commitment, then solves.
    def update_witness_block_with_transactions(self, block, tx_list, nonce=0):
        block.vtx.extend(tx_list)
        add_witness_commitment(block, nonce)
        block.solve()
        return

    ''' Individual tests '''
    def test_witness_services(self):
        self.log.info("Verifying NODE_WITNESS service bit")
        assert((self.test_node.connection.nServices & NODE_WITNESS) != 0)


    # See if sending a regular transaction works, and create a utxo
    # to use in later tests.
    def test_non_witness_transaction(self):
        # Mine a block with an anyone-can-spend coinbase,
        # let it mature, then try to spend it.
        self.log.info("Testing non-witness transaction")
        block = self.build_next_block(nVersion=1)
        block.solve()
        self.test_node.send_message(msg_block(block))
        self.test_node.sync_with_ping() # make sure the block was processed
        txid = block.vtx[0].sha256

        self.nodes[0].generate(99) # let the block mature

        # Create a transaction that spends the coinbase
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(txid, 0), b""))
        tx.vout.append(CTxOut(49*100000000, CScript([OP_TRUE])))
        tx.calc_sha256()

        # Check that serializing it with or without witness is the same
        # This is a sanity check of our testing framework.
        assert_equal(msg_tx(tx).serialize(), msg_witness_tx(tx).serialize())

        self.test_node.send_message(msg_witness_tx(tx))
        self.test_node.sync_with_ping() # make sure the tx was processed
        assert(tx.hash in self.nodes[0].getrawmempool())
        # Save this transaction for later
        self.utxo.append(UTXO(tx.sha256, 0, 49*100000000))
        self.nodes[0].generate(1)


    # Verify that blocks with witnesses are rejected before activation.
    def test_unnecessary_witness_before_segwit_activation(self):
        self.log.info("Testing behavior of unnecessary witnesses")
        # For now, rely on earlier tests to have created at least one utxo for
        # us to use
        assert(len(self.utxo) > 0)
        assert(get_bip9_status(self.nodes[0], 'segwit')['status'] != 'active')

        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))
        tx.vout.append(CTxOut(self.utxo[0].nValue-1000, CScript([OP_TRUE])))
        tx.wit.vtxinwit.append(CTxInWitness())
        tx.wit.vtxinwit[0].scriptWitness.stack = [CScript([CScriptNum(1)])]

        # Verify the hash with witness differs from the txid
        # (otherwise our testing framework must be broken!)
        tx.rehash()
        assert(tx.sha256 != tx.calc_sha256(with_witness=True))

        # Construct a segwit-signaling block that includes the transaction.
        block = self.build_next_block(nVersion=(VB_TOP_BITS|(1 << VB_WITNESS_BIT)))
        self.update_witness_block_with_transactions(block, [tx])
        # Sending witness data before activation is not allowed (anti-spam
        # rule).
        self.test_node.test_witness_block(block, accepted=False)
        # TODO: fix synchronization so we can test reject reason
        # Right now, bitcoind delays sending reject messages for blocks
        # until the future, making synchronization here difficult.
        #assert_equal(self.test_node.last_message["reject"].reason, "unexpected-witness")

        # But it should not be permanently marked bad...
        # Resend without witness information.
        self.test_node.send_message(msg_block(block))
        self.test_node.sync_with_ping()
        assert_equal(self.nodes[0].getbestblockhash(), block.hash)

        sync_blocks(self.nodes)

        # Create a p2sh output -- this is so we can pass the standardness
        # rules (an anyone-can-spend OP_TRUE would be rejected, if not wrapped
        # in P2SH).
        p2sh_program = CScript([OP_TRUE])
        p2sh_pubkey = hash160(p2sh_program)
        scriptPubKey = CScript([OP_HASH160, p2sh_pubkey, OP_EQUAL])

        # Now check that unnecessary witnesses can't be used to blind a node
        # to a transaction, eg by violating standardness checks.
        tx2 = CTransaction()
        tx2.vin.append(CTxIn(COutPoint(tx.sha256, 0), b""))
        tx2.vout.append(CTxOut(tx.vout[0].nValue-1000, scriptPubKey))
        tx2.rehash()
        self.test_node.test_transaction_acceptance(tx2, False, True)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        # We'll add an unnecessary witness to this transaction that would cause
        # it to be non-standard, to test that violating policy with a witness before
        # segwit activation doesn't blind a node to a transaction.  Transactions
        # rejected for having a witness before segwit activation shouldn't be added
        # to the rejection cache.
        tx3 = CTransaction()
        tx3.vin.append(CTxIn(COutPoint(tx2.sha256, 0), CScript([p2sh_program])))
        tx3.vout.append(CTxOut(tx2.vout[0].nValue-1000, scriptPubKey))
        tx3.wit.vtxinwit.append(CTxInWitness())
        tx3.wit.vtxinwit[0].scriptWitness.stack = [b'a'*400000]
        tx3.rehash()
        # Note that this should be rejected for the premature witness reason,
        # rather than a policy check, since segwit hasn't activated yet.
        self.std_node.test_transaction_acceptance(tx3, True, False, b'no-witness-yet')

        # If we send without witness, it should be accepted.
        self.std_node.test_transaction_acceptance(tx3, False, True)

        # Now create a new anyone-can-spend utxo for the next test.
        tx4 = CTransaction()
        tx4.vin.append(CTxIn(COutPoint(tx3.sha256, 0), CScript([p2sh_program])))
        tx4.vout.append(CTxOut(tx3.vout[0].nValue-1000, CScript([OP_TRUE])))
        tx4.rehash()
        self.test_node.test_transaction_acceptance(tx3, False, True)
        self.test_node.test_transaction_acceptance(tx4, False, True)

        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        # Update our utxo list; we spent the first entry.
        self.utxo.pop(0)
        self.utxo.append(UTXO(tx4.sha256, 0, tx4.vout[0].nValue))


    # Mine enough blocks for segwit's vb state to be 'started'.
    def advance_to_segwit_started(self):
        height = self.nodes[0].getblockcount()
        # Will need to rewrite the tests here if we are past the first period
        assert(height < VB_PERIOD - 1)
        # Genesis block is 'defined'.
        assert_equal(get_bip9_status(self.nodes[0], 'segwit')['status'], 'defined')
        # Advance to end of period, status should now be 'started'
        self.nodes[0].generate(VB_PERIOD-height-1)
        assert_equal(get_bip9_status(self.nodes[0], 'segwit')['status'], 'started')

    # Mine enough blocks to lock in segwit, but don't activate.
    # TODO: we could verify that lockin only happens at the right threshold of
    # signalling blocks, rather than just at the right period boundary.
    def advance_to_segwit_lockin(self):
        height = self.nodes[0].getblockcount()
        assert_equal(get_bip9_status(self.nodes[0], 'segwit')['status'], 'started')
        # Advance to end of period, and verify lock-in happens at the end
        self.nodes[0].generate(VB_PERIOD-1)
        height = self.nodes[0].getblockcount()
        assert((height % VB_PERIOD) == VB_PERIOD - 2)
        assert_equal(get_bip9_status(self.nodes[0], 'segwit')['status'], 'started')
        self.nodes[0].generate(1)
        assert_equal(get_bip9_status(self.nodes[0], 'segwit')['status'], 'locked_in')


    # Mine enough blocks to activate segwit.
    # TODO: we could verify that activation only happens at the right threshold
    # of signalling blocks, rather than just at the right period boundary.
    def advance_to_segwit_active(self):
        assert_equal(get_bip9_status(self.nodes[0], 'segwit')['status'], 'locked_in')
        height = self.nodes[0].getblockcount()
        self.nodes[0].generate(VB_PERIOD - (height%VB_PERIOD) - 2)
        assert_equal(get_bip9_status(self.nodes[0], 'segwit')['status'], 'locked_in')
        self.nodes[0].generate(1)
        assert_equal(get_bip9_status(self.nodes[0], 'segwit')['status'], 'active')


    # This test can only be run after segwit has activated
    def test_witness_commitments(self):
        self.log.info("Testing witness commitments")

        # First try a correct witness commitment.
        block = self.build_next_block()
        add_witness_commitment(block)
        block.solve()

        # Test the test -- witness serialization should be different
        assert(msg_witness_block(block).serialize() != msg_block(block).serialize())

        # This empty block should be valid.
        self.test_node.test_witness_block(block, accepted=True)

        # Try to tweak the nonce
        block_2 = self.build_next_block()
        add_witness_commitment(block_2, nonce=28)
        block_2.solve()

        # The commitment should have changed!
        assert(block_2.vtx[0].vout[-1] != block.vtx[0].vout[-1])

        # This should also be valid.
        self.test_node.test_witness_block(block_2, accepted=True)

        # Now test commitments with actual transactions
        assert (len(self.utxo) > 0)
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))

        # Let's construct a witness program
        witness_program = CScript([OP_TRUE])
        witness_hash = sha256(witness_program)
        scriptPubKey = CScript([OP_0, witness_hash])
        tx.vout.append(CTxOut(self.utxo[0].nValue-1000, scriptPubKey))
        tx.rehash()

        # tx2 will spend tx1, and send back to a regular anyone-can-spend address
        tx2 = CTransaction()
        tx2.vin.append(CTxIn(COutPoint(tx.sha256, 0), b""))
        tx2.vout.append(CTxOut(tx.vout[0].nValue-1000, witness_program))
        tx2.wit.vtxinwit.append(CTxInWitness())
        tx2.wit.vtxinwit[0].scriptWitness.stack = [witness_program]
        tx2.rehash()

        block_3 = self.build_next_block()
        self.update_witness_block_with_transactions(block_3, [tx, tx2], nonce=1)
        # Add an extra OP_RETURN output that matches the witness commitment template,
        # even though it has extra data after the incorrect commitment.
        # This block should fail.
        block_3.vtx[0].vout.append(CTxOut(0, CScript([OP_RETURN, WITNESS_COMMITMENT_HEADER + ser_uint256(2), 10])))
        block_3.vtx[0].rehash()
        block_3.hashMerkleRoot = block_3.calc_merkle_root()
        block_3.rehash()
        block_3.solve()

        self.test_node.test_witness_block(block_3, accepted=False)

        # Add a different commitment with different nonce, but in the
        # right location, and with some funds burned(!).
        # This should succeed (nValue shouldn't affect finding the
        # witness commitment).
        add_witness_commitment(block_3, nonce=0)
        block_3.vtx[0].vout[0].nValue -= 1
        block_3.vtx[0].vout[-1].nValue += 1
        block_3.vtx[0].rehash()
        block_3.hashMerkleRoot = block_3.calc_merkle_root()
        block_3.rehash()
        assert(len(block_3.vtx[0].vout) == 4) # 3 OP_returns
        block_3.solve()
        self.test_node.test_witness_block(block_3, accepted=True)

        # Finally test that a block with no witness transactions can
        # omit the commitment.
        block_4 = self.build_next_block()
        tx3 = CTransaction()
        tx3.vin.append(CTxIn(COutPoint(tx2.sha256, 0), b""))
        tx3.vout.append(CTxOut(tx.vout[0].nValue-1000, witness_program))
        tx3.rehash()
        block_4.vtx.append(tx3)
        block_4.hashMerkleRoot = block_4.calc_merkle_root()
        block_4.solve()
        self.test_node.test_witness_block(block_4, with_witness=False, accepted=True)

        # Update available utxo's for use in later test.
        self.utxo.pop(0)
        self.utxo.append(UTXO(tx3.sha256, 0, tx3.vout[0].nValue))


    def test_block_malleability(self):
        self.log.info("Testing witness block malleability")

        # Make sure that a block that has too big a virtual size
        # because of a too-large coinbase witness is not permanently
        # marked bad.
        block = self.build_next_block()
        add_witness_commitment(block)
        block.solve()

        block.vtx[0].wit.vtxinwit[0].scriptWitness.stack.append(b'a'*5000000)
        assert(get_virtual_size(block) > MAX_BLOCK_BASE_SIZE)

        # We can't send over the p2p network, because this is too big to relay
        # TODO: repeat this test with a block that can be relayed
        self.nodes[0].submitblock(bytes_to_hex_str(block.serialize(True)))

        assert(self.nodes[0].getbestblockhash() != block.hash)

        block.vtx[0].wit.vtxinwit[0].scriptWitness.stack.pop()
        assert(get_virtual_size(block) < MAX_BLOCK_BASE_SIZE)
        self.nodes[0].submitblock(bytes_to_hex_str(block.serialize(True)))

        assert(self.nodes[0].getbestblockhash() == block.hash)

        # Now make sure that malleating the witness nonce doesn't
        # result in a block permanently marked bad.
        block = self.build_next_block()
        add_witness_commitment(block)
        block.solve()

        # Change the nonce -- should not cause the block to be permanently
        # failed
        block.vtx[0].wit.vtxinwit[0].scriptWitness.stack = [ ser_uint256(1) ]
        self.test_node.test_witness_block(block, accepted=False)

        # Changing the witness nonce doesn't change the block hash
        block.vtx[0].wit.vtxinwit[0].scriptWitness.stack = [ ser_uint256(0) ]
        self.test_node.test_witness_block(block, accepted=True)


    def test_witness_block_size(self):
        self.log.info("Testing witness block size limit")
        # TODO: Test that non-witness carrying blocks can't exceed 1MB
        # Skipping this test for now; this is covered in p2p-fullblocktest.py

        # Test that witness-bearing blocks are limited at ceil(base + wit/4) <= 1MB.
        block = self.build_next_block()

        assert(len(self.utxo) > 0)
        
        # Create a P2WSH transaction.
        # The witness program will be a bunch of OP_2DROP's, followed by OP_TRUE.
        # This should give us plenty of room to tweak the spending tx's
        # virtual size.
        NUM_DROPS = 200 # 201 max ops per script!
        NUM_OUTPUTS = 50

        witness_program = CScript([OP_2DROP]*NUM_DROPS + [OP_TRUE])
        witness_hash = uint256_from_str(sha256(witness_program))
        scriptPubKey = CScript([OP_0, ser_uint256(witness_hash)])

        prevout = COutPoint(self.utxo[0].sha256, self.utxo[0].n)
        value = self.utxo[0].nValue

        parent_tx = CTransaction()
        parent_tx.vin.append(CTxIn(prevout, b""))
        child_value = int(value/NUM_OUTPUTS)
        for i in range(NUM_OUTPUTS):
            parent_tx.vout.append(CTxOut(child_value, scriptPubKey))
        parent_tx.vout[0].nValue -= 50000
        assert(parent_tx.vout[0].nValue > 0)
        parent_tx.rehash()

        child_tx = CTransaction()
        for i in range(NUM_OUTPUTS):
            child_tx.vin.append(CTxIn(COutPoint(parent_tx.sha256, i), b""))
        child_tx.vout = [CTxOut(value - 100000, CScript([OP_TRUE]))]
        for i in range(NUM_OUTPUTS):
            child_tx.wit.vtxinwit.append(CTxInWitness())
            child_tx.wit.vtxinwit[-1].scriptWitness.stack = [b'a'*195]*(2*NUM_DROPS) + [witness_program]
        child_tx.rehash()
        self.update_witness_block_with_transactions(block, [parent_tx, child_tx])

        vsize = get_virtual_size(block)
        additional_bytes = (MAX_BLOCK_BASE_SIZE - vsize)*4
        i = 0
        while additional_bytes > 0:
            # Add some more bytes to each input until we hit MAX_BLOCK_BASE_SIZE+1
            extra_bytes = min(additional_bytes+1, 55)
            block.vtx[-1].wit.vtxinwit[int(i/(2*NUM_DROPS))].scriptWitness.stack[i%(2*NUM_DROPS)] = b'a'*(195+extra_bytes)
            additional_bytes -= extra_bytes
            i += 1

        block.vtx[0].vout.pop()  # Remove old commitment
        add_witness_commitment(block)
        block.solve()
        vsize = get_virtual_size(block)
        assert_equal(vsize, MAX_BLOCK_BASE_SIZE + 1)
        # Make sure that our test case would exceed the old max-network-message
        # limit
        assert(len(block.serialize(True)) > 2*1024*1024)

        self.test_node.test_witness_block(block, accepted=False)

        # Now resize the second transaction to make the block fit.
        cur_length = len(block.vtx[-1].wit.vtxinwit[0].scriptWitness.stack[0])
        block.vtx[-1].wit.vtxinwit[0].scriptWitness.stack[0] = b'a'*(cur_length-1)
        block.vtx[0].vout.pop()
        add_witness_commitment(block)
        block.solve()
        assert(get_virtual_size(block) == MAX_BLOCK_BASE_SIZE)

        self.test_node.test_witness_block(block, accepted=True)

        # Update available utxo's
        self.utxo.pop(0)
        self.utxo.append(UTXO(block.vtx[-1].sha256, 0, block.vtx[-1].vout[0].nValue))


    # submitblock will try to add the nonce automatically, so that mining
    # software doesn't need to worry about doing so itself.
    def test_submit_block(self):
        block = self.build_next_block()

        # Try using a custom nonce and then don't supply it.
        # This shouldn't possibly work.
        add_witness_commitment(block, nonce=1)
        block.vtx[0].wit = CTxWitness() # drop the nonce
        block.solve()
        self.nodes[0].submitblock(bytes_to_hex_str(block.serialize(True)))
        assert(self.nodes[0].getbestblockhash() != block.hash)

        # Now redo commitment with the standard nonce, but let bitcoind fill it in.
        add_witness_commitment(block, nonce=0)
        block.vtx[0].wit = CTxWitness()
        block.solve()
        self.nodes[0].submitblock(bytes_to_hex_str(block.serialize(True)))
        assert_equal(self.nodes[0].getbestblockhash(), block.hash)

        # This time, add a tx with non-empty witness, but don't supply
        # the commitment.
        block_2 = self.build_next_block()

        add_witness_commitment(block_2)

        block_2.solve()

        # Drop commitment and nonce -- submitblock should not fill in.
        block_2.vtx[0].vout.pop()
        block_2.vtx[0].wit = CTxWitness()

        self.nodes[0].submitblock(bytes_to_hex_str(block_2.serialize(True)))
        # Tip should not advance!
        assert(self.nodes[0].getbestblockhash() != block_2.hash)


    # Consensus tests of extra witness data in a transaction.
    def test_extra_witness_data(self):
        self.log.info("Testing extra witness data in tx")

        assert(len(self.utxo) > 0)
        
        block = self.build_next_block()

        witness_program = CScript([OP_DROP, OP_TRUE])
        witness_hash = sha256(witness_program)
        scriptPubKey = CScript([OP_0, witness_hash])

        # First try extra witness data on a tx that doesn't require a witness
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))
        tx.vout.append(CTxOut(self.utxo[0].nValue-2000, scriptPubKey))
        tx.vout.append(CTxOut(1000, CScript([OP_TRUE]))) # non-witness output
        tx.wit.vtxinwit.append(CTxInWitness())
        tx.wit.vtxinwit[0].scriptWitness.stack = [CScript([])]
        tx.rehash()
        self.update_witness_block_with_transactions(block, [tx])

        # Extra witness data should not be allowed.
        self.test_node.test_witness_block(block, accepted=False)

        # Try extra signature data.  Ok if we're not spending a witness output.
        block.vtx[1].wit.vtxinwit = []
        block.vtx[1].vin[0].scriptSig = CScript([OP_0])
        block.vtx[1].rehash()
        add_witness_commitment(block)
        block.solve()

        self.test_node.test_witness_block(block, accepted=True)

        # Now try extra witness/signature data on an input that DOES require a
        # witness
        tx2 = CTransaction()
        tx2.vin.append(CTxIn(COutPoint(tx.sha256, 0), b"")) # witness output
        tx2.vin.append(CTxIn(COutPoint(tx.sha256, 1), b"")) # non-witness
        tx2.vout.append(CTxOut(tx.vout[0].nValue, CScript([OP_TRUE])))
        tx2.wit.vtxinwit.extend([CTxInWitness(), CTxInWitness()])
        tx2.wit.vtxinwit[0].scriptWitness.stack = [ CScript([CScriptNum(1)]), CScript([CScriptNum(1)]), witness_program ]
        tx2.wit.vtxinwit[1].scriptWitness.stack = [ CScript([OP_TRUE]) ]

        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx2])

        # This has extra witness data, so it should fail.
        self.test_node.test_witness_block(block, accepted=False)

        # Now get rid of the extra witness, but add extra scriptSig data
        tx2.vin[0].scriptSig = CScript([OP_TRUE])
        tx2.vin[1].scriptSig = CScript([OP_TRUE])
        tx2.wit.vtxinwit[0].scriptWitness.stack.pop(0)
        tx2.wit.vtxinwit[1].scriptWitness.stack = []
        tx2.rehash()
        add_witness_commitment(block)
        block.solve()

        # This has extra signature data for a witness input, so it should fail.
        self.test_node.test_witness_block(block, accepted=False)

        # Now get rid of the extra scriptsig on the witness input, and verify
        # success (even with extra scriptsig data in the non-witness input)
        tx2.vin[0].scriptSig = b""
        tx2.rehash()
        add_witness_commitment(block)
        block.solve()

        self.test_node.test_witness_block(block, accepted=True)

        # Update utxo for later tests
        self.utxo.pop(0)
        self.utxo.append(UTXO(tx2.sha256, 0, tx2.vout[0].nValue))


    def test_max_witness_push_length(self):
        ''' Should only allow up to 520 byte pushes in witness stack '''
        self.log.info("Testing maximum witness push size")
        MAX_SCRIPT_ELEMENT_SIZE = 520
        assert(len(self.utxo))

        block = self.build_next_block()

        witness_program = CScript([OP_DROP, OP_TRUE])
        witness_hash = sha256(witness_program)
        scriptPubKey = CScript([OP_0, witness_hash])

        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))
        tx.vout.append(CTxOut(self.utxo[0].nValue-1000, scriptPubKey))
        tx.rehash()

        tx2 = CTransaction()
        tx2.vin.append(CTxIn(COutPoint(tx.sha256, 0), b""))
        tx2.vout.append(CTxOut(tx.vout[0].nValue-1000, CScript([OP_TRUE])))
        tx2.wit.vtxinwit.append(CTxInWitness())
        # First try a 521-byte stack element
        tx2.wit.vtxinwit[0].scriptWitness.stack = [ b'a'*(MAX_SCRIPT_ELEMENT_SIZE+1), witness_program ]
        tx2.rehash()

        self.update_witness_block_with_transactions(block, [tx, tx2])
        self.test_node.test_witness_block(block, accepted=False)

        # Now reduce the length of the stack element
        tx2.wit.vtxinwit[0].scriptWitness.stack[0] = b'a'*(MAX_SCRIPT_ELEMENT_SIZE)

        add_witness_commitment(block)
        block.solve()
        self.test_node.test_witness_block(block, accepted=True)

        # Update the utxo for later tests
        self.utxo.pop()
        self.utxo.append(UTXO(tx2.sha256, 0, tx2.vout[0].nValue))

    def test_max_witness_program_length(self):
        # Can create witness outputs that are long, but can't be greater than
        # 10k bytes to successfully spend
        self.log.info("Testing maximum witness program length")
        assert(len(self.utxo))
        MAX_PROGRAM_LENGTH = 10000

        # This program is 19 max pushes (9937 bytes), then 64 more opcode-bytes.
        long_witness_program = CScript([b'a'*520]*19 + [OP_DROP]*63 + [OP_TRUE])
        assert(len(long_witness_program) == MAX_PROGRAM_LENGTH+1)
        long_witness_hash = sha256(long_witness_program)
        long_scriptPubKey = CScript([OP_0, long_witness_hash])

        block = self.build_next_block()

        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))
        tx.vout.append(CTxOut(self.utxo[0].nValue-1000, long_scriptPubKey))
        tx.rehash()

        tx2 = CTransaction()
        tx2.vin.append(CTxIn(COutPoint(tx.sha256, 0), b""))
        tx2.vout.append(CTxOut(tx.vout[0].nValue-1000, CScript([OP_TRUE])))
        tx2.wit.vtxinwit.append(CTxInWitness())
        tx2.wit.vtxinwit[0].scriptWitness.stack = [b'a']*44 + [long_witness_program]
        tx2.rehash()

        self.update_witness_block_with_transactions(block, [tx, tx2])

        self.test_node.test_witness_block(block, accepted=False)

        # Try again with one less byte in the witness program
        witness_program = CScript([b'a'*520]*19 + [OP_DROP]*62 + [OP_TRUE])
        assert(len(witness_program) == MAX_PROGRAM_LENGTH)
        witness_hash = sha256(witness_program)
        scriptPubKey = CScript([OP_0, witness_hash])

        tx.vout[0] = CTxOut(tx.vout[0].nValue, scriptPubKey)
        tx.rehash()
        tx2.vin[0].prevout.hash = tx.sha256
        tx2.wit.vtxinwit[0].scriptWitness.stack = [b'a']*43 + [witness_program]
        tx2.rehash()
        block.vtx = [block.vtx[0]]
        self.update_witness_block_with_transactions(block, [tx, tx2])
        self.test_node.test_witness_block(block, accepted=True)

        self.utxo.pop()
        self.utxo.append(UTXO(tx2.sha256, 0, tx2.vout[0].nValue))


    def test_witness_input_length(self):
        ''' Ensure that vin length must match vtxinwit length '''
        self.log.info("Testing witness input length")
        assert(len(self.utxo))

        witness_program = CScript([OP_DROP, OP_TRUE])
        witness_hash = sha256(witness_program)
        scriptPubKey = CScript([OP_0, witness_hash])
        
        # Create a transaction that splits our utxo into many outputs
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))
        nValue = self.utxo[0].nValue
        for i in range(10):
            tx.vout.append(CTxOut(int(nValue/10), scriptPubKey))
        tx.vout[0].nValue -= 1000
        assert(tx.vout[0].nValue >= 0)

        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx])
        self.test_node.test_witness_block(block, accepted=True)

        # Try various ways to spend tx that should all break.
        # This "broken" transaction serializer will not normalize
        # the length of vtxinwit.
        class BrokenCTransaction(CTransaction):
            def serialize_with_witness(self):
                flags = 0
                if not self.wit.is_null():
                    flags |= 1
                r = b""
                r += struct.pack("<i", self.nVersion)
                if flags:
                    dummy = []
                    r += ser_vector(dummy)
                    r += struct.pack("<B", flags)
                r += ser_vector(self.vin)
                r += ser_vector(self.vout)
                if flags & 1:
                    r += self.wit.serialize()
                r += struct.pack("<I", self.nLockTime)
                return r

        tx2 = BrokenCTransaction()
        for i in range(10):
            tx2.vin.append(CTxIn(COutPoint(tx.sha256, i), b""))
        tx2.vout.append(CTxOut(nValue-3000, CScript([OP_TRUE])))

        # First try using a too long vtxinwit
        for i in range(11):
            tx2.wit.vtxinwit.append(CTxInWitness())
            tx2.wit.vtxinwit[i].scriptWitness.stack = [b'a', witness_program]

        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx2])
        self.test_node.test_witness_block(block, accepted=False)

        # Now try using a too short vtxinwit
        tx2.wit.vtxinwit.pop()
        tx2.wit.vtxinwit.pop()

        block.vtx = [block.vtx[0]]
        self.update_witness_block_with_transactions(block, [tx2])
        self.test_node.test_witness_block(block, accepted=False)

        # Now make one of the intermediate witnesses be incorrect
        tx2.wit.vtxinwit.append(CTxInWitness())
        tx2.wit.vtxinwit[-1].scriptWitness.stack = [b'a', witness_program]
        tx2.wit.vtxinwit[5].scriptWitness.stack = [ witness_program ]

        block.vtx = [block.vtx[0]]
        self.update_witness_block_with_transactions(block, [tx2])
        self.test_node.test_witness_block(block, accepted=False)

        # Fix the broken witness and the block should be accepted.
        tx2.wit.vtxinwit[5].scriptWitness.stack = [b'a', witness_program]
        block.vtx = [block.vtx[0]]
        self.update_witness_block_with_transactions(block, [tx2])
        self.test_node.test_witness_block(block, accepted=True)

        self.utxo.pop()
        self.utxo.append(UTXO(tx2.sha256, 0, tx2.vout[0].nValue))


    def test_witness_tx_relay_before_segwit_activation(self):
        self.log.info("Testing relay of witness transactions")
        # Generate a transaction that doesn't require a witness, but send it
        # with a witness.  Should be rejected for premature-witness, but should
        # not be added to recently rejected list.
        assert(len(self.utxo))
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))
        tx.vout.append(CTxOut(self.utxo[0].nValue-1000, CScript([OP_TRUE])))
        tx.wit.vtxinwit.append(CTxInWitness())
        tx.wit.vtxinwit[0].scriptWitness.stack = [ b'a' ]
        tx.rehash()

        tx_hash = tx.sha256
        tx_value = tx.vout[0].nValue

        # Verify that if a peer doesn't set nServices to include NODE_WITNESS,
        # the getdata is just for the non-witness portion.
        self.old_node.announce_tx_and_wait_for_getdata(tx)
        assert(self.old_node.last_message["getdata"].inv[0].type == 1)

        # Since we haven't delivered the tx yet, inv'ing the same tx from
        # a witness transaction ought not result in a getdata.
        try:
            self.test_node.announce_tx_and_wait_for_getdata(tx, timeout=2)
            self.log.error("Error: duplicate tx getdata!")
            assert(False)
        except AssertionError as e:
            pass

        # Delivering this transaction with witness should fail (no matter who
        # its from)
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        assert_equal(len(self.nodes[1].getrawmempool()), 0)
        self.old_node.test_transaction_acceptance(tx, with_witness=True, accepted=False)
        self.test_node.test_transaction_acceptance(tx, with_witness=True, accepted=False)

        # But eliminating the witness should fix it
        self.test_node.test_transaction_acceptance(tx, with_witness=False, accepted=True)

        # Cleanup: mine the first transaction and update utxo
        self.nodes[0].generate(1)
        assert_equal(len(self.nodes[0].getrawmempool()),  0)

        self.utxo.pop(0)
        self.utxo.append(UTXO(tx_hash, 0, tx_value))


    # After segwit activates, verify that mempool:
    # - rejects transactions with unnecessary/extra witnesses
    # - accepts transactions with valid witnesses
    # and that witness transactions are relayed to non-upgraded peers.
    def test_tx_relay_after_segwit_activation(self):
        self.log.info("Testing relay of witness transactions")
        # Generate a transaction that doesn't require a witness, but send it
        # with a witness.  Should be rejected because we can't use a witness
        # when spending a non-witness output.
        assert(len(self.utxo))
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))
        tx.vout.append(CTxOut(self.utxo[0].nValue-1000, CScript([OP_TRUE])))
        tx.wit.vtxinwit.append(CTxInWitness())
        tx.wit.vtxinwit[0].scriptWitness.stack = [ b'a' ]
        tx.rehash()

        tx_hash = tx.sha256

        # Verify that unnecessary witnesses are rejected.
        self.test_node.announce_tx_and_wait_for_getdata(tx)
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        self.test_node.test_transaction_acceptance(tx, with_witness=True, accepted=False)

        # Verify that removing the witness succeeds.
        self.test_node.announce_tx_and_wait_for_getdata(tx)
        self.test_node.test_transaction_acceptance(tx, with_witness=False, accepted=True)

        # Now try to add extra witness data to a valid witness tx.
        witness_program = CScript([OP_TRUE])
        witness_hash = sha256(witness_program)
        scriptPubKey = CScript([OP_0, witness_hash])
        tx2 = CTransaction()
        tx2.vin.append(CTxIn(COutPoint(tx_hash, 0), b""))
        tx2.vout.append(CTxOut(tx.vout[0].nValue-1000, scriptPubKey))
        tx2.rehash()

        tx3 = CTransaction()
        tx3.vin.append(CTxIn(COutPoint(tx2.sha256, 0), b""))
        tx3.wit.vtxinwit.append(CTxInWitness())

        # Add too-large for IsStandard witness and check that it does not enter reject filter
        p2sh_program = CScript([OP_TRUE])
        p2sh_pubkey = hash160(p2sh_program)
        witness_program2 = CScript([b'a'*400000])
        tx3.vout.append(CTxOut(tx2.vout[0].nValue-1000, CScript([OP_HASH160, p2sh_pubkey, OP_EQUAL])))
        tx3.wit.vtxinwit[0].scriptWitness.stack = [witness_program2]
        tx3.rehash()

        # Node will not be blinded to the transaction
        self.std_node.announce_tx_and_wait_for_getdata(tx3)
        self.std_node.test_transaction_acceptance(tx3, True, False, b'tx-size')
        self.std_node.announce_tx_and_wait_for_getdata(tx3)
        self.std_node.test_transaction_acceptance(tx3, True, False, b'tx-size')

        # Remove witness stuffing, instead add extra witness push on stack
        tx3.vout[0] = CTxOut(tx2.vout[0].nValue-1000, CScript([OP_TRUE]))
        tx3.wit.vtxinwit[0].scriptWitness.stack = [CScript([CScriptNum(1)]), witness_program ]
        tx3.rehash()

        self.test_node.test_transaction_acceptance(tx2, with_witness=True, accepted=True)
        self.test_node.test_transaction_acceptance(tx3, with_witness=True, accepted=False)

        # Get rid of the extra witness, and verify acceptance.
        tx3.wit.vtxinwit[0].scriptWitness.stack = [ witness_program ]
        # Also check that old_node gets a tx announcement, even though this is
        # a witness transaction.
        self.old_node.wait_for_inv([CInv(1, tx2.sha256)]) # wait until tx2 was inv'ed
        self.test_node.test_transaction_acceptance(tx3, with_witness=True, accepted=True)
        self.old_node.wait_for_inv([CInv(1, tx3.sha256)])

        # Test that getrawtransaction returns correct witness information
        # hash, size, vsize
        raw_tx = self.nodes[0].getrawtransaction(tx3.hash, 1)
        assert_equal(int(raw_tx["hash"], 16), tx3.calc_sha256(True))
        assert_equal(raw_tx["size"], len(tx3.serialize_with_witness()))
        vsize = (len(tx3.serialize_with_witness()) + 3*len(tx3.serialize_without_witness()) + 3) / 4
        assert_equal(raw_tx["vsize"], vsize)
        assert_equal(len(raw_tx["vin"][0]["txinwitness"]), 1)
        assert_equal(raw_tx["vin"][0]["txinwitness"][0], hexlify(witness_program).decode('ascii'))
        assert(vsize != raw_tx["size"])

        # Cleanup: mine the transactions and update utxo for next test
        self.nodes[0].generate(1)
        assert_equal(len(self.nodes[0].getrawmempool()),  0)

        self.utxo.pop(0)
        self.utxo.append(UTXO(tx3.sha256, 0, tx3.vout[0].nValue))


    # Test that block requests to NODE_WITNESS peer are with MSG_WITNESS_FLAG
    # This is true regardless of segwit activation.
    # Also test that we don't ask for blocks from unupgraded peers
    def test_block_relay(self, segwit_activated):
        self.log.info("Testing block relay")

        blocktype = 2|MSG_WITNESS_FLAG

        # test_node has set NODE_WITNESS, so all getdata requests should be for
        # witness blocks.
        # Test announcing a block via inv results in a getdata, and that
        # announcing a version 4 or random VB block with a header results in a getdata
        block1 = self.build_next_block()
        block1.solve()

        self.test_node.announce_block_and_wait_for_getdata(block1, use_header=False)
        assert(self.test_node.last_message["getdata"].inv[0].type == blocktype)
        self.test_node.test_witness_block(block1, True)

        block2 = self.build_next_block(nVersion=4)
        block2.solve()

        self.test_node.announce_block_and_wait_for_getdata(block2, use_header=True)
        assert(self.test_node.last_message["getdata"].inv[0].type == blocktype)
        self.test_node.test_witness_block(block2, True)

        block3 = self.build_next_block(nVersion=(VB_TOP_BITS | (1<<15)))
        block3.solve()
        self.test_node.announce_block_and_wait_for_getdata(block3, use_header=True)
        assert(self.test_node.last_message["getdata"].inv[0].type == blocktype)
        self.test_node.test_witness_block(block3, True)

        # Check that we can getdata for witness blocks or regular blocks,
        # and the right thing happens.
        if segwit_activated == False:
            # Before activation, we should be able to request old blocks with
            # or without witness, and they should be the same.
            chain_height = self.nodes[0].getblockcount()
            # Pick 10 random blocks on main chain, and verify that getdata's
            # for MSG_BLOCK, MSG_WITNESS_BLOCK, and rpc getblock() are equal.
            all_heights = list(range(chain_height+1))
            random.shuffle(all_heights)
            all_heights = all_heights[0:10]
            for height in all_heights:
                block_hash = self.nodes[0].getblockhash(height)
                rpc_block = self.nodes[0].getblock(block_hash, False)
                block_hash = int(block_hash, 16)
                block = self.test_node.request_block(block_hash, 2)
                wit_block = self.test_node.request_block(block_hash, 2|MSG_WITNESS_FLAG)
                assert_equal(block.serialize(True), wit_block.serialize(True))
                assert_equal(block.serialize(), hex_str_to_bytes(rpc_block))
        else:
            # After activation, witness blocks and non-witness blocks should
            # be different.  Verify rpc getblock() returns witness blocks, while
            # getdata respects the requested type.
            block = self.build_next_block()
            self.update_witness_block_with_transactions(block, [])
            # This gives us a witness commitment.
            assert(len(block.vtx[0].wit.vtxinwit) == 1)
            assert(len(block.vtx[0].wit.vtxinwit[0].scriptWitness.stack) == 1)
            self.test_node.test_witness_block(block, accepted=True)
            # Now try to retrieve it...
            rpc_block = self.nodes[0].getblock(block.hash, False)
            non_wit_block = self.test_node.request_block(block.sha256, 2)
            wit_block = self.test_node.request_block(block.sha256, 2|MSG_WITNESS_FLAG)
            assert_equal(wit_block.serialize(True), hex_str_to_bytes(rpc_block))
            assert_equal(wit_block.serialize(False), non_wit_block.serialize())
            assert_equal(wit_block.serialize(True), block.serialize(True))

            # Test size, vsize, weight
            rpc_details = self.nodes[0].getblock(block.hash, True)
            assert_equal(rpc_details["size"], len(block.serialize(True)))
            assert_equal(rpc_details["strippedsize"], len(block.serialize(False)))
            weight = 3*len(block.serialize(False)) + len(block.serialize(True))
            assert_equal(rpc_details["weight"], weight)

            # Upgraded node should not ask for blocks from unupgraded
            block4 = self.build_next_block(nVersion=4)
            block4.solve()
            self.old_node.getdataset = set()

            # Blocks can be requested via direct-fetch (immediately upon processing the announcement)
            # or via parallel download (with an indeterminate delay from processing the announcement)
            # so to test that a block is NOT requested, we could guess a time period to sleep for,
            # and then check. We can avoid the sleep() by taking advantage of transaction getdata's
            # being processed after block getdata's, and announce a transaction as well,
            # and then check to see if that particular getdata has been received.
            # Since 0.14, inv's will only be responded to with a getheaders, so send a header
            # to announce this block.
            msg = msg_headers()
            msg.headers = [ CBlockHeader(block4) ]
            self.old_node.send_message(msg)
            self.old_node.announce_tx_and_wait_for_getdata(block4.vtx[0])
            assert(block4.sha256 not in self.old_node.getdataset)

    # V0 segwit outputs should be standard after activation, but not before.
    def test_standardness_v0(self, segwit_activated):
        self.log.info("Testing standardness of v0 outputs (%s activation)" % ("after" if segwit_activated else "before"))
        assert(len(self.utxo))

        witness_program = CScript([OP_TRUE])
        witness_hash = sha256(witness_program)
        scriptPubKey = CScript([OP_0, witness_hash])

        p2sh_pubkey = hash160(witness_program)
        p2sh_scriptPubKey = CScript([OP_HASH160, p2sh_pubkey, OP_EQUAL])

        # First prepare a p2sh output (so that spending it will pass standardness)
        p2sh_tx = CTransaction()
        p2sh_tx.vin = [CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b"")]
        p2sh_tx.vout = [CTxOut(self.utxo[0].nValue-1000, p2sh_scriptPubKey)]
        p2sh_tx.rehash()

        # Mine it on test_node to create the confirmed output.
        self.test_node.test_transaction_acceptance(p2sh_tx, with_witness=True, accepted=True)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        # Now test standardness of v0 P2WSH outputs.
        # Start by creating a transaction with two outputs.
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(p2sh_tx.sha256, 0), CScript([witness_program]))]
        tx.vout = [CTxOut(p2sh_tx.vout[0].nValue-10000, scriptPubKey)]
        tx.vout.append(CTxOut(8000, scriptPubKey)) # Might burn this later
        tx.rehash()

        self.std_node.test_transaction_acceptance(tx, with_witness=True, accepted=segwit_activated)

        # Now create something that looks like a P2PKH output. This won't be spendable.
        scriptPubKey = CScript([OP_0, hash160(witness_hash)])
        tx2 = CTransaction()
        if segwit_activated:
            # if tx was accepted, then we spend the second output.
            tx2.vin = [CTxIn(COutPoint(tx.sha256, 1), b"")]
            tx2.vout = [CTxOut(7000, scriptPubKey)]
            tx2.wit.vtxinwit.append(CTxInWitness())
            tx2.wit.vtxinwit[0].scriptWitness.stack = [witness_program]
        else:
            # if tx wasn't accepted, we just re-spend the p2sh output we started with.
            tx2.vin = [CTxIn(COutPoint(p2sh_tx.sha256, 0), CScript([witness_program]))]
            tx2.vout = [CTxOut(p2sh_tx.vout[0].nValue-1000, scriptPubKey)]
        tx2.rehash()

        self.std_node.test_transaction_acceptance(tx2, with_witness=True, accepted=segwit_activated)

        # Now update self.utxo for later tests.
        tx3 = CTransaction()
        if segwit_activated:
            # tx and tx2 were both accepted.  Don't bother trying to reclaim the
            # P2PKH output; just send tx's first output back to an anyone-can-spend.
            sync_mempools([self.nodes[0], self.nodes[1]])
            tx3.vin = [CTxIn(COutPoint(tx.sha256, 0), b"")]
            tx3.vout = [CTxOut(tx.vout[0].nValue-1000, CScript([OP_TRUE]))]
            tx3.wit.vtxinwit.append(CTxInWitness())
            tx3.wit.vtxinwit[0].scriptWitness.stack = [witness_program]
            tx3.rehash()
            self.test_node.test_transaction_acceptance(tx3, with_witness=True, accepted=True)
        else:
            # tx and tx2 didn't go anywhere; just clean up the p2sh_tx output.
            tx3.vin = [CTxIn(COutPoint(p2sh_tx.sha256, 0), CScript([witness_program]))]
            tx3.vout = [CTxOut(p2sh_tx.vout[0].nValue-1000, witness_program)]
            tx3.rehash()
            self.test_node.test_transaction_acceptance(tx3, with_witness=True, accepted=True)

        self.nodes[0].generate(1)
        sync_blocks(self.nodes)
        self.utxo.pop(0)
        self.utxo.append(UTXO(tx3.sha256, 0, tx3.vout[0].nValue))
        assert_equal(len(self.nodes[1].getrawmempool()), 0)


    # Verify that future segwit upgraded transactions are non-standard,
    # but valid in blocks. Can run this before and after segwit activation.
    def test_segwit_versions(self):
        self.log.info("Testing standardness/consensus for segwit versions (0-16)")
        assert(len(self.utxo))
        NUM_TESTS = 17 # will test OP_0, OP1, ..., OP_16
        if (len(self.utxo) < NUM_TESTS):
            tx = CTransaction()
            tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))
            split_value = (self.utxo[0].nValue - 4000) // NUM_TESTS
            for i in range(NUM_TESTS):
                tx.vout.append(CTxOut(split_value, CScript([OP_TRUE])))
            tx.rehash()
            block = self.build_next_block()
            self.update_witness_block_with_transactions(block, [tx])
            self.test_node.test_witness_block(block, accepted=True)
            self.utxo.pop(0)
            for i in range(NUM_TESTS):
                self.utxo.append(UTXO(tx.sha256, i, split_value))

        sync_blocks(self.nodes)
        temp_utxo = []
        tx = CTransaction()
        count = 0
        witness_program = CScript([OP_TRUE])
        witness_hash = sha256(witness_program)
        assert_equal(len(self.nodes[1].getrawmempool()), 0)
        for version in list(range(OP_1, OP_16+1)) + [OP_0]:
            count += 1
            # First try to spend to a future version segwit scriptPubKey.
            scriptPubKey = CScript([CScriptOp(version), witness_hash])
            tx.vin = [CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b"")]
            tx.vout = [CTxOut(self.utxo[0].nValue-1000, scriptPubKey)]
            tx.rehash()
            self.std_node.test_transaction_acceptance(tx, with_witness=True, accepted=False)
            self.test_node.test_transaction_acceptance(tx, with_witness=True, accepted=True)
            self.utxo.pop(0)
            temp_utxo.append(UTXO(tx.sha256, 0, tx.vout[0].nValue))

        self.nodes[0].generate(1) # Mine all the transactions
        sync_blocks(self.nodes)
        assert(len(self.nodes[0].getrawmempool()) == 0)

        # Finally, verify that version 0 -> version 1 transactions
        # are non-standard
        scriptPubKey = CScript([CScriptOp(OP_1), witness_hash])
        tx2 = CTransaction()
        tx2.vin = [CTxIn(COutPoint(tx.sha256, 0), b"")]
        tx2.vout = [CTxOut(tx.vout[0].nValue-1000, scriptPubKey)]
        tx2.wit.vtxinwit.append(CTxInWitness())
        tx2.wit.vtxinwit[0].scriptWitness.stack = [ witness_program ]
        tx2.rehash()
        # Gets accepted to test_node, because standardness of outputs isn't
        # checked with fRequireStandard
        self.test_node.test_transaction_acceptance(tx2, with_witness=True, accepted=True)
        self.std_node.test_transaction_acceptance(tx2, with_witness=True, accepted=False)
        temp_utxo.pop() # last entry in temp_utxo was the output we just spent
        temp_utxo.append(UTXO(tx2.sha256, 0, tx2.vout[0].nValue))

        # Spend everything in temp_utxo back to an OP_TRUE output.
        tx3 = CTransaction()
        total_value = 0
        for i in temp_utxo:
            tx3.vin.append(CTxIn(COutPoint(i.sha256, i.n), b""))
            tx3.wit.vtxinwit.append(CTxInWitness())
            total_value += i.nValue
        tx3.wit.vtxinwit[-1].scriptWitness.stack = [witness_program]
        tx3.vout.append(CTxOut(total_value - 1000, CScript([OP_TRUE])))
        tx3.rehash()
        # Spending a higher version witness output is not allowed by policy,
        # even with fRequireStandard=false.
        self.test_node.test_transaction_acceptance(tx3, with_witness=True, accepted=False)
        self.test_node.sync_with_ping()
        with mininode_lock:
            assert(b"reserved for soft-fork upgrades" in self.test_node.last_message["reject"].reason)

        # Building a block with the transaction must be valid, however.
        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx2, tx3])
        self.test_node.test_witness_block(block, accepted=True)
        sync_blocks(self.nodes)

        # Add utxo to our list
        self.utxo.append(UTXO(tx3.sha256, 0, tx3.vout[0].nValue))


    def test_premature_coinbase_witness_spend(self):
        self.log.info("Testing premature coinbase witness spend")
        block = self.build_next_block()
        # Change the output of the block to be a witness output.
        witness_program = CScript([OP_TRUE])
        witness_hash = sha256(witness_program)
        scriptPubKey = CScript([OP_0, witness_hash])
        block.vtx[0].vout[0].scriptPubKey = scriptPubKey
        # This next line will rehash the coinbase and update the merkle
        # root, and solve.
        self.update_witness_block_with_transactions(block, [])
        self.test_node.test_witness_block(block, accepted=True)

        spend_tx = CTransaction()
        spend_tx.vin = [CTxIn(COutPoint(block.vtx[0].sha256, 0), b"")]
        spend_tx.vout = [CTxOut(block.vtx[0].vout[0].nValue, witness_program)]
        spend_tx.wit.vtxinwit.append(CTxInWitness())
        spend_tx.wit.vtxinwit[0].scriptWitness.stack = [ witness_program ]
        spend_tx.rehash()

        # Now test a premature spend.
        self.nodes[0].generate(98)
        sync_blocks(self.nodes)
        block2 = self.build_next_block()
        self.update_witness_block_with_transactions(block2, [spend_tx])
        self.test_node.test_witness_block(block2, accepted=False)

        # Advancing one more block should allow the spend.
        self.nodes[0].generate(1)
        block2 = self.build_next_block()
        self.update_witness_block_with_transactions(block2, [spend_tx])
        self.test_node.test_witness_block(block2, accepted=True)
        sync_blocks(self.nodes)


    def test_signature_version_1(self):
        self.log.info("Testing segwit signature hash version 1")
        key = CECKey()
        key.set_secretbytes(b"9")
        pubkey = CPubKey(key.get_pubkey())

        witness_program = CScript([pubkey, CScriptOp(OP_CHECKSIG)])
        witness_hash = sha256(witness_program)
        scriptPubKey = CScript([OP_0, witness_hash])

        # First create a witness output for use in the tests.
        assert(len(self.utxo))
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))
        tx.vout.append(CTxOut(self.utxo[0].nValue-1000, scriptPubKey))
        tx.rehash()

        self.test_node.test_transaction_acceptance(tx, with_witness=True, accepted=True)
        # Mine this transaction in preparation for following tests.
        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx])
        self.test_node.test_witness_block(block, accepted=True)
        sync_blocks(self.nodes)
        self.utxo.pop(0)

        # Test each hashtype
        prev_utxo = UTXO(tx.sha256, 0, tx.vout[0].nValue)
        for sigflag in [ 0, SIGHASH_ANYONECANPAY ]:
            for hashtype in [SIGHASH_ALL, SIGHASH_NONE, SIGHASH_SINGLE]:
                hashtype |= sigflag
                block = self.build_next_block()
                tx = CTransaction()
                tx.vin.append(CTxIn(COutPoint(prev_utxo.sha256, prev_utxo.n), b""))
                tx.vout.append(CTxOut(prev_utxo.nValue - 1000, scriptPubKey))
                tx.wit.vtxinwit.append(CTxInWitness())
                # Too-large input value
                sign_P2PK_witness_input(witness_program, tx, 0, hashtype, prev_utxo.nValue+1, key)
                self.update_witness_block_with_transactions(block, [tx])
                self.test_node.test_witness_block(block, accepted=False)

                # Too-small input value
                sign_P2PK_witness_input(witness_program, tx, 0, hashtype, prev_utxo.nValue-1, key)
                block.vtx.pop() # remove last tx
                self.update_witness_block_with_transactions(block, [tx])
                self.test_node.test_witness_block(block, accepted=False)

                # Now try correct value
                sign_P2PK_witness_input(witness_program, tx, 0, hashtype, prev_utxo.nValue, key)
                block.vtx.pop()
                self.update_witness_block_with_transactions(block, [tx])
                self.test_node.test_witness_block(block, accepted=True)

                prev_utxo = UTXO(tx.sha256, 0, tx.vout[0].nValue)

        # Test combinations of signature hashes.
        # Split the utxo into a lot of outputs.
        # Randomly choose up to 10 to spend, sign with different hashtypes, and
        # output to a random number of outputs.  Repeat NUM_TESTS times.
        # Ensure that we've tested a situation where we use SIGHASH_SINGLE with
        # an input index > number of outputs.
        NUM_TESTS = 500
        temp_utxos = []
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(prev_utxo.sha256, prev_utxo.n), b""))
        split_value = prev_utxo.nValue // NUM_TESTS
        for i in range(NUM_TESTS):
            tx.vout.append(CTxOut(split_value, scriptPubKey))
        tx.wit.vtxinwit.append(CTxInWitness())
        sign_P2PK_witness_input(witness_program, tx, 0, SIGHASH_ALL, prev_utxo.nValue, key)
        for i in range(NUM_TESTS):
            temp_utxos.append(UTXO(tx.sha256, i, split_value))

        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx])
        self.test_node.test_witness_block(block, accepted=True)

        block = self.build_next_block()
        used_sighash_single_out_of_bounds = False
        for i in range(NUM_TESTS):
            # Ping regularly to keep the connection alive
            if (not i % 100):
                self.test_node.sync_with_ping()
            # Choose random number of inputs to use.
            num_inputs = random.randint(1, 10)
            # Create a slight bias for producing more utxos
            num_outputs = random.randint(1, 11)
            random.shuffle(temp_utxos)
            assert(len(temp_utxos) > num_inputs)
            tx = CTransaction()
            total_value = 0
            for i in range(num_inputs):
                tx.vin.append(CTxIn(COutPoint(temp_utxos[i].sha256, temp_utxos[i].n), b""))
                tx.wit.vtxinwit.append(CTxInWitness())
                total_value += temp_utxos[i].nValue
            split_value = total_value // num_outputs
            for i in range(num_outputs):
                tx.vout.append(CTxOut(split_value, scriptPubKey))
            for i in range(num_inputs):
                # Now try to sign each input, using a random hashtype.
                anyonecanpay = 0
                if random.randint(0, 1):
                    anyonecanpay = SIGHASH_ANYONECANPAY
                hashtype = random.randint(1, 3) | anyonecanpay
                sign_P2PK_witness_input(witness_program, tx, i, hashtype, temp_utxos[i].nValue, key)
                if (hashtype == SIGHASH_SINGLE and i >= num_outputs):
                    used_sighash_single_out_of_bounds = True
            tx.rehash()
            for i in range(num_outputs):
                temp_utxos.append(UTXO(tx.sha256, i, split_value))
            temp_utxos = temp_utxos[num_inputs:]

            block.vtx.append(tx)

            # Test the block periodically, if we're close to maxblocksize
            if (get_virtual_size(block) > MAX_BLOCK_BASE_SIZE - 1000):
                self.update_witness_block_with_transactions(block, [])
                self.test_node.test_witness_block(block, accepted=True)
                block = self.build_next_block()

        if (not used_sighash_single_out_of_bounds):
            self.log.info("WARNING: this test run didn't attempt SIGHASH_SINGLE with out-of-bounds index value")
        # Test the transactions we've added to the block
        if (len(block.vtx) > 1):
            self.update_witness_block_with_transactions(block, [])
            self.test_node.test_witness_block(block, accepted=True)

        # Now test witness version 0 P2PKH transactions
        pubkeyhash = hash160(pubkey)
        scriptPKH = CScript([OP_0, pubkeyhash])
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(temp_utxos[0].sha256, temp_utxos[0].n), b""))
        tx.vout.append(CTxOut(temp_utxos[0].nValue, scriptPKH))
        tx.wit.vtxinwit.append(CTxInWitness())
        sign_P2PK_witness_input(witness_program, tx, 0, SIGHASH_ALL, temp_utxos[0].nValue, key)
        tx2 = CTransaction()
        tx2.vin.append(CTxIn(COutPoint(tx.sha256, 0), b""))
        tx2.vout.append(CTxOut(tx.vout[0].nValue, CScript([OP_TRUE])))

        script = GetP2PKHScript(pubkeyhash)
        sig_hash = SegwitVersion1SignatureHash(script, tx2, 0, SIGHASH_ALL, tx.vout[0].nValue)
        signature = key.sign(sig_hash) + b'\x01' # 0x1 is SIGHASH_ALL

        # Check that we can't have a scriptSig
        tx2.vin[0].scriptSig = CScript([signature, pubkey])
        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx, tx2])
        self.test_node.test_witness_block(block, accepted=False)

        # Move the signature to the witness.
        block.vtx.pop()
        tx2.wit.vtxinwit.append(CTxInWitness())
        tx2.wit.vtxinwit[0].scriptWitness.stack = [signature, pubkey]
        tx2.vin[0].scriptSig = b""
        tx2.rehash()

        self.update_witness_block_with_transactions(block, [tx2])
        self.test_node.test_witness_block(block, accepted=True)

        temp_utxos.pop(0)

        # Update self.utxos for later tests. Just spend everything in
        # temp_utxos to a corresponding entry in self.utxos
        tx = CTransaction()
        index = 0
        for i in temp_utxos:
            # Just spend to our usual anyone-can-spend output
            # Use SIGHASH_SINGLE|SIGHASH_ANYONECANPAY so we can build up
            # the signatures as we go.
            tx.vin.append(CTxIn(COutPoint(i.sha256, i.n), b""))
            tx.vout.append(CTxOut(i.nValue, CScript([OP_TRUE])))
            tx.wit.vtxinwit.append(CTxInWitness())
            sign_P2PK_witness_input(witness_program, tx, index, SIGHASH_SINGLE|SIGHASH_ANYONECANPAY, i.nValue, key)
            index += 1
        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx])
        self.test_node.test_witness_block(block, accepted=True)

        for i in range(len(tx.vout)):
            self.utxo.append(UTXO(tx.sha256, i, tx.vout[i].nValue))


    # Test P2SH wrapped witness programs.
    def test_p2sh_witness(self, segwit_activated):
        self.log.info("Testing P2SH witness transactions")

        assert(len(self.utxo))

        # Prepare the p2sh-wrapped witness output
        witness_program = CScript([OP_DROP, OP_TRUE])
        witness_hash = sha256(witness_program)
        p2wsh_pubkey = CScript([OP_0, witness_hash])
        p2sh_witness_hash = hash160(p2wsh_pubkey)
        scriptPubKey = CScript([OP_HASH160, p2sh_witness_hash, OP_EQUAL])
        scriptSig = CScript([p2wsh_pubkey]) # a push of the redeem script

        # Fund the P2SH output
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))
        tx.vout.append(CTxOut(self.utxo[0].nValue-1000, scriptPubKey))
        tx.rehash()

        # Verify mempool acceptance and block validity
        self.test_node.test_transaction_acceptance(tx, with_witness=False, accepted=True)
        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx])
        self.test_node.test_witness_block(block, accepted=True, with_witness=segwit_activated)
        sync_blocks(self.nodes)

        # Now test attempts to spend the output.
        spend_tx = CTransaction()
        spend_tx.vin.append(CTxIn(COutPoint(tx.sha256, 0), scriptSig))
        spend_tx.vout.append(CTxOut(tx.vout[0].nValue-1000, CScript([OP_TRUE])))
        spend_tx.rehash()

        # This transaction should not be accepted into the mempool pre- or
        # post-segwit.  Mempool acceptance will use SCRIPT_VERIFY_WITNESS which
        # will require a witness to spend a witness program regardless of
        # segwit activation.  Note that older bitcoind's that are not
        # segwit-aware would also reject this for failing CLEANSTACK.
        self.test_node.test_transaction_acceptance(spend_tx, with_witness=False, accepted=False)

        # Try to put the witness script in the scriptSig, should also fail.
        spend_tx.vin[0].scriptSig = CScript([p2wsh_pubkey, b'a'])
        spend_tx.rehash()
        self.test_node.test_transaction_acceptance(spend_tx, with_witness=False, accepted=False)

        # Now put the witness script in the witness, should succeed after
        # segwit activates.
        spend_tx.vin[0].scriptSig = scriptSig
        spend_tx.rehash()
        spend_tx.wit.vtxinwit.append(CTxInWitness())
        spend_tx.wit.vtxinwit[0].scriptWitness.stack = [ b'a', witness_program ]

        # Verify mempool acceptance
        self.test_node.test_transaction_acceptance(spend_tx, with_witness=True, accepted=segwit_activated)
        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [spend_tx])

        # If we're before activation, then sending this without witnesses
        # should be valid.  If we're after activation, then sending this with
        # witnesses should be valid.
        if segwit_activated:
            self.test_node.test_witness_block(block, accepted=True)
        else:
            self.test_node.test_witness_block(block, accepted=True, with_witness=False)

        # Update self.utxo
        self.utxo.pop(0)
        self.utxo.append(UTXO(spend_tx.sha256, 0, spend_tx.vout[0].nValue))

    # Test the behavior of starting up a segwit-aware node after the softfork
    # has activated.  As segwit requires different block data than pre-segwit
    # nodes would have stored, this requires special handling.
    # To enable this test, pass --oldbinary=<path-to-pre-segwit-bitcoind> to
    # the test.
    def test_upgrade_after_activation(self, node_id):
        self.log.info("Testing software upgrade after softfork activation")

        assert(node_id != 0) # node0 is assumed to be a segwit-active bitcoind

        # Make sure the nodes are all up
        sync_blocks(self.nodes)

        # Restart with the new binary
        self.stop_node(node_id)
        self.nodes[node_id] = self.start_node(node_id, self.options.tmpdir)
        connect_nodes(self.nodes[0], node_id)

        sync_blocks(self.nodes)

        # Make sure that this peer thinks segwit has activated.
        assert(get_bip9_status(self.nodes[node_id], 'segwit')['status'] == "active")

        # Make sure this peers blocks match those of node0.
        height = self.nodes[node_id].getblockcount()
        while height >= 0:
            block_hash = self.nodes[node_id].getblockhash(height)
            assert_equal(block_hash, self.nodes[0].getblockhash(height))
            assert_equal(self.nodes[0].getblock(block_hash), self.nodes[node_id].getblock(block_hash))
            height -= 1


    def test_witness_sigops(self):
        '''Ensure sigop counting is correct inside witnesses.'''
        self.log.info("Testing sigops limit")

        assert(len(self.utxo))

        # Keep this under MAX_OPS_PER_SCRIPT (201)
        witness_program = CScript([OP_TRUE, OP_IF, OP_TRUE, OP_ELSE] + [OP_CHECKMULTISIG]*5 + [OP_CHECKSIG]*193 + [OP_ENDIF])
        witness_hash = sha256(witness_program)
        scriptPubKey = CScript([OP_0, witness_hash])

        sigops_per_script = 20*5 + 193*1
        # We'll produce 2 extra outputs, one with a program that would take us
        # over max sig ops, and one with a program that would exactly reach max
        # sig ops
        outputs = (MAX_SIGOP_COST // sigops_per_script) + 2
        extra_sigops_available = MAX_SIGOP_COST % sigops_per_script

        # We chose the number of checkmultisigs/checksigs to make this work:
        assert(extra_sigops_available < 100) # steer clear of MAX_OPS_PER_SCRIPT

        # This script, when spent with the first
        # N(=MAX_SIGOP_COST//sigops_per_script) outputs of our transaction,
        # would push us just over the block sigop limit.
        witness_program_toomany = CScript([OP_TRUE, OP_IF, OP_TRUE, OP_ELSE] + [OP_CHECKSIG]*(extra_sigops_available + 1) + [OP_ENDIF])
        witness_hash_toomany = sha256(witness_program_toomany)
        scriptPubKey_toomany = CScript([OP_0, witness_hash_toomany])

        # If we spend this script instead, we would exactly reach our sigop
        # limit (for witness sigops).
        witness_program_justright = CScript([OP_TRUE, OP_IF, OP_TRUE, OP_ELSE] + [OP_CHECKSIG]*(extra_sigops_available) + [OP_ENDIF])
        witness_hash_justright = sha256(witness_program_justright)
        scriptPubKey_justright = CScript([OP_0, witness_hash_justright])

        # First split our available utxo into a bunch of outputs
        split_value = self.utxo[0].nValue // outputs
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))
        for i in range(outputs):
            tx.vout.append(CTxOut(split_value, scriptPubKey))
        tx.vout[-2].scriptPubKey = scriptPubKey_toomany
        tx.vout[-1].scriptPubKey = scriptPubKey_justright
        tx.rehash()

        block_1 = self.build_next_block()
        self.update_witness_block_with_transactions(block_1, [tx])
        self.test_node.test_witness_block(block_1, accepted=True)

        tx2 = CTransaction()
        # If we try to spend the first n-1 outputs from tx, that should be
        # too many sigops.
        total_value = 0
        for i in range(outputs-1):
            tx2.vin.append(CTxIn(COutPoint(tx.sha256, i), b""))
            tx2.wit.vtxinwit.append(CTxInWitness())
            tx2.wit.vtxinwit[-1].scriptWitness.stack = [ witness_program ]
            total_value += tx.vout[i].nValue
        tx2.wit.vtxinwit[-1].scriptWitness.stack = [ witness_program_toomany ] 
        tx2.vout.append(CTxOut(total_value, CScript([OP_TRUE])))
        tx2.rehash()

        block_2 = self.build_next_block()
        self.update_witness_block_with_transactions(block_2, [tx2])
        self.test_node.test_witness_block(block_2, accepted=False)

        # Try dropping the last input in tx2, and add an output that has
        # too many sigops (contributing to legacy sigop count).
        checksig_count = (extra_sigops_available // 4) + 1
        scriptPubKey_checksigs = CScript([OP_CHECKSIG]*checksig_count)
        tx2.vout.append(CTxOut(0, scriptPubKey_checksigs))
        tx2.vin.pop()
        tx2.wit.vtxinwit.pop()
        tx2.vout[0].nValue -= tx.vout[-2].nValue
        tx2.rehash()
        block_3 = self.build_next_block()
        self.update_witness_block_with_transactions(block_3, [tx2])
        self.test_node.test_witness_block(block_3, accepted=False)

        # If we drop the last checksig in this output, the tx should succeed.
        block_4 = self.build_next_block()
        tx2.vout[-1].scriptPubKey = CScript([OP_CHECKSIG]*(checksig_count-1))
        tx2.rehash()
        self.update_witness_block_with_transactions(block_4, [tx2])
        self.test_node.test_witness_block(block_4, accepted=True)

        # Reset the tip back down for the next test
        sync_blocks(self.nodes)
        for x in self.nodes:
            x.invalidateblock(block_4.hash)

        # Try replacing the last input of tx2 to be spending the last
        # output of tx
        block_5 = self.build_next_block()
        tx2.vout.pop()
        tx2.vin.append(CTxIn(COutPoint(tx.sha256, outputs-1), b""))
        tx2.wit.vtxinwit.append(CTxInWitness())
        tx2.wit.vtxinwit[-1].scriptWitness.stack = [ witness_program_justright ]
        tx2.rehash()
        self.update_witness_block_with_transactions(block_5, [tx2])
        self.test_node.test_witness_block(block_5, accepted=True)

        # TODO: test p2sh sigop counting

    def test_getblocktemplate_before_lockin(self):
        self.log.info("Testing getblocktemplate setting of segwit versionbit (before lockin)")
        # Node0 is segwit aware, node2 is not.
        for node in [self.nodes[0], self.nodes[2]]:
            gbt_results = node.getblocktemplate()
            block_version = gbt_results['version']
            # If we're not indicating segwit support, we will still be
            # signalling for segwit activation.
            assert_equal((block_version & (1 << VB_WITNESS_BIT) != 0), node == self.nodes[0])
            # If we don't specify the segwit rule, then we won't get a default
            # commitment.
            assert('default_witness_commitment' not in gbt_results)

        # Workaround:
        # Can either change the tip, or change the mempool and wait 5 seconds
        # to trigger a recomputation of getblocktemplate.
        txid = int(self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1), 16)
        # Using mocktime lets us avoid sleep()
        sync_mempools(self.nodes)
        self.nodes[0].setmocktime(int(time.time())+10)
        self.nodes[2].setmocktime(int(time.time())+10)

        for node in [self.nodes[0], self.nodes[2]]:
            gbt_results = node.getblocktemplate({"rules" : ["segwit"]})
            block_version = gbt_results['version']
            if node == self.nodes[2]:
                # If this is a non-segwit node, we should still not get a witness
                # commitment, nor a version bit signalling segwit.
                assert_equal(block_version & (1 << VB_WITNESS_BIT), 0)
                assert('default_witness_commitment' not in gbt_results)
            else:
                # For segwit-aware nodes, check the version bit and the witness
                # commitment are correct.
                assert(block_version & (1 << VB_WITNESS_BIT) != 0)
                assert('default_witness_commitment' in gbt_results)
                witness_commitment = gbt_results['default_witness_commitment']

                # Check that default_witness_commitment is present.
                witness_root = CBlock.get_merkle_root([ser_uint256(0),
                                                       ser_uint256(txid)])
                script = get_witness_script(witness_root, 0)
                assert_equal(witness_commitment, bytes_to_hex_str(script))

        # undo mocktime
        self.nodes[0].setmocktime(0)
        self.nodes[2].setmocktime(0)

    # Uncompressed pubkeys are no longer supported in default relay policy,
    # but (for now) are still valid in blocks.
    def test_uncompressed_pubkey(self):
        self.log.info("Testing uncompressed pubkeys")
        # Segwit transactions using uncompressed pubkeys are not accepted
        # under default policy, but should still pass consensus.
        key = CECKey()
        key.set_secretbytes(b"9")
        key.set_compressed(False)
        pubkey = CPubKey(key.get_pubkey())
        assert_equal(len(pubkey), 65) # This should be an uncompressed pubkey

        assert(len(self.utxo) > 0)
        utxo = self.utxo.pop(0)

        # Test 1: P2WPKH
        # First create a P2WPKH output that uses an uncompressed pubkey
        pubkeyhash = hash160(pubkey)
        scriptPKH = CScript([OP_0, pubkeyhash])
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(utxo.sha256, utxo.n), b""))
        tx.vout.append(CTxOut(utxo.nValue-1000, scriptPKH))
        tx.rehash()

        # Confirm it in a block.
        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx])
        self.test_node.test_witness_block(block, accepted=True)

        # Now try to spend it. Send it to a P2WSH output, which we'll
        # use in the next test.
        witness_program = CScript([pubkey, CScriptOp(OP_CHECKSIG)])
        witness_hash = sha256(witness_program)
        scriptWSH = CScript([OP_0, witness_hash])

        tx2 = CTransaction()
        tx2.vin.append(CTxIn(COutPoint(tx.sha256, 0), b""))
        tx2.vout.append(CTxOut(tx.vout[0].nValue-1000, scriptWSH))
        script = GetP2PKHScript(pubkeyhash)
        sig_hash = SegwitVersion1SignatureHash(script, tx2, 0, SIGHASH_ALL, tx.vout[0].nValue)
        signature = key.sign(sig_hash) + b'\x01' # 0x1 is SIGHASH_ALL
        tx2.wit.vtxinwit.append(CTxInWitness())
        tx2.wit.vtxinwit[0].scriptWitness.stack = [ signature, pubkey ]
        tx2.rehash()

        # Should fail policy test.
        self.test_node.test_transaction_acceptance(tx2, True, False, b'non-mandatory-script-verify-flag (Using non-compressed keys in segwit)')
        # But passes consensus.
        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx2])
        self.test_node.test_witness_block(block, accepted=True)

        # Test 2: P2WSH
        # Try to spend the P2WSH output created in last test.
        # Send it to a P2SH(P2WSH) output, which we'll use in the next test.
        p2sh_witness_hash = hash160(scriptWSH)
        scriptP2SH = CScript([OP_HASH160, p2sh_witness_hash, OP_EQUAL])
        scriptSig = CScript([scriptWSH])

        tx3 = CTransaction()
        tx3.vin.append(CTxIn(COutPoint(tx2.sha256, 0), b""))
        tx3.vout.append(CTxOut(tx2.vout[0].nValue-1000, scriptP2SH))
        tx3.wit.vtxinwit.append(CTxInWitness())
        sign_P2PK_witness_input(witness_program, tx3, 0, SIGHASH_ALL, tx2.vout[0].nValue, key)

        # Should fail policy test.
        self.test_node.test_transaction_acceptance(tx3, True, False, b'non-mandatory-script-verify-flag (Using non-compressed keys in segwit)')
        # But passes consensus.
        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx3])
        self.test_node.test_witness_block(block, accepted=True)

        # Test 3: P2SH(P2WSH)
        # Try to spend the P2SH output created in the last test.
        # Send it to a P2PKH output, which we'll use in the next test.
        scriptPubKey = GetP2PKHScript(pubkeyhash)
        tx4 = CTransaction()
        tx4.vin.append(CTxIn(COutPoint(tx3.sha256, 0), scriptSig))
        tx4.vout.append(CTxOut(tx3.vout[0].nValue-1000, scriptPubKey))
        tx4.wit.vtxinwit.append(CTxInWitness())
        sign_P2PK_witness_input(witness_program, tx4, 0, SIGHASH_ALL, tx3.vout[0].nValue, key)

        # Should fail policy test.
        self.test_node.test_transaction_acceptance(tx4, True, False, b'non-mandatory-script-verify-flag (Using non-compressed keys in segwit)')
        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx4])
        self.test_node.test_witness_block(block, accepted=True)

        # Test 4: Uncompressed pubkeys should still be valid in non-segwit
        # transactions.
        tx5 = CTransaction()
        tx5.vin.append(CTxIn(COutPoint(tx4.sha256, 0), b""))
        tx5.vout.append(CTxOut(tx4.vout[0].nValue-1000, CScript([OP_TRUE])))
        (sig_hash, err) = SignatureHash(scriptPubKey, tx5, 0, SIGHASH_ALL)
        signature = key.sign(sig_hash) + b'\x01' # 0x1 is SIGHASH_ALL
        tx5.vin[0].scriptSig = CScript([signature, pubkey])
        tx5.rehash()
        # Should pass policy and consensus.
        self.test_node.test_transaction_acceptance(tx5, True, True)
        block = self.build_next_block()
        self.update_witness_block_with_transactions(block, [tx5])
        self.test_node.test_witness_block(block, accepted=True)
        self.utxo.append(UTXO(tx5.sha256, 0, tx5.vout[0].nValue))

    def test_non_standard_witness(self):
        self.log.info("Testing detection of non-standard P2WSH witness")
        pad = chr(1).encode('latin-1')

        # Create scripts for tests
        scripts = []
        scripts.append(CScript([OP_DROP] * 100))
        scripts.append(CScript([OP_DROP] * 99))
        scripts.append(CScript([pad * 59] * 59 + [OP_DROP] * 60))
        scripts.append(CScript([pad * 59] * 59 + [OP_DROP] * 61))

        p2wsh_scripts = []

        assert(len(self.utxo))
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.utxo[0].sha256, self.utxo[0].n), b""))

        # For each script, generate a pair of P2WSH and P2SH-P2WSH output.
        outputvalue = (self.utxo[0].nValue - 1000) // (len(scripts) * 2)
        for i in scripts:
            p2wsh = CScript([OP_0, sha256(i)])
            p2sh = hash160(p2wsh)
            p2wsh_scripts.append(p2wsh)
            tx.vout.append(CTxOut(outputvalue, p2wsh))
            tx.vout.append(CTxOut(outputvalue, CScript([OP_HASH160, p2sh, OP_EQUAL])))
        tx.rehash()
        txid = tx.sha256
        self.test_node.test_transaction_acceptance(tx, with_witness=False, accepted=True)

        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        # Creating transactions for tests
        p2wsh_txs = []
        p2sh_txs = []
        for i in range(len(scripts)):
            p2wsh_tx = CTransaction()
            p2wsh_tx.vin.append(CTxIn(COutPoint(txid,i*2)))
            p2wsh_tx.vout.append(CTxOut(outputvalue - 5000, CScript([OP_0, hash160(hex_str_to_bytes(""))])))
            p2wsh_tx.wit.vtxinwit.append(CTxInWitness())
            p2wsh_tx.rehash()
            p2wsh_txs.append(p2wsh_tx)
            p2sh_tx = CTransaction()
            p2sh_tx.vin.append(CTxIn(COutPoint(txid,i*2+1), CScript([p2wsh_scripts[i]])))
            p2sh_tx.vout.append(CTxOut(outputvalue - 5000, CScript([OP_0, hash160(hex_str_to_bytes(""))])))
            p2sh_tx.wit.vtxinwit.append(CTxInWitness())
            p2sh_tx.rehash()
            p2sh_txs.append(p2sh_tx)

        # Testing native P2WSH
        # Witness stack size, excluding witnessScript, over 100 is non-standard
        p2wsh_txs[0].wit.vtxinwit[0].scriptWitness.stack = [pad] * 101 + [scripts[0]]
        self.std_node.test_transaction_acceptance(p2wsh_txs[0], True, False, b'bad-witness-nonstandard')
        # Non-standard nodes should accept
        self.test_node.test_transaction_acceptance(p2wsh_txs[0], True, True)

        # Stack element size over 80 bytes is non-standard
        p2wsh_txs[1].wit.vtxinwit[0].scriptWitness.stack = [pad * 81] * 100 + [scripts[1]]
        self.std_node.test_transaction_acceptance(p2wsh_txs[1], True, False, b'bad-witness-nonstandard')
        # Non-standard nodes should accept
        self.test_node.test_transaction_acceptance(p2wsh_txs[1], True, True)
        # Standard nodes should accept if element size is not over 80 bytes
        p2wsh_txs[1].wit.vtxinwit[0].scriptWitness.stack = [pad * 80] * 100 + [scripts[1]]
        self.std_node.test_transaction_acceptance(p2wsh_txs[1], True, True)

        # witnessScript size at 3600 bytes is standard
        p2wsh_txs[2].wit.vtxinwit[0].scriptWitness.stack = [pad, pad, scripts[2]]
        self.test_node.test_transaction_acceptance(p2wsh_txs[2], True, True)
        self.std_node.test_transaction_acceptance(p2wsh_txs[2], True, True)

        # witnessScript size at 3601 bytes is non-standard
        p2wsh_txs[3].wit.vtxinwit[0].scriptWitness.stack = [pad, pad, pad, scripts[3]]
        self.std_node.test_transaction_acceptance(p2wsh_txs[3], True, False, b'bad-witness-nonstandard')
        # Non-standard nodes should accept
        self.test_node.test_transaction_acceptance(p2wsh_txs[3], True, True)

        # Repeating the same tests with P2SH-P2WSH
        p2sh_txs[0].wit.vtxinwit[0].scriptWitness.stack = [pad] * 101 + [scripts[0]]
        self.std_node.test_transaction_acceptance(p2sh_txs[0], True, False, b'bad-witness-nonstandard')
        self.test_node.test_transaction_acceptance(p2sh_txs[0], True, True)
        p2sh_txs[1].wit.vtxinwit[0].scriptWitness.stack = [pad * 81] * 100 + [scripts[1]]
        self.std_node.test_transaction_acceptance(p2sh_txs[1], True, False, b'bad-witness-nonstandard')
        self.test_node.test_transaction_acceptance(p2sh_txs[1], True, True)
        p2sh_txs[1].wit.vtxinwit[0].scriptWitness.stack = [pad * 80] * 100 + [scripts[1]]
        self.std_node.test_transaction_acceptance(p2sh_txs[1], True, True)
        p2sh_txs[2].wit.vtxinwit[0].scriptWitness.stack = [pad, pad, scripts[2]]
        self.test_node.test_transaction_acceptance(p2sh_txs[2], True, True)
        self.std_node.test_transaction_acceptance(p2sh_txs[2], True, True)
        p2sh_txs[3].wit.vtxinwit[0].scriptWitness.stack = [pad, pad, pad, scripts[3]]
        self.std_node.test_transaction_acceptance(p2sh_txs[3], True, False, b'bad-witness-nonstandard')
        self.test_node.test_transaction_acceptance(p2sh_txs[3], True, True)

        self.nodes[0].generate(1)  # Mine and clean up the mempool of non-standard node
        # Valid but non-standard transactions in a block should be accepted by standard node
        sync_blocks(self.nodes)
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        assert_equal(len(self.nodes[1].getrawmempool()), 0)

        self.utxo.pop(0)


    def run_test(self):
        # Setup the p2p connections and start up the network thread.
        self.test_node = TestNode() # sets NODE_WITNESS|NODE_NETWORK
        self.old_node = TestNode()  # only NODE_NETWORK
        self.std_node = TestNode() # for testing node1 (fRequireStandard=true)

        self.p2p_connections = [self.test_node, self.old_node]

        self.connections = []
        self.connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], self.test_node, services=NODE_NETWORK|NODE_WITNESS))
        self.connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], self.old_node, services=NODE_NETWORK))
        self.connections.append(NodeConn('127.0.0.1', p2p_port(1), self.nodes[1], self.std_node, services=NODE_NETWORK|NODE_WITNESS))
        self.test_node.add_connection(self.connections[0])
        self.old_node.add_connection(self.connections[1])
        self.std_node.add_connection(self.connections[2])

        NetworkThread().start() # Start up network handling in another thread

        # Keep a place to store utxo's that can be used in later tests
        self.utxo = []

        # Test logic begins here
        self.test_node.wait_for_verack()

        self.log.info("Starting tests before segwit lock in:")

        self.test_witness_services() # Verifies NODE_WITNESS
        self.test_non_witness_transaction() # non-witness tx's are accepted
        self.test_unnecessary_witness_before_segwit_activation()
        self.test_block_relay(segwit_activated=False)

        # Advance to segwit being 'started'
        self.advance_to_segwit_started()
        sync_blocks(self.nodes)
        self.test_getblocktemplate_before_lockin()

        sync_blocks(self.nodes)

        # At lockin, nothing should change.
        self.log.info("Testing behavior post lockin, pre-activation")
        self.advance_to_segwit_lockin()

        # Retest unnecessary witnesses
        self.test_unnecessary_witness_before_segwit_activation()
        self.test_witness_tx_relay_before_segwit_activation()
        self.test_block_relay(segwit_activated=False)
        self.test_p2sh_witness(segwit_activated=False)
        self.test_standardness_v0(segwit_activated=False)

        sync_blocks(self.nodes)

        # Now activate segwit
        self.log.info("Testing behavior after segwit activation")
        self.advance_to_segwit_active()

        sync_blocks(self.nodes)

        # Test P2SH witness handling again
        self.test_p2sh_witness(segwit_activated=True)
        self.test_witness_commitments()
        self.test_block_malleability()
        self.test_witness_block_size()
        self.test_submit_block()
        self.test_extra_witness_data()
        self.test_max_witness_push_length()
        self.test_max_witness_program_length()
        self.test_witness_input_length()
        self.test_block_relay(segwit_activated=True)
        self.test_tx_relay_after_segwit_activation()
        self.test_standardness_v0(segwit_activated=True)
        self.test_segwit_versions()
        self.test_premature_coinbase_witness_spend()
        self.test_uncompressed_pubkey()
        self.test_signature_version_1()
        self.test_non_standard_witness()
        sync_blocks(self.nodes)
        self.test_upgrade_after_activation(node_id=2)
        self.test_witness_sigops()


if __name__ == '__main__':
    SegWitTest().main()
