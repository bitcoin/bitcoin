// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <deploymentinfo.h>

#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(deploymentinfo_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(name_from_pos)
{
    // Check deployment names from Consensus::DeploymentPos
    BOOST_CHECK(DeploymentName(Consensus::DEPLOYMENT_TESTDUMMY) == VersionBitsDeploymentInfo[Consensus::DEPLOYMENT_TESTDUMMY].name);
    BOOST_CHECK(DeploymentName(Consensus::DEPLOYMENT_TAPROOT) == VersionBitsDeploymentInfo[Consensus::DEPLOYMENT_TAPROOT].name);
}

BOOST_AUTO_TEST_CASE(name_from_dep)
{
    // Check deployment names from Consensus::BuriedDeployment
    BOOST_CHECK(DeploymentName(Consensus::DEPLOYMENT_HEIGHTINCB) == "bip34");
    BOOST_CHECK(DeploymentName(Consensus::DEPLOYMENT_CLTV) == "bip65");
    BOOST_CHECK(DeploymentName(Consensus::DEPLOYMENT_DERSIG) == "bip66");
    BOOST_CHECK(DeploymentName(Consensus::DEPLOYMENT_CSV) == "csv");
    BOOST_CHECK(DeploymentName(Consensus::DEPLOYMENT_SEGWIT) == "segwit");
}

BOOST_AUTO_TEST_SUITE_END()
