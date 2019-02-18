// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <versionbitsinfo.h>

#include <consensus/params.h>

const struct VBDeploymentInfo VersionBitsDeploymentInfo[] = {
    {
        /*.name =*/ "testdummy",
        /*.gbt_force =*/ true,
    },
    {
        /*.name =*/ "strictder",
        /*.gbt_force =*/ true,
    },
    {
        /*.name =*/ "cltv",
        /*.gbt_force =*/ true,
    },
    {
        /*.name =*/ "csv",
        /*.gbt_force =*/ true,
    },
    {
        /*.name =*/ "segwit",
        /*.gbt_force =*/ true,
    }
};
static_assert(sizeof(VersionBitsDeploymentInfo) == Consensus::MAX_VERSION_BITS_DEPLOYMENTS * sizeof(VersionBitsDeploymentInfo[0]), "Must specify names for all deployments");
