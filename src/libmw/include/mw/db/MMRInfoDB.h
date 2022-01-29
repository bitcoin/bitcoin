#pragma once

#include <mw/mmr/MMRInfo.h>
#include <mw/interfaces/db_interface.h>

// Forward Declarations
class Database;

class MMRInfoDB
{
public:
    MMRInfoDB(mw::DBWrapper* pDBWrapper, mw::DBBatch* pBatch = nullptr);
    ~MMRInfoDB();

    /// <summary>
    /// Retrieves an MMRInfo by index.
    /// </summary>
    /// <param name="index">The index of the MMRInfo to retrieve.</param>
    /// <returns>The MMRInfo for the given index. nullptr if not found.</returns>
    std::unique_ptr<MMRInfo> GetByIndex(const uint32_t index) const;

    /// <summary>
    /// Retrieves the latest MMRInfo.
    /// </summary>
    /// <returns>The latest MMRInfo. nullptr if none are found.</returns>
    std::unique_ptr<MMRInfo> GetLatest() const;

    /// <summary>
    /// Saves by index and also saves the entry as the latest.
    /// </summary>
    /// <param name="info">The MMR info to save</param>
    void Save(const MMRInfo& info);

private:
    std::unique_ptr<Database> m_pDatabase;
};