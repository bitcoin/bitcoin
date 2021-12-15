// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SPEND_H
#define BITCOIN_WALLET_SPEND_H

#include <consensus/amount.h>
#include <wallet/coinselection.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

struct TxSize {
    int64_t vsize{-1};
    int64_t weight{-1};
};

/** Get the marginal bytes if spending the specified output from this transaction */
TxSize GetTxSpendSize(const CWallet& wallet, const CWalletTx& wtx, unsigned int out, const CCoinControl* coin_control = nullptr);

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
    TxSize nInputSize;

    /** Whether we have the private keys to spend this output */
    bool fSpendable;

    /** Whether we know how to spend this output, ignoring the lack of keys */
    bool fSolvable;

    /**
     * Whether this output is considered safe to spend. Unconfirmed transactions
     * from outside keys and unconfirmed replacement transactions are considered
     * unsafe and will not be used to fund new spending transactions.
     */
    bool fSafe;

    bool isSegwit;

    COutput(const CWallet& wallet, const CWalletTx& wtx, int iIn, int nDepthIn, bool fSpendableIn, bool fSolvableIn, bool fSafeIn, const CCoinControl* coin_control = nullptr)
    {
        tx = &wtx; i = iIn; nDepth = nDepthIn; fSpendable = fSpendableIn; fSolvable = fSolvableIn; fSafe = fSafeIn; nInputSize = {-1, -1};
        const CTxOut &txout = wtx.tx->vout[i];
        const auto provider = wallet.GetSolvingProvider(txout.scriptPubKey);
        // If known and signable by the given wallet, compute nInputSize
        // Failure will keep this value -1
        if (fSpendable && provider) {
            nInputSize = GetTxSpendSize(wallet, wtx, i, coin_control);
            isSegwit = IsSegWitOutput(*provider, txout.scriptPubKey);
        }
    }

    std::string ToString() const;

    inline CInputCoin GetInputCoin() const
    {
        return CInputCoin(tx->tx, i, nInputSize.vsize, nInputSize.weight, isSegwit);
    }
};

//Get the marginal bytes of spending the specified output
TxSize CalculateMaximumSignedInputSize(const CTxOut& txout, const CWallet* pwallet, const CCoinControl* coin_control = nullptr);
TxSize CalculateMaximumSignedInputSize(const CTxOut& txout, const COutPoint outpoint, const SigningProvider* pwallet, const CCoinControl* coin_control = nullptr);

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
 * Attempt to find a valid input set that meets the provided eligibility filter and target.
 * Multiple coin selection algorithms will be run and the input set that produces the least waste
 * (according to the waste metric) will be chosen.
 *
 * param@[in]  wallet                 The wallet which provides solving data for the coins
 * param@[in]  nTargetValue           The target value
 * param@[in]  eligilibity_filter     A filter containing rules for which coins are allowed to be included in this selection
 * param@[in]  coins                  The vector of coins available for selection prior to filtering
 * param@[in]  coin_selection_params  Parameters for the coin selection
 * returns                            If successful, a SelectionResult containing the input set
 *                                    If failed, a nullopt
 */
std::optional<SelectionResult> AttemptSelection(const CWallet& wallet, const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, std::vector<COutput> coins,
                        const CoinSelectionParams& coin_selection_params);

/**
 * Select a set of coins such that nTargetValue is met and at least
 * all coins from coin_control are selected; never select unconfirmed coins if they are not ours
 * param@[in]   wallet                 The wallet which provides data necessary to spend the selected coins
 * param@[in]   vAvailableCoins        The vector of coins available to be spent
 * param@[in]   nTargetValue           The target value
 * param@[in]   coin_selection_params  Parameters for this coin selection such as feerates, whether to avoid partial spends,
 *                                     and whether to subtract the fee from the outputs.
 * returns                             If successful, a SelectionResult containing the selected coins
 *                                     If failed, a nullopt.
 */
std::optional<SelectionResult> SelectCoins(const CWallet& wallet, const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, const CCoinControl& coin_control,
                 const CoinSelectionParams& coin_selection_params) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

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
