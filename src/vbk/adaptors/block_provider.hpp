// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_BLOCK_PROVIDER_HPP
#define INTEGRATION_REFERENCE_BTC_BLOCK_PROVIDER_HPP

#include <dbwrapper.h>

#include "veriblock/storage/block_provider.hpp"

namespace VeriBlock {

namespace details {


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

template <typename BlockT>
struct BlockIterator : public altintegration::BlockIterator<BlockT> {
    using hash_t = typename BlockT::hash_t;

    ~BlockIterator() override = default;

    BlockIterator(std::shared_ptr<CDBIterator> iter) : iter_(iter) {}

    void next() override
    {
        iter_->Next();
    }

    bool value(altintegration::BlockIndex<BlockT>& out) const override
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


template <typename BlockT>
struct GenericBlockWriter : public altintegration::details::GenericBlockWriter<BlockT> {
    ~GenericBlockWriter() override = default;

    GenericBlockWriter(CDBWrapper& db) : db_(db) {}

    void prepareBatch(CDBBatch* batch)
    {
        batch_ = batch;
    }

    void closeBatch()
    {
        batch_ = nullptr;
    }

    bool writeBlock(const altintegration::BlockIndex<BlockT>& value) override
    {
        if (batch_) {
            batch_->Write(block_key<BlockT>(value.getHash()), value);
        } else {
            return db_.Write(block_key<BlockT>(value.getHash()), value);
        }
        return true;
    }

    bool writeTip(const altintegration::BlockIndex<BlockT>& value) override
    {
        if (batch_) {
            batch_->Write(tip_key<BlockT>(), value.getHash());
        } else {
            return db_.Write(tip_key<BlockT>(), value.getHash());
        }
        return true;
    }


private:
    CDBWrapper& db_;
    CDBBatch* batch_{nullptr};
};

template <typename BlockT>
struct GenericBlockReader : public altintegration::details::GenericBlockReader<BlockT> {
    using hash_t = typename BlockT::hash_t;

    ~GenericBlockReader() override = default;

    GenericBlockReader(CDBWrapper& db) : db_(db) {}

    bool getTipHash(hash_t& out) const override
    {
        return db_.Read(tip_key<BlockT>(), out);
    }

    bool getBlock(const hash_t& hash, altintegration::BlockIndex<BlockT>& out) const override
    {
        return db_.Read(block_key<BlockT>(hash), out);
    }

    std::shared_ptr<altintegration::BlockIterator<BlockT>> getBlockIterator() const override
    {
        std::shared_ptr<CDBIterator> it(db_.NewIterator());
        return std::make_shared<BlockIterator<BlockT>>(it);
    }

private:
    CDBWrapper& db_;
};

} // namespace details

struct BlockProvider : public altintegration::BlockProvider {
    ~BlockProvider() override = default;

    BlockProvider(CDBWrapper& db) : btc_reader(std::make_shared<details::GenericBlockReader<altintegration::BtcBlock>>(db)),
                                    vbk_reader(std::make_shared<details::GenericBlockReader<altintegration::VbkBlock>>(db)),
                                    alt_reader(std::make_shared<details::GenericBlockReader<altintegration::AltBlock>>(db)),
                                    btc_writer(std::make_shared<details::GenericBlockWriter<altintegration::BtcBlock>>(db)),
                                    vbk_writer(std::make_shared<details::GenericBlockWriter<altintegration::VbkBlock>>(db)),
                                    alt_writer(std::make_shared<details::GenericBlockWriter<altintegration::AltBlock>>(db)) {}

    void prepareBatch(CDBBatch* batch)
    {
        alt_writer->prepareBatch(batch);
        vbk_writer->prepareBatch(batch);
        btc_writer->prepareBatch(batch);
    }

    void closeBatch()
    {
        alt_writer->closeBatch();
        vbk_writer->closeBatch();
        btc_writer->closeBatch();
    }

    std::shared_ptr<altintegration::details::GenericBlockReader<altintegration::AltBlock>>
    getAltBlockReader() const override
    {
        return alt_reader;
    }

    std::shared_ptr<altintegration::details::GenericBlockReader<altintegration::VbkBlock>>
    getVbkBlockReader() const override
    {
        return vbk_reader;
    }

    std::shared_ptr<altintegration::details::GenericBlockReader<altintegration::BtcBlock>>
    getBtcBlockReader() const override
    {
        return btc_reader;
    }


    std::shared_ptr<altintegration::details::GenericBlockWriter<altintegration::AltBlock>>
    getAltBlockWriter() const override
    {
        return alt_writer;
    }

    std::shared_ptr<altintegration::details::GenericBlockWriter<altintegration::VbkBlock>>
    getVbkBlockWriter() const override
    {
        return vbk_writer;
    }

    std::shared_ptr<altintegration::details::GenericBlockWriter<altintegration::BtcBlock>>
    getBtcBlockWriter() const override
    {
        return btc_writer;
    }


private:
    std::shared_ptr<details::GenericBlockReader<altintegration::BtcBlock>> btc_reader;
    std::shared_ptr<details::GenericBlockReader<altintegration::VbkBlock>> vbk_reader;
    std::shared_ptr<details::GenericBlockReader<altintegration::AltBlock>> alt_reader;

    std::shared_ptr<details::GenericBlockWriter<altintegration::BtcBlock>> btc_writer;
    std::shared_ptr<details::GenericBlockWriter<altintegration::VbkBlock>> vbk_writer;
    std::shared_ptr<details::GenericBlockWriter<altintegration::AltBlock>> alt_writer;
};

} // namespace VeriBlock

#endif