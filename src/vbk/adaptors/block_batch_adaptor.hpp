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

template <typename BlockT>
std::pair<char, std::string> tip_key();

template <>
inline std::pair<char, std::string> tip_key<altintegration::VbkBlock>()
{
    return std::make_pair(DB_VBK_TIP, "vbktip");
}
template <>
inline std::pair<char, std::string> tip_key<altintegration::BtcBlock>()
{
    return std::make_pair(DB_BTC_TIP, "btctip");
}
template <>
inline std::pair<char, std::string> tip_key<altintegration::AltBlock>()
{
    return std::make_pair(DB_ALT_TIP, "alttip");
}

template <typename BlockT>
std::pair<char, typename BlockT::hash_t> block_key(const typename BlockT::hash_t& hash);


template <>
inline std::pair<char, typename altintegration::BtcBlock::hash_t> block_key<altintegration::BtcBlock>(const typename altintegration::BtcBlock::hash_t& hash)
{
    return std::make_pair(DB_BTC_BLOCK, hash);
}

template <>
inline std::pair<char, typename altintegration::VbkBlock::hash_t> block_key<altintegration::VbkBlock>(const typename altintegration::VbkBlock::hash_t& hash)
{
    return std::make_pair(DB_VBK_BLOCK, hash);
}

template <>
inline std::pair<char, typename altintegration::AltBlock::hash_t> block_key<altintegration::AltBlock>(const typename altintegration::AltBlock::hash_t& hash)
{
    return std::make_pair(DB_ALT_BLOCK, hash);
}


struct BlockBatchAdaptor : public altintegration::BlockBatchAdaptor {
    ~BlockBatchAdaptor() override = default;

    explicit BlockBatchAdaptor(CDBBatch& batch) : batch_(batch)
    {
    }

    bool writeBlock(const altintegration::BlockIndex<altintegration::BtcBlock>& value) override
    {
        batch_.Write(block_key<altintegration::BtcBlock>(getHash(value)), value);
        return true;
    };
    bool writeBlock(const altintegration::BlockIndex<altintegration::VbkBlock>& value) override
    {
        batch_.Write(block_key<altintegration::VbkBlock>(getHash(value)), value);
        return true;
    };
    bool writeBlock(const altintegration::BlockIndex<altintegration::AltBlock>& value) override
    {
        batch_.Write(block_key<altintegration::AltBlock>(getHash(value)), value);
        return true;
    };

    bool writeTip(const altintegration::BlockIndex<altintegration::BtcBlock>& value) override
    {
        batch_.Write(tip_key<altintegration::BtcBlock>(), getHash(value));
        return true;
    };
    bool writeTip(const altintegration::BlockIndex<altintegration::VbkBlock>& value) override
    {
        batch_.Write(tip_key<altintegration::VbkBlock>(), getHash(value));
        return true;
    };
    bool writeTip(const altintegration::BlockIndex<altintegration::AltBlock>& value) override
    {
        batch_.Write(tip_key<altintegration::AltBlock>(), getHash(value));
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
