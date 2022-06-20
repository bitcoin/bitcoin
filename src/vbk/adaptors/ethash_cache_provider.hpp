// Copyright (c) 2019-2021 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_ETHASH_CACHE_PROVIDER_HPP
#define INTEGRATION_REFERENCE_BTC_ETHASH_CACHE_PROVIDER_HPP

#include <dbwrapper.h>
#include <vbk/pop_common.hpp>
#include <veriblock/pop.hpp>

namespace VeriBlock {

const char DB_ETHASH_PREFIX = '&';

inline std::vector<uint8_t> epoch_bytes(uint64_t epoch)
{
    altintegration::WriteStream write;
    write.writeBE(epoch);
    auto res = write.data();
    res.insert(res.begin(), DB_ETHASH_PREFIX);
    return res;
}

struct EthashCache : public altintegration::EthashCache {
    ~EthashCache() override = default;

    EthashCache(CDBWrapper& db) : db_(db) {}

    bool get(uint64_t epoch, std::shared_ptr<altintegration::CacheEntry> out) const override
    {
        std::vector<uint8_t> bytes_out;
        if (!db_.Read(epoch_bytes(epoch), bytes_out)) {
            return false;
        }

        altintegration::ReadStream read(bytes_out);
        altintegration::ValidationState state;
        if (!altintegration::DeserializeFromVbkEncoding(read, *out, state)) {
            return false;
        }

        return true;
    }

    void insert(uint64_t epoch, std::shared_ptr<altintegration::CacheEntry> value) override
    {
        altintegration::WriteStream write;
        value->toVbkEncoding(write);

        db_.Write(epoch_bytes(epoch), write.data());
    }

private:
    CDBWrapper& db_;
};

} // namespace VeriBlock

#endif