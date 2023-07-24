// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_SKIP_SET_H
#define BITCOIN_UTIL_SKIP_SET_H

#include <serialize.h>
#include <unordered_set>

// This data structure keeps efficiently all indexes and have a strict limit for used memory
// So far as CCreditPool is built only in direction from parent block to child
// there's no need to remove elements from CSkipSet ever, only add them
class CSkipSet {
private:
    std::unordered_set<uint64_t> skipped;
    uint64_t current_max{0};
    size_t capacity_limit;
public:
    explicit CSkipSet(size_t capacity_limit = 10'000) :
        capacity_limit(capacity_limit)
    {}

    /**
     * `Add` returns true if element has been added correctly and false if
     * capacity is depleted.
     *
     * `Add` should not be called if the element has been already added.
     * Use `Contains` to check if the element is here
     * Adding existing value will cause an exception
     */
    [[nodiscard]] bool Add(uint64_t value);

    bool CanBeAdded(uint64_t value) const;

    bool Contains(uint64_t value) const;

    size_t Size() const {
        return current_max - skipped.size();
    }
    size_t Capacity() const {
        return skipped.size();
    }

    SERIALIZE_METHODS(CSkipSet, obj)
    {
        READWRITE(obj.current_max);
        READWRITE(obj.skipped);
    }
};

#endif // BITCOIN_UTIL_SKIP_SET_H
