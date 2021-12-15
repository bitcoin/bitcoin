// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RECEIVE_H
#define BITCOIN_WALLET_RECEIVE_H

#include <consensus/amount.h>
#include <wallet/ismine.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

isminetype InputIsMine(const CWallet& wallet, const CTxIn& txin) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

/** Returns whether all of the inputs match the filter */
bool AllInputsMine(const CWallet& wallet, const CTransaction& tx, const isminefilter& filter);

CAmount OutputGetCredit(const CWallet& wallet, const CTxOut& txout, const isminefilter& filter);
CAmount TxGetCredit(const CWallet& wallet, const CTransaction& tx, const isminefilter& filter);

bool ScriptIsChange(const CWallet& wallet, const CScript& script) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
bool OutputIsChange(const CWallet& wallet, const CTxOut& txout) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
CAmount OutputGetChange(const CWallet& wallet, const CTxOut& txout) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
CAmount TxGetChange(const CWallet& wallet, const CTransaction& tx);

CAmount CachedTxGetCredit(const CWallet& wallet, const CWalletTx& wtx, const isminefilter& filter);
//! filter decides which addresses will count towards the debit
CAmount CachedTxGetDebit(const CWallet& wallet, const CWalletTx& wtx, const isminefilter& filter);
CAmount CachedTxGetChange(const CWallet& wallet, const CWalletTx& wtx);
CAmount CachedTxGetImmatureCredit(const CWallet& wallet, const CWalletTx& wtx, bool fUseCache = true);
CAmount CachedTxGetImmatureWatchOnlyCredit(const CWallet& wallet, const CWalletTx& wtx, const bool fUseCache = true);
// TODO: Remove "NO_THREAD_SAFETY_ANALYSIS" and replace it with the correct
// annotation "EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)". The
// annotation "NO_THREAD_SAFETY_ANALYSIS" was temporarily added to avoid
// having to resolve the issue of member access into incomplete type CWallet.
CAmount CachedTxGetAvailableCredit(const CWallet& wallet, const CWalletTx& wtx, bool fUseCache = true, const isminefilter& filter = ISMINE_SPENDABLE) NO_THREAD_SAFETY_ANALYSIS;
struct COutputEntry
{
    CTxDestination destination;
    CAmount amount;
    int vout;
};
void CachedTxGetAmounts(const CWallet& wallet, const CWalletTx& wtx,
                        std::list<COutputEntry>& listReceived,
                        std::list<COutputEntry>& listSent,
                        CAmount& nFee, const isminefilter& filter);
bool CachedTxIsFromMe(const CWallet& wallet, const CWalletTx& wtx, const isminefilter& filter);
bool CachedTxIsTrusted(const CWallet& wallet, const CWalletTx& wtx, std::set<uint256>& trusted_parents) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
bool CachedTxIsTrusted(const CWallet& wallet, const CWalletTx& wtx);

struct Balance {
    CAmount m_mine_trusted{0};           //!< Trusted, at depth=GetBalance.min_depth or more
    CAmount m_mine_untrusted_pending{0}; //!< Untrusted, but in mempool (pending)
    CAmount m_mine_immature{0};          //!< Immature coinbases in the main chain
    CAmount m_watchonly_trusted{0};
    CAmount m_watchonly_untrusted_pending{0};
    CAmount m_watchonly_immature{0};
};
Balance GetBalance(const CWallet& wallet, int min_depth = 0, bool avoid_reuse = true);

std::map<CTxDestination, CAmount> GetAddressBalances(const CWallet& wallet);
std::set<std::set<CTxDestination>> GetAddressGroupings(const CWallet& wallet) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

#endif // BITCOIN_WALLET_RECEIVE_H
