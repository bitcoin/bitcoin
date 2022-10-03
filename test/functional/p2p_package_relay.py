#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test package relay messages and net processing logic on a singular node.
This has its own test because it requires lots of setmocktimeing and that's hard to coordinate
across multiple nodes.
"""

import time

from test_framework.messages import (
    CInv,
    get_combined_hash,
    get_package_hash,
    MSG_ANCPKGINFO,
    MSG_PKGTXNS,
    MSG_WITNESS_TX,
    MSG_WTX,
    msg_ancpkginfo,
    msg_getdata,
    msg_inv,
    msg_notfound,
    msg_getpkgtxns,
    msg_pkgtxns,
    msg_sendpackages,
    msg_tx,
    msg_verack,
    msg_wtxidrelay,
    PKG_RELAY_ANCPKG,
)
from test_framework.p2p import (
    NONPREF_PEER_TX_DELAY,
    OVERLOADED_PEER_TX_DELAY,
    ORPHAN_ANCESTOR_GETDATA_INTERVAL,
    p2p_lock,
    P2PTxInvStore,
    TXID_RELAY_DELAY,
    UNCONDITIONAL_RELAY_DELAY,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
)
from test_framework.wallet import (
    COIN,
    DEFAULT_FEE,
    MiniWallet,
)

# Time to fastforward (using setmocktime) before waiting for the node to send getdata(tx) in response
# to an inv(tx), in seconds. This delay includes all possible delays + 1, so it should only be used
# when the value of the delay is not interesting. If we want to test that the node waits x seconds
# for one peer and y seconds for another, use specific values instead.
TXREQUEST_TIME_SKIP = NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY + OVERLOADED_PEER_TX_DELAY + 1

# Time to fastfoward (using setmocktime) in between subtests to ensure they do not interfere with
# one another, in seconds. Equal to 12 hours, which is enough to expire anything that may exist
# (though nothing should since state should be cleared) in p2p data structures.
LONG_TIME_SKIP = 12 * 60 * 60

def cleanup(func):
    def wrapper(self):
        assert len(self.nodes[0].getpeerinfo()) == 0
        assert len(self.nodes[0].getrawmempool()) == 0
        try:
            func(self)
        finally:
            # Clear mempool
            self.generate(self.nodes[0], 1)
            self.nodes[0].disconnect_p2ps()
            self.mocktime += LONG_TIME_SKIP
            self.nodes[0].setmocktime(self.mocktime)
    return wrapper

class PackageRelayer(P2PTxInvStore):
    def __init__(self, send_sendpackages=True, send_wtxidrelay=True):
        super().__init__()
        # List versions of each sendpackages received
        self._sendpackages_received = []
        self._send_sendpackages = send_sendpackages
        self._send_wtxidrelay = send_wtxidrelay
        self._ancpkginfo_received = []
        self._tx_received = []
        self._getdata_received = []

    @property
    def sendpackages_received(self):
        with p2p_lock:
            return self._sendpackages_received

    @property
    def ancpkginfo_received(self):
        with p2p_lock:
            return self._ancpkginfo_received

    @property
    def tx_received(self):
        with p2p_lock:
            return self._tx_received

    @property
    def getdata_received(self):
        with p2p_lock:
            return self._getdata_received

    def on_version(self, message):
        if self._send_wtxidrelay:
            self.send_message(msg_wtxidrelay())
        if self._send_sendpackages:
            sendpackages_message = msg_sendpackages()
            sendpackages_message.versions = PKG_RELAY_ANCPKG
            self.send_message(sendpackages_message)
        self.send_message(msg_verack())
        self.nServices = message.nServices

    def on_sendpackages(self, message):
        self._sendpackages_received.append(message.versions)

    def on_ancpkginfo(self, message):
        self._ancpkginfo_received.append(message.wtxids)

    def on_tx(self, message):
        self._tx_received.append(message)

    def on_getdata(self, message):
        self._getdata_received.append(message)

    def wait_for_getancpkginfo(self, wtxid16):
        def test_function():
            last_getdata = self.last_message.get('getdata')
            if not last_getdata:
                return False
            return last_getdata.inv[0].hash == wtxid16 and last_getdata.inv[0].type == MSG_ANCPKGINFO
        self.wait_until(test_function, timeout=10)

    def wait_for_getdata_txids(self, txids):
        def test_function():
            last_getdata = self.last_message.get('getdata')
            if not last_getdata:
                return False
            return all([item.type == MSG_WITNESS_TX and item.hash in txids for item in last_getdata.inv])
        self.wait_until(test_function, timeout=10)

    def wait_for_pkgtxns(self, wtxids):
        def test_function():
            last_pkgtxns = self.last_message.get('pkgtxns')
            if not last_pkgtxns:
                return False
            return [tx.getwtxid() for tx in last_pkgtxns.txns] == wtxids
        self.wait_until(test_function, timeout=10)

    def wait_for_notfound(self, expected):
        def test_function():
            last_notfound = self.last_message.get('notfound')
            if not last_notfound:
                return False
            return any([item.type == expected.type and item.hash == expected.hash for item in last_notfound.vec])
        self.wait_until(test_function, timeout=10)

class PackageRelayTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-packagerelay=1"]]
        self.mocktime = int(time.time())

    def create_package(self, cpfp=True):
        """Create package with these transactions:
        - Parent 1: fee=default
        - Parent 2: fee=0 if cpfp, else default
        - Child:    fee=high
        """
        parent1 = self.wallet.create_self_transfer()
        parent2 = self.wallet.create_self_transfer(fee_rate=0, fee=0) if cpfp else self.wallet.create_self_transfer()
        child = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[parent1['new_utxo'], parent2["new_utxo"]],
            num_outputs=1,
            fee_per_output=int(DEFAULT_FEE * COIN)
        )
        package_hex = [parent1["hex"], parent2["hex"], child["hex"]]
        package_txns = [parent1["tx"], parent2["tx"], child["tx"]]
        package_wtxids = [tx.getwtxid() for tx in package_txns]
        return package_hex, package_txns, package_wtxids

    def fastforward(self, seconds):
        """Convenience helper function to fast-forward, so we don't need to keep track of the
        starting time when we call setmocktime."""
        self.mocktime += seconds
        self.nodes[0].setmocktime(self.mocktime)

    @cleanup
    def test_sendpackages(self):
        self.log.info("Test sendpackages during version handshake")
        node = self.nodes[0]
        peer_normal = node.add_p2p_connection(PackageRelayer())
        assert_equal(node.getpeerinfo()[0]["bytesrecv_per_msg"]["sendpackages"], 32)
        assert_equal(node.getpeerinfo()[0]["bytessent_per_msg"]["sendpackages"], 32)
        assert_equal(peer_normal.sendpackages_received, [PKG_RELAY_ANCPKG])
        assert node.getpeerinfo()[0]["relaytxpackages"]
        node.disconnect_p2ps()

        self.log.info("Test sendpackages without wtxid relay")
        node = self.nodes[0]
        peer_no_wtxidrelay = node.add_p2p_connection(PackageRelayer(send_wtxidrelay=False))
        assert_equal(node.getpeerinfo()[0]["bytesrecv_per_msg"]["sendpackages"], 32)
        assert_equal(node.getpeerinfo()[0]["bytessent_per_msg"]["sendpackages"], 32)
        assert_equal(peer_no_wtxidrelay.sendpackages_received, [PKG_RELAY_ANCPKG])
        assert not node.getpeerinfo()[0]["relaytxpackages"]
        node.disconnect_p2ps()

        self.log.info("Test sendpackages is sent even so")
        node = self.nodes[0]
        peer_no_sendpackages = node.add_p2p_connection(PackageRelayer(send_sendpackages=False))
        # Sendpackages should still be sent
        assert_equal(node.getpeerinfo()[0]["bytessent_per_msg"]["sendpackages"], 32)
        assert "sendpackages" not in node.getpeerinfo()[0]["bytesrecv_per_msg"]
        assert_equal(peer_no_sendpackages.sendpackages_received, [PKG_RELAY_ANCPKG])
        assert not node.getpeerinfo()[0]["relaytxpackages"]
        node.disconnect_p2ps()

        self.log.info("Test disconnection if sendpackages is sent after version handshake")
        peer_sendpackages_after_verack = node.add_p2p_connection(P2PTxInvStore())
        sendpackages_message = msg_sendpackages()
        sendpackages_message.versions = PKG_RELAY_ANCPKG
        peer_sendpackages_after_verack.send_message(sendpackages_message)
        peer_sendpackages_after_verack.wait_for_disconnect()

    @cleanup
    def test_ancpkginfo_requests(self):
        node = self.nodes[0]
        peer_info_requester = node.add_p2p_connection(PackageRelayer())
        package_hex, package_txns, package_wtxids = self.create_package(cpfp=False)
        node.submitpackage(package_hex)
        assert_equal(node.getmempoolentry(package_txns[-1].rehash())["ancestorcount"], 3)

        self.log.info("Test that ancpkginfo requests for unannounced transactions are ignored until UNCONDITIONAL_RELAY_DELAY elapses")
        tx_request = msg_getdata([CInv(t=MSG_WTX, h=int(package_wtxids[-1], 16))])
        peer_info_requester.send_and_ping(tx_request)
        assert not peer_info_requester.tx_received
        child_ancpkginfo_request = msg_getdata([CInv(t=MSG_ANCPKGINFO, h=int(package_wtxids[-1], 16))])
        peer_info_requester.send_and_ping(child_ancpkginfo_request)
        assert not peer_info_requester.ancpkginfo_received

        self.fastforward(UNCONDITIONAL_RELAY_DELAY)
        peer_info_requester.send_and_ping(tx_request)
        peer_info_requester.wait_for_tx(package_txns[-1].rehash())

        self.log.info("Test that node responds to ancpkginfo request with ancestor package wtxids")
        peer_info_requester.send_and_ping(child_ancpkginfo_request)
        assert_greater_than_or_equal(len(peer_info_requester.ancpkginfo_received), 1)
        last_ancpkginfo_received = peer_info_requester.ancpkginfo_received.pop()
        assert all([int(wtxid, 16) in last_ancpkginfo_received for wtxid in package_wtxids])
        # When a tx has no unconfirmed ancestors, ancpkginfo just contains its own wtxid
        for i in range(len(package_txns) - 1):
            parent_ancpkginfo_request = msg_getdata([CInv(t=MSG_ANCPKGINFO, h=int(package_wtxids[i], 16))])
            peer_info_requester.send_and_ping(parent_ancpkginfo_request)
            assert_equal([int(package_wtxids[i], 16)], peer_info_requester.ancpkginfo_received.pop())

    @cleanup
    def test_orphan_get_ancpkginfo(self):
        self.log.info("Test that nodes deal with orphans by requesting ancestor package info")
        node = self.nodes[0]
        package_hex, package_txns, package_wtxids = self.create_package()
        orphan_tx = package_txns[-1]
        orphan_wtxid = package_wtxids[-1]

        peer_package_relayer = node.add_p2p_connection(PackageRelayer())
        orphan_inv = CInv(t=MSG_WTX, h=int(orphan_wtxid, 16))
        peer_package_relayer.send_and_ping(msg_inv([orphan_inv]))
        self.fastforward(NONPREF_PEER_TX_DELAY + 1)
        peer_package_relayer.wait_for_getdata([int(orphan_wtxid, 16)])
        peer_package_relayer.send_and_ping(msg_tx(orphan_tx))

        self.fastforward(NONPREF_PEER_TX_DELAY + 1)
        peer_package_relayer.sync_with_ping()
        peer_package_relayer.wait_for_getancpkginfo(int(orphan_wtxid, 16))
        peer_package_relayer.send_and_ping(msg_ancpkginfo([int(wtxid, 16) for wtxid in package_wtxids]))
        self.wait_until(lambda: "ancpkginfo" in node.getpeerinfo()[0]["bytesrecv_per_msg"])

    @cleanup
    def test_orphan_handling_prefer_outbound(self):
        self.log.info("Test that the node uses all announcers as potential candidates for orphan handling")
        node = self.nodes[0]
        package_hex, package_txns, package_wtxids = self.create_package()
        orphan_tx = package_txns[-1]
        orphan_wtxid = package_wtxids[-1]
        orphan_inv = CInv(t=MSG_WTX, h=int(orphan_wtxid, 16))

        peer_inbound = node.add_p2p_connection(PackageRelayer())
        peer_outbound = node.add_outbound_p2p_connection(PackageRelayer(), p2p_idx=1)

        peer_inbound.send_and_ping(msg_inv([orphan_inv]))
        self.fastforward(NONPREF_PEER_TX_DELAY + 1)
        peer_inbound.wait_for_getdata([int(orphan_wtxid, 16)])
        # Both send invs for the orphan, so the node an expect both to know its ancestors.
        peer_outbound.send_and_ping(msg_inv([orphan_inv]))
        peer_inbound.send_and_ping(msg_tx(orphan_tx))
        self.fastforward(NONPREF_PEER_TX_DELAY)

        self.log.info("Test that the node prefers requesting ancpkginfo from outbound peers")
        # The outbound peer should be preferred for getting ancpkginfo
        peer_outbound.wait_for_getancpkginfo(int(orphan_wtxid, 16))
        # There should be no request to the inbound peer
        ancpkginfo_request = peer_outbound.getdata_received.pop()
        assert ancpkginfo_request not in peer_inbound.getdata_received

        self.log.info("Test that, if the preferred peer doesn't respond, the node sends another request")
        self.fastforward(ORPHAN_ANCESTOR_GETDATA_INTERVAL)
        peer_inbound.sync_with_ping()
        peer_inbound.wait_for_getancpkginfo(int(orphan_wtxid, 16))

    @cleanup
    def test_orphan_handling_prefer_ancpkginfo(self):
        node = self.nodes[0]
        package_hex, package_txns, package_wtxids = self.create_package()
        orphan_tx = package_txns[-1]
        orphan_wtxid = package_wtxids[-1]
        orphan_inv = CInv(t=MSG_WTX, h=int(orphan_wtxid, 16))

        peer_legacy = node.add_p2p_connection(PackageRelayer(send_sendpackages=False))
        peer_package_relay = node.add_p2p_connection(PackageRelayer())
        assert not node.getpeerinfo()[0]["relaytxpackages"]
        assert node.getpeerinfo()[1]["relaytxpackages"]

        peer_legacy.send_and_ping(msg_inv([orphan_inv]))
        self.fastforward(TXREQUEST_TIME_SKIP)
        peer_legacy.wait_for_getdata([int(orphan_wtxid, 16)])
        # Both send invs for the orphan, so the node can expect both to know its ancestors.
        peer_package_relay.send_and_ping(msg_inv([orphan_inv]))
        peer_legacy.send_and_ping(msg_tx(orphan_tx))

        # Tx is an orphan. The package relay peer is preferred for orphan resolution.
        nonpackage_prev_getdata = len(peer_legacy.getdata_received)
        self.fastforward(NONPREF_PEER_TX_DELAY)
        peer_package_relay.sync_with_ping()
        peer_legacy.sync_with_ping()
        peer_package_relay.wait_for_getancpkginfo(int(orphan_wtxid, 16))
        # The legacy peer should not have received any request.
        assert_equal(nonpackage_prev_getdata, len(peer_legacy.getdata_received))

        self.log.info("Test that, if the package relay peer doesn't respond, node falls back to parent txids")
        self.fastforward(ORPHAN_ANCESTOR_GETDATA_INTERVAL)
        peer_legacy.wait_for_getdata_txids([int(tx.rehash(), 16) for tx in package_txns[:-1]])

    @cleanup
    def test_orphan_announcer_memory(self):
        self.log.info("Test that the node remembers who announced orphan transactions")
        node = self.nodes[0]
        package_hex, package_txns, package_wtxids = self.create_package()
        orphan_tx = package_txns[-1]
        orphan_wtxid = package_wtxids[-1]
        orphan_inv = CInv(t=MSG_WTX, h=int(orphan_wtxid, 16))

        # Original announcer of orphan
        peer_package_relay1 = node.add_outbound_p2p_connection(PackageRelayer(), p2p_idx=2)
        # Sends an inv for the orphan before the node requests orphan tx data.
        # Preferred for orphan handling over peer3 because it's an outbound connection.
        peer_package_relay2 = node.add_p2p_connection(PackageRelayer())
        # Sends an inv for the orphan while the node is requesting ancpkginfo
        peer_package_relay3 = node.add_p2p_connection(PackageRelayer())

        peer_package_relay1.send_and_ping(msg_inv([orphan_inv]))
        self.fastforward(NONPREF_PEER_TX_DELAY + 1)
        peer_package_relay1.wait_for_getdata([int(orphan_wtxid, 16)])

        # Both send invs for the orphan, so the node an expect both to know its ancestors.
        peer_package_relay2.send_and_ping(msg_inv([orphan_inv]))
        peer_package_relay1.send_and_ping(msg_tx(orphan_tx))

        peer2_prev_getdata = len(peer_package_relay2.getdata_received)
        peer3_prev_getdata = len(peer_package_relay3.getdata_received)
        self.fastforward(NONPREF_PEER_TX_DELAY)
        peer_package_relay1.wait_for_getancpkginfo(int(orphan_wtxid, 16))
        peer_package_relay3.send_and_ping(msg_inv([orphan_inv]))
        # Peers 2 and 3 should not have received any getdata
        # Not for tx data of the orphan, not for ancpkginfo, and not for parent txids
        assert_equal(peer2_prev_getdata, len(peer_package_relay2.getdata_received))
        assert_equal(peer3_prev_getdata, len(peer_package_relay3.getdata_received))

        self.log.info("Test that the node requests ancpkginfo from a different peer upon receiving notfound")
        orphan_ancpkginfo_notfound = msg_notfound(vec=[CInv(MSG_ANCPKGINFO, int(orphan_wtxid, 16))])
        peer_package_relay1.send_and_ping(orphan_ancpkginfo_notfound)
        self.fastforward(1)
        # Node should try again from peer2
        peer_package_relay2.wait_for_getancpkginfo(int(orphan_wtxid, 16))

        self.log.info("Test that the node requests ancpkginfo from a different peer if peer disconnects")
        # Peer 2 disconnected before responding
        peer_package_relay2.peer_disconnect()
        self.fastforward(1)
        peer_package_relay3.sync_with_ping()
        peer_package_relay3.wait_for_getancpkginfo(int(orphan_wtxid, 16))

    @cleanup
    def test_ancpkginfo_received(self):
        node = self.nodes[0]
        parent1 = self.wallet.create_self_transfer()
        parent2 = self.wallet.create_self_transfer()
        child = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[parent1['new_utxo'], parent2["new_utxo"]],
            num_outputs=1,
            fee_per_output=int(DEFAULT_FEE * COIN)
        )
        package_txns = [parent1["tx"], parent2["tx"], child["tx"]]
        package_wtxids = [tx.getwtxid() for tx in package_txns]
        ancpkginfo_message = msg_ancpkginfo([int(wtxid, 16) for wtxid in package_wtxids])

        self.log.info("Test that unsolicited ancpkginfo results in disconnection")
        peer1 = node.add_p2p_connection(PackageRelayer())
        peer1.send_message(ancpkginfo_message)
        peer1.wait_for_disconnect()

        self.log.info("Test that peer uses ancpkginfo to request orphan's ancestors")
        orphan_inv = CInv(t=MSG_WTX, h=int(package_wtxids[-1], 16))
        peer2 = node.add_p2p_connection(PackageRelayer())
        peer2.send_and_ping(msg_inv([orphan_inv]))
        self.fastforward(NONPREF_PEER_TX_DELAY + 1)
        peer2.wait_for_getdata([int(package_wtxids[-1], 16)])
        peer2.send_and_ping(msg_tx(package_txns[-1]))
        self.fastforward(NONPREF_PEER_TX_DELAY + 1)
        peer2.wait_for_getancpkginfo(int(package_wtxids[-1], 16))
        peer2.send_and_ping(ancpkginfo_message)

    @cleanup
    def test_pkgtxns(self):
        node = self.nodes[0]
        self.log.info("Test that node does not respond to an unannounced getpkgtxns until after relay delay")
        package_hex, package_txns, package_wtxids = self.create_package(cpfp=False)
        # Grind wtxids until they are not in lexicographical order
        while True:
            sorted_copy = [x for x in package_wtxids]
            sorted_copy.sort()
            if sorted_copy == package_wtxids:
                package_hex, package_txns, package_wtxids = self.create_package(cpfp=False)
            else:
                break

        node.submitpackage(package_hex)
        sorted_wtxids = [x for x in package_wtxids]
        sorted_wtxids.sort()

        sorted_uints = [int(x, 16) for x in package_wtxids]
        sorted_uints.sort()
        assert sorted_wtxids != package_wtxids
        assert_equal(set(sorted_wtxids), set(package_wtxids))

        # Peer is added only after the transactions were submitted, so they are not announced.
        peer_requester_unannounced = node.add_p2p_connection(PackageRelayer())
        getpkgtxns_request = msg_getpkgtxns([int(wtxid, 16) for wtxid in package_wtxids])
        peer_requester_unannounced.send_and_ping(getpkgtxns_request)

        packagehash = get_package_hash(package_txns)
        assert_equal(get_combined_hash(getpkgtxns_request.hashes), get_package_hash(package_txns))
        peer_requester_unannounced.wait_for_notfound(expected=CInv(t=MSG_PKGTXNS, h=packagehash))
        assert "pkgtxns" not in peer_requester_unannounced.last_message

        self.fastforward(UNCONDITIONAL_RELAY_DELAY)

        self.log.info("Test that node responds to getpkgtxns with notfound if any transactions are unavailable")
        # This transaction is not present in the mempool.
        new_tx = self.wallet.create_self_transfer()
        req_partially_unavailable = [package_wtxids[0], new_tx["tx"].getwtxid()]
        unavailable_getpkgtxns = msg_getpkgtxns([int(wtxid, 16) for wtxid in req_partially_unavailable])
        peer_requester_unannounced.send_and_ping(unavailable_getpkgtxns)
        partially_unavailable_sorted = [x for x in req_partially_unavailable]
        partially_unavailable_sorted.sort()
        unavailablehash = get_combined_hash(unavailable_getpkgtxns.hashes)
        peer_requester_unannounced.wait_for_notfound(expected=CInv(t=MSG_PKGTXNS, h=unavailablehash))

        self.log.info("Test that node responds to getpkgtxns with pkgtxns")
        peer_requester_unannounced.send_and_ping(getpkgtxns_request)
        peer_requester_unannounced.wait_for_pkgtxns(package_wtxids)

        self.log.info("Test that node disconnects peers that request more than 100 wtxids in getpkgtxns")
        peer_requester_101 = node.add_p2p_connection(PackageRelayer())
        somewtxid = package_wtxids[0]
        getpkgtxns_101 = msg_getpkgtxns([int(somewtxid, 16) + i for i in range(101)])
        peer_requester_101.send_message(getpkgtxns_101)
        peer_requester_101.wait_for_disconnect()

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        self.generate(self.wallet, 160)

        self.test_sendpackages()
        self.test_ancpkginfo_requests()
        self.test_orphan_get_ancpkginfo()
        self.test_orphan_handling_prefer_outbound()
        self.test_orphan_handling_prefer_ancpkginfo()
        self.test_orphan_announcer_memory()
        self.test_ancpkginfo_received()
        self.test_pkgtxns()


if __name__ == '__main__':
    PackageRelayTest().main()
