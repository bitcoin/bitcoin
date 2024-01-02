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
    return false;
}

bool BerkeleyROBatch::HasKey(DataStream&& key)
{
    return false;
}

DatabaseCursor::Status BerkeleyROCursor::Next(DataStream& ssKey, DataStream& ssValue)
{
    return DatabaseCursor::Status::FAIL;
}

} // namespace wallet
