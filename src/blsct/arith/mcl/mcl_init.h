// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_MCL_MCL_INIT_H
#define NAVCOIN_BLSCT_ARITH_MCL_MCL_INIT_H

#define BLS_ETH 1
#include <bls/bls384_256.h>
#include <iostream>
#include <mutex>
#include <stdexcept>

/**
 * Create an instance of this class somewhere at the beginning
 * of the execution of an executable to initialize Mcl library.
 * volatile keyword is necessary to protect the instance from
 * compiler optimization. e.g.
 *
 * ```
 * void main() {
 *     volatile MclInit for_side_effect_only;
 * }
 * ```
 */
class MclInit
{
public:
    MclInit()
    {
        std::lock_guard<std::mutex> lock(m_init_mutex);

        static bool is_initialized = false;
        if (is_initialized) return;

        if (blsInit(MCL_BLS12_381, MCLBN_COMPILED_TIME_VAR) != 0) {
            throw std::runtime_error("blsInit failed");
        }
        mclBn_setETHserialization(1);

        is_initialized = true;
    }

private:
    inline static std::mutex m_init_mutex;
};

#endif // NAVCOIN_BLSCT_ARITH_MCL_MCL_INIT_H
