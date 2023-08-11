// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_RECOVERED_AMOUNT_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_RECOVERED_AMOUNT_H

#include <consensus/amount.h>
#include <string>

namespace bulletproofs {

template <typename T>
struct RecoveredAmount {
    using Scalar = typename T::Scalar;

    RecoveredAmount(
        const size_t& id,
        const CAmount& amount,
        const Scalar& gamma,
        const std::string& message
    ): id{id}, amount{amount}, gamma{gamma}, message{message} {}

    size_t id;
    CAmount amount;
    Scalar gamma;
    std::string message;
};

} // namespace bulletproofs

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_RECOVERED_AMOUNT_H
