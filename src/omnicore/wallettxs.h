#ifndef OMNICORE_WALLETTXS_H
#define OMNICORE_WALLETTXS_H

class CCoinControl;

#include "script/standard.h"

#include <stdint.h>
#include <string>

namespace mastercore
{
/** Checks, whether the output qualifies as input for a transaction. */
bool CheckInput(const CTxOut& txOut, int nHeight, CTxDestination& dest);

/** Selects spendable outputs to create a transaction. */
int64_t SelectCoins(const std::string& fromAddress, CCoinControl& coinControl, int64_t additional = 0);
}

#endif // OMNICORE_WALLETTXS_H
