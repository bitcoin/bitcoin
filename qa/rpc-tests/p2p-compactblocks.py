#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.blocktools import create_block, create_coinbase, add_witness_commitment
from test_framework.siphash import siphash256
from test_framework.script import CScript, OP_TRUE

'''
CompactBlocksTest -- test compact blocks (BIP 152)

Version 1 compact blocks are pre-segwit (txids)
Version 2 compact blocks are post-segwit (wtxids)
'''

# TestNode: A peer we use to send messages to bitcoind, and store responses.
class TestNode(SingleNodeConnCB):
    def __init__(self):
        SingleNodeConnCB.__init__(self)
        self.last_sendcmpct = []
        self.last_headers = None
        self.last_inv = None
        self.last_cmpctblock = None
        self.block_announced = False
        self.last_getdata = None
        self.last_getheaders = None
        self.last_getblocktxn = None
        self.last_block = None
        self.last_blocktxn = None
        # Store the hashes of blocks we've seen announced.
        # This is for synchronizing the p2p message traffic,
        # so we can eg wait until a particular block is announced.
        self.set_announced_blockhashes = set()

    def on_sendcmpct(self, conn, message):
        self.last_sendcmpct.append(message)

    def on_block(self, conn, message):
        self.last_block = message

    def on_cmpctblock(self, conn, message):
        self.last_cmpctblock = message
        self.block_announced = True
        self.last_cmpctblock.header_and_shortids.header.calc_sha256()
        self.set_announced_blockhashes.add(self.last_cmpctblock.header_and_shortids.header.sha256)

    def on_headers(self, conn, message):
        self.last_headers = message
        self.block_announced = True
        for x in self.last_headers.headers:
            x.calc_sha256()
            self.set_announced_blockhashes.add(x.sha256)

    def on_inv(self, conn, message):
        self.last_inv = message
        for x in self.last_inv.inv:
            if x.type == 2:
                self.block_announced = True
                self.set_announced_blockhashes.add(x.hash)

    def on_getdata(self, conn, message):
        self.last_getdata = message

    def on_getheaders(self, conn, message):
        self.last_getheaders = message

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

    # Block until a block announcement for a particular block hash is
    # received.
    def wait_for_block_announcement(self, block_hash, timeout=30):
        def received_hash():
            return (block_hash in self.set_announced_blockhashes)
        return wait_until(received_hash, timeout=timeout)

class CompactBlocksTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        # Node0 = pre-segwit, node1 = segwit-aware
        self.num_nodes = 2
        self.utxos = []

    def setup_network(self):
        self.nodes = []

        # Start up node0 to be a version 1, pre-segwit node.
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, 
                [["-debug", "-logtimemicros=1", "-bip9params=segwit:0:0"], 
                 ["-debug", "-logtimemicros", "-txindex"]])
        connect_nodes(self.nodes[0], 1)

    def build_block_on_tip(self, node, segwit=False):
        height = node.getblockcount()
        tip = node.getbestblockhash()
        mtp = node.getblockheader(tip)['mediantime']
        block = create_block(int(tip, 16), create_coinbase(height + 1), mtp + 1)
        block.nVersion = 4
        if segwit:
            add_witness_commitment(block)
        block.solve()
        return block

    # Create 10 more anyone-can-spend utxo's for testing.
    def make_utxos(self):
        # Doesn't matter which node we use, just use node0.
        block = self.build_block_on_tip(self.nodes[0])
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

        block2 = self.build_block_on_tip(self.nodes[0])
        block2.vtx.append(tx)
        block2.hashMerkleRoot = block2.calc_merkle_root()
        block2.solve()
        self.test_node.send_and_ping(msg_block(block2))
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block2.sha256)
        self.utxos.extend([[tx.sha256, i, out_value] for i in range(10)])
        return

    # Test "sendcmpct" (between peers preferring the same version):
    # - No compact block announcements unless sendcmpct is sent.
    # - If sendcmpct is sent with version > preferred_version, the message is ignored.
    # - If sendcmpct is sent with boolean 0, then block announcements are not
    #   made with compact blocks.
    # - If sendcmpct is then sent with boolean 1, then new block announcements
    #   are made with compact blocks.
    # If old_node is passed in, request compact blocks with version=preferred-1
    # and verify that it receives block announcements via compact block.
    def test_sendcmpct(self, node, test_node, preferred_version, old_node=None):
        # Make sure we get a SENDCMPCT message from our peer
        def received_sendcmpct():
            return (len(test_node.last_sendcmpct) > 0)
        got_message = wait_until(received_sendcmpct, timeout=30)
        assert(received_sendcmpct())
        assert(got_message)
        with mininode_lock:
            # Check that the first version received is the preferred one
            assert_equal(test_node.last_sendcmpct[0].version, preferred_version)
            # And that we receive versions down to 1.
            assert_equal(test_node.last_sendcmpct[-1].version, 1)
            test_node.last_sendcmpct = []

        tip = int(node.getbestblockhash(), 16)

        def check_announcement_of_new_block(node, peer, predicate):
            peer.clear_block_announcement()
            block_hash = int(node.generate(1)[0], 16)
            peer.wait_for_block_announcement(block_hash, timeout=30)
            assert(peer.block_announced)
            assert(got_message)

            with mininode_lock:
                assert predicate(peer), (
                    "block_hash={!r}, cmpctblock={!r}, inv={!r}".format(
                        block_hash, peer.last_cmpctblock, peer.last_inv))

        # We shouldn't get any block announcements via cmpctblock yet.
        check_announcement_of_new_block(node, test_node, lambda p: p.last_cmpctblock is None)

        # Try one more time, this time after requesting headers.
        test_node.request_headers_and_sync(locator=[tip])
        check_announcement_of_new_block(node, test_node, lambda p: p.last_cmpctblock is None and p.last_inv is not None)

        # Test a few ways of using sendcmpct that should NOT
        # result in compact block announcements.
        # Before each test, sync the headers chain.
        test_node.request_headers_and_sync(locator=[tip])

        # Now try a SENDCMPCT message with too-high version
        sendcmpct = msg_sendcmpct()
        sendcmpct.version = preferred_version+1
        sendcmpct.announce = True
        test_node.send_and_ping(sendcmpct)
        check_announcement_of_new_block(node, test_node, lambda p: p.last_cmpctblock is None)

        # Headers sync before next test.
        test_node.request_headers_and_sync(locator=[tip])

        # Now try a SENDCMPCT message with valid version, but announce=False
        sendcmpct.version = preferred_version
        sendcmpct.announce = False
        test_node.send_and_ping(sendcmpct)
        check_announcement_of_new_block(node, test_node, lambda p: p.last_cmpctblock is None)

        # Headers sync before next test.
        test_node.request_headers_and_sync(locator=[tip])

        # Finally, try a SENDCMPCT message with announce=True
        sendcmpct.version = preferred_version
        sendcmpct.announce = True
        test_node.send_and_ping(sendcmpct)
        check_announcement_of_new_block(node, test_node, lambda p: p.last_cmpctblock is not None)

        # Try one more time (no headers sync should be needed!)
        check_announcement_of_new_block(node, test_node, lambda p: p.last_cmpctblock is not None)

        # Try one more time, after turning on sendheaders
        test_node.send_and_ping(msg_sendheaders())
        check_announcement_of_new_block(node, test_node, lambda p: p.last_cmpctblock is not None)

        # Try one more time, after sending a version-1, announce=false message.
        sendcmpct.version = preferred_version-1
        sendcmpct.announce = False
        test_node.send_and_ping(sendcmpct)
        check_announcement_of_new_block(node, test_node, lambda p: p.last_cmpctblock is not None)

        # Now turn off announcements
        sendcmpct.version = preferred_version
        sendcmpct.announce = False
        test_node.send_and_ping(sendcmpct)
        check_announcement_of_new_block(node, test_node, lambda p: p.last_cmpctblock is None and p.last_headers is not None)

        if old_node is not None:
            # Verify that a peer using an older protocol version can receive
            # announcements from this node.
            sendcmpct.version = preferred_version-1
            sendcmpct.announce = True
            old_node.send_and_ping(sendcmpct)
            # Header sync
            old_node.request_headers_and_sync(locator=[tip])
            check_announcement_of_new_block(node, old_node, lambda p: p.last_cmpctblock is not None)

    # This test actually causes bitcoind to (reasonably!) disconnect us, so do this last.
    def test_invalid_cmpctblock_message(self):
        self.nodes[0].generate(101)
        block = self.build_block_on_tip(self.nodes[0])

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
    def test_compactblock_construction(self, node, test_node, version, use_witness_address):
        # Generate a bunch of transactions.
        node.generate(101)
        num_transactions = 25
        address = node.getnewaddress()
        if use_witness_address:
            # Want at least one segwit spend, so move all funds to
            # a witness address.
            address = node.addwitnessaddress(address)
            value_to_send = node.getbalance()
            node.sendtoaddress(address, satoshi_round(value_to_send-Decimal(0.1)))
            node.generate(1)

        segwit_tx_generated = False
        for i in range(num_transactions):
            txid = node.sendtoaddress(address, 0.1)
            hex_tx = node.gettransaction(txid)["hex"]
            tx = FromHex(CTransaction(), hex_tx)
            if not tx.wit.is_null():
                segwit_tx_generated = True

        if use_witness_address:
            assert(segwit_tx_generated) # check that our test is not broken

        # Wait until we've seen the block announcement for the resulting tip
        tip = int(node.getbestblockhash(), 16)
        assert(test_node.wait_for_block_announcement(tip))

        # Make sure we will receive a fast-announce compact block
        self.request_cb_announcements(test_node, node, version)

        # Now mine a block, and look at the resulting compact block.
        test_node.clear_block_announcement()
        block_hash = int(node.generate(1)[0], 16)

        # Store the raw block in our internal format.
        block = FromHex(CBlock(), node.getblock("%02x" % block_hash, False))
        [tx.calc_sha256() for tx in block.vtx]
        block.rehash()

        # Wait until the block was announced (via compact blocks)
        wait_until(test_node.received_block_announcement, timeout=30)
        assert(test_node.received_block_announcement())

        # Now fetch and check the compact block
        header_and_shortids = None
        with mininode_lock:
            assert(test_node.last_cmpctblock is not None)
            # Convert the on-the-wire representation to absolute indexes
            header_and_shortids = HeaderAndShortIDs(test_node.last_cmpctblock.header_and_shortids)
        self.check_compactblock_construction_from_block(version, header_and_shortids, block_hash, block)

        # Now fetch the compact block using a normal non-announce getdata
        with mininode_lock:
            test_node.clear_block_announcement()
            inv = CInv(4, block_hash)  # 4 == "CompactBlock"
            test_node.send_message(msg_getdata([inv]))

        wait_until(test_node.received_block_announcement, timeout=30)
        assert(test_node.received_block_announcement())

        # Now fetch and check the compact block
        header_and_shortids = None
        with mininode_lock:
            assert(test_node.last_cmpctblock is not None)
            # Convert the on-the-wire representation to absolute indexes
            header_and_shortids = HeaderAndShortIDs(test_node.last_cmpctblock.header_and_shortids)
        self.check_compactblock_construction_from_block(version, header_and_shortids, block_hash, block)

    def check_compactblock_construction_from_block(self, version, header_and_shortids, block_hash, block):
        # Check that we got the right block!
        header_and_shortids.header.calc_sha256()
        assert_equal(header_and_shortids.header.sha256, block_hash)

        # Make sure the prefilled_txn appears to have included the coinbase
        assert(len(header_and_shortids.prefilled_txn) >= 1)
        assert_equal(header_and_shortids.prefilled_txn[0].index, 0)

        # Check that all prefilled_txn entries match what's in the block.
        for entry in header_and_shortids.prefilled_txn:
            entry.tx.calc_sha256()
            # This checks the non-witness parts of the tx agree
            assert_equal(entry.tx.sha256, block.vtx[entry.index].sha256)

            # And this checks the witness
            wtxid = entry.tx.calc_sha256(True)
            if version == 2:
                assert_equal(wtxid, block.vtx[entry.index].calc_sha256(True))
            else:
                # Shouldn't have received a witness
                assert(entry.tx.wit.is_null())

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
                tx_hash = block.vtx[index].sha256
                if version == 2:
                    tx_hash = block.vtx[index].calc_sha256(True)
                shortid = calculate_shortid(k0, k1, tx_hash)
                assert_equal(shortid, header_and_shortids.shortids[0])
                header_and_shortids.shortids.pop(0)
            index += 1

    # Test that bitcoind requests compact blocks when we announce new blocks
    # via header or inv, and that responding to getblocktxn causes the block
    # to be successfully reconstructed.
    # Post-segwit: upgraded nodes would only make this request of cb-version-2,
    # NODE_WITNESS peers.  Unupgraded nodes would still make this request of
    # any cb-version-1-supporting peer.
    def test_compactblock_requests(self, node, test_node, version, segwit):
        # Try announcing a block with an inv or header, expect a compactblock
        # request
        for announce in ["inv", "header"]:
            block = self.build_block_on_tip(node, segwit=segwit)
            with mininode_lock:
                test_node.last_getdata = None

            if announce == "inv":
                test_node.send_message(msg_inv([CInv(2, block.sha256)]))
                success = wait_until(lambda: test_node.last_getheaders is not None, timeout=30)
                assert(success)
                test_node.send_header_for_blocks([block])
            else:
                test_node.send_header_for_blocks([block])
            success = wait_until(lambda: test_node.last_getdata is not None, timeout=30)
            assert(success)
            assert_equal(len(test_node.last_getdata.inv), 1)
            assert_equal(test_node.last_getdata.inv[0].type, 4)
            assert_equal(test_node.last_getdata.inv[0].hash, block.sha256)

            # Send back a compactblock message that omits the coinbase
            comp_block = HeaderAndShortIDs()
            comp_block.header = CBlockHeader(block)
            comp_block.nonce = 0
            [k0, k1] = comp_block.get_siphash_keys()
            coinbase_hash = block.vtx[0].sha256
            if version == 2:
                coinbase_hash = block.vtx[0].calc_sha256(True)
            comp_block.shortids = [
                    calculate_shortid(k0, k1, coinbase_hash) ]
            test_node.send_and_ping(msg_cmpctblock(comp_block.to_p2p()))
            assert_equal(int(node.getbestblockhash(), 16), block.hashPrevBlock)
            # Expect a getblocktxn message.
            with mininode_lock:
                assert(test_node.last_getblocktxn is not None)
                absolute_indexes = test_node.last_getblocktxn.block_txn_request.to_absolute()
            assert_equal(absolute_indexes, [0])  # should be a coinbase request

            # Send the coinbase, and verify that the tip advances.
            if version == 2:
                msg = msg_witness_blocktxn()
            else:
                msg = msg_blocktxn()
            msg.block_transactions.blockhash = block.sha256
            msg.block_transactions.transactions = [block.vtx[0]]
            test_node.send_and_ping(msg)
            assert_equal(int(node.getbestblockhash(), 16), block.sha256)

    # Create a chain of transactions from given utxo, and add to a new block.
    def build_block_with_transactions(self, node, utxo, num_transactions):
        block = self.build_block_on_tip(node)

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
    def test_getblocktxn_requests(self, node, test_node, version):
        with_witness = (version==2)

        def test_getblocktxn_response(compact_block, peer, expected_result):
            msg = msg_cmpctblock(compact_block.to_p2p())
            peer.send_and_ping(msg)
            with mininode_lock:
                assert(peer.last_getblocktxn is not None)
                absolute_indexes = peer.last_getblocktxn.block_txn_request.to_absolute()
            assert_equal(absolute_indexes, expected_result)

        def test_tip_after_message(node, peer, msg, tip):
            peer.send_and_ping(msg)
            assert_equal(int(node.getbestblockhash(), 16), tip)

        # First try announcing compactblocks that won't reconstruct, and verify
        # that we receive getblocktxn messages back.
        utxo = self.utxos.pop(0)

        block = self.build_block_with_transactions(node, utxo, 5)
        self.utxos.append([block.vtx[-1].sha256, 0, block.vtx[-1].vout[0].nValue])
        comp_block = HeaderAndShortIDs()
        comp_block.initialize_from_block(block, use_witness=with_witness)

        test_getblocktxn_response(comp_block, test_node, [1, 2, 3, 4, 5])

        msg_bt = msg_blocktxn()
        if with_witness:
            msg_bt = msg_witness_blocktxn() # serialize with witnesses
        msg_bt.block_transactions = BlockTransactions(block.sha256, block.vtx[1:])
        test_tip_after_message(node, test_node, msg_bt, block.sha256)

        utxo = self.utxos.pop(0)
        block = self.build_block_with_transactions(node, utxo, 5)
        self.utxos.append([block.vtx[-1].sha256, 0, block.vtx[-1].vout[0].nValue])

        # Now try interspersing the prefilled transactions
        comp_block.initialize_from_block(block, prefill_list=[0, 1, 5], use_witness=with_witness)
        test_getblocktxn_response(comp_block, test_node, [2, 3, 4])
        msg_bt.block_transactions = BlockTransactions(block.sha256, block.vtx[2:5])
        test_tip_after_message(node, test_node, msg_bt, block.sha256)

        # Now try giving one transaction ahead of time.
        utxo = self.utxos.pop(0)
        block = self.build_block_with_transactions(node, utxo, 5)
        self.utxos.append([block.vtx[-1].sha256, 0, block.vtx[-1].vout[0].nValue])
        test_node.send_and_ping(msg_tx(block.vtx[1]))
        assert(block.vtx[1].hash in node.getrawmempool())

        # Prefill 4 out of the 6 transactions, and verify that only the one
        # that was not in the mempool is requested.
        comp_block.initialize_from_block(block, prefill_list=[0, 2, 3, 4], use_witness=with_witness)
        test_getblocktxn_response(comp_block, test_node, [5])

        msg_bt.block_transactions = BlockTransactions(block.sha256, [block.vtx[5]])
        test_tip_after_message(node, test_node, msg_bt, block.sha256)

        # Now provide all transactions to the node before the block is
        # announced and verify reconstruction happens immediately.
        utxo = self.utxos.pop(0)
        block = self.build_block_with_transactions(node, utxo, 10)
        self.utxos.append([block.vtx[-1].sha256, 0, block.vtx[-1].vout[0].nValue])
        for tx in block.vtx[1:]:
            test_node.send_message(msg_tx(tx))
        test_node.sync_with_ping()
        # Make sure all transactions were accepted.
        mempool = node.getrawmempool()
        for tx in block.vtx[1:]:
            assert(tx.hash in mempool)

        # Clear out last request.
        with mininode_lock:
            test_node.last_getblocktxn = None

        # Send compact block
        comp_block.initialize_from_block(block, prefill_list=[0], use_witness=with_witness)
        test_tip_after_message(node, test_node, msg_cmpctblock(comp_block.to_p2p()), block.sha256)
        with mininode_lock:
            # Shouldn't have gotten a request for any transaction
            assert(test_node.last_getblocktxn is None)

    # Incorrectly responding to a getblocktxn shouldn't cause the block to be
    # permanently failed.
    def test_incorrect_blocktxn_response(self, node, test_node, version):
        if (len(self.utxos) == 0):
            self.make_utxos()
        utxo = self.utxos.pop(0)

        block = self.build_block_with_transactions(node, utxo, 10)
        self.utxos.append([block.vtx[-1].sha256, 0, block.vtx[-1].vout[0].nValue])
        # Relay the first 5 transactions from the block in advance
        for tx in block.vtx[1:6]:
            test_node.send_message(msg_tx(tx))
        test_node.sync_with_ping()
        # Make sure all transactions were accepted.
        mempool = node.getrawmempool()
        for tx in block.vtx[1:6]:
            assert(tx.hash in mempool)

        # Send compact block
        comp_block = HeaderAndShortIDs()
        comp_block.initialize_from_block(block, prefill_list=[0], use_witness=(version == 2))
        test_node.send_and_ping(msg_cmpctblock(comp_block.to_p2p()))
        absolute_indexes = []
        with mininode_lock:
            assert(test_node.last_getblocktxn is not None)
            absolute_indexes = test_node.last_getblocktxn.block_txn_request.to_absolute()
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
        if version==2:
            msg = msg_witness_blocktxn()
        msg.block_transactions = BlockTransactions(block.sha256, [block.vtx[5]] + block.vtx[7:])
        test_node.send_and_ping(msg)

        # Tip should not have updated
        assert_equal(int(node.getbestblockhash(), 16), block.hashPrevBlock)

        # We should receive a getdata request
        success = wait_until(lambda: test_node.last_getdata is not None, timeout=10)
        assert(success)
        assert_equal(len(test_node.last_getdata.inv), 1)
        assert(test_node.last_getdata.inv[0].type == 2 or test_node.last_getdata.inv[0].type == 2|MSG_WITNESS_FLAG)
        assert_equal(test_node.last_getdata.inv[0].hash, block.sha256)

        # Deliver the block
        if version==2:
            test_node.send_and_ping(msg_witness_block(block))
        else:
            test_node.send_and_ping(msg_block(block))
        assert_equal(int(node.getbestblockhash(), 16), block.sha256)

    def test_getblocktxn_handler(self, node, test_node, version):
        # bitcoind will not send blocktxn responses for blocks whose height is
        # more than 10 blocks deep.
        MAX_GETBLOCKTXN_DEPTH = 10
        chain_height = node.getblockcount()
        current_height = chain_height
        while (current_height >= chain_height - MAX_GETBLOCKTXN_DEPTH):
            block_hash = node.getblockhash(current_height)
            block = FromHex(CBlock(), node.getblock(block_hash, False))

            msg = msg_getblocktxn()
            msg.block_txn_request = BlockTransactionsRequest(int(block_hash, 16), [])
            num_to_request = random.randint(1, len(block.vtx))
            msg.block_txn_request.from_absolute(sorted(random.sample(range(len(block.vtx)), num_to_request)))
            test_node.send_message(msg)
            success = wait_until(lambda: test_node.last_blocktxn is not None, timeout=10)
            assert(success)

            [tx.calc_sha256() for tx in block.vtx]
            with mininode_lock:
                assert_equal(test_node.last_blocktxn.block_transactions.blockhash, int(block_hash, 16))
                all_indices = msg.block_txn_request.to_absolute()
                for index in all_indices:
                    tx = test_node.last_blocktxn.block_transactions.transactions.pop(0)
                    tx.calc_sha256()
                    assert_equal(tx.sha256, block.vtx[index].sha256)
                    if version == 1:
                        # Witnesses should have been stripped
                        assert(tx.wit.is_null())
                    else:
                        # Check that the witness matches
                        assert_equal(tx.calc_sha256(True), block.vtx[index].calc_sha256(True))
                test_node.last_blocktxn = None
            current_height -= 1

        # Next request should send a full block response, as we're past the
        # allowed depth for a blocktxn response.
        block_hash = node.getblockhash(current_height)
        msg.block_txn_request = BlockTransactionsRequest(int(block_hash, 16), [0])
        with mininode_lock:
            test_node.last_block = None
            test_node.last_blocktxn = None
        test_node.send_and_ping(msg)
        with mininode_lock:
            test_node.last_block.block.calc_sha256()
            assert_equal(test_node.last_block.block.sha256, int(block_hash, 16))
            assert_equal(test_node.last_blocktxn, None)

    def test_compactblocks_not_at_tip(self, node, test_node):
        # Test that requesting old compactblocks doesn't work.
        MAX_CMPCTBLOCK_DEPTH = 5
        new_blocks = []
        for i in range(MAX_CMPCTBLOCK_DEPTH + 1):
            test_node.clear_block_announcement()
            new_blocks.append(node.generate(1)[0])
            wait_until(test_node.received_block_announcement, timeout=30)

        test_node.clear_block_announcement()
        test_node.send_message(msg_getdata([CInv(4, int(new_blocks[0], 16))]))
        success = wait_until(lambda: test_node.last_cmpctblock is not None, timeout=30)
        assert(success)

        test_node.clear_block_announcement()
        node.generate(1)
        wait_until(test_node.received_block_announcement, timeout=30)
        test_node.clear_block_announcement()
        with mininode_lock:
            test_node.last_block = None
        test_node.send_message(msg_getdata([CInv(4, int(new_blocks[0], 16))]))
        success = wait_until(lambda: test_node.last_block is not None, timeout=30)
        assert(success)
        with mininode_lock:
            test_node.last_block.block.calc_sha256()
            assert_equal(test_node.last_block.block.sha256, int(new_blocks[0], 16))

        # Generate an old compactblock, and verify that it's not accepted.
        cur_height = node.getblockcount()
        hashPrevBlock = int(node.getblockhash(cur_height-5), 16)
        block = self.build_block_on_tip(node)
        block.hashPrevBlock = hashPrevBlock
        block.solve()

        comp_block = HeaderAndShortIDs()
        comp_block.initialize_from_block(block)
        test_node.send_and_ping(msg_cmpctblock(comp_block.to_p2p()))

        tips = node.getchaintips()
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
            test_node.last_blocktxn = None
        test_node.send_and_ping(msg)
        with mininode_lock:
            assert(test_node.last_blocktxn is None)

    def activate_segwit(self, node):
        node.generate(144*3)
        assert_equal(get_bip9_status(node, "segwit")["status"], 'active')

    def test_end_to_end_block_relay(self, node, listeners):
        utxo = self.utxos.pop(0)

        block = self.build_block_with_transactions(node, utxo, 10)

        [l.clear_block_announcement() for l in listeners]

        # ToHex() won't serialize with witness, but this block has no witnesses
        # anyway. TODO: repeat this test with witness tx's to a segwit node.
        node.submitblock(ToHex(block))

        for l in listeners:
            wait_until(lambda: l.received_block_announcement(), timeout=30)
        with mininode_lock:
            for l in listeners:
                assert(l.last_cmpctblock is not None)
                l.last_cmpctblock.header_and_shortids.header.calc_sha256()
                assert_equal(l.last_cmpctblock.header_and_shortids.header.sha256, block.sha256)

    # Test that we don't get disconnected if we relay a compact block with valid header,
    # but invalid transactions.
    def test_invalid_tx_in_compactblock(self, node, test_node, use_segwit):
        assert(len(self.utxos))
        utxo = self.utxos[0]

        block = self.build_block_with_transactions(node, utxo, 5)
        del block.vtx[3]
        block.hashMerkleRoot = block.calc_merkle_root()
        if use_segwit:
            # If we're testing with segwit, also drop the coinbase witness,
            # but include the witness commitment.
            add_witness_commitment(block)
            block.vtx[0].wit.vtxinwit = []
        block.solve()

        # Now send the compact block with all transactions prefilled, and
        # verify that we don't get disconnected.
        comp_block = HeaderAndShortIDs()
        comp_block.initialize_from_block(block, prefill_list=[0, 1, 2, 3, 4], use_witness=use_segwit)
        msg = msg_cmpctblock(comp_block.to_p2p())
        test_node.send_and_ping(msg)

        # Check that the tip didn't advance
        assert(int(node.getbestblockhash(), 16) is not block.sha256)
        test_node.sync_with_ping()

    # Helper for enabling cb announcements
    # Send the sendcmpct request and sync headers
    def request_cb_announcements(self, peer, node, version):
        tip = node.getbestblockhash()
        peer.get_headers(locator=[int(tip, 16)], hashstop=0)

        msg = msg_sendcmpct()
        msg.version = version
        msg.announce = True
        peer.send_and_ping(msg)

    def test_compactblock_reconstruction_multiple_peers(self, node, stalling_peer, delivery_peer):
        assert(len(self.utxos))

        def announce_cmpct_block(node, peer):
            utxo = self.utxos.pop(0)
            block = self.build_block_with_transactions(node, utxo, 5)

            cmpct_block = HeaderAndShortIDs()
            cmpct_block.initialize_from_block(block)
            msg = msg_cmpctblock(cmpct_block.to_p2p())
            peer.send_and_ping(msg)
            with mininode_lock:
                assert(peer.last_getblocktxn is not None)
            return block, cmpct_block

        block, cmpct_block = announce_cmpct_block(node, stalling_peer)

        for tx in block.vtx[1:]:
            delivery_peer.send_message(msg_tx(tx))
        delivery_peer.sync_with_ping()
        mempool = node.getrawmempool()
        for tx in block.vtx[1:]:
            assert(tx.hash in mempool)

        delivery_peer.send_and_ping(msg_cmpctblock(cmpct_block.to_p2p()))
        assert_equal(int(node.getbestblockhash(), 16), block.sha256)

        self.utxos.append([block.vtx[-1].sha256, 0, block.vtx[-1].vout[0].nValue])

        # Now test that delivering an invalid compact block won't break relay

        block, cmpct_block = announce_cmpct_block(node, stalling_peer)
        for tx in block.vtx[1:]:
            delivery_peer.send_message(msg_tx(tx))
        delivery_peer.sync_with_ping()

        cmpct_block.prefilled_txn[0].tx.wit.vtxinwit = [ CTxInWitness() ]
        cmpct_block.prefilled_txn[0].tx.wit.vtxinwit[0].scriptWitness.stack = [ser_uint256(0)]

        cmpct_block.use_witness = True
        delivery_peer.send_and_ping(msg_cmpctblock(cmpct_block.to_p2p()))
        assert(int(node.getbestblockhash(), 16) != block.sha256)

        msg = msg_blocktxn()
        msg.block_transactions.blockhash = block.sha256
        msg.block_transactions.transactions = block.vtx[1:]
        stalling_peer.send_and_ping(msg)
        assert_equal(int(node.getbestblockhash(), 16), block.sha256)

    def run_test(self):
        # Setup the p2p connections and start up the network thread.
        self.test_node = TestNode()
        self.segwit_node = TestNode()
        self.old_node = TestNode()  # version 1 peer <--> segwit node

        connections = []
        connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], self.test_node))
        connections.append(NodeConn('127.0.0.1', p2p_port(1), self.nodes[1],
                    self.segwit_node, services=NODE_NETWORK|NODE_WITNESS))
        connections.append(NodeConn('127.0.0.1', p2p_port(1), self.nodes[1],
                    self.old_node, services=NODE_NETWORK))
        self.test_node.add_connection(connections[0])
        self.segwit_node.add_connection(connections[1])
        self.old_node.add_connection(connections[2])

        NetworkThread().start()  # Start up network handling in another thread

        # Test logic begins here
        self.test_node.wait_for_verack()

        # We will need UTXOs to construct transactions in later tests.
        self.make_utxos()

        print("Running tests, pre-segwit activation:")

        print("\tTesting SENDCMPCT p2p message... ")
        self.test_sendcmpct(self.nodes[0], self.test_node, 1)
        sync_blocks(self.nodes)
        self.test_sendcmpct(self.nodes[1], self.segwit_node, 2, old_node=self.old_node)
        sync_blocks(self.nodes)

        print("\tTesting compactblock construction...")
        self.test_compactblock_construction(self.nodes[0], self.test_node, 1, False)
        sync_blocks(self.nodes)
        self.test_compactblock_construction(self.nodes[1], self.segwit_node, 2, False)
        sync_blocks(self.nodes)

        print("\tTesting compactblock requests... ")
        self.test_compactblock_requests(self.nodes[0], self.test_node, 1, False)
        sync_blocks(self.nodes)
        self.test_compactblock_requests(self.nodes[1], self.segwit_node, 2, False)
        sync_blocks(self.nodes)

        print("\tTesting getblocktxn requests...")
        self.test_getblocktxn_requests(self.nodes[0], self.test_node, 1)
        sync_blocks(self.nodes)
        self.test_getblocktxn_requests(self.nodes[1], self.segwit_node, 2)
        sync_blocks(self.nodes)

        print("\tTesting getblocktxn handler...")
        self.test_getblocktxn_handler(self.nodes[0], self.test_node, 1)
        sync_blocks(self.nodes)
        self.test_getblocktxn_handler(self.nodes[1], self.segwit_node, 2)
        self.test_getblocktxn_handler(self.nodes[1], self.old_node, 1)
        sync_blocks(self.nodes)

        print("\tTesting compactblock requests/announcements not at chain tip...")
        self.test_compactblocks_not_at_tip(self.nodes[0], self.test_node)
        sync_blocks(self.nodes)
        self.test_compactblocks_not_at_tip(self.nodes[1], self.segwit_node)
        self.test_compactblocks_not_at_tip(self.nodes[1], self.old_node)
        sync_blocks(self.nodes)

        print("\tTesting handling of incorrect blocktxn responses...")
        self.test_incorrect_blocktxn_response(self.nodes[0], self.test_node, 1)
        sync_blocks(self.nodes)
        self.test_incorrect_blocktxn_response(self.nodes[1], self.segwit_node, 2)
        sync_blocks(self.nodes)

        # End-to-end block relay tests
        print("\tTesting end-to-end block relay...")
        self.request_cb_announcements(self.test_node, self.nodes[0], 1)
        self.request_cb_announcements(self.old_node, self.nodes[1], 1)
        self.request_cb_announcements(self.segwit_node, self.nodes[1], 2)
        self.test_end_to_end_block_relay(self.nodes[0], [self.segwit_node, self.test_node, self.old_node])
        self.test_end_to_end_block_relay(self.nodes[1], [self.segwit_node, self.test_node, self.old_node])

        print("\tTesting handling of invalid compact blocks...")
        self.test_invalid_tx_in_compactblock(self.nodes[0], self.test_node, False)
        self.test_invalid_tx_in_compactblock(self.nodes[1], self.segwit_node, False)
        self.test_invalid_tx_in_compactblock(self.nodes[1], self.old_node, False)

        print("\tTesting reconstructing compact blocks from all peers...")
        self.test_compactblock_reconstruction_multiple_peers(self.nodes[1], self.segwit_node, self.old_node)
        sync_blocks(self.nodes)

        # Advance to segwit activation
        print ("\nAdvancing to segwit activation\n")
        self.activate_segwit(self.nodes[1])
        print ("Running tests, post-segwit activation...")

        print("\tTesting compactblock construction...")
        self.test_compactblock_construction(self.nodes[1], self.old_node, 1, True)
        self.test_compactblock_construction(self.nodes[1], self.segwit_node, 2, True)
        sync_blocks(self.nodes)

        print("\tTesting compactblock requests (unupgraded node)... ")
        self.test_compactblock_requests(self.nodes[0], self.test_node, 1, True)

        print("\tTesting getblocktxn requests (unupgraded node)...")
        self.test_getblocktxn_requests(self.nodes[0], self.test_node, 1)

        # Need to manually sync node0 and node1, because post-segwit activation,
        # node1 will not download blocks from node0.
        print("\tSyncing nodes...")
        assert(self.nodes[0].getbestblockhash() != self.nodes[1].getbestblockhash())
        while (self.nodes[0].getblockcount() > self.nodes[1].getblockcount()):
            block_hash = self.nodes[0].getblockhash(self.nodes[1].getblockcount()+1)
            self.nodes[1].submitblock(self.nodes[0].getblock(block_hash, False))
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())

        print("\tTesting compactblock requests (segwit node)... ")
        self.test_compactblock_requests(self.nodes[1], self.segwit_node, 2, True)

        print("\tTesting getblocktxn requests (segwit node)...")
        self.test_getblocktxn_requests(self.nodes[1], self.segwit_node, 2)
        sync_blocks(self.nodes)

        print("\tTesting getblocktxn handler (segwit node should return witnesses)...")
        self.test_getblocktxn_handler(self.nodes[1], self.segwit_node, 2)
        self.test_getblocktxn_handler(self.nodes[1], self.old_node, 1)

        # Test that if we submitblock to node1, we'll get a compact block
        # announcement to all peers.
        # (Post-segwit activation, blocks won't propagate from node0 to node1
        # automatically, so don't bother testing a block announced to node0.)
        print("\tTesting end-to-end block relay...")
        self.request_cb_announcements(self.test_node, self.nodes[0], 1)
        self.request_cb_announcements(self.old_node, self.nodes[1], 1)
        self.request_cb_announcements(self.segwit_node, self.nodes[1], 2)
        self.test_end_to_end_block_relay(self.nodes[1], [self.segwit_node, self.test_node, self.old_node])

        print("\tTesting handling of invalid compact blocks...")
        self.test_invalid_tx_in_compactblock(self.nodes[0], self.test_node, False)
        self.test_invalid_tx_in_compactblock(self.nodes[1], self.segwit_node, True)
        self.test_invalid_tx_in_compactblock(self.nodes[1], self.old_node, True)

        print("\tTesting invalid index in cmpctblock message...")
        self.test_invalid_cmpctblock_message()


if __name__ == '__main__':
    CompactBlocksTest().main()
