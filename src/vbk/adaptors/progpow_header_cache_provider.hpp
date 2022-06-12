// Copyright (c) 2019-2021 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_PROGPOW_HEADER_CACHE_PROVIDER_HPP
#define INTEGRATION_REFERENCE_BTC_PROGPOW_HEADER_CACHE_PROVIDER_HPP

#include <dbwrapper.h>
#include <vbk/pop_common.hpp>
#include <veriblock/pop.hpp>

namespace VeriBlock {

const char DB_PROGPOW_HEADER_PREFIX = '!';

inline std::vector<uint8_t> key_bytes(const altintegration::uint256& key)
{
    auto res = key.asVector();
    res.insert(res.begin(), DB_PROGPOW_HEADER_PREFIX);
    return res;
}


struct ProgpowHeaderCache : public altintegration::ProgpowHeaderCache {
    ~ProgpowHeaderCache() override = default;

    ProgpowHeaderCache(CDBWrapper& db) : db_(db) {}

    bool get(const altintegration::uint256& key, altintegration::uint192& value) const override
    {
        std::vector<uint8_t> bytes_out;
        if (!db_.Read(key_bytes(key), bytes_out)) {
            return false;
        }
        value = bytes_out;

        return true;
    }

    void insert(const altintegration::uint256& key, altintegration::uint192 value) override
    {
         db_.Write(key_bytes(key), value.asVector());
    }

private:
    CDBWrapper& db_;
};

} // namespace VeriBlock

#endif