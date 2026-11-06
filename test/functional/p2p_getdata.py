#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test GETDATA processing behavior"""
from collections import defaultdict

from test_framework.messages import (
    CBlock,
    CInv,
    MSG_BLOCK,
    MSG_WITNESS_FLAG,
    from_binary,
    msg_block,
    msg_getdata,
)
from test_framework.p2p import MESSAGEMAP, P2PInterface, p2p_lock
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class RawBlockMessage(msg_block):
    def deserialize(self, stream):
        self.raw = stream.getvalue()
        super().deserialize(stream)


class P2PNoInv(P2PInterface):
    # Only request blocks explicitly so tip announcements cannot overwrite the checked reply.
    def on_inv(self, message): pass


class P2PStoreBlock(P2PInterface):
    def __init__(self):
        super().__init__()
        self.blocks = defaultdict(int)

    def on_block(self, message):
        self.blocks[message.block.hash_int] += 1


class GetdataTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def test_invalid_getdata(self):
        p2p_block_store = self.nodes[0].add_p2p_connection(P2PStoreBlock())

        self.log.info("test that an invalid GETDATA doesn't prevent processing of future messages")

        # Send invalid message and verify that node responds to later ping
        invalid_getdata = msg_getdata()
        invalid_getdata.inv.append(CInv(t=0, h=0))  # INV type 0 is invalid.
        p2p_block_store.send_and_ping(invalid_getdata)

        # Check getdata still works by fetching tip block
        best_block = int(self.nodes[0].getbestblockhash(), 16)
        good_getdata = msg_getdata()
        good_getdata.inv.append(CInv(t=2, h=best_block))
        p2p_block_store.send_and_ping(good_getdata)
        p2p_block_store.wait_until(lambda: p2p_block_store.blocks[best_block] == 1)

    def test_block_serialization(self):
        self.log.info("Check exact witness and non-witness block payloads from cache and disk")
        node = self.nodes[0]
        wallet = MiniWallet(node)
        wallet.send_self_transfer(from_node=node)
        blockhash = self.generate(wallet, 1)[0]
        full = bytes.fromhex(node.getblock(blockhash, 0))
        assert len(from_binary(CBlock, full).serialize(with_witness=False)) < len(full)
        peer = node.add_p2p_connection(P2PNoInv())

        def check_payloads(blockhash, full):
            stripped = from_binary(CBlock, full).serialize(with_witness=False)
            hash_int = int(blockhash, 16)
            for inv_type, expected in ((MSG_BLOCK, stripped), (MSG_BLOCK | MSG_WITNESS_FLAG, full)):
                with p2p_lock:
                    peer.last_message.pop("block", None)
                peer.send_and_ping(msg_getdata([CInv(inv_type, hash_int)]))
                with p2p_lock:
                    assert_equal(peer.last_message["block"].raw, expected)

        check_payloads(blockhash, full)
        self.generate(wallet, 1)  # Evict the requested block from the most-recent-block cache
        check_payloads(blockhash, full)

        genesis = node.getblockhash(0)
        check_payloads(genesis, bytes.fromhex(node.getblock(genesis, 0)))

    def run_test(self):
        MESSAGEMAP[b"block"] = RawBlockMessage
        self.test_invalid_getdata()
        self.test_block_serialization()


if __name__ == '__main__':
    GetdataTest(__file__).main()
