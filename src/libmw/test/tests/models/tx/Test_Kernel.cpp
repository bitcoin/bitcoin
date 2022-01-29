// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/crypto/Schnorr.h>
#include <mw/models/tx/Kernel.h>

#include <test_framework/TestMWEB.h>

BOOST_FIXTURE_TEST_SUITE(TestKernel, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(PlainKernel_Test)
{
    CAmount fee = 1000;
    BlindingFactor excess_blind = BlindingFactor::Random();
    Kernel kernel = Kernel::Create(excess_blind, boost::none, fee, boost::none, std::vector<PegOutCoin>{}, boost::none);

    //
    // Serialization
    //
    {
        std::vector<uint8_t> serialized = kernel.Serialized();

        Kernel kernel2;
        CDataStream(serialized, SER_DISK, 0) >> kernel2;
        BOOST_REQUIRE(kernel == kernel2);
    }

    //
    // Signature Message
    //
    {
        Hasher hasher;
        hasher << uint8_t(1);
        hasher << Commitment::Blinded(excess_blind, 0);
        ::WriteVarInt<Hasher, VarIntMode::NONNEGATIVE_SIGNED, CAmount>(hasher, fee);
        BOOST_REQUIRE(kernel.BuildSignedMsg().GetMsgHash() == hasher.hash());
    }

    //
    // Getters
    //
    {
        BOOST_REQUIRE(!kernel.HasPegIn());
        BOOST_REQUIRE(!kernel.HasPegOut());
        BOOST_REQUIRE(!kernel.HasStealthExcess());
        BOOST_REQUIRE(kernel.GetPegIn() == 0);
        BOOST_REQUIRE(kernel.GetPegOuts().empty());
        BOOST_REQUIRE(kernel.GetLockHeight() == 0);
        BOOST_REQUIRE(kernel.GetFee() == fee);
        BOOST_REQUIRE(kernel.GetCommitment() == Commitment::Blinded(excess_blind, 0));
        BOOST_REQUIRE(kernel.GetSignature() == Schnorr::Sign(excess_blind.data(), kernel.BuildSignedMsg().GetMsgHash()));
    }
}

BOOST_AUTO_TEST_CASE(NonStandardKernel_Test)
{
    CAmount fee = 1000;
    CAmount pegin = 10000;
    std::vector<uint8_t> rand1 = secret_key_t<30>::Random().vec();
    PegOutCoin pegout(2000, CScript(rand1.data(), rand1.data() + rand1.size()));
    int32_t lock_height = 123456;
    std::vector<uint8_t> rand2 = secret_key_t<30>::Random().vec();
    Kernel standard_kernel(
        Kernel::FEE_FEATURE_BIT | Kernel::PEGIN_FEATURE_BIT | Kernel::PEGOUT_FEATURE_BIT | Kernel::HEIGHT_LOCK_FEATURE_BIT | Kernel::STEALTH_EXCESS_FEATURE_BIT,
        fee,
        pegin,
        std::vector<PegOutCoin>{PegOutCoin(2000, CScript(rand2.data(), rand2.data() + rand2.size()))},
        lock_height,
        PublicKey::Random(),
        std::vector<uint8_t>{},
        Commitment::Random(),
        Signature(SecretKey64::Random().GetBigInt())
    );
    std::vector<uint8_t> rand3 = secret_key_t<30>::Random().vec();
    Kernel nonstandard_kernel1(
        Kernel::ALL_FEATURE_BITS,
        fee,
        pegin,
        std::vector<PegOutCoin>{PegOutCoin(2000, CScript(rand3.data(), rand3.data() + rand3.size()))},
        lock_height,
        PublicKey::Random(),
        secret_key_t<20>::Random().vec(),
        standard_kernel.GetCommitment(),
        standard_kernel.GetSignature()
    );

    Kernel nonstandard_kernel2(
        0x20,
        boost::none,
        boost::none,
        std::vector<PegOutCoin>{},
        boost::none,
        PublicKey::Random(),
        std::vector<uint8_t>{},
        standard_kernel.GetCommitment(),
        standard_kernel.GetSignature()
    );

    BOOST_REQUIRE(standard_kernel.IsStandard());
    BOOST_REQUIRE(!nonstandard_kernel1.IsStandard());
    BOOST_REQUIRE(!nonstandard_kernel2.IsStandard());
}

BOOST_AUTO_TEST_SUITE_END()