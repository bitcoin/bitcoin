// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_BATCH_HPP
#define INTEGRATION_REFERENCE_BTC_BATCH_HPP

#include "dbwrapper.h"
#include <veriblock/storage/batch.hpp>

namespace VeriBlock {

struct Batch : public altintegration::Batch {
    using key_t = std::vector<uint8_t>;
    using value_t = std::vector<uint8_t>;

    ~Batch() override = default;

    Batch(CDBWrapper& db, CDBBatch&& batch) : db_(db), batch_(std::move(batch)) {}

    /**
     * Write a single KV. If Key exists, it will be overwritten with Value.
     */
    void put(const key_t& key, const value_t& value) override
    {
        batch_.Write(key, value);
    };

    /**
     * Efficiently commit given batch. Clears batch. Throws on failure.
     */
    void commit() override
    {
        db_.WriteBatch(batch_, true);
        batch_.Clear();
    };

private:
    CDBWrapper& db_;
    CDBBatch batch_;
};

} // namespace VeriBlock

#endif //INTEGRATION_REFERENCE_BTC_BATCH_HPP
