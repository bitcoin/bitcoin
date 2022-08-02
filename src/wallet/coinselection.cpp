// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/coinselection.h>

#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <policy/feerate.h>
#include <util/check.h>
#include <util/system.h>
#include <util/moneystr.h>

#include <numeric>
#include <optional>

namespace wallet {
// Descending order comparator
struct {
    bool operator()(const OutputGroup& a, const OutputGroup& b) const
    {
        return a.GetSelectionAmount() > b.GetSelectionAmount();
    }
} descending;

/*
 * This is the Branch and Bound Coin Selection algorithm designed by Murch. It searches for an input
 * set that can pay for the spending target and does not exceed the spending target by more than the
 * cost of creating and spending a change output. The algorithm uses a depth-first search on a binary
 * tree. In the binary tree, each node corresponds to the inclusion or the omission of a UTXO. UTXOs
 * are sorted by their effective values and the tree is explored deterministically per the inclusion
 * branch first. At each node, the algorithm checks whether the selection is within the target range.
 * While the selection has not reached the target range, more UTXOs are included. When a selection's
 * value exceeds the target range, the complete subtree deriving from this selection can be omitted.
 * At that point, the last included UTXO is deselected and the corresponding omission branch explored
 * instead. The search ends after the complete tree has been searched or after a limited number of tries.
 *
 * The search continues to search for better solutions after one solution has been found. The best
 * solution is chosen by minimizing the waste metric. The waste metric is defined as the cost to
 * spend the current inputs at the given fee rate minus the long term expected cost to spend the
 * inputs, plus the amount by which the selection exceeds the spending target:
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
 * @param const std::vector<OutputGroup>& utxo_pool The set of UTXO groups that we are choosing from.
 *        These UTXO groups will be sorted in descending order by effective value and the OutputGroups'
 *        values are their effective values.
 * @param const CAmount& selection_target This is the value that we want to select. It is the lower
 *        bound of the range.
 * @param const CAmount& cost_of_change This is the cost of creating and spending a change output.
 *        This plus selection_target is the upper bound of the range.
 * @returns The result of this coin selection algorithm, or std::nullopt
 */

static const size_t TOTAL_TRIES = 100000;

std::optional<SelectionResult> SelectCoinsBnB(std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, const CAmount& cost_of_change)
{
    SelectionResult result(selection_target, SelectionAlgorithm::BNB);
    CAmount curr_value = 0;
    std::vector<size_t> curr_selection; // selected utxo indexes

    // Calculate curr_available_value
    CAmount curr_available_value = 0;
    for (const OutputGroup& utxo : utxo_pool) {
        // Assert that this utxo is not negative. It should never be negative,
        // effective value calculation should have removed it
        assert(utxo.GetSelectionAmount() > 0);
        curr_available_value += utxo.GetSelectionAmount();
    }
    if (curr_available_value < selection_target) {
        return std::nullopt;
    }

    // Sort the utxo_pool
    std::sort(utxo_pool.begin(), utxo_pool.end(), descending);

    CAmount curr_waste = 0;
    std::vector<size_t> best_selection;
    CAmount best_waste = MAX_MONEY;

    bool is_feerate_high = utxo_pool.at(0).fee > utxo_pool.at(0).long_term_fee;

    // Depth First search loop for choosing the UTXOs
    for (size_t curr_try = 0, utxo_pool_index = 0; curr_try < TOTAL_TRIES; ++curr_try, ++utxo_pool_index) {
        // Conditions for starting a backtrack
        bool backtrack = false;
        if (curr_value + curr_available_value < selection_target || // Cannot possibly reach target with the amount remaining in the curr_available_value.
            curr_value > selection_target + cost_of_change || // Selected value is out of range, go back and try other branch
            (curr_waste > best_waste && is_feerate_high)) { // Don't select things which we know will be more wasteful if the waste is increasing
            backtrack = true;
        } else if (curr_value >= selection_target) {       // Selected value is within range
            curr_waste += (curr_value - selection_target); // This is the excess value which is added to the waste for the below comparison
            // Adding another UTXO after this check could bring the waste down if the long term fee is higher than the current fee.
            // However we are not going to explore that because this optimization for the waste is only done when we have hit our target
            // value. Adding any more UTXOs will be just burning the UTXO; it will go entirely to fees. Thus we aren't going to
            // explore any more UTXOs to avoid burning money like that.
            if (curr_waste <= best_waste) {
                best_selection = curr_selection;
                best_waste = curr_waste;
            }
            curr_waste -= (curr_value - selection_target); // Remove the excess value as we will be selecting different coins now
            backtrack = true;
        }

        if (backtrack) { // Backtracking, moving backwards
            if (curr_selection.empty()) { // We have walked back to the first utxo and no branch is untraversed. All solutions searched
                break;
            }

            // Add omitted UTXOs back to lookahead before traversing the omission branch of last included UTXO.
            for (--utxo_pool_index; utxo_pool_index > curr_selection.back(); --utxo_pool_index) {
                curr_available_value += utxo_pool.at(utxo_pool_index).GetSelectionAmount();
            }

            // Output was included on previous iterations, try excluding now.
            assert(utxo_pool_index == curr_selection.back());
            OutputGroup& utxo = utxo_pool.at(utxo_pool_index);
            curr_value -= utxo.GetSelectionAmount();
            curr_waste -= utxo.fee - utxo.long_term_fee;
            curr_selection.pop_back();
        } else { // Moving forwards, continuing down this branch
            OutputGroup& utxo = utxo_pool.at(utxo_pool_index);

            // Remove this utxo from the curr_available_value utxo amount
            curr_available_value -= utxo.GetSelectionAmount();

            if (curr_selection.empty() ||
                // The previous index is included and therefore not relevant for exclusion shortcut
                (utxo_pool_index - 1) == curr_selection.back() ||
                // Avoid searching a branch if the previous UTXO has the same value and same waste and was excluded.
                // Since the ratio of fee to long term fee is the same, we only need to check if one of those values match in order to know that the waste is the same.
                utxo.GetSelectionAmount() != utxo_pool.at(utxo_pool_index - 1).GetSelectionAmount() ||
                utxo.fee != utxo_pool.at(utxo_pool_index - 1).fee)
            {
                // Inclusion branch first (Largest First Exploration)
                curr_selection.push_back(utxo_pool_index);
                curr_value += utxo.GetSelectionAmount();
                curr_waste += utxo.fee - utxo.long_term_fee;
            }
        }
    }

    // Check for solution
    if (best_selection.empty()) {
        return std::nullopt;
    }

    // Set output set
    for (const size_t& i : best_selection) {
        result.AddInput(utxo_pool.at(i));
    }
    result.ComputeAndSetWaste(cost_of_change, cost_of_change, CAmount{0});
    assert(best_waste == result.GetWaste());

    return result;
}

std::optional<SelectionResult> SelectCoinsSRD(const std::vector<OutputGroup>& utxo_pool, CAmount target_value, FastRandomContext& rng)
{
    SelectionResult result(target_value, SelectionAlgorithm::SRD);

    // Include change for SRD as we want to avoid making really small change if the selection just
    // barely meets the target. Just use the lower bound change target instead of the randomly
    // generated one, since SRD will result in a random change amount anyway; avoid making the
    // target needlessly large.
    target_value += CHANGE_LOWER;

    std::vector<size_t> indexes;
    indexes.resize(utxo_pool.size());
    std::iota(indexes.begin(), indexes.end(), 0);
    Shuffle(indexes.begin(), indexes.end(), rng);

    CAmount selected_eff_value = 0;
    for (const size_t i : indexes) {
        const OutputGroup& group = utxo_pool.at(i);
        Assume(group.GetSelectionAmount() > 0);
        selected_eff_value += group.GetSelectionAmount();
        result.AddInput(group);
        if (selected_eff_value >= target_value) {
            return result;
        }
    }
    return std::nullopt;
}

/** Find a subset of the OutputGroups that is at least as large as, but as close as possible to, the
 * target amount; solve subset sum.
 * param@[in]   groups          OutputGroups to choose from, sorted by value in descending order.
 * param@[in]   nTotalLower     Total (effective) value of the UTXOs in groups.
 * param@[in]   nTargetValue    Subset sum target, not including change.
 * param@[out]  vfBest          Boolean vector representing the subset chosen that is closest to
 *                              nTargetValue, with indices corresponding to groups. If the ith
 *                              entry is true, that means the ith group in groups was selected.
 * param@[out]  nBest           Total amount of subset chosen that is closest to nTargetValue.
 * param@[in]   iterations      Maximum number of tries.
 */
static void ApproximateBestSubset(FastRandomContext& insecure_rand, const std::vector<OutputGroup>& groups,
                                  const CAmount& nTotalLower, const CAmount& nTargetValue,
                                  std::vector<char>& vfBest, CAmount& nBest, int iterations = 1000)
{
    std::vector<char> vfIncluded;

    // Worst case "best" approximation is just all of the groups.
    vfBest.assign(groups.size(), true);
    nBest = nTotalLower;

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
                    nTotal += groups[i].GetSelectionAmount();
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue)
                    {
                        fReachedTarget = true;
                        // If the total is between nTargetValue and nBest, it's our new best
                        // approximation.
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= groups[i].GetSelectionAmount();
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }
}

std::optional<SelectionResult> KnapsackSolver(std::vector<OutputGroup>& groups, const CAmount& nTargetValue,
                                              CAmount change_target, FastRandomContext& rng)
{
    SelectionResult result(nTargetValue, SelectionAlgorithm::KNAPSACK);

    // List of values less than target
    std::optional<OutputGroup> lowest_larger;
    // Groups with selection amount smaller than the target and any change we might produce.
    // Don't include groups larger than this, because they will only cause us to overshoot.
    std::vector<OutputGroup> applicable_groups;
    CAmount nTotalLower = 0;

    Shuffle(groups.begin(), groups.end(), rng);

    for (const OutputGroup& group : groups) {
        if (group.GetSelectionAmount() == nTargetValue) {
            result.AddInput(group);
            return result;
        } else if (group.GetSelectionAmount() < nTargetValue + change_target) {
            applicable_groups.push_back(group);
            nTotalLower += group.GetSelectionAmount();
        } else if (!lowest_larger || group.GetSelectionAmount() < lowest_larger->GetSelectionAmount()) {
            lowest_larger = group;
        }
    }

    if (nTotalLower == nTargetValue) {
        for (const auto& group : applicable_groups) {
            result.AddInput(group);
        }
        return result;
    }

    if (nTotalLower < nTargetValue) {
        if (!lowest_larger) return std::nullopt;
        result.AddInput(*lowest_larger);
        return result;
    }

    // Solve subset sum by stochastic approximation
    std::sort(applicable_groups.begin(), applicable_groups.end(), descending);
    std::vector<char> vfBest;
    CAmount nBest;

    ApproximateBestSubset(rng, applicable_groups, nTotalLower, nTargetValue, vfBest, nBest);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + change_target) {
        ApproximateBestSubset(rng, applicable_groups, nTotalLower, nTargetValue + change_target, vfBest, nBest);
    }

    // If we have a bigger coin and (either the stochastic approximation didn't find a good solution,
    //                                   or the next bigger coin is closer), return the bigger coin
    if (lowest_larger &&
        ((nBest != nTargetValue && nBest < nTargetValue + change_target) || lowest_larger->GetSelectionAmount() <= nBest)) {
        result.AddInput(*lowest_larger);
    } else {
        for (unsigned int i = 0; i < applicable_groups.size(); i++) {
            if (vfBest[i]) {
                result.AddInput(applicable_groups[i]);
            }
        }

        if (LogAcceptCategory(BCLog::SELECTCOINS, BCLog::Level::Debug)) {
            std::string log_message{"Coin selection best subset: "};
            for (unsigned int i = 0; i < applicable_groups.size(); i++) {
                if (vfBest[i]) {
                    log_message += strprintf("%s ", FormatMoney(applicable_groups[i].m_value));
                }
            }
            LogPrint(BCLog::SELECTCOINS, "%stotal %s\n", log_message, FormatMoney(nBest));
        }
    }

    return result;
}

/******************************************************************************

 OutputGroup

 ******************************************************************************/

void OutputGroup::Insert(const std::shared_ptr<COutput>& output, size_t ancestors, size_t descendants) {
    m_outputs.push_back(output);
    auto& coin = *m_outputs.back();

    fee += coin.GetFee();

    coin.long_term_fee = coin.input_bytes < 0 ? 0 : m_long_term_feerate.GetFee(coin.input_bytes);
    long_term_fee += coin.long_term_fee;

    effective_value += coin.GetEffectiveValue();

    m_from_me &= coin.from_me;
    m_value += coin.txout.nValue;
    m_depth = std::min(m_depth, coin.depth);
    // ancestors here express the number of ancestors the new coin will end up having, which is
    // the sum, rather than the max; this will overestimate in the cases where multiple inputs
    // have common ancestors
    m_ancestors += ancestors;
    // descendants is the count as seen from the top ancestor, not the descendants as seen from the
    // coin itself; thus, this value is counted as the max, not the sum
    m_descendants = std::max(m_descendants, descendants);

    if (output->input_bytes > 0) {
        m_weight += output->input_bytes * WITNESS_SCALE_FACTOR;
    }
}

bool OutputGroup::EligibleForSpending(const CoinEligibilityFilter& eligibility_filter) const
{
    return m_depth >= (m_from_me ? eligibility_filter.conf_mine : eligibility_filter.conf_theirs)
        && m_ancestors <= eligibility_filter.max_ancestors
        && m_descendants <= eligibility_filter.max_descendants;
}

CAmount OutputGroup::GetSelectionAmount() const
{
    return m_subtract_fee_outputs ? m_value : effective_value;
}

void OutputGroupTypeMap::Push(const OutputGroup& group, OutputType type, bool insert_positive, bool insert_mixed)
{
    if (group.m_outputs.empty()) return;

    Groups& groups = groups_by_type[type];
    if (insert_positive && group.GetSelectionAmount() > 0) {
        groups.positive_group.emplace_back(group);
        all_groups.positive_group.emplace_back(group);
    }
    if (insert_mixed) {
        groups.mixed_group.emplace_back(group);
        all_groups.mixed_group.emplace_back(group);
    }
}

std::optional<Groups> OutputGroupTypeMap::Find(OutputType type)
{
    auto it_by_type = groups_by_type.find(type);
    if (it_by_type == groups_by_type.end()) return std::nullopt;
    return it_by_type->second;
}

CAmount GetSelectionWaste(const std::set<std::shared_ptr<COutput>>& inputs, CAmount change_cost, CAmount target, bool use_effective_value)
{
    // This function should not be called with empty inputs as that would mean the selection failed
    assert(!inputs.empty());

    // Always consider the cost of spending an input now vs in the future.
    CAmount waste = 0;
    CAmount selected_effective_value = 0;
    for (const auto& coin_ptr : inputs) {
        const COutput& coin = *coin_ptr;
        waste += coin.GetFee() - coin.long_term_fee;
        selected_effective_value += use_effective_value ? coin.GetEffectiveValue() : coin.txout.nValue;
    }

    if (change_cost) {
        // Consider the cost of making change and spending it in the future
        // If we aren't making change, the caller should've set change_cost to 0
        assert(change_cost > 0);
        waste += change_cost;
    } else {
        // When we are not making change (change_cost == 0), consider the excess we are throwing away to fees
        assert(selected_effective_value >= target);
        waste += selected_effective_value - target;
    }

    return waste;
}

CAmount GenerateChangeTarget(const CAmount payment_value, const CAmount change_fee, FastRandomContext& rng)
{
    if (payment_value <= CHANGE_LOWER / 2) {
        return change_fee + CHANGE_LOWER;
    } else {
        // random value between 50ksat and min (payment_value * 2, 1milsat)
        const auto upper_bound = std::min(payment_value * 2, CHANGE_UPPER);
        return change_fee + rng.randrange(upper_bound - CHANGE_LOWER) + CHANGE_LOWER;
    }
}

void SelectionResult::ComputeAndSetWaste(const CAmount min_viable_change, const CAmount change_cost, const CAmount change_fee)
{
    const CAmount change = GetChange(min_viable_change, change_fee);

    if (change > 0) {
        m_waste = GetSelectionWaste(m_selected_inputs, change_cost, m_target, m_use_effective);
    } else {
        m_waste = GetSelectionWaste(m_selected_inputs, 0, m_target, m_use_effective);
    }
}

CAmount SelectionResult::GetWaste() const
{
    return *Assert(m_waste);
}

CAmount SelectionResult::GetSelectedValue() const
{
    return std::accumulate(m_selected_inputs.cbegin(), m_selected_inputs.cend(), CAmount{0}, [](CAmount sum, const auto& coin) { return sum + coin->txout.nValue; });
}

CAmount SelectionResult::GetSelectedEffectiveValue() const
{
    return std::accumulate(m_selected_inputs.cbegin(), m_selected_inputs.cend(), CAmount{0}, [](CAmount sum, const auto& coin) { return sum + coin->GetEffectiveValue(); });
}

void SelectionResult::Clear()
{
    m_selected_inputs.clear();
    m_waste.reset();
    m_weight = 0;
}

void SelectionResult::AddInput(const OutputGroup& group)
{
    // As it can fail, combine inputs first
    InsertInputs(group.m_outputs);
    m_use_effective = !group.m_subtract_fee_outputs;

    m_weight += group.m_weight;
}

void SelectionResult::AddInputs(const std::set<std::shared_ptr<COutput>>& inputs, bool subtract_fee_outputs)
{
    // As it can fail, combine inputs first
    InsertInputs(inputs);
    m_use_effective = !subtract_fee_outputs;

    m_weight += std::accumulate(inputs.cbegin(), inputs.cend(), 0, [](int sum, const auto& coin) {
        return sum + std::max(coin->input_bytes, 0) * WITNESS_SCALE_FACTOR;
    });
}

void SelectionResult::Merge(const SelectionResult& other)
{
    // As it can fail, combine inputs first
    InsertInputs(other.m_selected_inputs);

    m_target += other.m_target;
    m_use_effective |= other.m_use_effective;
    if (m_algo == SelectionAlgorithm::MANUAL) {
        m_algo = other.m_algo;
    }

    m_weight += other.m_weight;
}

const std::set<std::shared_ptr<COutput>>& SelectionResult::GetInputSet() const
{
    return m_selected_inputs;
}

std::vector<std::shared_ptr<COutput>> SelectionResult::GetShuffledInputVector() const
{
    std::vector<std::shared_ptr<COutput>> coins(m_selected_inputs.begin(), m_selected_inputs.end());
    Shuffle(coins.begin(), coins.end(), FastRandomContext());
    return coins;
}

bool SelectionResult::operator<(SelectionResult other) const
{
    Assert(m_waste.has_value());
    Assert(other.m_waste.has_value());
    // As this operator is only used in std::min_element, we want the result that has more inputs when waste are equal.
    return *m_waste < *other.m_waste || (*m_waste == *other.m_waste && m_selected_inputs.size() > other.m_selected_inputs.size());
}

std::string COutput::ToString() const
{
    return strprintf("COutput(%s, %d, %d) [%s]", outpoint.hash.ToString(), outpoint.n, depth, FormatMoney(txout.nValue));
}

std::string GetAlgorithmName(const SelectionAlgorithm algo)
{
    switch (algo)
    {
    case SelectionAlgorithm::BNB: return "bnb";
    case SelectionAlgorithm::KNAPSACK: return "knapsack";
    case SelectionAlgorithm::SRD: return "srd";
    case SelectionAlgorithm::MANUAL: return "manual";
    // No default case to allow for compiler to warn
    }
    assert(false);
}

CAmount SelectionResult::GetChange(const CAmount min_viable_change, const CAmount change_fee) const
{
    // change = SUM(inputs) - SUM(outputs) - fees
    // 1) With SFFO we don't pay any fees
    // 2) Otherwise we pay all the fees:
    //  - input fees are covered by GetSelectedEffectiveValue()
    //  - non_input_fee is included in m_target
    //  - change_fee
    const CAmount change = m_use_effective
                           ? GetSelectedEffectiveValue() - m_target - change_fee
                           : GetSelectedValue() - m_target;

    if (change < min_viable_change) {
        return 0;
    }

    return change;
}

} // namespace wallet
