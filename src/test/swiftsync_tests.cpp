// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <primitives/transaction.h>
#include <streams.h>
#include <swiftsync.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>
#include <cstdio>

namespace {
Txid txid_1{Txid{"bd0f71c1d5e50589063e134fad22053cdae5ab2320db5bf5e540198b0b5a4e69"}};
Txid txid_2{Txid{"b4749f017444b051c44dfd2720e88f314ff94f3dd6d56d40ef65854fcd7fff6b"}};
Txid txid_3{Txid{"ee707be5201160e32c4fc715bec227d1aeea5940fb4295605e7373edce3b1a93"}};
COutPoint outpoint_1{COutPoint{txid_1, 2142}};
COutPoint outpoint_2{COutPoint{txid_2, 99328}};
COutPoint outpoint_3{COutPoint{txid_3, 5438584}};
// One byte worth of hints.
std::vector<bool> unspent_block_1{true, false, false, true, false, true, true, true};
// Between one and two bytes worth of hints.
std::vector<bool> unspent_block_2{true, false, false, true, false, true, true, true, true, false, true};
// Two bytes worth of hints.
std::vector<bool> unspent_block_3{true, false, false, true, false, true, true, true, true, false, true, true, true, true, true, true};
} // namespace

BOOST_FIXTURE_TEST_SUITE(swiftsync_tests, BasicTestingSetup);

BOOST_AUTO_TEST_CASE(swiftsync_aggregate_test)
{
    swiftsync::Aggregate agg{};
    agg.Create(outpoint_1);
    agg.Spend(outpoint_2);
    agg.Create(outpoint_3);
    BOOST_CHECK(!agg.IsBalanced());
    agg.Spend(outpoint_1);
    agg.Create(outpoint_2);
    agg.Spend(outpoint_3);
    BOOST_CHECK(agg.IsBalanced());
}

BOOST_AUTO_TEST_CASE(swiftsync_hintfile_test)
{
    const fs::path temppath = fsbridge::AbsPathJoin(gArgs.GetDataDirNet(), fs::u8path("test.hints"));
    {
        FILE* file{fsbridge::fopen(temppath, "wb")};
        AutoFile afile{file};
        swiftsync::HintsfileWriter writer{afile, 3};
        swiftsync::BlockHintsWriter block_one{};
        for (const bool hint : unspent_block_1) {
            if (hint) {
                block_one.PushHighBit();
            } else {
                block_one.PushLowBit();
            };
        }
        BOOST_CHECK(writer.WriteNextUnspents(block_one, 1));
        swiftsync::BlockHintsWriter block_three{};
        for (const bool hint : unspent_block_3) {
            if (hint) {
                block_three.PushHighBit();
            } else {
                block_three.PushLowBit();
            };
        }
        BOOST_CHECK(writer.WriteNextUnspents(block_three, 3));
        swiftsync::BlockHintsWriter block_two{};
        for (const bool hint : unspent_block_2) {
            if (hint) {
                block_two.PushHighBit();
            } else {
                block_two.PushLowBit();
            };
        }
        BOOST_CHECK(writer.WriteNextUnspents(block_two, 2));
    }
    FILE* file{fsbridge::fopen(temppath, "rb")};
    AutoFile afile{file};
    swiftsync::HintsfileReader reader{afile};
    BOOST_CHECK_EQUAL(reader.StopHeight(), 3);
    auto query_block_3 = reader.ReadBlock(3);
    for (const bool want : unspent_block_3) {
        BOOST_CHECK_EQUAL(want, query_block_3.IsCurrUnspent());
        query_block_3.Next();
    }
    auto query_block_1 = reader.ReadBlock(1);
    for (const bool want : unspent_block_1) {
        BOOST_CHECK_EQUAL(want, query_block_1.IsCurrUnspent());
        query_block_1.Next();
    }
    auto query_block_2 = reader.ReadBlock(2);
    for (const bool want : unspent_block_2) {
        BOOST_CHECK_EQUAL(want, query_block_2.IsCurrUnspent());
        query_block_2.Next();
    }
    BOOST_CHECK_THROW(reader.ReadBlock(0), std::out_of_range);
    BOOST_CHECK_THROW(reader.ReadBlock(4), std::out_of_range);
    const fs::path bogus_path = fsbridge::AbsPathJoin(gArgs.GetDataDirNet(), fs::u8path("bogus.hints"));
    // Unknown file version.
    {
        FILE* file{fsbridge::fopen(bogus_path, "wb")};
        AutoFile afile{file};
        afile << swiftsync::FILE_MAGIC;
        // Version not currently supported
        afile << 0x01;
        BOOST_CHECK_THROW(swiftsync::HintsfileReader{afile}, std::ios_base::failure);
    }
    // Magic does not match.
    {
        FILE* file{fsbridge::fopen(bogus_path, "wb")};
        AutoFile afile{file};
        afile.seek(0, SEEK_SET);
        afile << 0xFF;
        BOOST_CHECK_THROW(swiftsync::HintsfileReader{afile}, std::ios_base::failure);
    }
}

BOOST_AUTO_TEST_SUITE_END();
