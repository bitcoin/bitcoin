// Copyright (c) 2011-2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <boost/test/unit_test.hpp>

#include <util/strencodings.h>

using namespace std::literals;

BOOST_AUTO_TEST_SUITE(chainparams_tests)

struct ParseWrappedSignetChallenge_TestCase
{
        std::string wrappedChallengeHex;
        std::string wantParamsHex;
        std::string wantChallengeHex;
        std::string wantError;
};

BOOST_AUTO_TEST_CASE(parse_wrapped_signet_challenge)
{
    static const ParseWrappedSignetChallenge_TestCase cases[] = {
            {
                    "512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae",
                    "",
                    "512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae",
                    "",
            },
            {
                    "6a4c09011e000000000000004c25512102f7561d208dd9ae99bf497273e16f389bdbd6c4742ddb8e6b216e64fa2928ad8f51ae",
                    "011e00000000000000",
                    "512102f7561d208dd9ae99bf497273e16f389bdbd6c4742ddb8e6b216e64fa2928ad8f51ae",
                    "",
            },
            {
                    "6a4c004c25512102f7561d208dd9ae99bf497273e16f389bdbd6c4742ddb8e6b216e64fa2928ad8f51ae",
                    "",
                    "512102f7561d208dd9ae99bf497273e16f389bdbd6c4742ddb8e6b216e64fa2928ad8f51ae",
                    "",
            },
            {
                    "6a4c004c25512102f7561d208dd9ae99bf497273e16f389bdbd6c4742ddb8e6b216e64fa2928ad8f51ae4c00",
                    "",
                    "",
                    "too many operations in wrapped challenge, must be 3.",
            },
            {
                    "6a4c09011e00000000000000",
                    "",
                    "",
                    "too few operations in wrapped challenge, must be 3, got 2.",
            },
            {
                    "6a4c01",
                    "",
                    "",
                    "failed to parse operation 1 in wrapped challenge script.",
            },
            {
                    "6a4c004c25512102f7561d208dd9ae99bf497273",
                    "",
                    "",
                    "failed to parse operation 2 in wrapped challenge script.",
            },
            {
                    "6a6a4c25512102f7561d208dd9ae99bf497273e16f389bdbd6c4742ddb8e6b216e64fa2928ad8f51ae4c00",
                    "",
                    "",
                    "operation 1 of wrapped challenge script must be a PUSHDATA opcode, got 0x6a.",
            },
            {
                    "6a4c09011e0000000000000051",
                    "",
                    "",
                    "operation 2 of wrapped challenge script must be a PUSHDATA opcode, got 0x51.",
            },
    };

    for (unsigned int i=0; i<std::size(cases); i++)
    {
        const auto wrappedChallenge = ParseHex(cases[i].wrappedChallengeHex);
        const auto wantParamsHex = cases[i].wantParamsHex;
        const auto wantChallengeHex = cases[i].wantChallengeHex;
        const auto wantError = cases[i].wantError;

        std::vector<uint8_t> gotParams;
        std::vector<uint8_t> gotChallenge;
        std::string gotError;
        try {
            ParseWrappedSignetChallenge(wrappedChallenge, gotParams, gotChallenge);
        } catch (const std::exception& e) {
            gotError = e.what();
        }

        BOOST_CHECK_EQUAL(HexStr(gotParams), wantParamsHex);
        BOOST_CHECK_EQUAL(HexStr(gotChallenge), wantChallengeHex);
        BOOST_CHECK_EQUAL(gotError, wantError);
    }
}

struct ParseSignetParams_TestCase
{
        std::string paramsHex;
        int64_t wantPowTargetSpacing;
        std::string wantError;
};

BOOST_AUTO_TEST_CASE(parse_signet_params)
{
    static const ParseSignetParams_TestCase cases[] = {
            {
                    "",
                    600,
                    "",
            },
            {
                    "011e00000000000000",
                    30,
                    "",
            },
            {
                    "01e803000000000000",
                    1000,
                    "",
            },
            {
                    "015802000000000000",
                    600,
                    "",
            },
            {
                    "012502",
                    600,
                    "signet params must have length 9, got 3.",
            },
            {
                    "022502000000000000",
                    600,
                    "signet params[0] must be 0x01, got 0x02.",
            },
            {
                    "01ffffffffffffffff",
                    600,
                    "signet param pow_target_spacing <= 0.",
            },
    };

    for (unsigned int i=0; i<std::size(cases); i++)
    {
        const auto params = ParseHex(cases[i].paramsHex);
        const auto wantPowTargetSpacing = cases[i].wantPowTargetSpacing;
        const auto wantError = cases[i].wantError;

        CChainParams::SigNetOptions gotOptions;
        std::string gotError;
        try {
            ParseSignetParams(params, gotOptions);
        } catch (const std::exception& e) {
            gotError = e.what();
        }

        BOOST_CHECK_EQUAL(gotOptions.pow_target_spacing, wantPowTargetSpacing);
        BOOST_CHECK_EQUAL(gotError, wantError);
    }
}

BOOST_AUTO_TEST_SUITE_END()
