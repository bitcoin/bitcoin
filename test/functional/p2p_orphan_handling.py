#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import time

from test_framework.mempool_util import tx_in_orphanage
from test_framework.messages import (
    CInv,
    CTxInWitness,
    MSG_TX,
    MSG_WITNESS_TX,
    MSG_WTX,
    msg_getdata,
    msg_inv,
    msg_notfound,
    msg_tx,
    tx_from_hex,
)
from test_framework.p2p import (
    GETDATA_TX_INTERVAL,
    NONPREF_PEER_TX_DELAY,
    OVERLOADED_PEER_TX_DELAY,
    p2p_lock,
    P2PInterface,
    P2PTxInvStore,
    TXID_RELAY_DELAY,
)
from test_framework.util import (
    assert_not_equal,
    assert_equal,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)

# Time to bump forward (using setmocktime) before waiting for the node to send getdata(tx) in response
# to an inv(tx), in seconds. This delay includes all possible delays + 1, so it should only be used
# when the value of the delay is not interesting. If we want to test that the node waits x seconds
# for one peer and y seconds for another, use specific values instead.
TXREQUEST_TIME_SKIP = NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY + OVERLOADED_PEER_TX_DELAY + 1

DEFAULT_MAX_ORPHAN_TRANSACTIONS = 100

def cleanup(func):
    # Time to fastfoward (using setmocktime) in between subtests to ensure they do not interfere with
    # one another, in seconds. Equal to 12 hours, which is enough to expire anything that may exist
    # (though nothing should since state should be cleared) in p2p data structures.
    LONG_TIME_SKIP = 12 * 60 * 60

    def wrapper(self):
        try:
            func(self)
        finally:
            # Clear mempool
            self.generate(self.nodes[0], 1)
            self.nodes[0].disconnect_p2ps()
            self.nodes[0].bumpmocktime(LONG_TIME_SKIP)
            # Check that mempool and orphanage have been cleared
            self.wait_until(lambda: len(self.nodes[0].getorphantxs()) == 0)
            assert_equal(0, len(self.nodes[0].getrawmempool()))
            self.wallet.rescan_utxos(include_mempool=True)
    return wrapper

class PeerTxRelayer(P2PTxInvStore):
    """A P2PTxInvStore that also remembers all of the getdata and tx messages it receives."""
    def __init__(self, wtxidrelay=True):
        super().__init__(wtxidrelay=wtxidrelay)
        self._tx_received = []
        self._getdata_received = []

    @property
    def tx_received(self):
        with p2p_lock:
            return self._tx_received

    @property
    def getdata_received(self):
        with p2p_lock:
            return self._getdata_received

    def on_tx(self, message):
        self._tx_received.append(message)

    def on_getdata(self, message):
        self._getdata_received.append(message)

    def wait_for_parent_requests(self, txids):
        """Wait for requests for missing parents by txid with witness data (MSG_WITNESS_TX or
        WitnessTx). Requires that the getdata message match these txids exactly; all txids must be
        requested and no additional requests are allowed."""
        def test_function():
            last_getdata = self.last_message.get('getdata')
            if not last_getdata:
                return False
            return len(last_getdata.inv) == len(txids) and all([item.type == MSG_WITNESS_TX and item.hash in txids for item in last_getdata.inv])
        self.wait_until(test_function, timeout=10)

    def assert_no_immediate_response(self, message):
        """Check that the node does not immediately respond to this message with any of getdata,
        inv, tx. The node may respond later.
        """
        prev_lastmessage = self.last_message
        self.send_and_ping(message)
        after_lastmessage = self.last_message
        for msgtype in ["getdata", "inv", "tx"]:
            if msgtype not in prev_lastmessage:
                assert msgtype not in after_lastmessage
            else:
                assert_equal(prev_lastmessage[msgtype], after_lastmessage[msgtype])

    def assert_never_requested(self, txhash):
        """Check that the node has never sent us a getdata for this hash (int type)"""
        self.sync_with_ping()
        for getdata in self.getdata_received:
            for request in getdata.inv:
                assert_not_equal(request.hash, txhash)

class OrphanHandlingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[]]

    def create_parent_and_child(self):
        """Create package with 1 parent and 1 child, normal fees (no cpfp)."""
        parent = self.wallet.create_self_transfer()
        child = self.wallet.create_self_transfer(utxo_to_spend=parent['new_utxo'])
        return child["tx"].wtxid_hex, child["tx"], parent["tx"]

    def relay_transaction(self, peer, tx):
        """Relay transaction using MSG_WTX"""
        wtxid = tx.wtxid_int
        peer.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=wtxid)]))
        self.nodes[0].bumpmocktime(TXREQUEST_TIME_SKIP)
        peer.wait_for_getdata([wtxid])
        peer.send_and_ping(msg_tx(tx))

    def create_malleated_version(self, tx):
        """
        Create a malleated version of the tx where the witness is replaced with garbage data.
        Returns a CTransaction object.
        """
        tx_bad_wit = tx_from_hex(tx["hex"])
        tx_bad_wit.wit.vtxinwit = [CTxInWitness()]
        # Add garbage data to witness 0. We cannot simply strip the witness, as the node would
        # classify it as a transaction in which the witness was missing rather than wrong.
        tx_bad_wit.wit.vtxinwit[0].scriptWitness.stack = [b'garbage']

        assert_equal(tx["txid"], tx_bad_wit.txid_hex)
        assert_not_equal(tx["wtxid"], tx_bad_wit.wtxid_hex)

        return tx_bad_wit

    @cleanup
    def test_arrival_timing_orphan(self):
        self.log.info("Test missing parents that arrive during delay are not requested")
        node = self.nodes[0]
        tx_parent_arrives = self.wallet.create_self_transfer()
        tx_parent_doesnt_arrive = self.wallet.create_self_transfer()
        # Fake orphan spends nonexistent outputs of the two parents
        tx_fake_orphan = self.wallet.create_self_transfer_multi(utxos_to_spend=[
            {"txid": tx_parent_doesnt_arrive["txid"], "vout": 10, "value": tx_parent_doesnt_arrive["new_utxo"]["value"]},
            {"txid": tx_parent_arrives["txid"], "vout": 10, "value": tx_parent_arrives["new_utxo"]["value"]}
        ])

        peer_spy = node.add_p2p_connection(PeerTxRelayer())
        peer_normal = node.add_p2p_connection(PeerTxRelayer())
        # This transaction is an orphan because it is missing inputs. It is a "fake" orphan that the
        # spy peer has crafted to learn information about tx_parent_arrives even though it isn't
        # able to spend a real output of it, but it could also just be a normal, real child tx.
        # The node should not immediately respond with a request for orphan parents.
        # Also, no request should be sent later because it will be resolved by
        # the time the request is scheduled to be sent.
        peer_spy.assert_no_immediate_response(msg_tx(tx_fake_orphan["tx"]))

        # Node receives transaction. It attempts to obfuscate the exact timing at which this
        # transaction entered its mempool. Send unsolicited because otherwise we need to wait for
        # request delays.
        peer_normal.send_and_ping(msg_tx(tx_parent_arrives["tx"]))
        assert tx_parent_arrives["txid"] in node.getrawmempool()

        # Spy peer should not be able to query the node for the parent yet, since it hasn't been
        # announced / insufficient time has elapsed.
        parent_inv = CInv(t=MSG_WTX, h=tx_parent_arrives["tx"].wtxid_int)
        assert_equal(len(peer_spy.get_invs()), 0)
        peer_spy.assert_no_immediate_response(msg_getdata([parent_inv]))

        # Request would be scheduled with this delay because it is not a preferred relay peer.
        self.nodes[0].bumpmocktime(NONPREF_PEER_TX_DELAY)
        peer_spy.assert_never_requested(int(tx_parent_arrives["txid"], 16))
        peer_spy.assert_never_requested(int(tx_parent_doesnt_arrive["txid"], 16))
        # Request would be scheduled with this delay because it is by txid.
        self.nodes[0].bumpmocktime(TXID_RELAY_DELAY)
        peer_spy.wait_for_parent_requests([int(tx_parent_doesnt_arrive["txid"], 16)])
        peer_spy.assert_never_requested(int(tx_parent_arrives["txid"], 16))

    @cleanup
    def test_orphan_rejected_parents_exceptions(self):
        node = self.nodes[0]
        peer1 = node.add_p2p_connection(PeerTxRelayer())
        peer2 = node.add_p2p_connection(PeerTxRelayer())

        self.log.info("Test orphan handling when a nonsegwit parent is known to be invalid")
        parent_low_fee_nonsegwit = self.wallet_nonsegwit.create_self_transfer(fee_rate=0)
        assert_equal(parent_low_fee_nonsegwit["txid"], parent_low_fee_nonsegwit["tx"].wtxid_hex)
        parent_other = self.wallet_nonsegwit.create_self_transfer()
        child_nonsegwit = self.wallet_nonsegwit.create_self_transfer_multi(
            utxos_to_spend=[parent_other["new_utxo"], parent_low_fee_nonsegwit["new_utxo"]])

        # Relay the parent. It should be rejected because it pays 0 fees.
        self.relay_transaction(peer1, parent_low_fee_nonsegwit["tx"])
        assert parent_low_fee_nonsegwit["txid"] not in node.getrawmempool()

        # Relay the child. It should not be accepted because it has missing inputs.
        # Its parent should not be requested because its hash (txid == wtxid) has been added to the rejection filter.
        self.relay_transaction(peer2, child_nonsegwit["tx"])
        assert child_nonsegwit["txid"] not in node.getrawmempool()
        assert not tx_in_orphanage(node, child_nonsegwit["tx"])

        # No parents are requested.
        self.nodes[0].bumpmocktime(GETDATA_TX_INTERVAL)
        peer1.assert_never_requested(int(parent_other["txid"], 16))
        peer2.assert_never_requested(int(parent_other["txid"], 16))
        peer2.assert_never_requested(int(parent_low_fee_nonsegwit["txid"], 16))

        self.log.info("Test orphan handling when a segwit parent was invalid but may be retried with another witness")
        parent_low_fee = self.wallet.create_self_transfer(fee_rate=0)
        child_low_fee = self.wallet.create_self_transfer(utxo_to_spend=parent_low_fee["new_utxo"])

        # Relay the low fee parent. It should not be accepted.
        self.relay_transaction(peer1, parent_low_fee["tx"])
        assert parent_low_fee["txid"] not in node.getrawmempool()

        # Relay the child. It should not be accepted because it has missing inputs.
        self.relay_transaction(peer2, child_low_fee["tx"])
        assert child_low_fee["txid"] not in node.getrawmempool()
        assert tx_in_orphanage(node, child_low_fee["tx"])

        # The parent should be requested because even though the txid commits to the fee, it doesn't
        # commit to the feerate. Delayed because it's by txid and this is not a preferred relay peer.
        self.nodes[0].bumpmocktime(NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY)
        peer2.wait_for_getdata([parent_low_fee["tx"].txid_int])

        self.log.info("Test orphan handling when a parent was previously downloaded with witness stripped")
        parent_normal = self.wallet.create_self_transfer()
        parent1_witness_stripped = tx_from_hex(parent_normal["tx"].serialize_without_witness().hex())
        child_invalid_witness = self.wallet.create_self_transfer(utxo_to_spend=parent_normal["new_utxo"])

        # Relay the parent with witness stripped. It should not be accepted.
        self.relay_transaction(peer1, parent1_witness_stripped)
        assert_equal(parent_normal["txid"], parent1_witness_stripped.txid_hex)
        assert parent1_witness_stripped.txid_hex not in node.getrawmempool()

        # Relay the child. It should not be accepted because it has missing inputs.
        self.relay_transaction(peer2, child_invalid_witness["tx"])
        assert child_invalid_witness["txid"] not in node.getrawmempool()
        assert tx_in_orphanage(node, child_invalid_witness["tx"])

        # The parent should be requested since the unstripped wtxid would differ. Delayed because
        # it's by txid and this is not a preferred relay peer.
        self.nodes[0].bumpmocktime(NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY)
        peer2.wait_for_getdata([parent_normal["tx"].txid_int])

        # parent_normal can be relayed again even though parent1_witness_stripped was rejected
        self.relay_transaction(peer1, parent_normal["tx"])
        assert_equal(set(node.getrawmempool()), set([parent_normal["txid"], child_invalid_witness["txid"]]))

    @cleanup
    def test_orphan_multiple_parents(self):
        node = self.nodes[0]
        peer = node.add_p2p_connection(PeerTxRelayer())

        self.log.info("Test orphan parent requests with a mixture of confirmed, in-mempool and missing parents")
        # This UTXO confirmed a long time ago.
        utxo_conf_old = self.wallet.send_self_transfer(from_node=node)["new_utxo"]
        txid_conf_old = utxo_conf_old["txid"]
        self.generate(self.wallet, 10)

        # Create a fake reorg to trigger BlockDisconnected, which resets the rolling bloom filter.
        # The alternative is to mine thousands of transactions to push it out of the filter.
        last_block = node.getbestblockhash()
        node.invalidateblock(last_block)
        node.preciousblock(last_block)
        node.syncwithvalidationinterfacequeue()

        # This UTXO confirmed recently.
        utxo_conf_recent = self.wallet.send_self_transfer(from_node=node)["new_utxo"]
        self.generate(node, 1)

        # This UTXO is unconfirmed and in the mempool.
        assert_equal(len(node.getrawmempool()), 0)
        mempool_tx = self.wallet.send_self_transfer(from_node=node)
        utxo_unconf_mempool = mempool_tx["new_utxo"]

        # This UTXO is unconfirmed and missing.
        missing_tx = self.wallet.create_self_transfer()
        utxo_unconf_missing = missing_tx["new_utxo"]
        assert missing_tx["txid"] not in node.getrawmempool()

        orphan = self.wallet.create_self_transfer_multi(utxos_to_spend=[utxo_conf_old,
            utxo_conf_recent, utxo_unconf_mempool, utxo_unconf_missing])

        self.relay_transaction(peer, orphan["tx"])
        self.nodes[0].bumpmocktime(NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY)
        peer.sync_with_ping()
        assert tx_in_orphanage(node, orphan["tx"])
        assert_equal(len(peer.last_message["getdata"].inv), 2)
        peer.wait_for_parent_requests([int(txid_conf_old, 16), int(missing_tx["txid"], 16)])

        # Even though the peer would send a notfound for the "old" confirmed transaction, the node
        # doesn't give up on the orphan. Once all of the missing parents are received, it should be
        # submitted to mempool.
        peer.send_without_ping(msg_notfound(vec=[CInv(MSG_WITNESS_TX, int(txid_conf_old, 16))]))
        # Sync with ping to ensure orphans are reconsidered
        peer.send_and_ping(msg_tx(missing_tx["tx"]))
        assert_equal(node.getmempoolentry(orphan["txid"])["ancestorcount"], 3)

    @cleanup
    def test_orphans_overlapping_parents(self):
        node = self.nodes[0]
        # In the process of relaying inflight_parent_AB
        peer_txrequest = node.add_p2p_connection(PeerTxRelayer())
        # Sends the orphans
        peer_orphans = node.add_p2p_connection(PeerTxRelayer())

        confirmed_utxos = [self.wallet_nonsegwit.get_utxo() for _ in range(4)]
        assert all([utxo["confirmations"] > 0 for utxo in confirmed_utxos])
        self.log.info("Test handling of multiple orphans with missing parents that are already being requested")
        # Parent of child_A only
        missing_parent_A = self.wallet_nonsegwit.create_self_transfer(utxo_to_spend=confirmed_utxos[0])
        # Parents of child_A and child_B
        missing_parent_AB = self.wallet_nonsegwit.create_self_transfer(utxo_to_spend=confirmed_utxos[1])
        inflight_parent_AB = self.wallet_nonsegwit.create_self_transfer(utxo_to_spend=confirmed_utxos[2])
        # Parent of child_B only
        missing_parent_B = self.wallet_nonsegwit.create_self_transfer(utxo_to_spend=confirmed_utxos[3])
        child_A = self.wallet_nonsegwit.create_self_transfer_multi(
            utxos_to_spend=[missing_parent_A["new_utxo"], missing_parent_AB["new_utxo"], inflight_parent_AB["new_utxo"]]
        )
        child_B = self.wallet_nonsegwit.create_self_transfer_multi(
            utxos_to_spend=[missing_parent_B["new_utxo"], missing_parent_AB["new_utxo"], inflight_parent_AB["new_utxo"]]
        )

        # The wtxid and txid need to be the same for the node to recognize that the missing input
        # and in-flight request for inflight_parent_AB are the same transaction.
        assert_equal(inflight_parent_AB["txid"], inflight_parent_AB["wtxid"])

        # Announce inflight_parent_AB and wait for getdata
        peer_txrequest.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=inflight_parent_AB["tx"].wtxid_int)]))
        self.nodes[0].bumpmocktime(NONPREF_PEER_TX_DELAY)
        peer_txrequest.wait_for_getdata([inflight_parent_AB["tx"].wtxid_int])

        self.log.info("Test that the node does not request a parent if it has an in-flight txrequest")
        # Relay orphan child_A
        self.relay_transaction(peer_orphans, child_A["tx"])
        self.nodes[0].bumpmocktime(NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY)
        assert tx_in_orphanage(node, child_A["tx"])
        # There are 3 missing parents. missing_parent_A and missing_parent_AB should be requested.
        # But inflight_parent_AB should not, because there is already an in-flight request for it.
        peer_orphans.wait_for_parent_requests([int(missing_parent_A["txid"], 16), int(missing_parent_AB["txid"], 16)])

        self.log.info("Test that the node does not request a parent if it has an in-flight orphan parent request")
        # Relay orphan child_B
        self.relay_transaction(peer_orphans, child_B["tx"])
        self.nodes[0].bumpmocktime(NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY)
        assert tx_in_orphanage(node, child_B["tx"])
        # Only missing_parent_B should be requested. Not inflight_parent_AB or missing_parent_AB
        # because they are already being requested from peer_txrequest and peer_orphans respectively.
        peer_orphans.wait_for_parent_requests([int(missing_parent_B["txid"], 16)])
        peer_orphans.assert_never_requested(int(inflight_parent_AB["txid"], 16))

        # But inflight_parent_AB will be requested eventually if original peer doesn't respond
        node.bumpmocktime(GETDATA_TX_INTERVAL)
        peer_orphans.wait_for_parent_requests([int(inflight_parent_AB["txid"], 16)])

    @cleanup
    def test_orphan_of_orphan(self):
        node = self.nodes[0]
        peer = node.add_p2p_connection(PeerTxRelayer())

        self.log.info("Test handling of an orphan with a parent who is another orphan")
        missing_grandparent = self.wallet_nonsegwit.create_self_transfer()
        missing_parent_orphan = self.wallet_nonsegwit.create_self_transfer(utxo_to_spend=missing_grandparent["new_utxo"])
        missing_parent = self.wallet_nonsegwit.create_self_transfer()
        orphan = self.wallet_nonsegwit.create_self_transfer_multi(utxos_to_spend=[missing_parent["new_utxo"], missing_parent_orphan["new_utxo"]])

        # The node should put missing_parent_orphan into the orphanage and request missing_grandparent
        self.relay_transaction(peer, missing_parent_orphan["tx"])
        self.nodes[0].bumpmocktime(NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY)
        assert tx_in_orphanage(node, missing_parent_orphan["tx"])
        peer.wait_for_parent_requests([int(missing_grandparent["txid"], 16)])

        # The node should put the orphan into the orphanage and request missing_parent, skipping
        # missing_parent_orphan because it already has it in the orphanage.
        self.relay_transaction(peer, orphan["tx"])
        self.nodes[0].bumpmocktime(NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY)
        assert tx_in_orphanage(node, orphan["tx"])
        peer.wait_for_parent_requests([int(missing_parent["txid"], 16)])

    @cleanup
    def test_orphan_inherit_rejection(self):
        node = self.nodes[0]
        peer1 = node.add_p2p_connection(PeerTxRelayer())
        peer2 = node.add_p2p_connection(PeerTxRelayer())
        peer3 = node.add_p2p_connection(PeerTxRelayer(wtxidrelay=False))

        self.log.info("Test that an orphan with rejected parents, along with any descendants, cannot be retried with an alternate witness")
        parent_low_fee_nonsegwit = self.wallet_nonsegwit.create_self_transfer(fee_rate=0)
        assert_equal(parent_low_fee_nonsegwit["txid"], parent_low_fee_nonsegwit["tx"].wtxid_hex)
        child = self.wallet.create_self_transfer(utxo_to_spend=parent_low_fee_nonsegwit["new_utxo"])
        grandchild = self.wallet.create_self_transfer(utxo_to_spend=child["new_utxo"])
        assert_not_equal(child["txid"], child["tx"].wtxid_hex)
        assert_not_equal(grandchild["txid"], grandchild["tx"].wtxid_hex)

        # Relay the parent. It should be rejected because it pays 0 fees.
        self.relay_transaction(peer1, parent_low_fee_nonsegwit["tx"])
        assert parent_low_fee_nonsegwit["txid"] not in node.getrawmempool()

        # Relay the child. It should be rejected for having missing parents, and this rejection is
        # cached by txid and wtxid.
        self.relay_transaction(peer1, child["tx"])
        assert_equal(0, len(node.getrawmempool()))
        assert not tx_in_orphanage(node, child["tx"])
        peer1.assert_never_requested(parent_low_fee_nonsegwit["txid"])

        # Grandchild should also not be kept in orphanage because its parent has been rejected.
        self.relay_transaction(peer2, grandchild["tx"])
        assert_equal(0, len(node.getrawmempool()))
        assert not tx_in_orphanage(node, grandchild["tx"])
        peer2.assert_never_requested(child["txid"])
        peer2.assert_never_requested(child["tx"].wtxid_hex)

        # The child should never be requested, even if announced again with potentially different witness.
        # Sync with ping to ensure orphans are reconsidered
        peer3.send_and_ping(msg_inv([CInv(t=MSG_TX, h=int(child["txid"], 16))]))
        self.nodes[0].bumpmocktime(TXREQUEST_TIME_SKIP)
        peer3.assert_never_requested(child["txid"])

    @cleanup
    def test_same_txid_orphan(self):
        self.log.info("Check what happens when orphan with same txid is already in orphanage")
        node = self.nodes[0]

        tx_parent = self.wallet.create_self_transfer()

        # Create the real child
        tx_child = self.wallet.create_self_transfer(utxo_to_spend=tx_parent["new_utxo"])

        # Create a fake version of the child
        tx_orphan_bad_wit = self.create_malleated_version(tx_child)

        bad_peer = node.add_p2p_connection(P2PInterface())
        honest_peer = node.add_p2p_connection(P2PInterface())

        # 1. Fake orphan is received first. It is missing an input.
        bad_peer.send_and_ping(msg_tx(tx_orphan_bad_wit))
        assert tx_in_orphanage(node, tx_orphan_bad_wit)

        # 2. Node requests the missing parent by txid.
        parent_txid_int = int(tx_parent["txid"], 16)
        node.bumpmocktime(NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY)
        bad_peer.wait_for_getdata([parent_txid_int])

        # 3. Honest peer relays the real child, which is also missing parents and should be placed
        # in the orphanage.
        with node.assert_debug_log(["missingorspent"]):
            honest_peer.send_and_ping(msg_tx(tx_child["tx"]))
        assert tx_in_orphanage(node, tx_child["tx"])

        # Time out the previous request for the parent (node will not request the same transaction
        # from multiple nodes at the same time)
        node.bumpmocktime(GETDATA_TX_INTERVAL)

        # 4. The parent is requested. Honest peer sends it.
        honest_peer.wait_for_getdata([parent_txid_int])
        # Sync with ping to ensure orphans are reconsidered
        honest_peer.send_and_ping(msg_tx(tx_parent["tx"]))

        # 5. After parent is accepted, orphans should be reconsidered.
        # The real child should be accepted and the fake one rejected.
        node_mempool = node.getrawmempool()
        assert tx_parent["txid"] in node_mempool
        assert tx_child["txid"] in node_mempool
        assert_equal(node.getmempoolentry(tx_child["txid"])["wtxid"], tx_child["wtxid"])

    @cleanup
    def test_same_txid_orphan_of_orphan(self):
        self.log.info("Check what happens when orphan's parent with same txid is already in orphanage")
        node = self.nodes[0]

        tx_grandparent = self.wallet.create_self_transfer()

        # Create middle tx (both parent and child) which will be in orphanage.
        tx_middle = self.wallet.create_self_transfer(utxo_to_spend=tx_grandparent["new_utxo"])

        # Create a fake version of the middle tx
        tx_orphan_bad_wit = self.create_malleated_version(tx_middle)

        # Create grandchild spending from tx_middle (and spending from tx_orphan_bad_wit since they
        # have the same txid).
        tx_grandchild = self.wallet.create_self_transfer(utxo_to_spend=tx_middle["new_utxo"])

        bad_peer = node.add_p2p_connection(P2PInterface())
        honest_peer = node.add_p2p_connection(P2PInterface())

        # 1. Fake orphan is received first. It is missing an input.
        bad_peer.send_and_ping(msg_tx(tx_orphan_bad_wit))
        assert tx_in_orphanage(node, tx_orphan_bad_wit)

        # 2. Node requests missing tx_grandparent by txid.
        grandparent_txid_int = int(tx_grandparent["txid"], 16)
        node.bumpmocktime(NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY)
        bad_peer.wait_for_getdata([grandparent_txid_int])

        # 3. Honest peer relays the grandchild, which is missing a parent. The parent by txid already
        # exists in orphanage, but should be re-requested because the node shouldn't assume that the
        # witness data is the same. In this case, a same-txid-different-witness transaction exists!
        honest_peer.send_and_ping(msg_tx(tx_grandchild["tx"]))
        assert tx_in_orphanage(node, tx_grandchild["tx"])
        middle_txid_int = int(tx_middle["txid"], 16)
        node.bumpmocktime(NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY)
        honest_peer.wait_for_getdata([middle_txid_int])

        # 4. Honest peer relays the real child, which is also missing parents and should be placed
        # in the orphanage.
        honest_peer.send_and_ping(msg_tx(tx_middle["tx"]))
        assert tx_in_orphanage(node, tx_middle["tx"])
        assert_equal(len(node.getrawmempool()), 0)

        # 5. Honest peer sends tx_grandparent
        honest_peer.send_and_ping(msg_tx(tx_grandparent["tx"]))

        # 6. After parent is accepted, orphans should be reconsidered.
        # The real child should be accepted and the fake one rejected.
        node_mempool = node.getrawmempool()
        assert tx_grandparent["txid"] in node_mempool
        assert tx_middle["txid"] in node_mempool
        assert tx_grandchild["txid"] in node_mempool
        assert_equal(node.getmempoolentry(tx_middle["txid"])["wtxid"], tx_middle["wtxid"])
        self.wait_until(lambda: len(node.getorphantxs()) == 0)

    @cleanup
    def test_orphan_txid_inv(self):
        self.log.info("Check node does not ignore announcement with same txid as tx in orphanage")
        node = self.nodes[0]

        tx_parent = self.wallet.create_self_transfer()

        # Create the real child and fake version
        tx_child = self.wallet.create_self_transfer(utxo_to_spend=tx_parent["new_utxo"])
        tx_orphan_bad_wit = self.create_malleated_version(tx_child)

        bad_peer = node.add_p2p_connection(PeerTxRelayer())
        # Must not send wtxidrelay because otherwise the inv(TX) will be ignored later
        honest_peer = node.add_p2p_connection(P2PInterface(wtxidrelay=False))

        # 1. Fake orphan is received first. It is missing an input.
        bad_peer.send_and_ping(msg_tx(tx_orphan_bad_wit))
        assert tx_in_orphanage(node, tx_orphan_bad_wit)

        # 2. Node requests the missing parent by txid.
        parent_txid_int = int(tx_parent["txid"], 16)
        node.bumpmocktime(NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY)
        bad_peer.wait_for_getdata([parent_txid_int])

        # 3. Honest peer announces the real child, by txid (this isn't common but the node should
        # still keep track of it).
        child_txid_int = int(tx_child["txid"], 16)
        honest_peer.send_and_ping(msg_inv([CInv(t=MSG_TX, h=child_txid_int)]))

        # 4. The child is requested. Honest peer sends it.
        node.bumpmocktime(TXREQUEST_TIME_SKIP)
        honest_peer.wait_for_getdata([child_txid_int])
        honest_peer.send_and_ping(msg_tx(tx_child["tx"]))
        assert tx_in_orphanage(node, tx_child["tx"])

        # 5. After first parent request times out, the node sends another one for the missing parent
        # of the real orphan child.
        node.bumpmocktime(GETDATA_TX_INTERVAL)
        honest_peer.wait_for_getdata([parent_txid_int])
        honest_peer.send_and_ping(msg_tx(tx_parent["tx"]))

        # 6. After parent is accepted, orphans should be reconsidered.
        # The real child should be accepted and the fake one rejected. This may happen in either
        # order since the message-processing is randomized. If tx_orphan_bad_wit is validated first,
        # its consensus error leads to disconnection of bad_peer. If tx_child is validated first,
        # tx_orphan_bad_wit is rejected for txn-same-nonwitness-data-in-mempool (no punishment).
        node_mempool = node.getrawmempool()
        assert tx_parent["txid"] in node_mempool
        assert tx_child["txid"] in node_mempool
        assert_equal(node.getmempoolentry(tx_child["txid"])["wtxid"], tx_child["wtxid"])
        self.wait_until(lambda: len(node.getorphantxs()) == 0)

    @cleanup
    def test_max_orphan_amount(self):
        self.log.info("Check that we never exceed our storage limits for orphans")

        node = self.nodes[0]
        self.generate(self.wallet, 1)
        peer_1 = node.add_p2p_connection(P2PInterface())

        self.log.info("Check that orphanage is empty on start of test")
        assert len(node.getorphantxs()) == 0

        self.log.info("Filling up orphanage with " + str(DEFAULT_MAX_ORPHAN_TRANSACTIONS) + "(DEFAULT_MAX_ORPHAN_TRANSACTIONS) orphans")
        orphans = []
        parent_orphans = []
        for _ in range(DEFAULT_MAX_ORPHAN_TRANSACTIONS):
            tx_parent_1 = self.wallet.create_self_transfer()
            tx_child_1 = self.wallet.create_self_transfer(utxo_to_spend=tx_parent_1["new_utxo"])
            parent_orphans.append(tx_parent_1["tx"])
            orphans.append(tx_child_1["tx"])
            peer_1.send_without_ping(msg_tx(tx_child_1["tx"]))

        peer_1.sync_with_ping()
        orphanage = node.getorphantxs()
        self.wait_until(lambda: len(node.getorphantxs()) == DEFAULT_MAX_ORPHAN_TRANSACTIONS)

        for orphan in orphans:
            assert tx_in_orphanage(node, orphan)

        self.log.info("Check that we do not add more than the max orphan amount")
        tx_parent_1 = self.wallet.create_self_transfer()
        tx_child_1 = self.wallet.create_self_transfer(utxo_to_spend=tx_parent_1["new_utxo"])
        peer_1.send_and_ping(msg_tx(tx_child_1["tx"]))
        parent_orphans.append(tx_parent_1["tx"])
        orphanage = node.getorphantxs()
        assert_equal(len(orphanage), DEFAULT_MAX_ORPHAN_TRANSACTIONS)

        self.log.info("Clearing the orphanage")
        for index, parent_orphan in enumerate(parent_orphans):
            peer_1.send_and_ping(msg_tx(parent_orphan))
        self.wait_until(lambda: len(node.getorphantxs()) == 0)

    @cleanup
    def test_orphan_handling_prefer_outbound(self):
        self.log.info("Test that the node prefers requesting from outbound peers")
        node = self.nodes[0]
        orphan_wtxid, orphan_tx, parent_tx = self.create_parent_and_child()
        orphan_inv = CInv(t=MSG_WTX, h=int(orphan_wtxid, 16))

        peer_inbound = node.add_p2p_connection(PeerTxRelayer())
        peer_outbound = node.add_outbound_p2p_connection(PeerTxRelayer(), p2p_idx=1)

        # Inbound peer relays the transaction.
        peer_inbound.send_and_ping(msg_inv([orphan_inv]))
        self.nodes[0].bumpmocktime(TXREQUEST_TIME_SKIP)
        peer_inbound.wait_for_getdata([int(orphan_wtxid, 16)])

        # Both peers send invs for the orphan, so the node can expect both to know its ancestors.
        peer_outbound.send_and_ping(msg_inv([orphan_inv]))

        peer_inbound.send_and_ping(msg_tx(orphan_tx))

        # There should be 1 orphan with 2 announcers (we don't know what their peer IDs are)
        orphanage = node.getorphantxs(verbosity=2)
        assert_equal(orphanage[0]["wtxid"], orphan_wtxid)
        assert_equal(len(orphanage[0]["from"]), 2)

        # The outbound peer should be preferred for getting orphan parents
        self.nodes[0].bumpmocktime(TXID_RELAY_DELAY)
        peer_outbound.wait_for_parent_requests([parent_tx.txid_int])

        # There should be no request to the inbound peer
        peer_inbound.assert_never_requested(parent_tx.txid_int)

        self.log.info("Test that, if the preferred peer doesn't respond, the node sends another request")
        self.nodes[0].bumpmocktime(GETDATA_TX_INTERVAL)
        peer_inbound.sync_with_ping()
        peer_inbound.wait_for_parent_requests([parent_tx.txid_int])

    @cleanup
    def test_announcers_before_and_after(self):
        self.log.info("Test that the node uses all peers who announced the tx prior to realizing it's an orphan")
        node = self.nodes[0]
        orphan_wtxid, orphan_tx, parent_tx = self.create_parent_and_child()
        orphan_inv = CInv(t=MSG_WTX, h=int(orphan_wtxid, 16))

        # Announces before tx is sent, disconnects while node is requesting parents
        peer_early_disconnected = node.add_outbound_p2p_connection(PeerTxRelayer(), p2p_idx=3)
        # Announces before tx is sent, doesn't respond to parent request
        peer_early_unresponsive = node.add_p2p_connection(PeerTxRelayer())

        # Announces after tx is sent
        peer_late_announcer = node.add_p2p_connection(PeerTxRelayer())

        # Both peers send invs for the orphan, so the node can expect both to know its ancestors.
        peer_early_disconnected.send_and_ping(msg_inv([orphan_inv]))
        self.nodes[0].bumpmocktime(TXREQUEST_TIME_SKIP)
        peer_early_disconnected.wait_for_getdata([int(orphan_wtxid, 16)])
        peer_early_unresponsive.send_and_ping(msg_inv([orphan_inv]))
        peer_early_disconnected.send_and_ping(msg_tx(orphan_tx))

        # There should be 1 orphan with 2 announcers (we don't know what their peer IDs are)
        orphanage = node.getorphantxs(verbosity=2)
        assert_equal(len(orphanage), 1)
        assert_equal(orphanage[0]["wtxid"], orphan_wtxid)
        assert_equal(len(orphanage[0]["from"]), 2)

        # Peer disconnects before responding to request
        self.nodes[0].bumpmocktime(TXID_RELAY_DELAY)
        peer_early_disconnected.wait_for_parent_requests([parent_tx.txid_int])
        peer_early_disconnected.peer_disconnect()

        # The orphan should have 1 announcer left after the node finishes disconnecting peer_early_disconnected.
        self.wait_until(lambda: len(node.getorphantxs(verbosity=2)[0]["from"]) == 1)

        # The node should retry with the other peer that announced the orphan earlier.
        # This node's request was additionally delayed because it's an inbound peer.
        self.nodes[0].bumpmocktime(NONPREF_PEER_TX_DELAY)
        peer_early_unresponsive.wait_for_parent_requests([parent_tx.txid_int])

        self.log.info("Test that the node uses peers who announce the tx after realizing it's an orphan")
        peer_late_announcer.send_and_ping(msg_inv([orphan_inv]))

        # The orphan should have 2 announcers now
        orphanage = node.getorphantxs(verbosity=2)
        assert_equal(orphanage[0]["wtxid"], orphan_wtxid)
        assert_equal(len(orphanage[0]["from"]), 2)

        self.nodes[0].bumpmocktime(GETDATA_TX_INTERVAL)
        peer_late_announcer.wait_for_parent_requests([parent_tx.txid_int])

    @cleanup
    def test_parents_change(self):
        self.log.info("Test that, if a parent goes missing during orphan reso, it is requested")
        node = self.nodes[0]
        # Orphan will have 2 parents, 1 missing and 1 already in mempool when received.
        # Create missing parent.
        parent_missing = self.wallet.create_self_transfer()

        # Create parent that will already be in mempool, but become missing during orphan resolution.
        # Get 3 UTXOs for replacement-cycled parent, UTXOS A, B, C
        coin_A = self.wallet.get_utxo(confirmed_only=True)
        coin_B = self.wallet.get_utxo(confirmed_only=True)
        coin_C = self.wallet.get_utxo(confirmed_only=True)
        # parent_peekaboo_AB spends A and B. It is replaced by tx_replacer_BC (conflicting UTXO B),
        # and then replaced by tx_replacer_C (conflicting UTXO C). This replacement cycle is used to
        # ensure that parent_peekaboo_AB can be reintroduced without requiring package RBF.
        FEE_INCREMENT = 2400
        parent_peekaboo_AB = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[coin_A, coin_B],
            num_outputs=1,
            fee_per_output=FEE_INCREMENT
        )
        tx_replacer_BC = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[coin_B, coin_C],
            num_outputs=1,
            fee_per_output=2*FEE_INCREMENT
        )
        tx_replacer_C = self.wallet.create_self_transfer(
            utxo_to_spend=coin_C,
            fee_per_output=3*FEE_INCREMENT
        )

        # parent_peekaboo_AB starts out in the mempool
        node.sendrawtransaction(parent_peekaboo_AB["hex"])

        orphan = self.wallet.create_self_transfer_multi(utxos_to_spend=[parent_peekaboo_AB["new_utxos"][0], parent_missing["new_utxo"]])
        orphan_wtxid = orphan["wtxid"]
        orphan_inv = CInv(t=MSG_WTX, h=int(orphan_wtxid, 16))

        # peer1 sends the orphan and gets a request for the missing parent
        peer1 = node.add_p2p_connection(PeerTxRelayer())
        peer1.send_and_ping(msg_inv([orphan_inv]))
        node.bumpmocktime(TXREQUEST_TIME_SKIP)
        peer1.wait_for_getdata([int(orphan_wtxid, 16)])
        peer1.send_and_ping(msg_tx(orphan["tx"]))
        self.wait_until(lambda: node.getorphantxs(verbosity=0) == [orphan["txid"]])
        node.bumpmocktime(NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY)
        peer1.wait_for_getdata([int(parent_missing["txid"], 16)])

        # Replace parent_peekaboo_AB so that is a newly missing parent.
        # Then, replace the replacement so that it can be resubmitted.
        node.sendrawtransaction(tx_replacer_BC["hex"])
        assert tx_replacer_BC["txid"] in node.getrawmempool()
        node.sendrawtransaction(tx_replacer_C["hex"])
        assert tx_replacer_BC["txid"] not in node.getrawmempool()
        assert parent_peekaboo_AB["txid"] not in node.getrawmempool()
        assert tx_replacer_C["txid"] in node.getrawmempool()

        # Second peer is an additional announcer for this orphan, but its missing parents are different from when it was
        # previously announced.
        peer2 = node.add_p2p_connection(PeerTxRelayer())
        peer2.send_and_ping(msg_inv([orphan_inv]))
        assert_equal(len(node.getorphantxs(verbosity=2)[0]["from"]), 2)

        # Disconnect peer1. peer2 should become the new candidate for orphan resolution.
        peer1.peer_disconnect()
        self.wait_until(lambda: node.num_test_p2p_connections() == 1)
        peer2.sync_with_ping()  # Sync with the 'net' thread which completes the disconnection fully
        node.bumpmocktime(TXREQUEST_TIME_SKIP)
        self.wait_until(lambda: len(node.getorphantxs(verbosity=2)[0]["from"]) == 1)
        # Both parents should be requested, now that they are both missing.
        peer2.wait_for_parent_requests([int(parent_peekaboo_AB["txid"], 16), int(parent_missing["txid"], 16)])
        peer2.send_and_ping(msg_tx(parent_missing["tx"]))
        peer2.send_and_ping(msg_tx(parent_peekaboo_AB["tx"]))

        final_mempool = node.getrawmempool()
        assert parent_missing["txid"] in final_mempool
        assert parent_peekaboo_AB["txid"] in final_mempool
        assert orphan["txid"] in final_mempool
        assert tx_replacer_C["txid"] in final_mempool

    def run_test(self):
        self.nodes[0].setmocktime(int(time.time()))
        self.wallet_nonsegwit = MiniWallet(self.nodes[0], mode=MiniWalletMode.RAW_P2PK)
        self.generate(self.wallet_nonsegwit, 10)
        self.wallet = MiniWallet(self.nodes[0])
        self.generate(self.wallet, 160)

        self.test_arrival_timing_orphan()
        self.test_orphan_rejected_parents_exceptions()
        self.test_orphan_multiple_parents()
        self.test_orphans_overlapping_parents()
        self.test_orphan_of_orphan()
        self.test_orphan_inherit_rejection()
        self.test_same_txid_orphan()
        self.test_same_txid_orphan_of_orphan()
        self.test_orphan_txid_inv()
        self.test_max_orphan_amount()
        self.test_orphan_handling_prefer_outbound()
        self.test_announcers_before_and_after()
        self.test_parents_change()


if __name__ == '__main__':
    OrphanHandlingTest(__file__).main()
