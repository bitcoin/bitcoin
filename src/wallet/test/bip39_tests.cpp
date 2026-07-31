// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <key_io.h>
#include <test/util/setup_common.h>
#include <wallet/bip39.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace wallet::bip39 {
namespace {

struct TestVector {
    std::string_view mnemonic;
    std::string_view master_xprv;
};

// Official BIP39 vectors from trezor/python-mnemonic commit
// b57a5ad77a981e743f4167ab2f7927a55c1e82a8.
// SHA256(vectors.json): fa3b937b7cff9c9b8ecd3aa011faeb8d6dd67993174b72326e83f4de8fdb30f8.
constexpr std::array<TestVector, 24> OFFICIAL_VECTORS{{
    {
        "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",
        "xprv9s21ZrQH143K3h3fDYiay8mocZ3afhfULfb5GX8kCBdno77K4HiA15Tg23wpbeF1pLfs1c5SPmYHrEpTuuRhxMwvKDwqdKiGJS9XFKzUsAF",
    },
    {
        "legal winner thank year wave sausage worth useful legal winner thank yellow",
        "xprv9s21ZrQH143K2gA81bYFHqU68xz1cX2APaSq5tt6MFSLeXnCKV1RVUJt9FWNTbrrryem4ZckN8k4Ls1H6nwdvDTvnV7zEXs2HgPezuVccsq",
    },
    {
        "letter advice cage absurd amount doctor acoustic avoid letter advice cage above",
        "xprv9s21ZrQH143K2shfP28KM3nr5Ap1SXjz8gc2rAqqMEynmjt6o1qboCDpxckqXavCwdnYds6yBHZGKHv7ef2eTXy461PXUjBFQg6PrwY4Gzq",
    },
    {
        "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong",
        "xprv9s21ZrQH143K2V4oox4M8Zmhi2Fjx5XK4Lf7GKRvPSgydU3mjZuKGCTg7UPiBUD7ydVPvSLtg9hjp7MQTYsW67rZHAXeccqYqrsx8LcXnyd",
    },
    {
        "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon agent",
        "xprv9s21ZrQH143K3mEDrypcZ2usWqFgzKB6jBBx9B6GfC7fu26X6hPRzVjzkqkPvDqp6g5eypdk6cyhGnBngbjeHTe4LsuLG1cCmKJka5SMkmU",
    },
    {
        "legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal will",
        "xprv9s21ZrQH143K3Lv9MZLj16np5GzLe7tDKQfVusBni7toqJGcnKRtHSxUwbKUyUWiwpK55g1DUSsw76TF1T93VT4gz4wt5RM23pkaQLnvBh7",
    },
    {
        "letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter always",
        "xprv9s21ZrQH143K3VPCbxbUtpkh9pRG371UCLDz3BjceqP1jz7XZsQ5EnNkYAEkfeZp62cDNj13ZTEVG1TEro9sZ9grfRmcYWLBhCocViKEJae",
    },
    {
        "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo when",
        "xprv9s21ZrQH143K36Ao5jHRVhFGDbLP6FCx8BEEmpru77ef3bmA928BxsqvVM27WnvvyfWywiFN8K6yToqMaGYfzS6Db1EHAXT5TuyCLBXUfdm",
    },
    {
        "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon art",
        "xprv9s21ZrQH143K32qBagUJAMU2LsHg3ka7jqMcV98Y7gVeVyNStwYS3U7yVVoDZ4btbRNf4h6ibWpY22iRmXq35qgLs79f312g2kj5539ebPM",
    },
    {
        "legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth title",
        "xprv9s21ZrQH143K3Y1sd2XVu9wtqxJRvybCfAetjUrMMco6r3v9qZTBeXiBZkS8JxWbcGJZyio8TrZtm6pkbzG8SYt1sxwNLh3Wx7to5pgiVFU",
    },
    {
        "letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic bless",
        "xprv9s21ZrQH143K3CSnQNYC3MqAAqHwxeTLhDbhF43A4ss4ciWNmCY9zQGvAKUSqVUf2vPHBTSE1rB2pg4avopqSiLVzXEU8KziNnVPauTqLRo",
    },
    {
        "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo vote",
        "xprv9s21ZrQH143K2WFF16X85T2QCpndrGwx6GueB72Zf3AHwHJaknRXNF37ZmDrtHrrLSHvbuRejXcnYxoZKvRquTPyp2JiNG3XcjQyzSEgqCB",
    },
    {
        "ozone drill grab fiber curtain grace pudding thank cruise elder eight picnic",
        "xprv9s21ZrQH143K2oZ9stBYpoaZ2ktHj7jLz7iMqpgg1En8kKFTXJHsjxry1JbKH19YrDTicVwKPehFKTbmaxgVEc5TpHdS1aYhB2s9aFJBeJH",
    },
    {
        "gravity machine north sort system female filter attitude volume fold club stay feature office ecology stable narrow fog",
        "xprv9s21ZrQH143K3uT8eQowUjsxrmsA9YUuQQK1RLqFufzybxD6DH6gPY7NjJ5G3EPHjsWDrs9iivSbmvjc9DQJbJGatfa9pv4MZ3wjr8qWPAK",
    },
    {
        "hamster diagram private dutch cause delay private meat slide toddler razor book happy fancy gospel tennis maple dilemma loan word shrug inflict delay length",
        "xprv9s21ZrQH143K2XTAhys3pMNcGn261Fi5Ta2Pw8PwaVPhg3D8DWkzWQwjTJfskj8ofb81i9NP2cUNKxwjueJHHMQAnxtivTA75uUFqPFeWzk",
    },
    {
        "scheme spot photo card baby mountain device kick cradle pact join borrow",
        "xprv9s21ZrQH143K3FperxDp8vFsFycKCRcJGAFmcV7umQmcnMZaLtZRt13QJDsoS5F6oYT6BB4sS6zmTmyQAEkJKxJ7yByDNtRe5asP2jFGhT6",
    },
    {
        "horn tenant knee talent sponsor spell gate clip pulse soap slush warm silver nephew swap uncle crack brave",
        "xprv9s21ZrQH143K3R1SfVZZLtVbXEB9ryVxmVtVMsMwmEyEvgXN6Q84LKkLRmf4ST6QrLeBm3jQsb9gx1uo23TS7vo3vAkZGZz71uuLCcywUkt",
    },
    {
        "panda eyebrow bullet gorilla call smoke muffin taste mesh discover soft ostrich alcohol speed nation flash devote level hobby quick inner drive ghost inside",
        "xprv9s21ZrQH143K2WNnKmssvZYM96VAr47iHUQUTUyUXH3sAGNjhJANddnhw3i3y3pBbRAVk5M5qUGFr4rHbEWwXgX4qrvrceifCYQJbbFDems",
    },
    {
        "cat swing flag economy stadium alone churn speed unique patch report train",
        "xprv9s21ZrQH143K4G28omGMogEoYgDQuigBo8AFHAGDaJdqQ99QKMQ5J6fYTMfANTJy6xBmhvsNZ1CJzRZ64PWbnTFUn6CDV2FxoMDLXdk95DQ",
    },
    {
        "light rule cinnamon wrap drastic word pride squirrel upgrade then income fatal apart sustain crack supply proud access",
        "xprv9s21ZrQH143K3wtsvY8L2aZyxkiWULZH4vyQE5XkHTXkmx8gHo6RUEfH3Jyr6NwkJhvano7Xb2o6UqFKWHVo5scE31SGDCAUsgVhiUuUDyh",
    },
    {
        "all hour make first leader extend hole alien behind guard gospel lava path output census museum junior mass reopen famous sing advance salt reform",
        "xprv9s21ZrQH143K3rEfqSM4QZRVmiMuSWY9wugscmaCjYja3SbUD3KPEB1a7QXJoajyR2T1SiXU7rFVRXMV9XdYVSZe7JoUXdP4SRHTxsT1nzm",
    },
    {
        "vessel ladder alter error federal sibling chat ability sun glass valve picture",
        "xprv9s21ZrQH143K2QWV9Wn8Vvs6jbqfF1YbTCdURQW9dLFKDovpKaKrqS3SEWsXCu6ZNky9PSAENg6c9AQYHcg4PjopRGGKmdD313ZHszymnps",
    },
    {
        "scissors invite lock maple supreme raw rapid void congress muscle digital elegant little brisk hair mango congress clump",
        "xprv9s21ZrQH143K4aERa2bq7559eMCCEs2QmmqVjUuzfy5eAeDX4mqZffkYwpzGQRE2YEEeLVRoH4CSHxianrFaVnMN2RYaPUZJhJx8S5j6puX",
    },
    {
        "void come effort suffer camp survey warrior heavy shoot primary clutch crush open amazing screen patrol group space point ten exist slush involve unfold",
        "xprv9s21ZrQH143K39rnQJknpH1WEPFJrzmAqqasiDcVrNuk926oizzJDDQkdiTvNPr2FYDYzWgiMiC63YmfPAa2oPyNB23r2g7d1yiK6WpqaQS",
    },
}};

std::string RepeatedWord(size_t count)
{
    std::string mnemonic;
    for (size_t i{0}; i < count; ++i) {
        if (i > 0) mnemonic += ' ';
        mnemonic += "abandon";
    }
    return mnemonic;
}

void CheckError(std::string_view mnemonic, std::string_view passphrase, Error expected)
{
    const auto result{DecodeMnemonic(mnemonic, passphrase)};
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error() == expected);
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(bip39_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(official_vectors)
{
    for (const auto& vector : OFFICIAL_VECTORS) {
        const auto key{DecodeMnemonic(vector.mnemonic, "TREZOR")};
        BOOST_REQUIRE(key);
        BOOST_CHECK_EQUAL(EncodeExtKey(*key), vector.master_xprv);
    }
}

BOOST_AUTO_TEST_CASE(additional_valid_word_counts)
{
    constexpr std::array<TestVector, 2> vectors{{
        {
            "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon address",
            "xprv9s21ZrQH143K2nE8x21M7hF4msPN7WC7b6yCn1usVKBA9myCGie4yQCqsw479qmbjXeeRZgMjnNwvWweSCbBeUSPADo2aZbfpkTvJ8de1Qi",
        },
        {
            "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon admit",
            "xprv9s21ZrQH143K3CXWs34yKD87zsm1AVExnEEUnH8rKWqWVguXRprSz8bFFZDbKY7oai19nPTZ68HnW8xX62HFr8WxsNL4CEYmGLcTxAyKNHd",
        },
    }};

    for (const auto& vector : vectors) {
        const auto key{DecodeMnemonic(vector.mnemonic, "TREZOR")};
        BOOST_REQUIRE(key);
        BOOST_CHECK_EQUAL(EncodeExtKey(*key), vector.master_xprv);
    }
}

BOOST_AUTO_TEST_CASE(invalid_word_counts)
{
    constexpr std::array<size_t, 12> invalid_counts{0, 1, 11, 13, 14, 16, 17, 19, 20, 22, 23, 25};
    for (const size_t count : invalid_counts) {
        CheckError(RepeatedWord(count), "", Error::InvalidWordCount);
    }
}

BOOST_AUTO_TEST_CASE(unknown_words)
{
    CheckError(
        "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon nope",
        "",
        Error::UnknownWord);
    CheckError(
        "Abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",
        "",
        Error::UnknownWord);
}

BOOST_AUTO_TEST_CASE(invalid_checksum)
{
    CheckError(RepeatedWord(12), "", Error::InvalidChecksum);
    // 24 words is the only count with a full-byte checksum (checksum_bits == 8).
    CheckError(RepeatedWord(24), "", Error::InvalidChecksum);
}

BOOST_AUTO_TEST_CASE(mnemonic_whitespace_is_normalized)
{
    const auto key{DecodeMnemonic(
        "\t abandon  abandon\nabandon\rabandon\fabandon\vabandon abandon abandon abandon abandon abandon about \r\n",
        "TREZOR")};
    BOOST_REQUIRE(key);
    BOOST_CHECK_EQUAL(EncodeExtKey(*key), OFFICIAL_VECTORS[0].master_xprv);
}

BOOST_AUTO_TEST_CASE(ascii_passphrase_is_verbatim)
{
    const auto baseline{DecodeMnemonic(OFFICIAL_VECTORS[0].mnemonic, "TREZOR")};
    const auto empty{DecodeMnemonic(OFFICIAL_VECTORS[0].mnemonic, "")};
    const auto with_spaces{DecodeMnemonic(OFFICIAL_VECTORS[0].mnemonic, " TREZOR ")};
    const std::string nul_passphrase{"TREZOR\0suffix", 13};
    const auto with_nul{DecodeMnemonic(OFFICIAL_VECTORS[0].mnemonic, nul_passphrase)};

    BOOST_REQUIRE(baseline);
    BOOST_REQUIRE(empty);
    BOOST_REQUIRE(with_spaces);
    BOOST_REQUIRE(with_nul);
    BOOST_CHECK_EQUAL(
        EncodeExtKey(*empty),
        "xprv9s21ZrQH143K3GJpoapnV8SFfukcVBSfeCficPSGfubmSFDxo1kuHnLisriDvSnRRuL2Qrg5ggqHKNVpxR86QEC8w35uxmGoggxtQTPvfUu");
    BOOST_CHECK(EncodeExtKey(*with_spaces) != EncodeExtKey(*baseline));
    BOOST_CHECK(EncodeExtKey(*with_nul) != EncodeExtKey(*baseline));
}

BOOST_AUTO_TEST_CASE(non_ascii_passphrase_is_rejected)
{
    const std::string high_byte{static_cast<char>(0x80)};
    CheckError(OFFICIAL_VECTORS[0].mnemonic, high_byte, Error::InvalidPassphrase);
    CheckError(OFFICIAL_VECTORS[0].mnemonic, "\xc3\xa9", Error::InvalidPassphrase);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace wallet::bip39
