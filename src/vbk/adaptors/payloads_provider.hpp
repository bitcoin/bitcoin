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

struct PayloadsProvider : public altintegration::PayloadsProvider {
    using base = altintegration::PayloadsProvider;
    using key_t = std::vector<uint8_t>;
    using value_t = std::vector<uint8_t>;

    ~PayloadsProvider() = default;

    PayloadsProvider(CDBWrapper& db) : db_(db) {}

    void write(const altintegration::PopData& pop)
    {
        auto batch = CDBBatch(db_);
        for (const auto& b : pop.context) {
            batch.Write(std::make_pair(DB_VBK_PREFIX, b.getId()), b);
        }
        for (const auto& b : pop.vtbs) {
            batch.Write(std::make_pair(DB_VTB_PREFIX, b.getId()), b);
        }
        for (const auto& b : pop.atvs) {
            batch.Write(std::make_pair(DB_ATV_PREFIX, b.getId()), b);
        }
        bool ret = db_.WriteBatch(batch, true);
        VBK_ASSERT_MSG(ret, "payloads write batch failed");
        batch.Clear();
    }

    template <typename pop_t>
    bool getPayloads(char dbPrefix, const std::vector<typename pop_t::id_t>& ids, std::vector<pop_t>& out, altintegration::ValidationState& state)
    {
        auto& mempool = *GetPop().mempool;
        out.reserve(ids.size());
        for (size_t i = 0; i < ids.size(); i++) {
            pop_t value;
            const auto* memval = mempool.get<pop_t>(ids[i]);
            if (memval != nullptr) {
                value = *memval;
            } else {
                if (!db_.Read(std::make_pair(dbPrefix, ids[i]), value)) {
                    return state.Invalid(pop_t::name() + "-read-error", i);
                }
            }
            out.push_back(value);
        }
        return true;
    }

    bool getATVs(const std::vector<altintegration::ATV::id_t>& ids,
        std::vector<altintegration::ATV>& out,
        altintegration::ValidationState& state) override
    {
        return getPayloads(DB_ATV_PREFIX, ids, out, state);
    }

    bool getVTBs(const std::vector<altintegration::VTB::id_t>& ids,
        std::vector<altintegration::VTB>& out,
        altintegration::ValidationState& state) override
    {
        return getPayloads(DB_VTB_PREFIX, ids, out, state);
    }

    bool getVBKs(const std::vector<altintegration::VbkBlock::id_t>& ids,
        std::vector<altintegration::VbkBlock>& out,
        altintegration::ValidationState& state) override
    {
        return getPayloads(DB_VBK_PREFIX, ids, out, state);
    }

private:
    CDBWrapper& db_;
};

} // namespace VeriBlock

#endif //INTEGRATION_REFERENCE_BTC_PAYLOADS_PROVIDER_HPP
