// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script_flags.h>

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

static const std::map<std::string, CScriptFlag> mapFlagNames = {
    {std::string("NONE"), SCRIPT_VERIFY_NONE},
    {std::string("P2SH"), SCRIPT_VERIFY_P2SH},
    {std::string("STRICTENC"), SCRIPT_VERIFY_STRICTENC},
    {std::string("DERSIG"), SCRIPT_VERIFY_DERSIG},
    {std::string("LOW_S"), SCRIPT_VERIFY_LOW_S},
    {std::string("SIGPUSHONLY"), SCRIPT_VERIFY_SIGPUSHONLY},
    {std::string("MINIMALDATA"), SCRIPT_VERIFY_MINIMALDATA},
    {std::string("NULLDUMMY"), SCRIPT_VERIFY_NULLDUMMY},
    {std::string("DISCOURAGE_UPGRADABLE_NOPS"), SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS},
    {std::string("CLEANSTACK"), SCRIPT_VERIFY_CLEANSTACK},
    {std::string("MINIMALIF"), SCRIPT_VERIFY_MINIMALIF},
    {std::string("NULLFAIL"), SCRIPT_VERIFY_NULLFAIL},
    {std::string("CHECKLOCKTIMEVERIFY"), SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY},
    {std::string("CHECKSEQUENCEVERIFY"), SCRIPT_VERIFY_CHECKSEQUENCEVERIFY},
    {std::string("WITNESS"), SCRIPT_VERIFY_WITNESS},
    {std::string("DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM"), SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM},
    {std::string("WITNESS_PUBKEYTYPE"), SCRIPT_VERIFY_WITNESS_PUBKEYTYPE},
};

CScriptFlag ParseScriptFlag(const std::string flag_name)
{
    const auto it = mapFlagNames.find(flag_name);
    if (it == mapFlagNames.end()) {
        throw std::runtime_error(std::string(__func__) + ": unknown verification flag '" + flag_name + "'");
    }
    return it->second;
}

std::vector<std::string> ScriptFlagsToStrings(unsigned int flags)
{
    std::vector<std::string> ret;
    if (flags == 0) {
        return ret;
    }
    auto it = mapFlagNames.begin();
    while (flags && it != mapFlagNames.end()) {
        if (flags & it->second) {
            ret.push_back(it->first);
        }
        ++it;
    }
    return ret;
}
