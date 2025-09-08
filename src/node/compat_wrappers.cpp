#include <validation.h>

bool TestBlockValidity(Chainstate& chainstate, const CBlock& block, bool check_pow, bool check_merkle_root)
{
    BlockValidationState state;
    return chainstate.TestBlockValidity(state, block, check_pow, check_merkle_root);
}

