// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/skip_set.h>

#include <logging.h>
#include <stdexcept>

bool CSkipSet::Add(uint64_t value)
{
    if (Contains(value)) {
        throw std::runtime_error(strprintf("%s: trying to add an element that can't be added", __func__));
    }

    if (auto it = skipped.find(value); it != skipped.end()) {
        skipped.erase(it);
        return true;
    }

    assert(current_max <= value);
    if (Capacity() + value - current_max > capacity_limit) {
        LogPrintf("CSkipSet::Add failed due to capacity exceeded: requested %lld to %lld while limit is %lld\n",
                value - current_max, Capacity(), capacity_limit);
        return false;
    }
    for (uint64_t index = current_max; index < value; ++index) {
        bool insert_ret = skipped.insert(index).second;
        assert(insert_ret);
    }
    current_max = value + 1;
    return true;
}

bool CSkipSet::CanBeAdded(uint64_t value) const
{
    if (Contains(value)) return false;

    if (skipped.find(value) != skipped.end()) return true;

    if (Capacity() + value - current_max > capacity_limit) {
        return false;
    }

    return true;
}

bool CSkipSet::Contains(uint64_t value) const
{
    if (current_max <= value) return false;
    return skipped.find(value) == skipped.end();
}

