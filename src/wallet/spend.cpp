// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/common.h>
#include <coinjoin/options.h>
#include <consensus/validation.h>
#include <evo/dmn_types.h>
#include <interfaces/chain.h>
#include <policy/policy.h>
#include <util/check.h>
#include <util/fees.h>
#include <util/moneystr.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/receive.h>
#include <wallet/spend.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

using interfaces::FoundBlock;

static constexpr size_t OUTPUT_GROUP_MAX_ENTRIES{100};

std::string COutput::ToString() const
{
    return strprintf("COutput(%s, %d, %d) [%s]", tx->GetHash().ToString(), i, nDepth, FormatMoney(tx->tx->vout[i].nValue));
}

int CalculateMaximumSignedInputSize(const CTxOut& txout, const CWallet* wallet, bool use_max_sig)
{
    CMutableTransaction txn;
    txn.vin.push_back(CTxIn(COutPoint()));
    if (!wallet->DummySignInput(txn.vin[0], txout, use_max_sig)) {
        return -1;
    }
    return ::GetSerializeSize(txn.vin[0], PROTOCOL_VERSION);
}

// txouts needs to be in the order of tx.vin
int64_t CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, const std::vector<CTxOut>& txouts, bool use_max_sig)
{
    CMutableTransaction txNew(tx);
    if (!wallet->DummySignTx(txNew, txouts, use_max_sig)) {
        return -1;
    }
    return ::GetSerializeSize(txNew, PROTOCOL_VERSION);
}

int64_t CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, bool use_max_sig)
{
    std::vector<CTxOut> txouts;
    for (const CTxIn& input : tx.vin) {
        const auto mi = wallet->mapWallet.find(input.prevout.hash);
        // Can not estimate size without knowing the input details
        if (mi == wallet->mapWallet.end()) {
            return -1;
        }
        assert(input.prevout.n < mi->second.tx->vout.size());
        txouts.emplace_back(mi->second.tx->vout[input.prevout.n]);
    }
    return CalculateMaximumSignedTxSize(tx, wallet, txouts, use_max_sig);
}

void CWallet::AvailableCoins(std::vector<COutput> &vCoins, const CCoinControl* coinControl, const CAmount& nMinimumAmount, const CAmount& nMaximumAmount, const CAmount &nMinimumSumAmount, const uint64_t nMaximumCount) const
{
    AssertLockHeld(cs_wallet);

    vCoins.clear();
    CoinType nCoinType = coinControl ? coinControl->nCoinType : CoinType::ALL_COINS;

    CAmount nTotal = 0;
    // Either the WALLET_FLAG_AVOID_REUSE flag is not set (in which case we always allow), or we default to avoiding, and only in the case where
    // a coin control object is provided, and has the avoid address reuse flag set to false, do we allow already used addresses
    bool allow_used_addresses = !IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE) || (coinControl && !coinControl->m_avoid_address_reuse);
    const int min_depth = {coinControl ? coinControl->m_min_depth : DEFAULT_MIN_DEPTH};
    const int max_depth = {coinControl ? coinControl->m_max_depth : DEFAULT_MAX_DEPTH};
    const bool only_safe = {coinControl ? !coinControl->m_include_unsafe_inputs : true};

    std::set<uint256> trusted_parents;
    for (const auto* pcoin : GetSpendableTXs()) {
        const auto& wtx{*pcoin};

        const uint256& wtxid = wtx.GetHash();

        if (!chain().checkFinalTx(*wtx.tx))
            continue;

        if (wtx.IsImmatureCoinBase())
            continue;

        int nDepth = wtx.GetDepthInMainChain();

        // We should not consider coins which aren't at least in our mempool
        // It's possible for these to be conflicted via ancestors which we may never be able to detect
        if (nDepth == 0 && !wtx.InMempool())
            continue;

        bool safeTx = IsTrusted(wtx, trusted_parents);

        if (only_safe && !safeTx) {
            continue;
        }

        if (nDepth < min_depth || nDepth > max_depth) {
            continue;
        }

        for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
            bool found = false;
            switch (nCoinType) {
                case CoinType::ONLY_FULLY_MIXED: {
                    found = CoinJoin::IsDenominatedAmount(wtx.tx->vout[i].nValue) &&
                            IsFullyMixed(COutPoint(wtxid, i));
                    break;
                }
                case CoinType::ONLY_READY_TO_MIX: {
                    found = CoinJoin::IsDenominatedAmount(wtx.tx->vout[i].nValue) &&
                            !IsFullyMixed(COutPoint(wtxid, i));
                    break;
                }
                case CoinType::ONLY_NONDENOMINATED: {
                    // NOTE: do not use collateral amounts
                    found = !CoinJoin::IsCollateralAmount(wtx.tx->vout[i].nValue) &&
                            !CoinJoin::IsDenominatedAmount(wtx.tx->vout[i].nValue);
                    break;
                }
                case CoinType::ONLY_MASTERNODE_COLLATERAL: {
                    found = dmn_types::IsCollateralAmount(wtx.tx->vout[i].nValue);
                    break;
                }
                case CoinType::ONLY_COINJOIN_COLLATERAL: {
                    found = CoinJoin::IsCollateralAmount(wtx.tx->vout[i].nValue);
                    break;
                }
                case CoinType::ALL_COINS: {
                    found = true;
                    break;
                }
            } // no default case, so the compiler can warn about missing cases
            if(!found) continue;

            // Only consider selected coins if add_inputs is false
            if (coinControl && !coinControl->m_add_inputs && !coinControl->IsSelected(COutPoint(wtxid, i))) {
                continue;
            }

            if (wtx.tx->vout[i].nValue < nMinimumAmount || wtx.tx->vout[i].nValue > nMaximumAmount)
                continue;

            if (coinControl && coinControl->HasSelected() && !coinControl->fAllowOtherInputs && !coinControl->IsSelected(COutPoint(wtxid, i)))
                continue;

            if (IsLockedCoin(wtxid, i) && nCoinType != CoinType::ONLY_MASTERNODE_COLLATERAL)
                continue;

            if (IsSpent(wtxid, i))
                continue;

            isminetype mine = IsMine(wtx.tx->vout[i]);

            if (mine == ISMINE_NO) {
                continue;
            }

            if (!allow_used_addresses && IsSpentKey(wtxid, i)) {
                continue;
            }

            std::unique_ptr<SigningProvider> provider = GetSolvingProvider(wtx.tx->vout[i].scriptPubKey);

            bool solvable = provider ? IsSolvable(*provider, wtx.tx->vout[i].scriptPubKey) : false;
            bool spendable = ((mine & ISMINE_SPENDABLE) != ISMINE_NO) || (((mine & ISMINE_WATCH_ONLY) != ISMINE_NO) && (coinControl && coinControl->fAllowWatchOnly && solvable));

            vCoins.push_back(COutput(&wtx, i, nDepth, spendable, solvable, safeTx, (coinControl && coinControl->fAllowWatchOnly)));

            // Checks the sum amount of all UTXO's.
            if (nMinimumSumAmount != MAX_MONEY) {
                nTotal += wtx.tx->vout[i].nValue;

                if (nTotal >= nMinimumSumAmount) {
                    return;
                }
            }

            // Checks the maximum number of UTXO's.
            if (nMaximumCount > 0 && vCoins.size() >= nMaximumCount) {
                return;
            }
        }
    }
}

CAmount CWallet::GetAvailableBalance(const CCoinControl* coinControl) const
{
    LOCK(cs_wallet);

    CAmount balance = 0;
    std::vector<COutput> vCoins;
    AvailableCoins(vCoins, coinControl);
    for (const COutput& out : vCoins) {
        if (out.fSpendable) {
            balance += out.tx->tx->vout[out.i].nValue;
        }
    }
    return balance;
}

const CTxOut& CWallet::FindNonChangeParentOutput(const CTransaction& tx, int output) const
{
    AssertLockHeld(cs_wallet);
    const CTransaction* ptx = &tx;
    int n = output;
    while (IsChange(ptx->vout[n]) && ptx->vin.size() > 0) {
        const COutPoint& prevout = ptx->vin[0].prevout;
        auto it = mapWallet.find(prevout.hash);
        if (it == mapWallet.end() || it->second.tx->vout.size() <= prevout.n ||
            !IsMine(it->second.tx->vout[prevout.n])) {
            break;
        }
        ptx = it->second.tx.get();
        n = prevout.n;
    }
    return ptx->vout[n];
}

std::map<CTxDestination, std::vector<COutput>> CWallet::ListCoins() const
{
    AssertLockHeld(cs_wallet);

    std::map<CTxDestination, std::vector<COutput>> result;
    std::vector<COutput> availableCoins;

    AvailableCoins(availableCoins);

    for (const COutput& coin : availableCoins) {
        CTxDestination address;
        if ((coin.fSpendable || (IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && coin.fSolvable)) &&
            ExtractDestination(FindNonChangeParentOutput(*coin.tx->tx, coin.i).scriptPubKey, address)) {
            result[address].emplace_back(std::move(coin));
        }
    }

    // Include watch-only for LegacyScriptPubKeyMan wallets without private keys
    const bool include_watch_only = GetLegacyScriptPubKeyMan() && IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    const isminetype is_mine_filter = include_watch_only ? ISMINE_WATCH_ONLY : ISMINE_SPENDABLE;
    for (const COutPoint& output : setLockedCoins) {
        auto it = mapWallet.find(output.hash);
        if (it != mapWallet.end()) {
            int depth = it->second.GetDepthInMainChain();
            if (depth >= 0 && output.n < it->second.tx->vout.size() &&
                IsMine(it->second.tx->vout[output.n]) == is_mine_filter
            ) {
                CTxDestination address;
                if (ExtractDestination(FindNonChangeParentOutput(*it->second.tx, output.n).scriptPubKey, address)) {
                    result[address].emplace_back(
                        &it->second, output.n, depth, true /* spendable */, true /* solvable */, false /* safe */);
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

std::vector<OutputGroup> CWallet::GroupOutputs(const std::vector<COutput>& outputs, bool separate_coins, const CFeeRate& effective_feerate, const CFeeRate& long_term_feerate, const CoinEligibilityFilter& filter, bool positive_only) const
{
    std::vector<OutputGroup> groups_out;

    if (separate_coins) {
        // Single coin means no grouping. Each COutput gets its own OutputGroup.
        for (const COutput& output : outputs) {
            // Skip outputs we cannot spend
            if (!output.fSpendable) continue;

            size_t ancestors, descendants;
            chain().getTransactionAncestry(output.tx->GetHash(), ancestors, descendants);
            CInputCoin input_coin = output.GetInputCoin();

            // Make an OutputGroup containing just this output
            OutputGroup group{effective_feerate, long_term_feerate};
            group.Insert(input_coin, output.nDepth, output.tx->IsFromMe(ISMINE_ALL), ancestors, descendants, positive_only);

            // Check the OutputGroup's eligibility. Only add the eligible ones.
            if (positive_only && group.effective_value <= 0) continue;
            bool isISLocked = isGroupISLocked(group, chain());
            if (group.m_outputs.size() > 0 && group.EligibleForSpending(filter, isISLocked)) groups_out.push_back(group);
        }
        return groups_out;
    }

    // We want to combine COutputs that have the same scriptPubKey into single OutputGroups
    // except when there are more than OUTPUT_GROUP_MAX_ENTRIES COutputs grouped in an OutputGroup.
    // To do this, we maintain a map where the key is the scriptPubKey and the value is a vector of OutputGroups.
    // For each COutput, we check if the scriptPubKey is in the map, and if it is, the COutput's CInputCoin is added
    // to the last OutputGroup in the vector for the scriptPubKey. When the last OutputGroup has
    // OUTPUT_GROUP_MAX_ENTRIES CInputCoins, a new OutputGroup is added to the end of the vector.
    std::map<CScript, std::vector<OutputGroup>> spk_to_groups_map;
    for (const auto& output : outputs) {
        // Skip outputs we cannot spend
        if (!output.fSpendable) continue;

        size_t ancestors, descendants;
        chain().getTransactionAncestry(output.tx->GetHash(), ancestors, descendants);
        CInputCoin input_coin = output.GetInputCoin();
        CScript spk = input_coin.txout.scriptPubKey;

        std::vector<OutputGroup>& groups = spk_to_groups_map[spk];

        if (groups.size() == 0) {
            // No OutputGroups for this scriptPubKey yet, add one
            groups.emplace_back(effective_feerate, long_term_feerate);
        }

        // Get the last OutputGroup in the vector so that we can add the CInputCoin to it
        // A pointer is used here so that group can be reassigned later if it is full.
        OutputGroup* group = &groups.back();

        // Check if this OutputGroup is full. We limit to OUTPUT_GROUP_MAX_ENTRIES when using -avoidpartialspends
        // to avoid surprising users with very high fees.
        if (group->m_outputs.size() >= OUTPUT_GROUP_MAX_ENTRIES) {
            // The last output group is full, add a new group to the vector and use that group for the insertion
            groups.emplace_back(effective_feerate, long_term_feerate);
            group = &groups.back();
        }

        // Add the input_coin to group
        group->Insert(input_coin, output.nDepth, output.tx->IsFromMe(ISMINE_ALL), ancestors, descendants, positive_only);
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
            if (positive_only && group.effective_value <= 0) continue;
            bool isISLocked = isGroupISLocked(group, chain());
            if (group.m_outputs.size() > 0 && group.EligibleForSpending(filter, isISLocked)) groups_out.push_back(group);
        }
    }

    return groups_out;
}

bool CWallet::SelectCoinsMinConf(const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, std::vector<COutput> coins,
                                 std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CoinSelectionParams& coin_selection_params, bool& bnb_used, CoinType nCoinType) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    if (coin_selection_params.use_bnb) {
        // Get the feerate for effective value.
        // When subtracting the fee from the outputs, we want the effective feerate to be 0
        CFeeRate effective_feerate{0};
        if (!coin_selection_params.m_subtract_fee_outputs) {
            effective_feerate = coin_selection_params.m_effective_feerate;
        }

        std::vector<OutputGroup> groups = GroupOutputs(coins, !coin_selection_params.m_avoid_partial_spends, effective_feerate, coin_selection_params.m_long_term_feerate, eligibility_filter, true /* positive_only */);

        // Calculate cost of change
        CAmount cost_of_change = coin_selection_params.m_discard_feerate.GetFee(coin_selection_params.change_spend_size) + coin_selection_params.m_effective_feerate.GetFee(coin_selection_params.change_output_size);

        // Calculate the fees for things that aren't inputs
        CAmount not_input_fees = coin_selection_params.m_effective_feerate.GetFee(coin_selection_params.tx_noinputs_size);
        bnb_used = true;
        return SelectCoinsBnB(groups, nTargetValue, cost_of_change, setCoinsRet, nValueRet, not_input_fees);
    } else {
        std::vector<OutputGroup> groups = GroupOutputs(coins, !coin_selection_params.m_avoid_partial_spends, CFeeRate(0), CFeeRate(0), eligibility_filter, false /* positive_only */);
        bnb_used = false;
        return KnapsackSolver(nTargetValue, groups, setCoinsRet, nValueRet, nCoinType == CoinType::ONLY_FULLY_MIXED, m_default_max_tx_fee);
    }
}

bool CWallet::SelectCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CCoinControl& coin_control, CoinSelectionParams& coin_selection_params, bool& bnb_used) const
{
    // Note: this function should never be used for "always free" tx types like dstx

    std::vector<COutput> vCoins(vAvailableCoins);
    CoinType nCoinType = coin_control.nCoinType;
    CAmount value_to_select = nTargetValue;

    // Default to bnb was not used. If we use it, we set it later
    bnb_used = false;

    // coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
    if (coin_control.HasSelected() && !coin_control.fAllowOtherInputs)
    {
        for (const COutput& out : vCoins)
        {
            if(!out.fSpendable)
                continue;

            nValueRet += out.tx->tx->vout[out.i].nValue;
            setCoinsRet.insert(out.GetInputCoin());

            if (!coin_control.fRequireAllInputs && nValueRet >= nTargetValue) {
                // stop when we added at least one input and enough inputs to have at least nTargetValue funds
                return true;
            }
        }

        return (nValueRet >= nTargetValue);
    }

    // calculate value from preset inputs and store them
    std::set<CInputCoin> setPresetCoins;
    CAmount nValueFromPresetInputs = 0;

    std::vector<COutPoint> vPresetInputs;
    coin_control.ListSelected(vPresetInputs);
    for (const COutPoint& outpoint : vPresetInputs)
    {
        std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(outpoint.hash);
        if (it != mapWallet.end())
        {
            const CWalletTx& wtx = it->second;
            // Clearly invalid input, fail
            if (wtx.tx->vout.size() <= outpoint.n) {
                return false;
            }
            if (nCoinType == CoinType::ONLY_FULLY_MIXED) {
                // Make sure to include mixed preset inputs only,
                // even if some non-mixed inputs were manually selected via CoinControl
                if (!IsFullyMixed(outpoint)) continue;
            }
            // Just to calculate the marginal byte size
            CInputCoin coin(wtx.tx, outpoint.n, wtx.GetSpendSize(outpoint.n, false));
            nValueFromPresetInputs += coin.txout.nValue;
            if (coin.m_input_bytes <= 0) {
                return false; // Not solvable, can't estimate size for fee
            }
            coin.effective_value = coin.txout.nValue - coin_selection_params.m_effective_feerate.GetFee(coin.m_input_bytes);
            if (coin_selection_params.use_bnb) {
                value_to_select -= coin.effective_value;
            } else {
                value_to_select -= coin.txout.nValue;
            }
            setPresetCoins.insert(coin);
        } else {
            return false; // TODO: Allow non-wallet inputs
        }
    }

    // remove preset inputs from vCoins so that Coin Selection doesn't pick them.
    for (std::vector<COutput>::iterator it = vCoins.begin(); it != vCoins.end() && coin_control.HasSelected();)
    {
        if (setPresetCoins.count(it->GetInputCoin()))
            it = vCoins.erase(it);
        else
            ++it;
    }

    unsigned int limit_ancestor_count = 0;
    unsigned int limit_descendant_count = 0;
    chain().getPackageLimits(limit_ancestor_count, limit_descendant_count);
    const size_t max_ancestors = (size_t)std::max<int64_t>(1, limit_ancestor_count);
    const size_t max_descendants = (size_t)std::max<int64_t>(1, limit_descendant_count);
    const bool fRejectLongChains = gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

    // form groups from remaining coins; note that preset coins will not
    // automatically have their associated (same address) coins included
    if (coin_control.m_avoid_partial_spends && vCoins.size() > OUTPUT_GROUP_MAX_ENTRIES) {
        // Cases where we have 101+ outputs all pointing to the same destination may result in
        // privacy leaks as they will potentially be deterministically sorted. We solve that by
        // explicitly shuffling the outputs before processing
        Shuffle(vCoins.begin(), vCoins.end(), FastRandomContext());
    }
    // Coin Selection attempts to select inputs from a pool of eligible UTXOs to fund the
    // transaction at a target feerate. If an attempt fails, more attempts may be made using a more
    // permissive CoinEligibilityFilter.
    const bool res = [&] {
        // Pre-selected inputs already cover the target amount.
        if (value_to_select <= 0) return true;

        // If possible, fund the transaction with confirmed UTXOs only. Prefer at least six
        // confirmations on outputs received from other wallets and only spend confirmed change.
        if (SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(1, 6, 0), vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) return true;
        if (SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(1, 1, 0), vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) return true;

        // Fall back to using zero confirmation change (but with as few ancestors in the mempool as
        // possible) if we cannot fund the transaction otherwise.
        if (m_spend_zero_conf_change) {
            if (SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, 2), vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) return true;
            if (SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, std::min((size_t)4, max_ancestors/3), std::min((size_t)4, max_descendants/3)),
                                   vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) {
                return true;
            }
            if (SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, max_ancestors/2, max_descendants/2),
                                   vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) {
                return true;
            }
            // If partial groups are allowed, relax the requirement of spending OutputGroups (groups
            // of UTXOs sent to the same address, which are obviously controlled by a single wallet)
            // in their entirety.
            if (SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, max_ancestors-1, max_descendants-1, true /* include_partial_groups */),
                                   vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) {
                return true;
            }
            // Try with unsafe inputs if they are allowed. This may spend unconfirmed outputs
            // received from other wallets.
            if (coin_control.m_include_unsafe_inputs
                && SelectCoinsMinConf(value_to_select,
                    CoinEligibilityFilter(0 /* conf_mine */, 0 /* conf_theirs */, max_ancestors-1, max_descendants-1, true /* include_partial_groups */),
                    vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) {
                return true;
            }
            // Try with unlimited ancestors/descendants. The transaction will still need to meet
            // mempool ancestor/descendant policy to be accepted to mempool and broadcasted, but
            // OutputGroups use heuristics that may overestimate ancestor/descendant counts.
            if (!fRejectLongChains && SelectCoinsMinConf(value_to_select,
                                      CoinEligibilityFilter(0, 1, std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(), true /* include_partial_groups */),
                                      vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) {
                return true;
            }
        }
        // Coin Selection failed.
        return false;
    }();

    // SelectCoinsMinConf clears setCoinsRet, so add the preset inputs from coin_control to the coinset
    util::insert(setCoinsRet, setPresetCoins);

    // add preset inputs to the total value selected
    nValueRet += nValueFromPresetInputs;

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
 * Return a height-based locktime for new transactions (uses the height of the
 * current chain tip unless we are not synced with the current chain
 */
static uint32_t GetLocktimeForNewTransaction(interfaces::Chain& chain, const uint256& block_hash, int block_height)
{
    uint32_t locktime;
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
        locktime = block_height;

        // Secondly occasionally randomly pick a nLockTime even further back, so
        // that transactions that are delayed after signing for whatever reason,
        // e.g. high-latency mix networks and some CoinJoin implementations, have
        // better privacy.
        if (GetRandInt(10) == 0)
            locktime = std::max(0, (int)locktime - GetRandInt(100));
    } else {
        // If our chain is lagging behind, we can't discourage fee sniping nor help
        // the privacy of high-latency transactions. To avoid leaking a potentially
        // unique "nLockTime fingerprint", set nLockTime to a constant.
        locktime = 0;
    }
    assert(locktime < LOCKTIME_THRESHOLD);
    return locktime;
}

bool CWallet::CreateTransactionInternal(
        const std::vector<CRecipient>& vecSend,
        CTransactionRef& tx,
        CAmount& nFeeRet,
        int& nChangePosInOut,
        bilingual_str& error,
        const CCoinControl& coin_control,
        FeeCalculation& fee_calc_out,
        bool sign,
        int nExtraPayloadSize)
{
    CAmount nValue = 0;
    ReserveDestination reservedest(this);
    int nChangePosRequest = nChangePosInOut;
    unsigned int nSubtractFeeFromAmount = 0;
    for (const auto& recipient : vecSend)
    {
        if (nValue < 0 || recipient.nAmount < 0)
        {
            error = _("Transaction amounts must not be negative");
            return false;
        }
        nValue += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount)
            nSubtractFeeFromAmount++;
    }
    if (vecSend.empty())
    {
        error = _("Transaction must have at least one recipient");
        return false;
    }

    CMutableTransaction txNew;
    FeeCalculation feeCalc;

    CoinSelectionParams coin_selection_params; // Parameters for coin selection, init with dummy
    coin_selection_params.m_discard_feerate = coin_control.m_discard_feerate ? *coin_control.m_discard_feerate : GetDiscardRate(*this);

    // Get the fee rate to use effective values in coin selection
    coin_selection_params.m_effective_feerate = GetMinimumFeeRate(*this, coin_control, &feeCalc);
    // Do not, ever, assume that it's fine to change the fee rate if the user has explicitly
    // provided one
    if (coin_control.m_feerate && coin_selection_params.m_effective_feerate > *coin_control.m_feerate) {
        error = strprintf(_("Fee rate (%s) is lower than the minimum fee rate setting (%s)"), coin_control.m_feerate->ToString(FeeEstimateMode::DUFF_B), coin_selection_params.m_effective_feerate.ToString(FeeEstimateMode::DUFF_B));
        return false;
    }
    if (feeCalc.reason == FeeReason::FALLBACK && !m_allow_fallback_fee) {
        // eventually allow a fallback fee
        error = _("Fee estimation failed. Fallbackfee is disabled. Wait a few blocks or enable -fallbackfee.");
        return false;
    }

    int nBytes{0};
    {
        std::vector<CInputCoin> vecCoins;
        LOCK(cs_wallet);
        txNew.nLockTime = GetLocktimeForNewTransaction(chain(), GetLastBlockHash(), GetLastBlockHeight());
        {
            CAmount nAmountAvailable{0};
            std::vector<COutput> vAvailableCoins;
            AvailableCoins(vAvailableCoins, &coin_control, 1, MAX_MONEY, MAX_MONEY, 0);
            coin_selection_params.use_bnb = false; // never use BnB

            for (auto out : vAvailableCoins) {
                if (out.fSpendable) {
                    nAmountAvailable += out.tx->tx->vout[out.i].nValue;
                }
            }
            coin_selection_params.m_avoid_partial_spends = coin_control.m_avoid_partial_spends;

            // Create change script that will be used if we need change
            // TODO: pass in scriptChange instead of reservedest so
            // change transaction isn't always pay-to-bitcoin-address
            CScript scriptChange;

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
                if (!reservedest.GetReservedDestination(dest, true)) {
                    error = _("Transaction needs a change address, but we can't generate it. Please call keypoolrefill first.");
                }
                scriptChange = GetScriptForDestination(dest);
                // A valid destination implies a change script (and
                // vice-versa). An empty change script will abort later, if the
                // change keypool ran out, but change is required.
                CHECK_NONFATAL(IsValidDestination(dest) != scriptChange.empty());
            }

            nFeeRet = 0;
            bool pick_new_inputs = true;
            CAmount nValueIn = 0;
            CAmount nAmountToSelectAdditional{0};
            // Start with nAmountToSelectAdditional=0 and loop until there is enough to cover the request + fees, try it 500 times.
            int nMaxTries = 500;
            while (--nMaxTries > 0)
            {
                nChangePosInOut = std::numeric_limits<int>::max();
                txNew.vin.clear();
                txNew.vout.clear();
                bool fFirst = true;

                CAmount nValueToSelect = nValue;
                if (nSubtractFeeFromAmount == 0) {
                    assert(nAmountToSelectAdditional >= 0);
                    nValueToSelect += nAmountToSelectAdditional;
                }
                // vouts to the payees
                for (const auto& recipient : vecSend)
                {
                    CTxOut txout(recipient.nAmount, recipient.scriptPubKey);

                    if (recipient.fSubtractFeeFromAmount)
                    {
                        assert(nSubtractFeeFromAmount != 0);
                        txout.nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

                        if (fFirst) // first receiver pays the remainder not divisible by output count
                        {
                            fFirst = false;
                            txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
                        }
                    }

                    if (IsDust(txout, chain().relayDustFee()))
                    {
                        if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
                        {
                            if (txout.nValue < 0)
                                error = _("The transaction amount is too small to pay the fee");
                            else
                                error = _("The transaction amount is too small to send after the fee has been deducted");
                        }
                        else
                            error = _("Transaction amount too small");
                        return false;
                    }
                    txNew.vout.push_back(txout);
                }

                // Choose coins to use
                bool bnb_used = false;
                if (pick_new_inputs) {
                    nValueIn = 0;
                    std::set<CInputCoin> setCoinsTmp;
                    if (!SelectCoins(vAvailableCoins, nValueToSelect, setCoinsTmp, nValueIn, coin_control, coin_selection_params, bnb_used)) {
                        if (coin_control.nCoinType == CoinType::ONLY_NONDENOMINATED) {
                            error = _("Unable to locate enough non-denominated funds for this transaction.");
                        } else if (coin_control.nCoinType == CoinType::ONLY_FULLY_MIXED) {
                            error = _("Unable to locate enough mixed funds for this transaction.");
                            error = error + Untranslated(" ") + strprintf(_("%s uses exact denominated amounts to send funds, you might simply need to mix some more coins."), gCoinJoinName);
                        } else if (nValueIn < nValueToSelect) {
                            error = _("Insufficient funds.");
                        }
                        return false;
                    }
                    vecCoins.assign(setCoinsTmp.begin(), setCoinsTmp.end());
                }

                // Fill vin
                //
                // Note how the sequence number is set to max()-1 so that the
                // nLockTime set above actually works.
                txNew.vin.clear();
                for (const auto& coin : vecCoins) {
                    txNew.vin.emplace_back(coin.outpoint, CScript(), CTxIn::SEQUENCE_FINAL - 1);
                }

                auto calculateFee = [&](CAmount& nFee) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet) -> bool {
                    AssertLockHeld(cs_wallet);
                    nBytes = CalculateMaximumSignedTxSize(CTransaction(txNew), this, coin_control.fAllowWatchOnly);
                    if (nBytes < 0) {
                        error = _("Signing transaction failed");
                        return false;
                    }

                    if (nExtraPayloadSize != 0) {
                        // account for extra payload in fee calculation
                        nBytes += GetSizeOfCompactSize(nExtraPayloadSize) + nExtraPayloadSize;
                    }

                    // Remove scriptSigs to eliminate the fee calculation dummy signatures
                    for (auto& txin : txNew.vin) {
                        txin.scriptSig = CScript();
                    }

                    nFee = coin_selection_params.m_effective_feerate.GetFee(nBytes);

                    return true;
                };

                if (!calculateFee(nFeeRet)) {
                    return false;
                }

                CTxOut newTxOut;
                const CAmount nAmountLeft = nValueIn - nValue;
                auto getChange = [&]() {
                    if (nSubtractFeeFromAmount > 0) {
                        return nAmountLeft;
                    } else {
                        return nAmountLeft - nFeeRet;
                    }
                };

                if (getChange() > 0)
                {
                    //over pay for denominated transactions
                    if (coin_control.nCoinType == CoinType::ONLY_FULLY_MIXED) {
                        nChangePosInOut = -1;
                        nFeeRet += getChange();
                    } else {
                        // Fill a vout to ourself with zero amount until we know the correct change
                        newTxOut = CTxOut(0, scriptChange);
                        txNew.vout.push_back(newTxOut);

                        // Calculate the fee with the change output added, store the
                        // current fee to reset it in case the remainder is dust and we
                        // don't need to fee with change output added.
                        CAmount nFeePrev = nFeeRet;
                        if (!calculateFee(nFeeRet)) {
                            return false;
                        }

                        // Remove the change output again, it will be added later again if required
                        txNew.vout.pop_back();

                        // Set the change amount properly
                        newTxOut.nValue = getChange();

                        // Never create dust outputs; if we would, just
                        // add the dust to the fee.
                        if (IsDust(newTxOut, coin_selection_params.m_discard_feerate))
                        {
                            nFeeRet = nFeePrev;
                            nChangePosInOut = -1;
                            nFeeRet += getChange();
                        }
                        else
                        {
                            if (nChangePosRequest == -1)
                            {
                                // Insert change txn at random position:
                                nChangePosInOut = GetRandInt(txNew.vout.size()+1);
                            }
                            else if ((unsigned int)nChangePosRequest > txNew.vout.size())
                            {
                                error = _("Transaction change output index out of range");
                                return false;
                            } else {
                                nChangePosInOut = nChangePosRequest;
                            }

                            std::vector<CTxOut>::iterator position = txNew.vout.begin()+nChangePosInOut;
                            txNew.vout.insert(position, newTxOut);
                        }
                    }
                } else {
                    nChangePosInOut = -1;
                }

                if (getChange() < 0) {
                    if (nSubtractFeeFromAmount == 0) {
                        // nValueIn is not enough to cover nValue + nFeeRet. Add the missing amount abs(nChange) to the fee
                        // and try to select other inputs in the next loop step to cover the full required amount.
                        nAmountToSelectAdditional += abs(getChange());
                    } else if (nAmountToSelectAdditional > 0 && nValueToSelect == nAmountAvailable) {
                        // We tried selecting more and failed. We have no extra funds left,
                        // so just add 1 duff to fail in the next loop step with a correct reason
                        nAmountToSelectAdditional += 1;
                    }
                    continue;
                }

                // If no specific change position was requested, apply BIP69
                if (nChangePosRequest == -1) {
                    std::sort(vecCoins.begin(), vecCoins.end(), CompareInputCoinBIP69());
                    std::sort(txNew.vin.begin(), txNew.vin.end(), CompareInputBIP69());
                    std::sort(txNew.vout.begin(), txNew.vout.end(), CompareOutputBIP69());

                    // If there was a change output added before, we must update its position now
                    if (nChangePosInOut != -1) {
                        int i = 0;
                        for (const CTxOut& txOut : txNew.vout)
                        {
                            if (txOut == newTxOut)
                            {
                                nChangePosInOut = i;
                                break;
                            }
                            i++;
                        }
                    }
                }

                if (nAmountLeft == nFeeRet) {
                    // We either added the change amount to nFeeRet because the change amount was considered
                    // to be dust or the input exactly matches output + fee.
                    // Either way, we used the total amount of the inputs we picked and the transaction is ready.
                    break;
                }

                // We have a change output and we don't need to subtruct fees, which means the transaction is ready.
                if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                    break;
                }

                // If subtracting fee from recipients, we now know what fee we
                // need to subtract, we have no reason to reselect inputs
                if (nSubtractFeeFromAmount > 0) {
                    // If we are in here the second time it means we already subtracted the fee from the
                    // output(s) and there weren't any issues while doing that. So the transaction is ready now
                    // and we can break.
                    if (!pick_new_inputs) {
                        break;
                    }
                    pick_new_inputs = false;
                }
            }

            if (nMaxTries == 0) {
                error = _("Exceeded max tries.");
                return false;
            }

            // Give up if change keypool ran out and change is required
            if (scriptChange.empty() && nChangePosInOut != -1) {
                return false;
            }
        }

        // Make sure change position was updated one way or another
        assert(nChangePosInOut != std::numeric_limits<int>::max());

        if (sign && !SignTransaction(txNew)) {
            error = _("Signing transaction failed");
            return false;
        }

        // Return the constructed transaction data.
        tx = MakeTransactionRef(std::move(txNew));

        // Limit size
        if ((sign && ::GetSerializeSize(*tx, PROTOCOL_VERSION) > MAX_STANDARD_TX_SIZE) ||
            (!sign && static_cast<size_t>(nBytes) > MAX_STANDARD_TX_SIZE)) {
            error = _("Transaction too large");
            return false;
        }
    }

    if (nFeeRet > m_default_max_tx_fee) {
        error = TransactionErrorString(TransactionError::MAX_FEE_EXCEEDED);
        return false;
    }

    if (gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        // Lastly, ensure this tx will pass the mempool's chain limits
        if (!chain().checkChainLimits(tx)) {
            error = _("Transaction has too long of a mempool chain");
            return false;
        }
    }

    // Before we return success, we assume any change key will be used to prevent
    // accidental re-use.
    reservedest.KeepDestination();
    fee_calc_out = feeCalc;

    WalletLogPrintf("Fee Calculation: Fee:%d Bytes:%u Tgt:%d (requested %d) Reason:\"%s\" Decay %.5f: Estimation: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out) Fail: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out)\n",
              nFeeRet, nBytes, feeCalc.returnedTarget, feeCalc.desiredTarget, StringForFeeReason(feeCalc.reason), feeCalc.est.decay,
              feeCalc.est.pass.start, feeCalc.est.pass.end,
              (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) > 0.0 ? 100 * feeCalc.est.pass.withinTarget / (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) : 0.0,
              feeCalc.est.pass.withinTarget, feeCalc.est.pass.totalConfirmed, feeCalc.est.pass.inMempool, feeCalc.est.pass.leftMempool,
              feeCalc.est.fail.start, feeCalc.est.fail.end,
              (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) > 0.0 ? 100 * feeCalc.est.fail.withinTarget / (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) : 0.0,
              feeCalc.est.fail.withinTarget, feeCalc.est.fail.totalConfirmed, feeCalc.est.fail.inMempool, feeCalc.est.fail.leftMempool);
    return true;
}

bool CWallet::CreateTransaction(
        const std::vector<CRecipient>& vecSend,
        CTransactionRef& tx,
        CAmount& nFeeRet,
        int& nChangePosInOut,
        bilingual_str& error,
        const CCoinControl& coin_control,
        FeeCalculation& fee_calc_out,
        bool sign,
        int nExtraPayloadSize)
{
    int nChangePosIn = nChangePosInOut;
    Assert(!tx); // tx is an out-param. TODO change the return type from bool to tx (or nullptr)
    bool res = CreateTransactionInternal(vecSend, tx, nFeeRet, nChangePosInOut, error, coin_control, fee_calc_out, sign, nExtraPayloadSize);
    // try with avoidpartialspends unless it's enabled already
    if (res && nFeeRet > 0 /* 0 means non-functional fee rate estimation */ && m_max_aps_fee > -1 && !coin_control.m_avoid_partial_spends) {
        CCoinControl tmp_cc = coin_control;
        tmp_cc.m_avoid_partial_spends = true;

        // Re-use the change destination from the first creation attempt to avoid skipping BIP44 indexes
        const int ungrouped_change_pos = nChangePosInOut;
        if (ungrouped_change_pos != -1) {
            ExtractDestination(tx->vout[ungrouped_change_pos].scriptPubKey, tmp_cc.destChange);
        }

        CAmount nFeeRet2;
        CTransactionRef tx2;
        int nChangePosInOut2 = nChangePosIn;
        bilingual_str error2; // fired and forgotten; if an error occurs, we discard the results
        if (CreateTransactionInternal(vecSend, tx2, nFeeRet2, nChangePosInOut2, error2, tmp_cc, fee_calc_out, sign, nExtraPayloadSize)) {
            // if fee of this alternative one is within the range of the max fee, we use this one
            const bool use_aps = nFeeRet2 <= nFeeRet + m_max_aps_fee;
            WalletLogPrintf("Fee non-grouped = %lld, grouped = %lld, using %s\n", nFeeRet, nFeeRet2, use_aps ? "grouped" : "non-grouped");
            if (use_aps) {
                tx = tx2;
                nFeeRet = nFeeRet2;
                nChangePosInOut = nChangePosInOut2;
            }
        }
    }
    return res;
}

bool CWallet::FundTransaction(CMutableTransaction& tx, CAmount& nFeeRet, int& nChangePosInOut, bilingual_str& error, bool lockUnspents, const std::set<int>& setSubtractFeeFromOutputs, CCoinControl coinControl)
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

    coinControl.fAllowOtherInputs = true;

    for (const CTxIn& txin : tx.vin) {
        coinControl.Select(txin.prevout);
    }

    // Acquire the locks to prevent races to the new locked unspents between the
    // CreateTransaction call and LockCoin calls (when lockUnspents is true).
    LOCK(cs_wallet);

    CTransactionRef tx_new;
    FeeCalculation fee_calc_out;
    if (!CreateTransaction(vecSend, tx_new, nFeeRet, nChangePosInOut, error, coinControl, fee_calc_out, false, tx.vExtraPayload.size())) {
        return false;
    }

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
            LockCoin(txin.prevout);
        }

    }

    return true;
}
