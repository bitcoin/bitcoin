// Copyright (c) 2016-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <deploymentinfo.h>

#include <consensus/params.h>
#include <script/interpreter.h>

const struct VBDeploymentInfo VersionBitsDeploymentInfo[Consensus::MAX_VERSION_BITS_DEPLOYMENTS] = {
    {
        /*.name =*/ "testdummy",
        /*.gbt_force =*/ true,
    },
};

std::string DeploymentName(Consensus::BuriedDeployment dep)
{
    assert(ValidDeployment(dep));
    switch (dep) {
    case Consensus::DEPLOYMENT_HEIGHTINCB:
        return "bip34";
    case Consensus::DEPLOYMENT_P2SH:
        return "bip16";
    case Consensus::DEPLOYMENT_CLTV:
        return "bip65";
    case Consensus::DEPLOYMENT_DERSIG:
        return "bip66";
    case Consensus::DEPLOYMENT_CSV:
        return "csv";
    case Consensus::DEPLOYMENT_SEGWIT:
        return "segwit";
    case Consensus::DEPLOYMENT_TAPROOT:
        return "taproot";
    } // no default case, so the compiler can warn about missing cases
    return "";
}

bool BuriedException(Consensus::BuriedDeployment dep, uint32_t flags)
{
    switch (dep) {
    case Consensus::DEPLOYMENT_HEIGHTINCB:
    case Consensus::DEPLOYMENT_CLTV:
    case Consensus::DEPLOYMENT_DERSIG:
    case Consensus::DEPLOYMENT_CSV:
        // Because the BIP16 exception drops validation to SCRIPT_VERIFY_NONE,
        // technically all the above soft forks have this exception block. But
        // for the purpose of the getdeploymentinfo RPC this is omitted.
        break;
    case Consensus::DEPLOYMENT_P2SH:
    case Consensus::DEPLOYMENT_SEGWIT:
        return (flags & SCRIPT_VERIFY_P2SH) == 0;
    case Consensus::DEPLOYMENT_TAPROOT:
        return (flags & SCRIPT_VERIFY_TAPROOT) == 0;
    } // no default case, so the compiler can warn about missing cases
    return false;
}
