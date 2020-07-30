// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_CURSOR_HPP
#define INTEGRATION_REFERENCE_BTC_CURSOR_HPP

#include "dbwrapper.h"
#include <veriblock/storage/cursor.hpp>


namespace VeriBlock {

struct Cursor : public altintegration::Cursor<std::vector<uint8_t>, std::vector<uint8_t>> {
    ~Cursor() = default;

    Cursor(CDBIterator* iter) : iter_(iter) {}

    /**
     * Move cursor to the first key.
     */
    void seekToFirst()
    {
        iter_->SeekToFirst();
    };

    /**
     * Move cursor to specific key
     * @param key
     */
    void seek(const std::vector<uint8_t>& key)
    {
        iter_->Seek(key);
    };

    /**
     * Move cursor to the last key
     */
    void seekToLast()
    {
        assert(false && "not implemented");
    };

    /**
     * Does cursor point on a Key-Value pair?
     * @return true if cursor points to element of map, false otherwise
     */
    bool isValid() const
    {
        return iter_->Valid();
    };

    /**
     * Move cursor to the next element.
     */
    void next()
    {
        iter_->Next();
    };

    /**
     * Move cursor to the previous element.
     */
    void prev()
    {
        assert(false && "not implemented");
    };

    /**
     * Get Key in Key-Value mapping pointed by this cursor.
     */
    std::vector<uint8_t> key() const
    {
        std::vector<uint8_t> v;
        if (!iter_->GetKey(v)) {
            return {};
        }
        return v;
    };

    /**
     * Get Value in Key-Value mapping pointed by this cursor.
     * @return
     */
    std::vector<uint8_t> value() const
    {
        std::vector<uint8_t> v;
        if (!iter_->GetValue(v)) {
            return {};
        }
        return v;
    };

private:
    std::shared_ptr<CDBIterator> iter_;
};

} // namespace VeriBlock

#endif //INTEGRATION_REFERENCE_BTC_CURSOR_HPP
