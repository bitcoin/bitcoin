// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_types.h>

#include <netaddress.h>
#include <netbase.h>
#include <univalue.h>

CBanEntry::CBanEntry(const UniValue& json)
    : nVersion(json["version"].get_int()), nCreateTime(json["ban_created"].get_int64()),
      nBanUntil(json["banned_until"].get_int64())
{
}

UniValue CBanEntry::ToJson() const
{
    UniValue json(UniValue::VOBJ);
    json.pushKV("version", nVersion);
    json.pushKV("ban_created", nCreateTime);
    json.pushKV("banned_until", nBanUntil);
    return json;
}

static const char* BANMAN_JSON_ADDR_KEY = "address";

/**
 * Convert a `banmap_t` object to a JSON array.
 * @param[in] bans Bans list to convert.
 * @return a JSON array, similar to the one returned by the `listbanned` RPC. Suitable for
 * passing to `BanMapFromJson()`.
 */
UniValue BanMapToJson(const banmap_t& bans)
{
    UniValue bans_json(UniValue::VARR);
    for (const auto& it : bans) {
        const auto& address = it.first;
        const auto& ban_entry = it.second;
        UniValue j = ban_entry.ToJson();
        j.pushKV(BANMAN_JSON_ADDR_KEY, address.ToString());
        bans_json.push_back(j);
    }
    return bans_json;
}

/**
 * Convert a JSON array to a `banmap_t` object.
 * @param[in] bans_json JSON to convert, must be as returned by `BanMapToJson()`.
 * @param[out] bans Bans list to create from the JSON.
 * @throws std::runtime_error if the JSON does not have the expected fields or they contain
 * unparsable values.
 */
void BanMapFromJson(const UniValue& bans_json, banmap_t& bans)
{
    for (const auto& ban_entry_json : bans_json.getValues()) {
        CSubNet subnet;
        const auto& subnet_str = ban_entry_json[BANMAN_JSON_ADDR_KEY].get_str();
        if (!LookupSubNet(subnet_str, subnet)) {
            throw std::runtime_error(
                strprintf("Cannot parse banned address or subnet: %s", subnet_str));
        }
        bans.insert_or_assign(subnet, CBanEntry{ban_entry_json});
    }
}
