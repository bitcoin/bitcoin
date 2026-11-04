#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test stale-tip P2P relay."""

from test_framework.blocktools import create_block
from test_framework.messages import (
    CBlockHeader,
    msg_feature,
    msg_headers,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

STALETIP_FEATURE = "BIP332"
FEATURE_VERSION = 70017


class StaleTipPeer(P2PInterface):
    def __init__(self, *, send_feature=True, feature_data=b"\x00"):
        super().__init__()
        self.send_feature = send_feature
        self.feature_data = feature_data
        self.features = []

    def on_version(self, message):
        if self.send_feature and message.nVersion >= FEATURE_VERSION:
            self.send_without_ping(msg_feature(STALETIP_FEATURE, self.feature_data))
        super().on_version(message)

    def on_feature(self, message):
        self.features.append(message)


class P2PStaleTipTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-staletips=headers"]]

    def run_test(self):
        self.test_staletip_option_validation()

        self.generate(self.nodes[0], 101)
        self.block_time_offset = 1

        self.test_feature_negotiation()
        self.test_reorged_out_active_tip_tracked()
        self.test_tracked_tip_listed_with_known_descendants()
        self.test_startup_seeding_only_when_enabled()

    def tracked_staletips(self):
        return [tip["hash"] for tip in self.nodes[0].getchaintips(staletips_only=True)]

    def connect_peer(self, *, send_feature=True, feature_data=b"\x00", **kwargs):
        return self.nodes[0].add_p2p_connection(StaleTipPeer(send_feature=send_feature, feature_data=feature_data), **kwargs)

    def assert_staletip_hash_tracked(self, block_hash):
        self.wait_until(lambda: block_hash in self.tracked_staletips())

    def test_staletip_option_validation(self):
        self.log.info("Test staletip option validation")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=["-staletips=bogus"],
            expected_msg="Error: Invalid value for -staletips=<mode>: 'bogus'. Expected one of: none, headers.",
        )
        for valid_arg in ["-staletips=none", "-nostaletips", "-staletips=headers"]:
            self.start_node(0, extra_args=[valid_arg])
            self.stop_node(0)
        self.start_node(0)

    def test_reorged_out_active_tip_tracked(self):
        self.log.info("Test reorged-out active tip is tracked as stale")
        node = self.nodes[0]
        active_tip_hash = node.getbestblockhash()
        active_tip = node.getblock(active_tip_hash)

        fork_block = create_block(
            hashprev=int(active_tip["previousblockhash"], 16),
            height=active_tip["height"],
            ntime=active_tip["time"] + self.block_time_offset,
        )
        self.block_time_offset += 1
        fork_block.solve()
        assert_equal(node.submitblock(fork_block.serialize().hex()), "inconclusive")

        reorg_block = create_block(
            hashprev=fork_block.hash_int,
            height=active_tip["height"] + 1,
            ntime=active_tip["time"] + self.block_time_offset,
        )
        self.block_time_offset += 1
        reorg_block.solve()
        assert_equal(node.submitblock(reorg_block.serialize().hex()), None)
        assert_equal(node.getbestblockhash(), reorg_block.hash_hex)
        self.assert_staletip_hash_tracked(active_tip_hash)

        stale_tip = next(tip for tip in node.getchaintips() if tip["hash"] == active_tip_hash)
        assert_equal(stale_tip["fork_point"], active_tip["previousblockhash"])
        assert_equal(stale_tip["branchlen"], 1)

        # max_depth applies to tracked tips too.
        assert_equal([tip["hash"] for tip in node.getchaintips(staletips_only=True, max_depth=1)], [active_tip_hash])
        assert_equal(node.getchaintips(staletips_only=True, max_depth=0), [])

    def test_tracked_tip_listed_with_known_descendants(self):
        self.log.info("Test a tracked stale tip is listed after a known block builds on it")
        node = self.nodes[0]
        active_tip = node.getblock(node.getbestblockhash())
        stale = create_block(
            hashprev=int(active_tip["previousblockhash"], 16),
            height=active_tip["height"],
            ntime=active_tip["time"] + self.block_time_offset,
        )
        self.block_time_offset += 1
        stale.solve()
        assert_equal(node.submitblock(stale.serialize().hex()), "inconclusive")
        self.assert_staletip_hash_tracked(stale.hash_hex)

        # Headers that extend the stale tip beyond the active tip's work are
        # not tracked when they arrive in one batch, but they mean the stale
        # tip is no longer a chain tip.
        descendants = []
        prev_hash = stale.hash_int
        for height in [active_tip["height"] + 1, active_tip["height"] + 2]:
            block = create_block(hashprev=prev_hash, height=height, ntime=active_tip["time"] + self.block_time_offset)
            self.block_time_offset += 1
            block.solve()
            descendants.append(block)
            prev_hash = block.hash_int
        peer = node.add_p2p_connection(P2PInterface())
        peer.send_and_ping(msg_headers([CBlockHeader(block) for block in descendants]))
        node.disconnect_p2ps()
        # Mark the descendants invalid, so their higher-work headers are not
        # left as a better chain for the rest of the test. They are still
        # known blocks building on the stale tip.
        node.invalidateblock(descendants[0].hash_hex)

        assert stale.hash_hex not in [tip["hash"] for tip in node.getchaintips()]
        assert stale.hash_hex in self.tracked_staletips()

    def test_feature_negotiation(self):
        self.log.info("Test staletip feature advertisement modes")
        node = self.nodes[0]
        for mode, expected_data in [("headers", b"\x00"), ("none", None)]:
            self.restart_node(0, extra_args=[f"-staletips={mode}"])
            peer = self.connect_peer()
            stale_features = [feature for feature in peer.features if feature.feature_id == STALETIP_FEATURE]
            if expected_data is None:
                assert_equal(stale_features, [])
            else:
                assert_equal(len(stale_features), 1)
                assert_equal(stale_features[0].feature_data, expected_data)
            node.disconnect_p2ps()

        self.log.info("Test peer staletip feature negotiation")
        self.restart_node(0, extra_args=["-staletips=headers"])
        for feature_data, expected_log in [
            (b"", "ignoring malformed BIP332 feature data"),
            (b"\x02", "ignoring malformed BIP332 feature data"),
            (b"\x00\x01", "ignoring malformed BIP332 feature data"),
        ]:
            with node.assert_debug_log([expected_log], timeout=2):
                peer = self.connect_peer(feature_data=feature_data)
                peer.sync_with_ping()
            assert peer.is_connected
            node.disconnect_p2ps()

    def test_startup_seeding_only_when_enabled(self):
        self.log.info("Test stale-tip tracker is seeded at startup only when enabled")
        node = self.nodes[0]
        tracked = sorted(self.tracked_staletips())
        assert len(tracked) > 0

        # With stale-tip relay disabled, tracking is not reported.
        for disabled_arg in ["-staletips=none", "-nostaletips"]:
            self.restart_node(0, extra_args=[disabled_arg])
            assert_raises_rpc_error(-1, "Stale-tip tracking is disabled", node.getchaintips, staletips_only=True)

        # With stale-tip relay enabled, the same stale tips are seeded from
        # the block index at startup.
        self.restart_node(0)
        assert_equal(sorted(self.tracked_staletips()), tracked)


if __name__ == "__main__":
    P2PStaleTipTest(__file__).main()
