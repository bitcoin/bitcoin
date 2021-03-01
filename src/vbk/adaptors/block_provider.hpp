// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_BLOCK_PROVIDER_HPP
#define INTEGRATION_REFERENCE_BTC_BLOCK_PROVIDER_HPP

#include <dbwrapper.h>

#include <utility>

#include <veriblock/pop.hpp>

namespace VeriBlock {

using altintegration::AltBlock;
using altintegration::BlockIndex;
using altintegration::BtcBlock;
using altintegration::VbkBlock;

constexpr const char DB_BTC_BLOCK = 'Q';
constexpr const char DB_BTC_TIP = 'q';
constexpr const char DB_VBK_BLOCK = 'W';
constexpr const char DB_VBK_TIP = 'w';
constexpr const char DB_ALT_BLOCK = 'E';
constexpr const char DB_ALT_TIP = 'e';

template <typename BlockT>
std::pair<char, std::string> tip_key();

template <>
inline std::pair<char, std::string> tip_key<VbkBlock>()
{
    return std::make_pair(DB_VBK_TIP, "vbktip");
}
template <>
inline std::pair<char, std::string> tip_key<BtcBlock>()
{
    return std::make_pair(DB_BTC_TIP, "btctip");
}
template <>
inline std::pair<char, std::string> tip_key<AltBlock>()
{
    return std::make_pair(DB_ALT_TIP, "alttip");
}

template <typename BlockT>
std::pair<char, typename BlockT::hash_t> block_key(const typename BlockT::hash_t& hash);


template <>
inline std::pair<char, typename BtcBlock::hash_t> block_key<BtcBlock>(const typename BtcBlock::hash_t& hash)
{
    return std::make_pair(DB_BTC_BLOCK, hash);
}

template <>
inline std::pair<char, typename VbkBlock::hash_t> block_key<VbkBlock>(const typename VbkBlock::hash_t& hash)
{
    return std::make_pair(DB_VBK_BLOCK, hash);
}

template <>
inline std::pair<char, typename AltBlock::hash_t> block_key<AltBlock>(const typename AltBlock::hash_t& hash)
{
    return std::make_pair(DB_ALT_BLOCK, hash);
}


template <typename BlockT>
struct BlockIterator : public altintegration::BlockIterator<BlockT> {
    using hash_t = typename BlockT::hash_t;

    ~BlockIterator() override = default;

    BlockIterator(std::shared_ptr<CDBIterator> iter) : iter_(std::move(iter)) {}

    void next() override
    {
        iter_->Next();
    }

    bool value(BlockIndex<BlockT>& out) const override
    {
        return iter_->GetValue(out);
    }

    bool key(hash_t& out) const override
    {
        std::pair<char, hash_t> key;
        if (!iter_->GetKey(key)) {
            return false;
        }
        out = key.second;
        return true;
    }

    bool valid() const override
    {
        static char prefix = block_key<BlockT>(hash_t()).first;

        std::pair<char, hash_t> key;
        return iter_->Valid() && iter_->GetKey(key) && key.first == prefix;
    }

    void seek_start() override
    {
        iter_->Seek(block_key<BlockT>(hash_t()));
    }

private:
    std::shared_ptr<CDBIterator> iter_;
};

struct BlockReader : public altintegration::BlockReader {
    ~BlockReader() override = default;

    BlockReader(CDBWrapper& db) : db(&db) {}

    bool getAltTip(AltBlock::hash_t& out) const override
    {
        return db->Read(tip_key<AltBlock>(), out);
    }
    bool getVbkTip(VbkBlock::hash_t& out) const override
    {
        return db->Read(tip_key<VbkBlock>(), out);
    }
    bool getBtcTip(BtcBlock::hash_t& out) const override
    {
        return db->Read(tip_key<BtcBlock>(), out);
    }

    std::shared_ptr<altintegration::BlockIterator<AltBlock>> getAltBlockIterator() const override
    {
        std::shared_ptr<CDBIterator> it(db->NewIterator());
        return std::make_shared<BlockIterator<AltBlock>>(it);
    }
    std::shared_ptr<altintegration::BlockIterator<VbkBlock>> getVbkBlockIterator() const override
    {
        std::shared_ptr<CDBIterator> it(db->NewIterator());
        return std::make_shared<BlockIterator<VbkBlock>>(it);
    }
    std::shared_ptr<altintegration::BlockIterator<BtcBlock>> getBtcBlockIterator() const override
    {
        std::shared_ptr<CDBIterator> it(db->NewIterator());
        return std::make_shared<BlockIterator<BtcBlock>>(it);
    }

private:
    CDBWrapper* db{nullptr};
};


struct BlockBatch : public altintegration::BlockBatch {
    ~BlockBatch() override = default;

    BlockBatch(CDBBatch& batch) : batch_(&batch) {}

    void writeBlock(const BlockIndex<AltBlock>& value) override
    {
        auto key = block_key<AltBlock>(value.getHash());
        batch_->Write(key, value);
    }

    void writeBlock(const BlockIndex<VbkBlock>& value) override
    {
        auto key = block_key<VbkBlock>(value.getHash());
        batch_->Write(key, value);
    }

    void writeBlock(const BlockIndex<BtcBlock>& value) override
    {
        auto key = block_key<BtcBlock>(value.getHash());
        batch_->Write(key, value);
    }

    void writeTip(const BlockIndex<AltBlock>& value) override
    {
        auto hash = value.getHash();
        batch_->Write(tip_key<AltBlock>(), hash);
    }

    void writeTip(const BlockIndex<VbkBlock>& value) override
    {
        auto hash = value.getHash();
        batch_->Write(tip_key<VbkBlock>(), hash);
    }

    void writeTip(const BlockIndex<BtcBlock>& value) override
    {
        auto hash = value.getHash();
        batch_->Write(tip_key<BtcBlock>(), hash);
    }

private:
    CDBBatch* batch_{nullptr};
};

} // namespace VeriBlock

#endif