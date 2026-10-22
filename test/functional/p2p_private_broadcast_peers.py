#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that a private broadcast job cuts off a recipient that floods it, at the receive cap.

The harness is in test_framework/privbcast.py.
"""

from test_framework.messages import (
    CInv,
    MSG_TX,
    msg_inv,
)
from test_framework.p2p import NetworkThread
from test_framework.privbcast import (
    HANDSHAKE_TIMEOUT_S,
    MAX_RECV_BYTES,
    TIME_DIVISOR,
    PrivbcastJobTest,
    Recipient,
)
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)


class FloodingRecipient(Recipient):
    """Floods the connection with announcements once the handshake is done, well past the receive cap,
    and never asks for the transaction, so only the cap can end its attempt early. One message per
    turn of the event loop: encrypting one takes long enough in Python that a burst would hold up the
    other scripted peers' handshakes."""
    MESSAGES = 10  # about 36 kB each

    def on_inv(self, message):
        pass

    def on_verack(self, message):
        super().on_verack(message)
        junk = msg_inv([CInv(MSG_TX, i) for i in range(1, 1001)])

        def flood(left):
            if left and self.is_connected:
                self.send_without_ping(junk)
                NetworkThread.network_event_loop.call_soon(flood, left - 1)
        flood(self.MESSAGES)


class P2PPrivateBroadcastPeers(PrivbcastJobTest):
    def set_test_params(self):
        super().set_test_params()

    def run_test(self):
        self.setup_wallet()
        self.test_flooding_peer()

    def test_flooding_peer(self):
        self.log.info("A flooding peer is cut at the receive cap")
        flooder = "9.3.0.4"
        self.start_proxy({"w.seed.": [flooder]}, {flooder: (FloodingRecipient, True)})
        tx = self.wallet.create_self_transfer()
        job, report = self.run_job(tx)
        assert_equal(job["state"], "done")
        flooded = self.attempts_to(report, flooder)
        assert_equal(len(flooded), 1)
        a = flooded[0]
        # Whether the flooder was sent the INV before the cap cut it is a race.
        assert a["outcome"] in ("not_announced", "post_announcement_failure"), a
        # Past the cap, then cut, at most one read (READ_SIZE in job.cpp) past it, and early.
        assert_greater_than_or_equal(a["bytes_recv"], MAX_RECV_BYTES)
        assert_greater_than_or_equal(MAX_RECV_BYTES + 64 * 1024, a["bytes_recv"])
        assert_greater_than(HANDSHAKE_TIMEOUT_S * 1000 / TIME_DIVISOR, a["ended_ms"] - a["scheduled_start_ms"])
        self.stop_proxy()


if __name__ == '__main__':
    P2PPrivateBroadcastPeers(__file__).main()
