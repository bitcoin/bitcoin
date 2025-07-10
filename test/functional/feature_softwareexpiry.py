#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Knots developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.blocktools import (
    NORMAL_GBT_REQUEST_PARAMS,
    create_block,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

ONE_WEEK = 604800
EXPIRED_BLOCKS_ACCEPTED = 144

class SoftwareExpiryTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def run_test(self):
        nodes = self.nodes
        addr = nodes[0].get_deterministic_priv_key().address  # what address is irrelevant

        def setmocktime(t):
            self.mocktime = t
            for i in (0, 1):
                nodes[i].setmocktime(t)

        blocktime = nodes[0].getblock(nodes[0].getbestblockhash(), 1)['time']
        self.mocktime = blocktime + 300
        expirytime = self.mocktime + ONE_WEEK + 1
        self.restart_node(0, extra_args=[f'-mocktime={self.mocktime}', f'-softwareexpiry={expirytime}'])
        self.restart_node(1, extra_args=[f'-mocktime={self.mocktime}',])
        self.connect_nodes(1, 0)

        self.log.info("Checking everything works normal exactly at expiry time")
        setmocktime(expirytime)
        block = create_block(tmpl=nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS))
        block.solve()
        nodes[0].submitblock(block.serialize().hex())
        self.sync_blocks(nodes)

        self.log.info("When expiry is passed, we should get errors, but blocks should still be accepted")
        setmocktime(expirytime + 1)
        assert_raises_rpc_error(-9, 'node software has expired', nodes[0].getblocktemplate, NORMAL_GBT_REQUEST_PARAMS)
        blockhash = self.generatetoaddress(nodes[1], 1, addr)[0]
        blockinfo = nodes[1].getblock(blockhash, 1)
        assert blockinfo['time'] > expirytime

        self.log.info(f"After {EXPIRED_BLOCKS_ACCEPTED} blocks with expired median-time-past, we should begin rejecting blocks")
        while blockinfo['mediantime'] <= expirytime:
            blockhash = self.generatetoaddress(nodes[1], 1, addr, sync_fun=self.no_op)[0]
            blockinfo = nodes[1].getblock(blockhash, 1)
        blockhash = self.generatetoaddress(nodes[1], EXPIRED_BLOCKS_ACCEPTED, addr)[-1]
        with nodes[0].assert_debug_log(['node-expired'], unexpected_msgs=['UpdatedBlockTip: new block']):
            new_blockhash = self.generatetoaddress(nodes[1], 1, addr, sync_fun=self.no_op)[0]
        assert_equal(nodes[0].getbestblockhash(), blockhash)

        self.log.info("Restarting the node should fail")
        assert self.mocktime > expirytime
        self.stop_node(0)
        msg = 'Error: This software is expired, and may be out of consensus. You must choose to upgrade or override this expiration.'
        nodes[0].assert_start_raises_init_error(extra_args=[f'-mocktime={self.mocktime}', f'-softwareexpiry={expirytime}'], expected_msg=msg)

        self.log.info("Restarting the node with a later expiry should succeed and accept the newer block")
        expirytime = nodes[1].getblock(new_blockhash, 1)['time']
        self.restart_node(0, extra_args=[f'-mocktime={self.mocktime}', f'-softwareexpiry={expirytime}'])
        self.connect_nodes(1, 0)
        self.sync_blocks(nodes)


if __name__ == "__main__":
    SoftwareExpiryTest(__file__).main()
