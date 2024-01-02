// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/migrate.h>

namespace wallet {

void BerkeleyRODatabase::Open()
{
}

std::unique_ptr<DatabaseBatch> BerkeleyRODatabase::MakeBatch(bool flush_on_close)
{
    return std::make_unique<BerkeleyROBatch>(*this);
}

bool BerkeleyRODatabase::Backup(const std::string& dest) const
{
    return false;
}

bool BerkeleyROBatch::ReadKey(DataStream&& key, DataStream& value)
{
    SerializeData key_data{key.begin(), key.end()};
    const auto it{m_database.m_records.find(key_data)};
    if (it == m_database.m_records.end()) {
        return false;
    }
    auto val = it->second;
    value.clear();
    value.write(Span(val));
    return true;
}

bool BerkeleyROBatch::HasKey(DataStream&& key)
{
    SerializeData key_data{key.begin(), key.end()};
    return m_database.m_records.count(key_data) > 0;
}

BerkeleyROCursor::BerkeleyROCursor(const BerkeleyRODatabase& database, Span<const std::byte> prefix)
    : m_database(database)
{
    std::tie(m_cursor, m_cursor_end) = m_database.m_records.equal_range(BytePrefix{prefix});
}

DatabaseCursor::Status BerkeleyROCursor::Next(DataStream& ssKey, DataStream& ssValue)
{
    if (m_cursor == m_cursor_end) {
        return DatabaseCursor::Status::DONE;
    }
    ssKey.write(Span(m_cursor->first));
    ssValue.write(Span(m_cursor->second));
    m_cursor++;
    return DatabaseCursor::Status::MORE;
}

std::unique_ptr<DatabaseCursor> BerkeleyROBatch::GetNewPrefixCursor(Span<const std::byte> prefix)
{
    return std::make_unique<BerkeleyROCursor>(m_database, prefix);
}
} // namespace wallet
