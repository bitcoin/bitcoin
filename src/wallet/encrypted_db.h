// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_ENCRYPTED_DB_H
#define BITCOIN_WALLET_ENCRYPTED_DB_H

#include <streams.h>
#include <util/result.h>
#include <wallet/crypter.h>
#include <wallet/db.h>

namespace wallet {
// Map of decrypted record key data to the encrypted record key data
// This allows us to get the actual db key data in order to lookups against the underlying db.
using DecryptedRecordKeys = std::map<SerializeData, SerializeData, std::less<>>;

class EncryptedDatabase;
class EncryptedDBBatch;

class EncryptedDBCursor : public DatabaseCursor
{
public:
    DecryptedRecordKeys::const_iterator m_cursor;
    DecryptedRecordKeys::const_iterator m_cursor_end;
    EncryptedDBBatch& m_batch;

    explicit EncryptedDBCursor(const DecryptedRecordKeys& records, EncryptedDBBatch& batch) : m_cursor(records.begin()), m_cursor_end(records.end()), m_batch(batch) {}
    EncryptedDBCursor(const DecryptedRecordKeys& records, EncryptedDBBatch& batch, Span<const std::byte> prefix);
    ~EncryptedDBCursor() {}

    Status Next(DataStream& key, DataStream& value) override;
};

/** RAII class that provides access to a WalletDatabase */
class EncryptedDBBatch : public DatabaseBatch
{
private:
    //! A DatabaseBatch for the db underlying the EncryptedDatabase
    std::unique_ptr<DatabaseBatch> m_batch;
    EncryptedDatabase& m_database;

    bool m_txn_started{false};
    std::vector<std::pair<SerializeData, SerializeData>> m_txn_writes;
    std::vector<SerializeData> m_txn_erases;

    bool ReadKey(DataStream&& key, DataStream& value) override;
    bool WriteKey(DataStream&& key, DataStream&& value, bool overwrite = true) override;
    bool EraseKey(DataStream&& key) override;
    bool HasKey(DataStream&& key) override;

public:
    explicit EncryptedDBBatch(std::unique_ptr<DatabaseBatch> batch, EncryptedDatabase& database) : m_batch(std::move(batch)), m_database(database) {}
    ~EncryptedDBBatch() {}

    void Flush() override { m_batch->Flush(); }
    void Close() override;

    bool ErasePrefix(Span<const std::byte> prefix) override;

    std::unique_ptr<DatabaseCursor> GetNewCursor() override;
    std::unique_ptr<DatabaseCursor> GetNewPrefixCursor(Span<const std::byte> prefix) override;
    bool TxnBegin() override;
    bool TxnCommit() override;
    bool TxnAbort() override;

    bool ReadEncryptedKey(SerializeData enc_key, DataStream& value);
};

/**
 * EncryptedDatabase encrypts and decrypts records as they are read and written from an underlying
 * database. Most functions are simply passed through.
 * An unencrypted copy of every record key is held in memory. This allows to lookup records by
 * unencrypted record key. The value will be read from the underlying db and decrypted.
 **/
class EncryptedDatabase : public WalletDatabase
{
private:
    /** The underlying database */
    std::unique_ptr<WalletDatabase> m_database;

    /** CCrypter which encrypts and decrypts the data */
    CCrypter m_crypter;
    /** The key used to encrypt the records */
    CKeyingMaterial m_enc_secret;

public:
    /** The unencrypted record keys, using a data type with secure allocation */
    DecryptedRecordKeys m_record_keys;

    EncryptedDatabase() = delete;

    EncryptedDatabase(std::unique_ptr<WalletDatabase> database, const SecureString& passphrase, bool create);

    ~EncryptedDatabase() {};

    inline static const Span<const std::byte> ENCRYPTION_RECORD = MakeByteSpan("encrypted_db_key");

    util::Result<SerializeData> DecryptRecordData(Span<const std::byte> data);
    util::Result<SerializeData> EncryptRecordData(Span<const std::byte> data);

    /** Open the database if it is not already opened. */
    void Open() override;

    std::string Format() override { return "encrypted_" + m_database->Format(); }

    /** Passthrough */
    void Close() override { m_database->Close(); }
    void AddRef() override { m_database->AddRef() ;}
    void RemoveRef() override { m_database->RemoveRef(); }
    bool Rewrite(const char* pszSkip=nullptr) override { return m_database->Rewrite(pszSkip); }
    bool Backup(const std::string& strDest) const override { return m_database->Backup(strDest); }
    void Flush() override { m_database->Flush(); }
    bool PeriodicFlush() override { return m_database->PeriodicFlush(); }
    void IncrementUpdateCounter() override { m_database->IncrementUpdateCounter(); }
    void ReloadDbEnv() override { m_database->ReloadDbEnv(); }
    std::string Filename() override { return m_database->Filename(); }

    /** Make a DatabaseBatch connected to this database */
    std::unique_ptr<DatabaseBatch> MakeBatch(bool flush_on_close = true) override { return std::make_unique<EncryptedDBBatch>(m_database->MakeBatch(flush_on_close), *this); }
};

std::unique_ptr<EncryptedDatabase> MakeEncryptedSQLiteDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error);

} // namespace wallet

#endif // BITCOIN_WALLET_ENCRYPTED_DB_H
