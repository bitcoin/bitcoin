#include <mw/db/LeafDB.h>
#include "common/Database.h"
#include "common/SerializableVec.h"

LeafDB::LeafDB(const char prefix, mw::DBWrapper* pDBWrapper, mw::DBBatch* pBatch)
    : m_prefix(prefix), m_pDatabase(std::make_unique<Database>(pDBWrapper, pBatch))
{
}

LeafDB::~LeafDB() {}

std::unique_ptr<mmr::Leaf> LeafDB::Get(const mmr::LeafIndex& idx) const
{
    auto pVec = m_pDatabase->Get<SerializableVec>(m_prefix, std::to_string(idx.Get()));
    if (pVec == nullptr) {
        return nullptr;
    }

    return std::make_unique<mmr::Leaf>(mmr::Leaf::Create(idx, pVec->item->Get()));
}

void LeafDB::Add(const std::vector<mmr::Leaf>& leaves)
{
    if (leaves.empty()) {
        return;
    }

    std::vector<DBEntry<SerializableVec>> entries;
    std::transform(
        leaves.cbegin(), leaves.cend(), std::back_inserter(entries),
        [](const mmr::Leaf& leaf) {
            return DBEntry<SerializableVec>(std::to_string(leaf.GetLeafIndex().Get()), leaf.vec());
        }
    );
    m_pDatabase->Put(m_prefix, entries);
}

void LeafDB::Remove(const std::vector<mmr::LeafIndex>& indices)
{
    for (const mmr::LeafIndex& idx : indices) {
        m_pDatabase->Delete(m_prefix, std::to_string(idx.Get()));
    }
}

void LeafDB::RemoveAll()
{
    m_pDatabase->DeleteAll(m_prefix);
}