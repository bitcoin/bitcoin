// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/sqlite.h>

#include <util/memory.h>
#include <util/strencodings.h>
#include <util/translation.h>
#include <wallet/db.h>

#include <stdint.h>

static const char* const DATABASE_FILENAME = "wallet.dat";

SQLiteDatabase::SQLiteDatabase(const fs::path& dir_path, const fs::path& file_path, bool mock)
    : WalletDatabase(), m_mock(mock), m_dir_path(dir_path.string()), m_file_path(file_path.string())
{
}

SQLiteDatabase::~SQLiteDatabase()
{
}

void SQLiteDatabase::Open()
{
}

bool SQLiteDatabase::Rewrite(const char* skip)
{
    return false;
}

bool SQLiteDatabase::Backup(const std::string& dest) const
{
    return false;
}

void SQLiteDatabase::Close()
{
}

std::unique_ptr<DatabaseBatch> SQLiteDatabase::MakeBatch(bool flush_on_close)
{
    return nullptr;
}

void SQLiteBatch::Close()
{
}

bool SQLiteBatch::ReadKey(CDataStream&& key, CDataStream& value)
{
    return false;
}

bool SQLiteBatch::WriteKey(CDataStream&& key, CDataStream&& value, bool overwrite)
{
    return false;
}

bool SQLiteBatch::EraseKey(CDataStream&& key)
{
    return false;
}

bool SQLiteBatch::HasKey(CDataStream&& key)
{
    return false;
}

bool SQLiteBatch::StartCursor()
{
    return false;
}

bool SQLiteBatch::ReadAtCursor(CDataStream& key, CDataStream& value, bool& complete)
{
    return false;
}

void SQLiteBatch::CloseCursor()
{
}

bool SQLiteBatch::TxnBegin()
{
    return false;
}

bool SQLiteBatch::TxnCommit()
{
    return false;
}

bool SQLiteBatch::TxnAbort()
{
    return false;
}

bool ExistsSQLiteDatabase(const fs::path& path)
{
    return false;
}

std::unique_ptr<SQLiteDatabase> MakeSQLiteDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error)
{
    return MakeUnique<SQLiteDatabase>(path, path / DATABASE_FILENAME);
}
