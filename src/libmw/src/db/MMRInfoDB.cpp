#include <mw/db/MMRInfoDB.h>
#include "common/Database.h"
#include <limits>

static const DBTable MMR_TABLE = { 'M' };
static const uint32_t LATEST_INDEX = std::numeric_limits<uint32_t>::max();

MMRInfoDB::MMRInfoDB(mw::DBWrapper* pDBWrapper, mw::DBBatch* pBatch)
    : m_pDatabase(std::make_unique<Database>(pDBWrapper, pBatch)) { }

MMRInfoDB::~MMRInfoDB() { }

std::unique_ptr<MMRInfo> MMRInfoDB::GetByIndex(const uint32_t index) const
{
    auto pEntry = m_pDatabase->Get<MMRInfo>(MMR_TABLE, std::to_string(index));
    return pEntry != nullptr ? std::make_unique<MMRInfo>(*pEntry->item) : nullptr;
}

std::unique_ptr<MMRInfo> MMRInfoDB::GetLatest() const
{
    return GetByIndex(LATEST_INDEX);
}

void MMRInfoDB::Save(const MMRInfo& info)
{
    std::vector<DBEntry<MMRInfo>> entries{
        DBEntry<MMRInfo>(std::to_string(info.index), info),
        DBEntry<MMRInfo>(std::to_string(LATEST_INDEX), info)
    };

    m_pDatabase->Put(MMR_TABLE, entries);
}