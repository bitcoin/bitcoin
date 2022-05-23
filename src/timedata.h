// Copyright (c) 2014-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TIMEDATA_H
#define BITCOIN_TIMEDATA_H

#include <algorithm>
#include <array>
#include <assert.h>
#include <cstring>
#include <stdint.h>
#include <vector>

static const int64_t DEFAULT_MAX_TIME_ADJUSTMENT = 70 * 60;

class CNetAddr;

/**
 * Median filter over a stream of values.
 * Returns the median of the last N numbers
 */
template <typename T, uint32_t N>
class CMedianFilter
{
private:
    std::vector<T> vSorted;
    std::array<T, N> vValues;
    size_t vIdx;

public:
    CMedianFilter(T initial_value) : vIdx(0)
    {
        static_assert(std::is_trivially_copyable<T>::value);
        static_assert(N > 1);
        static_assert(N <= 256,
                      "Implement algorithm that does not require maintaining a sorted array");

        vValues[vIdx++] = initial_value;
        vSorted.reserve(N);
        vSorted.emplace_back(initial_value);
    }

    void input(T value)
    {
        T tmp = vValues[vIdx];
        vValues[vIdx++] = value;
        if (vIdx >= N) vIdx = 0;

        if (vSorted.size() < N) {
            vSorted.insert(std::upper_bound(vSorted.begin(), vSorted.end(), value), value);
            return;
        }

        // Find current element to remove and where new element should be inserted
        // [0, 2, 2, 4, 5 ,7, 8, 9, 11, 14, 15, 16]
        //        ^              ^
        //        e              i
        // Shift entire block of memory either left or right and insert new value

        const auto erase_it = std::find(vSorted.begin(), vSorted.end(), tmp);
        auto insert_it = std::upper_bound(vSorted.begin(), vSorted.end(), value);
        assert(erase_it != vSorted.end());

        if (erase_it == insert_it) {
            *insert_it = value;
        } else if (erase_it < insert_it) {
            // Left rotate ensures |insert_it > vSorted.begin()|
            std::rotate(erase_it, erase_it + 1, insert_it);
            --insert_it;
            *insert_it = value;
        } else {
            // Right rotate
            std::rotate(insert_it, erase_it, erase_it + 1);
            *insert_it = value;
        }
    }

    T median() const
    {
        return vSorted.size() & 1 ?
                   vSorted[vSorted.size() / 2] :
                   (vSorted[vSorted.size() / 2 - 1] + vSorted[vSorted.size() / 2]) / 2;
    }

    size_t size() const
    {
        return vSorted.size();
    }

    const std::vector<T>& sorted()
    {
        return vSorted;
    }
};

/** Functions to keep track of adjusted P2P time */
int64_t GetTimeOffset();
int64_t GetAdjustedTime();
void AddTimeData(const CNetAddr& ip, int64_t nTime);

/**
 * Reset the internal state of GetTimeOffset(), GetAdjustedTime() and AddTimeData().
 */
void TestOnlyResetTimeData();

#endif // BITCOIN_TIMEDATA_H
