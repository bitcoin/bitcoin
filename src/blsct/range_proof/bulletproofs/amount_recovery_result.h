// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_AMOUNT_RECOVERY_RESULT_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_AMOUNT_RECOVERY_RESULT_H

#include <vector>

#include <blsct/range_proof/bulletproofs/recovered_amount.h>
#include <consensus/amount.h>

namespace bulletproofs {

template <typename T>
struct AmountRecoveryResult {
    bool is_completed; // does not mean recovery success
    std::vector<RecoveredAmount<T>> amounts;

    static AmountRecoveryResult<T> failure();
};

} // namespace bulletproofs

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_AMOUNT_RECOVERY_RESULT_H

