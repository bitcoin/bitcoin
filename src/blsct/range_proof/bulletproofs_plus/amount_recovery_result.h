// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_PLUS_AMOUNT_RECOVERY_RESULT_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_PLUS_AMOUNT_RECOVERY_RESULT_H

#include <vector>

#include <blsct/range_proof/bulletproofs_plus/amount_recovery_result.h>
#include <blsct/range_proof/recovered_data.h>

namespace bulletproofs_plus {

template <typename T>
struct AmountRecoveryResult
{
    bool is_completed; // does not mean recovery success
    std::vector<range_proof::RecoveredData<T>> amounts;

    static AmountRecoveryResult<T> failure();
};

} // namespace bulletproofs_plus

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_PLUS_AMOUNT_RECOVERY_RESULT_H