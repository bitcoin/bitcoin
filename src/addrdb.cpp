// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrdb.h>

#include <addrman.h>
#include <chainparams.h>
#include <clientversion.h>
#include <cstdint>
#include <hash.h>
#include <logging/timer.h>
#include <netbase.h>
#include <random.h>
#include <streams.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/settings.h>
#include <util/system.h>

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

namespace {

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

template <typename Stream, typename Data>
bool SerializeDB(Stream& stream, const Data& data)
{
    // Write and commit header, data
    try {
        CHashWriter hasher(stream.GetType(), stream.GetVersion());
        stream << Params().MessageStart() << data;
        hasher << Params().MessageStart() << data;
        stream << hasher.GetHash();
    } catch (const std::exception& e) {
        return error("%s: Serialize or I/O error - %s", __func__, e.what());
    }

    return true;
}

template <typename Data>
bool SerializeFileDB(const std::string& prefix, const fs::path& path, const Data& data, int version)
{
    // Generate random temporary filename
    uint16_t randv = 0;
    GetRandBytes((unsigned char*)&randv, sizeof(randv));
    std::string tmpfn = strprintf("%s.%04x", prefix, randv);

    // open temp output file, and associate with CAutoFile
    fs::path pathTmp = gArgs.GetDataDirNet() / tmpfn;
    FILE *file = fsbridge::fopen(pathTmp, "wb");
    CAutoFile fileout(file, SER_DISK, version);
    if (fileout.IsNull()) {
        fileout.fclose();
        remove(pathTmp);
        return error("%s: Failed to open file %s", __func__, pathTmp.string());
    }

    // Serialize
    if (!SerializeDB(fileout, data)) {
        fileout.fclose();
        remove(pathTmp);
        return false;
    }
    if (!FileCommit(fileout.Get())) {
        fileout.fclose();
        remove(pathTmp);
        return error("%s: Failed to flush file %s", __func__, pathTmp.string());
    }
    fileout.fclose();

    // replace existing file, if any, with new file
    if (!RenameOver(pathTmp, path)) {
        remove(pathTmp);
        return error("%s: Rename-into-place failed", __func__);
    }

    return true;
}

template <typename Stream, typename Data>
bool DeserializeDB(Stream& stream, Data& data, bool fCheckSum = true)
{
    try {
        CHashVerifier<Stream> verifier(&stream);
        // de-serialize file header (network specific magic number) and ..
        unsigned char pchMsgTmp[4];
        verifier >> pchMsgTmp;
        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
            return error("%s: Invalid network magic number", __func__);

        // de-serialize data
        verifier >> data;

        // verify checksum
        if (fCheckSum) {
            uint256 hashTmp;
            stream >> hashTmp;
            if (hashTmp != verifier.GetHash()) {
                return error("%s: Checksum mismatch, data corrupted", __func__);
            }
        }
    }
    catch (const std::exception& e) {
        return error("%s: Deserialize or I/O error - %s", __func__, e.what());
    }

    return true;
}

template <typename Data>
bool DeserializeFileDB(const fs::path& path, Data& data, int version)
{
    // open input file, and associate with CAutoFile
    FILE* file = fsbridge::fopen(path, "rb");
    CAutoFile filein(file, SER_DISK, version);
    if (filein.IsNull()) {
        LogPrintf("Missing or invalid file %s\n", path.string());
        return false;
    }
    return DeserializeDB(filein, data);
}
} // namespace

CBanDB::CBanDB(fs::path ban_list_path)
    : m_banlist_dat(ban_list_path.string() + ".dat"),
      m_banlist_json(ban_list_path.string() + ".json")
{
}

bool CBanDB::Write(const banmap_t& banSet)
{
    std::vector<std::string> errors;
    if (util::WriteSettings(m_banlist_json, {{JSON_KEY, BanMapToJson(banSet)}}, errors)) {
        return true;
    }

    for (const auto& err : errors) {
        error("%s", err);
    }
    return false;
}

bool CBanDB::Read(banmap_t& banSet, bool& dirty)
{
    // If the JSON banlist does not exist, then try to read the non-upgraded banlist.dat.
    if (!fs::exists(m_banlist_json)) {
        // If this succeeds then we need to flush to disk in order to create the JSON banlist.
        dirty = true;
        return DeserializeFileDB(m_banlist_dat, banSet, CLIENT_VERSION);
    }

    dirty = false;

    std::map<std::string, util::SettingsValue> settings;
    std::vector<std::string> errors;

    if (!util::ReadSettings(m_banlist_json, settings, errors)) {
        for (const auto& err : errors) {
            LogPrintf("Cannot load banlist %s: %s\n", m_banlist_json.string(), err);
        }
        return false;
    }

    try {
        BanMapFromJson(settings[JSON_KEY], banSet);
    } catch (const std::runtime_error& e) {
        LogPrintf("Cannot parse banlist %s: %s\n", m_banlist_json.string(), e.what());
        return false;
    }

    return true;
}

CAddrDB::CAddrDB()
{
    pathAddr = gArgs.GetDataDirNet() / "peers.dat";
}

bool CAddrDB::Write(const CAddrMan& addr)
{
    return SerializeFileDB("peers", pathAddr, addr, CLIENT_VERSION);
}

bool CAddrDB::Read(CAddrMan& addr)
{
    return DeserializeFileDB(pathAddr, addr, CLIENT_VERSION);
}

bool CAddrDB::Read(CAddrMan& addr, CDataStream& ssPeers)
{
    bool ret = DeserializeDB(ssPeers, addr, false);
    if (!ret) {
        // Ensure addrman is left in a clean state
        addr.Clear();
    }
    return ret;
}

void DumpAnchors(const fs::path& anchors_db_path, const std::vector<CAddress>& anchors)
{
    LOG_TIME_SECONDS(strprintf("Flush %d outbound block-relay-only peer addresses to anchors.dat", anchors.size()));
    SerializeFileDB("anchors", anchors_db_path, anchors, CLIENT_VERSION | ADDRV2_FORMAT);
}

std::vector<CAddress> ReadAnchors(const fs::path& anchors_db_path)
{
    std::vector<CAddress> anchors;
    if (DeserializeFileDB(anchors_db_path, anchors, CLIENT_VERSION | ADDRV2_FORMAT)) {
        LogPrintf("Loaded %i addresses from %s\n", anchors.size(), anchors_db_path.filename());
    } else {
        anchors.clear();
    }

    fs::remove(anchors_db_path);
    return anchors;
}
