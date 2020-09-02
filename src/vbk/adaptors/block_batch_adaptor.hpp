// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_BLOCK_BATCH_ADAPTOR_HPP
#define INTEGRATION_REFERENCE_BTC_BLOCK_BATCH_ADAPTOR_HPP

#include <dbwrapper.h>
#include <veriblock/storage/block_batch_adaptor.hpp>

namespace VeriBlock {

constexpr const char DB_BTC_BLOCK = 'Q';
constexpr const char DB_BTC_TIP = 'q';
constexpr const char DB_VBK_BLOCK = 'W';
constexpr const char DB_VBK_TIP = 'w';
constexpr const char DB_ALT_BLOCK = 'E';
constexpr const char DB_ALT_TIP = 'e';

struct BlockBatchAdaptor : public altintegration::BlockBatchAdaptor {
    ~BlockBatchAdaptor() override = default;

    static std::pair<char, std::string> vbktip()
    {
        return std::make_pair(DB_VBK_TIP, "vbktip");
    }
    static std::pair<char, std::string> btctip()
    {
        return std::make_pair(DB_BTC_TIP, "btctip");
    }
    static std::pair<char, std::string> alttip()
    {
        return std::make_pair(DB_ALT_TIP, "alttip");
    }

    explicit BlockBatchAdaptor(CDBBatch& batch) : batch_(batch)
    {
    }

    bool writeBlock(const altintegration::BlockIndex<altintegration::BtcBlock>& value) override
    {
        batch_.Write(std::make_pair(DB_BTC_BLOCK, getHash(value)), value);
        return true;
    };
    bool writeBlock(const altintegration::BlockIndex<altintegration::VbkBlock>& value) override
    {
        batch_.Write(std::make_pair(DB_VBK_BLOCK, getHash(value)), value);
        return true;
    };
    bool writeBlock(const altintegration::BlockIndex<altintegration::AltBlock>& value) override
    {
        batch_.Write(std::make_pair(DB_ALT_BLOCK, getHash(value)), value);
        return true;
    };

    bool writeTip(const altintegration::BlockIndex<altintegration::BtcBlock>& value) override
    {
        batch_.Write(btctip(), getHash(value));
        return true;
    };
    bool writeTip(const altintegration::BlockIndex<altintegration::VbkBlock>& value) override
    {
        batch_.Write(vbktip(), getHash(value));
        return true;
    };
    bool writeTip(const altintegration::BlockIndex<altintegration::AltBlock>& value) override
    {
        batch_.Write(alttip(), getHash(value));
        return true;
    };


private:
    CDBBatch& batch_;

    template <typename T>
    typename T::hash_t getHash(const T& c)
    {
        return c.getHash();
    }
};

} // namespace VeriBlock

#endif //INTEGRATION_REFERENCE_BTC_BLOCK_BATCH_ADAPTOR_HPP
