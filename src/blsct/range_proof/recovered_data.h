// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RECOVERED_DATA_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RECOVERED_DATA_H

#include <consensus/amount.h>
#include <serialize.h>

#include <cstddef>
#include <string>

namespace range_proof {

template <typename T>
struct RecoveredData
{
    using Scalar = typename T::Scalar;

    RecoveredData(
        const size_t& id,
        const CAmount& amount,
        const Scalar& gamma,
        const std::string& message
    ): id{id}, amount{amount}, gamma{gamma}, message{message} {}

    RecoveredData() {}

    size_t id;
    CAmount amount;
    Scalar gamma;
    std::string message;

    SERIALIZE_METHODS(RecoveredData<T>, obj)
    {
        READWRITE(obj.amount, obj.gamma, obj.message);
    }
};

} // namespace range_proof

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RECOVERED_DATA_H