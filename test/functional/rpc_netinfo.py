#!/usr/bin/env python3
# Copyright (c) 2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test network information fields across RPCs."""

from test_framework.util import (
    assert_equal,
    softfork_active
)
from test_framework.script import (
    hash160
)
from test_framework.test_framework import (
    BitcoinTestFramework,
    MasternodeInfo,
    p2p_port
)
from test_framework.test_node import TestNode

from _decimal import Decimal
from random import randint
from typing import List, Optional

# Height at which BIP9 deployment DEPLOYMENT_V23 is activated
V23_ACTIVATION_THRESHOLD = 100
# See CMainParams in src/chainparams.cpp
DEFAULT_PORT_MAINNET_CORE_P2P = 9999
# See CRegTestParams in src/chainparams.cpp
DEFAULT_PORT_PLATFORM_P2P = 22200
DEFAULT_PORT_PLATFORM_HTTP = 22201
# See CDeterministicMNStateDiff::ToJson() in src/evo/dmnstate.h
DMNSTATE_DIFF_DUMMY_ADDR = "255.255.255.255"
# See ProTxVersion in src/evo/providertx.h
PROTXVER_BASIC = 2
PROTXVER_EXTADDR = 3

class EvoNode:
    mn: MasternodeInfo
    node: TestNode
    platform_nodeid: str = ""

    def __init__(self, node: TestNode):
        self.mn = MasternodeInfo(evo=True, legacy=False)
        self.mn.generate_addresses(node)
        self.mn.set_node(node.index)
        self.mn.set_params(nodePort=p2p_port(node.index))
        self.node = node

    def generate_collateral(self, test: BitcoinTestFramework):
        assert self.mn.nodeIdx is not None
        # Generate enough blocks to cover collateral amount
        while self.node.getbalance() < self.mn.get_collateral_value():
            test.bump_mocktime(1)
            test.generate(self.node, 10, sync_fun=test.no_op)
        # Create collateral UTXO
        collateral_txid = self.node.sendmany("", {self.mn.collateral_address: self.mn.get_collateral_value(), self.mn.fundsAddr: 1})
        self.mn.bury_tx(test, self.mn.nodeIdx, collateral_txid, 1)
        collateral_vout = self.mn.get_collateral_vout(self.node, collateral_txid)
        self.mn.set_params(collateral_txid=collateral_txid, collateral_vout=collateral_vout)

    def is_mn_visible(self, _protx_hash = None) -> bool:
        protx_hash = _protx_hash or self.mn.proTxHash
        mn_list = self.node.masternodelist()
        mn_visible = False
        for mn_entry in mn_list:
            dmn = mn_list.get(mn_entry)
            if dmn['proTxHash'] == protx_hash:
                assert_equal(dmn['type'], "Evo")
                mn_visible = True
        return mn_visible

    def set_active_state(self, test: BitcoinTestFramework, active: bool, more_extra_args: Optional[List[str]] = None):
        assert self.mn.proTxHash and self.mn.keyOperator
        target_extra_args = self.node.extra_args.copy()
        if active:
            target_extra_args += [f'-masternodeblsprivkey={self.mn.keyOperator}']
        if more_extra_args is not None:
            target_extra_args += more_extra_args
        test.restart_node(self.mn.nodeIdx, extra_args=target_extra_args)

    def register_mn(self, test: BitcoinTestFramework, submit: bool, addrs_core_p2p, addrs_platform_p2p, addrs_platform_https, code = None, msg = None) -> Optional[str]:
        assert self.mn.nodeIdx is not None
        self.platform_nodeid = hash160(b'%d' % randint(1, 65535)).hex()
        protx_output = self.mn.register(self.node, submit=submit, addrs_core_p2p=addrs_core_p2p, operator_reward=0,
                                        platform_node_id=self.platform_nodeid, addrs_platform_p2p=addrs_platform_p2p,
                                        addrs_platform_https=addrs_platform_https, expected_assert_code=code, expected_assert_msg=msg)
        if (code and msg) or not submit:
            return protx_output
        assert protx_output is not None
        # Bury ProTx transaction and check if masternode is online
        self.mn.set_params(proTxHash=protx_output, operator_reward=0)
        self.mn.bury_tx(test, self.mn.nodeIdx, protx_output, 1)
        assert_equal(self.is_mn_visible(), True)
        test.log.debug(f"Registered EvoNode with collateral_txid={self.mn.collateral_txid}, collateral_vout={self.mn.collateral_vout}, provider_txid={self.mn.proTxHash}")
        return self.mn.proTxHash

    def update_mn(self, test: BitcoinTestFramework, submit: bool, addrs_core_p2p, addrs_platform_p2p, addrs_platform_https, code = None, msg = None) -> Optional[str]:
        assert self.mn.nodeIdx is not None
        protx_output = self.mn.update_service(self.node, submit=submit, addrs_core_p2p=addrs_core_p2p, platform_node_id=self.platform_nodeid,
                                              addrs_platform_p2p=addrs_platform_p2p, addrs_platform_https=addrs_platform_https,
                                              expected_assert_code=code, expected_assert_msg=msg)
        if (code and msg) or not submit:
            return protx_output
        assert protx_output is not None
        self.mn.bury_tx(test, self.mn.nodeIdx, protx_output, 1)
        assert_equal(self.is_mn_visible(), True)
        test.log.debug(f"Updated EvoNode with collateral_txid={self.mn.collateral_txid}, collateral_vout={self.mn.collateral_vout}, provider_txid={self.mn.proTxHash}")
        return protx_output

    def destroy_mn(self, test: BitcoinTestFramework):
        assert self.mn.nodeIdx is not None
        # Get UTXO from address used to pay fees, generate new addresses
        address_funds_unspent = self.node.listunspent(0, 99999, [self.mn.fundsAddr])[0]
        address_funds_value = address_funds_unspent['amount']
        self.mn.generate_addresses(self.node, True)
        # Create transaction to spend old collateral and fee change
        raw_tx = self.node.createrawtransaction([
                { 'txid': self.mn.collateral_txid, 'vout': self.mn.collateral_vout },
                { 'txid': address_funds_unspent['txid'], 'vout': address_funds_unspent['vout'] }
            ], [
                {self.mn.collateral_address: float(self.mn.get_collateral_value())},
                {self.mn.fundsAddr: float(address_funds_value - Decimal(0.001))}
            ])
        raw_tx = self.node.signrawtransactionwithwallet(raw_tx)['hex']
        # Send that transaction, resulting txid is new collateral
        new_collateral_txid = self.node.sendrawtransaction(raw_tx)
        self.mn.bury_tx(test, self.mn.nodeIdx, new_collateral_txid, 1)
        new_collateral_vout = self.mn.get_collateral_vout(self.node, new_collateral_txid)
        old_protx_hash = self.mn.proTxHash
        self.mn.set_params(proTxHash="", collateral_txid=new_collateral_txid, collateral_vout=new_collateral_vout)
        # Old masternode entry should be dead
        assert_equal(self.is_mn_visible(old_protx_hash), False)
        test.log.debug(f"Destroyed EvoNode with collateral_txid={self.mn.collateral_txid}, collateral_vout={self.mn.collateral_vout}, provider_txid={old_protx_hash}")

class NetInfoTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [[
            "-dip3params=2:2", f"-vbparams=v23:{self.mocktime}:999999999999:{V23_ACTIVATION_THRESHOLD}:10:8:6:5:0"
        ] for _ in range(self.num_nodes)]
        self.extra_args[2] += ["-deprecatedrpc=service"]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def activate_v23(self):
        batch_size: int = 50
        while not softfork_active(self.nodes[0], "v23"):
            self.bump_mocktime(batch_size)
            self.generate(self.nodes[0], batch_size, sync_fun=lambda: self.sync_blocks())
        assert softfork_active(self.nodes[0], "v23")

    def check_netinfo_fields(self, val, core_p2p_port: int, plat_https_port: Optional[int], plat_p2p_port: Optional[int]):
        assert_equal(val['core_p2p'][0], f"127.0.0.1:{core_p2p_port}")
        if plat_https_port:
            assert_equal(val['platform_https'][0], f"127.0.0.1:{plat_https_port}")
        if plat_p2p_port:
            assert_equal(val['platform_p2p'][0], f"127.0.0.1:{plat_p2p_port}")

    def extract_from_listdiff(self, rpc_output, proregtx_hash):
        for arr in rpc_output['updatedMNs']:
            if proregtx_hash in arr.keys():
                return arr[proregtx_hash]
        raise AssertionError(f"Unable to find {proregtx_hash} in array")

    def reconnect_nodes(self):
        # Needed as restarts don't reconnect nodes
        for idx in range(1, self.num_nodes):
            self.connect_nodes(0, idx)

    def run_test(self):
        # Used for register RPC tests and payload reporting tests
        self.node_evo: EvoNode = EvoNode(self.nodes[0])
        self.node_evo.generate_collateral(self)
        # Used for update RPC tests
        self.node_two: EvoNode = EvoNode(self.nodes[1])
        self.node_two.generate_collateral(self)
        self.node_two.register_mn(self, True, "", DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP)
        # Used to check deprecated field behavior
        self.node_simple: TestNode = self.nodes[2]
        # Test routines
        self.log.info("Test input validation for masternode address fields (pre-fork)")
        self.test_validation_common()
        self.test_validation_legacy()
        self.log.info("Test output masternode address fields for consistency (pre-fork)")
        self.test_deprecation()
        self.log.info("Mine blocks to activate DEPLOYMENT_V23")
        self.activate_v23()
        self.log.info("Test input validation for masternode address fields (post-fork)")
        self.test_validation_common()
        self.test_validation_extended()
        self.log.info("Test masternode address fields for ProRegTx without addresses")
        self.test_empty_fields()
        self.log.info("Test output masternode address fields for consistency (post-fork)")
        self.test_shims()

    def test_validation_common(self):
        # Arrays of addresses with invalid inputs get refused
        self.node_evo.register_mn(self, False, [[f"127.0.0.1:{self.node_evo.mn.nodePort}"]],
                                  DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for coreP2PAddrs[0], must be string")
        self.node_evo.register_mn(self, False, [f"127.0.0.1:{self.node_evo.mn.nodePort}", ""],
                                  DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for coreP2PAddrs[1], cannot be empty")
        self.node_evo.register_mn(self, False, [f"127.0.0.1:{self.node_evo.mn.nodePort}", self.node_evo.mn.nodePort],
                                  DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for coreP2PAddrs[1], must be string")

        # platformP2PAddrs and platformHTTPSAddrs must be within acceptable range (i.e. a valid port number)
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", "0", DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for platformP2PAddrs, must be a valid port [1-65535]")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", "65536", DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for platformP2PAddrs, must be a valid port [1-65535]")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, "0",
                                  -8, "Invalid param for platformHTTPSAddrs, must be a valid port [1-65535]")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, "65536",
                                  -8, "Invalid param for platformHTTPSAddrs, must be a valid port [1-65535]")

        # coreP2PAddrs must be populated when updating a masternode
        self.node_two.update_mn(self, False, "", DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP,
                                -8, "Invalid param for coreP2PAddrs[0], cannot be empty")

        # Legacy:   Normal registration of a masternode
        # Extended: platformP2PAddrs and platformHTTPAddrs will be autopopulated with the addr from the first coreP2PAddrs entry
        assert self.node_evo.node.testmempoolaccept([
            self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}",
                                      DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP)])[0]['allowed']
        assert self.node_two.node.testmempoolaccept([
            self.node_two.update_mn(self, False, f"127.0.0.1:{self.node_two.mn.nodePort}",
                                    DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP)])[0]['allowed']

    def test_validation_legacy(self):
        # Using mainnet P2P port gets refused
        self.node_evo.register_mn(self, False, f"127.0.0.1:{DEFAULT_PORT_MAINNET_CORE_P2P}",
                                  DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, f"Error setting coreP2PAddrs[0] to '127.0.0.1:{DEFAULT_PORT_MAINNET_CORE_P2P}' (invalid port)")

        # Arrays of addresses are recognized by coreP2PAddrs (but get refused for too many entries)
        self.node_evo.register_mn(self, False, [f"127.0.0.1:{self.node_evo.mn.nodePort}", f"127.0.0.2:{self.node_evo.mn.nodePort}"],
                                  DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, f"Error setting coreP2PAddrs[1] to '127.0.0.2:{self.node_evo.mn.nodePort}' (too many entries)")

        # platformP2PAddrs and platformHTTPSAddrs don't accept non-numeric inputs
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", f"127.0.0.1:{DEFAULT_PORT_PLATFORM_P2P}", DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for platformP2PAddrs, ProTx version only supports ports")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", [f"127.0.0.1:{DEFAULT_PORT_PLATFORM_P2P}"], DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for platformP2PAddrs, ProTx version only supports ports")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, f"127.0.0.1:{DEFAULT_PORT_PLATFORM_HTTP}",
                                  -8, "Invalid param for platformHTTPSAddrs, ProTx version only supports ports")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, [f"127.0.0.1:{DEFAULT_PORT_PLATFORM_HTTP}"],
                                  -8, "Invalid param for platformHTTPSAddrs, ProTx version only supports ports")

        # Port numbers may not be wrapped in arrays, either as integers or strings
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", [DEFAULT_PORT_PLATFORM_P2P], DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for platformP2PAddrs, ProTx version only supports ports")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", [f"{DEFAULT_PORT_PLATFORM_P2P}"], DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for platformP2PAddrs, ProTx version only supports ports")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, [DEFAULT_PORT_PLATFORM_HTTP],
                                  -8, "Invalid param for platformHTTPSAddrs, ProTx version only supports ports")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, [f"{DEFAULT_PORT_PLATFORM_HTTP}"],
                                  -8, "Invalid param for platformHTTPSAddrs, ProTx version only supports ports")

        # coreP2PAddrs cannot be empty when registering a masternode without specifying platform fields
        self.node_evo.register_mn(self, False, "", "", "",
                                  -8, "Invalid param for platformP2PAddrs, ProTx version only supports ports")
        self.node_evo.register_mn(self, False, "", "", DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for platformP2PAddrs, ProTx version only supports ports")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", "", "",
                                  -8, "Invalid param for platformP2PAddrs, cannot be empty if other fields populated")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", "", DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for platformP2PAddrs, cannot be empty if other fields populated")
        self.node_evo.register_mn(self, False, "", DEFAULT_PORT_PLATFORM_P2P, "",
                                  -8, "Invalid param for platformHTTPSAddrs, ProTx version only supports ports")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, "",
                                  -8, "Invalid param for platformHTTPSAddrs, cannot be empty if other fields populated")

        # ...but it can empty if platform fields are specified
        assert self.node_evo.node.testmempoolaccept([
            self.node_evo.register_mn(self, False, "", DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP)])[0]['allowed']

        # coreP2PAddrs can omit the port (supplying only addr) and it will be autopopulated with the default P2P port
        assert self.node_evo.node.testmempoolaccept([
            self.node_evo.register_mn(self, False, "127.0.0.1", DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP)])[0]['allowed']

    def test_validation_extended(self):
        # Using mainnet P2P port gets accepted
        assert self.node_evo.node.testmempoolaccept([
            self.node_evo.register_mn(self, False, f"127.0.0.1:{DEFAULT_PORT_MAINNET_CORE_P2P}",
                                      DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP)])[0]['allowed']

        # Arrays of addresses are recognized by address fields and are accepted
        assert self.node_evo.node.testmempoolaccept([
            self.node_evo.register_mn(self, False, [f"127.0.0.1:{self.node_evo.mn.nodePort}", f"127.0.0.2:{self.node_evo.mn.nodePort}"],
                                      DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP)])[0]['allowed']
        assert self.node_two.node.testmempoolaccept([
            self.node_two.update_mn(self, False, [f"127.0.0.1:{self.node_two.mn.nodePort}", f"127.0.0.2:{self.node_two.mn.nodePort}"],
                                    DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP)])[0]['allowed']
        assert self.node_evo.node.testmempoolaccept([
            self.node_evo.register_mn(self, False, [f"127.0.0.1:{self.node_evo.mn.nodePort}", f"127.0.0.2:{self.node_evo.mn.nodePort}"],
                                      [f"127.0.0.1:{DEFAULT_PORT_PLATFORM_P2P}", f"127.0.0.2:{DEFAULT_PORT_PLATFORM_P2P}"],
                                      [f"127.0.0.1:{DEFAULT_PORT_PLATFORM_HTTP}", f"127.0.0.2:{DEFAULT_PORT_PLATFORM_HTTP}"])])[0]['allowed']
        assert self.node_two.node.testmempoolaccept([
            self.node_two.update_mn(self, False, [f"127.0.0.1:{self.node_two.mn.nodePort}", f"127.0.0.2:{self.node_two.mn.nodePort}"],
                                    [f"127.0.0.1:{DEFAULT_PORT_PLATFORM_P2P}", f"127.0.0.2:{DEFAULT_PORT_PLATFORM_P2P}"],
                                    [f"127.0.0.1:{DEFAULT_PORT_PLATFORM_HTTP}", f"127.0.0.2:{DEFAULT_PORT_PLATFORM_HTTP}"])])[0]['allowed']

        # ...but duplicates across and within those fields are not
        self.node_evo.register_mn(self, False, [f"127.0.0.1:{self.node_evo.mn.nodePort}", f"127.0.0.2:{self.node_evo.mn.nodePort}"],
                                  [f"127.0.0.1:{self.node_evo.mn.nodePort}", f"127.0.0.2:{self.node_evo.mn.nodePort}"],
                                  [f"127.0.0.1:{DEFAULT_PORT_PLATFORM_HTTP}", f"127.0.0.2:{DEFAULT_PORT_PLATFORM_HTTP}"],
                                  -8, f"Error setting platformP2PAddrs[0] to '127.0.0.1:{self.node_evo.mn.nodePort}' (duplicate)")
        self.node_evo.register_mn(self, False, [f"127.0.0.1:{self.node_evo.mn.nodePort}", f"127.0.0.2:{self.node_evo.mn.nodePort}"],
                                  [f"127.0.0.1:{DEFAULT_PORT_PLATFORM_P2P}", f"127.0.0.1:{DEFAULT_PORT_PLATFORM_P2P}"],
                                  [f"127.0.0.1:{DEFAULT_PORT_PLATFORM_HTTP}", f"127.0.0.2:{DEFAULT_PORT_PLATFORM_HTTP}"],
                                  -8, f"Error setting platformP2PAddrs[1] to '127.0.0.1:{DEFAULT_PORT_PLATFORM_P2P}' (duplicate)")

        # platformP2PAddrs and platformHTTPSAddrs accept non-numeric inputs
        assert self.node_evo.node.testmempoolaccept([
            self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", f"127.0.0.1:{DEFAULT_PORT_PLATFORM_P2P}",
                                      DEFAULT_PORT_PLATFORM_HTTP)])[0]['allowed']
        assert self.node_evo.node.testmempoolaccept([
            self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", [f"127.0.0.1:{DEFAULT_PORT_PLATFORM_P2P}"],
                                      DEFAULT_PORT_PLATFORM_HTTP)])[0]['allowed']
        assert self.node_evo.node.testmempoolaccept([
            self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P,
                                      f"127.0.0.1:{DEFAULT_PORT_PLATFORM_HTTP}")])[0]['allowed']
        assert self.node_evo.node.testmempoolaccept([
            self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P,
                                      [f"127.0.0.1:{DEFAULT_PORT_PLATFORM_HTTP}"])])[0]['allowed']

        # Port numbers may not be wrapped in arrays, either as integers or strings
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", [DEFAULT_PORT_PLATFORM_P2P], DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for platformP2PAddrs[0], must be string")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", [f"{DEFAULT_PORT_PLATFORM_P2P}"], DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for platformP2PAddrs[0], must be string")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, [DEFAULT_PORT_PLATFORM_HTTP],
                                  -8, "Invalid param for platformHTTPSAddrs[0], must be string")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, [f"{DEFAULT_PORT_PLATFORM_HTTP}"],
                                  -8, "Invalid param for platformHTTPSAddrs[0], must be string")

        # coreP2PAddrs can be empty when registering a masternode if platform fields are not specified
        assert self.node_evo.node.testmempoolaccept([
            self.node_evo.register_mn(self, False, "", "", "")])[0]['allowed']

        # ...but it cannot empty if any address field is specified
        self.node_evo.register_mn(self, False, "", DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Cannot set param for platformP2PAddrs, must specify coreP2PAddrs first")
        self.node_evo.register_mn(self, False, "", "", DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Cannot set param for platformHTTPSAddrs, must specify coreP2PAddrs first")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", "", "",
                                  -8, "Invalid param for platformP2PAddrs, cannot be empty if other fields populated")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", "", DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Invalid param for platformP2PAddrs, cannot be empty if other fields populated")
        self.node_evo.register_mn(self, False, "", DEFAULT_PORT_PLATFORM_P2P, "",
                                  -8, "Cannot set param for platformP2PAddrs, must specify coreP2PAddrs first")
        self.node_evo.register_mn(self, False, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, "",
                                  -8, "Invalid param for platformHTTPSAddrs, cannot be empty if other fields populated")

        # coreP2PAddrs cannot omit the port, extended addresses require explicit port specification
        self.node_evo.register_mn(self, False, "127.0.0.1", DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP,
                                  -8, "Error setting coreP2PAddrs[0] to '127.0.0.1' (invalid port)")

    def test_deprecation(self):
        # netInfo is represented with JSON in CProRegTx, CProUpServTx, CDeterministicMNState and CSimplifiedMNListEntry,
        # so we need to test calls that rely on these underlying implementations. Start by collecting RPC responses.
        self.log.info("Collect JSON RPC responses from node")

        # CProRegTx::ToJson() <- TxToUniv() <- TxToJSON() <- getrawtransaction
        proregtx_hash = self.node_evo.register_mn(self, True, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP)
        proregtx_rpc = self.node_evo.node.getrawtransaction(proregtx_hash, True)

        # CDeterministicMNState::ToJson() <- CDeterministicMN::pdmnState <- masternode_status
        self.node_evo.set_active_state(self, True)
        masternode_status = self.node_evo.node.masternode('status')

        # Generate deprecation-disabled response to avoid having to re-create a masternode again later on
        self.node_evo.set_active_state(self, True, ['-deprecatedrpc=service'])
        self.reconnect_nodes()
        masternode_status_depr = self.node_evo.node.masternode('status')

        # Stop actively running the masternode so we can issue a CProUpServTx (and enable the deprecation)
        self.node_evo.set_active_state(self, False)
        self.reconnect_nodes()

        # CProUpServTx::ToJson() <- TxToUniv() <- TxToJSON() <- getrawtransaction
        # We need to update *thrice*, the first time to incorrect values and the second time, (back) to correct values and the third time, only
        # updating Platform fields to trigger the conditions needed to report the dummy address
        proupservtx_hash = self.node_evo.update_mn(self, True, f"127.0.0.1:{self.node_evo.mn.nodePort + 10}", DEFAULT_PORT_PLATFORM_P2P + 10, DEFAULT_PORT_PLATFORM_HTTP + 10)
        proupservtx_rpc = self.node_evo.node.getrawtransaction(proupservtx_hash, True)

        # Restore back to defaults
        proupservtx_hash = self.node_evo.update_mn(self, True, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP)
        proupservtx_rpc = self.node_evo.node.getrawtransaction(proupservtx_hash, True)

        # Revert back to incorrect values but only for Platform fields
        proupservtx_hash_pl = self.node_evo.update_mn(self, True, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P + 10, DEFAULT_PORT_PLATFORM_HTTP + 10)
        proupservtx_rpc_pl = self.node_evo.node.getrawtransaction(proupservtx_hash_pl, True)

        # CSimplifiedMNListEntry::ToJson() <- CSimplifiedMNListDiff::mnList <- CSimplifiedMNListDiff::ToJson() <- protx_diff
        masternode_active_height: int = masternode_status['dmnState']['registeredHeight']
        protx_diff_rpc = self.node_evo.node.protx('diff', masternode_active_height - 1, masternode_active_height)

        # CDeterministicMNStateDiff::ToJson() <- CDeterministicMNListDiff::updatedMns <- protx_listdiff
        proupservtx_height = proupservtx_rpc['height']
        protx_listdiff_rpc = self.node_evo.node.protx('listdiff', proupservtx_height - 1, proupservtx_height)
        protx_listdiff_rpc = self.extract_from_listdiff(protx_listdiff_rpc, proregtx_hash)

        # If the core P2P address wasn't updated and the platform fields were, CDeterministicMNStateDiff will return a dummy address
        proupservtx_height_pl = proupservtx_rpc_pl['height']
        protx_listdiff_rpc_pl = self.node_evo.node.protx('listdiff', proupservtx_height_pl - 1, proupservtx_height_pl)
        protx_listdiff_rpc_pl = self.extract_from_listdiff(protx_listdiff_rpc_pl, proregtx_hash)

        self.log.info("Test RPCs return an 'addresses' field")
        assert "addresses" in proregtx_rpc['proRegTx'].keys()
        assert "addresses" in masternode_status['dmnState'].keys()
        assert "addresses" in proupservtx_rpc['proUpServTx'].keys()
        assert "addresses" in protx_diff_rpc['mnList'][0].keys()
        assert "addresses" in protx_listdiff_rpc.keys()

        self.log.info("Test 'addresses' report correctly")
        self.check_netinfo_fields(proregtx_rpc['proRegTx']['addresses'], self.node_evo.mn.nodePort, DEFAULT_PORT_PLATFORM_HTTP, DEFAULT_PORT_PLATFORM_P2P)
        self.check_netinfo_fields(masternode_status['dmnState']['addresses'], self.node_evo.mn.nodePort, DEFAULT_PORT_PLATFORM_HTTP, DEFAULT_PORT_PLATFORM_P2P)
        self.check_netinfo_fields(proupservtx_rpc['proUpServTx']['addresses'], self.node_evo.mn.nodePort, DEFAULT_PORT_PLATFORM_HTTP, DEFAULT_PORT_PLATFORM_P2P)
        # CSimplifiedMNListEntry doesn't store platform P2P network information before extended addresses
        self.check_netinfo_fields(protx_diff_rpc['mnList'][0]['addresses'], self.node_evo.mn.nodePort, DEFAULT_PORT_PLATFORM_HTTP, None)
        # CDeterministicMNStateDiff will fill in the correct address if the core P2P address was updated *alongside* platform fields
        self.check_netinfo_fields(protx_listdiff_rpc['addresses'], self.node_evo.mn.nodePort, DEFAULT_PORT_PLATFORM_HTTP, DEFAULT_PORT_PLATFORM_P2P)
        # CDeterministicMNStateDiff will use a dummy address if the core P2P address wasn't updated but Platform fields were to ensure changes are reported
        assert "core_p2p" not in protx_listdiff_rpc_pl['addresses'].keys()
        assert_equal(protx_listdiff_rpc_pl['addresses']['platform_https'][0], f"{DMNSTATE_DIFF_DUMMY_ADDR}:{DEFAULT_PORT_PLATFORM_HTTP + 10}")
        assert_equal(protx_listdiff_rpc_pl['addresses']['platform_p2p'][0], f"{DMNSTATE_DIFF_DUMMY_ADDR}:{DEFAULT_PORT_PLATFORM_P2P + 10}")

        self.log.info("Test RPCs by default no longer return a 'service' field")
        assert "service" not in proregtx_rpc['proRegTx'].keys()
        assert "service" not in masternode_status['dmnState'].keys()
        assert "service" not in proupservtx_rpc['proUpServTx'].keys()
        assert "service" not in protx_diff_rpc['mnList'][0].keys()
        assert "service" not in protx_listdiff_rpc.keys()
        # "service" in "masternode status" is exempt from the deprecation as the primary address is
        # relevant on the host node as opposed to expressing payload information in most other RPCs.
        assert "service" in masternode_status.keys()

        # Shut down masternode
        self.node_evo.destroy_mn(self)
        self.reconnect_nodes()

        self.log.info("Collect RPC responses from node with -deprecatedrpc=service")

        # Re-use chain activity from earlier
        proregtx_rpc = self.node_simple.getrawtransaction(proregtx_hash, True)
        proupservtx_rpc = self.node_simple.getrawtransaction(proupservtx_hash, True)
        protx_diff_rpc = self.node_simple.protx('diff', masternode_active_height - 1, masternode_active_height)
        masternode_status = masternode_status_depr # Pull in response generated from earlier
        protx_listdiff_rpc = self.node_simple.protx('listdiff', proupservtx_height - 1, proupservtx_height)
        protx_listdiff_rpc = self.extract_from_listdiff(protx_listdiff_rpc, proregtx_hash)

        self.log.info("Test RPCs return 'service' with -deprecatedrpc=service")
        assert "service" in proregtx_rpc['proRegTx'].keys()
        assert "service" in masternode_status['dmnState'].keys()
        assert "service" in proupservtx_rpc['proUpServTx'].keys()
        assert "service" in protx_diff_rpc['mnList'][0].keys()
        assert "service" in protx_listdiff_rpc.keys()

    def test_empty_fields(self):
        def empty_common(grt_dict):
            # Even if '[::]:0' is reported by 'service', 'addresses' should always be blank if we have nothing to report
            assert_equal(len(grt_dict['addresses']), 0)
            # 'service' will never be empty. Legacy addresses stored an empty CService, which evaluates to
            # '[::]:0' when printed in string form. This behavior has been preserved for continuity.
            assert_equal(grt_dict['service'], "[::]:0")

        # Validate that our masternode was registered before the fork
        grt_proregtx = self.node_simple.getrawtransaction(self.node_two.mn.proTxHash, True)['proRegTx']
        assert_equal(grt_proregtx['version'], PROTXVER_BASIC)

        # Test reporting on legacy address (i.e. basic BLS and earlier) nodes
        empty_common(grt_proregtx)

        # platform{P2P,HTTP}Port are non-optional fields for legacy addresses and will be populated
        assert_equal(grt_proregtx['platformP2PPort'], DEFAULT_PORT_PLATFORM_P2P)
        assert_equal(grt_proregtx['platformHTTPPort'], DEFAULT_PORT_PLATFORM_HTTP)

        # Destroy and re-register node as blank
        self.node_two.destroy_mn(self)
        self.reconnect_nodes()
        self.node_two.register_mn(self, True, "", "", "")

        # Validate that masternode uses extended addresses
        grt_proregtx = self.node_simple.getrawtransaction(self.node_two.mn.proTxHash, True)['proRegTx']
        assert_equal(grt_proregtx['version'], PROTXVER_EXTADDR)

        # Test reporting for extended addresses
        empty_common(grt_proregtx)

        # Extended addresses store addr:port pairs for platform{P2P,HTTPS}Addrs but ProRegTx allows blank
        # address declarations (for later update with a ProUpServTx), this means that *unlike* legacy
        # addresses, ProRegTxes that opt to omit addresses cannot report platform{P2P,HTTP}Port, which
        # is signified with -1.
        assert_equal(grt_proregtx['platformP2PPort'], -1)
        assert_equal(grt_proregtx['platformHTTPPort'], -1)

        # Destroy masternode
        self.node_evo.destroy_mn(self)
        self.reconnect_nodes()

    def test_shims(self):
        # There are two shims there to help with migrating between legacy and extended addresses, one reads from legacy platform
        # fields to ensure 'addresses' is adequately populated. The other, reads from netInfo to populate platform{HTTP,P2P}Port.
        # As the fork is now active, we can now evaluate the test cases that couldn't be evaluated in test_deprecation().
        self.log.info("Collect JSON RPC responses from node")

        # Create an masternode that clearly uses extended addresses (hello IPv6!)
        proregtx_hash = self.node_evo.register_mn(self, True,
                                                  [f"127.0.0.1:{self.node_evo.mn.nodePort}", f"[::1]:{self.node_evo.mn.nodePort}"],
                                                  [f"127.0.0.2:{DEFAULT_PORT_PLATFORM_P2P}", f"[::2]:{DEFAULT_PORT_PLATFORM_P2P}"],
                                                  [f"127.0.0.3:{DEFAULT_PORT_PLATFORM_HTTP}", f"[::3]:{DEFAULT_PORT_PLATFORM_HTTP}"])
        proregtx_rpc = self.node_evo.node.getrawtransaction(proregtx_hash, True)

        # Update only a platform field (platformP2PAddrs) but with an odd twist, we only specify the port number and expect the shim
        # to auto-fill the addr portion from coreP2PAddrs[0]
        proupservtx_hash = self.node_evo.update_mn(self, True,
                                                   [f"127.0.0.1:{self.node_evo.mn.nodePort}", f"[::1]:{self.node_evo.mn.nodePort}"],
                                                   DEFAULT_PORT_PLATFORM_P2P + 12,
                                                   [f"127.0.0.3:{DEFAULT_PORT_PLATFORM_HTTP}", f"[::3]:{DEFAULT_PORT_PLATFORM_HTTP}"])
        proupservtx_rpc = self.node_evo.node.getrawtransaction(proupservtx_hash, True)
        assert_equal(proupservtx_rpc['proUpServTx']['addresses']['platform_p2p'][0], f"127.0.0.1:{DEFAULT_PORT_PLATFORM_P2P + 12}")

        # Since all fields are stored in the netInfo, even though we updated only platformP2PAddrs, the dummy address should never be visible
        # and *all* the addresses fields will be visible as they're now in one data structure
        proupservtx_height = proupservtx_rpc['height']
        protx_listdiff_rpc = self.node_evo.node.protx('listdiff', proupservtx_height - 1, proupservtx_height)
        protx_listdiff_rpc = self.extract_from_listdiff(protx_listdiff_rpc, proregtx_hash)
        assert_equal(protx_listdiff_rpc['addresses'], {
            'core_p2p': [f"127.0.0.1:{self.node_evo.mn.nodePort}", f"[::1]:{self.node_evo.mn.nodePort}"],
            'platform_p2p': [f"127.0.0.1:{DEFAULT_PORT_PLATFORM_P2P + 12}"],
            'platform_https': [f"127.0.0.3:{DEFAULT_PORT_PLATFORM_HTTP}", f"[::3]:{DEFAULT_PORT_PLATFORM_HTTP}"]
        })

        # Check platform{HTTP,P2P}Port shims work as expected
        assert_equal(proregtx_rpc['proRegTx']['platformP2PPort'], DEFAULT_PORT_PLATFORM_P2P)
        assert_equal(proregtx_rpc['proRegTx']['platformHTTPPort'], DEFAULT_PORT_PLATFORM_HTTP)
        assert_equal(proupservtx_rpc['proUpServTx']['platformP2PPort'], DEFAULT_PORT_PLATFORM_P2P + 12)
        assert_equal(proupservtx_rpc['proUpServTx']['platformHTTPPort'], DEFAULT_PORT_PLATFORM_HTTP)
        # platform{HTTP,P2P}Port *won't* be populated by listdiff as that *field* is now unused and it's dangerous to report "updates" to disused fields
        assert "platformP2PPort" not in protx_listdiff_rpc.keys()
        assert "platformHTTPPort" not in protx_listdiff_rpc.keys()

        # Restart the client to see if (de)ser works as intended (CDeterministicMNStateDiff is a special case and we just made an update)
        self.node_evo.set_active_state(self, False)
        self.reconnect_nodes()

        # Check that 'service' correctly reports as coreP2PAddrs[0]
        proregtx_rpc = self.node_simple.getrawtransaction(proregtx_hash, True)
        proupservtx_rpc = self.node_simple.getrawtransaction(proupservtx_hash, True)
        assert_equal(proregtx_rpc['proRegTx']['service'], f"127.0.0.1:{self.node_evo.mn.nodePort}")
        assert_equal(proupservtx_rpc['proUpServTx']['service'], f"127.0.0.1:{self.node_evo.mn.nodePort}")

if __name__ == "__main__":
    NetInfoTest().main()
