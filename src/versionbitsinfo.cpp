// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <versionbitsinfo.h>

#include <consensus/params.h>

const struct VBDeploymentInfo VersionBitsDeploymentInfo[Consensus::MAX_VERSION_BITS_DEPLOYMENTS] = {
    {
        /*.name =*/ "testdummy",
        /*.gbt_force =*/ true,
        /*.check_mn_protocol =*/ false,
    },
    {
        /*.name =*/ "csv",
        /*.gbt_force =*/ true,
        /*.check_mn_protocol =*/ false,
    },
    {
        /*.name =*/ "dip0001",
        /*.gbt_force =*/ true,
        /*.check_mn_protocol =*/ true,
    },
    {
        /*.name =*/ "bip147",
        /*.gbt_force =*/ true,
        /*.check_mn_protocol =*/ false,
    },
    {
        /*.name =*/ "dip0003",
        /*.gbt_force =*/ true,
        /*.check_mn_protocol =*/ false,
    },
    {
        /*.name =*/ "dip0008",
        /*.gbt_force =*/ true,
        /*.check_mn_protocol =*/ false,
    },
    {
        /*.name =*/ "realloc",
        /*.gbt_force =*/ true,
        /*.check_mn_protocol =*/ false,
    },
    {
        /*.name =*/ "dip0020",
        /*.gbt_force =*/ true,
        /*.check_mn_protocol =*/ false,
    },
    {
        /*.name =*/"dip0024",
        /*.gbt_force =*/true,
        /*.check_mn_protocol =*/false,
    }
};
