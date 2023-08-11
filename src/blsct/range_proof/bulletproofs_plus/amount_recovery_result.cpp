#include <blsct/arith/mcl/mcl.h>
#include <blsct/range_proof/bulletproofs_plus/amount_recovery_result.h>

namespace bulletproofs_plus {

template <typename T>
AmountRecoveryResult<T> AmountRecoveryResult<T>::failure()
{
    return {
        false,
        std::vector<RecoveredAmount<T>>()};
}
template AmountRecoveryResult<Mcl> AmountRecoveryResult<Mcl>::failure();

} // namespace bulletproofs_plus
