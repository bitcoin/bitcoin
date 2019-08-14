// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_permissions.h>
#include <util/system.h>
#include <netbase.h>

// The parse the following format "perm1,perm2@xxxxxx"
bool TryParsePermissionFlags(const std::string str, NetPermissionFlags& output, size_t& readen, std::string& error)
{
    NetPermissionFlags flags = PF_NONE;
    const auto atSeparator = str.find('@');

    // if '@' is not found (ie, "xxxxx"), the caller should apply implicit permissions
    if (atSeparator == std::string::npos) {
        NetPermissions::AddFlag(flags, PF_ISIMPLICIT);
        readen = 0;
    }
    // else (ie, "perm1,perm2@xxxxx"), let's enumerate the permissions by splitting by ',' and calculate the flags
    else {
        readen = 0;
        // permissions == perm1,perm2
        const auto permissions = str.substr(0, atSeparator);
        while (readen < permissions.length()) {
            const auto commaSeparator = permissions.find(',', readen);
            const auto len = commaSeparator == std::string::npos ? permissions.length() - readen : commaSeparator - readen;
            // permission == perm1
            const auto permission = permissions.substr(readen, len);
            readen += len; // We read "perm1"
            if (commaSeparator != std::string::npos) readen++; // We read ","

            if (permission == "bloomfilter" || permission == "bloom") NetPermissions::AddFlag(flags, PF_BLOOMFILTER);
            else if (permission == "noban") NetPermissions::AddFlag(flags, PF_NOBAN);
            else if (permission == "forcerelay") NetPermissions::AddFlag(flags, PF_FORCERELAY);
            else if (permission == "mempool") NetPermissions::AddFlag(flags, PF_MEMPOOL);
            else if (permission == "all") NetPermissions::AddFlag(flags, PF_ALL);
            else if (permission == "relay") NetPermissions::AddFlag(flags, PF_RELAY);
            else if (permission.length() == 0); // Allow empty entries
            else {
                error = strprintf(_("Invalid P2P permission: '%s'"), permission);
                return false;
            }
        }
        readen++;
    }

    output = flags;
    error = "";
    return true;
}

std::vector<std::string> NetPermissions::ToStrings(NetPermissionFlags flags)
{
    std::vector<std::string> strings;
    if (NetPermissions::HasFlag(flags, PF_BLOOMFILTER)) strings.push_back("bloomfilter");
    if (NetPermissions::HasFlag(flags, PF_NOBAN)) strings.push_back("noban");
    if (NetPermissions::HasFlag(flags, PF_FORCERELAY)) strings.push_back("forcerelay");
    if (NetPermissions::HasFlag(flags, PF_RELAY)) strings.push_back("relay");
    if (NetPermissions::HasFlag(flags, PF_MEMPOOL)) strings.push_back("mempool");
    return strings;
}

bool NetWhitebindPermissions::TryParse(const std::string str, NetWhitebindPermissions& output, std::string& error)
{
    NetPermissionFlags flags;
    size_t offset;
    if (!TryParsePermissionFlags(str, flags, offset, error)) return false;

    const std::string strBind = str.substr(offset);
    CService addrBind;
    if (!Lookup(strBind.c_str(), addrBind, 0, false)) {
        error = strprintf(_("Cannot resolve -%s address: '%s'"), "whitebind", strBind);
        return false;
    }
    if (addrBind.GetPort() == 0) {
        error = strprintf(_("Need to specify a port with -whitebind: '%s'"), strBind);
        return false;
    }

    output.m_flags = flags;
    output.m_service = addrBind;
    error = "";
    return true;
}

bool NetWhitelistPermissions::TryParse(const std::string str, NetWhitelistPermissions& output, std::string& error)
{
    NetPermissionFlags flags;
    size_t offset;
    if (!TryParsePermissionFlags(str, flags, offset, error)) return false;

    const std::string net = str.substr(offset);
    CSubNet subnet;
    LookupSubNet(net.c_str(), subnet);
    if (!subnet.IsValid()) {
        error = strprintf(_("Invalid netmask specified in -whitelist: '%s'"), net);
        return false;
    }

    output.m_flags = flags;
    output.m_subnet = subnet;
    error = "";
    return true;
}
