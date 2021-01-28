// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_PAYLOADS_PROVIDER_HPP
#define INTEGRATION_REFERENCE_BTC_PAYLOADS_PROVIDER_HPP

#include <dbwrapper.h>
#include <vbk/pop_common.hpp>
#include <veriblock/storage/payloads_provider.hpp>

namespace VeriBlock {

constexpr const char DB_VBK_PREFIX = '^';
constexpr const char DB_VTB_PREFIX = '<';
constexpr const char DB_ATV_PREFIX = '>';

struct PayloadsProvider : public altintegration::PayloadsStorage {
    using key_t = std::vector<uint8_t>;
    using value_t = std::vector<uint8_t>;

    ~PayloadsProvider() = default;

    PayloadsProvider(CDBWrapper& db) : db_(db) {}

    void writePayloads(const altintegration::PopData& payloads) override
    {
        auto batch = CDBBatch(db_);
        for (const auto& b : payloads.context) {
            batch.Write(std::make_pair(DB_VBK_PREFIX, b.getId()), b);
        }
        for (const auto& b : payloads.vtbs) {
            batch.Write(std::make_pair(DB_VTB_PREFIX, b.getId()), b);
        }
        for (const auto& b : payloads.atvs) {
            batch.Write(std::make_pair(DB_ATV_PREFIX, b.getId()), b);
        }
        bool ret = db_.WriteBatch(batch, true);
        VBK_ASSERT_MSG(ret, "payloads write batch failed");
        batch.Clear();
    }

    template <typename pop_t>
    bool getPayloads(char dbPrefix, const typename pop_t::id_t& id, pop_t& out, altintegration::ValidationState& state)
    {
        auto& mempool = *GetPop().mempool;
        const auto* memval = mempool.template get<pop_t>(id);
        if (memval != nullptr) {
            out = *memval;
        } else {
            if (!db_.Read(std::make_pair(dbPrefix, id), out)) {
                return state.Invalid(pop_t::name() + "-read-error");
            }
        }
        return true;
    }

    bool getATV(const altintegration::ATV::id_t& id, altintegration::ATV& out, altintegration::ValidationState& state) override
    {
        return getPayloads(DB_ATV_PREFIX, id, out, state);
    }

    bool getVTB(const altintegration::VTB::id_t& id, altintegration::VTB& out, altintegration::ValidationState& state) override
    {
        return getPayloads(DB_VTB_PREFIX, id, out, state);
    }

    bool getVBK(const altintegration::VbkBlock::id_t& id, altintegration::VbkBlock& out, altintegration::ValidationState& state) override
    {
        return getPayloads(DB_VBK_PREFIX, id, out, state);
    }

private:
    CDBWrapper& db_;
};

} // namespace VeriBlock

#endif //INTEGRATION_REFERENCE_BTC_PAYLOADS_PROVIDER_HPP
