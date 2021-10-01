// Copyright (c) 2019-2021 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <chainparams.h>
#include <consensus/validation.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <vbk/pop_service.hpp>
#include <vbk/test/util/consts.hpp>
#include <vbk/test/util/e2e_fixture.hpp>

#include <string>

inline std::vector<uint8_t> operator""_v(const char* s, size_t size)
{
    return std::vector<uint8_t>{s, s + size};
}

BOOST_AUTO_TEST_SUITE(block_validation_tests)


static altintegration::PopData generateRandPopData()
{
    // add PopData
    auto atv = altintegration::AssertDeserializeFromHex<altintegration::ATV>(VeriBlockTest::defaultAtvEncoded);
    auto vtb = altintegration::AssertDeserializeFromHex<altintegration::VTB>(VeriBlockTest::defaultVtbEncoded);


    altintegration::PopData popData;
    popData.atvs = {atv};
    popData.vtbs = {vtb, vtb, vtb};

    return popData;
}

BOOST_AUTO_TEST_CASE(GetBlockWeight_test)
{
    // Create random block
    CBlock block;
    block.hashMerkleRoot.SetNull();
    block.hashPrevBlock.SetNull();
    block.nBits = 10000;
    block.nNonce = 10000;
    block.nTime = 10000;
    block.nVersion = 1 | VeriBlock::POP_BLOCK_VERSION_BIT;

    int64_t expected_block_weight = GetBlockWeight(block);

    BOOST_CHECK(expected_block_weight > 0);

    altintegration::PopData popData = generateRandPopData();

    int64_t popDataWeight = VeriBlock::GetPopDataWeight(popData);

    BOOST_CHECK(popDataWeight > 0);

    // put PopData into block
    block.popData = popData;

    int64_t new_block_weight = GetBlockWeight(block);
    BOOST_CHECK_EQUAL(new_block_weight, expected_block_weight);
}

BOOST_AUTO_TEST_CASE(block_serialization_test)
{
    // Create random block
    CBlock block;
    block.hashMerkleRoot.SetNull();
    block.hashPrevBlock.SetNull();
    block.nBits = 10000;
    block.nNonce = 10000;
    block.nTime = 10000;
    block.nVersion = 1 | VeriBlock::POP_BLOCK_VERSION_BIT;

    altintegration::PopData popData = generateRandPopData();

    block.popData = popData;

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    BOOST_CHECK(stream.size() == 0);
    stream << block;
    BOOST_CHECK(stream.size() != 0);

    CBlock decoded_block;
    stream >> decoded_block;

    BOOST_CHECK(decoded_block.GetHash() == block.GetHash());
    BOOST_CHECK(decoded_block.popData == block.popData);
}

BOOST_AUTO_TEST_CASE(block_network_passing_test)
{
    // Create random block
    CBlock block;
    block.hashMerkleRoot.SetNull();
    block.hashPrevBlock.SetNull();
    block.nBits = 10000;
    block.nNonce = 10000;
    block.nTime = 10000;
    block.nVersion = 1 | VeriBlock::POP_BLOCK_VERSION_BIT;

    altintegration::PopData popData = generateRandPopData();

    block.popData = popData;

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
}

BOOST_FIXTURE_TEST_CASE(BlockPoPVersion_test, E2eFixture)
{
    for (size_t i = 0; i < 400; ++i) {
        CreateAndProcessBlock({}, ChainActive().Tip()->GetBlockHash(), cbKey);
    }

    auto block = CreateAndProcessBlock({}, ChainActive().Tip()->GetBlockHash(), cbKey);
}

BOOST_AUTO_TEST_SUITE_END()
