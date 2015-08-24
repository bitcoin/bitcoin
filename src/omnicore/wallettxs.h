#ifndef OMNICORE_WALLETTXS_H
#define OMNICORE_WALLETTXS_H

class CCoinControl;
class CPubKey;

#include "script/standard.h"

#include <stdint.h>
#include <string>

namespace mastercore
{
/** Retrieves a public key from the wallet, or converts a hex-string to a public key. */
bool AddressToPubKey(const std::string& key, CPubKey& pubKey);

/** Checks, whether enough spendable outputs are available to pay for transaction fees. */
bool CheckFee(const std::string& fromAddress, size_t nDataSize);

/** Checks, whether the output qualifies as input for a transaction. */
bool CheckInput(const CTxOut& txOut, int nHeight, CTxDestination& dest);

/** Retrieves the label, used by the UI, for an address from the wallet. */
std::string GetAddressLabel(const std::string& address);

/** IsMine wrapper to determine whether the address is in the wallet. */
int IsMyAddress(const std::string& address);

/** Selects spendable outputs to create a transaction. */
int64_t SelectCoins(const std::string& fromAddress, CCoinControl& coinControl, int64_t additional = 0);
}

#endif // OMNICORE_WALLETTXS_H
