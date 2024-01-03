// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_COMMON_H
#define NAVCOIN_BLSCT_COMMON_H

#include <vector>
#include <cstdint>
#include <streams.h>

namespace blsct {

using Message = std::vector<uint8_t>;

class Common
{
public:
    inline static const std::vector<uint8_t> BLSCTBALANCE = {
        '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
        '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
        '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
        '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
        'B', 'L', 'S', 'C', 'T', 'B', 'A', 'L', 'A', 'N', 'C', 'E',
        '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
        '0', '0', '0', '0', '0', '0', '0', '0'};

    static std::vector<uint8_t> DataStreamToVector(const DataStream& st);

    /**
     * Returns power of 2 that is greater or equal to input_value_len
     * throws exception if such a number exceeds the maximum
     */
    static size_t GetFirstPowerOf2GreaterOrEqTo(const size_t& input_value_vec_len);

    template <typename T>
    static std::vector<T> TrimPreceedingZeros(const std::vector<T>& vec);

    template <typename T>
    static void AddZeroIfEmpty(std::vector<T>& vec);
};
}

#endif  // NAVCOIN_BLSCT_COMMON_H
