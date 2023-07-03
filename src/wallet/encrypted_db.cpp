// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <util/check.h>
#include <util/result.h>
#include <util/time.h>
#include <wallet/crypter.h>
#include <wallet/db.h>
#include <wallet/encrypted_db.h>
#include <wallet/sqlite.h>

namespace wallet {
EncryptedDatabase::EncryptedDatabase(std::unique_ptr<WalletDatabase> database, const SecureString& passphrase, bool create)
    : m_database(std::move(database))
{
    std::unique_ptr<DatabaseBatch> batch = m_database->MakeBatch();

    if (create) {
        // Doesn't exist, set it up
        // Generate the encryption secret
        CKeyingMaterial enc_secret;
        enc_secret.resize(WALLET_CRYPTO_KEY_SIZE);
        GetStrongRandBytes(enc_secret);

        // Encrypt the secret with the passphrase
        CMasterKey enc_key;
        enc_key.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
        GetStrongRandBytes(enc_key.vchSalt);

        CCrypter crypter;
        constexpr MillisecondsDouble target{100};
        auto start{SteadyClock::now()};
        crypter.SetKeyFromPassphrase(passphrase, enc_key.vchSalt, 25000, enc_key.nDerivationMethod);
        enc_key.nDeriveIterations = static_cast<unsigned int>(25000 * target / (SteadyClock::now() - start));

        start = SteadyClock::now();
        crypter.SetKeyFromPassphrase(passphrase, enc_key.vchSalt, enc_key.nDeriveIterations, enc_key.nDerivationMethod);
        enc_key.nDeriveIterations = (enc_key.nDeriveIterations + static_cast<unsigned int>(enc_key.nDeriveIterations * target / (SteadyClock::now() - start))) / 2;

        if (enc_key.nDeriveIterations < 25000)
            enc_key.nDeriveIterations = 25000;

        if (!crypter.SetKeyFromPassphrase(passphrase, enc_key.vchSalt, enc_key.nDeriveIterations, enc_key.nDerivationMethod)) {
            throw std::runtime_error("Unable to set encryption key from passphrase");
        }
        if (!crypter.Encrypt(enc_secret, enc_key.vchCryptedKey)) {
            throw std::runtime_error("Unable to encrypt key with passphrase");
        }

        // Write that to disk
        if (!batch->Write(ENCRYPTION_RECORD, enc_key)) {
            throw std::runtime_error("Unable to write the encryption key data");
        }
    }

    // Read the encrypted encryption key
    CMasterKey enc_key;
    if (!batch->Read(ENCRYPTION_RECORD, enc_key)) {
        throw std::runtime_error("Unable to read the encryption key data");
    }
    batch.reset();

    // Decrypt the key with passphrase
    CCrypter crypter;
    if (!crypter.SetKeyFromPassphrase(passphrase, enc_key.vchSalt, enc_key.nDeriveIterations, enc_key.nDerivationMethod)) {
        throw std::runtime_error("Unable to get decryption key from passphrase");
    }
    CKeyingMaterial enc_secret;
    if (!crypter.Decrypt(enc_key.vchCryptedKey, enc_secret)) {
        throw std::runtime_error("Unable to decrypt database, are you sure the passphrase is correct?");
    }
    m_enc_secret = std::move(enc_secret);

    // Make sure our crypter has the correct secret
    m_crypter.SetKey(m_enc_secret);

    Open();
}

util::Result<SerializeData> EncryptedDatabase::DecryptRecordData(Span<const std::byte> data)
{
    DataStream s_data(data);
    std::vector<unsigned char> iv;
    std::vector<unsigned char> ciphertext;
    s_data >> iv;
    s_data >> ciphertext;

    CKeyingMaterial plaintext;
    if (!m_crypter.SetIV(iv)) return util::Error{Untranslated("IV is not valid")};
    if (!m_crypter.Decrypt(ciphertext, plaintext)) return util::Error{Untranslated("Could not decrypt data")};

    SerializeData out{reinterpret_cast<std::byte*>(plaintext.data()), reinterpret_cast<std::byte*>(plaintext.data() + plaintext.size())};
    return out;
}

util::Result<SerializeData> EncryptedDatabase::EncryptRecordData(Span<const std::byte> data)
{
    DataStream s_out;

    HashWriter hasher;
    hasher << data;
    uint256 hash = hasher.GetHash();
    std::vector<unsigned char> iv(hash.begin(), hash.begin() + WALLET_CRYPTO_IV_SIZE);
    if (!m_crypter.SetIV(iv)) return util::Error{Untranslated("Unable to set IV")};
    s_out << iv;

    CKeyingMaterial plaintext(UCharCast(data.begin()), UCharCast(data.end()));
    std::vector<unsigned char> enc;
    if (!m_crypter.Encrypt(plaintext, enc)) return util::Error{Untranslated("Could not encrypt data")};
    s_out << enc;
    SerializeData out{s_out.begin(), s_out.end()};
    return out;
}

void EncryptedDatabase::Open()
{
    // Read all records into memory and decrypt them
    std::unique_ptr<DatabaseBatch> batch = m_database->MakeBatch();
    if (!batch) {
        throw std::runtime_error("Error getting database batch");
    }
    std::unique_ptr<DatabaseCursor> cursor = batch->GetNewCursor();
    if (!cursor) {
        throw std::runtime_error("Error getting database cursor");
    }
    DataStream key, value;
    while (true) {
        DatabaseCursor::Status status = cursor->Next(key, value);
        if (status == DatabaseCursor::Status::DONE) {
            break;
        } else if (status == DatabaseCursor::Status::FAIL) {
            throw std::runtime_error("Error reading next record in database");
        }

        // If this record is the encrypted key record, ignore it
        if (key == ENCRYPTION_RECORD) {
            continue;
        }

        // Every key-value record is serialized in the same way: both keys and values are an
        // IV followed by the ciphertext as a vector of bytes
        // The decrypted ciphertext is the data that the application stored.
        util::Result<SerializeData> key_data = DecryptRecordData(key);
        if (!key_data) {
            throw std::runtime_error(util::ErrorString(key_data).original);
        }
        SerializeData enc_key_data{key.begin(), key.end()};
        m_record_keys.emplace(*key_data, enc_key_data);
    }
}

bool EncryptedDBBatch::ReadKey(DataStream&& key, DataStream& value)
{
    // Lookup the encrypted key data from our map
    SerializeData key_data{key.begin(), key.end()};
    const auto& it = m_database.m_record_keys.find(key_data);
    if (it == m_database.m_record_keys.end()) {
        return false;
    }
    return ReadEncryptedKey(it->second, value);
}

bool EncryptedDBBatch::ReadEncryptedKey(SerializeData enc_key, DataStream& value)
{
    DataStream crypt_value;
    if (!m_batch->Read(MakeByteSpan(enc_key), crypt_value)) {
        return false;
    }
    util::Result<SerializeData> value_data = m_database.DecryptRecordData(crypt_value);
    if (!value_data) {
        return false;
    }
    value.write(*value_data);
    return true;
}

bool EncryptedDBBatch::WriteKey(DataStream&& key, DataStream&& value, bool overwrite)
{
    util::Result<SerializeData> enc_value = m_database.EncryptRecordData(value);
    if (!enc_value) {
        return false;
    }

    // Lookup the encrypted key data from our map
    SerializeData key_data{key.begin(), key.end()};
    SerializeData enc_key;
    const auto& it = m_database.m_record_keys.find(key_data);
    if (it != m_database.m_record_keys.end()) {
        enc_key = it->second;
    } else {
        util::Result<SerializeData> enc_key_res = m_database.EncryptRecordData(key);
        if (!enc_key_res) {
            return false;
        }
        enc_key = *enc_key_res;
    }

    if (!m_batch->Write(MakeByteSpan(enc_key), MakeByteSpan(*enc_value), overwrite)) {
        return false;
    }
    if (m_txn_started) {
        m_txn_writes.emplace_back(key_data, enc_key);
    } else {
        m_database.m_record_keys.emplace(key_data, enc_key);
    }
    return true;
}

bool EncryptedDBBatch::EraseKey(DataStream&& key)
{
    // Lookup the encrypted key data from our map
    SerializeData key_data{key.begin(), key.end()};
    const auto& it = m_database.m_record_keys.find(key_data);
    if (it == m_database.m_record_keys.end()) {
        return false;
    }
    if (!m_batch->Erase(MakeByteSpan(it->second))) {
        return false;
    }
    if (m_txn_started) {
        m_txn_erases.emplace_back(key_data);
    } else {
        m_database.m_record_keys.erase(it);
    }
    return true;
}
bool EncryptedDBBatch::HasKey(DataStream&& key)
{
    // Lookup the encrypted key data from our map
    SerializeData key_data{key.begin(), key.end()};
    const auto& it = m_database.m_record_keys.find(key_data);
    if (it == m_database.m_record_keys.end()) {
        return false;
    }
    Assume(m_batch->Exists(MakeByteSpan(it->second)));
    return true;
}

void EncryptedDBBatch::Close()
{
    if (m_txn_started) {
        TxnAbort();
    }
    m_batch->Close();
}

bool EncryptedDBBatch::ErasePrefix(Span<const std::byte> prefix)
{
    auto it = m_database.m_record_keys.begin();
    while (it != m_database.m_record_keys.end()) {
        auto& key = it->first;
        if (key.size() < prefix.size() || std::search(key.begin(), key.end(), prefix.begin(), prefix.end()) != key.begin()) {
            it++;
            continue;
        }
        m_batch->Erase(MakeByteSpan(it->second));
        if (m_txn_started) {
            m_txn_erases.emplace_back(key);
            it++;
        } else {
            it = m_database.m_record_keys.erase(it);
        }
    }
    return true;
}

bool EncryptedDBBatch::TxnBegin()
{
    if (m_txn_started) {
        return false;
    }
    if (!m_batch->TxnBegin()) {
        return false;
    }
    m_txn_writes.clear();
    m_txn_erases.clear();
    m_txn_started = true;
    return true;
}

bool EncryptedDBBatch::TxnCommit()
{
    if (!m_txn_started) {
        return false;
    }
    if (!m_batch->TxnCommit()) {
        return false;
    }

    for (const auto& [key_data, enc_key] : m_txn_writes) {
        m_database.m_record_keys.emplace(key_data, enc_key);
    }
    for (const auto& key_data : m_txn_erases) {
        m_database.m_record_keys.erase(key_data);
    }

    m_txn_started = false;
    m_txn_writes.clear();
    m_txn_erases.clear();
    return true;
}

bool EncryptedDBBatch::TxnAbort()
{
    if (!m_txn_started) {
        return false;
    }
    if (!m_batch->TxnAbort()) {
        return false;
    }
    m_txn_started = false;
    m_txn_writes.clear();
    m_txn_erases.clear();
    return true;
}

std::unique_ptr<DatabaseCursor> EncryptedDBBatch::GetNewCursor()
{
    return std::make_unique<EncryptedDBCursor>(m_database.m_record_keys, *this);
}

std::unique_ptr<DatabaseCursor> EncryptedDBBatch::GetNewPrefixCursor(Span<const std::byte> prefix)
{
    return std::make_unique<EncryptedDBCursor>(m_database.m_record_keys, *this, prefix);
}

EncryptedDBCursor::EncryptedDBCursor(const DecryptedRecordKeys& records, EncryptedDBBatch& batch, Span<const std::byte> prefix) : m_batch(batch)
{
    std::tie(m_cursor, m_cursor_end) = records.equal_range(BytePrefix{prefix});
}

DatabaseCursor::Status EncryptedDBCursor::Next(DataStream& key, DataStream& value)
{
    if (m_cursor == m_cursor_end) {
        return DatabaseCursor::Status::DONE;
    }
    key.clear();
    value.clear();

    const auto& [key_data, enc_key] = *m_cursor;
    key.write(key_data);

    if (!m_batch.ReadEncryptedKey(enc_key, value)) {
        return DatabaseCursor::Status::FAIL;
    }
    m_cursor++;
    return DatabaseCursor::Status::MORE;
}

std::unique_ptr<EncryptedDatabase> MakeEncryptedSQLiteDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error)
{
    std::unique_ptr<SQLiteDatabase> backing_db = MakeSQLiteDatabase(path, options, status, error);
    try {
        auto db = std::make_unique<EncryptedDatabase>(std::move(backing_db), options.db_passphrase, options.require_create);
        status = DatabaseStatus::SUCCESS;
        return db;
    } catch (const std::runtime_error& e) {
        status = DatabaseStatus::FAILED_LOAD;
        error = Untranslated(e.what());
        return nullptr;
    }
}

} // namespace wallet
