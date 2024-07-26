// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cluster_linearize.h>
#include <test/util/cluster_linearize.h>
#include <test/util/setup_common.h>
#include <util/bitset.h>
#include <util/strencodings.h>

#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(cluster_linearize_tests, BasicTestingSetup)

using namespace cluster_linearize;

namespace {

template<typename SetType>
void TestDepGraphSerialization(const Cluster<SetType>& cluster, const std::string& hexenc)
{
    DepGraph depgraph(cluster);

    // Run normal sanity and correspondence checks, which includes a round-trip test.
    VerifyDepGraphFromCluster(cluster, depgraph);

    // There may be multiple serializations of the same graph, but DepGraphFormatter's serializer
    // only produces one of those. Verify that hexenc matches that canonical serialization.
    std::vector<unsigned char> encoding;
    VectorWriter writer(encoding, 0);
    writer << Using<DepGraphFormatter>(depgraph);
    BOOST_CHECK_EQUAL(HexStr(encoding), hexenc);

    // Test that deserializing that encoding yields depgraph. This is effectively already implied
    // by the round-trip test above (if depgraph is acyclic), but verify it explicitly again here.
    SpanReader reader(encoding);
    DepGraph<SetType> depgraph_read;
    reader >> Using<DepGraphFormatter>(depgraph_read);
    BOOST_CHECK(depgraph == depgraph_read);
}

} // namespace

BOOST_AUTO_TEST_CASE(depgraph_ser_tests)
{
    // Empty cluster.
    TestDepGraphSerialization<TestBitSet>(
        {},
        "00" /* end of graph */);

    // Transactions: A(fee=0,size=1).
    TestDepGraphSerialization<TestBitSet>(
        {{{0, 1}, {}}},
        "01" /* A size */
        "00" /* A fee */
        "00" /* A insertion position (no skips): A */
        "00" /* end of graph */);

    // Transactions: A(fee=42,size=11), B(fee=-13,size=7), B depends on A.
    TestDepGraphSerialization<TestBitSet>(
        {{{42, 11}, {}}, {{-13, 7}, {0}}},
        "0b" /* A size */
        "54" /* A fee */
        "00" /* A insertion position (no skips): A */
        "07" /* B size */
        "19" /* B fee */
        "00" /* B->A dependency (no skips) */
        "00" /* B insertion position (no skips): A,B */
        "00" /* end of graph */);

    // Transactions: A(64,128), B(128,256), C(1,1), C depends on A and B.
    TestDepGraphSerialization<TestBitSet>(
        {{{64, 128}, {}}, {{128, 256}, {}}, {{1, 1}, {0, 1}}},
        "8000" /* A size */
        "8000" /* A fee */
        "00"   /* A insertion position (no skips): A */
        "8100" /* B size */
        "8100" /* B fee */
        "01"   /* B insertion position (skip B->A dependency): A,B */
        "01"   /* C size */
        "02"   /* C fee */
        "00"   /* C->B dependency (no skips) */
        "00"   /* C->A dependency (no skips) */
        "00"   /* C insertion position (no skips): A,B,C */
        "00"   /* end of graph */);

    // Transactions: A(-57,113), B(57,114), C(-58,115), D(58,116). Deps: B->A, C->A, D->C, in order
    // [B,A,C,D]. This exercises non-topological ordering (internally serialized as A,B,C,D).
    TestDepGraphSerialization<TestBitSet>(
        {{{57, 114}, {1}}, {{-57, 113}, {}}, {{-58, 115}, {1}}, {{58, 116}, {2}}},
        "71" /* A size */
        "71" /* A fee */
        "00" /* A insertion position (no skips): A */
        "72" /* B size */
        "72" /* B fee */
        "00" /* B->A dependency (no skips) */
        "01" /* B insertion position (skip A): B,A */
        "73" /* C size */
        "73" /* C fee */
        "01" /* C->A dependency (skip C->B dependency) */
        "00" /* C insertion position (no skips): B,A,C */
        "74" /* D size */
        "74" /* D fee */
        "00" /* D->C dependency (no skips) */
        "01" /* D insertion position (skip D->B dependency, D->A is implied): B,A,C,D */
        "00" /* end of graph */);

    // Transactions: A(1,2), B(3,1), C(2,1), D(1,3), E(1,1). Deps: C->A, D->A, D->B, E->D.
    // In order: [D,A,B,E,C]. Internally serialized in order A,B,C,D,E.
    TestDepGraphSerialization<TestBitSet>(
        {{{1, 3}, {1, 2}}, {{1, 2}, {}}, {{3, 1}, {}}, {{1, 1}, {0}}, {{2, 1}, {1}}},
        "02" /* A size */
        "02" /* A fee */
        "00" /* A insertion position (no skips): A */
        "01" /* B size */
        "06" /* B fee */
        "01" /* B insertion position (skip B->A dependency): A,B */
        "01" /* C size */
        "04" /* C fee */
        "01" /* C->A dependency (skip C->B dependency) */
        "00" /* C insertion position (no skips): A,B,C */
        "03" /* D size */
        "02" /* D fee */
        "01" /* D->B dependency (skip D->C dependency) */
        "00" /* D->A dependency (no skips) */
        "03" /* D insertion position (skip C,B,A): D,A,B,C */
        "01" /* E size */
        "02" /* E fee */
        "00" /* E->D dependency (no skips) */
        "02" /* E insertion position (skip E->C dependency, E->B and E->A are implied,
                skip insertion C): D,A,B,E,C */
        "00" /* end of graph */
    );
}

BOOST_AUTO_TEST_SUITE_END()
