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
using altintegration::BtcBlock;
using altintegration::StoredBlockIndex;
using altintegration::VbkBlock;

constexpr const char DB_BTC_BLOCK = 'Q';
constexpr const char DB_BTC_PREV_BLOCK = 'R';
constexpr const char DB_BTC_TIP = 'q';
constexpr const char DB_VBK_BLOCK = 'W';
constexpr const char DB_VBK_PREV_BLOCK = 'T';
constexpr const char DB_VBK_TIP = 'w';
constexpr const char DB_ALT_BLOCK = 'E';
constexpr const char DB_ALT_PREV_BLOCK = 'Y';
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
std::pair<char, typename BlockT::prev_hash_t> block_prev_key(const typename BlockT::prev_hash_t& hash);


template <>
inline std::pair<char, typename BtcBlock::prev_hash_t> block_prev_key<BtcBlock>(const typename BtcBlock::prev_hash_t& hash)
{
    return std::make_pair(DB_BTC_PREV_BLOCK, hash);
}

template <>
inline std::pair<char, typename VbkBlock::prev_hash_t> block_prev_key<VbkBlock>(const typename VbkBlock::prev_hash_t& hash)
{
    return std::make_pair(DB_VBK_PREV_BLOCK, hash);
}

template <>
inline std::pair<char, typename AltBlock::prev_hash_t> block_prev_key<AltBlock>(const typename AltBlock::prev_hash_t& hash)
{
    return std::make_pair(DB_ALT_PREV_BLOCK, hash);
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

    bool value(StoredBlockIndex<BlockT>& out) const override
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
        static const auto key = block_key<BlockT>({});

        iter_->Seek(key);
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

    bool getBlock(const AltBlock::prev_hash_t& prev_hash,
        StoredBlockIndex<AltBlock>& out) const override
    {
        AltBlock::hash_t hash;
        return db->Read(block_prev_key<AltBlock>(prev_hash), hash) && db->Read(block_key<AltBlock>(hash), out);
    }

    bool getBlock(const VbkBlock::prev_hash_t& prev_hash,
        StoredBlockIndex<VbkBlock>& out) const override
    {
        VbkBlock::hash_t hash;
        return db->Read(block_prev_key<VbkBlock>(prev_hash), hash) && db->Read(block_key<VbkBlock>(hash), out);
    }

    bool getBlock(const BtcBlock::prev_hash_t& prev_hash,
        StoredBlockIndex<BtcBlock>& out) const override
    {
        BtcBlock::hash_t hash;
        return db->Read(block_prev_key<BtcBlock>(prev_hash), hash) && db->Read(block_key<BtcBlock>(hash), out);
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

    void writeBlock(const AltBlock::hash_t& hash, const AltBlock::prev_hash_t& prev_hash, const StoredBlockIndex<AltBlock>& value) override
    {
        batch_->Write(block_prev_key<AltBlock>(prev_hash), hash);
        batch_->Write(block_key<AltBlock>(hash), value);
    }

    void writeBlock(const VbkBlock::hash_t& hash, const VbkBlock::prev_hash_t& prev_hash, const StoredBlockIndex<VbkBlock>& value) override
    {
        batch_->Write(block_prev_key<VbkBlock>(prev_hash), hash);
        batch_->Write(block_key<VbkBlock>(hash), value);
    }

    void writeBlock(const BtcBlock::hash_t& hash, const BtcBlock::prev_hash_t& prev_hash, const StoredBlockIndex<BtcBlock>& value) override
    {
        batch_->Write(block_prev_key<BtcBlock>(prev_hash), hash);
        batch_->Write(block_key<BtcBlock>(hash), value);
    }

    void writeTip(const AltBlock::hash_t& hash) override
    {
        batch_->Write(tip_key<AltBlock>(), hash);
    }

    void writeTip(const VbkBlock::hash_t& hash) override
    {
        batch_->Write(tip_key<VbkBlock>(), hash);
    }

    void writeTip(const BtcBlock::hash_t& hash) override
    {
        batch_->Write(tip_key<BtcBlock>(), hash);
    }

private:
    CDBBatch* batch_{nullptr};
};

} // namespace VeriBlock

#endif