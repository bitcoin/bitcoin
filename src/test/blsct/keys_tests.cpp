// Copyright (c) 2022 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <test/util/setup_common.h>

#include <blsct/keys.h>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(keys_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(blsct_keys)
{
    // Single Public Key
    G1Point generator = G1Point::GetBasePoint();

    blsct::PublicKey invalidKey;
    BOOST_CHECK(!invalidKey.IsValid());

    blsct::PublicKey keyFromVch(generator.GetVch());
    BOOST_CHECK(keyFromVch.IsValid());
    BOOST_CHECK(generator.GetVch() == keyFromVch.GetVch());
    BOOST_CHECK(keyFromVch.ToString() ==
                "97f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac58"
                "6c55e83ff97a1aeffb3af00adb22c6bb");

    blsct::PublicKey keyFromPoint(generator);
    BOOST_CHECK(keyFromPoint.IsValid());
    BOOST_CHECK(generator.GetVch() == keyFromPoint.GetVch());

    G1Point point(generator.GetVch());
    G1Point keyPointFromPoint;
    G1Point keyPointFromVector;

    bool ret = keyFromVch.GetG1Point(keyPointFromVector);
    BOOST_CHECK(ret == true);

    ret = keyFromPoint.GetG1Point(keyPointFromPoint);
    BOOST_CHECK(ret == true);

    BOOST_CHECK(point == keyPointFromVector);
    BOOST_CHECK(point == keyPointFromPoint);

    G1Point randomPoint = G1Point::Rand();

    blsct::PublicKey keyFromVchRandom(randomPoint.GetVch());
    BOOST_CHECK(randomPoint.GetVch() == keyFromVchRandom.GetVch());

    blsct::PublicKey keyFromPointRandom(randomPoint);
    BOOST_CHECK(randomPoint.GetVch() == keyFromPointRandom.GetVch());

    G1Point pointR(randomPoint.GetVch());
    G1Point keyPointFromPointRandom;
    G1Point keyPointFromVectorRandom;

    ret = keyFromVchRandom.GetG1Point(keyPointFromVectorRandom);
    BOOST_CHECK(ret == true);

    ret = keyFromPointRandom.GetG1Point(keyPointFromPointRandom);
    BOOST_CHECK(ret == true);

    BOOST_CHECK(pointR == keyPointFromVectorRandom);
    BOOST_CHECK(pointR == keyPointFromPointRandom);

    // Double Public Key
    blsct::DoublePublicKey invalidDoublePublicKey;
    BOOST_CHECK(!invalidDoublePublicKey.IsValid());

    blsct::DoublePublicKey doubleKeyFromPoints(generator, pointR);
    BOOST_CHECK(doubleKeyFromPoints.IsValid());

    blsct::DoublePublicKey doubleKeyFromVectors(generator.GetVch(),
                                                pointR.GetVch());
    BOOST_CHECK(doubleKeyFromVectors.IsValid());

    std::vector<unsigned char> serializedDoubleKey = doubleKeyFromPoints.GetVch();
    std::vector<unsigned char> serializedViewKey(serializedDoubleKey.begin(), serializedDoubleKey.begin() + blsct::PublicKey::SIZE);
    std::vector<unsigned char> serializedSpendKey(serializedDoubleKey.begin() + blsct::PublicKey::SIZE, serializedDoubleKey.end());
    BOOST_CHECK(serializedViewKey == generator.GetVch());
    BOOST_CHECK(serializedSpendKey == pointR.GetVch());

    G1Point viewKey;

    ret = doubleKeyFromPoints.GetViewKey(viewKey);
    BOOST_CHECK(ret == true);
    BOOST_CHECK(viewKey == generator);

    G1Point spendKey;

    ret = doubleKeyFromPoints.GetSpendKey(spendKey);
    BOOST_CHECK(ret == true);
    BOOST_CHECK(spendKey == pointR);

    ret = doubleKeyFromVectors.GetViewKey(viewKey);
    BOOST_CHECK(ret == true);
    BOOST_CHECK(viewKey == generator);

    ret = doubleKeyFromVectors.GetSpendKey(spendKey);
    BOOST_CHECK(ret == true);
    BOOST_CHECK(spendKey == pointR);

    // Private Key
    blsct::PrivateKey invalidPrivateKey;
    BOOST_CHECK(!invalidPrivateKey.IsValid());

    std::vector<unsigned char> vectorKey = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 1, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

    blsct::PrivateKey privateKeyFromVector(vectorKey);
    BOOST_CHECK(privateKeyFromVector.IsValid());
    BOOST_CHECK(privateKeyFromVector.GetScalar().GetVch() == vectorKey);

    Scalar scalarFromVector(vectorKey);
    blsct::PrivateKey privateKeyFromScalar(scalarFromVector);
    BOOST_CHECK(privateKeyFromScalar.IsValid());
    BOOST_CHECK(privateKeyFromScalar.GetScalar().GetVch() == vectorKey);

    // G(a+b) == Ga + Gb
    blsct::PrivateKey privateKeyFromAddition(privateKeyFromScalar.GetScalar() +
                                             privateKeyFromVector.GetScalar());
    blsct::PublicKey publicKeyFromAddition(privateKeyFromScalar.GetG1Point() +
                                           privateKeyFromVector.GetG1Point());
    BOOST_CHECK(privateKeyFromAddition.GetPublicKey() == publicKeyFromAddition);

    std::vector<blsct::PublicKey> vecKeys = {privateKeyFromScalar.GetPublicKey(), privateKeyFromVector.GetPublicKey()};
    BOOST_CHECK(blsct::PublicKey::Aggregate(vecKeys) == publicKeyFromAddition);
}

BOOST_AUTO_TEST_SUITE_END()
