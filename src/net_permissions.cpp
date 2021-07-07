// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_permissions.h>
#include <netbase.h>
#include <util/error.h>
#include <util/system.h>
#include <util/translation.h>

const std::vector<std::string> NET_PERMISSIONS_DOC{
    "bloomfilter (allow requesting BIP37 filtered blocks and transactions)",
    "noban (do not ban for misbehavior; implies download)",
    "forcerelay (relay transactions that are already in the mempool; implies relay)",
    "relay (relay even in -blocksonly mode, and unlimited transaction announcements)",
    "mempool (allow requesting BIP35 mempool contents)",
    "download (allow getheaders during IBD, no disconnect after maxuploadtarget limit)",
    "addr (responses to GETADDR avoid hitting the cache and contain random records with the most up-to-date info)"
};

namespace {

// Parse the following format: "perm1,perm2@xxxxxx"
bool TryParsePermissionFlags(const std::string& str, NetPermissionFlags& output, ConnectionDirection* output_connection_direction, size_t& readen, bilingual_str& error)
{
    NetPermissionFlags flags = NetPermissionFlags::None;
    ConnectionDirection connection_direction = ConnectionDirection::None;
    const auto atSeparator = str.find('@');

    // if '@' is not found (ie, "xxxxx"), the caller should apply implicit permissions
    if (atSeparator == std::string::npos) {
        NetPermissions::AddFlag(flags, NetPermissionFlags::Implicit);
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

            if (permission == "bloomfilter" || permission == "bloom") NetPermissions::AddFlag(flags, NetPermissionFlags::BloomFilter);
            else if (permission == "noban") NetPermissions::AddFlag(flags, NetPermissionFlags::NoBan);
            else if (permission == "forcerelay") NetPermissions::AddFlag(flags, NetPermissionFlags::ForceRelay);
            else if (permission == "mempool") NetPermissions::AddFlag(flags, NetPermissionFlags::Mempool);
            else if (permission == "download") NetPermissions::AddFlag(flags, NetPermissionFlags::Download);
            else if (permission == "all") NetPermissions::AddFlag(flags, NetPermissionFlags::All);
            else if (permission == "relay") NetPermissions::AddFlag(flags, NetPermissionFlags::Relay);
            else if (permission == "addr") NetPermissions::AddFlag(flags, NetPermissionFlags::Addr);
            else if (permission == "in") connection_direction |= ConnectionDirection::In;
            else if (permission == "out") {
                if (!output_connection_direction) {
                    error = _("whitebind is only used for incoming connections");
                    return false;
                }
                connection_direction |= ConnectionDirection::Out;
            }
            else if (permission.length() == 0); // Allow empty entries
            else {
                error = strprintf(_("Invalid P2P permission: '%s'"), permission);
                return false;
            }
        }
        readen++;
    }

    // By default, whitelist only applies to incoming connections
    if (connection_direction == ConnectionDirection::None) connection_direction = ConnectionDirection::In;

    output = flags;
    if (output_connection_direction) *output_connection_direction = connection_direction;
    error = Untranslated("");
    return true;
}

}

std::vector<std::string> NetPermissions::ToStrings(NetPermissionFlags flags)
{
    std::vector<std::string> strings;
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::BloomFilter)) strings.push_back("bloomfilter");
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::NoBan)) strings.push_back("noban");
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::ForceRelay)) strings.push_back("forcerelay");
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::Relay)) strings.push_back("relay");
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::Mempool)) strings.push_back("mempool");
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::Download)) strings.push_back("download");
    if (NetPermissions::HasFlag(flags, NetPermissionFlags::Addr)) strings.push_back("addr");
    return strings;
}

bool NetWhitebindPermissions::TryParse(const std::string& str, NetWhitebindPermissions& output, bilingual_str& error)
{
    NetPermissionFlags flags;
    size_t offset;
    if (!TryParsePermissionFlags(str, flags, nullptr, offset, error)) return false;

    const std::string strBind = str.substr(offset);
    CService addrBind;
    if (!Lookup(strBind, addrBind, 0, false)) {
        error = ResolveErrMsg("whitebind", strBind);
        return false;
    }
    if (addrBind.GetPort() == 0) {
        error = strprintf(_("Need to specify a port with -whitebind: '%s'"), strBind);
        return false;
    }

    output.m_flags = flags;
    output.m_service = addrBind;
    error = Untranslated("");
    return true;
}

bool NetWhitelistPermissions::TryParse(const std::string& str, NetWhitelistPermissions& output, ConnectionDirection& output_connection_direction, bilingual_str& error)
{
    NetPermissionFlags flags;
    size_t offset;
    if (!TryParsePermissionFlags(str, flags, &output_connection_direction, offset, error)) return false;

    const std::string net = str.substr(offset);
    CSubNet subnet;
    LookupSubNet(net, subnet);
    if (!subnet.IsValid()) {
        error = strprintf(_("Invalid netmask specified in -whitelist: '%s'"), net);
        return false;
    }

    output.m_flags = flags;
    output.m_subnet = subnet;
    error = Untranslated("");
    return true;
}
