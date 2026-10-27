#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test -privatebroadcast: transactions submitted with sendrawtransaction run as bounded
bitcoin-privbcast jobs inside the node.

The node reaches the network only through its Tor SOCKS5 proxy; here that is the framework's
SOCKS5 server, which answers RESOLVE for the test seed name from a fixed script and redirects
CONNECT requests to Python recipients or to a second bitcoind. The wire behaviour is covered by
tool_privbcast.py; here the first job's recipients, schedule and summary are only checked with the
same functions (U3: a node job is a tool job). This test covers the node side: queueing,
concurrency, the RPCs, abort, the report, and that the node's own mempool only learns the
transaction from the network.

The clock is mocked on both nodes: setmocktime moves a job's schedule (a job runs on the node's
clock) together with the recipient's own timers (its request delays, its INV trickle). A job is
driven to its delivery start, where its prompt slots announce over real sockets in real time, and
then past its last opportunity; whatever the mid and late slots have not reached by then is missed.
While it waits on the network, the test ticks the clock a second per poll, some twenty seconds a
second, so a prompt attempt has a few real seconds to get through the proxy, BIP324 and VERSION
before its 45 s handshake budget runs out.
"""
from decimal import Decimal
import threading
import time

from test_framework.address import address_to_scriptpubkey
from test_framework.messages import (
    CTxOut,
    msg_tx,
    tx_from_hex,
)
from test_framework.p2p import (
    P2PInterface,
    P2P_SERVICES,
    start_p2p_listener,
)
from test_framework.privbcast import (
    DISCOVERY_WINDOW_S,
    REGTEST_PORT,
    Recipient,
    check_profile,
    check_schedule,
    check_summary,
    drain_socks_commands,
    query_sets,
)
from test_framework.script_util import build_malleated_tx_package
from test_framework.socks5 import start_socks5_server
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    p2p_port,
)
from test_framework.v2_p2p import EncryptedP2PState
from test_framework.wallet import MiniWallet

# The Parameters of doc/design/private-broadcast-tool.md that this test needs besides those in
# test_framework/privbcast.py.
PROMPT_SLOTS = 3  # the slots whose first opportunity opens at delivery start
SCHEDULED_BOUND_S = 562  # a job has ended by this long after it started: its last slot has ended
# A queued job starts this long after the previous start, drawn.
START_SPACING_MIN_S = 35
START_SPACING_MAX_S = 55
MAX_FINISHED_JOBS = 100


class P2PPrivateBroadcast(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.uses_wallet = None  # the wallet section runs when the wallet is compiled

    def add_options(self, parser):
        parser.add_argument("--package", action="store_true", help="test one-parent-one-child package mode (the extension) only")

    def setup_nodes(self):
        self.listeners = {}
        self.lock = threading.Lock()
        self.exit_path = [f"11.22.33.{i}" for i in range(1, 13)]  # routable, as discovery requires
        # a.seed. answers the bitcoind recipient's endpoints unless a section says otherwise; the other
        # six endpoints are Python recipients. b.seed. always answers three more Python recipients.
        # Seeds take turns in a job's slots (R5b), so every job's prompt slots reach both kinds.
        node_endpoints = set(self.exit_path[:6])
        self.python_endpoints = [f"11.22.33.{i}" for i in range(13, 16)]

        self.resolve_count = 0
        self.python_resolve_count = 0
        self.exit_path_active = self.exit_path[:6]  # a test section may change the answers

        def resolve_factory(name):
            # One answer per query, cycling through the seed's endpoints.
            with self.lock:
                if name == "a.seed.":
                    answer = self.exit_path_active[self.resolve_count % len(self.exit_path_active)]
                    self.resolve_count += 1
                elif name == "b.seed.":
                    answer = self.python_endpoints[self.python_resolve_count % len(self.python_endpoints)]
                    self.python_resolve_count += 1
                else:
                    answer = None
            return answer

        def destinations_factory(requested_to_addr, requested_to_port, proxy_client):
            with self.lock:
                if requested_to_addr in node_endpoints:
                    return {"actual_to_addr": "127.0.0.1", "actual_to_port": p2p_port(1)}
                # A fresh listener per connection: the framework's listeners accept once, and later
                # jobs dial the same endpoints again.
                listener = Recipient()
                listener.peer_connect_helper(dstaddr="0.0.0.0", dstport=0, net=self.chain, timeout_factor=self.options.timeout_factor)
                listener.peer_connect_send_version(services=P2P_SERVICES)
                listener.v2_state = EncryptedP2PState(initiating=False, net=self.chain)
                addr, port = start_p2p_listener(self.network_thread, listener)
                self.listeners.setdefault(requested_to_addr, []).append(listener)
                return {"actual_to_addr": addr, "actual_to_port": port}

        self.socks5_server = start_socks5_server(destinations_factory, resolve_factory)
        self.extra_args = [
            [
                "-privatebroadcast",
                f"-onion=127.0.0.1:{self.socks5_server.conf.addr[1]}",
                "-privatebroadcastseed=a.seed.",
                "-privatebroadcastseed=b.seed.",
                "-v2transport=1",
                "-proxyrandomize=0",  # private broadcast authenticates every stream regardless (U2)
                "-debug=privatebroadcast",
            ],
            ["-v2transport=1"],
        ]
        super().setup_nodes()

    def check_wire(self, wtxid):
        """Every Python recipient so far saw the profile tool_privbcast.py checks for the tool."""
        with self.lock:
            listeners = [listener for ls in self.listeners.values() for listener in ls if "version" in listener.last_message]
        assert listeners
        for listener in listeners:
            check_profile(listener, wtxid)

    @staticmethod
    def witness_variant(tx):
        """The same transaction with another witness: the same txid, a witness the node never validated."""
        other = tx_from_hex(tx["hex"])
        other.wit.vtxinwit[0].scriptWitness.stack.insert(0, b"\x01")
        assert_equal(other.txid_hex, tx["txid"])
        return other

    def node_state(self):
        """What a job must leave alone: the node's peers, bans and address manager."""
        return sorted(p["addr"] for p in self.nodes[0].getpeerinfo()), self.nodes[0].listbanned(), self.nodes[0].getaddrmaninfo()

    def entries(self):
        """Every job the node lists: retained finished ones first, then running, then queued."""
        return self.nodes[0].getprivatebroadcastinfo()["jobs"]

    def jobs(self):
        """The latest job of each transaction, by wtxid."""
        return {j["wtxid"]: j for j in self.entries()}

    def jobs_after_submit(self, hex_tx):
        before = set(self.jobs())
        self.nodes[0].sendrawtransaction(hex_tx)
        new = set(self.jobs()) - before
        assert_equal(len(new), 1)
        return new.pop()

    def wait_for_state(self, wtxid, state, timeout=60):
        self.wait_until(lambda: self.jobs()[wtxid]["state"] == state, timeout=timeout)
        return self.jobs()[wtxid]

    def advance(self, seconds):
        """Move both nodes' clocks forward together."""
        self.mocktime += seconds
        for node in self.nodes:
            node.setmocktime(self.mocktime)

    def progress(self, wtxid):
        return self.jobs()[wtxid]["progress"]

    def start_delivery(self, batch):
        """Take the running jobs (they started together, on the frozen clock) to their delivery start
        once their discovery is over: the prompt slots then dial in real time."""
        # A submission only queues the job, which starts a moment later.
        self.wait_until(lambda: sorted(batch) == sorted(i for i, j in self.jobs().items() if j["state"] == "running"))
        jobs = self.jobs()
        starts = {jobs[i]["time_started"] for i in batch}
        assert_equal(len(starts), 1)
        t0 = starts.pop()
        assert_greater_than_or_equal(t0 + DISCOVERY_WINDOW_S, self.mocktime)
        self.wait_until(lambda: all(self.progress(i)["discovery_done"] for i in batch))
        self.advance(t0 + DISCOVERY_WINDOW_S - self.mocktime)

    def tick_until(self, condition):
        """Let the clock tick a second at a time until `condition`: the recipient's own delays (its
        request timers) then pass while a job's next opportunity stays far ahead."""
        def step():
            if condition():
                return True
            self.advance(1)
            return False
        self.wait_until(step)

    def wait_for_prompt_slots(self, batch):
        """Tick until every prompt slot's first opportunity has ended: dialled and over, missed, or
        without a candidate. No later opportunity is due for another 35 s of the schedule."""
        def over():
            jobs = self.jobs()
            return all(jobs[i]["state"] != "running" or jobs[i]["progress"]["opportunities_ended"] >= PROMPT_SLOTS for i in batch)
        self.tick_until(over)

    def end_jobs(self, batch):
        """Jump past the last opportunity of these jobs and wait for them to end."""
        t0 = max(self.jobs()[i]["time_started"] for i in batch)
        self.advance(max(t0 + SCHEDULED_BOUND_S - self.mocktime, 0))
        return [self.wait_for_state(i, "done") for i in batch]

    def open_gate(self, ids=()):
        """Move the clock to where the next queued job may start, whatever spacing was drawn, in steps
        no longer than a job's discovery window, stopping once one of `ids` runs: a job whose worker
        starts it during a step, however late, still finds its delivery start ahead of the clock."""
        last = max((j["time_started"] for j in self.entries() if "time_started" in j), default=0)
        target = last + START_SPACING_MAX_S
        while self.mocktime < target and not any(self.jobs()[i]["state"] == "running" for i in ids):
            self.advance(min(DISCOVERY_WINDOW_S, target - self.mocktime))

    def finish(self, ids):
        """Run these queued or running jobs to their end, batch by batch."""
        ids = list(ids)
        while ids:
            if not any(self.jobs()[i]["state"] == "running" for i in ids):
                self.open_gate(ids)
            self.wait_until(lambda: any(self.jobs()[i]["state"] == "running" for i in ids))
            batch = [i for i in ids if self.jobs()[i]["state"] == "running"]
            self.start_delivery(batch)
            self.wait_for_prompt_slots(batch)
            self.end_jobs(batch)
            ids = [i for i in ids if i not in batch]

    def test_package(self):
        self.log.info("submitpackage with a low-fee parent and its child queues one job that serves the parent on request")
        # The recipient asks for a missing parent after its orphan-resolution delays: 2 s for a non-preferred
        # announcer plus 2 s because it has wtxid-relay peers (the private broadcast connection is one), well
        # inside the 30 s parent hold. Take the ordinary node0-node1 link away for this part. And let node1
        # be reached through one endpoint only: with several connections from the same job, node1 may ask
        # a connection that has not served the child for the parent, which the protocol does not answer
        # (one parent, one child, on one connection).
        self.disconnect_nodes(0, 1)
        with self.lock:
            self.exit_path_active = [self.exit_path[0]]
        parent = self.wallet.create_self_transfer(fee_rate=Decimal("0"))
        child = self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"])
        res = self.nodes[0].submitpackage([parent["hex"], child["hex"]])
        assert_equal(res["package_msg"], "success")
        assert_equal(sorted(res["tx-results"][child["wtxid"]]["fees"]["effective-includes"]), sorted([parent["wtxid"], child["wtxid"]]))
        job_id = child["wtxid"]
        assert_equal(self.jobs()[job_id]["parent_txid"], parent["txid"])
        # The same package submitted while its job is queued or running is covered by that job.
        before = len(self.entries())
        assert_equal(self.nodes[0].submitpackage([parent["hex"], child["hex"]])["package_msg"], "success")
        assert_equal(len(self.entries()), before)
        # Abort matches a package job by its child only, never by the parent, which several jobs can share.
        for parent_id in (parent["txid"], parent["wtxid"]):
            assert_raises_rpc_error(-5, None, self.nodes[0].abortprivatebroadcast, parent_id)
        assert self.jobs()[job_id]["state"] in ("queued", "running")
        assert parent["txid"] not in self.nodes[0].getrawmempool()
        self.start_delivery([job_id])
        self.tick_until(lambda: child["txid"] in self.nodes[1].getrawmempool() and parent["txid"] in self.nodes[1].getrawmempool())
        job = self.end_jobs([job_id])[0]
        assert_equal(job["report"]["summary"]["parents_served"], 1)  # node1's one endpoint, dialled once (R4)
        assert child["txid"] not in self.nodes[0].getrawmempool()

        self.log.info("A parent already in the mempool counts as accepted, and the job serves it as given")
        parent = self.wallet.create_self_transfer()
        child = self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"])
        self.nodes[0].add_p2p_connection(P2PInterface()).send_and_ping(msg_tx(parent["tx"]))
        assert parent["txid"] in self.nodes[0].getrawmempool()
        assert parent["txid"] not in self.nodes[1].getrawmempool()
        res = self.nodes[0].submitpackage([parent["hex"], child["hex"]])
        assert_equal(res["package_msg"], "success")
        assert "error" not in res["tx-results"][parent["wtxid"]]
        assert "fees" in res["tx-results"][child["wtxid"]]
        assert_equal(self.jobs()[child["wtxid"]]["parent_txid"], parent["txid"])
        self.start_delivery([child["wtxid"]])
        self.tick_until(lambda: child["txid"] in self.nodes[1].getrawmempool() and parent["txid"] in self.nodes[1].getrawmempool())
        job = self.end_jobs([child["wtxid"]])[0]
        assert_equal(job["report"]["summary"]["parents_served"], 1)
        self.nodes[0].disconnect_p2ps()
        # node1 does not announce transactions it already had when a peer connects, so node0 is not
        # expected to learn these two; receipt-back is covered above.
        self.connect_nodes(0, 1)

        self.log.info("submitpackage sends a copy whose txid is in the mempool with another witness as given")
        tx = self.wallet.create_self_transfer()
        self.nodes[0].add_p2p_connection(P2PInterface()).send_and_ping(msg_tx(tx["tx"]))
        other = self.witness_variant(tx)
        res = self.nodes[0].submitpackage([other.serialize().hex()])
        assert_equal(res["tx-results"][other.wtxid_hex]["other-wtxid"], tx["wtxid"])
        assert_equal(self.jobs()[other.wtxid_hex]["txid"], tx["txid"])
        self.nodes[0].abortprivatebroadcast(other.wtxid_hex)
        self.nodes[0].disconnect_p2ps()

        self.log.info("With a parent that fails alone, maxfeerate still holds for the child")
        parent = self.wallet.create_self_transfer(fee_rate=Decimal("0"))
        child = self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"], fee_rate=Decimal("0.01"))
        before = len(self.entries())
        res = self.nodes[0].submitpackage([parent["hex"], child["hex"]], maxfeerate=Decimal("0.005"))
        assert res["tx-results"][child["wtxid"]]["error"]
        assert_equal(len(self.entries()), before)

        self.log.info("With a parent that fails alone, a child with an input the node cannot find is refused")
        parent = self.wallet.create_self_transfer(fee_rate=Decimal("0"))
        unknown = {"txid": "ee" * 32, "vout": 0, "value": Decimal("0.0001")}
        child = self.wallet.create_self_transfer_multi(utxos_to_spend=[parent["new_utxo"], unknown])
        res = self.nodes[0].submitpackage([parent["hex"], child["hex"]])
        assert res["tx-results"][child["wtxid"]]["error"]
        assert_equal(len(self.entries()), before)
        with self.lock:
            self.exit_path_active = self.exit_path[:6]

        self.log.info("A one-transaction package may replace a mempool transaction, as with sendrawtransaction")
        coin = self.wallet.get_utxo()
        original = self.wallet.create_self_transfer(utxo_to_spend=coin)
        replacement = self.wallet.create_self_transfer(utxo_to_spend=coin, fee_rate=Decimal("0.01"))
        self.nodes[0].add_p2p_connection(P2PInterface()).send_and_ping(msg_tx(original["tx"]))
        assert original["txid"] in self.nodes[0].getrawmempool()
        res = self.nodes[0].submitpackage([replacement["hex"]])
        assert_equal(res["package_msg"], "success")
        assert "fees" in res["tx-results"][replacement["wtxid"]]
        assert self.jobs()[replacement["wtxid"]]["state"] in ("queued", "running")
        self.nodes[0].abortprivatebroadcast(replacement["wtxid"])
        self.nodes[0].disconnect_p2ps()
        self.open_gate()

        self.log.info("Other submitpackage combinations: what counts as accepted, what fails, and how many jobs result")
        peer = self.nodes[0].add_p2p_connection(P2PInterface())

        def to_mempool(*txs):
            for t in txs:
                peer.send_and_ping(msg_tx(t["tx"]))
                assert t["txid"] in self.nodes[0].getrawmempool()

        def live(t):
            return [j for j in self.entries() if j["wtxid"] == t["wtxid"] and j["state"] in ("queued", "running")]

        def clean(t):
            if live(t):
                self.nodes[0].abortprivatebroadcast(t["wtxid"])

        def check(txs, msg, errors, jobs):
            """msg None: any failure. Each of errors: "" for none, None for any, else validation's reason."""
            res = self.nodes[0].submitpackage([t["hex"] for t in txs])
            if msg is None:
                assert res["package_msg"] != "success", res
            else:
                assert_equal(res["package_msg"].split(",")[0], msg)
            got = [res["tx-results"][t["wtxid"]].get("error", "").split(",")[0] for t in txs]
            assert_equal(len(got), len(errors))
            for g, want in zip(got, errors):
                if want is None:
                    assert g, got
                else:
                    assert_equal(g, want)
            assert_equal(len(live(txs[-1])), jobs)
            clean(txs[-1])

        def pair(parent_fee_rate=Decimal("0.003"), child_fee_rate=Decimal("0.003")):
            parent = self.wallet.create_self_transfer(fee_rate=parent_fee_rate)
            return parent, self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"], fee_rate=child_fee_rate)

        check([self.wallet.create_self_transfer()], "success", [""], 1)
        in_mempool = self.wallet.create_self_transfer()
        to_mempool(in_mempool)
        check([in_mempool], "success", [""], 1)  # rebroadcast, sent as given
        check([self.wallet.create_self_transfer(fee_rate=Decimal("0"))], None, ["min relay fee not met"], 0)
        check(pair(), "success", ["", ""], 1)
        check(pair(Decimal("0"), Decimal("0")), None, ["min relay fee not met", None], 0)

        # A child that pays for its parent is checked in full, as validation checks the two at their
        # package feerate: one it rejects for its witness, a dust output or its TRUC size queues nothing.
        def altered(t):
            return {**t, "hex": t["tx"].serialize().hex(), "wtxid": t["tx"].wtxid_hex}

        parent, child = pair(Decimal("0"), Decimal("0.01"))
        child["tx"].wit.vtxinwit[0].scriptWitness.stack = [b"\x00"]
        check([parent, altered(child)], None, ["min relay fee not met", None], 0)
        parent, child = pair(Decimal("0"), Decimal("0.01"))
        child["tx"].vout[0].nValue -= 1
        child["tx"].vout.append(CTxOut(1, child["tx"].vout[0].scriptPubKey))
        check([parent, altered(child)], None, ["min relay fee not met", "dust"], 0)
        parent = self.wallet.create_self_transfer(fee_rate=Decimal("0"), version=3)
        child = self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"], version=3, target_vsize=1500)
        check([parent, child], "TRUC-violation", ["min relay fee not met", None], 0)
        # And replacements go as a peer takes them: a TRUC parent that pays nothing replaces a mempool
        # transaction, its child paying for both; a parent that pays for itself replaces one, its
        # child paying less per vbyte; and a child replaces one.
        coin = self.wallet.get_utxo(confirmed_only=True)
        to_mempool(self.wallet.create_self_transfer(utxo_to_spend=coin, version=3))
        parent = self.wallet.create_self_transfer(utxo_to_spend=coin, fee_rate=Decimal("0"), version=3)
        check([parent, self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"], fee_rate=Decimal("0.01"), version=3)], "success", ["", ""], 1)
        coin = self.wallet.get_utxo(confirmed_only=True)
        to_mempool(self.wallet.create_self_transfer(utxo_to_spend=coin))
        parent = self.wallet.create_self_transfer(utxo_to_spend=coin, fee_rate=Decimal("0.05"))
        check([parent, self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"])], "success", ["", ""], 1)
        coin = self.wallet.get_utxo(confirmed_only=True)
        to_mempool(self.wallet.create_self_transfer(utxo_to_spend=coin))
        parent = self.wallet.create_self_transfer()
        check([parent, self.wallet.create_self_transfer_multi(utxos_to_spend=[parent["new_utxo"], coin], fee_per_output=50000)], "success", ["", ""], 1)
        parent, child = pair(child_fee_rate=Decimal("0"))
        to_mempool(parent)
        check([parent, child], None, ["", "min relay fee not met"], 0)
        parent, child = pair()
        to_mempool(parent, child)
        check([parent, child], "success", ["", ""], 1)
        assert_raises_rpc_error(-25, None, self.nodes[0].submitpackage,
                                [self.wallet.create_self_transfer()["hex"], self.wallet.create_self_transfer()["hex"]])

        # A job that serves the parent also covers the child submitted on its own; a job for the child
        # alone does not cover the package, which asks for the parent to be served.
        parent, child = pair()
        to_mempool(parent)
        self.nodes[0].submitpackage([parent["hex"], child["hex"]])
        self.nodes[0].sendrawtransaction(child["hex"])
        assert_equal(len(live(child)), 1)
        clean(child)
        parent, child = pair()
        to_mempool(parent)
        self.nodes[0].sendrawtransaction(child["hex"])
        self.nodes[0].submitpackage([parent["hex"], child["hex"]])
        assert_equal(len(live(child)), 2)
        clean(child)
        self.nodes[0].disconnect_p2ps()
        self.open_gate()

    def run_test(self):
        self.mocktime = int(time.time())
        self.advance(0)
        self.wallet = MiniWallet(self.nodes[0])
        self.generate(self.wallet, 260)  # enough mature coins for the retention section
        if self.options.package:
            self.log.info("Under -privatebroadcast a package is at most one parent and its child, and a valid single transaction also works")
            p1 = self.wallet.create_self_transfer()
            p2 = self.wallet.create_self_transfer()
            c = self.wallet.create_self_transfer_multi(utxos_to_spend=[p1["new_utxo"], p2["new_utxo"]])
            assert_raises_rpc_error(-8, None, self.nodes[0].submitpackage, [p1["hex"], p2["hex"], c["hex"]])
            single = self.wallet.create_self_transfer()
            res = self.nodes[0].submitpackage([single["hex"]])
            assert_equal(res["package_msg"], "success")
            assert "fees" in res["tx-results"][single["wtxid"]]
            assert "parent_txid" not in self.jobs()[single["wtxid"]]
            assert single["txid"] not in self.nodes[0].getrawmempool()
            self.finish([single["wtxid"]])

            self.log.info("A package that validation rejects as a whole, before any per-transaction result, queues no job")
            before = set(self.jobs())
            coin = self.wallet.get_utxo()
            parent = self.wallet.create_self_transfer(utxo_to_spend=coin)
            double_spend = self.wallet.create_self_transfer_multi(utxos_to_spend=[parent["new_utxo"], coin])
            res = self.nodes[0].submitpackage([parent["hex"], double_spend["hex"]])
            assert_equal(res["package_msg"].split(",")[0], "conflict-in-package")
            for t in (parent, double_spend):
                assert res["tx-results"][t["wtxid"]]["error"]
            assert_equal(set(self.jobs()), before)

            self.test_package()
            self.socks5_server.stop()
            return

        self.log.info("The RPCs are unavailable without -privatebroadcast")
        assert_raises_rpc_error(-32601, None, self.nodes[1].getprivatebroadcastinfo)
        assert_raises_rpc_error(-32601, None, self.nodes[1].abortprivatebroadcast, "00" * 32)

        self.log.info("A submitted transaction becomes a job that announces it over the proxy and never enters the mempool directly")
        node_state = self.node_state()
        tx = self.wallet.create_self_transfer()
        assert_equal(self.nodes[0].sendrawtransaction(tx["hex"]), tx["txid"])
        assert tx["txid"] not in self.nodes[0].getrawmempool()
        first = tx["wtxid"]
        jobs = self.jobs()
        assert_equal(list(jobs), [first])
        assert_equal(jobs[first]["txid"], tx["txid"])
        assert jobs[first]["state"] in ("queued", "running")
        self.finish([first])
        assert_equal(self.node_state(), node_state)
        job = self.jobs()[first]
        assert_equal(job["announced"], True)
        report = job["report"]
        assert_equal(report["txid"], tx["txid"])
        assert_greater_than_or_equal(report["summary"]["announcements_written"], 1)
        assert "time_started" in job and "time_ended" in job
        # N1: the bitcoind recipient took it, and this node only saw it once the network relayed it
        # back: node1's INV trickle and node0's request delay pass on the ticking clock.
        self.wait_until(lambda: tx["txid"] in self.nodes[1].getrawmempool())
        self.tick_until(lambda: tx["txid"] in self.nodes[0].getrawmempool())
        self.wait_until(lambda: "seen_in_mempool" in self.jobs()[first])
        served = [a for s in report["slots"] for a in s["attempts"] if a["tx_written_ms"] is not None]
        assert_greater_than_or_equal(len(served), 1)
        # The job is the tool's: its recipients saw the tool's profile, and its schedule and summary have the tool's shape.
        self.check_wire(tx["wtxid"])
        check_schedule(report, divisor=1, tolerance_ms=0)
        check_summary(report)

        self.log.info("Queued jobs start a drawn spacing after the previous start, whatever has ended; queued and running jobs can be aborted")
        txs = [self.wallet.create_self_transfer() for _ in range(3)]
        w = [t["wtxid"] for t in txs]
        for t in txs:
            self.nodes[0].sendrawtransaction(t["hex"])
        self.wait_for_state(w[0], "running")
        assert_equal([self.jobs()[i]["state"] for i in w[1:]], ["queued", "queued"])
        # Aborting a running job, here by txid, ends it early with a report of what it did, and does
        # not start the next one.
        started = self.jobs()[w[0]]["time_started"]
        res = self.nodes[0].abortprivatebroadcast(txs[0]["txid"])
        assert_equal([(r["wtxid"], r["hex"], r["state"]) for r in res["removed_transactions"]], [(w[0], txs[0]["hex"], "running")])
        job = self.wait_for_state(w[0], "aborted", timeout=30)
        assert_equal(job["report"]["summary"]["interrupted"], True)
        assert_equal(job["announced"], False)  # stopped before delivery start: no INV written
        never_announced = txs[0]
        # Just short of the minimum spacing the next job still waits, however long the node has had to
        # notice the clock: it must within a second.
        self.advance(started + START_SPACING_MIN_S - 1 - self.mocktime)
        time.sleep(2)
        assert_equal(self.jobs()[w[1]]["state"], "queued")
        # Once the spacing has passed the next job starts, and the one after it starts while it still runs.
        self.open_gate()
        self.wait_for_state(w[1], "running")
        self.open_gate()
        self.wait_for_state(w[2], "running")
        assert_equal(self.jobs()[w[1]]["state"], "running")
        # A queued job aborted before it runs, here by wtxid, ends with no report.
        extra = self.wallet.create_self_transfer()
        self.nodes[0].sendrawtransaction(extra["hex"])
        assert_equal(self.jobs()[extra["wtxid"]]["state"], "queued")
        # A transaction whose job is still queued or running is not queued again.
        before = len(self.entries())
        for t in (txs[1], txs[2], extra):
            assert_equal(self.nodes[0].sendrawtransaction(t["hex"]), t["txid"])
        assert_equal(len(self.entries()), before)
        res = self.nodes[0].abortprivatebroadcast(extra["wtxid"])
        assert_equal([(r["txid"], r["state"]) for r in res["removed_transactions"]], [(extra["txid"], "aborted")])
        assert "report" not in self.jobs()[extra["wtxid"]]
        assert_raises_rpc_error(-5, None, self.nodes[0].abortprivatebroadcast, extra["wtxid"])
        assert_raises_rpc_error(-5, None, self.nodes[0].abortprivatebroadcast, "00" * 32)
        # Once its job has finished, the same transaction may be queued again as a new job.
        assert_equal(self.nodes[0].sendrawtransaction(extra["hex"]), extra["txid"])
        assert_equal(len(self.entries()), before + 1)
        assert_equal(self.jobs()[extra["wtxid"]]["state"], "queued")
        self.end_jobs(w[1:])
        self.finish([extra["wtxid"]])

        self.log.info("A transaction whose running job is being aborted can be queued again at once")
        t = self.wallet.create_self_transfer()
        self.nodes[0].sendrawtransaction(t["hex"])
        self.wait_for_state(t["wtxid"], "running")
        self.nodes[0].abortprivatebroadcast(t["wtxid"])
        self.nodes[0].sendrawtransaction(t["hex"])
        runs = [j for j in self.entries() if j["wtxid"] == t["wtxid"]]
        assert_equal([j["state"] for j in runs][-1], "queued")
        assert_equal(len(runs), 2)
        self.wait_until(lambda: [j["state"] for j in self.entries() if j["wtxid"] == t["wtxid"]] == ["aborted", "queued"])
        self.nodes[0].abortprivatebroadcast(t["wtxid"])
        self.open_gate()

        self.log.info("A clock that steps back holds the queue for a spacing from the step, not until it catches up")
        txs = [self.wallet.create_self_transfer() for _ in range(2)]
        w = [t["wtxid"] for t in txs]
        for t in txs:
            self.nodes[0].sendrawtransaction(t["hex"])
        self.wait_for_state(w[0], "running")
        assert_equal(self.jobs()[w[1]]["state"], "queued")
        # Only node0's clock moves back, then ticks until the next job starts; end_jobs then puts both
        # nodes' clocks in step again. The node notices the step within a few ticks, and the next start
        # comes a spacing after that.
        back = self.mocktime - 3600
        clock = back

        def tick():
            nonlocal clock
            if self.jobs()[w[1]]["state"] == "running":
                return True
            self.nodes[0].setmocktime(clock)
            clock += 1
            return False
        self.wait_until(tick)
        started = self.jobs()[w[1]]["time_started"]
        assert back + START_SPACING_MIN_S <= started <= back + 2 * START_SPACING_MAX_S, started - back
        self.end_jobs(w)

        self.log.info("Receipt from the network while a job runs is recorded, and does not stop the job")
        # N7. This job announces to the Python recipients only, so node1 still lacks the transaction when it
        # hands it to node0 through the ordinary link.
        with self.lock:
            self.exit_path_active = self.exit_path[6:]
        seen = self.wallet.create_self_transfer()
        self.nodes[0].sendrawtransaction(seen["hex"])
        job_id = seen["wtxid"]
        assert_equal(self.jobs()[job_id]["txid"], seen["txid"])
        self.start_delivery([job_id])
        self.wait_for_prompt_slots([job_id])
        progress = self.progress(job_id)
        assert_greater_than_or_equal(progress["connections"], 1)
        assert_greater_than_or_equal(progress["announcements_written"], 1)
        # Announced only by the job, it is in neither node's mempool yet.
        for node in self.nodes:
            assert seen["txid"] not in node.getrawmempool()
        assert "seen_in_mempool" not in self.jobs()[job_id]
        self.nodes[1].sendrawtransaction(seen["hex"])
        self.tick_until(lambda: seen["txid"] in self.nodes[0].getrawmempool())  # the job's late slots are still ahead
        self.wait_until(lambda: "seen_in_mempool" in self.jobs()[job_id])
        assert_equal(self.jobs()[job_id]["state"], "running")
        # Nothing about the job changes: it keeps running, and its later slots still dial.
        dialled = self.progress(job_id)["connections"]

        def dialled_again():
            job = self.jobs()[job_id]
            assert_equal(job["state"], "running")
            return job["progress"]["connections"] > dialled
        self.tick_until(dialled_again)
        job = self.end_jobs([job_id])[0]
        assert_equal(job["announced"], True)
        assert_equal(job["report"]["summary"]["interrupted"], False)
        assert_greater_than_or_equal(job["report"]["summary"]["announcements_written"], 1)
        with self.lock:
            self.exit_path_active = self.exit_path[:6]


        self.log.info("A copy whose txid is in the mempool with another witness is its own job, and is sent as given")
        tx = self.wallet.create_self_transfer()
        self.nodes[0].add_p2p_connection(P2PInterface()).send_and_ping(msg_tx(tx["tx"]))
        other = self.witness_variant(tx)
        assert_equal(self.nodes[0].sendrawtransaction(tx["hex"]), tx["txid"])
        assert_equal(self.nodes[0].sendrawtransaction(other.serialize().hex()), tx["txid"])
        live = {j["wtxid"] for j in self.entries() if j["state"] in ("queued", "running")}
        assert {tx["wtxid"], other.wtxid_hex} <= live, live  # keyed by wtxid
        # The txid was in the mempool at submission, so both jobs show it from then on (N7, Interface/Node).
        for wtxid in (tx["wtxid"], other.wtxid_hex):
            assert "seen_in_mempool" in self.jobs()[wtxid], self.jobs()[wtxid]
        self.nodes[0].abortprivatebroadcast(tx["wtxid"])
        # The variant's job announces to Python recipients only: each that asked got the bytes submitted.
        with self.lock:
            self.exit_path_active = self.exit_path[6:]
            before = {e: len(ls) for e, ls in self.listeners.items()}
        self.finish([other.wtxid_hex])
        with self.lock:
            sent = {t.serialize() for e, ls in self.listeners.items() for listener in ls[before.get(e, 0):] for t in listener.txs_received}
            self.exit_path_active = self.exit_path[:6]
        assert_equal(sent, {other.serialize()})
        self.nodes[0].disconnect_p2ps()
        self.open_gate()

        self.log.info("The txid reaching the mempool with another witness after submission is seen too")
        funding = self.wallet.create_self_transfer()["tx"]
        amount = funding.vout[0].nValue - 10000
        funding, submitted, relayed = build_malleated_tx_package(parent=funding, rebalance_parent_output_amount=amount, child_amount=amount - 10000)
        self.wallet.sendrawtransaction(from_node=self.nodes[1], tx_hex=funding.serialize().hex())
        self.generate(self.nodes[1], 1, sync_fun=self.sync_blocks)
        assert_equal(submitted.txid_hex, relayed.txid_hex)
        job_id = self.jobs_after_submit(submitted.serialize().hex())
        assert "seen_in_mempool" not in self.jobs()[job_id]
        self.nodes[1].sendrawtransaction(relayed.serialize().hex())
        self.tick_until(lambda: relayed.txid_hex in self.nodes[0].getrawmempool())
        self.wait_until(lambda: "seen_in_mempool" in self.jobs()[job_id])
        self.nodes[0].abortprivatebroadcast(job_id)
        self.wait_for_state(job_id, "aborted")
        self.open_gate()

        self.log.info("Disabling networking aborts running and queued jobs for good; re-enabling admits new ones")
        txs = [self.wallet.create_self_transfer() for _ in range(3)]
        ids = [self.jobs_after_submit(t["hex"]) for t in txs]
        self.wait_for_state(ids[0], "running")
        self.open_gate()
        self.wait_for_state(ids[1], "running")
        assert_equal(self.jobs()[ids[-1]]["state"], "queued")
        self.nodes[0].setnetworkactive(False)
        # Running and queued jobs all end (N6; the unit tests check its bound). Networking stays off
        # until they have.
        for i in ids:
            job = self.wait_for_state(i, "aborted", timeout=30)
            assert "error" in job
        for i in ids[:2]:
            assert_equal(self.jobs()[i]["report"]["summary"]["interrupted"], True)
        assert_raises_rpc_error(-37, None, self.nodes[0].sendrawtransaction, self.wallet.create_self_transfer()["hex"])
        self.nodes[0].setnetworkactive(True)
        # setnetworkactive dropped node0's peers asynchronously; restore the ordinary link explicitly.
        self.disconnect_nodes(0, 1)
        self.connect_nodes(0, 1)
        revived = self.wallet.create_self_transfer()
        self.finish([self.jobs_after_submit(revived["hex"])])

        if self.is_wallet_compiled():
            self.log.info("Wallet sends are not private broadcasts: they enter the mempool and queue no job")
            # N11.
            self.nodes[0].createwallet("w")
            w = self.nodes[0].get_wallet_rpc("w")
            self.wallet.send_to(from_node=self.nodes[1], scriptPubKey=address_to_scriptpubkey(w.getnewaddress()), amount=1_000_000)
            self.generate(self.nodes[1], 1, sync_fun=self.sync_blocks)
            before = set(self.jobs())
            txid = w.sendtoaddress(address=w.getnewaddress(), amount=Decimal("0.001"), fee_rate=10)
            assert txid in self.nodes[0].getrawmempool()
            assert_equal(set(self.jobs()), before)

        self.log.info("Finished jobs are retained up to a bound, oldest dropped first")
        txs = [self.wallet.create_self_transfer() for _ in range(MAX_FINISHED_JOBS + 2)]
        for t in txs:
            self.nodes[0].sendrawtransaction(t["hex"])
        self.wait_for_state(txs[0]["wtxid"], "running")
        entries = self.entries()
        states = [j["state"] for j in entries]
        assert_equal(states.count("running"), 1)
        assert_equal(states.count("queued"), MAX_FINISHED_JOBS + 1)
        latest = {j["wtxid"]: j for j in entries}
        running = [t["wtxid"] for t in txs if latest[t["wtxid"]]["state"] == "running"]
        for t in txs:
            if t["wtxid"] not in running:
                self.nodes[0].abortprivatebroadcast(t["wtxid"])
        entries = self.entries()
        assert first not in {j["wtxid"] for j in entries}  # the first job's report has been trimmed
        assert_equal(len(entries), MAX_FINISHED_JOBS + 1)
        self.finish(running)

        self.log.info("Every connection and every query set had credentials of its own, with -proxyrandomize=0")
        # B2, U2.
        commands = drain_socks_commands(self.socks5_server)
        assert_greater_than_or_equal(len(commands), 20)
        query_sets(commands)

        # The transaction whose job was aborted before it announced anything is still not in the mempool.
        assert never_announced["txid"] not in self.nodes[0].getrawmempool()

        self.log.info("Discovery ignores node state: a banned address and one of the node's own addresses are still dialled")
        # R6.
        own, banned = self.exit_path[10], self.exit_path[11]
        self.restart_node(0, extra_args=self.extra_args[0] + [f"-externalip={own}"])
        self.advance(0)
        self.nodes[0].setban(banned, "add")
        with self.lock:
            self.exit_path_active = [own, banned]
            before = {e: len(self.listeners.get(e, [])) for e in (own, banned)}
        t = self.wallet.create_self_transfer()
        job_id = self.jobs_after_submit(t["hex"])
        self.start_delivery([job_id])
        self.tick_until(lambda: all(len(self.listeners.get(e, [])) > before[e] for e in (own, banned)))
        report = self.end_jobs([job_id])[0]["report"]
        assert {f"{own}:{REGTEST_PORT}", f"{banned}:{REGTEST_PORT}"} <= {a["endpoint"] for s in report["slots"] for a in s["attempts"]}
        # Nothing brings the transaction back here (Python recipients, and the node has no peer), so it
        # is not in the mempool, during the job or after it.
        assert t["txid"] not in self.nodes[0].getrawmempool()
        self.nodes[0].setban(banned, "remove")
        with self.lock:
            self.exit_path_active = self.exit_path[:6]

        self.log.info("With no proxy yet, though onion can become reachable through Tor control, submissions fail")
        self.restart_node(0, extra_args=[a for a in self.extra_args[0] if not a.startswith("-onion=")] + ["-listenonion=1", "-torcontrol=127.0.0.1:1"])
        self.advance(0)
        assert_raises_rpc_error(-1, None, self.nodes[0].sendrawtransaction, self.wallet.create_self_transfer()["hex"])

        self.log.info("Jobs are kept in memory only; the default log names no transaction or peer; peer settings do not apply")
        # N9, N8, U2.
        self.restart_node(0, extra_args=self.extra_args[0] + ["-debug=none", "-onlynet=onion", "-dnsseed=0", "-fixedseeds=0"])
        self.advance(0)
        assert_equal(self.entries(), [])
        t = self.wallet.create_self_transfer()
        with self.nodes[0].assert_debug_log(expected_msgs=[], unexpected_msgs=[t["txid"], t["wtxid"], "11.22.33.", ".onion"]):
            job_id = self.jobs_after_submit(t["hex"])
            self.finish([job_id])
        report = self.jobs()[job_id]["report"]
        assert_greater_than(report["discovery"]["exit_path_candidates"], 0)
        assert any(a["provenance"] == "a.seed." for s in report["slots"] for a in s["attempts"])
        self.restart_node(0)
        self.advance(0)

        self.log.info("Stopping the node cuts short a job blocked in a SOCKS exchange, in discovery or in delivery")
        # Proxies that hold every RESOLVE, or every CONNECT, an hour. With the clock frozen only the stop
        # can end the job, and it must not wait for any deadline. The handler threads sleep through the
        # hold and end with the test.
        held_resolve = start_socks5_server(None, lambda name: "11.22.33.1", resolve_reply_delay=3600)
        held_connect = start_socks5_server(None, lambda name: "11.22.33.1", connect_reply_delay=3600)
        self.stop_node(0)
        for proxy, blocked_in in ((held_resolve, "discovery"), (held_connect, "delivery")):
            self.start_node(0, extra_args=[a if not a.startswith("-onion=") else f"-onion=127.0.0.1:{proxy.conf.addr[1]}"
                                           for a in self.extra_args[0]])
            self.advance(0)
            wtxid = self.jobs_after_submit(self.wallet.create_self_transfer()["hex"])
            if blocked_in == "discovery":
                self.wait_until(lambda: not proxy.queue.empty())  # a RESOLVE is held
            else:
                self.start_delivery([wtxid])
                self.wait_until(lambda: proxy.connects_received > 0)  # a CONNECT is held
            start = time.time()
            self.stop_node(0)
            assert_greater_than(10 * self.options.timeout_factor, time.time() - start)
        self.start_node(0)
        self.advance(0)
        self.socks5_server.stop()


if __name__ == '__main__':
    P2PPrivateBroadcast(__file__).main()
