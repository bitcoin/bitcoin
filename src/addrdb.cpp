// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <addrdb.h>

#include <addrman.h>
#include <chainparams.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/settings.h>
#include <cstdint>
#include <hash.h>
#include <logging.h>
#include <logging/timer.h>
#include <netbase.h>
#include <netgroup.h>
#include <random.h>
#include <streams.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/syserror.h>
#include <util/translation.h>

namespace {

class DbNotFoundError : public std::exception
{
    using std::exception::exception;
};

template <typename Stream, typename Data>
bool SerializeDB(Stream& stream, const Data& data)
{
    // Write and commit header, data
    try {
        HashedSourceWriter hashwriter{stream};
        hashwriter << Params().MessageStart() << data;
        stream << hashwriter.GetHash();
    } catch (const std::exception& e) {
        LogError("%s: Serialize or I/O error - %s\n", __func__, e.what());
        return false;
    }

    return true;
}

template <typename Data>
bool SerializeFileDB(const std::string& prefix, const fs::path& path, const Data& data)
{
    // Generate random temporary filename
    const uint16_t randv{FastRandomContext().rand<uint16_t>()};
    std::string tmpfn = strprintf("%s.%04x", prefix, randv);

    // open temp output file
    fs::path pathTmp = gArgs.GetDataDirNet() / fs::u8path(tmpfn);
    FILE *file = fsbridge::fopen(pathTmp, "wb");
    AutoFile fileout{file};
    if (fileout.IsNull()) {
        remove(pathTmp);
        LogError("%s: Failed to open file %s\n", __func__, fs::PathToString(pathTmp));
        return false;
    }

    // Serialize
    if (!SerializeDB(fileout, data)) {
        (void)fileout.fclose();
        remove(pathTmp);
        return false;
    }
    if (!fileout.Commit()) {
        (void)fileout.fclose();
        remove(pathTmp);
        LogError("%s: Failed to flush file %s\n", __func__, fs::PathToString(pathTmp));
        return false;
    }
    if (fileout.fclose() != 0) {
        const int errno_save{errno};
        remove(pathTmp);
        LogError("Failed to close file %s after commit: %s", fs::PathToString(pathTmp), SysErrorString(errno_save));
        return false;
    }

    // replace existing file, if any, with new file
    if (!RenameOver(pathTmp, path)) {
        remove(pathTmp);
        LogError("%s: Rename-into-place failed\n", __func__);
        return false;
    }

    return true;
}

template <typename Stream, typename Data>
void DeserializeDB(Stream& stream, Data&& data, bool fCheckSum = true)
{
    HashVerifier verifier{stream};
    // de-serialize file header (network specific magic number) and ..
    MessageStartChars pchMsgTmp;
    verifier >> pchMsgTmp;
    // ... verify the network matches ours
    if (pchMsgTmp != Params().MessageStart()) {
        throw std::runtime_error{"Invalid network magic number"};
    }

    // de-serialize data
    verifier >> data;

    // verify checksum
    if (fCheckSum) {
        uint256 hashTmp;
        stream >> hashTmp;
        if (hashTmp != verifier.GetHash()) {
            throw std::runtime_error{"Checksum mismatch, data corrupted"};
        }
    }
}

template <typename Data>
void DeserializeFileDB(const fs::path& path, Data&& data)
{
    FILE* file = fsbridge::fopen(path, "rb");
    AutoFile filein{file};
    if (filein.IsNull()) {
        throw DbNotFoundError{};
    }
    DeserializeDB(filein, data);
}
} // namespace

CBanDB::CBanDB(fs::path ban_list_path)
    : m_banlist_dat(ban_list_path + ".dat"),
      m_banlist_json(ban_list_path + ".json")
{
}

bool CBanDB::Write(const banmap_t& banSet)
{
    std::vector<std::string> errors;
    if (common::WriteSettings(m_banlist_json, {{JSON_KEY, BanMapToJson(banSet)}}, errors)) {
        return true;
    }

    for (const auto& err : errors) {
        LogError("%s\n", err);
    }
    return false;
}

bool CBanDB::Read(banmap_t& banSet)
{
    if (fs::exists(m_banlist_dat)) {
        LogWarning("banlist.dat ignored because it can only be read by " CLIENT_NAME " version 22.x. Remove %s to silence this warning.", fs::quoted(fs::PathToString(m_banlist_dat)));
    }
    // If the JSON banlist does not exist, then recreate it
    if (!fs::exists(m_banlist_json)) {
        return false;
    }

    std::map<std::string, common::SettingsValue> settings;
    std::vector<std::string> errors;

    if (!common::ReadSettings(m_banlist_json, settings, errors)) {
        for (const auto& err : errors) {
            LogWarning("Cannot load banlist %s: %s", fs::PathToString(m_banlist_json), err);
        }
        return false;
    }

    try {
        BanMapFromJson(settings[JSON_KEY], banSet);
    } catch (const std::runtime_error& e) {
        LogWarning("Cannot parse banlist %s: %s", fs::PathToString(m_banlist_json), e.what());
        return false;
    }

    return true;
}

bool DumpPeerAddresses(const ArgsManager& args, const AddrMan& addr)
{
    const auto pathAddr = args.GetDataDirNet() / "peers.dat";
    return SerializeFileDB("peers", pathAddr, addr);
}

void ReadFromStream(AddrMan& addr, DataStream& ssPeers)
{
    DeserializeDB(ssPeers, addr, false);
}

util::Result<std::unique_ptr<AddrMan>> LoadAddrman(const NetGroupManager& netgroupman, const ArgsManager& args)
{
    auto check_addrman = std::clamp<int32_t>(args.GetIntArg("-checkaddrman", DEFAULT_ADDRMAN_CONSISTENCY_CHECKS), 0, 1000000);
    bool deterministic = HasTestOption(args, "addrman"); // use a deterministic addrman only for tests

    auto addrman{std::make_unique<AddrMan>(netgroupman, deterministic, /*consistency_check_ratio=*/check_addrman)};

    const auto start{SteadyClock::now()};
    const auto path_addr{args.GetDataDirNet() / "peers.dat"};
    try {
        DeserializeFileDB(path_addr, *addrman);
        LogInfo("Loaded %i addresses from peers.dat  %dms", addrman->Size(), Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));
    } catch (const DbNotFoundError&) {
        // Addrman can be in an inconsistent state after failure, reset it
        addrman = std::make_unique<AddrMan>(netgroupman, deterministic, /*consistency_check_ratio=*/check_addrman);
        LogInfo("Creating peers.dat because the file was not found (%s)", fs::quoted(fs::PathToString(path_addr)));
        DumpPeerAddresses(args, *addrman);
    } catch (const InvalidAddrManVersionError&) {
        if (!RenameOver(path_addr, (fs::path)path_addr + ".bak")) {
            return util::Error{strprintf(_("Failed to rename invalid peers.dat file. Please move or delete it and try again."))};
        }
        // Addrman can be in an inconsistent state after failure, reset it
        addrman = std::make_unique<AddrMan>(netgroupman, deterministic, /*consistency_check_ratio=*/check_addrman);
        LogWarning("Creating new peers.dat because the file version was not compatible (%s). Original backed up to peers.dat.bak", fs::quoted(fs::PathToString(path_addr)));
        DumpPeerAddresses(args, *addrman);
    } catch (const std::exception& e) {
        return util::Error{strprintf(_("Invalid or corrupt peers.dat (%s). If you believe this is a bug, please report it to %s. As a workaround, you can move the file (%s) out of the way (rename, move, or delete) to have a new one created on the next start."),
                                     e.what(), CLIENT_BUGREPORT, fs::quoted(fs::PathToString(path_addr)))};
    }
    return addrman;
}

void DumpAnchors(const fs::path& anchors_db_path, const std::vector<CAddress>& anchors)
{
    LOG_TIME_SECONDS(strprintf("Flush %d outbound block-relay-only peer addresses to anchors.dat", anchors.size()));
    SerializeFileDB("anchors", anchors_db_path, CAddress::V2_DISK(anchors));
}

std::vector<CAddress> ReadAnchors(const fs::path& anchors_db_path)
{
    std::vector<CAddress> anchors;
    try {
        DeserializeFileDB(anchors_db_path, CAddress::V2_DISK(anchors));
        LogInfo("Loaded %i addresses from %s", anchors.size(), fs::quoted(fs::PathToString(anchors_db_path.filename())));
    } catch (const std::exception&) {
        anchors.clear();
    }

    fs::remove(anchors_db_path);
    return anchors;
}
