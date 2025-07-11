#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Knots developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import platform
import re
import time

from test_framework.blocktools import (
    NORMAL_GBT_REQUEST_PARAMS,
    create_block,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

PRERELEASE_WARNING = 'This is a pre-release test build - use at your own risk - do not use for mining or merchant applications'

SECONDS_PER_WEEK = 604800
SOFTWARE_EXPIRY_WARN_PERIOD = SECONDS_PER_WEEK * 4
EXPIRED_BLOCKS_ACCEPTED = 144

class SoftwareExpiryTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def get_warnings(self, node):
        warnings = node.getnetworkinfo()['warnings']
        try:
            warnings.remove(PRERELEASE_WARNING)
        except ValueError:
            pass
        return warnings

    def check_warning(self, node, alerts_path, re_warning_stderr_prefix, re_expected_warning):
        warnings = "\n".join(self.get_warnings(node))
        assert re.match(re_expected_warning, warnings)
        node.stderr.seek(0)
        stderr = node.stderr.read().decode('utf-8').strip()
        assert re.match(re_warning_stderr_prefix + re_expected_warning, stderr)
        assert alerts_path.exists()
        with open(alerts_path, 'r', encoding='utf8') as f:
            alerts_data = f.read()
            if platform.system() == 'Windows':
                # The echo command on Windows includes the quotes and space in the output
                assert alerts_data[0] == '"' and alerts_data[-3:] == "\" \n"
                alerts_data = alerts_data[1:-3]
            assert re.match('^' + re_expected_warning, alerts_data)
        alerts_path.unlink()
        return stderr

    def run_test(self):
        nodes = self.nodes
        addr = nodes[0].get_deterministic_priv_key().address  # what address is irrelevant
        alerts_path = self.nodes[0].datadir_path / 'alerts'

        def setmocktime(t):
            self.mocktime = t
            for i in (0, 1):
                nodes[i].setmocktime(t)

        blocktime = nodes[0].getblock(nodes[0].getbestblockhash(), 1)['time']
        self.mocktime = blocktime + 300
        expirytime = self.mocktime + SOFTWARE_EXPIRY_WARN_PERIOD + 60
        self.restart_node(0, extra_args=[
            f'-mocktime={self.mocktime}',
            f'-softwareexpiry={expirytime}',
            f"-alertnotify=echo %s >> {alerts_path}",
        ])
        self.restart_node(1, extra_args=[f'-mocktime={self.mocktime}',])
        self.connect_nodes(1, 0)

        self.log.info("Everything normal and no alerts over 4 weeks prior to expiry")
        nodes[0].mockscheduler(5)
        time.sleep(1)
        block = create_block(tmpl=nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS))
        block.solve()
        nodes[0].submitblock(block.serialize().hex())
        self.sync_blocks(nodes)
        assert_equal(self.get_warnings(nodes[0]), [])
        nodes[0].stderr.seek(0)
        assert_equal(nodes[0].stderr.read().decode('utf-8').strip(), '')
        assert not alerts_path.exists()

        self.log.info("Once we're within 4 weeks, warnings should trigger")
        nodes[0].mockscheduler(60)
        time.sleep(1)
        re_expected_warning = r'This software expires soon, and may fall out of consensus. Before .*, you must choose to upgrade or override this expiration.$'
        stderr = self.check_warning(nodes[0], alerts_path, r'^Warning: ', re_expected_warning)

        self.log.info("Checking everything still works normal exactly at expiry time")
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
        self.stop_node(0, expected_stderr=stderr)
        msg = 'Error: This software is expired, and may be out of consensus. You must choose to upgrade or override this expiration.'
        nodes[0].assert_start_raises_init_error(extra_args=[f'-mocktime={self.mocktime}', f'-softwareexpiry={expirytime}'], expected_msg=msg)

        self.log.info("Restarting the node with a later expiry should succeed and accept the newer block")
        expirytime = nodes[1].getblock(new_blockhash, 1)['time']
        assert expirytime - SOFTWARE_EXPIRY_WARN_PERIOD < self.mocktime <= expirytime
        self.restart_node(0, extra_args=[
            f'-mocktime={self.mocktime}',
            f'-softwareexpiry={expirytime}',
            f"-alertnotify=echo %s >> {alerts_path}",
        ])
        self.connect_nodes(1, 0)
        nodes[0].mockscheduler(5)
        time.sleep(1)
        self.sync_blocks(nodes)

        # But we're still close enough to get warned
        stderr = self.check_warning(nodes[0], alerts_path, r'^Warning: ', re_expected_warning)

        self.stop_node(0, expected_stderr=stderr)
        self.log.info("Checking no unexpected alerts were triggered")
        assert not alerts_path.exists()


if __name__ == "__main__":
    SoftwareExpiryTest(__file__).main()
