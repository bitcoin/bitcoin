#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Analyze a txgraph binary trace file.

Prints cluster size distribution and identifies chain-shaped clusters.
A cluster is "chain-shaped" if every transaction in it has at most one
parent and at most one child (i.e. a linear A->B->C->... topology).

Usage:
    python3 analyze_trace.py <trace_file>
"""

import struct
import sys
from collections import defaultdict

from tracing_defs import (
    PROBES_BY_OPCODE, read_event,
    OP_INIT, OP_ADD_TRANSACTION, OP_REMOVE_TRANSACTION,
    OP_ADD_DEPENDENCY, OP_UNLINK_REF,
    OP_START_STAGING, OP_ABORT_STAGING, OP_COMMIT_STAGING,
)


class Graph:
    """Track live transactions and their dependency edges."""

    def __init__(self):
        self.live = set()
        self.parents = defaultdict(set)   # child -> {parents}
        self.children = defaultdict(set)  # parent -> {children}

    def add_tx(self, idx):
        self.live.add(idx)

    def remove_tx(self, idx):
        self.live.discard(idx)
        for p in list(self.parents.get(idx, ())):
            self.children[p].discard(idx)
        for c in list(self.children.get(idx, ())):
            self.parents[c].discard(idx)
        self.parents.pop(idx, None)
        self.children.pop(idx, None)

    def add_dep(self, parent, child):
        if parent in self.live and child in self.live:
            self.parents[child].add(parent)
            self.children[parent].add(child)

    def snapshot(self):
        """Return a deep copy of the graph state."""
        g = Graph()
        g.live = set(self.live)
        g.parents = defaultdict(set, {k: set(v) for k, v in self.parents.items()})
        g.children = defaultdict(set, {k: set(v) for k, v in self.children.items()})
        return g

    def sanity_check(self):
        """Verify all edges reference live transactions."""
        errors = 0
        for node in self.live:
            for p in self.parents.get(node, ()):
                if p not in self.live:
                    if errors < 5:
                        print(f"  STALE EDGE: live node {node} has dead parent {p}", file=sys.stderr)
                    errors += 1
            for c in self.children.get(node, ()):
                if c not in self.live:
                    if errors < 5:
                        print(f"  STALE EDGE: live node {node} has dead child {c}", file=sys.stderr)
                    errors += 1
        if errors:
            print(f"  Total stale edges: {errors}", file=sys.stderr)
        return errors

    def analyze(self):
        """Find connected components; classify each as chain or non-chain.

        Returns (size_dist, chain_dist) where each is {size: count}.
        """
        visited = set()
        size_dist = defaultdict(int)
        chain_dist = defaultdict(int)

        for tx in self.live:
            if tx in visited:
                continue
            # BFS
            component = []
            queue = [tx]
            while queue:
                node = queue.pop()
                if node in visited:
                    continue
                visited.add(node)
                component.append(node)
                for p in self.parents.get(node, ()):
                    if p in self.live and p not in visited:
                        queue.append(p)
                for c in self.children.get(node, ()):
                    if c in self.live and c not in visited:
                        queue.append(c)

            sz = len(component)
            size_dist[sz] += 1

            # Chain: every node has <= 1 parent and <= 1 child within cluster
            comp_set = set(component)
            is_chain = all(
                len(self.parents.get(n, set()) & comp_set) <= 1
                and len(self.children.get(n, set()) & comp_set) <= 1
                for n in component
            )
            if is_chain:
                chain_dist[sz] += 1

        return size_dist, chain_dist


def print_report(graph, label, max_cluster_count=None):
    size_dist, chain_dist = graph.analyze()
    total_clusters = sum(size_dist.values())
    total_txs = sum(k * v for k, v in size_dist.items())
    total_chains = sum(chain_dist.values())
    total_chain_txs = sum(k * chain_dist.get(k, 0) for k in size_dist)

    print(f"\n{'=' * 60}")
    print(f"  {label}")
    print(f"  {total_txs} transactions, {total_clusters} clusters")
    print(f"{'=' * 60}\n")

    print(f"  {'Size':>6}  {'Clusters':>10}  {'Chains':>10}  {'Non-chain':>10}  {'Chain%':>8}")
    print(f"  {'----':>6}  {'--------':>10}  {'------':>10}  {'---------':>10}  {'------':>8}")

    for sz in sorted(size_dist):
        c = size_dist[sz]
        ch = chain_dist.get(sz, 0)
        nc = c - ch
        pct = 100.0 * ch / c if c else 0
        print(f"  {sz:>6}  {c:>10}  {ch:>10}  {nc:>10}  {pct:>7.1f}%")

    print(f"  {'----':>6}  {'--------':>10}  {'------':>10}  {'---------':>10}")
    pct = 100.0 * total_chains / total_clusters if total_clusters else 0
    print(f"  {'TOTAL':>6}  {total_clusters:>10}  {total_chains:>10}  {total_clusters - total_chains:>10}  {pct:>7.1f}%")

    # Transactions in chain clusters vs non-chain clusters
    non_chain_txs = total_txs - total_chain_txs
    print(f"\n  Transactions in chain clusters:     {total_chain_txs:>8} ({100.0 * total_chain_txs / total_txs if total_txs else 0:.1f}%)")
    print(f"  Transactions in non-chain clusters: {non_chain_txs:>8} ({100.0 * non_chain_txs / total_txs if total_txs else 0:.1f}%)")

    # Transactions with dependencies
    has_dep = sum(1 for t in graph.live
                  if graph.parents.get(t) or graph.children.get(t))
    print(f"  Transactions with dependencies:     {has_dep:>8} ({100.0 * has_dep / total_txs if total_txs else 0:.1f}%)")

    # Warn about oversized clusters
    if max_cluster_count is not None:
        for sz in sorted(size_dist):
            if sz > max_cluster_count:
                print(f"\n  WARNING: {size_dist[sz]} cluster(s) of size {sz} exceed max_cluster_count={max_cluster_count}")
                # Find and print details of one oversized cluster
                visited = set()
                for tx in graph.live:
                    if tx in visited:
                        continue
                    component = []
                    queue = [tx]
                    while queue:
                        node = queue.pop()
                        if node in visited:
                            continue
                        visited.add(node)
                        component.append(node)
                        for p in graph.parents.get(node, ()):
                            if p in graph.live and p not in visited:
                                queue.append(p)
                        for c in graph.children.get(node, ()):
                            if c in graph.live and c not in visited:
                                queue.append(c)
                    if len(component) == sz:
                        comp_set = set(component)
                        max_parents = max(len(graph.parents.get(n, set()) & comp_set) for n in component)
                        max_children = max(len(graph.children.get(n, set()) & comp_set) for n in component)
                        total_edges = sum(len(graph.children.get(n, set()) & comp_set) for n in component)
                        roots = [n for n in component if not (graph.parents.get(n, set()) & comp_set)]
                        leaves = [n for n in component if not (graph.children.get(n, set()) & comp_set)]
                        print(f"    Sample cluster: {sz} txs, {total_edges} edges, "
                              f"{len(roots)} roots, {len(leaves)} leaves, "
                              f"max_fan_in={max_parents}, max_fan_out={max_children}")
                        break


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <trace_file>", file=sys.stderr)
        sys.exit(1)

    with open(sys.argv[1], 'rb') as f:
        # --- Header ---
        magic = f.read(8)
        if magic != b'TXGTRACE':
            print("Invalid trace file (bad magic)", file=sys.stderr)
            sys.exit(1)
        ver = struct.unpack('<I', f.read(4))[0]
        print(f"Trace format version: {ver}")

        graph = Graph()
        staging_buf = None   # list of (op, args) when inside staging
        max_cluster_count = None
        peak_size = 0
        peak_snapshot = None
        peak_op = 0
        op_count = 0
        commit_count = 0
        unstaged_add_tx = 0
        unstaged_add_dep = 0
        unstaged_remove_tx = 0
        unstaged_unlink_ref = 0

        def apply_mutation(op, args):
            if op == OP_ADD_TRANSACTION:
                graph.add_tx(args[0])
            elif op in (OP_REMOVE_TRANSACTION, OP_UNLINK_REF):
                graph.remove_tx(args[0])
            elif op == OP_ADD_DEPENDENCY:
                graph.add_dep(args[0], args[1])

        def check_peak():
            nonlocal peak_size, peak_snapshot, peak_op, op_count
            cur = len(graph.live)
            if cur > peak_size:
                peak_size = cur
                peak_snapshot = graph.snapshot()
                peak_op = op_count

        while True:
            b = f.read(1)
            if not b:
                break
            op = b[0]
            op_count += 1

            if op not in PROBES_BY_OPCODE:
                print(f"Unknown opcode 0x{op:02x} at op {op_count}", file=sys.stderr)
                break

            event = read_event(f, op)
            if event is None:
                break

            # Semantic handling: only mutations and staging affect the graph
            if op == OP_INIT:
                max_cluster_count = event["max_cluster_count"]
                print(f"Parameters: max_cluster_count={event['max_cluster_count']} "
                      f"max_cluster_size={event['max_cluster_size']} "
                      f"acceptable_cost={event['acceptable_cost']}")
                continue
            elif op == OP_ADD_TRANSACTION:
                mut = (op, (event["graph_idx"],))
            elif op in (OP_REMOVE_TRANSACTION, OP_UNLINK_REF):
                mut = (op, (event["graph_idx"],))
            elif op == OP_ADD_DEPENDENCY:
                mut = (op, (event["parent"], event["child"]))
            elif op == OP_START_STAGING:
                staging_buf = []
                continue
            elif op == OP_COMMIT_STAGING:
                if staging_buf is not None:
                    commit_count += 1
                    for m_op, m_args in staging_buf:
                        apply_mutation(m_op, m_args)
                staging_buf = None
                check_peak()
                continue
            elif op == OP_ABORT_STAGING:
                staging_buf = None
                continue
            else:
                # Non-mutation op (queries, maintenance), skip
                continue

            # Buffer or apply mutation
            if staging_buf is not None:
                staging_buf.append(mut)
            else:
                if op == OP_ADD_TRANSACTION:
                    unstaged_add_tx += 1
                elif op == OP_ADD_DEPENDENCY:
                    unstaged_add_dep += 1
                elif op == OP_REMOVE_TRANSACTION:
                    unstaged_remove_tx += 1
                elif op == OP_UNLINK_REF:
                    unstaged_unlink_ref += 1
                apply_mutation(mut[0], mut[1])
                check_peak()

        print(f"\nProcessed {op_count} operations, {commit_count} CommitStagings")
        print(f"Peak mempool size: {peak_size} transactions (at op #{peak_op})")
        print(f"Unstaged mutations: {unstaged_add_tx} ADD_TX, {unstaged_add_dep} ADD_DEP, "
              f"{unstaged_remove_tx} REMOVE_TX, {unstaged_unlink_ref} UNLINK_REF")

        # Sanity check
        print("\nEdge sanity check (final state):")
        err = graph.sanity_check()
        if not err:
            print("  OK - all edges reference live transactions")

        # Report
        if peak_snapshot is not None and len(graph.live) != peak_size:
            print("\nEdge sanity check (peak state):")
            perr = peak_snapshot.sanity_check()
            if not perr:
                print("  OK - all edges reference live transactions")
            print_report(peak_snapshot, f"Peak state ({peak_size} transactions)", max_cluster_count)

        print_report(graph, "Final state", max_cluster_count)


if __name__ == '__main__':
    main()
