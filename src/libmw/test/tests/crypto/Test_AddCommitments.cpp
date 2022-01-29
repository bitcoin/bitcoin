// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/crypto/Pedersen.h>

#include <test_framework/TestMWEB.h>

BOOST_FIXTURE_TEST_SUITE(TestAddCommitments, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(AddCommitment)
{
    // Test adding blinded commitment with transparent one
    {
        BlindingFactor blind_a = BlindingFactor::Random();

        Commitment commit_a = Commitment::Blinded(blind_a, 3);
        Commitment commit_b = Commitment::Transparent(2);

        Commitment sum = Pedersen::AddCommitments(
            std::vector<Commitment>({ commit_a, commit_b }),
            std::vector<Commitment>()
        );
        Commitment expected = Commitment::Blinded(blind_a, 5);

        BOOST_REQUIRE(sum == expected);
    }

    // Test adding 2 blinded commitments
    {
        BlindingFactor blind_a = BlindingFactor::Random();
        BlindingFactor blind_b = BlindingFactor::Random();

        Commitment commit_a = Pedersen::Commit(3, blind_a);
        Commitment commit_b = Pedersen::Commit(2, blind_b);
        Commitment sum = Pedersen::AddCommitments(
            std::vector<Commitment>({ commit_a, commit_b }),
            std::vector<Commitment>()
        );

        BlindingFactor blind_c = Pedersen::AddBlindingFactors({ blind_a, blind_b });
        Commitment commit_c = Pedersen::Commit(5, blind_c);
        BOOST_REQUIRE(commit_c == sum);
    }

    // Test adding negative blinded commitment
    {
        BlindingFactor blind_a = BlindingFactor::Random();
        BlindingFactor blind_b = BlindingFactor::Random();

        Commitment commit_a = Commitment::Blinded(blind_a, 3);
        Commitment commit_b = Commitment::Blinded(blind_b, 2);
        Commitment difference = Pedersen::AddCommitments(
            std::vector<Commitment>({ commit_a }),
            std::vector<Commitment>({ commit_b })
        );

        BlindingFactor blind_c = Pedersen::AddBlindingFactors({blind_a}, {blind_b});
        Commitment commit_c = Commitment::Blinded(blind_c, 1);
        BOOST_REQUIRE(commit_c == difference);
    }
}

BOOST_AUTO_TEST_SUITE_END()