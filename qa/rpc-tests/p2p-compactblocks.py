#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.blocktools import create_block, create_coinbase
from test_framework.siphash import siphash256
from test_framework.script import CScript, OP_TRUE

'''
CompactBlocksTest -- test compact blocks (BIP 152)
'''


# TestNode: A peer we use to send messages to bitcoind, and store responses.
class TestNode(SingleNodeConnCB):
    def __init__(self):
        SingleNodeConnCB.__init__(self)
        self.last_sendcmpct = None
        self.last_headers = None
        self.last_inv = None
        self.last_cmpctblock = None
        self.block_announced = False
        self.last_getdata = None
        self.last_getblocktxn = None
        self.last_block = None
        self.last_blocktxn = None

    def on_sendcmpct(self, conn, message):
        self.last_sendcmpct = message

    def on_block(self, conn, message):
        self.last_block = message

    def on_cmpctblock(self, conn, message):
        self.last_cmpctblock = message
        self.block_announced = True

    def on_headers(self, conn, message):
        self.last_headers = message
        self.block_announced = True

    def on_inv(self, conn, message):
        self.last_inv = message
        self.block_announced = True

    def on_getdata(self, conn, message):
        self.last_getdata = message

    def on_getblocktxn(self, conn, message):
        self.last_getblocktxn = message

    def on_blocktxn(self, conn, message):
        self.last_blocktxn = message

    # Requires caller to hold mininode_lock
    def received_block_announcement(self):
        return self.block_announced

    def clear_block_announcement(self):
        with mininode_lock:
            self.block_announced = False
            self.last_inv = None
            self.last_headers = None
            self.last_cmpctblock = None

    def get_headers(self, locator, hashstop):
        msg = msg_getheaders()
        msg.locator.vHave = locator
        msg.hashstop = hashstop
        self.connection.send_message(msg)

    def send_header_for_blocks(self, new_blocks):
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in new_blocks]
        self.send_message(headers_message)

    def request_headers_and_sync(self, locator, hashstop=0):
        self.clear_block_announcement()
        self.get_headers(locator, hashstop)
        assert(wait_until(self.received_block_announcement, timeout=30))
        assert(self.received_block_announcement())
        self.clear_block_announcement()


class CompactBlocksTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.utxos = []

    def setup_network(self):
        self.nodes = []

        # Turn off segwit in this test, as compact blocks don't currently work
        # with segwit.  (After BIP 152 is updated to support segwit, we can
        # test behavior with and without segwit enabled by adding a second node
        # to the test.)
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [["-debug", "-logtimemicros=1", "-bip9params=segwit:0:0"]])

    def build_block_on_tip(self):
        height = self.nodes[0].getblockcount()
        tip = self.nodes[0].getbestblockhash()
        mtp = self.nodes[0].getblockheader(tip)['mediantime']
        block = create_block(int(tip, 16), create_coinbase(height + 1), mtp + 1)
        block.solve()
        return block

    # Create 10 more anyone-can-spend utxo's for testing.
    def make_utxos(self):
        block = self.build_block_on_tip()
        self.test_node.send_and_ping(msg_block(block))
        assert(int(self.nodes[0].getbestblockhash(), 16) == block.sha256)
        self.nodes[0].generate(100)

        total_value = block.vtx[0].vout[0].nValue
        out_value = total_value // 10
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(block.vtx[0].sha256, 0), b''))
        for i in range(10):
            tx.vout.append(CTxOut(out_value, CScript([OP_TRUE])))
        tx.rehash()

        block2 = self.build_block_on_tip()
        block2.vtx.append(tx)
        block2.hashMerkleRoot = block2.calc_merkle_root()
        block2.solve()
        self.test_node.send_and_ping(msg_block(block2))
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block2.sha256)
        self.utxos.extend([[tx.sha256, i, out_value] for i in range(10)])
        return

    # Test "sendcmpct":
    # - No compact block announcements or getdata(MSG_CMPCT_BLOCK) unless
    #   sendcmpct is sent.
    # - If sendcmpct is sent with version > 1, the message is ignored.
    # - If sendcmpct is sent with boolean 0, then block announcements are not
    #   made with compact blocks.
    # - If sendcmpct is then sent with boolean 1, then new block announcements
    #   are made with compact blocks.
    def test_sendcmpct(self):
        print("Testing SENDCMPCT p2p message... ")

        # Make sure we get a version 0 SENDCMPCT message from our peer
        def received_sendcmpct():
            return (self.test_node.last_sendcmpct is not None)
        got_message = wait_until(received_sendcmpct, timeout=30)
        assert(received_sendcmpct())
        assert(got_message)
        assert_equal(self.test_node.last_sendcmpct.version, 1)

        tip = int(self.nodes[0].getbestblockhash(), 16)

        def check_announcement_of_new_block(node, peer, predicate):
            peer.clear_block_announcement()
            node.generate(1)
            got_message = wait_until(lambda: peer.block_announced, timeout=30)
            assert(peer.block_announced)
            assert(got_message)
            with mininode_lock:
                assert(predicate(peer))

        # We shouldn't get any block announcements via cmpctblock yet.
        check_announcement_of_new_block(self.nodes[0], self.test_node, lambda p: p.last_cmpctblock is None)

        # Try one more time, this time after requesting headers.
        self.test_node.request_headers_and_sync(locator=[tip])
        check_announcement_of_new_block(self.nodes[0], self.test_node, lambda p: p.last_cmpctblock is None and p.last_inv is not None)

        # Test a few ways of using sendcmpct that should NOT
        # result in compact block announcements.
        # Before each test, sync the headers chain.
        self.test_node.request_headers_and_sync(locator=[tip])

        # Now try a SENDCMPCT message with too-high version
        sendcmpct = msg_sendcmpct()
        sendcmpct.version = 2
        self.test_node.send_and_ping(sendcmpct)
        check_announcement_of_new_block(self.nodes[0], self.test_node, lambda p: p.last_cmpctblock is None)

        # Headers sync before next test.
        self.test_node.request_headers_and_sync(locator=[tip])

        # Now try a SENDCMPCT message with valid version, but announce=False
        self.test_node.send_and_ping(msg_sendcmpct())
        check_announcement_of_new_block(self.nodes[0], self.test_node, lambda p: p.last_cmpctblock is None)

        # Headers sync before next test.
        self.test_node.request_headers_and_sync(locator=[tip])

        # Finally, try a SENDCMPCT message with announce=True
        sendcmpct.version = 1
        sendcmpct.announce = True
        self.test_node.send_and_ping(sendcmpct)
        check_announcement_of_new_block(self.nodes[0], self.test_node, lambda p: p.last_cmpctblock is not None)

        # Try one more time (no headers sync should be needed!)
        check_announcement_of_new_block(self.nodes[0], self.test_node, lambda p: p.last_cmpctblock is not None)

        # Try one more time, after turning on sendheaders
        self.test_node.send_and_ping(msg_sendheaders())
        check_announcement_of_new_block(self.nodes[0], self.test_node, lambda p: p.last_cmpctblock is not None)

        # Now turn off announcements
        sendcmpct.announce = False
        self.test_node.send_and_ping(sendcmpct)
        check_announcement_of_new_block(self.nodes[0], self.test_node, lambda p: p.last_cmpctblock is None and p.last_headers is not None)

    # This test actually causes bitcoind to (reasonably!) disconnect us, so do this last.
    def test_invalid_cmpctblock_message(self):
        print("Testing invalid index in cmpctblock message...")
        self.nodes[0].generate(101)
        block = self.build_block_on_tip()

        cmpct_block = P2PHeaderAndShortIDs()
        cmpct_block.header = CBlockHeader(block)
        cmpct_block.prefilled_txn_length = 1
        # This index will be too high
        prefilled_txn = PrefilledTransaction(1, block.vtx[0])
        cmpct_block.prefilled_txn = [prefilled_txn]
        self.test_node.send_and_ping(msg_cmpctblock(cmpct_block))
        assert(int(self.nodes[0].getbestblockhash(), 16) == block.hashPrevBlock)

    # Compare the generated shortids to what we expect based on BIP 152, given
    # bitcoind's choice of nonce.
    def test_compactblock_construction(self):
        print("Testing compactblock headers and shortIDs are correct...")

        # Generate a bunch of transactions.
        self.nodes[0].generate(101)
        num_transactions = 25
        address = self.nodes[0].getnewaddress()
        for i in range(num_transactions):
            self.nodes[0].sendtoaddress(address, 0.1)

        self.test_node.sync_with_ping()

        # Now mine a block, and look at the resulting compact block.
        self.test_node.clear_block_announcement()
        block_hash = int(self.nodes[0].generate(1)[0], 16)

        # Store the raw block in our internal format.
        block = FromHex(CBlock(), self.nodes[0].getblock("%02x" % block_hash, False))
        [tx.calc_sha256() for tx in block.vtx]
        block.rehash()

        # Don't care which type of announcement came back for this test; just
        # request the compact block if we didn't get one yet.
        wait_until(self.test_node.received_block_announcement, timeout=30)

        with mininode_lock:
            if self.test_node.last_cmpctblock is None:
                self.test_node.clear_block_announcement()
                inv = CInv(4, block_hash)  # 4 == "CompactBlock"
                self.test_node.send_message(msg_getdata([inv]))

        wait_until(self.test_node.received_block_announcement, timeout=30)

        # Now we should have the compactblock
        header_and_shortids = None
        with mininode_lock:
            assert(self.test_node.last_cmpctblock is not None)
            # Convert the on-the-wire representation to absolute indexes
            header_and_shortids = HeaderAndShortIDs(self.test_node.last_cmpctblock.header_and_shortids)

        # Check that we got the right block!
        header_and_shortids.header.calc_sha256()
        assert_equal(header_and_shortids.header.sha256, block_hash)

        # Make sure the prefilled_txn appears to have included the coinbase
        assert(len(header_and_shortids.prefilled_txn) >= 1)
        assert_equal(header_and_shortids.prefilled_txn[0].index, 0)

        # Check that all prefilled_txn entries match what's in the block.
        for entry in header_and_shortids.prefilled_txn:
            entry.tx.calc_sha256()
            assert_equal(entry.tx.sha256, block.vtx[entry.index].sha256)

        # Check that the cmpctblock message announced all the transactions.
        assert_equal(len(header_and_shortids.prefilled_txn) + len(header_and_shortids.shortids), len(block.vtx))

        # And now check that all the shortids are as expected as well.
        # Determine the siphash keys to use.
        [k0, k1] = header_and_shortids.get_siphash_keys()

        index = 0
        while index < len(block.vtx):
            if (len(header_and_shortids.prefilled_txn) > 0 and
                    header_and_shortids.prefilled_txn[0].index == index):
                # Already checked prefilled transactions above
                header_and_shortids.prefilled_txn.pop(0)
            else:
                shortid = calculate_shortid(k0, k1, block.vtx[index].sha256)
                assert_equal(shortid, header_and_shortids.shortids[0])
                header_and_shortids.shortids.pop(0)
            index += 1

    # Test that bitcoind requests compact blocks when we announce new blocks
    # via header or inv, and that responding to getblocktxn causes the block
    # to be successfully reconstructed.
    def test_compactblock_requests(self):
        print("Testing compactblock requests... ")

        # Try announcing a block with an inv or header, expect a compactblock
        # request
        for announce in ["inv", "header"]:
            block = self.build_block_on_tip()
            with mininode_lock:
                self.test_node.last_getdata = None

            if announce == "inv":
                self.test_node.send_message(msg_inv([CInv(2, block.sha256)]))
            else:
                self.test_node.send_header_for_blocks([block])
            success = wait_until(lambda: self.test_node.last_getdata is not None, timeout=30)
            assert(success)
            assert_equal(len(self.test_node.last_getdata.inv), 1)
            assert_equal(self.test_node.last_getdata.inv[0].type, 4)
            assert_equal(self.test_node.last_getdata.inv[0].hash, block.sha256)

            # Send back a compactblock message that omits the coinbase
            comp_block = HeaderAndShortIDs()
            comp_block.header = CBlockHeader(block)
            comp_block.nonce = 0
            comp_block.shortids = [1]  # this is useless, and wrong
            self.test_node.send_and_ping(msg_cmpctblock(comp_block.to_p2p()))
            assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.hashPrevBlock)
            # Expect a getblocktxn message.
            with mininode_lock:
                assert(self.test_node.last_getblocktxn is not None)
                absolute_indexes = self.test_node.last_getblocktxn.block_txn_request.to_absolute()
            assert_equal(absolute_indexes, [0])  # should be a coinbase request

            # Send the coinbase, and verify that the tip advances.
            msg = msg_blocktxn()
            msg.block_transactions.blockhash = block.sha256
            msg.block_transactions.transactions = [block.vtx[0]]
            self.test_node.send_and_ping(msg)
            assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.sha256)

    # Create a chain of transactions from given utxo, and add to a new block.
    def build_block_with_transactions(self, utxo, num_transactions):
        block = self.build_block_on_tip()

        for i in range(num_transactions):
            tx = CTransaction()
            tx.vin.append(CTxIn(COutPoint(utxo[0], utxo[1]), b''))
            tx.vout.append(CTxOut(utxo[2] - 1000, CScript([OP_TRUE])))
            tx.rehash()
            utxo = [tx.sha256, 0, tx.vout[0].nValue]
            block.vtx.append(tx)

        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        return block

    # Test that we only receive getblocktxn requests for transactions that the
    # node needs, and that responding to them causes the block to be
    # reconstructed.
    def test_getblocktxn_requests(self):
        print("Testing getblocktxn requests...")

        # First try announcing compactblocks that won't reconstruct, and verify
        # that we receive getblocktxn messages back.
        utxo = self.utxos.pop(0)

        block = self.build_block_with_transactions(utxo, 5)
        self.utxos.append([block.vtx[-1].sha256, 0, block.vtx[-1].vout[0].nValue])

        comp_block = HeaderAndShortIDs()
        comp_block.initialize_from_block(block)

        self.test_node.send_and_ping(msg_cmpctblock(comp_block.to_p2p()))
        with mininode_lock:
            assert(self.test_node.last_getblocktxn is not None)
            absolute_indexes = self.test_node.last_getblocktxn.block_txn_request.to_absolute()
        assert_equal(absolute_indexes, [1, 2, 3, 4, 5])
        msg = msg_blocktxn()
        msg.block_transactions = BlockTransactions(block.sha256, block.vtx[1:])
        self.test_node.send_and_ping(msg)
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.sha256)

        utxo = self.utxos.pop(0)
        block = self.build_block_with_transactions(utxo, 5)
        self.utxos.append([block.vtx[-1].sha256, 0, block.vtx[-1].vout[0].nValue])

        # Now try interspersing the prefilled transactions
        comp_block.initialize_from_block(block, prefill_list=[0, 1, 5])
        self.test_node.send_and_ping(msg_cmpctblock(comp_block.to_p2p()))
        with mininode_lock:
            assert(self.test_node.last_getblocktxn is not None)
            absolute_indexes = self.test_node.last_getblocktxn.block_txn_request.to_absolute()
        assert_equal(absolute_indexes, [2, 3, 4])
        msg.block_transactions = BlockTransactions(block.sha256, block.vtx[2:5])
        self.test_node.send_and_ping(msg)
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.sha256)

        # Now try giving one transaction ahead of time.
        utxo = self.utxos.pop(0)
        block = self.build_block_with_transactions(utxo, 5)
        self.utxos.append([block.vtx[-1].sha256, 0, block.vtx[-1].vout[0].nValue])
        self.test_node.send_and_ping(msg_tx(block.vtx[1]))
        assert(block.vtx[1].hash in self.nodes[0].getrawmempool())

        # Prefill 4 out of the 6 transactions, and verify that only the one
        # that was not in the mempool is requested.
        comp_block.initialize_from_block(block, prefill_list=[0, 2, 3, 4])
        self.test_node.send_and_ping(msg_cmpctblock(comp_block.to_p2p()))
        with mininode_lock:
            assert(self.test_node.last_getblocktxn is not None)
            absolute_indexes = self.test_node.last_getblocktxn.block_txn_request.to_absolute()
        assert_equal(absolute_indexes, [5])

        msg.block_transactions = BlockTransactions(block.sha256, [block.vtx[5]])
        self.test_node.send_and_ping(msg)
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.sha256)

        # Now provide all transactions to the node before the block is
        # announced and verify reconstruction happens immediately.
        utxo = self.utxos.pop(0)
        block = self.build_block_with_transactions(utxo, 10)
        self.utxos.append([block.vtx[-1].sha256, 0, block.vtx[-1].vout[0].nValue])
        for tx in block.vtx[1:]:
            self.test_node.send_message(msg_tx(tx))
        self.test_node.sync_with_ping()
        # Make sure all transactions were accepted.
        mempool = self.nodes[0].getrawmempool()
        for tx in block.vtx[1:]:
            assert(tx.hash in mempool)

        # Clear out last request.
        with mininode_lock:
            self.test_node.last_getblocktxn = None

        # Send compact block
        comp_block.initialize_from_block(block, prefill_list=[0])
        self.test_node.send_and_ping(msg_cmpctblock(comp_block.to_p2p()))
        with mininode_lock:
            # Shouldn't have gotten a request for any transaction
            assert(self.test_node.last_getblocktxn is None)
        # Tip should have updated
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.sha256)

    # Incorrectly responding to a getblocktxn shouldn't cause the block to be
    # permanently failed.
    def test_incorrect_blocktxn_response(self):
        print("Testing handling of incorrect blocktxn responses...")

        if (len(self.utxos) == 0):
            self.make_utxos()
        utxo = self.utxos.pop(0)

        block = self.build_block_with_transactions(utxo, 10)
        self.utxos.append([block.vtx[-1].sha256, 0, block.vtx[-1].vout[0].nValue])
        # Relay the first 5 transactions from the block in advance
        for tx in block.vtx[1:6]:
            self.test_node.send_message(msg_tx(tx))
        self.test_node.sync_with_ping()
        # Make sure all transactions were accepted.
        mempool = self.nodes[0].getrawmempool()
        for tx in block.vtx[1:6]:
            assert(tx.hash in mempool)

        # Send compact block
        comp_block = HeaderAndShortIDs()
        comp_block.initialize_from_block(block, prefill_list=[0])
        self.test_node.send_and_ping(msg_cmpctblock(comp_block.to_p2p()))
        absolute_indexes = []
        with mininode_lock:
            assert(self.test_node.last_getblocktxn is not None)
            absolute_indexes = self.test_node.last_getblocktxn.block_txn_request.to_absolute()
        assert_equal(absolute_indexes, [6, 7, 8, 9, 10])

        # Now give an incorrect response.
        # Note that it's possible for bitcoind to be smart enough to know we're
        # lying, since it could check to see if the shortid matches what we're
        # sending, and eg disconnect us for misbehavior.  If that behavior
        # change were made, we could just modify this test by having a
        # different peer provide the block further down, so that we're still
        # verifying that the block isn't marked bad permanently. This is good
        # enough for now.
        msg = msg_blocktxn()
        msg.block_transactions = BlockTransactions(block.sha256, [block.vtx[5]] + block.vtx[7:])
        self.test_node.send_and_ping(msg)

        # Tip should not have updated
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.hashPrevBlock)

        # We should receive a getdata request
        success = wait_until(lambda: self.test_node.last_getdata is not None, timeout=10)
        assert(success)
        assert_equal(len(self.test_node.last_getdata.inv), 1)
        assert_equal(self.test_node.last_getdata.inv[0].type, 2)
        assert_equal(self.test_node.last_getdata.inv[0].hash, block.sha256)

        # Deliver the block
        self.test_node.send_and_ping(msg_block(block))
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.sha256)

    def test_getblocktxn_handler(self):
        print("Testing getblocktxn handler...")

        # bitcoind won't respond for blocks whose height is more than 15 blocks
        # deep.
        MAX_GETBLOCKTXN_DEPTH = 15
        chain_height = self.nodes[0].getblockcount()
        current_height = chain_height
        while (current_height >= chain_height - MAX_GETBLOCKTXN_DEPTH):
            block_hash = self.nodes[0].getblockhash(current_height)
            block = FromHex(CBlock(), self.nodes[0].getblock(block_hash, False))

            msg = msg_getblocktxn()
            msg.block_txn_request = BlockTransactionsRequest(int(block_hash, 16), [])
            num_to_request = random.randint(1, len(block.vtx))
            msg.block_txn_request.from_absolute(sorted(random.sample(range(len(block.vtx)), num_to_request)))
            self.test_node.send_message(msg)
            success = wait_until(lambda: self.test_node.last_blocktxn is not None, timeout=10)
            assert(success)

            [tx.calc_sha256() for tx in block.vtx]
            with mininode_lock:
                assert_equal(self.test_node.last_blocktxn.block_transactions.blockhash, int(block_hash, 16))
                all_indices = msg.block_txn_request.to_absolute()
                for index in all_indices:
                    tx = self.test_node.last_blocktxn.block_transactions.transactions.pop(0)
                    tx.calc_sha256()
                    assert_equal(tx.sha256, block.vtx[index].sha256)
                self.test_node.last_blocktxn = None
            current_height -= 1

        # Next request should be ignored, as we're past the allowed depth.
        block_hash = self.nodes[0].getblockhash(current_height)
        msg.block_txn_request = BlockTransactionsRequest(int(block_hash, 16), [0])
        self.test_node.send_and_ping(msg)
        with mininode_lock:
            assert_equal(self.test_node.last_blocktxn, None)

    def test_compactblocks_not_at_tip(self):
        print("Testing compactblock requests/announcements not at chain tip...")

        # Test that requesting old compactblocks doesn't work.
        MAX_CMPCTBLOCK_DEPTH = 11
        new_blocks = []
        for i in range(MAX_CMPCTBLOCK_DEPTH):
            self.test_node.clear_block_announcement()
            new_blocks.append(self.nodes[0].generate(1)[0])
            wait_until(self.test_node.received_block_announcement, timeout=30)

        self.test_node.clear_block_announcement()
        self.test_node.send_message(msg_getdata([CInv(4, int(new_blocks[0], 16))]))
        success = wait_until(lambda: self.test_node.last_cmpctblock is not None, timeout=30)
        assert(success)

        self.test_node.clear_block_announcement()
        self.nodes[0].generate(1)
        wait_until(self.test_node.received_block_announcement, timeout=30)
        self.test_node.clear_block_announcement()
        self.test_node.send_message(msg_getdata([CInv(4, int(new_blocks[0], 16))]))
        success = wait_until(lambda: self.test_node.last_block is not None, timeout=30)
        assert(success)
        with mininode_lock:
            self.test_node.last_block.block.calc_sha256()
            assert_equal(self.test_node.last_block.block.sha256, int(new_blocks[0], 16))

        # Generate an old compactblock, and verify that it's not accepted.
        cur_height = self.nodes[0].getblockcount()
        hashPrevBlock = int(self.nodes[0].getblockhash(cur_height-5), 16)
        block = self.build_block_on_tip()
        block.hashPrevBlock = hashPrevBlock
        block.solve()

        comp_block = HeaderAndShortIDs()
        comp_block.initialize_from_block(block)
        self.test_node.send_and_ping(msg_cmpctblock(comp_block.to_p2p()))

        tips = self.nodes[0].getchaintips()
        found = False
        for x in tips:
            if x["hash"] == block.hash:
                assert_equal(x["status"], "headers-only")
                found = True
                break
        assert(found)

        # Requesting this block via getblocktxn should silently fail
        # (to avoid fingerprinting attacks).
        msg = msg_getblocktxn()
        msg.block_txn_request = BlockTransactionsRequest(block.sha256, [0])
        with mininode_lock:
            self.test_node.last_blocktxn = None
        self.test_node.send_and_ping(msg)
        with mininode_lock:
            assert(self.test_node.last_blocktxn is None)

    def run_test(self):
        # Setup the p2p connections and start up the network thread.
        self.test_node = TestNode()

        connections = []
        connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], self.test_node))
        self.test_node.add_connection(connections[0])

        NetworkThread().start()  # Start up network handling in another thread

        # Test logic begins here
        self.test_node.wait_for_verack()

        # We will need UTXOs to construct transactions in later tests.
        self.make_utxos()

        self.test_sendcmpct()
        self.test_compactblock_construction()
        self.test_compactblock_requests()
        self.test_getblocktxn_requests()
        self.test_getblocktxn_handler()
        self.test_compactblocks_not_at_tip()
        self.test_incorrect_blocktxn_response()
        self.test_invalid_cmpctblock_message()


if __name__ == '__main__':
    CompactBlocksTest().main()
