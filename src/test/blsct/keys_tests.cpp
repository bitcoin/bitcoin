// Copyright (c) 2022 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_UNIT_TEST

#include <test/util/setup_common.h>

#include <blsct/arith/mcl/mcl.h>
#include <blsct/double_public_key.h>
#include <blsct/private_key.h>
#include <blsct/public_key.h>
#include <blsct/public_keys.h>
#include <blsct/signature.h>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(blsct_keys_tests, BasicTestingSetup)

using Point = MclG1Point;
using Scalar = MclScalar;

BOOST_AUTO_TEST_CASE(blsct_keys)
{
    // Single Public Key
    Point generator = Point::GetBasePoint();

    blsct::PublicKey invalidKey{std::vector<unsigned char>{
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
    }};
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

    Point point(generator.GetVch());
    Point keyPointFromPoint;
    Point keyPointFromVector;

    keyPointFromVector = keyFromVch.GetG1Point();
    BOOST_CHECK(keyPointFromVector.IsValid());

    keyPointFromPoint = keyFromPoint.GetG1Point();
    BOOST_CHECK(keyPointFromPoint.IsValid());

    BOOST_CHECK(point == keyPointFromVector);
    BOOST_CHECK(point == keyPointFromPoint);

    Point randomPoint = Point::Rand();

    blsct::PublicKey keyFromVchRandom(randomPoint.GetVch());
    BOOST_CHECK(randomPoint.GetVch() == keyFromVchRandom.GetVch());

    blsct::PublicKey keyFromPointRandom(randomPoint);
    BOOST_CHECK(randomPoint.GetVch() == keyFromPointRandom.GetVch());

    Point pointR(randomPoint.GetVch());
    Point keyPointFromPointRandom;
    Point keyPointFromVectorRandom;

    keyPointFromVectorRandom = keyFromVchRandom.GetG1Point();
    BOOST_CHECK(keyPointFromVectorRandom.IsValid());

    keyPointFromPointRandom = keyFromPointRandom.GetG1Point();
    BOOST_CHECK(keyPointFromPointRandom.IsValid());

    BOOST_CHECK(pointR == keyPointFromVectorRandom);
    BOOST_CHECK(pointR == keyPointFromPointRandom);

    // Double Public Key
    blsct::DoublePublicKey invalidDoublePublicKey{invalidKey, invalidKey};
    BOOST_CHECK(!invalidDoublePublicKey.IsValid());

    blsct::DoublePublicKey doubleKeyFromPoints(generator, pointR);
    BOOST_CHECK(doubleKeyFromPoints.IsValid());

    blsct::DoublePublicKey doubleKeyFromVectors(generator.GetVch(),
                                                pointR.GetVch());
    BOOST_CHECK(doubleKeyFromVectors.IsValid());

    auto genVector = generator.GetVch();
    auto pointRVector = pointR.GetVch();
    genVector.insert(genVector.end(), pointRVector.begin(), pointRVector.end());

    blsct::DoublePublicKey doubleKeyFromConcatenatedVectors(genVector);
    BOOST_CHECK(doubleKeyFromConcatenatedVectors.IsValid());
    BOOST_CHECK(doubleKeyFromConcatenatedVectors == doubleKeyFromVectors);

    std::vector<unsigned char> serializedDoubleKey = doubleKeyFromPoints.GetVch();
    std::vector<unsigned char> serializedViewKey(serializedDoubleKey.begin(), serializedDoubleKey.begin() + blsct::PublicKey::SIZE);
    std::vector<unsigned char> serializedSpendKey(serializedDoubleKey.begin() + blsct::PublicKey::SIZE, serializedDoubleKey.end());
    BOOST_CHECK(serializedViewKey == generator.GetVch());
    BOOST_CHECK(serializedSpendKey == pointR.GetVch());

    Point viewKey;

    auto ret = doubleKeyFromPoints.GetViewKey(viewKey);
    BOOST_CHECK(ret == true);
    BOOST_CHECK(viewKey == generator);

    Point spendKey;

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

    // BOOST_CHECK_THROW(blsct::PrivateKey zeroPrivateKey(Scalar(0)), std::runtime_error);

    {
        MclScalar n(123);
        blsct::PrivateKey pk(n);
        BOOST_CHECK(pk.GetScalar() == n);
    }

    std::vector<unsigned char> vectorKey = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 1, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

    blsct::PrivateKey privateKeyFromVector(vectorKey);
    BOOST_CHECK(privateKeyFromVector.IsValid());
    BOOST_CHECK(privateKeyFromVector.GetScalar().GetVch() == vectorKey);

    blsct::PrivateKey privateKeyFromStream;

    DataStream s;
    s << privateKeyFromVector;
    s >> privateKeyFromStream;

    BOOST_CHECK(privateKeyFromVector == privateKeyFromStream);

    Scalar scalarFromVector(vectorKey);
    blsct::PrivateKey privateKeyFromScalar(scalarFromVector);
    BOOST_CHECK(privateKeyFromScalar.IsValid());
    BOOST_CHECK(privateKeyFromScalar.GetScalar().GetVch() == vectorKey);

    // G(a+b) == Ga + Gb
    blsct::PrivateKey privateKeyFromAddition(privateKeyFromScalar.GetScalar() +
                                             privateKeyFromVector.GetScalar());
    blsct::PublicKey publicKeyFromAddition(privateKeyFromScalar.GetPoint() +
                                           privateKeyFromVector.GetPoint());
    BOOST_CHECK(privateKeyFromAddition.GetPublicKey() == publicKeyFromAddition);

    blsct::PublicKeys vecKeys(std::vector<blsct::PublicKey>{privateKeyFromScalar.GetPublicKey(), privateKeyFromVector.GetPublicKey()});
    BOOST_CHECK(vecKeys.Aggregate() == publicKeyFromAddition);
}

BOOST_AUTO_TEST_CASE(aggretate_empty_public_keys)
{
    blsct::PublicKeys pks(std::vector<blsct::PublicKey>{});
    BOOST_CHECK_THROW(pks.Aggregate(), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(double_public_key_with_bad_input_vector)
{
    {
        // empty input vector
        std::vector<uint8_t> keys;
        blsct::DoublePublicKey dpk(keys);
        BOOST_CHECK(dpk.IsValid() == false);
    }
    {
        // input vector of invalid shorter size
        auto g = MclG1Point::GetBasePoint();
        auto keys = g.GetVch();

        // drop the last element
        keys.pop_back();

        blsct::DoublePublicKey dpk(keys);
        BOOST_CHECK(dpk.IsValid() == false);
    }
    {
        // input vector of invalid larger size
        auto g = MclG1Point::GetBasePoint();
        auto keys = g.GetVch();

        // append an element
        keys.push_back(1);

        blsct::DoublePublicKey dpk(keys);
        BOOST_CHECK(dpk.IsValid() == false);
    }
    {
        // input vector of valid size with bad content
        auto g = MclG1Point::GetBasePoint();
        auto keys = g.GetVch();

        // alter keys[0] from 151 to 152
        keys[0] = keys[0] + 1;

        blsct::DoublePublicKey dpk(keys);
        BOOST_CHECK(dpk.IsValid() == false);
    }
}

BOOST_AUTO_TEST_SUITE_END()
