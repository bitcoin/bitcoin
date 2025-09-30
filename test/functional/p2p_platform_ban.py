#!/usr/bin/env python3
# Copyright (c) 2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.messages import msg_platformban, hash256, ser_string, ser_uint256
from test_framework.p2p import (
    p2p_lock,
    P2PInterface,
)
from test_framework.test_framework import DashTestFramework
from test_framework.util import wait_until_helper

import struct

class PlatformBanInterface(P2PInterface):
    def __init__(self):
        super().__init__()


class PlatformBanMessagesTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(1, 0, [[]], evo_count=3)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def check_banned(self, mn):
        info = self.nodes[0].protx('info', mn.proTxHash)
        return info['state']['PoSeBanHeight'] != -1

    def run_test(self):
        node = self.nodes[0]

        node.sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        node.sporkupdate("SPORK_23_QUORUM_POSE", 0)
        self.wait_for_sporks_same()

        evo_info_0 = self.dynamically_add_masternode(evo=True)
        for _ in range(2):
            self.dynamically_add_masternode(evo=True)

        self.mempool_size = 0

        self.mine_quorum(llmq_type_name='llmq_test_platform', llmq_type=106)

        self.log.info("Create and sign platform-ban message for mn-0")
        msg = msg_platformban()
        msg.protx_hash = int(self.mninfo[0].proTxHash, 16)
        msg.requested_height = node.getblockcount()

        request_id_buf = ser_string(b"PlatformPoSeBan") + ser_uint256(msg.protx_hash) + struct.pack("<I", msg.requested_height)
        request_id = hash256(request_id_buf)[::-1].hex()

        quorum_hash = self.mninfo[1].get_node(self).quorum("selectquorum", 106, request_id)["quorumHash"]
        msg.quorum_hash = int(quorum_hash, 16)

        msg_hash = format(msg.calc_sha256(), '064x')

        recsig = self.get_recovered_sig(request_id, msg_hash, llmq_type=106, use_platformsign=True)
        msg.sig = bytearray.fromhex(recsig["sig"])

        self.log.info("Platform ban message is created and signed")
        p2p_node2 = self.mninfo[2].get_node(self).add_p2p_connection(PlatformBanInterface())
        p2p_node2.send_message(msg)
        wait_until_helper(lambda: p2p_node2.message_count["platformban"] > 0, timeout=10, lock=p2p_lock)
        p2p_node2.message_count["platformban"] = 0

        assert not self.check_banned(self.mninfo[0])

        mninfos_valid = self.mninfo.copy()[1:]
        self.mine_quorum(llmq_type_name='llmq_test_platform', expected_members=2, expected_connections=1, expected_contributions=2, expected_commitments=2, llmq_type=106, mninfos_valid=mninfos_valid)

        p2p_node = node.add_p2p_connection(PlatformBanInterface())
        p2p_node.send_message(msg)
        self.mine_quorum(llmq_type_name='llmq_test_platform', expected_members=2, expected_connections=1, expected_contributions=2, expected_commitments=2, llmq_type=106, mninfos_valid=mninfos_valid)
        assert self.check_banned(self.mninfo[0])

        self.dynamically_evo_update_service(evo_info_0)
        assert not self.check_banned(self.mninfo[0])


if __name__ == '__main__':
    PlatformBanMessagesTest().main()
