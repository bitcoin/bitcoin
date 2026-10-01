#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the private broadcast queue cap: submissions beyond it are rejected and the queue is left
unchanged, rather than evicting jobs already queued.

The clock is mocked and never advanced, so the first job starts and every later one stays queued.
The proxy answers every RESOLVE, so the running job has candidates, and the frozen clock never
reaches its delivery start: it stays running.
"""

from test_framework.socks5 import start_socks5_server
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet


MAX_QUEUED_JOBS = 10_000  # the spec's MAX_QUEUED_JOBS
OVER_CAP = 5


class PrivateBroadcastCapTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def setup_nodes(self):
        self.socks5_server = start_socks5_server(None, lambda name: "11.22.33.1")
        self.extra_args = [["-privatebroadcast", f"-onion=127.0.0.1:{self.socks5_server.conf.addr[1]}", "-privatebroadcastseed=a.seed."]]
        super().setup_nodes()

    def jobs(self):
        return self.nodes[0].getprivatebroadcastinfo()["jobs"]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        self.generate(wallet, 101)
        node.setmocktime(node.getblockheader(node.getbestblockhash())["time"] + 1)

        # A parent that fans out to one output per job. It is too large to be standard, so it is
        # mined directly; nothing reaches the mempool under -privatebroadcast anyway.
        parent = wallet.create_self_transfer_multi(num_outputs=1 + MAX_QUEUED_JOBS + OVER_CAP, fee_per_output=500)
        self.generateblock(node, wallet.get_address(), [parent["hex"]])
        children = [wallet.create_self_transfer(utxo_to_spend=u) for u in parent["new_utxos"]]
        running, queued, over = children[0], children[1:1 + MAX_QUEUED_JOBS], children[1 + MAX_QUEUED_JOBS:]

        self.log.info(f"One job starts; {MAX_QUEUED_JOBS} more fill the queue")
        node.sendrawtransaction(running["hex"])
        for child in queued:
            node.sendrawtransaction(child["hex"])
        jobs = self.jobs()
        assert_equal([(j["wtxid"], j["state"]) for j in jobs], [(running["wtxid"], "running")] + [(c["wtxid"], "queued") for c in queued])

        self.log.info(f"Submitting {OVER_CAP} more: each is rejected and the queue is left unchanged")
        for child in over:
            assert_raises_rpc_error(-37, None, node.sendrawtransaction, child["hex"])
        assert_equal(self.jobs(), jobs)

        self.log.info("A transaction whose job is queued or running is accepted again without queuing anything, even with the queue full")
        for child in (running, queued[0], queued[-1]):
            assert_equal(node.sendrawtransaction(child["hex"]), child["txid"])
        assert_equal(self.jobs(), jobs)

        self.log.info("Aborting a queued job frees its place for a new submission")
        res = node.abortprivatebroadcast(queued[1]["txid"])
        assert_equal([(r["wtxid"], r["state"]) for r in res["removed_transactions"]], [(queued[1]["wtxid"], "aborted")])
        node.sendrawtransaction(over[0]["hex"])
        states = {}
        for j in self.jobs():
            states.setdefault(j["state"], []).append(j["wtxid"])
        assert_equal(states["aborted"], [queued[1]["wtxid"]])
        assert_equal(len(states["queued"]), MAX_QUEUED_JOBS)
        assert_equal(states["queued"][-1], over[0]["wtxid"])
        self.socks5_server.stop()


if __name__ == "__main__":
    PrivateBroadcastCapTest(__file__).main()
