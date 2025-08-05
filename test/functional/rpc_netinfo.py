#!/usr/bin/env python3
# Copyright (c) 2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test network information fields across RPCs."""

from test_framework.util import (
    assert_equal
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

# See CMainParams in src/chainparams.cpp
DEFAULT_PORT_MAINNET_CORE_P2P = 9999
# See CRegTestParams in src/chainparams.cpp
DEFAULT_PORT_PLATFORM_P2P = 22200
DEFAULT_PORT_PLATFORM_HTTP = 22201
# See CDeterministicMNStateDiff::ToJson() in src/evo/dmnstate.h
DMNSTATE_DIFF_DUMMY_ADDR = "255.255.255.255"

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

    def update_mn(self, test: BitcoinTestFramework, addrs_core_p2p, addrs_platform_p2p, addrs_platform_https) -> str:
        assert self.mn.nodeIdx is not None
        protx_output = self.mn.update_service(self.node, submit=True, addrs_core_p2p=addrs_core_p2p, platform_node_id=self.platform_nodeid,
                                              addrs_platform_p2p=addrs_platform_p2p, addrs_platform_https=addrs_platform_https)
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
        self.num_nodes = 2
        self.extra_args = [
            ["-dip3params=2:2"],
            ["-deprecatedrpc=service", "-dip3params=2:2"]
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def check_netinfo_fields(self, val, core_p2p_port: int, plat_https_port: Optional[int], plat_p2p_port: Optional[int]):
        assert_equal(val['core_p2p'][0], f"127.0.0.1:{core_p2p_port}")
        if plat_https_port:
            assert_equal(val['platform_https'][0], f"127.0.0.1:{plat_https_port}")
        if plat_p2p_port:
            assert_equal(val['platform_p2p'][0], f"127.0.0.1:{plat_p2p_port}")

    def reconnect_nodes(self):
        # Needed as restarts don't reconnect nodes
        for idx in range(1, self.num_nodes):
            self.connect_nodes(0, idx)

    def run_test(self):
        self.node_evo: EvoNode = EvoNode(self.nodes[0])
        self.node_evo.generate_collateral(self)

        self.node_simple: TestNode = self.nodes[1]

        self.log.info("Test input validation for masternode address fields")
        self.test_validation_common()
        self.test_validation_legacy()

        self.log.info("Test output masternode address fields for consistency")
        self.test_deprecation()

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
        proupservtx_hash = self.node_evo.update_mn(self, f"127.0.0.1:{self.node_evo.mn.nodePort + 10}", DEFAULT_PORT_PLATFORM_P2P + 10, DEFAULT_PORT_PLATFORM_HTTP + 10)
        proupservtx_rpc = self.node_evo.node.getrawtransaction(proupservtx_hash, True)

        # Restore back to defaults
        proupservtx_hash = self.node_evo.update_mn(self, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P, DEFAULT_PORT_PLATFORM_HTTP)
        proupservtx_rpc = self.node_evo.node.getrawtransaction(proupservtx_hash, True)

        # Revert back to incorrect values but only for Platform fields
        proupservtx_hash_pl = self.node_evo.update_mn(self, f"127.0.0.1:{self.node_evo.mn.nodePort}", DEFAULT_PORT_PLATFORM_P2P + 10, DEFAULT_PORT_PLATFORM_HTTP + 10)
        proupservtx_rpc_pl = self.node_evo.node.getrawtransaction(proupservtx_hash_pl, True)

        # CSimplifiedMNListEntry::ToJson() <- CSimplifiedMNListDiff::mnList <- CSimplifiedMNListDiff::ToJson() <- protx_diff
        masternode_active_height: int = masternode_status['dmnState']['registeredHeight']
        protx_diff_rpc = self.node_evo.node.protx('diff', masternode_active_height - 1, masternode_active_height)

        # CDeterministicMNStateDiff::ToJson() <- CDeterministicMNListDiff::updatedMns <- protx_listdiff
        proupservtx_height = proupservtx_rpc['height']
        protx_listdiff_rpc = self.node_evo.node.protx('listdiff', proupservtx_height - 1, proupservtx_height)

        # If the core P2P address wasn't updated and the platform fields were, CDeterministicMNStateDiff will return a dummy address
        proupservtx_height_pl = proupservtx_rpc_pl['height']
        protx_listdiff_rpc_pl = self.node_evo.node.protx('listdiff', proupservtx_height_pl - 1, proupservtx_height_pl)

        self.log.info("Test RPCs return an 'addresses' field")
        assert "addresses" in proregtx_rpc['proRegTx'].keys()
        assert "addresses" in masternode_status['dmnState'].keys()
        assert "addresses" in proupservtx_rpc['proUpServTx'].keys()
        assert "addresses" in protx_diff_rpc['mnList'][0].keys()
        assert "addresses" in protx_listdiff_rpc['updatedMNs'][0][proregtx_hash].keys()

        self.log.info("Test 'addresses' report correctly")
        self.check_netinfo_fields(proregtx_rpc['proRegTx']['addresses'], self.node_evo.mn.nodePort, DEFAULT_PORT_PLATFORM_HTTP, DEFAULT_PORT_PLATFORM_P2P)
        self.check_netinfo_fields(masternode_status['dmnState']['addresses'], self.node_evo.mn.nodePort, DEFAULT_PORT_PLATFORM_HTTP, DEFAULT_PORT_PLATFORM_P2P)
        self.check_netinfo_fields(proupservtx_rpc['proUpServTx']['addresses'], self.node_evo.mn.nodePort, DEFAULT_PORT_PLATFORM_HTTP, DEFAULT_PORT_PLATFORM_P2P)
        # CSimplifiedMNListEntry doesn't store platform P2P network information before extended addresses
        self.check_netinfo_fields(protx_diff_rpc['mnList'][0]['addresses'], self.node_evo.mn.nodePort, DEFAULT_PORT_PLATFORM_HTTP, None)
        # CDeterministicMNStateDiff will fill in the correct address if the core P2P address was updated *alongside* platform fields
        self.check_netinfo_fields(protx_listdiff_rpc['updatedMNs'][0][proregtx_hash]['addresses'], self.node_evo.mn.nodePort, DEFAULT_PORT_PLATFORM_HTTP, DEFAULT_PORT_PLATFORM_P2P)
        # CDeterministicMNStateDiff will use a dummy address if the core P2P address wasn't updated but Platform fields were to ensure changes are reported
        assert "core_p2p" not in protx_listdiff_rpc_pl['updatedMNs'][0][proregtx_hash]['addresses'].keys()
        assert_equal(protx_listdiff_rpc_pl['updatedMNs'][0][proregtx_hash]['addresses']['platform_https'][0], f"{DMNSTATE_DIFF_DUMMY_ADDR}:{DEFAULT_PORT_PLATFORM_HTTP + 10}")
        assert_equal(protx_listdiff_rpc_pl['updatedMNs'][0][proregtx_hash]['addresses']['platform_p2p'][0], f"{DMNSTATE_DIFF_DUMMY_ADDR}:{DEFAULT_PORT_PLATFORM_P2P + 10}")

        self.log.info("Test RPCs by default no longer return a 'service' field")
        assert "service" not in proregtx_rpc['proRegTx'].keys()
        assert "service" not in masternode_status['dmnState'].keys()
        assert "service" not in proupservtx_rpc['proUpServTx'].keys()
        assert "service" not in protx_diff_rpc['mnList'][0].keys()
        assert "service" not in protx_listdiff_rpc['updatedMNs'][0][proregtx_hash].keys()
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

        self.log.info("Test RPCs return 'service' with -deprecatedrpc=service")
        assert "service" in proregtx_rpc['proRegTx'].keys()
        assert "service" in masternode_status['dmnState'].keys()
        assert "service" in proupservtx_rpc['proUpServTx'].keys()
        assert "service" in protx_diff_rpc['mnList'][0].keys()
        assert "service" in protx_listdiff_rpc['updatedMNs'][0][proregtx_hash].keys()

if __name__ == "__main__":
    NetInfoTest().main()
