// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_AVAILABLECOINS_H
#define BITCOIN_WALLET_AVAILABLECOINS_H

#include <consensus/amount.h>
#include <policy/fees.h> // for FeeCalculation
#include <wallet/coinselection.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

#include <optional>

namespace wallet {
enum class CoinStatus {
    TRUSTED,
    UNTRUSTED_PENDING,
    IMMATURE
};

enum class CoinOwnership {
    MINE,
    WATCH_ONLY
};

/**
 * COutputs available for spending, stored by OutputType.
 * This struct is really just a wrapper around OutputType vectors with a convenient
 * method for concatenating and returning all COutputs as one vector.
 *
 * Size(), Clear(), Erase(), Shuffle(), and Add() methods are implemented to
 * allow easy interaction with the struct.
 */
struct CoinsResult {
    std::map<OutputType, std::vector<COutput>> coins;

    std::map<std::pair<CoinOwnership,CoinStatus>, CAmount> balances;

    /** Concatenate and return all COutputs as one vector */
    std::vector<COutput> All() const;

    /** The following methods are provided so that CoinsResult can mimic a vector,
     * i.e., methods can work with individual OutputType vectors or on the entire object */
    size_t Size() const;
    /** Return how many different output types this struct stores */
    size_t TypesCount() const { return coins.size(); }
    void Clear();
    void Erase(const std::unordered_set<COutPoint, SaltedOutpointHasher>& coins_to_remove);
    void Shuffle(FastRandomContext& rng_fast);
    void Add(OutputType type, const COutput& out);
    void Add(CoinOwnership ownership, CoinStatus status, OutputType type, const COutput& out);

    CAmount GetTotalAmount() { return total_amount; }
    std::optional<CAmount> GetEffectiveTotalAmount() {return total_effective_amount; }

private:
    /** Sum of all available coins raw value */
    CAmount total_amount{0};
    /** Sum of all available coins effective value (each output value minus fees required to spend it) */
    std::optional<CAmount> total_effective_amount{0};
};

struct CoinFilterParams {
    // Outputs below the minimum amount will not get selected
    CAmount min_amount{1};
    // Outputs above the maximum amount will not get selected
    CAmount max_amount{MAX_MONEY};
    // Return outputs until the minimum sum amount is covered
    CAmount min_sum_amount{MAX_MONEY};
    // Maximum number of outputs that can be returned
    uint64_t max_count{0};
    // By default, return only spendable outputs
    bool only_spendable{true};
    // By default, do not include immature coinbase outputs
    bool include_immature_coinbase{false};
    // By default, skip locked UTXOs
    bool skip_locked{true};
    // By default, do not include coins from transactions that are not in our mempool
    // Even with this option enabled, conflicted or inactive transactions will not be included
    bool include_tx_not_in_mempool{false};
};

/**
 * Populate the CoinsResult struct with vectors of available COutputs, organized by OutputType.
 */
CoinsResult AvailableCoins(const CWallet& wallet,
                           const CCoinControl* coinControl = nullptr,
                           std::optional<CFeeRate> feerate = std::nullopt,
                           const CoinFilterParams& params = {}) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

bool CachedTxIsFromMe(const CWallet& wallet, const CWalletTx& wtx, const isminefilter& filter);
bool CachedTxIsTrusted(const CWallet& wallet, const CWalletTx& wtx, std::set<uint256>& trusted_parents) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
bool CachedTxIsTrusted(const CWallet& wallet, const CWalletTx& wtx);
//! filter decides which addresses will count towards the debit
CAmount CachedTxGetDebit(const CWallet& wallet, const CWalletTx& wtx, const isminefilter& filter);
CAmount GetCachableAmount(const CWallet& wallet, const CWalletTx& wtx, CWalletTx::AmountType type, const isminefilter& filter);
CAmount OutputGetCredit(const CWallet& wallet, const CTxOut& txout, const isminefilter& filter);
CAmount TxGetCredit(const CWallet& wallet, const CTransaction& tx, const isminefilter& filter);
/** Get the marginal bytes if spending the specified output from this transaction.
 * Use CoinControl to determine whether to expect signature grinding when calculating the size of the input spend. */
int CalculateMaximumSignedInputSize(const CTxOut& txout, const CWallet* pwallet, const CCoinControl* coin_control = nullptr);
int CalculateMaximumSignedInputSize(const CTxOut& txout, const COutPoint outpoint, const SigningProvider* pwallet, const CCoinControl* coin_control = nullptr);
} // namespace wallet

#endif // BITCOIN_WALLET_AVAILABLECOINS_H