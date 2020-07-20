// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_REPOSITORY_HPP
#define INTEGRATION_REFERENCE_BTC_REPOSITORY_HPP

#include <veriblock/storage/repository.hpp>

#include "batch.hpp"
#include "cursor.hpp"

namespace VeriBlock {

struct Repository : public altintegration::Repository
{
    using base = altintegration::Repository;

    ~Repository() = default;

    Repository(CDBWrapper& db) : db_(db) {}

    bool remove(const key_t& id)
    {
        return db_.Erase(id, true);
    };

    //! returns true if operation is successful, false otherwise
    bool put(const key_t& key, const value_t& value)
    {
        return db_.Write(key, value, true);
    }

    //! returns true if key exists, false otherwise/on error
    bool get(const key_t& key, value_t* value) const
    {
        if (value) {
            return db_.Read(key, *value);
        } else {
            return db_.Exists(key);
        }
    }

    std::unique_ptr<batch_t> newBatch()
    {
        CDBBatch batch(db_);
        return MakeUnique<Batch>(db_, std::move(batch));
    }

    std::shared_ptr<cursor_t> newCursor() const
    {
        return std::make_shared<Cursor>(db_.NewIterator());
    }

private:
    CDBWrapper& db_;
};

} // namespace VeriBlock

#endif //INTEGRATION_REFERENCE_BTC_REPOSITORY_HPP
