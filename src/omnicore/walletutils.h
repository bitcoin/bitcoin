#ifndef BITCOIN_OMNICORE_WALLETUTILS_H
#define BITCOIN_OMNICORE_WALLETUTILS_H

class CCoinControl;
class CPubKey;
class CWallet;

namespace interfaces {
class Wallet;
} // namespace interfaces

#include <script/ismine.h>             // For isminefilter, isminetype
#include <script/standard.h>

#include <stdint.h>
#include <string>

namespace mastercore
{
/** Retrieves a public key from the wallet, or converts a hex-string to a public key. */
bool AddressToPubKey(const interfaces::Wallet* iWallet, const std::string& key, CPubKey& pubKey);

/** Checks, whether enough spendable outputs are available to pay for transaction fees. */
bool CheckFee(interfaces::Wallet& iWallet, const std::string& fromAddress, size_t nDataSize);

/** Checks, whether the output qualifies as input for a transaction. */
bool CheckInput(const CTxOut& txOut, int nHeight, CTxDestination& dest);

/** IsMine wrapper to determine whether the address is in the wallet. */
int IsMyAddress(const std::string& address, interfaces::Wallet* iWallet);

/** IsMine wrapper to determine whether the address is in the wallet. */
int IsMyAddressAllWallets(const std::string& address, const bool matchAny = false, const isminefilter& filter = ISMINE_SPENDABLE);

/** Estimate the minimum fee considering user set parameters and the required fee. */
CAmount GetEstimatedFeePerKb(interfaces::Wallet& iWallet);

/** Output values below this value are considered uneconomic, because it would
* require more fees to pay than the output is worth. */
int64_t GetEconomicThreshold(interfaces::Wallet& iWallet, const CTxOut& txOut);

#ifdef ENABLE_WALLET
/** Selects spendable outputs to create a transaction. */
int64_t SelectCoins(interfaces::Wallet& iWallet, const std::string& fromAddress, CCoinControl& coinControl, int64_t additional = 0);

/** Selects all spendable outputs to create a transaction. */
int64_t SelectAllCoins(interfaces::Wallet& iWallet, const std::string& fromAddress, CCoinControl& coinControl);
#endif
}

#endif // BITCOIN_OMNICORE_WALLETUTILS_H
