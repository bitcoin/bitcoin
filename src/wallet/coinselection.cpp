// Copyright (c) 2017-2019 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/coinselection.h>

#include <optional.h>
#include <policy/feerate.h>
#include <util/system.h>
#include <util/moneystr.h>

// Descending order comparator
struct {
    bool operator()(const OutputGroup& a, const OutputGroup& b) const
    {
        return a.effective_value > b.effective_value;
    }
} descending;

/*
 * This is the Branch and Bound Coin Selection algorithm designed by Murch. It searches for an input
 * set that can pay for the spending target and does not exceed the spending target by more than the
 * cost of creating and spending a change output. The algorithm uses a depth-first search on a binary
 * tree. In the binary tree, each node corresponds to the inclusion or the omission of a UTXO. UTXOs
 * are sorted by their effective values and the trees is explored deterministically per the inclusion
 * branch first. At each node, the algorithm checks whether the selection is within the target range.
 * While the selection has not reached the target range, more UTXOs are included. When a selection's
 * value exceeds the target range, the complete subtree deriving from this selection can be omitted.
 * At that point, the last included UTXO is deselected and the corresponding omission branch explored
 * instead. The search ends after the complete tree has been searched or after a limited number of tries.
 *
 * The search continues to search for better solutions after one solution has been found. The best
 * solution is chosen by minimizing the waste metric. The waste metric is defined as the cost to
 * spend the current inputs at the given fee rate minus the long term expected cost to spend the
 * inputs, plus the amount the selection exceeds the spending target:
 *
 * waste = selectionTotal - target + inputs × (currentFeeRate - longTermFeeRate)
 *
 * The algorithm uses two additional optimizations. A lookahead keeps track of the total value of
 * the unexplored UTXOs. A subtree is not explored if the lookahead indicates that the target range
 * cannot be reached. Further, it is unnecessary to test equivalent combinations. This allows us
 * to skip testing the inclusion of UTXOs that match the effective value and waste of an omitted
 * predecessor.
 *
 * The Branch and Bound algorithm is described in detail in Murch's Master Thesis:
 * https://murch.one/wp-content/uploads/2016/11/erhardt2016coinselection.pdf
 *
 * @param const std::vector<CInputCoin>& utxo_pool The set of UTXOs that we are choosing from.
 *        These UTXOs will be sorted in descending order by effective value and the CInputCoins'
 *        values are their effective values.
 * @param const CAmount& target_value This is the value that we want to select. It is the lower
 *        bound of the range.
 * @param const CAmount& cost_of_change This is the cost of creating and spending a change output.
 *        This plus target_value is the upper bound of the range.
 * @param std::set<CInputCoin>& out_set -> This is an output parameter for the set of CInputCoins
 *        that have been selected.
 * @param CAmount& value_ret -> This is an output parameter for the total value of the CInputCoins
 *        that were selected.
 * @param CAmount not_input_fees -> The fees that need to be paid for the outputs and fixed size
 *        overhead (version, locktime, marker and flag)
 */

static const size_t TOTAL_TRIES = 100000;

bool SelectCoinsBnB(std::vector<OutputGroup>& utxo_pool, const CAmount& target_value, const CAmount& cost_of_change, std::set<CInputCoin>& out_set, CAmount& value_ret, CAmount not_input_fees)
{
    out_set.clear();
    CAmount curr_value = 0;

    std::vector<bool> curr_selection; // select the utxo at this index
    curr_selection.reserve(utxo_pool.size());
    CAmount actual_target = not_input_fees + target_value;

    // Calculate curr_available_value
    CAmount curr_available_value = 0;
    for (const OutputGroup& utxo : utxo_pool) {
        // Assert that this utxo is not negative. It should never be negative, effective value calculation should have removed it
        assert(utxo.effective_value > 0);
        curr_available_value += utxo.effective_value;
    }
    if (curr_available_value < actual_target) {
        return false;
    }

    // Sort the utxo_pool
    std::sort(utxo_pool.begin(), utxo_pool.end(), descending);

    CAmount curr_waste = 0;
    std::vector<bool> best_selection;
    CAmount best_waste = MAX_MONEY;

    // Depth First search loop for choosing the UTXOs
    for (size_t i = 0; i < TOTAL_TRIES; ++i) {
        // Conditions for starting a backtrack
        bool backtrack = false;
        if (curr_value + curr_available_value < actual_target ||                // Cannot possibly reach target with the amount remaining in the curr_available_value.
            curr_value > actual_target + cost_of_change ||    // Selected value is out of range, go back and try other branch
            (curr_waste > best_waste && (utxo_pool.at(0).fee - utxo_pool.at(0).long_term_fee) > 0)) { // Don't select things which we know will be more wasteful if the waste is increasing
            backtrack = true;
        } else if (curr_value >= actual_target) {       // Selected value is within range
            curr_waste += (curr_value - actual_target); // This is the excess value which is added to the waste for the below comparison
            // Adding another UTXO after this check could bring the waste down if the long term fee is higher than the current fee.
            // However we are not going to explore that because this optimization for the waste is only done when we have hit our target
            // value. Adding any more UTXOs will be just burning the UTXO; it will go entirely to fees. Thus we aren't going to
            // explore any more UTXOs to avoid burning money like that.
            if (curr_waste <= best_waste) {
                best_selection = curr_selection;
                best_selection.resize(utxo_pool.size());
                best_waste = curr_waste;
                if (best_waste == 0) {
                    break;
                }
            }
            curr_waste -= (curr_value - actual_target); // Remove the excess value as we will be selecting different coins now
            backtrack = true;
        }

        // Backtracking, moving backwards
        if (backtrack) {
            // Walk backwards to find the last included UTXO that still needs to have its omission branch traversed.
            while (!curr_selection.empty() && !curr_selection.back()) {
                curr_selection.pop_back();
                curr_available_value += utxo_pool.at(curr_selection.size()).effective_value;
            }

            if (curr_selection.empty()) { // We have walked back to the first utxo and no branch is untraversed. All solutions searched
                break;
            }

            // Output was included on previous iterations, try excluding now.
            curr_selection.back() = false;
            OutputGroup& utxo = utxo_pool.at(curr_selection.size() - 1);
            curr_value -= utxo.effective_value;
            curr_waste -= utxo.fee - utxo.long_term_fee;
        } else { // Moving forwards, continuing down this branch
            OutputGroup& utxo = utxo_pool.at(curr_selection.size());

            // Remove this utxo from the curr_available_value utxo amount
            curr_available_value -= utxo.effective_value;

            // Avoid searching a branch if the previous UTXO has the same value and same waste and was excluded. Since the ratio of fee to
            // long term fee is the same, we only need to check if one of those values match in order to know that the waste is the same.
            if (!curr_selection.empty() && !curr_selection.back() &&
                utxo.effective_value == utxo_pool.at(curr_selection.size() - 1).effective_value &&
                utxo.fee == utxo_pool.at(curr_selection.size() - 1).fee) {
                curr_selection.push_back(false);
            } else {
                // Inclusion branch first (Largest First Exploration)
                curr_selection.push_back(true);
                curr_value += utxo.effective_value;
                curr_waste += utxo.fee - utxo.long_term_fee;
            }
        }
    }

    // Check for solution
    if (best_selection.empty()) {
        return false;
    }

    // Set output set
    value_ret = 0;
    for (size_t i = 0; i < best_selection.size(); ++i) {
        if (best_selection.at(i)) {
            util::insert(out_set, utxo_pool.at(i).m_outputs);
            value_ret += utxo_pool.at(i).m_value;
        }
    }

    return true;
}

static void ApproximateBestSubset(const std::vector<OutputGroup>& groups, const CAmount& nTotalLower, const CAmount& nTargetValue,
                                  std::vector<char>& vfBest, CAmount& nBest, int iterations = 1000)
{
    std::vector<char> vfIncluded;

    vfBest.assign(groups.size(), true);
    nBest = nTotalLower;

    FastRandomContext insecure_rand;

    for (int nRep = 0; nRep < iterations && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(groups.size(), false);
        CAmount nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (unsigned int i = 0; i < groups.size(); i++)
            {
                //The solver here uses a randomized algorithm,
                //the randomness serves no real security purpose but is just
                //needed to prevent degenerate behavior and it is important
                //that the rng is fast. We do not use a constant random sequence,
                //because there may be some privacy improvement by making
                //the selection random.
                if (nPass == 0 ? insecure_rand.randbool() : !vfIncluded[i])
                {
                    nTotal += groups[i].m_value;
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue)
                    {
                        fReachedTarget = true;
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= groups[i].m_value;
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }
}

bool KnapsackSolver(const CAmount& nTargetValue, std::vector<OutputGroup>& groups, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet)
{
    setCoinsRet.clear();
    nValueRet = 0;

    // List of values less than target
    Optional<OutputGroup> lowest_larger;
    std::vector<OutputGroup> applicable_groups;
    CAmount nTotalLower = 0;

    Shuffle(groups.begin(), groups.end(), FastRandomContext());

    for (const OutputGroup& group : groups) {
        if (group.m_value == nTargetValue) {
            util::insert(setCoinsRet, group.m_outputs);
            nValueRet += group.m_value;
            return true;
        } else if (group.m_value < nTargetValue + MIN_CHANGE) {
            applicable_groups.push_back(group);
            nTotalLower += group.m_value;
        } else if (!lowest_larger || group.m_value < lowest_larger->m_value) {
            lowest_larger = group;
        }
    }

    if (nTotalLower == nTargetValue) {
        for (const auto& group : applicable_groups) {
            util::insert(setCoinsRet, group.m_outputs);
            nValueRet += group.m_value;
        }
        return true;
    }

    if (nTotalLower < nTargetValue) {
        if (!lowest_larger) return false;
        util::insert(setCoinsRet, lowest_larger->m_outputs);
        nValueRet += lowest_larger->m_value;
        return true;
    }

    // Solve subset sum by stochastic approximation
    std::sort(applicable_groups.begin(), applicable_groups.end(), descending);
    std::vector<char> vfBest;
    CAmount nBest;

    ApproximateBestSubset(applicable_groups, nTotalLower, nTargetValue, vfBest, nBest);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + MIN_CHANGE) {
        ApproximateBestSubset(applicable_groups, nTotalLower, nTargetValue + MIN_CHANGE, vfBest, nBest);
    }

    // If we have a bigger coin and (either the stochastic approximation didn't find a good solution,
    //                                   or the next bigger coin is closer), return the bigger coin
    if (lowest_larger &&
        ((nBest != nTargetValue && nBest < nTargetValue + MIN_CHANGE) || lowest_larger->m_value <= nBest)) {
        util::insert(setCoinsRet, lowest_larger->m_outputs);
        nValueRet += lowest_larger->m_value;
    } else {
        for (unsigned int i = 0; i < applicable_groups.size(); i++) {
            if (vfBest[i]) {
                util::insert(setCoinsRet, applicable_groups[i].m_outputs);
                nValueRet += applicable_groups[i].m_value;
            }
        }

        if (LogAcceptCategory(BCLog::SELECTCOINS)) {
            LogPrint(BCLog::SELECTCOINS, "SelectCoins() best subset: "); /* Continued */
            for (unsigned int i = 0; i < applicable_groups.size(); i++) {
                if (vfBest[i]) {
                    LogPrint(BCLog::SELECTCOINS, "%s ", FormatMoney(applicable_groups[i].m_value)); /* Continued */
                }
            }
            LogPrint(BCLog::SELECTCOINS, "total %s\n", FormatMoney(nBest));
        }
    }

    return true;
}

/******************************************************************************

 OutputGroup

 ******************************************************************************/

void OutputGroup::Insert(const CInputCoin& output, int depth, bool from_me, size_t ancestors, size_t descendants) {
    m_outputs.push_back(output);
    m_from_me &= from_me;
    m_value += output.txout.nValue;
    m_depth = std::min(m_depth, depth);
    // ancestors here express the number of ancestors the new coin will end up having, which is
    // the sum, rather than the max; this will overestimate in the cases where multiple inputs
    // have common ancestors
    m_ancestors += ancestors;
    // descendants is the count as seen from the top ancestor, not the descendants as seen from the
    // coin itself; thus, this value is counted as the max, not the sum
    m_descendants = std::max(m_descendants, descendants);
    effective_value += output.effective_value;
    fee += output.m_fee;
    long_term_fee += output.m_long_term_fee;
}

std::vector<CInputCoin>::iterator OutputGroup::Discard(const CInputCoin& output) {
    auto it = m_outputs.begin();
    while (it != m_outputs.end() && it->outpoint != output.outpoint) ++it;
    if (it == m_outputs.end()) return it;
    m_value -= output.txout.nValue;
    effective_value -= output.effective_value;
    fee -= output.m_fee;
    long_term_fee -= output.m_long_term_fee;
    return m_outputs.erase(it);
}

bool OutputGroup::EligibleForSpending(const CoinEligibilityFilter& eligibility_filter) const
{
    return m_depth >= (m_from_me ? eligibility_filter.conf_mine : eligibility_filter.conf_theirs)
        && m_ancestors <= eligibility_filter.max_ancestors
        && m_descendants <= eligibility_filter.max_descendants;
}

void OutputGroup::SetFees(const CFeeRate effective_feerate, const CFeeRate long_term_feerate)
{
    fee = 0;
    long_term_fee = 0;
    effective_value = 0;
    for (CInputCoin& coin : m_outputs) {
        coin.m_fee = coin.m_input_bytes < 0 ? 0 : effective_feerate.GetFee(coin.m_input_bytes);
        fee += coin.m_fee;

        coin.m_long_term_fee = coin.m_input_bytes < 0 ? 0 : long_term_feerate.GetFee(coin.m_input_bytes);
        long_term_fee += coin.m_long_term_fee;

        coin.effective_value = coin.txout.nValue - coin.m_fee;
        effective_value += coin.effective_value;
    }
}

OutputGroup OutputGroup::GetPositiveOnlyGroup()
{
    OutputGroup group(*this);
    for (auto it = group.m_outputs.begin(); it != group.m_outputs.end(); ) {
        const CInputCoin& coin = *it;
        // Only include outputs that are positive effective value (i.e. not dust)
        if (coin.effective_value <= 0) {
            it = group.Discard(coin);
        } else {
            ++it;
        }
    }
    return group;
}
