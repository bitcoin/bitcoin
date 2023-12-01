// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <deploymentinfo.h>

#include <consensus/params.h>

const struct VBDeploymentInfo VersionBitsDeploymentInfo[Consensus::MAX_VERSION_BITS_DEPLOYMENTS] = {
    {
        /*.name =*/ "testdummy",
        /*.gbt_force =*/ true,
    },
    {
        /*.name =*/"v20",
        /*.gbt_force =*/true,
    },
    {
        /*.name =*/"mn_rr",
        /*.gbt_force =*/true,
    },
};

std::string DeploymentName(Consensus::BuriedDeployment dep)
{
    assert(ValidDeployment(dep));
    switch (dep) {
    case Consensus::DEPLOYMENT_HEIGHTINCB:
        return "bip34";
    case Consensus::DEPLOYMENT_CLTV:
        return "bip65";
    case Consensus::DEPLOYMENT_DERSIG:
        return "bip66";
    case Consensus::DEPLOYMENT_BIP147:
        return "bip147";
    case Consensus::DEPLOYMENT_CSV:
        return "csv";
    case Consensus::DEPLOYMENT_DIP0001:
        return "dip0001";
    case Consensus::DEPLOYMENT_DIP0003:
        return "dip0003";
    case Consensus::DEPLOYMENT_DIP0008:
        return "dip0008";
    case Consensus::DEPLOYMENT_DIP0020:
        return "dip0020";
    case Consensus::DEPLOYMENT_DIP0024:
        return "dip0024";
    case Consensus::DEPLOYMENT_BRR:
        return "realloc";
    case Consensus::DEPLOYMENT_V19:
        return "v19";
    } // no default case, so the compiler can warn about missing cases
    return "";
}
