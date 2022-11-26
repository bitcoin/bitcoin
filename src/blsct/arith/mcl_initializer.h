// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Diverse arithmetic operations in the bls curve
// inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
// and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc

#ifndef NAVCOIN_BLSCT_ARITH_MCL_INITIALIZER_H
#define NAVCOIN_BLSCT_ARITH_MCL_INITIALIZER_H

#include <bls/bls384_256.h> // must include this before bls/bls.h
#include <bls/bls.h>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

class MclInitializer
{
public:
    static void Init();

private:
    inline static boost::mutex m_init_mutex;
};

#endif // NAVCOIN_BLSCT_ARITH_MCL_INITIALIZER_H