// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_MCL_ATOMIC_MCL_INIT_H
#define NAVCOIN_BLSCT_ARITH_MCL_ATOMIC_MCL_INIT_H

#define BLS_ETH 1
#include <bls/bls384_256.h>
#include <blsct/arith/mcl/init/mcl_init.h>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <iostream>
#include <stdexcept>

/**
 * Define this class at the beginning of execution of each executable
 * to initialize Mcl library for non-static context. volatile keyword
 * is necessary to protect the line from comipler optimization. e.g.
 * ```
 * void main() {
 *     static volatile AtomicMclInit for_side_effect_only;
 * }
 * ```
 */
class AtomicMclInit : MclInit
{
public:
    AtomicMclInit()
    {
        static bool is_initialized = false;
        if (is_initialized) return;
        boost::lock_guard<boost::mutex> lock(m_init_mutex);

        Initialize();

        is_initialized = true;
    }

private:
    inline static boost::mutex m_init_mutex;
};

#endif // NAVCOIN_BLSCT_ARITH_MCL_ATOMIC_MCL_INIT_H
