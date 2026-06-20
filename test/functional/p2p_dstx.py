#!/usr/bin/env python3
# Copyright (c) 2026 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test P2P CoinJoin broadcast transaction handling.

Verifies that DSTX messages with an unverifiable (unknown) masternode incur
only a small misbehavior penalty, while clearly malformed DSTXes get the
existing stronger penalty. Also exercises the cumulative behavior so that
a peer flooding unknown-MN DSTXes is eventually discouraged.
"""

import time

from test_framework.messages import (
    CCoinJoinBroadcastTx,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    msg_dstx,
)
from test_framework.p2p import P2PInterface
from test_framework.script import (
    CScript,
    OP_CHECKSIG,
    OP_DUP,
    OP_EQUALVERIFY,
    OP_HASH160,
)
from test_framework.test_framework import BitcoinTestFramework

# Default DISCOURAGEMENT_THRESHOLD in net_processing.h.
DISCOURAGEMENT_THRESHOLD = 100
# Penalty applied when the relaying peer sends a DSTX whose masternode we
# can't find in the deterministic MN list (and therefore can't verify).
UNKNOWN_MN_SCORE = 1
# Penalty applied when the DSTX itself is structurally bad / bad signature.
INVALID_DSTX_SCORE = 10


class P2PDSTXTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-debug=net", "-debug=coinjoin"]]

    def make_dstx(self, nonce=0):
        tx = CTransaction()
        # The nonce flows into one of the prevouts so each DSTX has a distinct
        # txid (and therefore is not deduped by dstxman.GetDSTX).
        tx.vin = [CTxIn(COutPoint(hash=(nonce << 8) | (i + 1), n=0)) for i in range(3)]
        p2pkh = CScript([
            OP_DUP,
            OP_HASH160,
            b"\x01" * 20,
            OP_EQUALVERIFY,
            OP_CHECKSIG,
        ])
        # CoinJoin::IsDenominatedAmount requires a recognised denom; the
        # smallest denom is 0.001 DASH + 0.0000001 fee == COIN//1000 + 1.
        tx.vout = [CTxOut(nValue=COIN // 1000 + 1, scriptPubKey=p2pkh) for _ in tx.vin]
        return CCoinJoinBroadcastTx(
            tx=tx,
            m_protxHash=1,
            vchSig=b"\x01" * 96,
            sigTime=int(time.time()),
        )

    def run_test(self):
        node = self.nodes[0]
        self.log.info("Leave IBD so unsolicited DSTX is processed")
        self.generate(node, 1)

        self.log.info("Unknown-MN DSTX => small (+%d) misbehavior penalty", UNKNOWN_MN_SCORE)
        peer = node.add_p2p_connection(P2PInterface())
        # Match the substring of the Misbehaving log that identifies the score
        # jump of a fresh peer (0 -> 1) along with our message tag.
        with node.assert_debug_log([
            "Can't find masternode",
            "Misbehaving",
            "(0 -> {})".format(UNKNOWN_MN_SCORE),
            "invalid dstx",
        ]):
            peer.send_and_ping(msg_dstx(self.make_dstx(nonce=1)))

        self.log.info("Structurally invalid DSTX => stronger (+%d) misbehavior penalty", INVALID_DSTX_SCORE)
        peer_invalid = node.add_p2p_connection(P2PInterface())
        bad = self.make_dstx(nonce=2)
        bad.tx.vout.pop()  # vin.size() != vout.size() trips IsValidStructure
        with node.assert_debug_log([
            "Invalid DSTX structure",
            "Misbehaving",
            "(0 -> {})".format(INVALID_DSTX_SCORE),
            "invalid dstx",
        ]):
            peer_invalid.send_and_ping(msg_dstx(bad))

        self.log.info("A peer flooding unknown-MN DSTXes is eventually discouraged")
        peer_flood = node.add_p2p_connection(P2PInterface())
        # +1 per unknown-MN DSTX, so DISCOURAGEMENT_THRESHOLD distinct DSTXes
        # are enough to cross the threshold exactly once on this peer.
        # send_message is fire-and-forget; the node disconnects once
        # discouraged, so we wait on the threshold log line directly and then
        # confirm the peer was dropped.
        with node.assert_debug_log(["DISCOURAGE THRESHOLD EXCEEDED"], timeout=10):
            for nonce in range(DISCOURAGEMENT_THRESHOLD):
                # offset nonces to avoid txid clashes with earlier sends
                peer_flood.send_message(msg_dstx(self.make_dstx(nonce=1000 + nonce)))
        peer_flood.wait_for_disconnect()


if __name__ == '__main__':
    P2PDSTXTest().main()
