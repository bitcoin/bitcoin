// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SPEND_H
#define BITCOIN_WALLET_SPEND_H

#include <consensus/amount.h>
#include <policy/fees.h> // for FeeCalculation
#include <util/result.h>
#include <wallet/coinselection.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

#include <optional>

namespace wallet {
/** Get the marginal bytes if spending the specified output from this transaction.
 * Use CoinControl to determine whether to expect signature grinding when calculating the size of the input spend. */
int CalculateMaximumSignedInputSize(const CTxOut& txout, const CWallet* pwallet, const CCoinControl* coin_control = nullptr);
int CalculateMaximumSignedInputSize(const CTxOut& txout, const COutPoint outpoint, const SigningProvider* pwallet, const CCoinControl* coin_control = nullptr);
struct TxSize {
    int64_t vsize{-1};
    int64_t weight{-1};
};

/** Calculate the size of the transaction using CoinControl to determine
 * whether to expect signature grinding when calculating the size of the input spend. */
TxSize CalculateMaximumSignedTxSize(const CTransaction& tx, const CWallet* wallet, const std::vector<CTxOut>& txouts, const CCoinControl* coin_control = nullptr);
TxSize CalculateMaximumSignedTxSize(const CTransaction& tx, const CWallet* wallet, const CCoinControl* coin_control = nullptr) EXCLUSIVE_LOCKS_REQUIRED(wallet->cs_wallet);

/**
 * COutputs available for spending, stored by OutputType.
 * This struct is really just a wrapper around OutputType vectors with a convenient
 * method for concatenating and returning all COutputs as one vector.
 *
 * clear(), size() methods are implemented so that one can interact with
 * the CoinsResult struct as if it was a vector
 */
struct CoinsResult {
    /** Vectors for each OutputType */
    std::vector<COutput> legacy;
    std::vector<COutput> P2SH_segwit;
    std::vector<COutput> bech32;
    std::vector<COutput> bech32m;

    /** Other is a catch-all for anything that doesn't match the known OutputTypes */
    std::vector<COutput> other;

    /** Concatenate and return all COutputs as one vector */
    std::vector<COutput> all() const;

    /** The following methods are provided so that CoinsResult can mimic a vector,
     * i.e., methods can work with individual OutputType vectors or on the entire object */
    uint64_t size() const;
    void clear();

    /** Sum of all available coins */
    CAmount total_amount{0};
};

/**
 * Populate the CoinsResult struct with vectors of available COutputs, organized by OutputType.
 */
CoinsResult AvailableCoins(const CWallet& wallet,
                           const CCoinControl* coinControl = nullptr,
                           std::optional<CFeeRate> feerate = std::nullopt,
                           const CAmount& nMinimumAmount = 1,
                           const CAmount& nMaximumAmount = MAX_MONEY,
                           const CAmount& nMinimumSumAmount = MAX_MONEY,
                           const uint64_t nMaximumCount = 0,
                           bool only_spendable = true) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

/**
 * Wrapper function for AvailableCoins which skips the `feerate` parameter. Use this function
 * to list all available coins (e.g. listunspent RPC) while not intending to fund a transaction.
 */
CoinsResult AvailableCoinsListUnspent(const CWallet& wallet, const CCoinControl* coinControl = nullptr, const CAmount& nMinimumAmount = 1, const CAmount& nMaximumAmount = MAX_MONEY, const CAmount& nMinimumSumAmount = MAX_MONEY, const uint64_t nMaximumCount = 0) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

CAmount GetAvailableBalance(const CWallet& wallet, const CCoinControl* coinControl = nullptr);

/**
 * Find non-change parent output.
 */
const CTxOut& FindNonChangeParentOutput(const CWallet& wallet, const CTransaction& tx, int output) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
const CTxOut& FindNonChangeParentOutput(const CWallet& wallet, const COutPoint& outpoint) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

/**
 * Return list of available coins and locked coins grouped by non-change output address.
 */
std::map<CTxDestination, std::vector<COutput>> ListCoins(const CWallet& wallet) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

std::vector<OutputGroup> GroupOutputs(const CWallet& wallet, const std::vector<COutput>& outputs, const CoinSelectionParams& coin_sel_params, const CoinEligibilityFilter& filter, bool positive_only);
/**
 * Attempt to find a valid input set that preserves privacy by not mixing OutputTypes.
 * `ChooseSelectionResult()` will be called on each OutputType individually and the best
 * the solution (according to the waste metric) will be chosen. If a valid input cannot be found from any
 * single OutputType, fallback to running `ChooseSelectionResult()` over all available coins.
 *
 * param@[in]  wallet                    The wallet which provides solving data for the coins
 * param@[in]  nTargetValue              The target value
 * param@[in]  eligilibity_filter        A filter containing rules for which coins are allowed to be included in this selection
 * param@[in]  available_coins           The struct of coins, organized by OutputType, available for selection prior to filtering
 * param@[in]  coin_selection_params     Parameters for the coin selection
 * param@[in]  allow_mixed_output_types  Relax restriction that SelectionResults must be of the same OutputType
 * returns                               If successful, a SelectionResult containing the input set
 *                                       If failed, a nullopt
 */
std::optional<SelectionResult> AttemptSelection(const CWallet& wallet, const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, const CoinsResult& available_coins,
                        const CoinSelectionParams& coin_selection_params, bool allow_mixed_output_types);

/**
 * Attempt to find a valid input set that meets the provided eligibility filter and target.
 * Multiple coin selection algorithms will be run and the input set that produces the least waste
 * (according to the waste metric) will be chosen.
 *
 * param@[in]  wallet                    The wallet which provides solving data for the coins
 * param@[in]  nTargetValue              The target value
 * param@[in]  eligilibity_filter        A filter containing rules for which coins are allowed to be included in this selection
 * param@[in]  available_coins           The struct of coins, organized by OutputType, available for selection prior to filtering
 * param@[in]  coin_selection_params     Parameters for the coin selection
 * returns                               If successful, a SelectionResult containing the input set
 *                                       If failed, a nullopt
 */
std::optional<SelectionResult> ChooseSelectionResult(const CWallet& wallet, const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, const std::vector<COutput>& available_coins,
                        const CoinSelectionParams& coin_selection_params);

/**
 * Select a set of coins such that nTargetValue is met and at least
 * all coins from coin_control are selected; never select unconfirmed coins if they are not ours
 * param@[in]   wallet                 The wallet which provides data necessary to spend the selected coins
 * param@[in]   available_coins        The struct of coins, organized by OutputType, available for selection prior to filtering
 * param@[in]   nTargetValue           The target value
 * param@[in]   coin_selection_params  Parameters for this coin selection such as feerates, whether to avoid partial spends,
 *                                     and whether to subtract the fee from the outputs.
 * returns                             If successful, a SelectionResult containing the selected coins
 *                                     If failed, a nullopt.
 */
std::optional<SelectionResult> SelectCoins(const CWallet& wallet, CoinsResult& available_coins, const CAmount& nTargetValue, const CCoinControl& coin_control,
                 const CoinSelectionParams& coin_selection_params) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

struct CreatedTransactionResult
{
    CTransactionRef tx;
    CAmount fee;
    FeeCalculation fee_calc;
    int change_pos;

    CreatedTransactionResult(CTransactionRef _tx, CAmount _fee, int _change_pos, const FeeCalculation& _fee_calc)
        : tx(_tx), fee(_fee), fee_calc(_fee_calc), change_pos(_change_pos) {}
};

/**
 * Create a new transaction paying the recipients with a set of coins
 * selected by SelectCoins(); Also create the change output, when needed
 * @note passing change_pos as -1 will result in setting a random position
 */
util::Result<CreatedTransactionResult> CreateTransaction(CWallet& wallet, const std::vector<CRecipient>& vecSend, int change_pos, const CCoinControl& coin_control, bool sign = true);

/**
 * Insert additional inputs into the transaction by
 * calling CreateTransaction();
 */
bool FundTransaction(CWallet& wallet, CMutableTransaction& tx, CAmount& nFeeRet, int& nChangePosInOut, bilingual_str& error, bool lockUnspents, const std::set<int>& setSubtractFeeFromOutputs, CCoinControl);
} // namespace wallet

#endif // BITCOIN_WALLET_SPEND_H
