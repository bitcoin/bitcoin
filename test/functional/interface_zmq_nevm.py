#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the ZMQ notification interface."""

from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE
from test_framework.test_framework import SyscoinTestFramework
from test_framework.messages import hash256, CNEVMBlock, CNEVMBlockConnect, CNEVMBlockDisconnect, uint256_from_str
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from io import BytesIO
from time import sleep
from threading import Thread
import random
# these would be handlers for the 3 types of calls from Syscoin on Geth
def receive_thread_nevm(self, idx, subscriber):
    while True:
        try:
            self.log.info('receive_thread_nevm waiting to receive... idx {}'.format(idx))
            data = subscriber.receive()
            if data[0] == b"nevmcomms":
                subscriber.send([b"nevmcomms", b"ack"])
            elif data[0] == b"nevmblock":
                hashStr = hash256(str(random.randint(-0x80000000, 0x7fffffff)).encode())
                hashTopic = uint256_from_str(hashStr)
                nevmBlock = CNEVMBlock(hashTopic, hashStr, hashStr, hashStr, b"nevmblock")
                subscriber.send([b"nevmblock", nevmBlock.serialize()])
            elif data[0] == b"nevmconnect":
                evmBlockConnect = CNEVMBlockConnect()
                evmBlockConnect.deserialize(BytesIO(data[1]))
                resBlock = subscriber.addBlock(evmBlockConnect)
                res = b""
                if resBlock:
                    res = b"connected"
                else:
                    res = b"not connected"
                # stay paused during delay test
                while subscriber.artificialDelay == True:
                    sleep(0.1)
                subscriber.send([b"nevmconnect", res])
            elif data[0] == b"nevmdisconnect":
                evmBlockDisconnect = CNEVMBlockDisconnect()
                evmBlockDisconnect.deserialize(BytesIO(data[1]))
                resBlock = subscriber.deleteBlock(evmBlockDisconnect)
                res = b""
                if resBlock:
                    res = b"disconnected"
                else:
                    res = b"not disconnected"
                subscriber.send([b"nevmdisconnect", res])
            else:
                self.log.info("Unknown topic in REQ {}".format(data))
        except zmq.ContextTerminated:
            sleep(1)
            break
        except zmq.ZMQError:
            self.log.warning('zmq error, socket closed unexpectedly.')
            sleep(1)
            break



def thread_generate(self, node):
    self.log.info('thread_generate start')
    node.generatetoaddress(1, ADDRESS_BCRT1_UNSPENDABLE)
    self.log.info('thread_generate done')

# Test may be skipped and not have zmq installed
try:
    import zmq
except ImportError:
    pass

# this simulates the Geth node publisher, publishing back to Syscoin subscriber
class ZMQPublisher:
    def __init__(self, socket):
        self.socket = socket
        self.sysToNEVMBlockMapping = {}
        self.NEVMToSysBlockMapping = {}
        self.artificialDelay = False
        self.doneTest = False

    # Send message to subscriber
    def _send_to_publisher_and_check(self, msg_parts):
        self.socket.send_multipart(msg_parts)

    def receive(self):
        return self.socket.recv_multipart()

    def send(self, msg_parts):
        return self._send_to_publisher_and_check(msg_parts)

    def close(self):
        self.socket.close()

    def addBlock(self, evmBlockConnect):
        # special case if miner is just testing validity of block, sys block hash is 0, we just want to provide message if evm block is valid without updating mappings
        if evmBlockConnect.sysblockhash == 0:
            return True
        # mappings should not already exist, if they do flag the block as invalid
        if self.sysToNEVMBlockMapping.get(evmBlockConnect.sysblockhash) != None or self.NEVMToSysBlockMapping.get(evmBlockConnect.blockhash) != None:
            return False
        self.sysToNEVMBlockMapping[evmBlockConnect.sysblockhash] = evmBlockConnect
        self.NEVMToSysBlockMapping[evmBlockConnect.blockhash] = evmBlockConnect.sysblockhash
        return True

    def deleteBlock(self, evmBlockDisconnect):
        # mappings should already exist on disconnect, if they do not flag the disconnect as invalid
        nevmConnect = self.sysToNEVMBlockMapping.get(evmBlockDisconnect.sysblockhash)
        if nevmConnect == None:
            return False
        sysMappingHash = self.NEVMToSysBlockMapping.get(nevmConnect.blockhash)
        if sysMappingHash == None:
            return False
        # sanity to ensure sys block hashes match so the maps are consistent
        if sysMappingHash != nevmConnect.sysblockhash:
            return False
        
        self.sysToNEVMBlockMapping.pop(evmBlockDisconnect.sysblockhash, None)
        self.NEVMToSysBlockMapping.pop(nevmConnect.blockhash, None)
        return True

    def getLastSYSBlock(self):
        return list(self.NEVMToSysBlockMapping.values())[-1]

    def getLastNEVMBlock(self):
        return self.sysToNEVMBlockMapping[self.getLastSYSBlock()]
    
    def clearMappings(self):
        self.sysToNEVMBlockMapping = {}
        self.NEVMToSysBlockMapping = {}

class ZMQTest (SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        if self.is_wallet_compiled():
            self.requires_wallet = True
        # This test isn't testing txn relay/timing, so set whitelist on the
        # peers for instant txn relay. This speeds up the test run time 2-3x.
        self.extra_args = [["-whitelist=noban@127.0.0.1"]] * self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_py3_zmq()
        self.skip_if_no_syscoind_zmq()

    def run_test(self):
        self.ctx = zmq.Context()
        self.ctxpub = zmq.Context()
        try:
            self.test_basic()
        finally:
            # Destroy the ZMQ context.
            self.log.debug("Destroying ZMQ context")
            self.ctx.destroy(linger=None)
            self.ctxpub.destroy(linger=None)
    # Restart node with the specified zmq notifications enabled, subscribe to
    # all of them and return the corresponding ZMQSubscriber objects.
    def setup_zmq_test(self, address, idx, *, recv_timeout=60, sync_blocks=True):
        socket = self.ctx.socket(zmq.REP)
        subscriber = ZMQPublisher(socket)
        self.extra_args[idx] = ["-zmqpubnevm=%s" % address]

       
        self.restart_node(idx, self.extra_args[idx])

        # set subscriber's desired timeout for the test
        subscriber.socket.bind(address)
        subscriber.socket.set(zmq.RCVTIMEO, recv_timeout*1000)
        return subscriber

    def test_basic(self):
        address = 'tcp://127.0.0.1:29445'
        address1 = 'tcp://127.0.0.1:29443'

        self.log.info("setup subscribers...")
        nevmsub  = self.setup_zmq_test(address, 0)
        nevmsub1 = self.setup_zmq_test(address1, 1)
        self.connect_nodes(0, 1)
        self.sync_blocks()


        num_blocks = 10
        self.log.info("Generate %(n)d blocks (and %(n)d coinbase txes)" % {"n": num_blocks})
        # start the threads to handle pub/sub of SYS/GETH communications
        Thread(target=receive_thread_nevm, args=(self, 0, nevmsub,)).start()
        Thread(target=receive_thread_nevm, args=(self, 1, nevmsub1,)).start()

        self.nodes[0].generatetoaddress(num_blocks, ADDRESS_BCRT1_UNSPENDABLE)
        self.sync_blocks()
        # test simple disconnect, save best block go back to 205 (first NEVM block) and then reconsider back to tip
        bestblockhash = self.nodes[0].getbestblockhash()
        assert_equal(int(bestblockhash, 16), nevmsub.getLastSYSBlock())
        assert_equal(nevmsub1.getLastSYSBlock(), nevmsub.getLastSYSBlock())
        assert_equal(self.nodes[1].getbestblockhash(), bestblockhash)
        # save 205 since when invalidating 206, the best block should be 205
        prevblockhash = self.nodes[0].getblockhash(205)
        blockhash = self.nodes[0].getblockhash(206)
        self.nodes[0].invalidateblock(blockhash)
        self.nodes[1].invalidateblock(blockhash)
        self.sync_blocks()
        # ensure block 205 is the latest on publisher
        assert_equal(int(prevblockhash, 16), nevmsub.getLastSYSBlock())
        assert_equal(nevmsub1.getLastSYSBlock(), nevmsub.getLastSYSBlock())
        # go back to 210 (tip)
        self.nodes[0].reconsiderblock(blockhash)
        self.nodes[1].reconsiderblock(blockhash)
        self.sync_blocks()
        # check that publisher is on the tip (210) again
        assert_equal(int(bestblockhash, 16), nevmsub.getLastSYSBlock())
        assert_equal(self.nodes[1].getbestblockhash(), bestblockhash)
        assert_equal(nevmsub1.getLastSYSBlock(), nevmsub.getLastSYSBlock())
        # restart nodes and check for consistency
        self.log.info('restarting node 0')
        self.restart_node(0, self.extra_args[0])
        self.sync_blocks()
        assert_equal(int(bestblockhash, 16), nevmsub.getLastSYSBlock())
        assert_equal(self.nodes[1].getbestblockhash(), bestblockhash)
        assert_equal(nevmsub1.getLastSYSBlock(), nevmsub.getLastSYSBlock())
        self.log.info('restarting node 1')
        self.restart_node(1, self.extra_args[1])
        self.connect_nodes(0, 1)
        self.sync_blocks()
        assert_equal(int(bestblockhash, 16), nevmsub.getLastSYSBlock())
        assert_equal(self.nodes[1].getbestblockhash(), bestblockhash)
        assert_equal(nevmsub1.getLastSYSBlock(), nevmsub.getLastSYSBlock())
        # reindex nodes and there should be 6 connect messages from blocks 205-210
        # but SYS node does not wait and validate a response, because publisher would have returned "not connected" yet its still OK because its set and forget on sync/reindex
        self.log.info('reindexing node 0')
        self.extra_args[0] += ["-reindex"]
        # clear mappings since reindex should replace
        nevmsub.clearMappings()
        self.restart_node(0, self.extra_args[0])
        self.connect_nodes(0, 1)
        self.sync_blocks()
        assert_equal(int(bestblockhash, 16), nevmsub.getLastSYSBlock())
        assert_equal(self.nodes[1].getbestblockhash(), bestblockhash)
        assert_equal(nevmsub1.getLastSYSBlock(), nevmsub.getLastSYSBlock())
        self.log.info('reindexing node 1')
        self.extra_args[1] += ["-reindex"]
        # clear mappings since reindex should replace
        nevmsub1.clearMappings()
        self.restart_node(1, self.extra_args[1])
        self.connect_nodes(0, 1)
        self.sync_blocks()
        assert_equal(int(bestblockhash, 16), nevmsub.getLastSYSBlock())
        assert_equal(self.nodes[1].getbestblockhash(), bestblockhash)
        assert_equal(nevmsub1.getLastSYSBlock(), nevmsub.getLastSYSBlock())
        # reorg test
        self.disconnect_nodes(0, 1)
        self.log.info("Mine 4 blocks on Node 0")
        self.nodes[0].generatetoaddress(4, ADDRESS_BCRT1_UNSPENDABLE)
        # node1 should have 210 because its disconnected and node0 should have 4 more (214)
        assert_equal(self.nodes[1].getblockcount(), 210)
        assert_equal(self.nodes[0].getblockcount(), 214)
        besthash_n0 = self.nodes[0].getbestblockhash()

        self.log.info("Mine competing 6 blocks on Node 1")
        self.nodes[1].generatetoaddress(6, ADDRESS_BCRT1_UNSPENDABLE)
        assert_equal(self.nodes[1].getblockcount(), 216)

        self.log.info("Connect nodes to force a reorg")
        self.connect_nodes(0, 1)
        self.sync_blocks()
        assert_equal(self.nodes[0].getblockcount(), 216)
        badhash = self.nodes[1].getblockhash(212)

        self.log.info("Invalidate block 2 on node 0 and verify we reorg to node 0's original chain")
        self.nodes[0].invalidateblock(badhash)
        assert_equal(self.nodes[0].getblockcount(), 214)
        assert_equal(self.nodes[0].getbestblockhash(), besthash_n0)
        self.nodes[0].reconsiderblock(badhash)
        self.sync_blocks()
        # test artificially delaying node0 then fork, and remove artificial delay and see node0 gets onto longest chain of node1
        self.log.info("Artificially delaying node0")
        nevmsub.artificialDelay = True
        self.log.info("Generating on node0 in separate thread")
        Thread(target=thread_generate, args=(self, self.nodes[0],)).start()
        self.log.info("Creating re-org and letting node1 become longest chain, node0 should re-org to node0")
        self.nodes[1].generatetoaddress(10, ADDRESS_BCRT1_UNSPENDABLE)
        besthash = self.nodes[1].getbestblockhash()
        nevmsub.artificialDelay = False
        sleep(1)
        self.sync_blocks()
        assert_equal(nevmsub1.getLastSYSBlock(), nevmsub.getLastSYSBlock())
        assert_equal(int(besthash, 16), nevmsub.getLastSYSBlock())
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())
        self.log.info('done')
if __name__ == '__main__':
    ZMQTest().main()
