// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <versionbitsinfo.h>

#include <consensus/params.h>
#include <map>

const std::map<Consensus::DeploymentPos, VBDeploymentInfo> VersionBitsDeploymentInfo = {
    {
        Consensus::DeploymentPos::DEPLOYMENT_TESTDUMMY,
        {
            /*.name =*/ "testdummy",
            /*.gbt_force =*/ true,
        }
    },
    {
        Consensus::DeploymentPos::DEPLOYMENT_TAPROOT,
        {
            /*.name =*/ "taproot",
            /*.gbt_force =*/ true,
        }
    },
};
