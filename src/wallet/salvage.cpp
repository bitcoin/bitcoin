// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <streams.h>
#include <util/fs.h>
#include <util/translation.h>
#include <wallet/bdb.h>
#include <wallet/salvage.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>

#include <db_cxx.h>

namespace wallet {
/* End of headers, beginning of key/value data */
static const char *HEADER_END = "HEADER=END";
/* End of key/value data */
static const char *DATA_END = "DATA=END";
typedef std::pair<std::vector<unsigned char>, std::vector<unsigned char> > KeyValPair;

class DummyCursor : public DatabaseCursor
{
    Status Next(DataStream& key, DataStream& value) override { return Status::FAIL; }
};

/** RAII class that provides access to a DummyDatabase. Never fails. */
class DummyBatch : public DatabaseBatch
{
private:
    bool ReadKey(DataStream&& key, DataStream& value) override { return true; }
    bool WriteKey(DataStream&& key, DataStream&& value, bool overwrite=true) override { return true; }
    bool EraseKey(DataStream&& key) override { return true; }
    bool HasKey(DataStream&& key) override { return true; }
    bool ErasePrefix(Span<const std::byte> prefix) override { return true; }

public:
    void Flush() override {}
    void Close() override {}

    std::unique_ptr<DatabaseCursor> GetNewCursor() override { return std::make_unique<DummyCursor>(); }
    std::unique_ptr<DatabaseCursor> GetNewPrefixCursor(Span<const std::byte> prefix) override { return GetNewCursor(); }
    bool TxnBegin() override { return true; }
    bool TxnCommit() override { return true; }
    bool TxnAbort() override { return true; }
};

/** A dummy WalletDatabase that does nothing and never fails. Only used by salvage.
 **/
class DummyDatabase : public WalletDatabase
{
public:
    void Open() override {};
    void AddRef() override {}
    void RemoveRef() override {}
    bool Rewrite(const char* pszSkip=nullptr) override { return true; }
    bool Backup(const std::string& strDest) const override { return true; }
    void Close() override {}
    void Flush() override {}
    bool PeriodicFlush() override { return true; }
    void IncrementUpdateCounter() override { ++nUpdateCounter; }
    void ReloadDbEnv() override {}
    std::string Filename() override { return "dummy"; }
    std::string Format() override { return "dummy"; }
    std::unique_ptr<DatabaseBatch> MakeBatch(bool flush_on_close = true) override { return std::make_unique<DummyBatch>(); }
};

bool RecoverDatabaseFile(const ArgsManager& args, const fs::path& file_path, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    DatabaseOptions options;
    DatabaseStatus status;
    ReadDatabaseArgs(args, options);
    options.require_existing = true;
    options.verify = false;
    options.require_format = DatabaseFormat::BERKELEY;
    std::unique_ptr<WalletDatabase> database = MakeDatabase(file_path, options, status, error);
    if (!database) return false;

    BerkeleyDatabase& berkeley_database = static_cast<BerkeleyDatabase&>(*database);
    std::string filename = berkeley_database.Filename();
    std::shared_ptr<BerkeleyEnvironment> env = berkeley_database.env;

    if (!ResultExtract(env->Open(), nullptr, &error)) {
        return false;
    }

    // Recovery procedure:
    // move wallet file to walletfilename.timestamp.bak
    // Call Salvage with fAggressive=true to
    // get as much data as possible.
    // Rewrite salvaged data to fresh wallet file
    // Rescan so any missing transactions will be
    // found.
    int64_t now = GetTime();
    std::string newFilename = strprintf("%s.%d.bak", filename, now);

    int result = env->dbenv->dbrename(nullptr, filename.c_str(), nullptr,
                                       newFilename.c_str(), DB_AUTO_COMMIT);
    if (result != 0)
    {
        error = strprintf(Untranslated("Failed to rename %s to %s"), filename, newFilename);
        return false;
    }

    /**
     * Salvage data from a file. The DB_AGGRESSIVE flag is being used (see berkeley DB->verify() method documentation).
     * key/value pairs are appended to salvagedData which are then written out to a new wallet file.
     * NOTE: reads the entire database into memory, so cannot be used
     * for huge databases.
     */
    std::vector<KeyValPair> salvagedData;

    std::stringstream strDump;

    Db db(env->dbenv.get(), 0);
    result = db.verify(newFilename.c_str(), nullptr, &strDump, DB_SALVAGE | DB_AGGRESSIVE);
    if (result == DB_VERIFY_BAD) {
        warnings.push_back(Untranslated("Salvage: Database salvage found errors, all data may not be recoverable."));
    }
    if (result != 0 && result != DB_VERIFY_BAD) {
        error = strprintf(Untranslated("Salvage: Database salvage failed with result %d."), result);
        return false;
    }

    // Format of bdb dump is ascii lines:
    // header lines...
    // HEADER=END
    //  hexadecimal key
    //  hexadecimal value
    //  ... repeated
    // DATA=END

    std::string strLine;
    while (!strDump.eof() && strLine != HEADER_END)
        getline(strDump, strLine); // Skip past header

    std::string keyHex, valueHex;
    while (!strDump.eof() && keyHex != DATA_END) {
        getline(strDump, keyHex);
        if (keyHex != DATA_END) {
            if (strDump.eof())
                break;
            getline(strDump, valueHex);
            if (valueHex == DATA_END) {
                warnings.push_back(Untranslated("Salvage: WARNING: Number of keys in data does not match number of values."));
                break;
            }
            salvagedData.emplace_back(ParseHex(keyHex), ParseHex(valueHex));
        }
    }

    bool fSuccess;
    if (keyHex != DATA_END) {
        warnings.push_back(Untranslated("Salvage: WARNING: Unexpected end of file while reading salvage output."));
        fSuccess = false;
    } else {
        fSuccess = (result == 0);
    }

    if (salvagedData.empty())
    {
        error = strprintf(Untranslated("Salvage(aggressive) found no records in %s."), newFilename);
        return false;
    }

    std::unique_ptr<Db> pdbCopy = std::make_unique<Db>(env->dbenv.get(), 0);
    int ret = pdbCopy->open(nullptr,               // Txn pointer
                            filename.c_str(),   // Filename
                            "main",             // Logical db name
                            DB_BTREE,           // Database type
                            DB_CREATE,          // Flags
                            0);
    if (ret > 0) {
        error = strprintf(Untranslated("Cannot create database file %s"), filename);
        pdbCopy->close(0);
        return false;
    }

    DbTxn* ptxn = env->TxnBegin(DB_TXN_WRITE_NOSYNC);
    CWallet dummyWallet(nullptr, "", std::make_unique<DummyDatabase>());
    for (KeyValPair& row : salvagedData)
    {
        /* Filter for only private key type KV pairs to be added to the salvaged wallet */
        DataStream ssKey{row.first};
        DataStream ssValue(row.second);
        std::string strType, strErr;

        // We only care about KEY, MASTER_KEY, CRYPTED_KEY, and HDCHAIN types
        ssKey >> strType;
        bool fReadOK = false;
        if (strType == DBKeys::KEY) {
            fReadOK = LoadKey(&dummyWallet, ssKey, ssValue, strErr);
        } else if (strType == DBKeys::CRYPTED_KEY) {
            fReadOK = LoadCryptedKey(&dummyWallet, ssKey, ssValue, strErr);
        } else if (strType == DBKeys::MASTER_KEY) {
            fReadOK = LoadEncryptionKey(&dummyWallet, ssKey, ssValue, strErr);
        } else if (strType == DBKeys::HDCHAIN) {
            fReadOK = LoadHDChain(&dummyWallet, ssValue, strErr);
        } else {
            continue;
        }

        if (!fReadOK)
        {
            warnings.push_back(strprintf(Untranslated("WARNING: WalletBatch::Recover skipping %s: %s"), strType, strErr));
            continue;
        }
        Dbt datKey(row.first.data(), row.first.size());
        Dbt datValue(row.second.data(), row.second.size());
        int ret2 = pdbCopy->put(ptxn, &datKey, &datValue, DB_NOOVERWRITE);
        if (ret2 > 0)
            fSuccess = false;
    }
    ptxn->commit(0);
    pdbCopy->close(0);

    return fSuccess;
}
} // namespace wallet
