// Copyright (c) 2016-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <deploymentinfo.h>

#include <consensus/params.h>
#include <script/interpreter.h>
#include <tinyformat.h>

#include <string_view>

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

std::optional<Consensus::BuriedDeployment> GetBuriedDeployment(const std::string_view name)
{
    if (name == "segwit") {
        return Consensus::BuriedDeployment::DEPLOYMENT_SEGWIT;
    } else if (name == "bip34") {
        return Consensus::BuriedDeployment::DEPLOYMENT_HEIGHTINCB;
    } else if (name == "dersig") {
        return Consensus::BuriedDeployment::DEPLOYMENT_DERSIG;
    } else if (name == "cltv") {
        return Consensus::BuriedDeployment::DEPLOYMENT_CLTV;
    } else if (name == "csv") {
        return Consensus::BuriedDeployment::DEPLOYMENT_CSV;
    }
    return std::nullopt;
}

#define FLAG_NAME(flag) {std::string(#flag), SCRIPT_VERIFY_##flag}
const std::map<std::string, uint32_t> g_verify_flag_names{
    FLAG_NAME(P2SH),
    FLAG_NAME(STRICTENC),
    FLAG_NAME(DERSIG),
    FLAG_NAME(LOW_S),
    FLAG_NAME(SIGPUSHONLY),
    FLAG_NAME(MINIMALDATA),
    FLAG_NAME(NULLDUMMY),
    FLAG_NAME(DISCOURAGE_UPGRADABLE_NOPS),
    FLAG_NAME(CLEANSTACK),
    FLAG_NAME(MINIMALIF),
    FLAG_NAME(NULLFAIL),
    FLAG_NAME(CHECKLOCKTIMEVERIFY),
    FLAG_NAME(CHECKSEQUENCEVERIFY),
    FLAG_NAME(WITNESS),
    FLAG_NAME(DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM),
    FLAG_NAME(WITNESS_PUBKEYTYPE),
    FLAG_NAME(CONST_SCRIPTCODE),
    FLAG_NAME(TAPROOT),
    FLAG_NAME(DISCOURAGE_UPGRADABLE_PUBKEYTYPE),
    FLAG_NAME(DISCOURAGE_OP_SUCCESS),
    FLAG_NAME(DISCOURAGE_UPGRADABLE_TAPROOT_VERSION),
};
#undef FLAG_NAME

std::vector<std::string> GetScriptFlagNames(uint32_t flags)
{
    std::vector<std::string> res;
    if (flags == 0) return res;

    uint32_t leftover = flags;
    for (const auto& [name, flag] : g_verify_flag_names) {
        if ((flags & flag) != 0) {
            res.push_back(name);
            leftover &= ~flag;
        }
    }
    if (leftover != 0) {
        res.push_back(strprintf("0x%08x", leftover));
    }
    return res;
}
