// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSIONBITSINFO_H
#define BITCOIN_VERSIONBITSINFO_H

#include <consensus/params.h>
#include <map>

struct VBDeploymentInfo {
    /** Deployment name */
    const char *name;
    /** Whether GBT clients can safely ignore this rule in simplified usage */
    bool gbt_force;
};

extern const std::map<Consensus::DeploymentPos, VBDeploymentInfo> VersionBitsDeploymentInfo;

#endif // BITCOIN_VERSIONBITSINFO_H
