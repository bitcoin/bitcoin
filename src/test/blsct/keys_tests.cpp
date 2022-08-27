// Copyright (c) 2022 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <blsct/keys.h>

BOOST_FIXTURE_TEST_SUITE(keys_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(keys_constructors)
{
    // Single Public Key
    auto generator = G1Point::GetBasePoint();    

    blsct::PublicKey invalidKey;
    BOOST_CHECK(!invalidKey.IsValid());

    blsct::PublicKey keyFromVch(generator.GetVch());
    BOOST_CHECK(keyFromVch.IsValid());
    BOOST_CHECK(generator.GetVch() == keyFromVch.GetVch());

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

    auto randomPoint = G1Point::Rand();

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
    blsct::DoublePublicKey doubleKeyFromPoints(generator, pointR);
    BOOST_CHECK(doubleKeyFromPoints.IsValid());

    blsct::DoublePublicKey doubleKeyFromVectors(generator.GetVch(), pointR.GetVch());
    BOOST_CHECK(doubleKeyFromVectors.IsValid());

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
}


BOOST_AUTO_TEST_SUITE_END()
