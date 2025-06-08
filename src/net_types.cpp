// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_types.h>

#include <logging.h>
#include <netaddress.h>
#include <netbase.h>
#include <univalue.h>

namespace {
    constexpr char BANMAN_JSON_VERSION_KEY[] = "version";
    constexpr char BANMAN_JSON_ADDR_KEY[] = "address";
    constexpr char BANMAN_JSON_CREATED_KEY[] = "ban_created";
    constexpr char BANMAN_JSON_UNTIL_KEY[] = "banned_until";
}

CBanEntry::CBanEntry(const UniValue& json)
    : nVersion(json[BANMAN_JSON_VERSION_KEY].getInt<int>()),
      nCreateTime(json[BANMAN_JSON_CREATED_KEY].getInt<int64_t>()),
      nBanUntil(json[BANMAN_JSON_UNTIL_KEY].getInt<int64_t>())
{
}

UniValue CBanEntry::ToJson() const
{
    UniValue json(UniValue::VOBJ);
    json.pushKV(BANMAN_JSON_VERSION_KEY, nVersion);
    json.pushKV(BANMAN_JSON_CREATED_KEY, nCreateTime);
    json.pushKV(BANMAN_JSON_UNTIL_KEY, nBanUntil);
    return json;
}

UniValue BanMapToJson(const banmap_t& bans)
{
    UniValue bans_json(UniValue::VARR);
    bans_json.reserve(bans.size()); // Pre-allocate memory
    
    for (const auto& [address, ban_entry] : bans) {
        UniValue j = ban_entry.ToJson();
        j.pushKV(BANMAN_JSON_ADDR_KEY, address.ToString());
        bans_json.push_back(std::move(j));
    }
    
    return bans_json;
}

void BanMapFromJson(const UniValue& bans_json, banmap_t& bans)
{
    bans.clear(); // Ensure clean state
    
    for (const auto& ban_entry_json : bans_json.getValues()) {
        try {
            const int version{ban_entry_json[BANMAN_JSON_VERSION_KEY].getInt<int>()};
            if (version != CBanEntry::CURRENT_VERSION) {
                LogPrintf("Dropping entry with unknown version (%d) from ban list\n", version);
                continue;
            }

            const auto& subnet_str = ban_entry_json[BANMAN_JSON_ADDR_KEY].get_str();
            CSubNet subnet = LookupSubNet(subnet_str);
            
            if (!subnet.IsValid()) {
                LogPrintf("Dropping entry with unparseable address or subnet (%s) from ban list\n", subnet_str);
                continue;
            }

            bans.insert_or_assign(std::move(subnet), CBanEntry{ban_entry_json});
            
        } catch (const std::exception& e) {
            LogPrintf("Error processing ban entry: %s\n", e.what());
            continue;
        }
    }
}
