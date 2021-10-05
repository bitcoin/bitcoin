// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SPEND_H
#define BITCOIN_WALLET_SPEND_H

#include <consensus/amount.h>
#include <wallet/coinselection.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

/** Get the marginal bytes if spending the specified output from this transaction */
int GetTxSpendSize(const CWallet& wallet, const CWalletTx& wtx, unsigned int out, bool use_max_sig = false);

class COutput
{
public:
    const CWalletTx *tx;

    /** Index in tx->vout. */
    int i;

    /**
     * Depth in block chain.
     * If > 0: the tx is on chain and has this many confirmations.
     * If = 0: the tx is waiting confirmation.
     * If < 0: a conflicting tx is on chain and has this many confirmations. */
    int nDepth;

    /** Pre-computed estimated size of this output as a fully-signed input in a transaction. Can be -1 if it could not be calculated */
    int nInputBytes;

    /** Whether we have the private keys to spend this output */
    bool fSpendable;

    /** Whether we know how to spend this output, ignoring the lack of keys */
    bool fSolvable;

    /** Whether to use the maximum sized, 72 byte signature when calculating the size of the input spend. This should only be set when watch-only outputs are allowed */
    bool use_max_sig;

    /**
     * Whether this output is considered safe to spend. Unconfirmed transactions
     * from outside keys and unconfirmed replacement transactions are considered
     * unsafe and will not be used to fund new spending transactions.
     */
    bool fSafe;

    COutput(const CWallet& wallet, const CWalletTx& wtx, int iIn, int nDepthIn, bool fSpendableIn, bool fSolvableIn, bool fSafeIn, bool use_max_sig_in = false)
    {
        tx = &wtx; i = iIn; nDepth = nDepthIn; fSpendable = fSpendableIn; fSolvable = fSolvableIn; fSafe = fSafeIn; nInputBytes = -1; use_max_sig = use_max_sig_in;
        // If known and signable by the given wallet, compute nInputBytes
        // Failure will keep this value -1
        if (fSpendable) {
            nInputBytes = GetTxSpendSize(wallet, wtx, i, use_max_sig);
        }
    }

    std::string ToString() const;

    inline CInputCoin GetInputCoin() const
    {
        return CInputCoin(tx->tx, i, nInputBytes);
    }
};

//Get the marginal bytes of spending the specified output
int CalculateMaximumSignedInputSize(const CTxOut& txout, const CWallet* pwallet, bool use_max_sig = false);
int CalculateMaximumSignedInputSize(const CTxOut& txout, const SigningProvider* pwallet, bool use_max_sig = false);

struct TxSize {
    int64_t vsize{-1};
    int64_t weight{-1};
};

/** Calculate the size of the transaction assuming all signatures are max size
* Use DummySignatureCreator, which inserts 71 byte signatures everywhere.
* NOTE: this requires that all inputs must be in mapWallet (eg the tx should
* be AllInputsMine). */
TxSize CalculateMaximumSignedTxSize(const CTransaction& tx, const CWallet* wallet, const std::vector<CTxOut>& txouts, const CCoinControl* coin_control = nullptr);
TxSize CalculateMaximumSignedTxSize(const CTransaction& tx, const CWallet* wallet, const CCoinControl* coin_control = nullptr) EXCLUSIVE_LOCKS_REQUIRED(wallet->cs_wallet);

/**
 * populate vCoins with vector of available COutputs.
 */
void AvailableCoins(const CWallet& wallet, std::vector<COutput>& vCoins, const CCoinControl* coinControl = nullptr, const CAmount& nMinimumAmount = 1, const CAmount& nMaximumAmount = MAX_MONEY, const CAmount& nMinimumSumAmount = MAX_MONEY, const uint64_t nMaximumCount = 0) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

CAmount GetAvailableBalance(const CWallet& wallet, const CCoinControl* coinControl = nullptr);

/**
 * Find non-change parent output.
 */
const CTxOut& FindNonChangeParentOutput(const CWallet& wallet, const CTransaction& tx, int output) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

/**
 * Return list of available coins and locked coins grouped by non-change output address.
 */
std::map<CTxDestination, std::vector<COutput>> ListCoins(const CWallet& wallet) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

std::vector<OutputGroup> GroupOutputs(const CWallet& wallet, const std::vector<COutput>& outputs, const CoinSelectionParams& coin_sel_params, const CoinEligibilityFilter& filter, bool positive_only);

/**
 * Shuffle and select coins until nTargetValue is reached while avoiding
 * small change; This method is stochastic for some inputs and upon
 * completion the coin set and corresponding actual target value is
 * assembled
 * param@[in]   coins           Set of UTXOs to consider. These will be categorized into
 *                              OutputGroups and filtered using eligibility_filter before
 *                              selecting coins.
 * param@[out]  setCoinsRet     Populated with the coins selected if successful.
 * param@[out]  nValueRet       Used to return the total value of selected coins.
 */
bool AttemptSelection(const CWallet& wallet, const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, std::vector<COutput> coins,
                        std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CoinSelectionParams& coin_selection_params);

/**
 * Select a set of coins such that nValueRet >= nTargetValue and at least
 * all coins from coin_control are selected; never select unconfirmed coins if they are not ours
 * param@[out]  setCoinsRet         Populated with inputs including pre-selected inputs from
 *                                  coin_control and Coin Selection if successful.
 * param@[out]  nValueRet           Total value of selected coins including pre-selected ones
 *                                  from coin_control and Coin Selection if successful.
 */
bool SelectCoins(const CWallet& wallet, const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet,
                 const CCoinControl& coin_control, CoinSelectionParams& coin_selection_params) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

/**
 * Create a new transaction paying the recipients with a set of coins
 * selected by SelectCoins(); Also create the change output, when needed
 * @note passing nChangePosInOut as -1 will result in setting a random position
 */
bool CreateTransaction(CWallet& wallet, const std::vector<CRecipient>& vecSend, CTransactionRef& tx, CAmount& nFeeRet, int& nChangePosInOut, bilingual_str& error, const CCoinControl& coin_control, FeeCalculation& fee_calc_out, bool sign = true);

/**
 * Insert additional inputs into the transaction by
 * calling CreateTransaction();
 */
bool FundTransaction(CWallet& wallet, CMutableTransaction& tx, CAmount& nFeeRet, int& nChangePosInOut, bilingual_str& error, bool lockUnspents, const std::set<int>& setSubtractFeeFromOutputs, CCoinControl);

#endif // BITCOIN_WALLET_SPEND_H
