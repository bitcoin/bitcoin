#include <blsct/range_proof/bulletproofs/amount_recovery_result.h>
#include <blsct/arith/mcl/mcl.h>

namespace bulletproofs {

template <typename T>
AmountRecoveryResult<T> AmountRecoveryResult<T>::failure()
{
    return {
        false,
        std::vector<RecoveredAmount<T>>()};
}
template AmountRecoveryResult<Mcl> AmountRecoveryResult<Mcl>::failure();

} // namespace bulletproofs

