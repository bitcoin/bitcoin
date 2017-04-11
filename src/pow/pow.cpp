// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow/pow.h"
#include "pow/cuckoo_cycle/cuckoo_cycle.h"
#include "pow/sha256/sha256.h"

namespace powa {

bool fZeroStartingNonce = false;

pow_ref pow::create(const uint32_t powid, challenge_ref c_in, callback_ref cb_in) {
    switch (powid) {
    case POWID_CUCKOO_CYCLE: return pow_ref(new cuckoo_cycle::cuckoo_cycle(cuckoo_cycle::cc_challenge_ref(new cuckoo_cycle::cc_challenge(*c_in)), cb_in));
    case POWID_SHA256:       return pow_ref(new sha256(challenge_ref(new sha256challenge(*c_in)), cb_in));
    default:
        fprintf(stderr, "error: unknown powid:%u\n", powid);
        return nullptr;
    }
}

}  // namespace powa
