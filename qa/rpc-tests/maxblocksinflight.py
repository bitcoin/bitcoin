#!/usr/bin/env python2
#
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import logging

'''
In this test we connect to one node over p2p, send it numerous inv's, and
compare the resulting number of getdata requests to a max allowed value.  We
test for exceeding 128 blocks in flight, which was the limit an 0.9 client will
reach. [0.10 clients shouldn't request more than 16 from a single peer.]
'''
MAX_REQUESTS = 128

class TestManager(NodeConnCB):
    # set up NodeConnCB callbacks, overriding base class
    def on_getdata(self, conn, message):
        self.log.debug("got getdata %s" % repr(message))
        # Log the requests
        for inv in message.inv:
            if inv.hash not in self.blockReqCounts:
                self.blockReqCounts[inv.hash] = 0
            self.blockReqCounts[inv.hash] += 1

    def on_close(self, conn):
        if not self.disconnectOkay:
            raise EarlyDisconnectError(0)

    def __init__(self):
        NodeConnCB.__init__(self)
        self.log = logging.getLogger("BlockRelayTest")

    def add_new_connection(self, connection):
        self.connection = connection
        self.blockReqCounts = {}
        self.disconnectOkay = False

    def run(self):
        try:
            fail = False
            self.connection.rpc.generate(1) # Leave IBD

            numBlocksToGenerate = [ 8, 16, 128, 1024 ]
            for count in range(len(numBlocksToGenerate)):
                current_invs = []
                for i in range(numBlocksToGenerate[count]):
                    current_invs.append(CInv(2, random.randrange(0, 1<<256)))
                    if len(current_invs) >= 50000:
                        self.connection.send_message(msg_inv(current_invs))
                        current_invs = []
                if len(current_invs) > 0:
                    self.connection.send_message(msg_inv(current_invs))
                
                # Wait and see how many blocks were requested
                time.sleep(2)

                total_requests = 0
                with mininode_lock:
                    for key in self.blockReqCounts:
                        total_requests += self.blockReqCounts[key]
                        if self.blockReqCounts[key] > 1:
                            raise AssertionError("Error, test failed: block %064x requested more than once" % key)
                if total_requests > MAX_REQUESTS:
                    raise AssertionError("Error, too many blocks (%d) requested" % total_requests)
                print "Round %d: success (total requests: %d)" % (count, total_requests)
        except AssertionError as e:
            print "TEST FAILED: ", e.args

        self.disconnectOkay = True
        self.connection.disconnect_node()

        
class MaxBlocksInFlightTest(BitcoinTestFramework):
    def add_options(self, parser):
        parser.add_option("--testbinary", dest="testbinary",
                          default=os.getenv("BITCOIND", "bitcoind"),
                          help="Binary to test max block requests behavior")

    def setup_chain(self):
        print "Initializing test directory "+self.options.tmpdir
        initialize_chain_clean(self.options.tmpdir, 1)

    def setup_network(self):
        self.nodes = start_nodes(1, self.options.tmpdir, 
                                 extra_args=[['-debug', '-whitelist=127.0.0.1']],
                                 binary=[self.options.testbinary])

    def run_test(self):
        test = TestManager()
        test.add_new_connection(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], test))
        NetworkThread().start()  # Start up network handling in another thread
        test.run()

if __name__ == '__main__':
    MaxBlocksInFlightTest().main()
