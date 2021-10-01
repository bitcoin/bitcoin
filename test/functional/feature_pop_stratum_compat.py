#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2021 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Start 1 node.
Run "getblocktemplate" at node[0] height == 0.
Expect to return no POP fields in JSON response.
Mine until POP is activated.
Submit 1 ATV, 1 VTB, 1 VBK (valid) to POP mempool.
Run "getblocktemplate".
Expect to get stratum-related fields in response.
"""

from test_framework.pop import endorse_block, create_endorsed_chain, mine_until_pop_active
from test_framework.authproxy import JSONRPCException
from test_framework.pop_const import POP_PAYOUT_DELAY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
    disconnect_nodes, assert_equal, assert_raises_rpc_error,
)
import json

def assert_no_field(source, key):
    assert isinstance(source, dict)
    if key in source:
        raise Exception("Key {} expected to be missing in {}, but it is present: {}".format(key, source, source[key]))

def assert_field_exists(source, key, type):
    assert isinstance(source, dict)
    if key not in source:
        raise Exception("Expected key {} to exist in {}".format(key, source))

    try:
        if not type(source[key]):
            raise Exception("Key {} type verification failed. Value={}".format(key, source[key]))
    except Exception as e:
        print(e)
        raise

'''
If this test fails, then compatibility with Stratum (https://github.com/VeriBlock/vbk-ri-stratum-pool) is broken.
'''
class PopStratumCompat(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()

    def getblocktemplate(self):
        arg = {
            "capabilities": ["coinbasetxn", "workid", "coinbase/append"],
            "rules": ["segwit"]
        }
        return self.nodes[0].getblocktemplate(arg)

    def run_test(self):
        """Main test logic"""
        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()

        node = self.nodes[0]
        addr = node.getnewaddress()

        # stop 1 block behind activation
        self.log.info("Mining blocks until activation -5 blocks")
        self.log.info("Current node[0] height {}".format(node.getblockcount()))
        mine_until_pop_active(node, addr, delta=-5)
        self.log.info("Current node[0] height {}".format(node.getblockcount()))
        # check that getblocktemplate does NOT have pop-related fields before POP activation

        self.log.info("Check that getblocktemplate does not have POP fields")
        resp = self.getblocktemplate()
        assert_no_field(resp, 'pop_data')
        assert_no_field(resp, 'pop_data_root')
        assert_no_field(resp, 'pop_first_previous_keystone')
        assert_no_field(resp, 'pop_second_previous_keystone')
        assert_no_field(resp, 'pop_rewards')

        self.log.info("Mining blocks until activation +5 blocks")
        self.log.info("Current node[0] height {}".format(node.getblockcount()))
        mine_until_pop_active(node, addr, delta=+5)
        self.log.info("Current node[0] height {}".format(node.getblockcount()))

        self.log.info("Mine chain of {} consecutive endorsed blocks".format(POP_PAYOUT_DELAY))
        create_endorsed_chain(node, self.apm, POP_PAYOUT_DELAY, addr)
        self.log.info("Current node[0] height {}".format(node.getblockcount()))
        endorse_block(self.nodes[0], self.apm, node.getblockcount() - 5, addr)
        self.log.info("Current node[0] height {}".format(node.getblockcount()))
        self.log.info("Check that getblocktemplate does have POP fields")
        resp = self.getblocktemplate()

        is_dict = lambda x: isinstance(x, dict)
        is_list = lambda x: isinstance(x, list)
        is_int = lambda x: isinstance(x, int)
        is_hex = lambda x: bytes.fromhex(x)
        is_payload = lambda x: is_dict(x) and "id" in x and "serialized" in x
        is_payload_list = lambda x: is_list(x) and all(is_payload(p) for p in x)

        assert_field_exists(resp, 'pop_data', type=is_dict)
        assert_field_exists(resp['pop_data'], 'atvs', type=is_payload_list)
        assert_field_exists(resp['pop_data'], 'vtbs', type=is_payload_list)
        assert_field_exists(resp['pop_data'], 'vbkblocks', type=is_payload_list)
        assert_field_exists(resp['pop_data'], 'version', type=is_int)
        assert_field_exists(resp, 'pop_data_root', type=is_hex)
        assert_field_exists(resp, 'pop_first_previous_keystone', type=is_hex)
        assert_field_exists(resp, 'pop_second_previous_keystone', type=is_hex)
        assert_field_exists(resp, 'pop_rewards', type=is_list)
        for reward in resp['pop_rewards']:
            assert_field_exists(reward, 'amount', type=is_int)
            assert_field_exists(reward, 'payout_info', type=is_hex)
        self.log.info("Current node[0] height {}".format(node.getblockcount()))


if __name__ == '__main__':
    PopStratumCompat().main()
