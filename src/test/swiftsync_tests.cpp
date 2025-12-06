// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <boost/test/unit_test.hpp>
#include <common/args.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <swiftsync.h>
#include <test/util/setup_common.h>
#include <cstdio>

namespace {
Txid txid_1{Txid::FromHex("bd0f71c1d5e50589063e134fad22053cdae5ab2320db5bf5e540198b0b5a4e69").value()};
Txid txid_2{Txid::FromHex("b4749f017444b051c44dfd2720e88f314ff94f3dd6d56d40ef65854fcd7fff6b").value()};
Txid txid_3{Txid::FromHex("ee707be5201160e32c4fc715bec227d1aeea5940fb4295605e7373edce3b1a93").value()};
COutPoint outpoint_1{COutPoint(txid_1, 2142)};
COutPoint outpoint_2{COutPoint(txid_2, 99328)};
COutPoint outpoint_3{COutPoint(txid_3, 5438584)};
std::vector<uint64_t> unspent_block_1{0, 3253, 120};
std::vector<uint64_t> unspent_block_2{0, 999, 532, 624, 623623, 436134, 32443, 2346, 3, 3564, 234, 122};
std::vector<uint64_t> unspent_block_3{0, 4231, 92385, 53894, 82, 3, 2389453, 92, 2, 23985};
std::vector<uint64_t> unspent_block_4{0, 83948, 1111, 12424, 12, 2, 3, 3, 14};
} // namespace

BOOST_FIXTURE_TEST_SUITE(swiftsync_tests, BasicTestingSetup);

BOOST_AUTO_TEST_CASE(swiftsync_aggregate_test)
{
    swiftsync::Aggregate agg{};
    agg.Add(outpoint_1);
    agg.Spend(outpoint_2);
    agg.Add(outpoint_3);
    BOOST_CHECK(!agg.IsZero());
    agg.Spend(outpoint_1);
    agg.Add(outpoint_2);
    agg.Spend(outpoint_3);
    BOOST_CHECK(agg.IsZero());
}

BOOST_AUTO_TEST_CASE(swiftsync_hintfile_test)
{
    const fs::path temppath = fsbridge::AbsPathJoin(gArgs.GetDataDirNet(), fs::u8path("test.hints"));
    {
        FILE* file{fsbridge::fopen(temppath, "wb")};
        AutoFile afile{file};
        swiftsync::HintsfileWriter writer{afile, 4};
        BOOST_CHECK(writer.WriteNextUnspents(unspent_block_1, 1));
        BOOST_CHECK(writer.WriteNextUnspents(unspent_block_3, 3));
        BOOST_CHECK(writer.WriteNextUnspents(unspent_block_4, 4));
        BOOST_CHECK(writer.WriteNextUnspents(unspent_block_2, 2));
    }
    FILE* file{fsbridge::fopen(temppath, "rb")};
    AutoFile afile{file};
    swiftsync::HintsfileReader reader{afile};
    BOOST_CHECK(reader.StopHeight() == 4);
    auto query_block_4 = reader.ReadBlock(4);
    BOOST_CHECK(unspent_block_4 == query_block_4);
    auto query_block_3 = reader.ReadBlock(3);
    BOOST_CHECK(unspent_block_3 == query_block_3);
    auto query_block_2 = reader.ReadBlock(2);
    BOOST_CHECK(unspent_block_2 == query_block_2);
    auto query_block_1 = reader.ReadBlock(1);
    BOOST_CHECK(unspent_block_1 == query_block_1);
    BOOST_CHECK_THROW(reader.ReadBlock(0), std::out_of_range);
    BOOST_CHECK_THROW(reader.ReadBlock(5), std::out_of_range);
}

BOOST_AUTO_TEST_SUITE_END();
