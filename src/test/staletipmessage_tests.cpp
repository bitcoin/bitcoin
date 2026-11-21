// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <protocol.h>
#include <staletipmessage.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <ios>
#include <optional>
#include <span>
#include <string>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(staletipmessage_tests, BasicTestingSetup)

namespace {

// BIP 332 test vector "Signet Stale Branch (19 blocks)":
// signet branch 00000012602f...e1c -> 0000000024ff...001.
const std::string SIGNET_TEST_VECTOR_HEX{
    "1c7e50802a59c42d780861d2c154a87cb02ff42305a933af2ede2f6012000000"
    "13"
    "0000002067aa2937b235e209f1ad40f37b17dcad8ca02d1fb9cf4c4928902eab3ff853a6cda16e691658151d81ad7b04"
    "00000020721ab6f36042842bab4583f98e15181b646c47c7b9dfb07a204c4e5152e54e97fda46e691658151dd2dcec18"
    "00000020ddc6b628101b404f78d446591a623e12bb7e7cd2ae1593d0cc6ef082e19fa81014a66e691658151d46070e10"
    "000000208be5b9607b20ac0163b885881aa6018ef60a5637b93a9cd85981515a169a31c36aab6e691658151de4e10600"
    "000000206c974ef806b2e6a21cc9a53fedcbd5d5e203b4929a1bd7c5859ec77d0a755c1d64ae6e691658151d2a2e130e"
    "0000002039505fa2ee91f5e5b2540b4a9a7f02ed58d79fc387412cf2520a39ee16bf435f6eb16e691658151db29c830a"
    "00000020411bce69cda2b239c74faea1f0cfaceabd90377114dc0ace14e2a58858cd4f9a91b46e691658151d4c2d3f05"
    "0000002096464a2ec42fea8419a5c6adcc41ca7dc3f53461cd0afc880f76498aa8eedbf993b56e691658151d5e99700b"
    "00000020c3cad611ee3c7bffec3f80fc83f68192ca0a86e8c89050f8353480310d080caf08b76e691658151dbc61a615"
    "0000002079e36b5a06bb5a524a2f5bad6725451613c354cbf64f9a0283f77fe90782d59fcbba6e691658151d6c669300"
    "000000204c69fd467818994e7ff885181e7451d23869a7fd19447dad7a95398077ed7774dcba6e691658151dc9c7ff02"
    "000000208ea9a871b12cd45226ab6df280ec46b8c9ca196b98dc8adc27718cab333e19a50abc6e691658151df6bae902"
    "000000209c182979424a5c85d55d5bddbf3b9421b01b7810edf95ad2529552dd2eec80848ebd6e691658151d439e5c00"
    "00000020cfb9f6d1643613dfdd23a6e1eabadac38c392f985f5dc05b00ef682c428922c96cbe6e691658151d278b4d14"
    "00000020eea983b8d64947368de4f7626531a3134d18f20a8d87462ba011a4f56a53b6ac4fbf6e691658151ddae86408"
    "00000020140bc70b1a3282fbebf9d48ff40a9b73ab7c76ad522799cee73315e9c21aab50dcc06e691658151dedaae90f"
    "000000209b91a056fe089429f2ba4298e764acb4177249ce7d68e6883836f70d1d9218b355c16e691658151d28a43206"
    "00000020b26e80c6a12cc4595556d2318fda564ced5d2a526c7b77f57a9897a56c10539056c16e691658151dc64fb500"
    "00000020369d36495749c96d36260d839abfa703a611aad7b3016342ef05d0412f42221e6ac56e691658151d8f8bfb0f"
    "00"};

// BIP 332 test vector "BCH Fork Headers (18 blocks)": mainnet headers of
// the BCH fork, 000000000000...d43 -> 000000000000...ffa. They meet Bitcoin
// proof of work but belong to blocks that are invalid on Bitcoin.
const std::string BCH_TEST_VECTOR_HEX{
    "432d350741fbf28f2e1486eabe2c4e143bfe2241af6518010000000000000000"
    "12"
    "00000020abaa4bd8a48c1c6bc08ee39b66065e5e9484304cab8b56d5eed3e40b1ac996c899c480593547011822ca4ae8"
    "0000002082afc8ef7eb41a4ecac1fea46983742e491f804ad662e3745ab9c6c4297d8a0862c980593547011840a772cb"
    "0200002058874e50628fdf83aeea4e8cbc7ade946e9ba14bcb1d8ffb28c3daf8ade84df65fca805935470118e2f51003"
    "00000020111b85f9d3b969a1f7ff3d50af08893c500edfc5623b96dbeab6daf16a5164a40ace805935470118c4f4240a"
    "0000002040a045063b551b61d6a1c9db6d3231e2d7403185bbb2332ae1f66db24aac7fa288d8805935470118f15dd76b"
    "0000002070cb14529e8757c359c2e8b1e987f6eee6fbc4472ee9ad4a2e5df6905c19d6d70bed80593547011885ae00d0"
    "000000209653314c1d73e4630bb485fb25ce7a2583cec7c3ccfc27a6d24163be1e9fb19530f4805935470118f17ad2c5"
    "00000020cdf48b8e7ac6bf3a51d1878ee3ff7e6fd0022926dd69cc5cc8d9126e77c4dba809f58059354701188114e836"
    "00000020890cf1dc60edbf0fd4fb667f28ac785849c031d8b24d5e5a0af56ee2bd8a739bf51081593547011812f32a96"
    "00000020ff5244613ad20fdc39b7ee6f4fbc7016432d2dbf45c2a950c59665b39c3954b5b525815935470118aa790d66"
    "00000020aeed520e7c1693de5cfe7531e7d3e73dff7858b09cb6e1ec29229a75c3da2b92453e81593547011830a4314d"
    "000000202e4a4054e64c3f5810c23ec0144d9793aab2d5a7d77d1660eee24d3d55e8b715b543815935470118da1c00e0"
    "0000002073152af68778a98fd984a158aeb29d28e094e23c3a7dff02260c345791e52498c3fb8159354701188d5abed9"
    "0000002022606e744a29f9d4a67ff1fcd2f0e31300ddbd145f8f1db8a68270bfbde77dd88fff8159354701186af175f8"
    "000000209f5db27969fecc0ef71503279069b2df981ba545592a7b425f353b5060e77f3e7e13825935470118da70378e"
    "000000202f0d316b08350f5cd998c6a11762d10adb9f951b5f79ce2a073f8187c05f561f1b1c8259354701184834c623"
    "00000020cf8fc3bad8dad139a3dd6a30481d87e1f760122573168002cc9ef7a58fc53ad387848259354701188a3b54f7"
    "000000200eae92d9b46d81a011a79726a802d4eb195a7af8b70a09b0e115c391968c50d51c8a825935470118cd786d13"
    "00"};

StaleTipMessage DecodeExact(DataStream stream)
{
    StaleTipMessage data;
    stream >> data;
    if (!stream.empty()) throw std::ios_base::failure{"trailing staletip bytes"};
    return data;
}

StaleTipMessage DecodeExact(const std::vector<unsigned char>& payload)
{
    return DecodeExact(DataStream{payload});
}

} // namespace

BOOST_AUTO_TEST_CASE(staletip_testvectors)
{
    struct TestVector {
        std::string hex;
        std::string fork_point;
        size_t header_count;
        std::string tip_hash;
    };
    for (const TestVector& vector : {
             TestVector{SIGNET_TEST_VECTOR_HEX, "00000012602fde2eaf33a90523f42fb07ca854c1d26108782dc4592a80507e1c", 19, "0000000024ff924ff932668d497bba7da9157559a68d9c87d2f28d22e5e4a001"},
             TestVector{BCH_TEST_VECTOR_HEX, "0000000000000000011865af4122fe3b144e2cbeea86142e8ff2fb4107352d43", 18, "000000000000000001416af072f8989829f4c60a1a9658e1cec08411798e4ffa"},
         }) {
        const auto data{DecodeExact(ParseHex(vector.hex))};
        BOOST_CHECK_EQUAL(data.fork_point.ToString(), vector.fork_point);
        BOOST_CHECK_EQUAL(data.headers.size(), vector.header_count);
        BOOST_CHECK(!data.have_block);

        const auto decompressed{data.DecompressHeaders()};
        BOOST_CHECK_EQUAL(decompressed.size(), vector.header_count);
        BOOST_CHECK_EQUAL(decompressed.front().hashPrevBlock.ToString(), vector.fork_point);
        BOOST_CHECK_EQUAL(decompressed.back().GetHash().ToString(), vector.tip_hash);

        DataStream stream;
        stream << data;
        BOOST_CHECK_EQUAL(HexStr(stream), vector.hex);
    }

    const auto signet{DecodeExact(ParseHex(SIGNET_TEST_VECTOR_HEX))};
    BOOST_CHECK_EQUAL(signet.headers.front().version, 536870912);
    BOOST_CHECK_EQUAL(signet.headers.front().merkle_root.ToString(), "a653f83fab2e9028494ccfb91f2da08caddc177bf340adf109e235b23729aa67");
    BOOST_CHECK_EQUAL(signet.headers.front().time, 1768858061U);
    BOOST_CHECK_EQUAL(signet.headers.front().bits, 0x1d155816U);
    BOOST_CHECK_EQUAL(signet.headers.front().nonce, 75214209U);
}

BOOST_AUTO_TEST_CASE(staletip_malformed_payloads)
{
    auto valid{ParseHex(SIGNET_TEST_VECTOR_HEX)};

    auto trailing{valid};
    trailing.push_back(0x00);
    BOOST_CHECK_THROW(DecodeExact(trailing), std::ios_base::failure);

    auto invalid_bool{valid};
    invalid_bool.back() = 0x02;
    BOOST_CHECK_THROW(DecodeExact(invalid_bool), std::ios_base::failure);

    auto truncated{valid};
    truncated.pop_back();
    BOOST_CHECK_THROW(DecodeExact(truncated), std::ios_base::failure);

    std::vector<unsigned char> noncanonical;
    noncanonical.insert(noncanonical.end(), valid.begin(), valid.begin() + 32);
    noncanonical.push_back(0xfd);
    noncanonical.push_back(0x13);
    noncanonical.push_back(0x00);
    noncanonical.insert(noncanonical.end(), valid.begin() + 33, valid.end());
    BOOST_CHECK_THROW(DecodeExact(noncanonical), std::ios_base::failure);

    DataStream empty_headers;
    empty_headers << uint256::ZERO;
    WriteCompactSize(empty_headers, 0);
    empty_headers << uint8_t{0};
    BOOST_CHECK_THROW(DecodeExact(empty_headers), std::ios_base::failure);

    DataStream too_many;
    too_many << uint256::ZERO;
    WriteCompactSize(too_many, MAX_STALETIP_HEADERS + 1);
    for (size_t i{0}; i <= MAX_STALETIP_HEADERS; ++i) too_many << StaleTipCompressedHeader{};
    too_many << uint8_t{0};
    BOOST_CHECK_THROW(DecodeExact(too_many), StaleTipHeadersLimitExceeded);
}

BOOST_AUTO_TEST_CASE(staletip_feature_parse)
{
    const auto parse{[](std::vector<uint8_t> data) { return StaleTipFeature::Parse(std::as_bytes(std::span{data})); }};

    BOOST_CHECK(parse({}) == std::nullopt);
    BOOST_CHECK(parse({0x02}) == std::nullopt);
    BOOST_CHECK(parse({0xff}) == std::nullopt);
    BOOST_CHECK(parse({0x00, 0x00}) == std::nullopt);
    BOOST_CHECK(parse({0x01, 0x00}) == std::nullopt);
    BOOST_CHECK(parse({0x00}) == StaleTipFeature{.prefers_blocks = false});
    BOOST_CHECK(parse({0x01}) == StaleTipFeature{.prefers_blocks = true});

    for (const bool prefers_blocks : {false, true}) {
        const StaleTipFeature feature{.prefers_blocks = prefers_blocks};
        DataStream stream;
        stream << feature;
        BOOST_CHECK_EQUAL(stream.size(), 1U);
        BOOST_CHECK(StaleTipFeature::Parse(stream) == feature);
    }
}

BOOST_AUTO_TEST_SUITE_END()
