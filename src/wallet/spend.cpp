// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <interfaces/chain.h>
#include <numeric>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/signingprovider.h>
#include <util/check.h>
#include <util/fees.h>
#include <util/moneystr.h>
#include <util/rbf.h>
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

int CalculateMaximumSignedInputSize(const CTxOut& txout, const COutPoint outpoint, const SigningProvider* provider, bool can_grind_r, const CCoinControl* coin_control)
{
    CMutableTransaction txn;
    txn.vin.push_back(CTxIn(outpoint));
    if (!provider || !DummySignInput(*provider, txn.vin[0], txout, can_grind_r, coin_control)) {
        return -1;
    }
    return GetVirtualTransactionInputSize(txn.vin[0]);
}

int CalculateMaximumSignedInputSize(const CTxOut& txout, const CWallet* wallet, const CCoinControl* coin_control)
{
    const std::unique_ptr<SigningProvider> provider = wallet->GetSolvingProvider(txout.scriptPubKey);
    return CalculateMaximumSignedInputSize(txout, COutPoint(), provider.get(), wallet->CanGrindR(), coin_control);
}

// txouts needs to be in the order of tx.vin
TxSize CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, const std::vector<CTxOut>& txouts, const CCoinControl* coin_control)
{
    CMutableTransaction txNew(tx);
    if (!wallet->DummySignTx(txNew, txouts, coin_control)) {
        return TxSize{-1, -1};
    }
    CTransaction ctx(txNew);
    int64_t vsize = GetVirtualTransactionSize(ctx);
    int64_t weight = GetTransactionWeight(ctx);
    return TxSize{vsize, weight};
}

TxSize CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, const CCoinControl* coin_control)
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
                return TxSize{-1, -1};
            }
            txouts.emplace_back(txout);
        } else {
            return TxSize{-1, -1};
        }
    }
    return CalculateMaximumSignedTxSize(tx, wallet, txouts, coin_control);
}

size_t CoinsResult::Size() const
{
    size_t size{0};
    for (const auto& it : coins) {
        size += it.second.size();
    }
    return size;
}

std::vector<COutput> CoinsResult::All() const
{
    std::vector<COutput> all;
    all.reserve(coins.size());
    for (const auto& it : coins) {
        all.insert(all.end(), it.second.begin(), it.second.end());
    }
    return all;
}

void CoinsResult::Clear() {
    coins.clear();
}

void CoinsResult::Erase(const std::unordered_set<COutPoint, SaltedOutpointHasher>& coins_to_remove)
{
    for (auto& [type, vec] : coins) {
        auto remove_it = std::remove_if(vec.begin(), vec.end(), [&](const COutput& coin) {
            // remove it if it's on the set
            if (coins_to_remove.count(coin.outpoint) == 0) return false;

            // update cached amounts
            total_amount -= coin.txout.nValue;
            if (coin.HasEffectiveValue()) total_effective_amount = *total_effective_amount - coin.GetEffectiveValue();
            return true;
        });
        vec.erase(remove_it, vec.end());
    }
}

void CoinsResult::Shuffle(FastRandomContext& rng_fast)
{
    for (auto& it : coins) {
        ::Shuffle(it.second.begin(), it.second.end(), rng_fast);
    }
}

void CoinsResult::Add(OutputType type, const COutput& out)
{
    coins[type].emplace_back(out);
    total_amount += out.txout.nValue;
    if (out.HasEffectiveValue()) {
        total_effective_amount = total_effective_amount.has_value() ?
                *total_effective_amount + out.GetEffectiveValue() : out.GetEffectiveValue();
    }
}

static OutputType GetOutputType(TxoutType type, bool is_from_p2sh)
{
    switch (type) {
        case TxoutType::WITNESS_V1_TAPROOT:
            return OutputType::BECH32M;
        case TxoutType::WITNESS_V0_KEYHASH:
        case TxoutType::WITNESS_V0_SCRIPTHASH:
            if (is_from_p2sh) return OutputType::P2SH_SEGWIT;
            else return OutputType::BECH32;
        case TxoutType::SCRIPTHASH:
        case TxoutType::PUBKEYHASH:
            return OutputType::LEGACY;
        default:
            return OutputType::UNKNOWN;
    }
}

// Fetch and validate the coin control selected inputs.
// Coins could be internal (from the wallet) or external.
util::Result<PreSelectedInputs> FetchSelectedInputs(const CWallet& wallet, const CCoinControl& coin_control,
                                            const CoinSelectionParams& coin_selection_params) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    PreSelectedInputs result;
    std::vector<COutPoint> vPresetInputs;
    coin_control.ListSelected(vPresetInputs);
    const bool can_grind_r = wallet.CanGrindR();
    for (const COutPoint& outpoint : vPresetInputs) {
        int input_bytes = -1;
        CTxOut txout;
        if (auto ptr_wtx = wallet.GetWalletTx(outpoint.hash)) {
            // Clearly invalid input, fail
            if (ptr_wtx->tx->vout.size() <= outpoint.n) {
                return util::Error{strprintf(_("Invalid pre-selected input %s"), outpoint.ToString())};
            }
            txout = ptr_wtx->tx->vout.at(outpoint.n);
            input_bytes = CalculateMaximumSignedInputSize(txout, &wallet, &coin_control);
        } else {
            // The input is external. We did not find the tx in mapWallet.
            if (!coin_control.GetExternalOutput(outpoint, txout)) {
                return util::Error{strprintf(_("Not found pre-selected input %s"), outpoint.ToString())};
            }
        }

        if (input_bytes == -1) {
            input_bytes = CalculateMaximumSignedInputSize(txout, outpoint, &coin_control.m_external_provider, can_grind_r, &coin_control);
        }

        // If available, override calculated size with coin control specified size
        if (coin_control.HasInputWeight(outpoint)) {
            input_bytes = GetVirtualTransactionSize(coin_control.GetInputWeight(outpoint), 0, 0);
        }

        if (input_bytes == -1) {
            return util::Error{strprintf(_("Not solvable pre-selected input %s"), outpoint.ToString())}; // Not solvable, can't estimate size for fee
        }

        /* Set some defaults for depth, spendable, solvable, safe, time, and from_me as these don't matter for preset inputs since no selection is being done. */
        COutput output(outpoint, txout, /*depth=*/ 0, input_bytes, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, coin_selection_params.m_effective_feerate);
        result.Insert(output, coin_selection_params.m_subtract_fee_outputs);
    }
    return result;
}

CoinsResult AvailableCoins(const CWallet& wallet,
                           const CCoinControl* coinControl,
                           std::optional<CFeeRate> feerate,
                           const CoinFilterParams& params)
{
    AssertLockHeld(wallet.cs_wallet);

    CoinsResult result;
    // Either the WALLET_FLAG_AVOID_REUSE flag is not set (in which case we always allow), or we default to avoiding, and only in the case where
    // a coin control object is provided, and has the avoid address reuse flag set to false, do we allow already used addresses
    bool allow_used_addresses = !wallet.IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE) || (coinControl && !coinControl->m_avoid_address_reuse);
    const int min_depth = {coinControl ? coinControl->m_min_depth : DEFAULT_MIN_DEPTH};
    const int max_depth = {coinControl ? coinControl->m_max_depth : DEFAULT_MAX_DEPTH};
    const bool only_safe = {coinControl ? !coinControl->m_include_unsafe_inputs : true};
    const bool can_grind_r = wallet.CanGrindR();

    std::set<uint256> trusted_parents;
    for (const auto& entry : wallet.mapWallet)
    {
        const uint256& wtxid = entry.first;
        const CWalletTx& wtx = entry.second;

        if (wallet.IsTxImmatureCoinBase(wtx) && !params.include_immature_coinbase)
            continue;

        int nDepth = wallet.GetTxDepthInMainChain(wtx);
        if (nDepth < 0)
            continue;

        // We should not consider coins which aren't at least in our mempool
        // It's possible for these to be conflicted via ancestors which we may never be able to detect
        if (nDepth == 0 && !wtx.InMempool())
            continue;

        bool safeTx = CachedTxIsTrusted(wallet, wtx, trusted_parents);

        // We should not consider coins from transactions that are replacing
        // other transactions.
        //
        // Example: There is a transaction A which is replaced by bumpfee
        // transaction B. In this case, we want to prevent creation of
        // a transaction B' which spends an output of B.
        //
        // Reason: If transaction A were initially confirmed, transactions B
        // and B' would no longer be valid, so the user would have to create
        // a new transaction C to replace B'. However, in the case of a
        // one-block reorg, transactions B' and C might BOTH be accepted,
        // when the user only wanted one of them. Specifically, there could
        // be a 1-block reorg away from the chain where transactions A and C
        // were accepted to another chain where B, B', and C were all
        // accepted.
        if (nDepth == 0 && wtx.mapValue.count("replaces_txid")) {
            safeTx = false;
        }

        // Similarly, we should not consider coins from transactions that
        // have been replaced. In the example above, we would want to prevent
        // creation of a transaction A' spending an output of A, because if
        // transaction B were initially confirmed, conflicting with A and
        // A', we wouldn't want to the user to create a transaction D
        // intending to replace A', but potentially resulting in a scenario
        // where A, A', and D could all be accepted (instead of just B and
        // D, or just A and A' like the user would want).
        if (nDepth == 0 && wtx.mapValue.count("replaced_by_txid")) {
            safeTx = false;
        }

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

            if (output.nValue < params.min_amount || output.nValue > params.max_amount)
                continue;

            // Skip manually selected coins (the caller can fetch them directly)
            if (coinControl && coinControl->HasSelected() && coinControl->IsSelected(outpoint))
                continue;

            if (wallet.IsLockedCoin(outpoint) && params.skip_locked)
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

            int input_bytes = CalculateMaximumSignedInputSize(output, COutPoint(), provider.get(), can_grind_r, coinControl);
            bool solvable = provider ? InferDescriptor(output.scriptPubKey, *provider)->IsSolvable() : false;
            bool spendable = ((mine & ISMINE_SPENDABLE) != ISMINE_NO) || (((mine & ISMINE_WATCH_ONLY) != ISMINE_NO) && (coinControl && coinControl->fAllowWatchOnly && solvable));

            // Filter by spendable outputs only
            if (!spendable && params.only_spendable) continue;

            // Obtain script type
            std::vector<std::vector<uint8_t>> script_solutions;
            TxoutType type = Solver(output.scriptPubKey, script_solutions);

            // If the output is P2SH and solvable, we want to know if it is
            // a P2SH (legacy) or one of P2SH-P2WPKH, P2SH-P2WSH (P2SH-Segwit). We can determine
            // this from the redeemScript. If the output is not solvable, it will be classified
            // as a P2SH (legacy), since we have no way of knowing otherwise without the redeemScript
            bool is_from_p2sh{false};
            if (type == TxoutType::SCRIPTHASH && solvable) {
                CScript script;
                if (!provider->GetCScript(CScriptID(uint160(script_solutions[0])), script)) continue;
                type = Solver(script, script_solutions);
                is_from_p2sh = true;
            }

            result.Add(GetOutputType(type, is_from_p2sh),
                       COutput(outpoint, output, nDepth, input_bytes, spendable, solvable, safeTx, wtx.GetTxTime(), tx_from_me, feerate));

            // Checks the sum amount of all UTXO's.
            if (params.min_sum_amount != MAX_MONEY) {
                if (result.GetTotalAmount() >= params.min_sum_amount) {
                    return result;
                }
            }

            // Checks the maximum number of UTXO's.
            if (params.max_count > 0 && result.Size() >= params.max_count) {
                return result;
            }
        }
    }

    return result;
}

CoinsResult AvailableCoinsListUnspent(const CWallet& wallet, const CCoinControl* coinControl, CoinFilterParams params)
{
    params.only_spendable = false;
    return AvailableCoins(wallet, coinControl, /*feerate=*/ std::nullopt, params);
}

CAmount GetAvailableBalance(const CWallet& wallet, const CCoinControl* coinControl)
{
    LOCK(wallet.cs_wallet);
    return AvailableCoins(wallet, coinControl).GetTotalAmount();
}

const CTxOut& FindNonChangeParentOutput(const CWallet& wallet, const COutPoint& outpoint)
{
    AssertLockHeld(wallet.cs_wallet);
    const CWalletTx* wtx{Assert(wallet.GetWalletTx(outpoint.hash))};

    const CTransaction* ptx = wtx->tx.get();
    int n = outpoint.n;
    while (OutputIsChange(wallet, ptx->vout[n]) && ptx->vin.size() > 0) {
        const COutPoint& prevout = ptx->vin[0].prevout;
        const CWalletTx* it = wallet.GetWalletTx(prevout.hash);
        if (!it || it->tx->vout.size() <= prevout.n ||
            !wallet.IsMine(it->tx->vout[prevout.n])) {
            break;
        }
        ptx = it->tx.get();
        n = prevout.n;
    }
    return ptx->vout[n];
}

std::map<CTxDestination, std::vector<COutput>> ListCoins(const CWallet& wallet)
{
    AssertLockHeld(wallet.cs_wallet);

    std::map<CTxDestination, std::vector<COutput>> result;

    CCoinControl coin_control;
    // Include watch-only for LegacyScriptPubKeyMan wallets without private keys
    coin_control.fAllowWatchOnly = wallet.GetLegacyScriptPubKeyMan() && wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    CoinFilterParams coins_params;
    coins_params.only_spendable = false;
    coins_params.skip_locked = false;
    for (const COutput& coin : AvailableCoins(wallet, &coin_control, /*feerate=*/std::nullopt, coins_params).All()) {
        CTxDestination address;
        if ((coin.spendable || (wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && coin.solvable)) &&
            ExtractDestination(FindNonChangeParentOutput(wallet, coin.outpoint).scriptPubKey, address)) {
            result[address].emplace_back(coin);
        }
    }
    return result;
}

FilteredOutputGroups GroupOutputs(const CWallet& wallet,
                          const CoinsResult& coins,
                          const CoinSelectionParams& coin_sel_params,
                          const std::vector<SelectionFilter>& filters)
{
    FilteredOutputGroups filtered_groups;

    if (!coin_sel_params.m_avoid_partial_spends) {
        // Allowing partial spends means no grouping. Each COutput gets its own OutputGroup
        for (const auto& [type, outputs] : coins.coins) {
            for (const COutput& output : outputs) {
                // Skip outputs we cannot spend
                if (!output.spendable) continue;

                // Get mempool info
                size_t ancestors, descendants;
                wallet.chain().getTransactionAncestry(output.outpoint.hash, ancestors, descendants);

                // Create a new group per output and add it to the all groups vector
                OutputGroup group(coin_sel_params);
                group.Insert(std::make_shared<COutput>(output), ancestors, descendants);

                // Each filter maps to a different set of groups
                for (const auto& sel_filter : filters) {
                    const auto& filter = sel_filter.filter;
                    if (!group.EligibleForSpending(filter)) continue;
                    filtered_groups[filter].Push(group, type, /*insert_positive=*/true, /*insert_mixed=*/true);
                }
            }
        }
        return filtered_groups;
    }

    // We want to combine COutputs that have the same scriptPubKey into single OutputGroups
    // except when there are more than OUTPUT_GROUP_MAX_ENTRIES COutputs grouped in an OutputGroup.
    // To do this, we maintain a map where the key is the scriptPubKey and the value is a vector of OutputGroups.
    // For each COutput, we check if the scriptPubKey is in the map, and if it is, the COutput is added
    // to the last OutputGroup in the vector for the scriptPubKey. When the last OutputGroup has
    // OUTPUT_GROUP_MAX_ENTRIES COutputs, a new OutputGroup is added to the end of the vector.
    typedef std::map<std::pair<CScript, OutputType>, std::vector<OutputGroup>> ScriptPubKeyToOutgroup;
    const auto& group_outputs = [](
            const COutput& output, OutputType type, size_t ancestors, size_t descendants,
            ScriptPubKeyToOutgroup& groups_map, const CoinSelectionParams& coin_sel_params,
            bool positive_only) {
        std::vector<OutputGroup>& groups = groups_map[std::make_pair(output.txout.scriptPubKey,type)];

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

        // Filter for positive only before adding the output to group
        if (!positive_only || output.GetEffectiveValue() > 0) {
            group->Insert(std::make_shared<COutput>(output), ancestors, descendants);
        }
    };

    ScriptPubKeyToOutgroup spk_to_groups_map;
    ScriptPubKeyToOutgroup spk_to_positive_groups_map;
    for (const auto& [type, outs] : coins.coins) {
        for (const COutput& output : outs) {
            // Skip outputs we cannot spend
            if (!output.spendable) continue;

            size_t ancestors, descendants;
            wallet.chain().getTransactionAncestry(output.outpoint.hash, ancestors, descendants);

            group_outputs(output, type, ancestors, descendants, spk_to_groups_map, coin_sel_params, /*positive_only=*/ false);
            group_outputs(output, type, ancestors, descendants, spk_to_positive_groups_map,
                          coin_sel_params, /*positive_only=*/ true);
        }
    }

    // Now we go through the entire maps and pull out the OutputGroups
    const auto& push_output_groups = [&](const ScriptPubKeyToOutgroup& groups_map, bool positive_only) {
        for (const auto& [script, groups] : groups_map) {
            // Go through the vector backwards. This allows for the first item we deal with being the partial group.
            for (auto group_it = groups.rbegin(); group_it != groups.rend(); group_it++) {
                const OutputGroup& group = *group_it;

                // Each filter maps to a different set of groups
                for (const auto& sel_filter : filters) {
                    const auto& filter = sel_filter.filter;
                    if (!group.EligibleForSpending(filter)) continue;

                    // Don't include partial groups if there are full groups too and we don't want partial groups
                    if (group_it == groups.rbegin() && groups.size() > 1 && !filter.m_include_partial_groups) {
                        continue;
                    }

                    OutputType type = script.second;
                    // Either insert the group into the positive-only groups or the mixed ones.
                    filtered_groups[filter].Push(group, type, positive_only, /*insert_mixed=*/!positive_only);
                }
            }
        }
    };

    push_output_groups(spk_to_groups_map, /*positive_only=*/ false);
    push_output_groups(spk_to_positive_groups_map, /*positive_only=*/ true);

    return filtered_groups;
}

// Returns true if the result contains an error and the message is not empty
static bool HasErrorMsg(const util::Result<SelectionResult>& res) { return !util::ErrorString(res).empty(); }

util::Result<SelectionResult> AttemptSelection(const CAmount& nTargetValue, OutputGroupTypeMap& groups,
                               const CoinSelectionParams& coin_selection_params, bool allow_mixed_output_types)
{
    // Run coin selection on each OutputType and compute the Waste Metric
    std::vector<SelectionResult> results;
    for (auto& [type, group] : groups.groups_by_type) {
        auto result{ChooseSelectionResult(nTargetValue, group, coin_selection_params)};
        // If any specific error message appears here, then something particularly wrong happened.
        if (HasErrorMsg(result)) return result; // So let's return the specific error.
        // Append the favorable result.
        if (result) results.push_back(*result);
    }
    // If we have at least one solution for funding the transaction without mixing, choose the minimum one according to waste metric
    // and return the result
    if (results.size() > 0) return *std::min_element(results.begin(), results.end());

    // If we can't fund the transaction from any individual OutputType, run coin selection one last time
    // over all available coins, which would allow mixing.
    // If TypesCount() <= 1, there is nothing to mix.
    if (allow_mixed_output_types && groups.TypesCount() > 1) {
        return ChooseSelectionResult(nTargetValue, groups.all_groups, coin_selection_params);
    }
    // Either mixing is not allowed and we couldn't find a solution from any single OutputType, or mixing was allowed and we still couldn't
    // find a solution using all available coins
    return util::Error();
};

util::Result<SelectionResult> ChooseSelectionResult(const CAmount& nTargetValue, Groups& groups, const CoinSelectionParams& coin_selection_params)
{
    // Vector of results. We will choose the best one based on waste.
    std::vector<SelectionResult> results;

    if (auto bnb_result{SelectCoinsBnB(groups.positive_group, nTargetValue, coin_selection_params.m_cost_of_change)}) {
        results.push_back(*bnb_result);
    }

    // The knapsack solver has some legacy behavior where it will spend dust outputs. We retain this behavior, so don't filter for positive only here.
    if (auto knapsack_result{KnapsackSolver(groups.mixed_group, nTargetValue, coin_selection_params.m_min_change_target, coin_selection_params.rng_fast)}) {
        knapsack_result->ComputeAndSetWaste(coin_selection_params.min_viable_change, coin_selection_params.m_cost_of_change, coin_selection_params.m_change_fee);
        results.push_back(*knapsack_result);
    }

    if (auto srd_result{SelectCoinsSRD(groups.positive_group, nTargetValue, coin_selection_params.rng_fast)}) {
        srd_result->ComputeAndSetWaste(coin_selection_params.min_viable_change, coin_selection_params.m_cost_of_change, coin_selection_params.m_change_fee);
        results.push_back(*srd_result);
    }

    if (results.empty()) {
        // No solution found
        return util::Error();
    }

    std::vector<SelectionResult> eligible_results;
    std::copy_if(results.begin(), results.end(), std::back_inserter(eligible_results), [coin_selection_params](const SelectionResult& result) {
        const auto initWeight{coin_selection_params.tx_noinputs_size * WITNESS_SCALE_FACTOR};
        return initWeight + result.GetWeight() <= static_cast<int>(MAX_STANDARD_TX_WEIGHT);
    });

    if (eligible_results.empty()) {
        return util::Error{_("The inputs size exceeds the maximum weight. "
                             "Please try sending a smaller amount or manually consolidating your wallet's UTXOs")};
    }

    // Choose the result with the least waste
    // If the waste is the same, choose the one which spends more inputs.
    auto& best_result = *std::min_element(eligible_results.begin(), eligible_results.end());
    return best_result;
}

util::Result<SelectionResult> SelectCoins(const CWallet& wallet, CoinsResult& available_coins, const PreSelectedInputs& pre_set_inputs,
                                          const CAmount& nTargetValue, const CCoinControl& coin_control,
                                          const CoinSelectionParams& coin_selection_params)
{
    // Deduct preset inputs amount from the search target
    CAmount selection_target = nTargetValue - pre_set_inputs.total_amount;

    // Return if automatic coin selection is disabled, and we don't cover the selection target
    if (!coin_control.m_allow_other_inputs && selection_target > 0) {
        return util::Error{_("The preselected coins total amount does not cover the transaction target. "
                             "Please allow other inputs to be automatically selected or include more coins manually")};
    }

    // Return if we can cover the target only with the preset inputs
    if (selection_target <= 0) {
        SelectionResult result(nTargetValue, SelectionAlgorithm::MANUAL);
        result.AddInputs(pre_set_inputs.coins, coin_selection_params.m_subtract_fee_outputs);
        result.ComputeAndSetWaste(coin_selection_params.min_viable_change, coin_selection_params.m_cost_of_change, coin_selection_params.m_change_fee);
        return result;
    }

    // Return early if we cannot cover the target with the wallet's UTXO.
    // We use the total effective value if we are not subtracting fee from outputs and 'available_coins' contains the data.
    CAmount available_coins_total_amount = coin_selection_params.m_subtract_fee_outputs ? available_coins.GetTotalAmount() :
            (available_coins.GetEffectiveTotalAmount().has_value() ? *available_coins.GetEffectiveTotalAmount() : 0);
    if (selection_target > available_coins_total_amount) {
        return util::Error(); // Insufficient funds
    }

    // Start wallet Coin Selection procedure
    auto op_selection_result = AutomaticCoinSelection(wallet, available_coins, selection_target, coin_control, coin_selection_params);
    if (!op_selection_result) return op_selection_result;

    // If needed, add preset inputs to the automatic coin selection result
    if (!pre_set_inputs.coins.empty()) {
        SelectionResult preselected(pre_set_inputs.total_amount, SelectionAlgorithm::MANUAL);
        preselected.AddInputs(pre_set_inputs.coins, coin_selection_params.m_subtract_fee_outputs);
        op_selection_result->Merge(preselected);
        op_selection_result->ComputeAndSetWaste(coin_selection_params.min_viable_change,
                                                coin_selection_params.m_cost_of_change,
                                                coin_selection_params.m_change_fee);
    }
    return op_selection_result;
}

util::Result<SelectionResult> AutomaticCoinSelection(const CWallet& wallet, CoinsResult& available_coins, const CAmount& value_to_select, const CCoinControl& coin_control, const CoinSelectionParams& coin_selection_params)
{
    unsigned int limit_ancestor_count = 0;
    unsigned int limit_descendant_count = 0;
    wallet.chain().getPackageLimits(limit_ancestor_count, limit_descendant_count);
    const size_t max_ancestors = (size_t)std::max<int64_t>(1, limit_ancestor_count);
    const size_t max_descendants = (size_t)std::max<int64_t>(1, limit_descendant_count);
    const bool fRejectLongChains = gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

    // form groups from remaining coins; note that preset coins will not
    // automatically have their associated (same address) coins included
    if (coin_control.m_avoid_partial_spends && available_coins.Size() > OUTPUT_GROUP_MAX_ENTRIES) {
        // Cases where we have 101+ outputs all pointing to the same destination may result in
        // privacy leaks as they will potentially be deterministically sorted. We solve that by
        // explicitly shuffling the outputs before processing
        available_coins.Shuffle(coin_selection_params.rng_fast);
    }

    // Coin Selection attempts to select inputs from a pool of eligible UTXOs to fund the
    // transaction at a target feerate. If an attempt fails, more attempts may be made using a more
    // permissive CoinEligibilityFilter.
    util::Result<SelectionResult> res = [&] {
        // Place coins eligibility filters on a scope increasing order.
        std::vector<SelectionFilter> ordered_filters{
                // If possible, fund the transaction with confirmed UTXOs only. Prefer at least six
                // confirmations on outputs received from other wallets and only spend confirmed change.
                {CoinEligibilityFilter(1, 6, 0), /*allow_mixed_output_types=*/false},
                {CoinEligibilityFilter(1, 1, 0)},
        };
        // Fall back to using zero confirmation change (but with as few ancestors in the mempool as
        // possible) if we cannot fund the transaction otherwise.
        if (wallet.m_spend_zero_conf_change) {
            ordered_filters.push_back({CoinEligibilityFilter(0, 1, 2)});
            ordered_filters.push_back({CoinEligibilityFilter(0, 1, std::min(size_t{4}, max_ancestors/3), std::min(size_t{4}, max_descendants/3))});
            ordered_filters.push_back({CoinEligibilityFilter(0, 1, max_ancestors/2, max_descendants/2)});
            // If partial groups are allowed, relax the requirement of spending OutputGroups (groups
            // of UTXOs sent to the same address, which are obviously controlled by a single wallet)
            // in their entirety.
            ordered_filters.push_back({CoinEligibilityFilter(0, 1, max_ancestors-1, max_descendants-1, /*include_partial=*/true)});
            // Try with unsafe inputs if they are allowed. This may spend unconfirmed outputs
            // received from other wallets.
            if (coin_control.m_include_unsafe_inputs) {
                ordered_filters.push_back({CoinEligibilityFilter(/*conf_mine=*/0, /*conf_theirs*/0, max_ancestors-1, max_descendants-1, /*include_partial=*/true)});
            }
            // Try with unlimited ancestors/descendants. The transaction will still need to meet
            // mempool ancestor/descendant policy to be accepted to mempool and broadcasted, but
            // OutputGroups use heuristics that may overestimate ancestor/descendant counts.
            if (!fRejectLongChains) {
                ordered_filters.push_back({CoinEligibilityFilter(0, 1, std::numeric_limits<uint64_t>::max(),
                                                                   std::numeric_limits<uint64_t>::max(),
                                                                   /*include_partial=*/true)});
            }
        }

        // Group outputs and map them by coin eligibility filter
        FilteredOutputGroups filtered_groups = GroupOutputs(wallet, available_coins, coin_selection_params, ordered_filters);

        // Walk-through the filters until the solution gets found.
        // If no solution is found, return the first detailed error (if any).
        // future: add "error level" so the worst one can be picked instead.
        std::vector<util::Result<SelectionResult>> res_detailed_errors;
        for (const auto& select_filter : ordered_filters) {
            auto it = filtered_groups.find(select_filter.filter);
            if (it == filtered_groups.end()) continue;
            if (auto res{AttemptSelection(value_to_select, it->second,
                                          coin_selection_params, select_filter.allow_mixed_output_types)}) {
                return res; // result found
            } else {
                // If any specific error message appears here, then something particularly wrong might have happened.
                // Save the error and continue the selection process. So if no solutions gets found, we can return
                // the detailed error to the upper layers.
                if (HasErrorMsg(res)) res_detailed_errors.emplace_back(res);
            }
        }
        // Coin Selection failed.
        return res_detailed_errors.empty() ? util::Result<SelectionResult>(util::Error()) : res_detailed_errors.front();
    }();

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
        // May be MAX NONFINAL to disable both BIP68 and BIP125
        if (in.nSequence == CTxIn::MAX_SEQUENCE_NONFINAL) continue;
        // May be MAX BIP125 to disable BIP68 and enable BIP125
        if (in.nSequence == MAX_BIP125_RBF_SEQUENCE) continue;
        // The wallet does not support any other sequence-use right now.
        assert(false);
    }
}

static util::Result<CreatedTransactionResult> CreateTransactionInternal(
        CWallet& wallet,
        const std::vector<CRecipient>& vecSend,
        int change_pos,
        const CCoinControl& coin_control,
        bool sign) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    AssertLockHeld(wallet.cs_wallet);

    // out variables, to be packed into returned result structure
    int nChangePosInOut = change_pos;

    FastRandomContext rng_fast;
    CMutableTransaction txNew; // The resulting transaction that we make
    // SYSCOIN
    txNew.nVersion = coin_control.m_version;
    CoinSelectionParams coin_selection_params{rng_fast}; // Parameters for coin selection, init with dummy
    coin_selection_params.m_avoid_partial_spends = coin_control.m_avoid_partial_spends;

    // Set the long term feerate estimate to the wallet's consolidate feerate
    coin_selection_params.m_long_term_feerate = wallet.m_consolidate_feerate;

    CAmount recipients_sum = 0;
    const OutputType change_type = wallet.TransactionChangeType(coin_control.m_change_type ? *coin_control.m_change_type : wallet.m_default_change_type, vecSend);
    ReserveDestination reservedest(&wallet, change_type);
    unsigned int outputs_to_subtract_fee_from = 0; // The number of outputs which we are subtracting the fee from
    for (const auto& recipient : vecSend) {
        recipients_sum += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount) {
            outputs_to_subtract_fee_from++;
            coin_selection_params.m_subtract_fee_outputs = true;
        }
    }

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
    int change_spend_size = CalculateMaximumSignedInputSize(change_prototype_txout, &wallet, /*coin_control=*/nullptr);
    // If the wallet doesn't know how to sign change output, assume p2sh-p2wpkh
    // as lower-bound to allow BnB to do it's thing
    if (change_spend_size == -1) {
        coin_selection_params.change_spend_size = DUMMY_NESTED_P2WPKH_INPUT_SIZE;
    } else {
        coin_selection_params.change_spend_size = (size_t)change_spend_size;
    }

    // Set discard feerate
    coin_selection_params.m_discard_feerate = GetDiscardRate(wallet);

    // Get the fee rate to use effective values in coin selection
    FeeCalculation feeCalc;
    coin_selection_params.m_effective_feerate = GetMinimumFeeRate(wallet, coin_control, &feeCalc);
    // Do not, ever, assume that it's fine to change the fee rate if the user has explicitly
    // provided one
    if (coin_control.m_feerate && coin_selection_params.m_effective_feerate > *coin_control.m_feerate) {
        return util::Error{strprintf(_("Fee rate (%s) is lower than the minimum fee rate setting (%s)"), coin_control.m_feerate->ToString(FeeEstimateMode::SAT_VB), coin_selection_params.m_effective_feerate.ToString(FeeEstimateMode::SAT_VB))};
    }
    if (feeCalc.reason == FeeReason::FALLBACK && !wallet.m_allow_fallback_fee) {
        // eventually allow a fallback fee
        return util::Error{_("Fee estimation failed. Fallbackfee is disabled. Wait a few blocks or enable -fallbackfee.")};
    }

    // Calculate the cost of change
    // Cost of change is the cost of creating the change output + cost of spending the change output in the future.
    // For creating the change output now, we use the effective feerate.
    // For spending the change output in the future, we use the discard feerate for now.
    // So cost of change = (change output size * effective feerate) + (size of spending change output * discard feerate)
    coin_selection_params.m_change_fee = coin_selection_params.m_effective_feerate.GetFee(coin_selection_params.change_output_size);
    coin_selection_params.m_cost_of_change = coin_selection_params.m_discard_feerate.GetFee(coin_selection_params.change_spend_size) + coin_selection_params.m_change_fee;

    coin_selection_params.m_min_change_target = GenerateChangeTarget(std::floor(recipients_sum / vecSend.size()), coin_selection_params.m_change_fee, rng_fast);

    // The smallest change amount should be:
    // 1. at least equal to dust threshold
    // 2. at least 1 sat greater than fees to spend it at m_discard_feerate
    const auto dust = GetDustThreshold(change_prototype_txout, coin_selection_params.m_discard_feerate);
    const auto change_spend_fee = coin_selection_params.m_discard_feerate.GetFee(coin_selection_params.change_spend_size);
    coin_selection_params.min_viable_change = std::max(change_spend_fee + 1, dust);

    // Static vsize overhead + outputs vsize. 4 nVersion, 4 nLocktime, 1 input count, 1 witness overhead (dummy, flag, stack size)
    coin_selection_params.tx_noinputs_size = 10 + GetSizeOfCompactSize(vecSend.size()); // bytes for output count

    // vouts to the payees
    for (const auto& recipient : vecSend)
    {
        // SYSCOIN
        CTxOut txout(recipient.nAmount, recipient.scriptPubKey);
        // add poda data to opreturn output
        if(!coin_control.m_nevmdata.empty() && recipient.scriptPubKey.IsUnspendable()) {
            txout.vchNEVMData = coin_control.m_nevmdata;
        }

        // Include the fee cost for outputs.
        // SYSCOIN need to account for CNEVMData data size for PoDA fees
        coin_selection_params.tx_noinputs_size += ::GetSerializeSize(txout, PROTOCOL_VERSION, SER_SIZE, txNew.nVersion);

        if (IsDust(txout, wallet.chain().relayDustFee())) {
            return util::Error{_("Transaction amount too small")};
        }
        txNew.vout.push_back(txout);
    }

    // Include the fees for things that aren't inputs, excluding the change output
    const CAmount not_input_fees = coin_selection_params.m_effective_feerate.GetFee(coin_selection_params.m_subtract_fee_outputs ? 0 : coin_selection_params.tx_noinputs_size);
    CAmount selection_target = recipients_sum + not_input_fees;

    // Fetch manually selected coins
    PreSelectedInputs preset_inputs;
    if (coin_control.HasSelected()) {
        auto res_fetch_inputs = FetchSelectedInputs(wallet, coin_control, coin_selection_params);
        if (!res_fetch_inputs) return util::Error{util::ErrorString(res_fetch_inputs)};
        preset_inputs = *res_fetch_inputs;
    }

    // Fetch wallet available coins if "other inputs" are
    // allowed (coins automatically selected by the wallet)
    CoinsResult available_coins;
    if (coin_control.m_allow_other_inputs) {
        available_coins = AvailableCoins(wallet, &coin_control, coin_selection_params.m_effective_feerate);
    }

    // Choose coins to use
    auto select_coins_res = SelectCoins(wallet, available_coins, preset_inputs, /*nTargetValue=*/selection_target, coin_control, coin_selection_params);
    if (!select_coins_res) {
        // 'SelectCoins' either returns a specific error message or, if empty, means a general "Insufficient funds".
        const bilingual_str& err = util::ErrorString(select_coins_res);
        return util::Error{err.empty() ?_("Insufficient funds") : err};
    }
    const SelectionResult& result = *select_coins_res;
    TRACE5(coin_selection, selected_coins, wallet.GetName().c_str(), GetAlgorithmName(result.GetAlgo()).c_str(), result.GetTarget(), result.GetWaste(), result.GetSelectedValue());

    const CAmount change_amount = result.GetChange(coin_selection_params.min_viable_change, coin_selection_params.m_change_fee);
    if (change_amount > 0) {
        CTxOut newTxOut(change_amount, scriptChange);
        if (nChangePosInOut == -1) {
            // Insert change txn at random position:
            nChangePosInOut = rng_fast.randrange(txNew.vout.size() + 1);
        } else if ((unsigned int)nChangePosInOut > txNew.vout.size()) {
            return util::Error{_("Transaction change output index out of range")};
        }
        txNew.vout.insert(txNew.vout.begin() + nChangePosInOut, newTxOut);
    } else {
        nChangePosInOut = -1;
    }

    // Shuffle selected coins and fill in final vin
    std::vector<std::shared_ptr<COutput>> selected_coins = result.GetShuffledInputVector();

    // The sequence number is set to non-maxint so that DiscourageFeeSniping
    // works.
    //
    // BIP125 defines opt-in RBF as any nSequence < maxint-1, so
    // we use the highest possible value in that range (maxint-2)
    // to avoid conflicting with other possible uses of nSequence,
    // and in the spirit of "smallest possible change from prior
    // behavior."
    const uint32_t nSequence{coin_control.m_signal_bip125_rbf.value_or(wallet.m_signal_rbf) ? MAX_BIP125_RBF_SEQUENCE : CTxIn::MAX_SEQUENCE_NONFINAL};
    for (const auto& coin : selected_coins) {
        txNew.vin.push_back(CTxIn(coin->outpoint, CScript(), nSequence));
    }
    DiscourageFeeSniping(txNew, rng_fast, wallet.chain(), wallet.GetLastBlockHash(), wallet.GetLastBlockHeight());

    // Calculate the transaction fee
    TxSize tx_sizes = CalculateMaximumSignedTxSize(CTransaction(txNew), &wallet, &coin_control);
    int nBytes = tx_sizes.vsize;
    if (nBytes == -1) {
        return util::Error{_("Missing solving data for estimating transaction size")};
    }
    CAmount fee_needed = coin_selection_params.m_effective_feerate.GetFee(nBytes);
    const CAmount output_value = CalculateOutputValue(txNew);
    Assume(recipients_sum + change_amount == output_value);
    CAmount current_fee = result.GetSelectedValue() - output_value;

    // Sanity check that the fee cannot be negative as that means we have more output value than input value
    if (current_fee < 0) {
        return util::Error{Untranslated(STR_INTERNAL_BUG("Fee paid < 0"))};
    }

    // If there is a change output and we overpay the fees then increase the change to match the fee needed
    if (nChangePosInOut != -1 && fee_needed < current_fee) {
        auto& change = txNew.vout.at(nChangePosInOut);
        change.nValue += current_fee - fee_needed;
        current_fee = result.GetSelectedValue() - CalculateOutputValue(txNew);
        if (fee_needed != current_fee) {
            return util::Error{Untranslated(STR_INTERNAL_BUG("Change adjustment: Fee needed != fee paid"))};
        }
    }

    // Reduce output values for subtractFeeFromAmount
    if (coin_selection_params.m_subtract_fee_outputs) {
        CAmount to_reduce = fee_needed - current_fee;
        int i = 0;
        bool fFirst = true;
        for (const auto& recipient : vecSend)
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
        current_fee = result.GetSelectedValue() - CalculateOutputValue(txNew);
        if (fee_needed != current_fee) {
            return util::Error{Untranslated(STR_INTERNAL_BUG("SFFO: Fee needed != fee paid"))};
        }
    }

    // fee_needed should now always be less than or equal to the current fees that we pay.
    // If it is not, it is a bug.
    if (fee_needed > current_fee) {
        return util::Error{Untranslated(STR_INTERNAL_BUG("Fee needed > fee paid"))};
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
    if ((sign && GetTransactionWeight(*tx) > MAX_STANDARD_TX_WEIGHT) ||
        (!sign && tx_sizes.weight > MAX_STANDARD_TX_WEIGHT))
    {
        return util::Error{_("Transaction too large")};
    }

    if (current_fee > wallet.m_default_max_tx_fee) {
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
              current_fee, nBytes, feeCalc.returnedTarget, feeCalc.desiredTarget, StringForFeeReason(feeCalc.reason), feeCalc.est.decay,
              feeCalc.est.pass.start, feeCalc.est.pass.end,
              (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) > 0.0 ? 100 * feeCalc.est.pass.withinTarget / (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) : 0.0,
              feeCalc.est.pass.withinTarget, feeCalc.est.pass.totalConfirmed, feeCalc.est.pass.inMempool, feeCalc.est.pass.leftMempool,
              feeCalc.est.fail.start, feeCalc.est.fail.end,
              (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) > 0.0 ? 100 * feeCalc.est.fail.withinTarget / (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) : 0.0,
              feeCalc.est.fail.withinTarget, feeCalc.est.fail.totalConfirmed, feeCalc.est.fail.inMempool, feeCalc.est.fail.leftMempool);
    return CreatedTransactionResult(tx, current_fee, nChangePosInOut, feeCalc);
}

util::Result<CreatedTransactionResult> CreateTransaction(
        CWallet& wallet,
        const std::vector<CRecipient>& vecSend,
        int change_pos,
        const CCoinControl& coin_control,
        bool sign)
{
    if (vecSend.empty()) {
        return util::Error{_("Transaction must have at least one recipient")};
    }

    if (std::any_of(vecSend.cbegin(), vecSend.cend(), [](const auto& recipient){ return recipient.nAmount < 0; })) {
        return util::Error{_("Transaction amounts must not be negative")};
    }

    LOCK(wallet.cs_wallet);

    auto res = CreateTransactionInternal(wallet, vecSend, change_pos, coin_control, sign);
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

        auto txr_grouped = CreateTransactionInternal(wallet, vecSend, change_pos, tmp_cc, sign);
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

    // Turn the txout set into a CRecipient vector.
    for (size_t idx = 0; idx < tx.vout.size(); idx++) {
        const CTxOut& txOut = tx.vout[idx];
        // SYSCOIN
        if(coinControl.m_nevmdata.empty() && !txOut.vchNEVMData.empty()) {
            coinControl.m_nevmdata = txOut.vchNEVMData; 
        }
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
        const auto& outPoint = txin.prevout;
        if (wallet.IsMine(outPoint)) {
            // The input was found in the wallet, so select as internal
            coinControl.Select(outPoint);
        } else if (coins[outPoint].out.IsNull()) {
            error = _("Unable to find UTXO for external input");
            return false;
        } else {
            // SYSCOIN The input was not in the wallet, but is in the UTXO set, so select as external
            coinControl.SelectExternal(outPoint, CTxOut(coins[outPoint].out.nValue,coins[outPoint].out.scriptPubKey));
        }
    }

    auto res = CreateTransaction(wallet, vecSend, nChangePosInOut, coinControl, false);
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
// SYSCOIN
util::Result<CreatedTransactionResult> GetBudgetSystemCollateralTX(CWallet& wallet, uint256 hash, CAmount amount, const COutPoint& outpoint)
{

    CScript scriptChange;
    scriptChange << OP_RETURN << ToByteVector(hash);

    std::vector< CRecipient > vecSend;
    vecSend.push_back((CRecipient){scriptChange, amount, false});

    CCoinControl coinControl;
    if (!outpoint.IsNull()) {
        coinControl.Select(outpoint);
    }
    constexpr int RANDOM_CHANGE_POSITION = -1;
    return CreateTransaction(wallet, vecSend, RANDOM_CHANGE_POSITION, coinControl, false);
}
} // namespace wallet
