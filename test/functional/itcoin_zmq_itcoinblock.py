#!/usr/bin/env python3

"""Test the itcoinblock zmq topic.

This test is in addition to the itcoin specific modifications that have been
done in interface_zmq.py. In particular:
- blocks are created in itcoin's signet via the python miner, whereas
  interface_zmq.py uses regtest
- there is a verification that itcoinblock sends notifications even when
  bitcoind is in initial block download mode, while the other topics (for
  example blockhash) do not send notifications when initial block download is
  true.
"""
import datetime
from typing import Any, Dict

from interface_zmq import ZMQSubscriber
from test_framework.test_framework_itcoin import (BaseItcoinTest,
                                                  MsgItcoinblock,
                                                  utc_timestamp)
from test_framework.util import assert_equal, assert_raises

# import itcoin's miner
from test_framework.itcoin_abs_import import import_miner

miner = import_miner()

# Test may be skipped and not have zmq installed
try:
    import zmq
except ImportError:
    pass


class ZMQItcoinblockIBD(BaseItcoinTest):
    def set_test_params(self):
        self.num_nodes = 1
        self.signet_num_signers = 1
        self.signet_num_signatures = 1
        super().set_test_params()

    def mine_and_getblockchaininfo(self, blocktime: int) -> Dict[str, Any]:
        args0 = self.node(0).args
        block, _, _ = miner.do_generate_next_block(args0, blocktime=blocktime)
        signed_block = miner.do_sign_block(args0, block, self.signet_challenge)
        miner.do_propagate_block(args0, signed_block)
        return self.nodes[0].getblockchaininfo()

    def skip_test_if_missing_module(self):
        self.skip_if_no_py3_zmq()
        self.skip_if_no_bitcoind_zmq()

    def run_test(self):
        self.ctx = zmq.Context()
        try:
            address = 'tcp://127.0.0.1:28332'
            subs = self.setup_zmq_test([(topic, address) for topic in ["hashblock", "itcoinblock"]])
            self.log.info("Check that itcoinblock notifications are sent even in IBD and verify their content")

            # Verify initial state
            self.assert_blockchaininfo_property_forall_nodes("blocks", 0)

            hashblock = subs[0]
            itcoinblock = subs[1]

            # =============================
            # Mine the 1st block at 2021-01-01 00:00:00
            blocktime = utc_timestamp(2021, 1, 1, 0, 0, 0)
            blockchaininfo = self.mine_and_getblockchaininfo(blocktime)
            # On the chain:
            # - the block time will exactly be the one we submitted
            # - we will be in Initial Block Download
            assert_equal(blockchaininfo["blocks"], 1)
            assert_equal(blockchaininfo["time"], blocktime)
            assert_equal(blockchaininfo["initialblockdownload"], True)
            # On zmq:
            # - an itcoinblock message for block at height 1 will be received, even
            #   if we are in initial block download
            # - no hashblock message will be received, because we are in initial
            #   block download
            msg_itcoinblock = MsgItcoinblock.from_bin_buf(itcoinblock.receive())
            assert_equal(msg_itcoinblock.blockhash_hex(), blockchaininfo["bestblockhash"])
            assert_equal(msg_itcoinblock.height, 1)
            assert_equal(msg_itcoinblock.nTime, blocktime)
            assert_raises(zmq.Again, hashblock.receive)

            # =============================
            # Mine the 2nd block at current date
            blocktime = int(datetime.datetime.today().timestamp())
            blockchaininfo = self.mine_and_getblockchaininfo(blocktime)
            # On the chain:
            # - the block time will exactly be the one we submitted
            # - we will no longer be in Initial Block Download
            assert_equal(blockchaininfo["blocks"], 2)
            assert_equal(blockchaininfo["time"], blocktime)
            assert_equal(blockchaininfo["initialblockdownload"], False)
            # On zmq:
            # - an itcoinblock message for block at height 2 will be received
            # - now that we are out of Initial Block Download, also an hashblock
            #   message will be received
            received_itcoinblock = MsgItcoinblock.from_bin_buf(itcoinblock.receive())
            assert_equal(received_itcoinblock.blockhash_hex(), blockchaininfo["bestblockhash"])
            assert_equal(received_itcoinblock.height, 2)
            assert_equal(received_itcoinblock.nTime, blocktime)
            received_hashblock = hashblock.receive().hex()
            assert_equal(received_hashblock, blockchaininfo["bestblockhash"])
        finally:
            # Destroy the ZMQ context.
            self.log.debug("Destroying ZMQ context")
            self.ctx.destroy(linger=None)

    # Restart node with the specified zmq notifications enabled, subscribe to
    # all of them and return the corresponding ZMQSubscriber objects.
    def setup_zmq_test(self, services, *, recv_timeout=60):
        subscribers = []
        for topic, address in services:
            socket = self.ctx.socket(zmq.SUB)
            subscribers.append(ZMQSubscriber(socket, topic.encode(), zmq.NOBLOCK))

        self.restart_node(0, [f"-zmqpub{topic}={address}" for topic, address in services] +
                             self.extra_args[0])

        for i, sub in enumerate(subscribers):
            sub.socket.connect(services[i][1])

        # set subscriber's desired timeout for the test
        for sub in subscribers:
            sub.socket.set(zmq.RCVTIMEO, recv_timeout*1000)

        return subscribers


if __name__ == '__main__':
    ZMQItcoinblockIBD().main()
