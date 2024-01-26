// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLSCT_VERIFICATION_H
#define BLSCT_VERIFICATION_H

#include <blsct/arith/mcl/mcl.h>
#include <blsct/public_keys.h>
#include <blsct/range_proof/bulletproofs/range_proof_logic.h>
#include <blsct/range_proof/generators.h>
#include <coins.h>

namespace blsct {
bool VerifyTx(const CTransaction& tx, const CCoinsViewCache& view, const CAmount& blockReward = 0, const CAmount& minStake = 0);
}
#endif // BLSCT_VERIFICATION_H