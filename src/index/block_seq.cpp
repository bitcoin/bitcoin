// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/block_seq.h>

#include <crypto/siphash.h>
#include <dbwrapper.h>
#include <random.h>

#include <cstdint>
#include <string>
#include <utility>

namespace block_seq {
SipHasher13UJ ReadOrCreateHasher(CDBWrapper& db, const std::string& salt_key)
{
    std::pair<uint64_t, uint64_t> salt;
    if (!db.Read(salt_key, salt)) {
        FastRandomContext rng{};
        salt = {rng.rand64(), rng.rand64()};
        db.Write(salt_key, salt, /*fSync=*/true);
    }
    return SipHasher13UJ{salt.first, salt.second};
}
} // namespace block_seq
