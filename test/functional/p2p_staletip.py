#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test stale-tip P2P relay."""

from test_framework.blocktools import create_block
from test_framework.messages import (
    CBlockHeader,
    MSG_BLOCK,
    MSG_TYPE_MASK,
    StaleTipCompressedHeader,
    msg_feature,
    msg_headers,
    msg_staletip,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

STALETIP_FEATURE = "BIP332"
FEATURE_VERSION = 70017
STALETIP_RECENT_WINDOW = 1000  # Mirrors STALETIP_RECENT_WINDOW in src/node/staletiptracker.h


class msg_raw_staletip:
    """A staletip message with an arbitrary payload, for malformed inputs."""
    msgtype = b"staletip"

    def __init__(self, payload):
        self.payload = payload

    def serialize(self):
        return self.payload

    def __repr__(self):
        return f"msg_raw_staletip({self.payload.hex()})"


class StaleTipPeer(P2PInterface):
    def __init__(self, *, send_feature=True, feature_data=b"\x00"):
        super().__init__()
        self.send_feature = send_feature
        self.feature_data = feature_data
        self.features = []
        self.getdata = []

    def on_version(self, message):
        if self.send_feature and message.nVersion >= FEATURE_VERSION:
            self.send_without_ping(msg_feature(STALETIP_FEATURE, self.feature_data))
        super().on_version(message)

    def on_feature(self, message):
        self.features.append(message)

    def on_getdata(self, message):
        self.getdata.extend(message.inv)

    def wait_for_getdata_hash(self, block_hash):
        self.wait_until(lambda: any(
            inv.hash == block_hash and inv.type & MSG_TYPE_MASK == MSG_BLOCK
            for inv in self.getdata
        ))


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
        self.test_inbound_staletip_handling()
        self.test_higher_work_staletip_requests_block()
        self.test_startup_seeding_only_when_enabled()

    def tracked_staletips(self):
        return [tip["hash"] for tip in self.nodes[0].getchaintips(staletips_only=True)]

    def connect_peer(self, *, send_feature=True, feature_data=b"\x00", **kwargs):
        return self.nodes[0].add_p2p_connection(StaleTipPeer(send_feature=send_feature, feature_data=feature_data), **kwargs)

    def assert_staletip_hash_tracked(self, block_hash):
        self.wait_until(lambda: block_hash in self.tracked_staletips())

    def stale_block(self, *, fork_depth=1):
        blocks, fork_point_hash = self.stale_branch(length=1, fork_depth=fork_depth)
        return blocks[0], fork_point_hash

    def stale_branch(self, *, length, fork_depth):
        node = self.nodes[0]
        active_tip = node.getblock(node.getbestblockhash())
        fork_point = active_tip
        for _ in range(fork_depth):
            fork_point = node.getblock(fork_point["previousblockhash"])
        fork_point_hash = int(fork_point["hash"], 16)
        prev_hash = fork_point_hash
        height = fork_point["height"]
        blocks = []
        for _ in range(length):
            block = create_block(
                hashprev=prev_hash,
                height=height + 1,
                ntime=active_tip["time"] + self.block_time_offset,
            )
            self.block_time_offset += 1
            block.solve()
            blocks.append(block)
            prev_hash = block.hash_int
            height += 1
        return blocks, fork_point_hash

    def active_block(self):
        node = self.nodes[0]
        active_tip_hash = node.getbestblockhash()
        active_tip = node.getblock(active_tip_hash)
        block = create_block(
            hashprev=int(active_tip_hash, 16),
            height=active_tip["height"] + 1,
            ntime=active_tip["time"] + self.block_time_offset,
        )
        self.block_time_offset += 1
        block.solve()
        return block, int(active_tip_hash, 16)

    def staletip_msg(self, blocks, fork_point_hash, *, have_block=False):
        if not isinstance(blocks, list):
            blocks = [blocks]
        return msg_staletip(
            hash_fork_point=fork_point_hash,
            headers=[StaleTipCompressedHeader(CBlockHeader(block)) for block in blocks],
            have_block=have_block,
        )

    def block_getdata_hashes(self, peer):
        return [inv.hash for inv in peer.getdata if inv.type & MSG_TYPE_MASK == MSG_BLOCK]

    def assert_staletip_tracked(self, block):
        self.assert_staletip_hash_tracked(block.hash_hex)

    def assert_staletip_not_tracked(self, block):
        assert block.hash_hex not in self.tracked_staletips()

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
        block, fork_point_hash = self.stale_block()
        for mode, expected_data in [("headers", b"\x00"), ("none", None)]:
            self.restart_node(0, extra_args=[f"-staletips={mode}"])
            peer = self.connect_peer()
            stale_features = [feature for feature in peer.features if feature.feature_id == STALETIP_FEATURE]
            if expected_data is None:
                assert_equal(stale_features, [])
                assert_equal(node.getnetworkinfo()["localfeatures"], {})
                # The peer advertised the feature, but relay is off here.
                with node.assert_debug_log(["ignoring unnegotiated staletip"], timeout=2):
                    peer.send_and_ping(self.staletip_msg(block, fork_point_hash))
            else:
                assert_equal(len(stale_features), 1)
                assert_equal(stale_features[0].feature_data, expected_data)
                assert_equal(node.getnetworkinfo()["localfeatures"],
                             {STALETIP_FEATURE: {"prefers_blocks": expected_data == b"\x01"}})
            node.disconnect_p2ps()

        self.log.info("Test peer staletip feature negotiation")
        self.restart_node(0, extra_args=["-staletips=headers"])
        for send_feature, feature_data, expected_log, expected_features in [
            (True, b"\x00", None, {STALETIP_FEATURE: {"prefers_blocks": False}}),
            (True, b"\x01", None, {STALETIP_FEATURE: {"prefers_blocks": True}}),
            (False, b"\x00", None, {}),
            (True, b"", "ignoring malformed BIP332 feature data", {}),
            (True, b"\x02", "ignoring malformed BIP332 feature data", {}),
            (True, b"\x00\x01", "ignoring malformed BIP332 feature data", {}),
        ]:
            with node.assert_debug_log([expected_log] if expected_log else [], timeout=2):
                peer = self.connect_peer(send_feature=send_feature, feature_data=feature_data)
            peer.sync_with_ping()
            assert_equal(node.getpeerinfo()[0]["features"], expected_features)
            if not expected_features:
                with node.assert_debug_log(["ignoring unnegotiated staletip"], timeout=2):
                    peer.send_and_ping(self.staletip_msg(block, fork_point_hash))
                assert peer.is_connected
                self.assert_staletip_not_tracked(block)
            node.disconnect_p2ps()

    def check_inbound_staletip(self, message, block, *, outcome, expected_log=None):
        """Send `message` from a negotiated peer and check the outcome: "track",
        "ignore", or "disconnect"."""
        node = self.nodes[0]
        peer = self.connect_peer()
        with node.assert_debug_log([expected_log] if expected_log else [], timeout=2):
            if outcome == "disconnect":
                peer.send_without_ping(message)
                peer.wait_for_disconnect()
            else:
                peer.send_and_ping(message)
        if outcome == "track":
            self.assert_staletip_tracked(block)
        else:
            self.assert_staletip_not_tracked(block)
        if outcome != "disconnect":
            assert peer.is_connected
        node.disconnect_p2ps()

    def test_inbound_staletip_handling(self):
        self.log.info("Test inbound staletip validation")
        node = self.nodes[0]

        def oversized():
            block, fork_point_hash = self.stale_block()
            message = self.staletip_msg(block, fork_point_hash)
            message.headers *= 21
            return message, block

        def raw(mutate):
            def build():
                block, fork_point_hash = self.stale_block()
                return msg_raw_staletip(mutate(self.staletip_msg(block, fork_point_hash).serialize())), block
            return build

        def invalid_pow():
            block, fork_point_hash = self.stale_block()
            message = self.staletip_msg(block, fork_point_hash)
            message.headers[0].nBits = 0x1d00ffff
            return message, block

        def unknown_fork_point():
            active_tip = node.getblock(node.getbestblockhash())
            block = create_block(hashprev=0xdead, height=active_tip["height"], ntime=active_tip["time"] + self.block_time_offset)
            self.block_time_offset += 1
            block.solve()
            return self.staletip_msg(block, 0xdead), block

        def time_too_old():
            fork_point = node.getblock(node.getblock(node.getbestblockhash())["previousblockhash"])
            block = create_block(hashprev=int(fork_point["hash"], 16), height=fork_point["height"] + 1, ntime=fork_point["mediantime"])
            block.solve()
            return self.staletip_msg(block, int(fork_point["hash"], 16)), block

        def within_recent_window():
            # Deeper than the default anti-DoS window of 144 blocks.
            block, fork_point_hash = self.stale_block(fork_depth=200)
            return self.staletip_msg(block, fork_point_hash), block

        self.generate(node, 250)
        for name, build, outcome, expected_log in [
            ("oversized", oversized, "ignore", "headers limit exceeded"),
            ("trailing byte", raw(lambda payload: payload + b"\x00"), "disconnect", "malformed staletip"),
            ("invalid have_block", raw(lambda payload: payload[:-1] + b"\x02"), "disconnect", "malformed staletip"),
            ("truncated", raw(lambda payload: payload[:-1]), "disconnect", "malformed staletip"),
            ("invalid proof of work", invalid_pow, "ignore", "with invalid headers"),
            ("unknown fork point", unknown_fork_point, "ignore", "with unknown fork point"),
            ("time too old", time_too_old, "ignore", "time-too-old"),
            ("within recency window", within_recent_window, "track", None),
        ]:
            self.log.info(f"- {name}")
            message, block = build()
            self.check_inbound_staletip(message, block, outcome=outcome, expected_log=expected_log)

        self.log.info("- already known tip")
        # A header with more work than the active tip is not tracked. Once a
        # competing block leaves it with equal work, only the staletip
        # message can track it.
        block, fork_point_hash = self.stale_block(fork_depth=0)
        assert_equal(node.submitheader(CBlockHeader(block).serialize().hex()), None)
        self.generate(node, 1)
        self.assert_staletip_not_tracked(block)
        peer = self.connect_peer()
        with node.assert_debug_log([], unexpected_msgs=["ignoring staletip"]):
            peer.send_and_ping(self.staletip_msg(block, fork_point_hash))
        self.assert_staletip_tracked(block)
        assert peer.is_connected
        node.disconnect_p2ps()

        self.log.info("- outside recency window")
        self.generate(node, max(0, STALETIP_RECENT_WINDOW + 2 - node.getblockcount()))
        fork_height = node.getblockcount() - STALETIP_RECENT_WINDOW - 2
        fork_point_hash = node.getblockhash(fork_height)
        block = create_block(hashprev=int(fork_point_hash, 16), height=fork_height + 1, ntime=node.getblock(fork_point_hash)["time"] + 1)
        block.solve()
        self.check_inbound_staletip(self.staletip_msg(block, int(fork_point_hash, 16)), block, outcome="ignore", expected_log="outside recency window")

        self.log.info("- low work")
        # With uniform regtest difficulty, a tip inside the recency window
        # always has enough work, so raise the threshold instead.
        chainwork = node.getblockheader(node.getbestblockhash())["chainwork"]
        self.restart_node(0, extra_args=["-staletips=headers", f"-minimumchainwork={chainwork}"])
        block, fork_point_hash = self.stale_block(fork_depth=2)
        self.check_inbound_staletip(self.staletip_msg(block, fork_point_hash), block, outcome="ignore", expected_log="ignoring low-work staletip")
        self.restart_node(0, extra_args=["-staletips=headers"])

    def test_higher_work_staletip_requests_block(self):
        self.log.info("Test block data is requested for a higher-work staletip")
        headers_peer = self.connect_peer()
        block, fork_point_hash = self.active_block()
        headers_peer.send_and_ping(self.staletip_msg(block, fork_point_hash, have_block=False))
        assert_equal(self.block_getdata_hashes(headers_peer), [])
        self.nodes[0].disconnect_p2ps()

        block_peer = self.connect_peer()
        block_peer.send_and_ping(self.staletip_msg(block, fork_point_hash, have_block=True))
        block_peer.wait_for_getdata_hash(block.hash_int)
        self.nodes[0].disconnect_p2ps()

        # Headers mode does not download a stale tip that has no more work.
        equal_work_peer = self.connect_peer()
        equal_work_block, equal_work_fork_point_hash = self.stale_block()
        equal_work_peer.send_and_ping(self.staletip_msg(equal_work_block, equal_work_fork_point_hash, have_block=True))
        self.assert_staletip_tracked(equal_work_block)
        equal_work_peer.sync_with_ping()
        assert_equal(self.block_getdata_hashes(equal_work_peer), [])
        self.nodes[0].disconnect_p2ps()

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
