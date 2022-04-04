// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/common.h>
#include <coinjoin/options.h>
#include <consensus/validation.h>
#include <evo/dmn_types.h>
#include <interfaces/chain.h>
#include <policy/policy.h>
#include <script/signingprovider.h>
#include <util/check.h>
#include <util/fees.h>
#include <util/moneystr.h>
#include <util/trace.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/receive.h>
#include <wallet/spend.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

#include <cmath>

using interfaces::FoundBlock;

namespace wallet {
static constexpr size_t OUTPUT_GROUP_MAX_ENTRIES{100};

int CalculateMaximumSignedInputSize(const CTxOut& txout, const COutPoint outpoint, const SigningProvider* provider, const CCoinControl* coin_control)
{
    CMutableTransaction txn;
    txn.vin.push_back(CTxIn(outpoint));
    if (!provider || !DummySignInput(*provider, txn.vin[0], txout, coin_control)) {
        return -1;
    }
    return ::GetSerializeSize(txn.vin[0], PROTOCOL_VERSION);
}

int CalculateMaximumSignedInputSize(const CTxOut& txout, const CWallet* wallet, const CCoinControl* coin_control)
{
    const std::unique_ptr<SigningProvider> provider = wallet->GetSolvingProvider(txout.scriptPubKey);
    return CalculateMaximumSignedInputSize(txout, COutPoint(), provider.get(), coin_control);
}

// txouts needs to be in the order of tx.vin
int64_t CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, const std::vector<CTxOut>& txouts, const CCoinControl* coin_control)
{
    CMutableTransaction txNew(tx);
    if (!wallet->DummySignTx(txNew, txouts, coin_control)) {
        return -1;
    }
    return ::GetSerializeSize(txNew, PROTOCOL_VERSION);
}

int64_t CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, const CCoinControl* coin_control)
{
    std::vector<CTxOut> txouts;
    // Look up the inputs. The inputs are either in the wallet, or in coin_control.
    for (const CTxIn& input : tx.vin) {
        const auto mi = wallet->mapWallet.find(input.prevout.hash);
        // Can not estimate size without knowing the input details
        if (mi != wallet->mapWallet.end()) {
            assert(input.prevout.n < mi->second.tx->vout.size());
            txouts.emplace_back(mi->second.tx->vout.at(input.prevout.n));
        } else if (coin_control) {
            CTxOut txout;
            if (!coin_control->GetExternalOutput(input.prevout, txout)) {
                return -1;
            }
            txouts.emplace_back(txout);
        } else {
            return -1;
        }
    }
    return CalculateMaximumSignedTxSize(tx, wallet, txouts, coin_control);
}

uint64_t CoinsResult::size() const
{
    return legacy.size() + other.size();
}

std::vector<COutput> CoinsResult::all() const
{
    std::vector<COutput> all;
    all.reserve(this->size());
    all.insert(all.end(), legacy.begin(), legacy.end());
    all.insert(all.end(), other.begin(), other.end());
    return all;
}

void CoinsResult::clear()
{
    legacy.clear();
    other.clear();
}

CoinsResult AvailableCoins(const CWallet& wallet,
                           const CCoinControl* coinControl,
                           std::optional<CFeeRate> feerate,
                           const CAmount& nMinimumAmount,
                           const CAmount& nMaximumAmount,
                           const CAmount& nMinimumSumAmount,
                           const uint64_t nMaximumCount,
                           bool only_spendable)
{
    AssertLockHeld(wallet.cs_wallet);

    CoinType nCoinType = coinControl ? coinControl->nCoinType : CoinType::ALL_COINS;

    CoinsResult result;
    // Either the WALLET_FLAG_AVOID_REUSE flag is not set (in which case we always allow), or we default to avoiding, and only in the case where
    // a coin control object is provided, and has the avoid address reuse flag set to false, do we allow already used addresses
    bool allow_used_addresses = !wallet.IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE) || (coinControl && !coinControl->m_avoid_address_reuse);
    const int min_depth = {coinControl ? coinControl->m_min_depth : DEFAULT_MIN_DEPTH};
    const int max_depth = {coinControl ? coinControl->m_max_depth : DEFAULT_MAX_DEPTH};
    const bool only_safe = {coinControl ? !coinControl->m_include_unsafe_inputs : true};

    std::set<uint256> trusted_parents;
    for (const auto* pwtx : wallet.GetSpendableTXs()) {
        const uint256& wtxid = pwtx->GetHash();
        const CWalletTx& wtx = *pwtx;

        if (wallet.IsTxImmatureCoinBase(wtx))
            continue;

        int nDepth = wallet.GetTxDepthInMainChain(wtx);

        // We should not consider coins which aren't at least in our mempool
        // It's possible for these to be conflicted via ancestors which we may never be able to detect
        if (nDepth == 0 && !wtx.InMempool())
            continue;

        bool safeTx = CachedTxIsTrusted(wallet, wtx, trusted_parents);

        if (only_safe && !safeTx) {
            continue;
        }

        if (nDepth < min_depth || nDepth > max_depth) {
            continue;
        }

        bool tx_from_me = CachedTxIsFromMe(wallet, wtx, ISMINE_ALL);

        for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
            const CTxOut& output = wtx.tx->vout[i];
            const COutPoint outpoint(wtxid, i);

            bool found = false;
            switch (nCoinType) {
                case CoinType::ONLY_FULLY_MIXED: {
                    found = CoinJoin::IsDenominatedAmount(output.nValue) &&
                            wallet.IsFullyMixed(outpoint);
                    break;
                }
                case CoinType::ONLY_READY_TO_MIX: {
                    found = CoinJoin::IsDenominatedAmount(output.nValue) &&
                            !wallet.IsFullyMixed(outpoint);
                    break;
                }
                case CoinType::ONLY_NONDENOMINATED: {
                    // NOTE: do not use collateral amounts
                    found = !CoinJoin::IsCollateralAmount(output.nValue) &&
                            !CoinJoin::IsDenominatedAmount(output.nValue);
                    break;
                }
                case CoinType::ONLY_MASTERNODE_COLLATERAL: {
                    found = dmn_types::IsCollateralAmount(output.nValue);
                    break;
                }
                case CoinType::ONLY_COINJOIN_COLLATERAL: {
                    found = CoinJoin::IsCollateralAmount(output.nValue);
                    break;
                }
                case CoinType::ALL_COINS: {
                    found = true;
                    break;
                }
            } // no default case, so the compiler can warn about missing cases
            if (!found) continue;

            if (output.nValue < nMinimumAmount || output.nValue > nMaximumAmount)
                continue;

            if (coinControl && coinControl->HasSelected() && !coinControl->m_allow_other_inputs && !coinControl->IsSelected(outpoint))
                continue;

            if (wallet.IsLockedCoin(outpoint) && nCoinType != CoinType::ONLY_MASTERNODE_COLLATERAL)
                continue;

            if (wallet.IsSpent(outpoint))
                continue;

            isminetype mine = wallet.IsMine(output);

            if (mine == ISMINE_NO) {
                continue;
            }

            if (!allow_used_addresses && wallet.IsSpentKey(output.scriptPubKey)) {
                continue;
            }

            std::unique_ptr<SigningProvider> provider = wallet.GetSolvingProvider(output.scriptPubKey);

            int input_bytes = CalculateMaximumSignedInputSize(output, COutPoint(), provider.get(), coinControl);
            // Because CalculateMaximumSignedInputSize just uses ProduceSignature and makes a dummy signature,
            // it is safe to assume that this input is solvable if input_bytes is greater -1.
            bool solvable = input_bytes > -1;
            bool spendable = ((mine & ISMINE_SPENDABLE) != ISMINE_NO) || (((mine & ISMINE_WATCH_ONLY) != ISMINE_NO) && (coinControl && coinControl->fAllowWatchOnly && solvable));

            // Filter by spendable outputs only
            if (!spendable && only_spendable) continue;

            // When parsing a scriptPubKey, Solver returns the parsed pubkeys or hashes (depending on the script)
            // We don't need those here, so we are leaving them in return_values_unused
            std::vector<std::vector<uint8_t>> return_values_unused;
            TxoutType type;

            // If the Output is P2SH and spendable, we want to know if it is
            // a P2SH (legacy). We can determine this from the redeemScript.
            // If the Output is not spendable, it will be classified as a P2SH (legacy),
            // since we have no way of knowing otherwise without the redeemScript
            if (output.scriptPubKey.IsPayToScriptHash() && solvable) {
                CScript redeemScript;
                CTxDestination destination;
                if (!ExtractDestination(output.scriptPubKey, destination))
                    continue;
                const CScriptID& hash = CScriptID(std::get<ScriptHash>(destination));
                if (!provider->GetCScript(hash, redeemScript))
                    continue;
                type = Solver(redeemScript, return_values_unused);
            } else {
                type = Solver(output.scriptPubKey, return_values_unused);
            }

            COutput coin(outpoint, output, nDepth, input_bytes, spendable, solvable, safeTx, wtx.GetTxTime(), tx_from_me, feerate);
            switch (type) {
            case TxoutType::SCRIPTHASH:
            case TxoutType::PUBKEYHASH:
                result.legacy.push_back(coin);
                break;
            default:
                result.other.push_back(coin);
            };

            // Cache total amount as we go
            result.total_amount += output.nValue;
            // Checks the sum amount of all UTXO's.
            if (nMinimumSumAmount != MAX_MONEY) {
                if (result.total_amount >= nMinimumSumAmount) {
                    return result;
                }
            }

            // Checks the maximum number of UTXO's.
            if (nMaximumCount > 0 && result.size() >= nMaximumCount) {
                return result;
            }
        }
    }

    return result;
}

CoinsResult AvailableCoinsListUnspent(const CWallet& wallet, const CCoinControl* coinControl, const CAmount& nMinimumAmount, const CAmount& nMaximumAmount, const CAmount& nMinimumSumAmount, const uint64_t nMaximumCount)
{
    return AvailableCoins(wallet, coinControl, /*feerate=*/ std::nullopt, nMinimumAmount, nMaximumAmount, nMinimumSumAmount, nMaximumCount, /*only_spendable=*/false);
}

CAmount GetAvailableBalance(const CWallet& wallet, const CCoinControl* coinControl)
{
    LOCK(wallet.cs_wallet);
    return AvailableCoins(wallet, coinControl,
            /*feerate=*/ std::nullopt,
            /*nMinimumAmount=*/ 1,
            /*nMaximumAmount=*/ MAX_MONEY,
            /*nMinimumSumAmount=*/ MAX_MONEY,
            /*nMaximumCount=*/ 0
    ).total_amount;
}

const CTxOut& FindNonChangeParentOutput(const CWallet& wallet, const CTransaction& tx, int output)
{
    AssertLockHeld(wallet.cs_wallet);
    const CTransaction* ptx = &tx;
    int n = output;
    while (OutputIsChange(wallet, ptx->vout[n]) && ptx->vin.size() > 0) {
        const COutPoint& prevout = ptx->vin[0].prevout;
        auto it = wallet.mapWallet.find(prevout.hash);
        if (it == wallet.mapWallet.end() || it->second.tx->vout.size() <= prevout.n ||
            !wallet.IsMine(it->second.tx->vout[prevout.n])) {
            break;
        }
        ptx = it->second.tx.get();
        n = prevout.n;
    }
    return ptx->vout[n];
}

const CTxOut& FindNonChangeParentOutput(const CWallet& wallet, const COutPoint& outpoint)
{
    AssertLockHeld(wallet.cs_wallet);
    return FindNonChangeParentOutput(wallet, *wallet.GetWalletTx(outpoint.hash)->tx, outpoint.n);
}

std::map<CTxDestination, std::vector<COutput>> ListCoins(const CWallet& wallet)
{
    AssertLockHeld(wallet.cs_wallet);

    std::map<CTxDestination, std::vector<COutput>> result;

    for (const COutput& coin : AvailableCoinsListUnspent(wallet).all()) {
        CTxDestination address;
        if ((coin.spendable || (wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && coin.solvable)) &&
            ExtractDestination(FindNonChangeParentOutput(wallet, coin.outpoint).scriptPubKey, address)) {
            result[address].emplace_back(std::move(coin));
        }
    }

    // Include watch-only for LegacyScriptPubKeyMan wallets without private keys
    const bool include_watch_only = wallet.GetLegacyScriptPubKeyMan() && wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    const isminetype is_mine_filter = include_watch_only ? ISMINE_WATCH_ONLY : ISMINE_SPENDABLE;
    for (const COutPoint& output : wallet.setLockedCoins) {
        auto it = wallet.mapWallet.find(output.hash);
        if (it != wallet.mapWallet.end()) {
            const auto& wtx = it->second;
            int depth = wallet.GetTxDepthInMainChain(wtx);
            if (depth >= 0 && output.n < wtx.tx->vout.size() &&
                wallet.IsMine(wtx.tx->vout[output.n]) == is_mine_filter
            ) {
                CTxDestination address;
                if (ExtractDestination(FindNonChangeParentOutput(wallet, *wtx.tx, output.n).scriptPubKey, address)) {
                    const auto out = wtx.tx->vout.at(output.n);
                    result[address].emplace_back(
                            COutPoint(wtx.GetHash(), output.n), out, depth, CalculateMaximumSignedInputSize(out, &wallet, /*coin_control=*/nullptr), /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ false, wtx.GetTxTime(), CachedTxIsFromMe(wallet, wtx, ISMINE_ALL));
                }
            }
        }
    }

    return result;
}

static bool isGroupISLocked(const OutputGroup& group, interfaces::Chain& chain)
{
    return std::all_of(group.m_outputs.begin(), group.m_outputs.end(), [&chain](const auto& output) {
        return chain.isInstantSendLockedTx(output.outpoint.hash);
    });
}

std::vector<OutputGroup> GroupOutputs(const CWallet& wallet, const std::vector<COutput>& outputs, const CoinSelectionParams& coin_sel_params, const CoinEligibilityFilter& filter, bool positive_only)
{
    std::vector<OutputGroup> groups_out;

    if (!coin_sel_params.m_avoid_partial_spends) {
        // Allowing partial spends  means no grouping. Each COutput gets its own OutputGroup.
        for (const COutput& output : outputs) {
            // Skip outputs we cannot spend
            if (!output.spendable) continue;

            size_t ancestors, descendants;
            wallet.chain().getTransactionAncestry(output.outpoint.hash, ancestors, descendants);

            // Make an OutputGroup containing just this output
            OutputGroup group{coin_sel_params};
            group.Insert(output, ancestors, descendants, positive_only);

            // Check the OutputGroup's eligibility. Only add the eligible ones.
            if (positive_only && group.GetSelectionAmount() <= 0) continue;
            bool isISLocked = isGroupISLocked(group, wallet.chain());
            if (group.m_outputs.size() > 0 && group.EligibleForSpending(filter, isISLocked)) groups_out.push_back(group);
        }
        return groups_out;
    }

    // We want to combine COutputs that have the same scriptPubKey into single OutputGroups
    // except when there are more than OUTPUT_GROUP_MAX_ENTRIES COutputs grouped in an OutputGroup.
    // To do this, we maintain a map where the key is the scriptPubKey and the value is a vector of OutputGroups.
    // For each COutput, we check if the scriptPubKey is in the map, and if it is, the COutput is added
    // to the last OutputGroup in the vector for the scriptPubKey. When the last OutputGroup has
    // OUTPUT_GROUP_MAX_ENTRIES COutputs, a new OutputGroup is added to the end of the vector.
    std::map<CScript, std::vector<OutputGroup>> spk_to_groups_map;
    for (const auto& output : outputs) {
        // Skip outputs we cannot spend
        if (!output.spendable) continue;

        size_t ancestors, descendants;
        wallet.chain().getTransactionAncestry(output.outpoint.hash, ancestors, descendants);
        CScript spk = output.txout.scriptPubKey;

        std::vector<OutputGroup>& groups = spk_to_groups_map[spk];

        if (groups.size() == 0) {
            // No OutputGroups for this scriptPubKey yet, add one
            groups.emplace_back(coin_sel_params);
        }

        // Get the last OutputGroup in the vector so that we can add the COutput to it
        // A pointer is used here so that group can be reassigned later if it is full.
        OutputGroup* group = &groups.back();

        // Check if this OutputGroup is full. We limit to OUTPUT_GROUP_MAX_ENTRIES when using -avoidpartialspends
        // to avoid surprising users with very high fees.
        if (group->m_outputs.size() >= OUTPUT_GROUP_MAX_ENTRIES) {
            // The last output group is full, add a new group to the vector and use that group for the insertion
            groups.emplace_back(coin_sel_params);
            group = &groups.back();
        }

        // Add the output to group
        group->Insert(output, ancestors, descendants, positive_only);
    }

    // Now we go through the entire map and pull out the OutputGroups
    for (const auto& spk_and_groups_pair: spk_to_groups_map) {
        const std::vector<OutputGroup>& groups_per_spk= spk_and_groups_pair.second;

        // Go through the vector backwards. This allows for the first item we deal with being the partial group.
        for (auto group_it = groups_per_spk.rbegin(); group_it != groups_per_spk.rend(); group_it++) {
            const OutputGroup& group = *group_it;

            // Don't include partial groups if there are full groups too and we don't want partial groups
            if (group_it == groups_per_spk.rbegin() && groups_per_spk.size() > 1 && !filter.m_include_partial_groups) {
                continue;
            }

            // Check the OutputGroup's eligibility. Only add the eligible ones.
            if (positive_only && group.GetSelectionAmount() <= 0) continue;
            bool isISLocked = isGroupISLocked(group, wallet.chain());
            if (group.m_outputs.size() > 0 && group.EligibleForSpending(filter, isISLocked)) groups_out.push_back(group);
        }
    }

    return groups_out;
}

std::optional<SelectionResult> AttemptSelection(const CWallet& wallet, const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, const CoinsResult& available_coins,
                                                const CoinSelectionParams& coin_selection_params, bool allow_mixed_output_types, CoinType nCoinType)
{
    // Run coin selection on each OutputType and compute the Waste Metric
    std::vector<SelectionResult> results;
    if (auto result{ChooseSelectionResult(wallet, nTargetValue, eligibility_filter, available_coins.legacy, coin_selection_params, nCoinType)}) {
        results.push_back(*result);
    }

    // If we can't fund the transaction from any individual OutputType, run coin selection
    // over all available coins, else pick the best solution from the results
    if (results.size() == 0) {
        if (allow_mixed_output_types) {
            if (auto result{ChooseSelectionResult(wallet, nTargetValue, eligibility_filter, available_coins.all(), coin_selection_params, nCoinType)}) {
                return result;
            }
        }
        return std::optional<SelectionResult>();
    };
    std::optional<SelectionResult> result{*std::min_element(results.begin(), results.end())};
    return result;
};

std::optional<SelectionResult> ChooseSelectionResult(const CWallet& wallet, const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, const std::vector<COutput>& available_coins,
                                                     const CoinSelectionParams& coin_selection_params, CoinType nCoinType)
{
    // Vector of results. We will choose the best one based on waste.
    std::vector<SelectionResult> results;

    // Note that unlike KnapsackSolver, we do not include the fee for creating a change output as BnB will not create a change output.
    std::vector<OutputGroup> positive_groups = GroupOutputs(wallet, available_coins, coin_selection_params, eligibility_filter, true /* positive_only */);
    positive_groups.clear(); // Cleared to skip BnB and SRD as they're unaware of mixed coins
    if (auto bnb_result{SelectCoinsBnB(positive_groups, nTargetValue, coin_selection_params.m_cost_of_change)}) {
        results.push_back(*bnb_result);
    }

    // The knapsack solver has some legacy behavior where it will spend dust outputs. We retain this behavior, so don't filter for positive only here.
    std::vector<OutputGroup> all_groups = GroupOutputs(wallet, available_coins, coin_selection_params, eligibility_filter, false /* positive_only */);
    CAmount target_with_change = nTargetValue;
    // While nTargetValue includes the transaction fees for non-input things, it does not include the fee for creating a change output.
    // So we need to include that for KnapsackSolver as well, as we are expecting to create a change output.
    // There is also no change output when spending fully mixed coins.
    if (!coin_selection_params.m_subtract_fee_outputs && nCoinType != CoinType::ONLY_FULLY_MIXED) {
        target_with_change += coin_selection_params.m_change_fee;
    }
    if (auto knapsack_result{KnapsackSolver(all_groups, target_with_change, coin_selection_params.m_min_change_target,
                                            coin_selection_params.rng_fast, nCoinType == CoinType::ONLY_FULLY_MIXED,
                                            wallet.m_default_max_tx_fee)}) {
        knapsack_result->ComputeAndSetWaste(coin_selection_params.m_cost_of_change);
        results.push_back(*knapsack_result);
    }

    // Include change for SRD as we want to avoid making really small change if the selection just
    // barely meets the target. Just use the lower bound change target instead of the randomly
    // generated one, since SRD will result in a random change amount anyway; avoid making the
    // target needlessly large.
    const CAmount srd_target = target_with_change + CHANGE_LOWER;
    if (auto srd_result{SelectCoinsSRD(positive_groups, srd_target, coin_selection_params.rng_fast)}) {
        srd_result->ComputeAndSetWaste(coin_selection_params.m_cost_of_change);
        results.push_back(*srd_result);
    }

    if (results.size() == 0) {
        // No solution found
        return std::nullopt;
    }

    // Choose the result with the least waste
    // If the waste is the same, choose the one which spends more inputs.
    auto& best_result = *std::min_element(results.begin(), results.end());
    return best_result;
}

std::optional<SelectionResult> SelectCoins(const CWallet& wallet, CoinsResult& available_coins, const CAmount& nTargetValue, const CCoinControl& coin_control, const CoinSelectionParams& coin_selection_params)
{
    // Note: this function should never be used for "always free" tx types like dstx
    CoinType nCoinType = coin_control.nCoinType;
    CAmount value_to_select = nTargetValue;

    OutputGroup preset_inputs(coin_selection_params);

    // calculate value from preset inputs and store them
    std::set<COutPoint> preset_coins;

    std::vector<COutPoint> vPresetInputs;
    coin_control.ListSelected(vPresetInputs);
    for (const COutPoint& outpoint : vPresetInputs) {
        int input_bytes = -1;
        CTxOut txout;
        auto ptr_wtx = wallet.GetWalletTx(outpoint.hash);
        if (ptr_wtx) {
            // Clearly invalid input, fail
            if (ptr_wtx->tx->vout.size() <= outpoint.n) {
                return std::nullopt;
            }
            txout = ptr_wtx->tx->vout.at(outpoint.n);
            input_bytes = CalculateMaximumSignedInputSize(txout, &wallet, &coin_control);
        } else {
            // The input is external. We did not find the tx in mapWallet.
            if (!coin_control.GetExternalOutput(outpoint, txout)) {
                return std::nullopt;
            }
            input_bytes = CalculateMaximumSignedInputSize(txout, outpoint, &coin_control.m_external_provider, &coin_control);
        }
        if (nCoinType == CoinType::ONLY_FULLY_MIXED) {
            // Make sure to include mixed preset inputs only,
            // even if some non-mixed inputs were manually selected via CoinControl
            if (!wallet.IsFullyMixed(outpoint)) continue;
        }
        if (input_bytes == -1) {
            return std::nullopt; // Not solvable, can't estimate size for fee
        }

        /* Set some defaults for depth, spendable, solvable, safe, time, and from_me as these don't matter for preset inputs since no selection is being done. */
        COutput output(outpoint, txout, /*depth=*/ 0, input_bytes, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, coin_selection_params.m_effective_feerate);
        if (coin_selection_params.m_subtract_fee_outputs) {
            value_to_select -= output.txout.nValue;
        } else {
            value_to_select -= output.GetEffectiveValue();
        }
        preset_coins.insert(outpoint);
        /* Set ancestors and descendants to 0 as they don't matter for preset inputs since no actual selection is being done.
         * positive_only is set to false because we want to include all preset inputs, even if they are dust.
         */
        preset_inputs.Insert(output, /*ancestors=*/ 0, /*descendants=*/ 0, /*positive_only=*/ false);
    }

    // coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
    if (coin_control.HasSelected() && !coin_control.m_allow_other_inputs) {
        SelectionResult result(nTargetValue, SelectionAlgorithm::MANUAL);
        bool all_inputs{coin_control.fRequireAllInputs};
        if (!all_inputs) {
            // Calculate the smallest set of inputs required to meet nTargetValue from available_coins
            bool success{false};
            OutputGroup preset_candidates(coin_selection_params);
            for (const COutput& out : available_coins.all()) {
                if (!out.spendable) continue;
                if (preset_coins.count(out.outpoint)) {
                    preset_candidates.Insert(out, /*ancestors=*/0, /*descendants=*/0, /*positive_only=*/false);
                }
                if (preset_candidates.GetSelectionAmount() >= nTargetValue) {
                    result.AddInput(preset_candidates);
                    success = true;
                    break;
                }
            }
            // Couldn't meet target, add all inputs
            if (!success) all_inputs = true;
        }
        if (all_inputs) {
            result.AddInput(preset_inputs);
        }
        if (result.GetSelectedValue() < nTargetValue) return std::nullopt;
        result.ComputeAndSetWaste(coin_selection_params.m_cost_of_change);
        return result;
    }

    // remove preset inputs from coins so that Coin Selection doesn't pick them.
    if (coin_control.HasSelected()) {
        available_coins.legacy.erase(remove_if(available_coins.legacy.begin(), available_coins.legacy.end(), [&](const COutput& c) { return preset_coins.count(c.outpoint); }), available_coins.legacy.end());
        available_coins.other.erase(remove_if(available_coins.other.begin(), available_coins.other.end(), [&](const COutput& c) { return preset_coins.count(c.outpoint); }), available_coins.other.end());
    }

    unsigned int limit_ancestor_count = 0;
    unsigned int limit_descendant_count = 0;
    wallet.chain().getPackageLimits(limit_ancestor_count, limit_descendant_count);
    const size_t max_ancestors = (size_t)std::max<int64_t>(1, limit_ancestor_count);
    const size_t max_descendants = (size_t)std::max<int64_t>(1, limit_descendant_count);
    const bool fRejectLongChains = gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

    // form groups from remaining coins; note that preset coins will not
    // automatically have their associated (same address) coins included
    if (coin_control.m_avoid_partial_spends && available_coins.size() > OUTPUT_GROUP_MAX_ENTRIES) {
        // Cases where we have 101+ outputs all pointing to the same destination may result in
        // privacy leaks as they will potentially be deterministically sorted. We solve that by
        // explicitly shuffling the outputs before processing
        Shuffle(available_coins.legacy.begin(), available_coins.legacy.end(), coin_selection_params.rng_fast);
        Shuffle(available_coins.other.begin(), available_coins.other.end(), coin_selection_params.rng_fast);
    }
    // Coin Selection attempts to select inputs from a pool of eligible UTXOs to fund the
    // transaction at a target feerate. If an attempt fails, more attempts may be made using a more
    // permissive CoinEligibilityFilter.
    std::optional<SelectionResult> res = [&] {
        // Pre-selected inputs already cover the target amount.
        if (value_to_select <= 0) return std::make_optional(SelectionResult(nTargetValue, SelectionAlgorithm::MANUAL));

        // If possible, fund the transaction with confirmed UTXOs only. Prefer at least six
        // confirmations on outputs received from other wallets and only spend confirmed change.
        if (auto r1{AttemptSelection(wallet, value_to_select, CoinEligibilityFilter(1, 6, 0), available_coins, coin_selection_params, /*allow_mixed_output_types=*/false, nCoinType)}) return r1;
        // Allow mixing only if no solution from any single output type can be found
        if (auto r2{AttemptSelection(wallet, value_to_select, CoinEligibilityFilter(1, 1, 0), available_coins, coin_selection_params, /*allow_mixed_output_types=*/true, nCoinType)}) return r2;

        // Fall back to using zero confirmation change (but with as few ancestors in the mempool as
        // possible) if we cannot fund the transaction otherwise.
        if (wallet.m_spend_zero_conf_change) {
            if (auto r3{AttemptSelection(wallet, value_to_select, CoinEligibilityFilter(0, 1, 2), available_coins, coin_selection_params, /*allow_mixed_output_types=*/true, nCoinType)}) return r3;
            if (auto r4{AttemptSelection(wallet, value_to_select, CoinEligibilityFilter(0, 1, std::min(size_t{4}, max_ancestors/3), std::min(size_t{4}, max_descendants/3)),
                                         available_coins, coin_selection_params, /*allow_mixed_output_types=*/true, nCoinType)}) {
                return r4;
            }
            if (auto r5{AttemptSelection(wallet, value_to_select, CoinEligibilityFilter(0, 1, max_ancestors/2, max_descendants/2),
                                         available_coins, coin_selection_params, /*allow_mixed_output_types=*/true, nCoinType)}) {
                return r5;
            }
            // If partial groups are allowed, relax the requirement of spending OutputGroups (groups
            // of UTXOs sent to the same address, which are obviously controlled by a single wallet)
            // in their entirety.
            if (auto r6{AttemptSelection(wallet, value_to_select, CoinEligibilityFilter(0, 1, max_ancestors-1, max_descendants-1, true /* include_partial_groups */),
                                         available_coins, coin_selection_params, /*allow_mixed_output_types=*/true, nCoinType)}) {
                return r6;
            }
            // Try with unsafe inputs if they are allowed. This may spend unconfirmed outputs
            // received from other wallets.
            if (coin_control.m_include_unsafe_inputs) {
                if (auto r7{AttemptSelection(wallet, value_to_select,
                    CoinEligibilityFilter(0 /* conf_mine */, 0 /* conf_theirs */, max_ancestors-1, max_descendants-1, true /* include_partial_groups */),
                    available_coins, coin_selection_params, /*allow_mixed_output_types=*/true, nCoinType)}) {
                    return r7;
                }
            }
            // Try with unlimited ancestors/descendants. The transaction will still need to meet
            // mempool ancestor/descendant policy to be accepted to mempool and broadcasted, but
            // OutputGroups use heuristics that may overestimate ancestor/descendant counts.
            if (!fRejectLongChains) {
                if (auto r8{AttemptSelection(wallet, value_to_select,
                                      CoinEligibilityFilter(0, 1, std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(), true /* include_partial_groups */),
                                      available_coins, coin_selection_params, /*allow_mixed_output_types=*/true, nCoinType)}) {
                    return r8;
                }
            }
        }
        // Coin Selection failed.
        return std::optional<SelectionResult>();
    }();

    if (!res) return std::nullopt;

    // Add preset inputs to result
    res->AddInput(preset_inputs);
    if (res->m_algo == SelectionAlgorithm::MANUAL) {
        res->ComputeAndSetWaste(coin_selection_params.m_cost_of_change);
    }

    return res;
}

static bool IsCurrentForAntiFeeSniping(interfaces::Chain& chain, const uint256& block_hash)
{
    if (chain.isInitialBlockDownload()) {
        return false;
    }
    constexpr int64_t MAX_ANTI_FEE_SNIPING_TIP_AGE = 8 * 60 * 60; // in seconds
    int64_t block_time;
    CHECK_NONFATAL(chain.findBlock(block_hash, FoundBlock().time(block_time)));
    if (block_time < (GetTime() - MAX_ANTI_FEE_SNIPING_TIP_AGE)) {
        return false;
    }
    return true;
}

/**
 * Set a height-based locktime for new transactions (uses the height of the
 * current chain tip unless we are not synced with the current chain
 */
static void DiscourageFeeSniping(CMutableTransaction& tx, FastRandomContext& rng_fast,
                                 interfaces::Chain& chain, const uint256& block_hash, int block_height)
{
    // All inputs must be added by now
    assert(!tx.vin.empty());
    // Discourage fee sniping.
    //
    // For a large miner the value of the transactions in the best block and
    // the mempool can exceed the cost of deliberately attempting to mine two
    // blocks to orphan the current best block. By setting nLockTime such that
    // only the next block can include the transaction, we discourage this
    // practice as the height restricted and limited blocksize gives miners
    // considering fee sniping fewer options for pulling off this attack.
    //
    // A simple way to think about this is from the wallet's point of view we
    // always want the blockchain to move forward. By setting nLockTime this
    // way we're basically making the statement that we only want this
    // transaction to appear in the next block; we don't want to potentially
    // encourage reorgs by allowing transactions to appear at lower heights
    // than the next block in forks of the best chain.
    //
    // Of course, the subsidy is high enough, and transaction volume low
    // enough, that fee sniping isn't a problem yet, but by implementing a fix
    // now we ensure code won't be written that makes assumptions about
    // nLockTime that preclude a fix later.
    if (IsCurrentForAntiFeeSniping(chain, block_hash)) {
        tx.nLockTime = block_height;

        // Secondly occasionally randomly pick a nLockTime even further back, so
        // that transactions that are delayed after signing for whatever reason,
        // e.g. high-latency mix networks and some CoinJoin implementations, have
        // better privacy.
        if (rng_fast.randrange(10) == 0) {
            tx.nLockTime = std::max(0, int(tx.nLockTime) - int(rng_fast.randrange(100)));
        }
    } else {
        // If our chain is lagging behind, we can't discourage fee sniping nor help
        // the privacy of high-latency transactions. To avoid leaking a potentially
        // unique "nLockTime fingerprint", set nLockTime to a constant.
        tx.nLockTime = 0;
    }
    // Sanity check all values
    assert(tx.nLockTime < LOCKTIME_THRESHOLD); // Type must be block height
    assert(tx.nLockTime <= uint64_t(block_height));
    for (const auto& in : tx.vin) {
        // Can not be FINAL for locktime to work
        assert(in.nSequence != CTxIn::SEQUENCE_FINAL);
        // May be MAX NONFINAL to disable BIP68
        if (in.nSequence == CTxIn::MAX_SEQUENCE_NONFINAL) continue;
        // The wallet does not support any other sequence-use right now.
        assert(false);
    }
}

static util::Result<CreatedTransactionResult> CreateTransactionInternal(
        CWallet& wallet,
        const std::vector<CRecipient>& vecSend,
        int change_pos,
        const CCoinControl& coin_control,
        bool sign,
        int nExtraPayloadSize) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    AssertLockHeld(wallet.cs_wallet);

    // out variables, to be packed into returned result structure
    CAmount nFeeRet;
    int nChangePosInOut = change_pos;

    FastRandomContext rng_fast;
    CMutableTransaction txNew; // The resulting transaction that we make

    CoinSelectionParams coin_selection_params{rng_fast}; // Parameters for coin selection, init with dummy
    coin_selection_params.m_avoid_partial_spends = coin_control.m_avoid_partial_spends;

    // Set the long term feerate estimate to the wallet's consolidate feerate
    coin_selection_params.m_long_term_feerate = wallet.m_consolidate_feerate;

    CAmount recipients_sum = 0;
    ReserveDestination reservedest(&wallet);
    const bool sort_bip69{nChangePosInOut == -1};
    unsigned int outputs_to_subtract_fee_from = 0; // The number of outputs which we are subtracting the fee from
    for (const auto& recipient : vecSend) {
        recipients_sum += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount) {
            outputs_to_subtract_fee_from++;
            coin_selection_params.m_subtract_fee_outputs = true;
        }
    }
    coin_selection_params.m_min_change_target = GenerateChangeTarget(std::floor(recipients_sum / vecSend.size()), rng_fast);

    // Create change script that will be used if we need change
    CScript scriptChange;
    bilingual_str error; // possible error str

    // coin control: send change to custom address
    if (!std::get_if<CNoDestination>(&coin_control.destChange)) {
        scriptChange = GetScriptForDestination(coin_control.destChange);
    } else { // no coin control: send change to newly generated address
        // Note: We use a new key here to keep it from being obvious which side is the change.
        //  The drawback is that by not reusing a previous key, the change may be lost if a
        //  backup is restored, if the backup doesn't have the new private key for the change.
        //  If we reused the old key, it would be possible to add code to look for and
        //  rediscover unknown transactions that were written with keys of ours to recover
        //  post-backup change.

        // Reserve a new key pair from key pool. If it fails, provide a dummy
        // destination in case we don't need change.
        CTxDestination dest;
        auto op_dest = reservedest.GetReservedDestination(true);
        if (!op_dest) {
            error = _("Transaction needs a change address, but we can't generate it.") + Untranslated(" ") + util::ErrorString(op_dest);
        } else {
            dest = *op_dest;
            scriptChange = GetScriptForDestination(dest);
        }
        // A valid destination implies a change script (and
        // vice-versa). An empty change script will abort later, if the
        // change keypool ran out, but change is required.
        CHECK_NONFATAL(IsValidDestination(dest) != scriptChange.empty());
    }
    CTxOut change_prototype_txout(0, scriptChange);
    coin_selection_params.change_output_size = GetSerializeSize(change_prototype_txout);

    // Get size of spending the change output
    int change_spend_size = CalculateMaximumSignedInputSize(change_prototype_txout, &wallet);
    // If the wallet doesn't know how to sign change output, assume p2sh-p2pkh
    // as lower-bound to allow BnB to do it's thing
    if (change_spend_size == -1) {
        coin_selection_params.change_spend_size = DUMMY_NESTED_P2PKH_INPUT_SIZE;
    } else {
        coin_selection_params.change_spend_size = (size_t)change_spend_size;
    }

    // Set discard feerate
    coin_selection_params.m_discard_feerate = coin_control.m_discard_feerate ? *coin_control.m_discard_feerate : GetDiscardRate(wallet);

    // Get the fee rate to use effective values in coin selection
    FeeCalculation feeCalc;
    coin_selection_params.m_effective_feerate = GetMinimumFeeRate(wallet, coin_control, &feeCalc);
    // Do not, ever, assume that it's fine to change the fee rate if the user has explicitly
    // provided one
    if (coin_control.m_feerate && coin_selection_params.m_effective_feerate > *coin_control.m_feerate) {
        return util::Error{strprintf(_("Fee rate (%s) is lower than the minimum fee rate setting (%s)"), coin_control.m_feerate->ToString(FeeEstimateMode::DUFF_B), coin_selection_params.m_effective_feerate.ToString(FeeEstimateMode::DUFF_B))};
    }
    if (feeCalc.reason == FeeReason::FALLBACK && !wallet.m_allow_fallback_fee) {
        // eventually allow a fallback fee
        return util::Error{strprintf(_("Fee estimation failed. Fallbackfee is disabled. Wait a few blocks or enable %s."), "-fallbackfee")};
    }

    // Calculate the cost of change
    // Cost of change is the cost of creating the change output + cost of spending the change output in the future.
    // For creating the change output now, we use the effective feerate.
    // For spending the change output in the future, we use the discard feerate for now.
    // So cost of change = (change output size * effective feerate) + (size of spending change output * discard feerate)
    coin_selection_params.m_change_fee = coin_selection_params.m_effective_feerate.GetFee(coin_selection_params.change_output_size);
    coin_selection_params.m_cost_of_change = coin_selection_params.m_discard_feerate.GetFee(coin_selection_params.change_spend_size) + coin_selection_params.m_change_fee;

    // vouts to the payees
    if (!coin_selection_params.m_subtract_fee_outputs) {
        coin_selection_params.tx_noinputs_size = 9; // Static vsize overhead + outputs vsize. 4 nVersion, 4 nLocktime, 1 input count
        coin_selection_params.tx_noinputs_size += GetSizeOfCompactSize(vecSend.size()); // bytes for output count
    }
    for (const auto& recipient : vecSend)
    {
        CTxOut txout(recipient.nAmount, recipient.scriptPubKey);

        // Include the fee cost for outputs.
        if (!coin_selection_params.m_subtract_fee_outputs) {
            coin_selection_params.tx_noinputs_size += ::GetSerializeSize(txout, PROTOCOL_VERSION);
        }

        if (IsDust(txout, wallet.chain().relayDustFee())) {
            return util::Error{_("Transaction amount too small")};
        }
        txNew.vout.push_back(txout);
    }

    // Include the fees for things that aren't inputs, excluding the change output
    const CAmount not_input_fees = coin_selection_params.m_effective_feerate.GetFee(coin_selection_params.tx_noinputs_size);
    CAmount selection_target = recipients_sum + not_input_fees;

    // This can only happen if feerate is 0, and requested destinations are value of 0 (e.g. OP_RETURN)
    // and no pre-selected inputs. This will result in 0-input transaction, which is consensus-invalid anyways
    if (selection_target == 0 && !coin_control.HasSelected()) {
        return util::Error{_("Transaction requires one destination of non-0 value, a non-0 feerate, or a pre-selected input")};
    }

    // Get available coins
    auto available_coins = AvailableCoins(wallet,
                                              &coin_control,
                                              coin_selection_params.m_effective_feerate,
                                              1,            /*nMinimumAmount*/
                                              MAX_MONEY,    /*nMaximumAmount*/
                                              MAX_MONEY,    /*nMinimumSumAmount*/
                                              0);           /*nMaximumCount*/

    // Choose coins to use
    std::optional<SelectionResult> result = SelectCoins(wallet, available_coins, /*nTargetValue=*/selection_target, coin_control, coin_selection_params);
    if (!result) {
        if (coin_control.nCoinType == CoinType::ONLY_NONDENOMINATED) {
            return util::Error{_("Unable to locate enough non-denominated funds for this transaction.")};
        } else if (coin_control.nCoinType == CoinType::ONLY_FULLY_MIXED) {
            return util::Error{_("Unable to locate enough mixed funds for this transaction.") +
                               Untranslated(" ") + strprintf(_("%s uses exact denominated amounts to send funds, you might simply need to mix some more coins."), gCoinJoinName)};
        }
        return util::Error{_("Insufficient funds.")};
    }
    TRACE5(coin_selection, selected_coins, wallet.GetName().c_str(), GetAlgorithmName(result->m_algo).c_str(), result->m_target, result->GetWaste(), result->GetSelectedValue());

    // Always make a change output
    // We will reduce the fee from this change output later, and remove the output if it is too small.
    const CAmount change_and_fee = result->GetSelectedValue() - recipients_sum;
    assert(change_and_fee >= 0);
    CTxOut newTxOut(change_and_fee, scriptChange);

    if (nChangePosInOut == -1) {
        // Insert change txn at random position:
        nChangePosInOut = rng_fast.randrange(txNew.vout.size() + 1);
    }
    else if ((unsigned int)nChangePosInOut > txNew.vout.size()) {
        return util::Error{_("Transaction change output index out of range")};
    }

    assert(nChangePosInOut != -1);
    auto change_position = txNew.vout.insert(txNew.vout.begin() + nChangePosInOut, newTxOut);

    // We're making a copy of vecSend because it's const, sortedVecSend should be used
    // in place of vecSend in all subsequent usage.
    std::vector<CRecipient> sortedVecSend{vecSend};
    if (sort_bip69) {
        std::sort(txNew.vout.begin(), txNew.vout.end(), CompareOutputBIP69());
        // The output reduction loop uses vecSend to map to txNew.vout, we need to
        // shuffle them both to ensure this mapping remains consistent
        std::sort(sortedVecSend.begin(), sortedVecSend.end(),
                    [](const CRecipient& a, const CRecipient& b) {
                        return a.nAmount < b.nAmount || (a.nAmount == b.nAmount && a.scriptPubKey < b.scriptPubKey);
                    });

        // If there was a change output added before, we must update its position now
        if (const auto it = std::find(txNew.vout.begin(), txNew.vout.end(), newTxOut); it != txNew.vout.end()) {
            change_position = it;
            nChangePosInOut = std::distance(txNew.vout.begin(), change_position);
        }
    };

    // The sequence number is set to non-maxint so that DiscourageFeeSniping
    // works.
    const uint32_t nSequence{CTxIn::SEQUENCE_FINAL - 1};
    for (const auto& coin : result->GetInputSet()) {
        txNew.vin.push_back(CTxIn(coin.outpoint, CScript(), nSequence));
    }
    DiscourageFeeSniping(txNew, rng_fast, wallet.chain(), wallet.GetLastBlockHash(), wallet.GetLastBlockHeight());

    // Fill in final vin and shuffle/sort it
    if (sort_bip69) { std::sort(txNew.vin.begin(), txNew.vin.end(), CompareInputBIP69()); }
    else { Shuffle(txNew.vin.begin(), txNew.vin.end(), coin_selection_params.rng_fast); }

    // Calculate the transaction fee
    int nBytes = CalculateMaximumSignedTxSize(CTransaction(txNew), &wallet, &coin_control);
    if (nBytes == -1) {
        return util::Error{_("Missing solving data for estimating transaction size")};
    }

    if (nExtraPayloadSize != 0) {
        // account for extra payload in fee calculation
        nBytes += GetSizeOfCompactSize(nExtraPayloadSize) + nExtraPayloadSize;
    }

    CAmount fee_needed = coin_selection_params.m_effective_feerate.GetFee(nBytes);

    if (!coin_selection_params.m_subtract_fee_outputs) {
        change_position->nValue -= fee_needed;
    }

    // We want to drop the change to fees if:
    // 1. The change output would be dust
    // 2. The change is within the (almost) exact match window, i.e. it is less than or equal to the cost of the change output (cost_of_change)
    // 3. We are working with fully mixed CoinJoin denominations
    CAmount change_amount = change_position->nValue;
    if (IsDust(*change_position, coin_selection_params.m_discard_feerate) || change_amount <= coin_selection_params.m_cost_of_change || coin_control.nCoinType == CoinType::ONLY_FULLY_MIXED)
    {
        nChangePosInOut = -1;
        change_amount = 0;
        txNew.vout.erase(change_position);

        nBytes = CalculateMaximumSignedTxSize(CTransaction(txNew), &wallet, &coin_control);
        fee_needed = coin_selection_params.m_effective_feerate.GetFee(nBytes);
    }

    nFeeRet = result->GetSelectedValue() - recipients_sum - change_amount;

    // Update nFeeRet in case fee_needed changed due to dropping the change output
    if (fee_needed <= change_and_fee - change_amount) {
        nFeeRet = change_and_fee - change_amount;
    }

    // Reduce output values for subtractFeeFromAmount
    if (coin_selection_params.m_subtract_fee_outputs) {
        CAmount to_reduce = fee_needed + change_amount - change_and_fee;
        int i = 0;
        bool fFirst = true;
        for (const auto& recipient : sortedVecSend)
        {
            if (i == nChangePosInOut) {
                ++i;
            }
            CTxOut& txout = txNew.vout[i];

            if (recipient.fSubtractFeeFromAmount)
            {
                txout.nValue -= to_reduce / outputs_to_subtract_fee_from; // Subtract fee equally from each selected recipient

                if (fFirst) // first receiver pays the remainder not divisible by output count
                {
                    fFirst = false;
                    txout.nValue -= to_reduce % outputs_to_subtract_fee_from;
                }
                // Error if this output is reduced to be below dust
                if (IsDust(txout, wallet.chain().relayDustFee())) {
                    if (txout.nValue < 0) {
                        return util::Error{_("The transaction amount is too small to pay the fee")};
                    } else {
                        return util::Error{_("The transaction amount is too small to send after the fee has been deducted")};
                    }
                }
            }
            ++i;
        }
        nFeeRet = fee_needed;
    }

    // Give up if change keypool ran out and change is required
    if (scriptChange.empty() && nChangePosInOut != -1) {
        return util::Error{error};
    }

    if (sign && !wallet.SignTransaction(txNew)) {
        return util::Error{_("Signing transaction failed")};
    }

    // Return the constructed transaction data.
    CTransactionRef tx = MakeTransactionRef(std::move(txNew));

    // Limit size
    if ((sign && ::GetSerializeSize(*tx, PROTOCOL_VERSION) > MAX_STANDARD_TX_SIZE) ||
        (!sign && static_cast<size_t>(nBytes) > MAX_STANDARD_TX_SIZE)) {
        return util::Error{_("Transaction too large")};
    }

    if (fee_needed > nFeeRet) {
        return util::Error{_("Fee needed > fee paid")};
    }

    if (nFeeRet > wallet.m_default_max_tx_fee) {
        return util::Error{TransactionErrorString(TransactionError::MAX_FEE_EXCEEDED)};
    }

    if (gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        // Lastly, ensure this tx will pass the mempool's chain limits
        if (!wallet.chain().checkChainLimits(tx)) {
            return util::Error{_("Transaction has too long of a mempool chain")};
        }
    }

    // Before we return success, we assume any change key will be used to prevent
    // accidental re-use.
    reservedest.KeepDestination();

    wallet.WalletLogPrintf("Fee Calculation: Fee:%d Bytes:%u Tgt:%d (requested %d) Reason:\"%s\" Decay %.5f: Estimation: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out) Fail: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out)\n",
              nFeeRet, nBytes, feeCalc.returnedTarget, feeCalc.desiredTarget, StringForFeeReason(feeCalc.reason), feeCalc.est.decay,
              feeCalc.est.pass.start, feeCalc.est.pass.end,
              (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) > 0.0 ? 100 * feeCalc.est.pass.withinTarget / (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) : 0.0,
              feeCalc.est.pass.withinTarget, feeCalc.est.pass.totalConfirmed, feeCalc.est.pass.inMempool, feeCalc.est.pass.leftMempool,
              feeCalc.est.fail.start, feeCalc.est.fail.end,
              (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) > 0.0 ? 100 * feeCalc.est.fail.withinTarget / (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) : 0.0,
              feeCalc.est.fail.withinTarget, feeCalc.est.fail.totalConfirmed, feeCalc.est.fail.inMempool, feeCalc.est.fail.leftMempool);
    return CreatedTransactionResult(tx, nFeeRet, nChangePosInOut, feeCalc);
}

util::Result<CreatedTransactionResult> CreateTransaction(
        CWallet& wallet,
        const std::vector<CRecipient>& vecSend,
        int change_pos,
        const CCoinControl& coin_control,
        bool sign,
        int nExtraPayloadSize)
{
    if (vecSend.empty()) {
        return util::Error{_("Transaction must have at least one recipient")};
    }

    if (std::any_of(vecSend.cbegin(), vecSend.cend(), [](const auto& recipient){ return recipient.nAmount < 0; })) {
        return util::Error{_("Transaction amounts must not be negative")};
    }

    LOCK(wallet.cs_wallet);

    auto res = CreateTransactionInternal(wallet, vecSend, change_pos, coin_control, sign, nExtraPayloadSize);
    TRACE4(coin_selection, normal_create_tx_internal, wallet.GetName().c_str(), bool(res),
           res ? res->fee : 0, res ? res->change_pos : 0);
    if (!res) return res;
    const auto& txr_ungrouped = *res;
    // try with avoidpartialspends unless it's enabled already
    if (txr_ungrouped.fee > 0 /* 0 means non-functional fee rate estimation */ && wallet.m_max_aps_fee > -1 && !coin_control.m_avoid_partial_spends) {
        TRACE1(coin_selection, attempting_aps_create_tx, wallet.GetName().c_str());
        CCoinControl tmp_cc = coin_control;
        tmp_cc.m_avoid_partial_spends = true;

        // Re-use the change destination from the first creation attempt to avoid skipping BIP44 indexes
        const int ungrouped_change_pos = txr_ungrouped.change_pos;
        if (ungrouped_change_pos != -1) {
            ExtractDestination(txr_ungrouped.tx->vout[ungrouped_change_pos].scriptPubKey, tmp_cc.destChange);
        }

        auto txr_grouped = CreateTransactionInternal(wallet, vecSend, change_pos, tmp_cc, sign, nExtraPayloadSize);
        // if fee of this alternative one is within the range of the max fee, we use this one
        const bool use_aps{txr_grouped.has_value() ? (txr_grouped->fee <= txr_ungrouped.fee + wallet.m_max_aps_fee) : false};
        TRACE5(coin_selection, aps_create_tx_internal, wallet.GetName().c_str(), use_aps, txr_grouped.has_value(),
               txr_grouped.has_value() ? txr_grouped->fee : 0, txr_grouped.has_value() ? txr_grouped->change_pos : 0);
        if (txr_grouped) {
            wallet.WalletLogPrintf("Fee non-grouped = %lld, grouped = %lld, using %s\n",
                txr_ungrouped.fee, txr_grouped->fee, use_aps ? "grouped" : "non-grouped");
            if (use_aps) return txr_grouped;
        }
    }
    return res;
}

bool FundTransaction(CWallet& wallet, CMutableTransaction& tx, CAmount& nFeeRet, int& nChangePosInOut, bilingual_str& error, bool lockUnspents, const std::set<int>& setSubtractFeeFromOutputs, CCoinControl coinControl)
{
    std::vector<CRecipient> vecSend;

    // If no specific change position was requested, apply BIP69
    if (nChangePosInOut == -1) {
        std::sort(tx.vin.begin(), tx.vin.end(), CompareInputBIP69());
        std::sort(tx.vout.begin(), tx.vout.end(), CompareOutputBIP69());
    }

    // Turn the txout set into a CRecipient vector.
    for (size_t idx = 0; idx < tx.vout.size(); idx++) {
        const CTxOut& txOut = tx.vout[idx];
        CRecipient recipient = {txOut.scriptPubKey, txOut.nValue, setSubtractFeeFromOutputs.count(idx) == 1};
        vecSend.push_back(recipient);
    }

    // Acquire the locks to prevent races to the new locked unspents between the
    // CreateTransaction call and LockCoin calls (when lockUnspents is true).
    LOCK(wallet.cs_wallet);

    // Fetch specified UTXOs from the UTXO set to get the scriptPubKeys and values of the outputs being selected
    // and to match with the given solving_data. Only used for non-wallet outputs.
    std::map<COutPoint, Coin> coins;
    for (const CTxIn& txin : tx.vin) {
        coins[txin.prevout]; // Create empty map entry keyed by prevout.
    }
    wallet.chain().findCoins(coins);

    for (const CTxIn& txin : tx.vin) {
        // if it's not in the wallet and corresponding UTXO is found than select as external output
        const auto& outPoint = txin.prevout;
        if (wallet.mapWallet.find(outPoint.hash) == wallet.mapWallet.end() && !coins[outPoint].out.IsNull()) {
            coinControl.SelectExternal(outPoint, coins[outPoint].out);
        } else {
            coinControl.Select(outPoint);
        }
    }

    auto res = CreateTransaction(wallet, vecSend, nChangePosInOut, coinControl, /*sign=*/false, tx.vExtraPayload.size());
    if (!res) {
        error = util::ErrorString(res);
        return false;
    }
    const auto& txr = *res;
    CTransactionRef tx_new = txr.tx;
    nFeeRet = txr.fee;
    nChangePosInOut = txr.change_pos;

    if (nChangePosInOut != -1) {
        tx.vout.insert(tx.vout.begin() + nChangePosInOut, tx_new->vout[nChangePosInOut]);
    }

    // Copy output sizes from new transaction; they may have had the fee
    // subtracted from them.
    for (unsigned int idx = 0; idx < tx.vout.size(); idx++) {
        tx.vout[idx].nValue = tx_new->vout[idx].nValue;
    }

    // Add new txins while keeping original txin scriptSig/order.
    for (const CTxIn& txin : tx_new->vin) {
        if (!coinControl.IsSelected(txin.prevout)) {
            tx.vin.push_back(txin);

        }
        if (lockUnspents) {
            wallet.LockCoin(txin.prevout);
        }

    }

    return true;
}

bool GenBudgetSystemCollateralTx(CWallet& wallet, CTransactionRef& tx, uint256 hash, CAmount amount, const COutPoint& outpoint)
{
    const CScript scriptChange{CScript() << OP_RETURN << ToByteVector(hash)};
    const std::vector<CRecipient> vecSend{{scriptChange, amount, false}};

    CCoinControl coinControl;
    if (!outpoint.IsNull()) {
        coinControl.Select(outpoint);
    }

    auto res{CreateTransaction(wallet, vecSend, RANDOM_CHANGE_POSITION, coinControl)};
    if (!res) {
        wallet.WalletLogPrintf("%s -- Error: %s\n", __func__, util::ErrorString(res).original);
        return false;
    }
    tx = res->tx;
    return true;
}
} // namespace wallet
