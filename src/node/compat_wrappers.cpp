#include <validation.h>

namespace node {

bool TestBlockValidity(Chainstate& chainstate, const CBlock& block, bool check_pow, bool check_merkle_root)
{
    const BlockValidationState state = ::TestBlockValidity(chainstate, block, check_pow, check_merkle_root);
    return state.IsValid();
}

} // namespace node
