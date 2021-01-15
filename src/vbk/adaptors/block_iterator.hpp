// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_BLOCK_HASH_ITERATOR_HPP
#define INTEGRATION_REFERENCE_BTC_BLOCK_HASH_ITERATOR_HPP

#include <dbwrapper.h>

namespace VeriBlock {

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

} // namespace VeriBlock


#endif