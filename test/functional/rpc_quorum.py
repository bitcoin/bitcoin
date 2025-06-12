#!/usr/bin/env python3
# Copyright (c) 2022-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.test_framework import (
    DashTestFramework,
    MasternodeInfo,
)
from test_framework.util import assert_equal

'''
rpc_quorum.py

Test "quorum" rpc subcommands
'''

class RPCMasternodeTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(4, 3)

    def run_test(self):
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()
        quorum_hash = self.mine_quorum()

        quorum_info = self.nodes[0].quorum("info", 100, quorum_hash)
        for idx in range(0, self.mn_count):
            mn: MasternodeInfo = self.mninfo[idx]
            for member in quorum_info["members"]:
                if member["proTxHash"] == mn.proTxHash:
                    assert_equal(member["service"], f'127.0.0.1:{mn.nodePort}')

if __name__ == '__main__':
    RPCMasternodeTest().main()
