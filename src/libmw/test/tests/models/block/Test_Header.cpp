// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/models/block/Header.h>

#include <test_framework/TestMWEB.h>

BOOST_FIXTURE_TEST_SUITE(TestHeader, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(Header)
{
    const uint64_t height = 1;
    const uint64_t outputMMRSize = 2;
    const uint64_t kernelMMRSize = 3;

    mw::Hash outputRoot = mw::Hash::FromHex("000102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F20");
    mw::Hash kernelRoot = mw::Hash::FromHex("002102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F20");
    mw::Hash leafsetRoot = mw::Hash::FromHex("003102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F20");
    BlindingFactor kernelOffset = BigInt<32>::FromHex("004102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F20");
    BlindingFactor stealth_offset = BigInt<32>::FromHex("005102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F20");

    mw::Header header(
        height,
        outputRoot,
        kernelRoot,
        leafsetRoot,
        kernelOffset,
        stealth_offset,
        outputMMRSize,
        kernelMMRSize
    );
    mw::Header header2(
        height + 1,
        outputRoot,
        kernelRoot,
        leafsetRoot,
        kernelOffset,
        stealth_offset,
        outputMMRSize,
        kernelMMRSize
    );

    BOOST_REQUIRE(header != header2);
    BOOST_REQUIRE(header.GetHeight() == height);
    BOOST_REQUIRE(header.GetOutputRoot() == outputRoot);
    BOOST_REQUIRE(header.GetKernelRoot() == kernelRoot);
    BOOST_REQUIRE(header.GetLeafsetRoot() == leafsetRoot);
    BOOST_REQUIRE(header.GetKernelOffset() == kernelOffset);
    BOOST_REQUIRE(header.GetStealthOffset() == stealth_offset);
    BOOST_REQUIRE(header.GetNumTXOs() == outputMMRSize);
    BOOST_REQUIRE(header.GetNumKernels() == kernelMMRSize);
    BOOST_REQUIRE_EQUAL(header.Format(), "688fed89903b3f312daa3f04fe94994c67f3d0688482cdd5ada2851c0b70d7de");

    std::vector<uint8_t> header_serialized = header.Serialized();
    mw::Header header3;
    CDataStream(header_serialized, SER_DISK, 0) >> header3;
    BOOST_REQUIRE(header == header3);
}

BOOST_AUTO_TEST_SUITE_END()