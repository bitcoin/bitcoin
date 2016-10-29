#!/usr/bin/env python2
#
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.comptool import wait_until
import time

'''
Test behavior of -maxuploadtarget.

* Verify that getdata requests for old blocks (>1week) are dropped
if uploadtarget has been reached.
* Verify that getdata requests for recent blocks are respecteved even
if uploadtarget has been reached.
* Verify that the upload counters are reset after 24 hours.
'''

# BU: the EB parameter now controls the daily buffer size (the amount of
#     data that a peer can upload before the user-specified maxuploadtarget
#     parameter actually takes effect.
#     EB in this test is set to match the historic size, which resulted in
#     a daily buffer of 144 * 1MB.
#     But it is possible to adjust it (pick a multiple of 1MB below) and
#     tests should pass. This has been tested up to 16MB.
EXCESSIVE_BLOCKSIZE = 1000000  # bytes
print "running with -excessiveblocksize = %s bytes" % EXCESSIVE_BLOCKSIZE

# sync_with_ping() timeouts also need to scale with block size
# (this has been determined empirically)
# The formula below allows a sync timeout to scale up sufficiently to pass
# even with 16MB blocks (tested on a reasonably fast machine).
# The baseline has been increased from 30s to 60s which enables the test
# to pass with 1MB on a slow i686 machine. Raising the baseline timeout
# does not increase the test duration on faster machines.
SYNC_WITH_PING_TIMEOUT = 60 + 20 * int((max(1000000, EXCESSIVE_BLOCKSIZE)-1000000) / 1000000)  # seconds
print "sync_with_ping timeout =  %s sec" % SYNC_WITH_PING_TIMEOUT


# TestNode: bare-bones "peer".  Used mostly as a conduit for a test to sending
# p2p messages to a node, generating the messages in the main testing logic.
class TestNode(NodeConnCB):
    def __init__(self):
        NodeConnCB.__init__(self)
        self.connection = None
        self.ping_counter = 1
        self.last_pong = msg_pong()
        self.block_receive_map = {}

    def add_connection(self, conn):
        self.connection = conn
        self.peer_disconnected = False

    def on_inv(self, conn, message):
        pass

    # Track the last getdata message we receive (used in the test)
    def on_getdata(self, conn, message):
        self.last_getdata = message

    def on_block(self, conn, message):
        message.block.calc_sha256()
        try:
            self.block_receive_map[message.block.sha256] += 1
        except KeyError as e:
            self.block_receive_map[message.block.sha256] = 1

    # Spin until verack message is received from the node.
    # We use this to signal that our test can begin. This
    # is called from the testing thread, so it needs to acquire
    # the global lock.
    def wait_for_verack(self):
        def veracked():
            return self.verack_received
        return wait_until(veracked, timeout=10)

    def wait_for_disconnect(self):
        def disconnected():
            return self.peer_disconnected
        return wait_until(disconnected, timeout=10)

    # Wrapper for the NodeConn's send_message function
    def send_message(self, message):
        self.connection.send_message(message)

    def on_pong(self, conn, message):
        self.last_pong = message

    def on_close(self, conn):
        self.peer_disconnected = True

    # Sync up with the node after delivery of a block
    def sync_with_ping(self, timeout=SYNC_WITH_PING_TIMEOUT):
        def received_pong():
            return (self.last_pong.nonce == self.ping_counter)
        self.connection.send_message(msg_ping(nonce=self.ping_counter))
        success = wait_until(received_pong, timeout)
        self.ping_counter += 1
        return success

class MaxUploadTest(BitcoinTestFramework):
    def __init__(self):
        self.utxo = []
        self.txouts = gen_return_txouts()
 
    def add_options(self, parser):
        parser.add_option("--testbinary", dest="testbinary",
                          default=os.getenv("BITCOIND", "bitcoind"),
                          help="bitcoind binary to test")

    def setup_chain(self):
        initialize_chain_clean(self.options.tmpdir, 2)

    def setup_network(self):
        # Start a node with maxuploadtarget of 200*EB (MB/24h)
        # some other attributes defined below have been factored out of
        # other methods in this class.
        self.nodes = []
        # an overhead factor approximates how the traffic with bigger blocks
        self.overhead_factor_a = 10  # percent
        print "overhead_factor = %s %%" % self.overhead_factor_a
        # maxuploadtarget (unit: MiB) was 200 originally, now scale with EB
        self.maxuploadtarget = int(200 * EXCESSIVE_BLOCKSIZE / 1000000)
        print "maxuploadtarget = %sMiB" % self.maxuploadtarget
        self.blockmaxsize = EXCESSIVE_BLOCKSIZE-1000  # EB-1KB
        print "blockmaxsize = %s bytes" % self.blockmaxsize
        self.max_bytes_per_day = self.maxuploadtarget * 1024 * 1024
        print "max_bytes_per_day = %s bytes" % self.max_bytes_per_day
        self.daily_buffer = 144 * EXCESSIVE_BLOCKSIZE
        print "daily buffer = %s bytes" % self.daily_buffer
        self.max_bytes_available = self.max_bytes_per_day - self.daily_buffer
        print "max_bytes_available = %s bytes" % self.max_bytes_available
        # roughly how many 66k transactions we need to create a big block
        self.num_transactions = int(EXCESSIVE_BLOCKSIZE / 66000) - 1
        print "num txs in big block = %s" % self.num_transactions
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug",
                                                              "-use-thinblocks=0", # turned off to predict size of transmitted data
                                                              "-excessiveblocksize=%s" % EXCESSIVE_BLOCKSIZE,
                                                              "-maxuploadtarget=%s" % self.maxuploadtarget,
                                                              "-blockmaxsize=%s" % self.blockmaxsize]))

    def mine_big_block(self, node, address):
        # Want to create a big block
        # We'll generate a 66k transaction below
        for j in xrange(self.num_transactions):
            if len(self.utxo) < self.num_transactions:
                self.utxo = node.listunspent()
            inputs=[]
            outputs = {}
            t = self.utxo.pop()
            inputs.append({ "txid" : t["txid"], "vout" : t["vout"]})
            remchange = t["amount"] - Decimal("0.001000")
            outputs[address]=remchange
            # Create a basic transaction that will send change back to ourself after account for a fee
            # And then insert the 128 generated transaction outs in the middle rawtx[92] is where the #
            # of txouts is stored and is the only thing we overwrite from the original transaction
            rawtx = node.createrawtransaction(inputs, outputs)
            newtx = rawtx[0:92]
            newtx = newtx + self.txouts
            newtx = newtx + rawtx[94:]
            # Appears to be ever so slightly faster to sign with SIGHASH_NONE
            signresult = node.signrawtransaction(newtx,None,None,"NONE")
            txid = node.sendrawtransaction(signresult["hex"], True)
        # Mine a big sized block which will be these transactions we just created
        node.generate(1)

    def run_test(self):
        # Before we connect anything, we first set the time on the node
        # to be in the past, otherwise things break because the CNode
        # time counters can't be reset backward after initialization

        # set 2 weeks in the past
        old_time = int(time.time() - 2*60*60*24*7)
        self.nodes[0].setmocktime(old_time)

        # Generate some old blocks
        # we scale this by EB
        self.nodes[0].generate((130 * EXCESSIVE_BLOCKSIZE / 1000000))

        # test_nodes[0] will only request old blocks
        # test_nodes[1] will only request new blocks
        # test_nodes[2] will test resetting the counters
        test_nodes = []
        connections = []

        for i in xrange(3):
            test_nodes.append(TestNode())
            connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], test_nodes[i]))
            test_nodes[i].add_connection(connections[i])

        NetworkThread().start() # Start up network handling in another thread
        [x.wait_for_verack() for x in test_nodes]

        # Test logic begins here

        # Now mine a big block
        self.mine_big_block(self.nodes[0], self.nodes[0].getnewaddress())

        # Store the hash; we'll request this later
        big_old_block = self.nodes[0].getbestblockhash()
        old_block_size = self.nodes[0].getblock(big_old_block, True)['size']
        print "mined big block size = %s bytes" % old_block_size
        big_old_block = int(big_old_block, 16)

        # Advance to two days ago
        self.nodes[0].setmocktime(int(time.time()) - 2*60*60*24)

        # Mine one more block, so that the prior block looks old
        self.mine_big_block(self.nodes[0], self.nodes[0].getnewaddress())

        # We'll be requesting this new block too
        big_new_block = self.nodes[0].getbestblockhash()
        new_block_size = self.nodes[0].getblock(big_new_block)['size']
        big_new_block = int(big_new_block, 16)

        # test_nodes[0] will test what happens if we just keep requesting the
        # the same big old block too many times (expect: disconnect)

        getdata_request = msg_getdata()
        getdata_request.inv.append(CInv(2, big_old_block))

        # Compute a theoretical amount of blocks that should be transferable
        # based purely on the size of the big_old_block.
        # This calculation will be inaccurate due to other protocol
        # overheads, so a compensation factor is used later on to adjust
        # for those.
        successcount = self.max_bytes_available / old_block_size
        print "successcount = %s" % successcount
        compensation = int(successcount * (self.overhead_factor_a / 100.0))
        print "compensation = %s" % compensation

        # 144MB will be reserved for relaying new blocks, so expect this to
        # succeed for a certain number tries.
        for i in xrange(successcount - compensation):
            #print "data request #%s" % (i+1)
            test_nodes[0].send_message(getdata_request)
            test_nodes[0].sync_with_ping()
            assert_equal(test_nodes[0].block_receive_map[big_old_block], i+1)

        # check that the node has not been disconnected yet
        assert_equal(len(self.nodes[0].getpeerinfo()), 3)
        print "Peer 0 still connected after downloading old block %d times" % (successcount - compensation)

        # At most a couple more tries should succeed (depending on how long 
        # the test has been running so far).
        i = 1
        while True:
            test_nodes[0].send_message(getdata_request)
            test_nodes[0].sync_with_ping()
            if test_nodes[0].peer_disconnected: break
            i += 1

        #for i in xrange(3 * compensation):
        #    test_nodes[0].send_message(getdata_request)
        test_nodes[0].wait_for_disconnect()
        assert_equal(len(self.nodes[0].getpeerinfo()), 2)
        print "Peer 0 disconnected after downloading old block %d times" % (successcount - compensation + i)

        # Requesting the current block on test_nodes[1] should succeed indefinitely,
        # even when over the max upload target.
        # We'll try 200 times
        getdata_request.inv = [CInv(2, big_new_block)]
        for i in xrange(200):
            test_nodes[1].send_message(getdata_request)
            test_nodes[1].sync_with_ping()
            assert_equal(test_nodes[1].block_receive_map[big_new_block], i+1)

        print "Peer 1 able to repeatedly download new block"

        # But if test_nodes[1] tries for an old block, it gets disconnected too.
        getdata_request.inv = [CInv(2, big_old_block)]
        test_nodes[1].send_message(getdata_request)
        test_nodes[1].wait_for_disconnect()
        assert_equal(len(self.nodes[0].getpeerinfo()), 1)

        print "Peer 1 disconnected after trying to download old block"

        print "Advancing system time on node to clear counters..."

        # If we advance the time by 24 hours, then the counters should reset,
        # and test_nodes[2] should be able to retrieve the old block.
        self.nodes[0].setmocktime(int(time.time()))
        test_nodes[2].sync_with_ping()
        test_nodes[2].send_message(getdata_request)
        test_nodes[2].sync_with_ping()
        assert_equal(test_nodes[2].block_receive_map[big_old_block], 1)

        print "Peer 2 able to download old block"

        [c.disconnect_node() for c in connections]

        #stop and start node 0 with 1MB maxuploadtarget, whitelist 127.0.0.1
        print "Restarting nodes with -whitelist=127.0.0.1"
        stop_node(self.nodes[0], 0)
        self.nodes[0] = start_node(0, self.options.tmpdir, ["-debug", "-use-thinblocks=0", "-whitelist=127.0.0.1", "-excessiveblocksize=1000000", "-maxuploadtarget=1", "-blockmaxsize=999000"])

        #recreate/reconnect 3 test nodes
        test_nodes = []
        connections = []

        for i in xrange(3):
            test_nodes.append(TestNode())
            connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], test_nodes[i]))
            test_nodes[i].add_connection(connections[i])

        NetworkThread().start() # Start up network handling in another thread
        [x.wait_for_verack() for x in test_nodes]

        #retrieve 20 blocks which should be enough to break the 1MB limit
        getdata_request.inv = [CInv(2, big_new_block)]
        for i in xrange(20):
            test_nodes[1].send_message(getdata_request)
            test_nodes[1].sync_with_ping()
            assert_equal(test_nodes[1].block_receive_map[big_new_block], i+1)

        getdata_request.inv = [CInv(2, big_old_block)]
        test_nodes[1].send_message(getdata_request)
        test_nodes[1].wait_for_disconnect()
        assert_equal(len(self.nodes[0].getpeerinfo()), 3) #node is still connected because of the whitelist

        print "Peer 1 still connected after trying to download old block (whitelisted)"

        [c.disconnect_node() for c in connections]

if __name__ == '__main__':
    MaxUploadTest().main()
