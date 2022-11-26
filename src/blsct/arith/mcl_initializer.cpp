// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl_initializer.h>

void MclInitializer::Init()
{
    boost::lock_guard<boost::mutex> lock(MclInitializer::m_init_mutex);
    static bool is_initialized = false;
    if (is_initialized) return;

    if (blsInit(MCL_BLS12_381, MCLBN_COMPILED_TIME_VAR) != 0) {
        throw std::runtime_error("blsInit failed");
    }
    mclBn_setETHserialization(1);

    is_initialized = true;
}