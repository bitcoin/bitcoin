// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/crypto/MuSig.h>
#include <mw/crypto/PublicKeys.h>
#include <mw/crypto/Schnorr.h>

#include <test_framework/TestMWEB.h>

BOOST_FIXTURE_TEST_SUITE(TestAggSig, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(AggSigInteraction)
{
    mw::Hash message = SecretKey::Random().GetBigInt();

    // Generate sender keypairs
    SecretKey secretKeySender = SecretKey::Random();
    PublicKey publicKeySender = PublicKey::From(secretKeySender);
    SecretKey secretNonceSender = MuSig::GenerateSecureNonce();
    PublicKey publicNonceSender = PublicKey::From(secretNonceSender);

    // Generate receiver keypairs
    SecretKey secretKeyReceiver = SecretKey::Random();
    PublicKey publicKeyReceiver = PublicKey::From(secretKeyReceiver);
    SecretKey secretNonceReceiver = MuSig::GenerateSecureNonce();
    PublicKey publicNonceReceiver = PublicKey::From(secretNonceReceiver);

    // Add pubKeys and pubNonces
    PublicKey sumPubKeys = PublicKeys::Add(
        std::vector<PublicKey>({ publicKeySender, publicKeyReceiver })
    );

    PublicKey sumPubNonces = PublicKeys::Add(
        std::vector<PublicKey>({ publicNonceSender, publicNonceReceiver })
    );

    // Generate partial signatures
    CompactSignature senderPartialSignature = MuSig::CalculatePartial(
        secretKeySender,
        secretNonceSender,
        sumPubKeys,
        sumPubNonces,
        message
    );
    CompactSignature receiverPartialSignature = MuSig::CalculatePartial(
        secretKeyReceiver,
        secretNonceReceiver,
        sumPubKeys,
        sumPubNonces,
        message
    );

    // Validate partial signatures
    const bool senderSigValid = MuSig::VerifyPartial(
        senderPartialSignature,
        publicKeySender,
        sumPubKeys,
        sumPubNonces,
        message
    );
    BOOST_REQUIRE(senderSigValid == true);

    const bool receiverSigValid = MuSig::VerifyPartial(
        receiverPartialSignature,
        publicKeyReceiver,
        sumPubKeys,
        sumPubNonces,
        message
    );
    BOOST_REQUIRE(receiverSigValid == true);

    // Aggregate signature and validate
    Signature aggregateSignature = MuSig::Aggregate(
        std::vector<CompactSignature>({ senderPartialSignature, receiverPartialSignature }),
        sumPubNonces
    );
    const bool aggSigValid = Schnorr::Verify(
        aggregateSignature,
        sumPubKeys,
        message
    );
    BOOST_REQUIRE(aggSigValid == true);
}

BOOST_AUTO_TEST_CASE(SingleSchnorrSig)
{
    mw::Hash message = SecretKey::Random().GetBigInt();
    SecretKey secret_key = SecretKey::Random();
    PublicKey public_key = PublicKey::From(secret_key);
    Signature signature = Schnorr::Sign(secret_key.data(), message);

    const bool valid = Schnorr::Verify(signature, public_key, message);
    BOOST_REQUIRE(valid == true);
}

BOOST_AUTO_TEST_SUITE_END()