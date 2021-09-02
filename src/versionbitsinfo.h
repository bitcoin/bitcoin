// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSIONBITSINFO_H
#define BITCOIN_VERSIONBITSINFO_H

struct VBDeploymentInfo {
    /** Deployment name */
    const char *name;
    /** Whether GBT clients can safely ignore this rule in simplified usage */
    bool gbt_force;
    /** Whether to check current MN protocol or not */
    bool check_mn_protocol;
};

extern const struct VBDeploymentInfo VersionBitsDeploymentInfo[];

#endif // BITCOIN_VERSIONBITSINFO_H
