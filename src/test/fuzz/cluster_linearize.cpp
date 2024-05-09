// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cluster_linearize.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/util/cluster_linearize.h>
#include <util/bitset.h>
#include <util/feefrac.h>

#include <stdint.h>
#include <vector>
#include <utility>

FUZZ_TARGET(clusterlin_add_dependency)
{
    // Verify that computing a DepGraph from a cluster, or building it step by step using AddDependency
    // have the same effect.

    // Construct a cluster of a certain length, with no dependencies.
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    auto num_tx = provider.ConsumeIntegralInRange<ClusterIndex>(2, 32);
    Cluster<TestBitSet> cluster(num_tx, std::pair{FeeFrac{0, 1}, TestBitSet{}});
    // Construct the corresponding DepGraph object (also no dependencies).
    DepGraph depgraph(cluster);
    SanityCheck(depgraph);
    // Read (parent, child) pairs, and add them to the cluster and depgraph.
    LIMITED_WHILE(provider.remaining_bytes() > 0, TestBitSet::Size() * TestBitSet::Size()) {
        auto parent = provider.ConsumeIntegralInRange<ClusterIndex>(0, num_tx - 1);
        auto child = provider.ConsumeIntegralInRange<ClusterIndex>(0, num_tx - 2);
        child += (child >= parent);
        cluster[child].second.Set(parent);
        depgraph.AddDependency(parent, child);
        assert(depgraph.Ancestors(child)[parent]);
        assert(depgraph.Descendants(parent)[child]);
    }
    // Sanity check the result.
    SanityCheck(depgraph);
    // Verify that the resulting DepGraph matches one recomputed from the cluster.
    assert(DepGraph(cluster) == depgraph);
}

FUZZ_TARGET(clusterlin_cluster_serialization)
{
    // Verify that any graph of transactions has its ancestry correctly computed by DepGraph, and
    // if it is a DAG, that it can be serialized as a DepGraph in a way that roundtrips. This
    // guarantees that any acyclic cluster has a corresponding DepGraph serialization.

    FuzzedDataProvider provider(buffer.data(), buffer.size());

    // Construct a cluster in a naive way (using a FuzzedDataProvider-based serialization).
    Cluster<TestBitSet> cluster;
    auto num_tx = provider.ConsumeIntegralInRange<ClusterIndex>(1, 32);
    cluster.resize(num_tx);
    for (ClusterIndex i = 0; i < num_tx; ++i) {
        cluster[i].first.size = provider.ConsumeIntegralInRange<int32_t>(1, 0x3fffff);
        cluster[i].first.fee = provider.ConsumeIntegralInRange<int64_t>(-0x8000000000000, 0x7ffffffffffff);
        for (ClusterIndex j = 0; j < num_tx; ++j) {
            if (i == j) continue;
            if (provider.ConsumeBool()) cluster[i].second.Set(j);
        }
    }

    // Construct dependency graph, and verify it matches the cluster (which includes a round-trip
    // check for the serialization).
    DepGraph depgraph(cluster);
    VerifyDepGraphFromCluster(cluster, depgraph);
}

FUZZ_TARGET(clusterlin_depgraph_serialization)
{
    // Verify that any deserialized depgraph is acyclic and roundtrips to an identical depgraph.

    // Construct a graph by deserializing.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    try {
        reader >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}
    SanityCheck(depgraph);

    // Verify the graph is a DAG.
    assert(IsAcyclic(depgraph));
}
