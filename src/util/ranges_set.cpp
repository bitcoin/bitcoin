// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/ranges_set.h>

CRangesSet::Range::Range() : CRangesSet::Range::Range(0, 0) {}

CRangesSet::Range::Range(uint64_t begin, uint64_t end) :
    begin(begin),
    end(end)
{
}

bool CRangesSet::Add(uint64_t value)
{
    if (Contains(value)) return false;

    // If element is not in CRangesSet, we need to add a new range [value, value + 1)
    // But this operation can cause 2 operation of merge (total 3 cases):
    // - if there're exist ranges [x, value) and [value + 1, y) than
    //   all 3 of them should be merged in one range [x, y)
    // - if there's exist a range [x, value) - we need to replace it to new range [x, value + 1)
    // - if there's exist a range [value + 1, y) - we need to replace it to new range [value, y)
    Range new_range{value, value + 1};
    auto it = ranges.lower_bound({value, value});
    if (it != ranges.begin()) {
        auto prev = it;
        --prev;

        if (prev->end == value) {
            new_range.begin = prev->begin;
            it = ranges.erase(prev);
        }
    }
    const auto next = it;
    if (next != ranges.end()) {
        if (next->begin == value + 1) {
            new_range.end = next->end;
            it = ranges.erase(next);
        }
    }
    const auto ret = ranges.insert(new_range);
    assert(ret.second);
    return true;
}

bool CRangesSet::Remove(uint64_t value)
{
    if (!Contains(value)) return false;

    // If element is in CRangesSet, there's possible 2 cases:
    // - value in the range that is lower-bound (if begin of that range equals to `value`)
    // - value in the previous range (otherwise)
    auto it = ranges.lower_bound({value, value});

    if (it == ranges.end() || it->begin != value) {
        assert(it != ranges.begin());
        --it;
    }

    const Range current_range = *it;
    ranges.erase(it);
    if (current_range.begin != value) {
        const auto ret = ranges.insert({current_range.begin, value});
        assert(ret.second);
    }
    if (value + 1 != current_range.end) {
        const auto ret = ranges.insert({value + 1, current_range.end});
        assert(ret.second);
    }
    return true;
}

bool CRangesSet::IsEmpty() const noexcept
{
    return ranges.empty();
}

size_t CRangesSet::Size() const noexcept
{
    size_t result{0};
    for (auto i : ranges) {
        result += i.end - i.begin;
    }
    return result;
}

bool CRangesSet::Contains(uint64_t value) const noexcept
{
    const auto it = ranges.lower_bound({value, value});
    if (it != ranges.end() && it->begin == value) return true;
    if (it == ranges.begin()) return false;
    auto prev = it;
    --prev;
    return prev->begin <= value && prev->end > value;
}

