// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_RANGES_SET_H
#define BITCOIN_UTIL_RANGES_SET_H

#include <hash.h>
#include <saltedhasher.h>
#include <serialize.h>

#include <set>

/**
 * The CRangesSet is a datastructure that keeps efficiently numbers as set of
 * continuous ranges of numbers.
 * CRangesSet let's to keep elements with gaps of any size while CSkipList has
 * limited capacity (total size of all gaps)
 *
 * The CRangesSet provides transaction guarantees: element can be added or
 * removed and data structure will be consistent. For case if any of these
 * operation failed (out-memory for example), the `assert` will be called to
 * terminate program.
 */
class CRangesSet
{
    // internal datastructure, doesn't have a reason to be publicly available
    struct Range
    {
        uint64_t begin;
        uint64_t end;
        Range();
        Range(uint64_t begin, uint64_t end);
        bool operator<(const Range& other) const
        {
            if (begin != other.begin) return begin < other.begin;
            return end < other.end;
        }

        SERIALIZE_METHODS(Range, obj)
        {
            READWRITE(obj.begin);
            READWRITE(obj.end);
        }
    };

    std::set<Range> ranges;

public:
    /**
     * this function adds `value` to the datastructure.
     * it returns true if `add` succeed
     */
    [[nodiscard]] bool Add(uint64_t value);

    /**
     * this function returns true if `value` exists in the datastructure
     */
    [[nodiscard]] bool Contains(uint64_t value) const noexcept;

    /**
     * this function removes `value` from the datastructure.
     * it returns `false` if element didn't existed or removing failed by any reason
     */
    [[nodiscard]] bool Remove(uint64_t value);

    /**
     * Size() works with complexity O(N) times, avoid calling it without a good reason
     * Instead prefer to use IsEmpty()
     */
    [[nodiscard]] size_t Size() const noexcept;

    /**
     * IsEmpty() returns true if there's no any elements added
     */
    [[nodiscard]] bool IsEmpty() const noexcept;

    SERIALIZE_METHODS(CRangesSet, obj)
    {
        READWRITE(obj.ranges);
    }
};

#endif // BITCOIN_UTIL_RANGES_SET_H
