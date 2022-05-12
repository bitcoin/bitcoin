// Copyright (c) 2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <saltedhasher.h>
#include <random.h>

#include <limits>

SaltedHasherBase::SaltedHasherBase() : k0(GetRand<uint64_t>()), k1(GetRand<uint64_t>()) {}

SaltedHasherBase StaticSaltedHasher::s;
