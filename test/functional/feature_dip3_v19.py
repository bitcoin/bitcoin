#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_dip3_v19.py

Checks DIP3 for v19

'''
from test_framework.test_framework import DashTestFramework
from test_framework.util import (
    assert_equal,
    p2p_port,
    force_finish_mnsync
)


class DIP3V19Test(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 5, [["-whitelist=noban@127.0.0.1"]] * 6, fast_dip3_enforcement=True)
        self.extra_args += [['-dip19params=200'],['-dip19params=200'],['-dip19params=200'],['-dip19params=200'],['-dip19params=200'],['-dip19params=200']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()

    def run_test(self):
        # Connect all nodes to node1 so that we always have the whole network connected
        # Otherwise only masternode connections will be established between nodes, which won't propagate TXs/blocks
        # Usually node0 is the one that does this, but in this test we isolate it multiple times

        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

   
        self.move_to_next_cycle()
        self.log.info("Cycle H height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+C height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+2C height:" + str(self.nodes[0].getblockcount()))
        self.mine_quorum()

        for mn in self.mninfo:
            self.log.info(f"Trying to update MN payee")
            self.update_mn_payee(mn, self.nodes[0].getnewaddress())

            self.log.info(f"Trying to update MN service")
            self.test_protx_update_service(mn)

        mn = self.mninfo[-1]
        revoke_protx = mn.proTxHash
        revoke_keyoperator = mn.keyOperator
        self.log.info(f"Trying to revoke proTx:{revoke_protx}")
        self.test_revoke_protx(revoke_protx, revoke_keyoperator)

        self.mine_quorum()

        return

    def test_revoke_protx(self, revoke_protx, revoke_keyoperator):
        funds_address = self.nodes[0].getnewaddress()
        self.nodes[0].sendtoaddress(funds_address, 1)
        self.generate(self.nodes[0], 1)
        self.nodes[0].protx_revoke(revoke_protx, revoke_keyoperator, 1, funds_address)
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        self.log.info(f"Successfully revoked={revoke_protx}")
        for mn in self.mninfo:
            if mn.proTxHash == revoke_protx:
                self.mninfo.remove(mn)
                return
        for i in range(len(self.nodes)):
            if i != 0:
                self.connect_nodes(i, 0)

    def update_mn_payee(self, mn, payee):
        self.nodes[0].sendtoaddress(mn.collateral_address, 0.001)
        self.nodes[0].protx_update_registrar(mn.proTxHash, '', '', payee, mn.collateral_address)
        self.generate(self.nodes[0], 1)
        info = self.nodes[0].protx_info(mn.proTxHash)
        assert info['state']['payoutAddress'] == payee

    def test_protx_update_service(self, mn):
        self.nodes[0].sendtoaddress(mn.collateral_address, 0.001)
        self.nodes[0].protx_update_service( mn.proTxHash, '127.0.0.2:%d' % p2p_port(mn.nodeIdx), mn.keyOperator, "", mn.collateral_address)
        self.generate(self.nodes[0], 1)
        for node in self.nodes:
            protx_info = node.protx_info( mn.proTxHash)
            assert_equal(protx_info['state']['service'], '127.0.0.2:%d' % p2p_port(mn.nodeIdx))

        # undo
        self.nodes[0].protx_update_service(mn.proTxHash, '127.0.0.1:%d' % p2p_port(mn.nodeIdx), mn.keyOperator, "", mn.collateral_address)
        self.generate(self.nodes[0], 1)


if __name__ == '__main__':
    DIP3V19Test().main()
