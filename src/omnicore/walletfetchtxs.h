#ifndef OMNICORE_WALLETFETCHTXS_H
#define OMNICORE_WALLETFETCHTXS_H

class CWallet;
class uint256;

namespace interfaces {
class Wallet;
} // namespace interfaces

#include <map>
#include <string>

namespace mastercore
{
/** Returns an ordered list of Omni transactions that are relevant to the wallet. */
std::map<std::string, uint256> FetchWalletOmniTransactions(interfaces::Wallet& iWallet, unsigned int count, int startBlock = 0, int endBlock = 999999);
}

#endif // OMNICORE_WALLETFETCHTXS_H
