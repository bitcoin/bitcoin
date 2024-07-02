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

    // Test that depgraph has all the fees/sizes/parents required by cluster.
    for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) {
        BOOST_CHECK(depgraph.FeeRate(i) == cluster[i].first);
        for (ClusterIndex par : cluster[i].second) {
            BOOST_CHECK(depgraph.Ancestors(i)[par]);
            BOOST_CHECK(depgraph.Descendants(par)[i]);
        }
    }

    // Run normal sanity checks on depgraph.
    SanityCheck(depgraph);

    // Test that the serialization of depgraph matches hexenc.
    std::vector<unsigned char> encoding;
    VectorWriter writer(encoding, 0);
    writer << Using<DepGraphFormatter>(depgraph);
    BOOST_CHECK_EQUAL(HexStr(encoding), hexenc);

    // Test that deserializing that encoding yields depgraph.
    SpanReader reader(encoding);
    DepGraph<SetType> depgraph_read;
    reader >> Using<DepGraphFormatter>(depgraph_read);
    BOOST_CHECK(depgraph == depgraph_read);
}

} // namespace

BOOST_AUTO_TEST_CASE(depgraph_ser_tests)
{
    // Empty cluster.
    TestDepGraphSerialization<TestBitSet>({}, "00");

    // Transactions: A(fee=0,size=1).
    TestDepGraphSerialization<TestBitSet>({{{0, 1}, {}}}, "010000");

    // Transactions: A(fee=42,size=11), B(fee=-13,size=7), B depends on A.
    TestDepGraphSerialization<TestBitSet>(
        {{{42, 11}, {}}, {{-13, 7}, {0}}},
        "0b5407190100");

    // Transactions: A(64,128), B(128,256), C(1,1), C depends on A and B.
    TestDepGraphSerialization<TestBitSet>(
        {{{64, 128}, {}}, {{128, 256}, {}}, {{1, 1}, {0, 1}}},
        "8000800081008100000102010100");

    // Transactions: A(-58,113), B(36,114), C(-59,115), D(37,116). Deps: B->A, C->A, D->C, in order
    // [B,A,C,D]. This exercises the 3rd condition in CanAddDependency; without it, the encoding
    // would include a "00" after "737301" (to signal the lack of C->B, which would still be
    // considered possible, despite it making C->A redundant).
    TestDepGraphSerialization<TestBitSet>(
        {{{57, 114}, {1}}, {{-57, 113}, {}}, {{-58, 115}, {1}}, {{58, 116}, {2}}},
        "72727171027373017474010000");
}

BOOST_AUTO_TEST_SUITE_END()
