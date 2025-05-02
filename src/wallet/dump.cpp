// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/dump.h>

#include <common/args.h>
#include <util/fs.h>
#include <util/translation.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace wallet {
static const std::string DUMP_MAGIC = "BITCOIN_CORE_WALLET_DUMP";
uint32_t DUMP_VERSION = 1;

bool DumpWallet(const ArgsManager& args, WalletDatabase& db, bilingual_str& error)
{
    // Get the dumpfile
    std::string dump_filename = args.GetArg("-dumpfile", "");
    if (dump_filename.empty()) {
        error = _("No dump file provided. To use dump, -dumpfile=<filename> must be provided.");
        return false;
    }

    fs::path path = fs::PathFromString(dump_filename);
    path = fs::absolute(path);
    if (fs::exists(path)) {
        error = strprintf(_("File %s already exists. If you are sure this is what you want, move it out of the way first."), fs::PathToString(path));
        return false;
    }
    std::ofstream dump_file;
    dump_file.open(path);
    if (dump_file.fail()) {
        error = strprintf(_("Unable to open %s for writing"), fs::PathToString(path));
        return false;
    }

    HashWriter hasher{};

    std::unique_ptr<DatabaseBatch> batch = db.MakeBatch();

    bool ret = true;
    std::unique_ptr<DatabaseCursor> cursor = batch->GetNewCursor();
    if (!cursor) {
        error = _("Error: Couldn't create cursor into database");
        ret = false;
    }

    // Write out a magic string with version
    std::string line = strprintf("%s,%u\n", DUMP_MAGIC, DUMP_VERSION);
    dump_file.write(line.data(), line.size());
    hasher << std::span{line};

    // Write out the file format
    std::string format = db.Format();
    // BDB files that are opened using BerkeleyRODatabase have it's format as "bdb_ro"
    // We want to override that format back to "bdb"
    if (format == "bdb_ro") {
        format = "bdb";
    }
    line = strprintf("%s,%s\n", "format", format);
    dump_file.write(line.data(), line.size());
    hasher << std::span{line};

    if (ret) {

        // Read the records
        while (true) {
            DataStream ss_key{};
            DataStream ss_value{};
            DatabaseCursor::Status status = cursor->Next(ss_key, ss_value);
            if (status == DatabaseCursor::Status::DONE) {
                ret = true;
                break;
            } else if (status == DatabaseCursor::Status::FAIL) {
                error = _("Error reading next record from wallet database");
                ret = false;
                break;
            }
            std::string key_str = HexStr(ss_key);
            std::string value_str = HexStr(ss_value);
            line = strprintf("%s,%s\n", key_str, value_str);
            dump_file.write(line.data(), line.size());
            hasher << std::span{line};
        }
    }

    cursor.reset();
    batch.reset();

    if (ret) {
        // Write the hash
        tfm::format(dump_file, "checksum,%s\n", HexStr(hasher.GetHash()));
        dump_file.close();
    } else {
        // Remove the dumpfile on failure
        dump_file.close();
        fs::remove(path);
    }

    return ret;
}

// The standard wallet deleter function blocks on the validation interface
// queue, which doesn't exist for the bitcoin-wallet. Define our own
// deleter here.
static void WalletToolReleaseWallet(CWallet* wallet)
{
    wallet->WalletLogPrintf("Releasing wallet\n");
    wallet->Close();
    delete wallet;
}

bool CreateFromDump(const ArgsManager& args, const std::string& name, const fs::path& wallet_path, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    // Get the dumpfile
    std::string dump_filename = args.GetArg("-dumpfile", "");
    if (dump_filename.empty()) {
        error = _("No dump file provided. To use createfromdump, -dumpfile=<filename> must be provided.");
        return false;
    }

    fs::path dump_path = fs::PathFromString(dump_filename);
    dump_path = fs::absolute(dump_path);
    if (!fs::exists(dump_path)) {
        error = strprintf(_("Dump file %s does not exist."), fs::PathToString(dump_path));
        return false;
    }
    std::ifstream dump_file{dump_path};

    // Compute the checksum
    HashWriter hasher{};
    uint256 checksum;

    // Check the magic and version
    std::string magic_key;
    std::getline(dump_file, magic_key, ',');
    std::string version_value;
    std::getline(dump_file, version_value, '\n');
    if (magic_key != DUMP_MAGIC) {
        error = strprintf(_("Error: Dumpfile identifier record is incorrect. Got \"%s\", expected \"%s\"."), magic_key, DUMP_MAGIC);
        dump_file.close();
        return false;
    }
    // Check the version number (value of first record)
    uint32_t ver;
    if (!ParseUInt32(version_value, &ver)) {
        error =strprintf(_("Error: Unable to parse version %u as a uint32_t"), version_value);
        dump_file.close();
        return false;
    }
    if (ver != DUMP_VERSION) {
        error = strprintf(_("Error: Dumpfile version is not supported. This version of bitcoin-wallet only supports version 1 dumpfiles. Got dumpfile with version %s"), version_value);
        dump_file.close();
        return false;
    }
    std::string magic_hasher_line = strprintf("%s,%s\n", magic_key, version_value);
    hasher << std::span{magic_hasher_line};

    // Get the stored file format
    std::string format_key;
    std::getline(dump_file, format_key, ',');
    std::string format_value;
    std::getline(dump_file, format_value, '\n');
    if (format_key != "format") {
        error = strprintf(_("Error: Dumpfile format record is incorrect. Got \"%s\", expected \"format\"."), format_key);
        dump_file.close();
        return false;
    }
    // Make sure that the dump was created from a sqlite database only as that is the only
    // type of database that we still support.
    // Other formats such as BDB should not be loaded into a sqlite database since they also
    // use a different type of wallet entirely which is no longer compatible with this software.
    if (format_value != "sqlite") {
        error = strprintf(_("Error: Dumpfile specifies an unsupported database format (%s). Only sqlite database dumps are supported"), format_value);
        return false;
    }
    std::string format_hasher_line = strprintf("%s,%s\n", format_key, format_value);
    hasher << std::span{format_hasher_line};

    DatabaseOptions options;
    DatabaseStatus status;
    ReadDatabaseArgs(args, options);
    options.require_create = true;
    options.require_format = DatabaseFormat::SQLITE;
    std::unique_ptr<WalletDatabase> database = MakeDatabase(wallet_path, options, status, error);
    if (!database) return false;

    // dummy chain interface
    bool ret = true;
    std::shared_ptr<CWallet> wallet(new CWallet(/*chain=*/nullptr, name, std::move(database)), WalletToolReleaseWallet);
    {
        LOCK(wallet->cs_wallet);
        DBErrors load_wallet_ret = wallet->LoadWallet();
        if (load_wallet_ret != DBErrors::LOAD_OK) {
            error = strprintf(_("Error creating %s"), name);
            return false;
        }

        // Get the database handle
        WalletDatabase& db = wallet->GetDatabase();
        std::unique_ptr<DatabaseBatch> batch = db.MakeBatch();
        batch->TxnBegin();

        // Read the records from the dump file and write them to the database
        while (dump_file.good()) {
            std::string key;
            std::getline(dump_file, key, ',');
            std::string value;
            std::getline(dump_file, value, '\n');

            if (key == "checksum") {
                std::vector<unsigned char> parsed_checksum = ParseHex(value);
                if (parsed_checksum.size() != checksum.size()) {
                    error = Untranslated("Error: Checksum is not the correct size");
                    ret = false;
                    break;
                }
                std::copy(parsed_checksum.begin(), parsed_checksum.end(), checksum.begin());
                break;
            }

            std::string line = strprintf("%s,%s\n", key, value);
            hasher << std::span{line};

            if (key.empty() || value.empty()) {
                continue;
            }

            if (!IsHex(key)) {
                error = strprintf(_("Error: Got key that was not hex: %s"), key);
                ret = false;
                break;
            }
            if (!IsHex(value)) {
                error = strprintf(_("Error: Got value that was not hex: %s"), value);
                ret = false;
                break;
            }

            std::vector<unsigned char> k = ParseHex(key);
            std::vector<unsigned char> v = ParseHex(value);
            if (!batch->Write(std::span{k}, std::span{v})) {
                error = strprintf(_("Error: Unable to write record to new wallet"));
                ret = false;
                break;
            }
        }

        if (ret) {
            uint256 comp_checksum = hasher.GetHash();
            if (checksum.IsNull()) {
                error = _("Error: Missing checksum");
                ret = false;
            } else if (checksum != comp_checksum) {
                error = strprintf(_("Error: Dumpfile checksum does not match. Computed %s, expected %s"), HexStr(comp_checksum), HexStr(checksum));
                ret = false;
            }
        }

        if (ret) {
            batch->TxnCommit();
        } else {
            batch->TxnAbort();
        }

        batch.reset();

        dump_file.close();
    }
    wallet.reset(); // The pointer deleter will close the wallet for us.

    // Remove the wallet dir if we have a failure
    if (!ret) {
        fs::remove_all(wallet_path);
    }

    return ret;
}
} // namespace wallet
