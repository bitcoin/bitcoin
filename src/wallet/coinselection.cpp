// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/coinselection.h>

#include <common/system.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <interfaces/chain.h>
#include <logging.h>
#include <policy/feerate.h>
#include <util/check.h>
#include <util/moneystr.h>

#include <numeric>
#include <optional>
#include <queue>

namespace wallet {
// Common selection error across the algorithms
static util::Result<SelectionResult> ErrorMaxWeightExceeded()
{
    return util::Error{_("The inputs size exceeds the maximum weight. "
                         "Please try sending a smaller amount or manually consolidating your wallet's UTXOs")};
}

// Sort by descending (effective) value prefer lower waste on tie
struct {
    bool operator()(const OutputGroup& a, const OutputGroup& b) const
    {
        if (a.GetSelectionAmount() == b.GetSelectionAmount()) {
            // Lower waste is better when effective_values are tied
            return (a.fee - a.long_term_fee) < (b.fee - b.long_term_fee);
        }
        return a.GetSelectionAmount() > b.GetSelectionAmount();
    }
} descending;

// Sort by descending (effective) value prefer lower weight on tie
struct {
    bool operator()(const OutputGroup& a, const OutputGroup& b) const
    {
        if (a.GetSelectionAmount() == b.GetSelectionAmount()) {
            // Sort lower weight to front on tied effective_value
            return a.m_weight < b.m_weight;
        }
        return a.GetSelectionAmount() > b.GetSelectionAmount();
    }
} descending_effval_weight;

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
 * @param int max_selection_weight The maximum allowed weight for a selection result to be valid.
 * @returns The result of this coin selection algorithm, or std::nullopt
 */

static const size_t TOTAL_TRIES = 100000;

util::Result<SelectionResult> SelectCoinsBnB(std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, const CAmount& cost_of_change,
                                             int max_selection_weight)
{
    SelectionResult result(selection_target, SelectionAlgorithm::BNB);
    CAmount curr_value = 0;
    std::vector<size_t> curr_selection; // selected utxo indexes
    int curr_selection_weight = 0; // sum of selected utxo weight

    // Calculate curr_available_value
    CAmount curr_available_value = 0;
    for (const OutputGroup& utxo : utxo_pool) {
        // Assert that this utxo is not negative. It should never be negative,
        // effective value calculation should have removed it
        assert(utxo.GetSelectionAmount() > 0);
        curr_available_value += utxo.GetSelectionAmount();
    }
    if (curr_available_value < selection_target) {
        return util::Error();
    }

    // Sort the utxo_pool
    std::sort(utxo_pool.begin(), utxo_pool.end(), descending);

    CAmount curr_waste = 0;
    std::vector<size_t> best_selection;
    CAmount best_waste = MAX_MONEY;

    bool is_feerate_high = utxo_pool.at(0).fee > utxo_pool.at(0).long_term_fee;
    bool max_tx_weight_exceeded = false;

    // Depth First search loop for choosing the UTXOs
    for (size_t curr_try = 0, utxo_pool_index = 0; curr_try < TOTAL_TRIES; ++curr_try, ++utxo_pool_index) {
        // Conditions for starting a backtrack
        bool backtrack = false;
        if (curr_value + curr_available_value < selection_target || // Cannot possibly reach target with the amount remaining in the curr_available_value.
            curr_value > selection_target + cost_of_change || // Selected value is out of range, go back and try other branch
            (curr_waste > best_waste && is_feerate_high)) { // Don't select things which we know will be more wasteful if the waste is increasing
            backtrack = true;
        } else if (curr_selection_weight > max_selection_weight) { // Selected UTXOs weight exceeds the maximum weight allowed, cannot find more solutions by adding more inputs
            max_tx_weight_exceeded = true; // at least one selection attempt exceeded the max weight
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
            curr_selection_weight -= utxo.m_weight;
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
                curr_selection_weight += utxo.m_weight;
            }
        }
    }

    // Check for solution
    if (best_selection.empty()) {
        return max_tx_weight_exceeded ? ErrorMaxWeightExceeded() : util::Error();
    }

    // Set output set
    for (const size_t& i : best_selection) {
        result.AddInput(utxo_pool.at(i));
    }
    result.RecalculateWaste(cost_of_change, cost_of_change, CAmount{0});
    assert(best_waste == result.GetWaste());

    return result;
}

/*
 * TL;DR: Coin Grinder is a DFS-based algorithm that deterministically searches for the minimum-weight input set to fund
 * the transaction. The algorithm is similar to the Branch and Bound algorithm, but will produce a transaction _with_ a
 * change output instead of a changeless transaction.
 *
 * Full description: CoinGrinder can be thought of as a graph walking algorithm. It explores a binary tree
 * representation of the powerset of the UTXO pool. Each node in the tree represents a candidate input set. The tree’s
 * root is the empty set. Each node in the tree has two children which are formed by either adding or skipping the next
 * UTXO ("inclusion/omission branch"). Each level in the tree after the root corresponds to a decision about one UTXO in
 * the UTXO pool.
 *
 * Example:
 * We represent UTXOs as _alias=[effective_value/weight]_ and indicate omitted UTXOs with an underscore. Given a UTXO
 * pool {A=[10/2], B=[7/1], C=[5/1], D=[4/2]} sorted by descending effective value, our search tree looks as follows:
 *
 *                                       _______________________ {} ________________________
 *                                      /                                                   \
 * A=[10/2]               __________ {A} _________                                __________ {_} _________
 *                       /                        \                              /                        \
 * B=[7/1]            {AB} _                      {A_} _                      {_B} _                      {__} _
 *                  /       \                   /       \                   /       \                   /       \
 * C=[5/1]     {ABC}         {AB_}         {A_C}         {A__}         {_BC}         {_B_}         {__C}         {___}
 *              / \           / \           / \           / \           / \           / \           / \           / \
 * D=[4/2] {ABCD} {ABC_} {AB_D} {AB__} {A_CD} {A_C_} {A__D} {A___} {_BCD} {_BC_} {_B_D} {_B__} {__CD} {__C_} {___D} {____}
 *
 *
 * CoinGrinder uses a depth-first search to walk this tree. It first tries inclusion branches, then omission branches. A
 * naive exploration of a tree with four UTXOs requires visiting all 31 nodes:
 *
 *     {} {A} {AB} {ABC} {ABCD} {ABC_} {AB_} {AB_D} {AB__} {A_} {A_C} {A_CD} {A_C_} {A__} {A__D} {A___} {_} {_B} {_BC}
 *     {_BCD} {_BC_} {_B_} {_B_D} {_B__} {__} {__C} {__CD} {__C} {___} {___D} {____}
 *
 * As powersets grow exponentially with the set size, walking the entire tree would quickly get computationally
 * infeasible with growing UTXO pools. Thanks to traversing the tree in a deterministic order, we can keep track of the
 * progress of the search solely on basis of the current selection (and the best selection so far). We visit as few
 * nodes as possible by recognizing and skipping any branches that can only contain solutions worse than the best
 * solution so far. This makes CoinGrinder a branch-and-bound algorithm
 * (https://en.wikipedia.org/wiki/Branch_and_bound).
 * CoinGrinder is searching for the input set with lowest weight that can fund a transaction, so for example we can only
 * ever find a _better_ candidate input set in a node that adds a UTXO, but never in a node that skips a UTXO. After
 * visiting {A} and exploring the inclusion branch {AB} and its descendants, the candidate input set in the omission
 * branch {A_} is equivalent to the parent {A} in effective value and weight. While CoinGrinder does need to visit the
 * descendants of the omission branch {A_}, it is unnecessary to evaluate the candidate input set in the omission branch
 * itself. By skipping evaluation of all nodes on an omission branch we reduce the visited nodes to 15:
 *
 *     {A} {AB} {ABC} {ABCD} {AB_D} {A_C} {A_CD} {A__D} {_B} {_BC} {_BCD} {_B_D} {__C} {__CD} {___D}
 *
 *                                       _______________________ {} ________________________
 *                                      /                                                   \
 * A=[10/2]               __________ {A} _________                                ___________\____________
 *                       /                        \                              /                        \
 * B=[7/1]            {AB} __                    __\_____                     {_B} __                    __\_____
 *                  /        \                  /        \                  /        \                  /        \
 * C=[5/1]     {ABC}          \            {A_C}          \            {_BC}          \            {__C}          \
 *              /             /             /             /             /             /             /             /
 * D=[4/2] {ABCD}        {AB_D}        {A_CD}        {A__D}        {_BCD}        {_B_D}        {__CD}        {___D}
 *
 *
 * We refer to the move from the inclusion branch {AB} via the omission branch {A_} to its inclusion-branch child {A_C}
 * as _shifting to the omission branch_ or just _SHIFT_. (The index of the ultimate element in the candidate input set
 * shifts right by one: {AB} ⇒ {A_C}.)
 * When we reach a leaf node in the last level of the tree, shifting to the omission branch is not possible. Instead we
 * go to the omission branch of the node’s last ancestor on an inclusion branch: from {ABCD}, we go to {AB_D}. From
 * {AB_D}, we go to {A_C}. We refer to this operation as a _CUT_. (The ultimate element in
 * the input set is deselected, and the penultimate element is shifted right by one: {AB_D} ⇒ {A_C}.)
 * If a candidate input set in a node has not selected sufficient funds to build the transaction, we continue directly
 * along the next inclusion branch. We call this operation _EXPLORE_. (We go from one inclusion branch to the next
 * inclusion branch: {_B} ⇒ {_BC}.)
 * Further, any prefix that already has selected sufficient effective value to fund the transaction cannot be improved
 * by adding more UTXOs. If for example the candidate input set in {AB} is a valid solution, all potential descendant
 * solutions {ABC}, {ABCD}, and {AB_D} must have a higher weight, thus instead of exploring the descendants of {AB}, we
 * can SHIFT from {AB} to {A_C}.
 *
 * Given the above UTXO set, using a target of 11, and following these initial observations, the basic implementation of
 * CoinGrinder visits the following 10 nodes:
 *
 *     Node   [eff_val/weight]  Evaluation
 *     ---------------------------------------------------------------
 *     {A}    [10/2]            Insufficient funds: EXPLORE
 *     {AB}   [17/3]            Solution: SHIFT to omission branch
 *     {A_C}  [15/3]            Better solution: SHIFT to omission branch
 *     {A__D} [14/4]            Worse solution, shift impossible due to leaf node: CUT to omission branch of {A__D},
 *                              i.e. SHIFT to omission branch of {A}
 *     {_B}   [7/1]             Insufficient funds: EXPLORE
 *     {_BC}  [12/2]            Better solution: SHIFT to omission branch
 *     {_B_D} [11/3]            Worse solution, shift impossible due to leaf node: CUT to omission branch of {_B_D},
 *                              i.e. SHIFT to omission branch of {_B}
 *     {__C}  [5/1]             Insufficient funds: EXPLORE
 *     {__CD} [9/3]             Insufficient funds, leaf node: CUT
 *     {___D} [4/2]             Insufficient funds, leaf node, cannot CUT since only one UTXO selected: done.
 *
 *                                       _______________________ {} ________________________
 *                                      /                                                   \
 * A=[10/2]               __________ {A} _________                                ___________\____________
 *                       /                        \                              /                        \
 * B=[7/1]            {AB}                       __\_____                     {_B} __                    __\_____
 *                                              /        \                  /        \                  /        \
 * C=[5/1]                                 {A_C}          \            {_BC}          \            {__C}          \
 *                                                        /                           /             /             /
 * D=[4/2]                                           {A__D}                      {_B_D}        {__CD}        {___D}
 *
 *
 * We implement this tree walk in the following algorithm:
 * 1. Add `next_utxo`
 * 2. Evaluate candidate input set
 * 3. Determine `next_utxo` by deciding whether to
 *    a) EXPLORE: Add next inclusion branch, e.g. {_B} ⇒ {_B} + `next_uxto`: C
 *    b) SHIFT: Replace last selected UTXO by next higher index, e.g. {A_C} ⇒ {A__} + `next_utxo`: D
 *    c) CUT: deselect last selected UTXO and shift to omission branch of penultimate UTXO, e.g. {AB_D} ⇒ {A_} + `next_utxo: C
 *
 * The implementation then adds further optimizations by discovering further situations in which either the inclusion
 * branch can be skipped, or both the inclusion and omission branch can be skipped after evaluating the candidate input
 * set in the node.
 *
 * @param std::vector<OutputGroup>& utxo_pool The UTXOs that we are choosing from. These UTXOs will be sorted in
 *        descending order by effective value, with lower weight preferred as a tie-breaker. (We can think of an output
 *        group with multiple as a heavier UTXO with the combined amount here.)
 * @param const CAmount& selection_target This is the minimum amount that we need for the transaction without considering change.
 * @param const CAmount& change_target The minimum budget for creating a change output, by which we increase the selection_target.
 * @param int max_selection_weight The maximum allowed weight for a selection result to be valid.
 * @returns The result of this coin selection algorithm, or std::nullopt
 */
util::Result<SelectionResult> CoinGrinder(std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, CAmount change_target, int max_selection_weight)
{
    std::sort(utxo_pool.begin(), utxo_pool.end(), descending_effval_weight);
    // The sum of UTXO amounts after this UTXO index, e.g. lookahead[5] = Σ(UTXO[6+].amount)
    std::vector<CAmount> lookahead(utxo_pool.size());
    // The minimum UTXO weight among the remaining UTXOs after this UTXO index, e.g. min_tail_weight[5] = min(UTXO[6+].weight)
    std::vector<int> min_tail_weight(utxo_pool.size());

    // Calculate lookahead values, min_tail_weights, and check that there are sufficient funds
    CAmount total_available = 0;
    int min_group_weight = std::numeric_limits<int>::max();
    for (size_t i = 0; i < utxo_pool.size(); ++i) {
        size_t index = utxo_pool.size() - 1 - i; // Loop over every element in reverse order
        lookahead[index] = total_available;
        min_tail_weight[index] = min_group_weight;
        // UTXOs with non-positive effective value must have been filtered
        Assume(utxo_pool[index].GetSelectionAmount() > 0);
        total_available += utxo_pool[index].GetSelectionAmount();
        min_group_weight = std::min(min_group_weight, utxo_pool[index].m_weight);
    }

    const CAmount total_target = selection_target + change_target;
    if (total_available < total_target) {
        // Insufficient funds
        return util::Error();
    }

    // The current selection and the best input set found so far, stored as the utxo_pool indices of the UTXOs forming them
    std::vector<size_t> curr_selection;
    std::vector<size_t> best_selection;

    // The currently selected effective amount, and the effective amount of the best selection so far
    CAmount curr_amount = 0;
    CAmount best_selection_amount = MAX_MONEY;

    // The weight of the currently selected input set, and the weight of the best selection
    int curr_weight = 0;
    int best_selection_weight = max_selection_weight; // Tie is fine, because we prefer lower selection amount

    // Whether the input sets generated during this search have exceeded the maximum transaction weight at any point
    bool max_tx_weight_exceeded = false;

    // Index of the next UTXO to consider in utxo_pool
    size_t next_utxo = 0;

    /*
     * You can think of the current selection as a vector of booleans that has decided inclusion or exclusion of all
     * UTXOs before `next_utxo`. When we consider the next UTXO, we extend this hypothetical boolean vector either with
     * a true value if the UTXO is included or a false value if it is omitted. The equivalent state is stored more
     * compactly as the list of indices of the included UTXOs and the `next_utxo` index.
     *
     * We can never find a new solution by deselecting a UTXO, because we then revisit a previously evaluated
     * selection. Therefore, we only need to check whether we found a new solution _after adding_ a new UTXO.
     *
     * Each iteration of CoinGrinder starts by selecting the `next_utxo` and evaluating the current selection. We
     * use three state transitions to progress from the current selection to the next promising selection:
     *
     * - EXPLORE inclusion branch: We do not have sufficient funds, yet. Add `next_utxo` to the current selection, then
     *                             nominate the direct successor of the just selected UTXO as our `next_utxo` for the
     *                             following iteration.
     *
     *                             Example:
     *                                 Current Selection: {0, 5, 7}
     *                                 Evaluation: EXPLORE, next_utxo: 8
     *                                 Next Selection: {0, 5, 7, 8}
     *
     * - SHIFT to omission branch: Adding more UTXOs to the current selection cannot produce a solution that is better
     *                             than the current best, e.g. the current selection weight exceeds the max weight or
     *                             the current selection amount is equal to or greater than the target.
     *                             We designate our `next_utxo` the one after the tail of our current selection, then
     *                             deselect the tail of our current selection.
     *
     *                             Example:
     *                                 Current Selection: {0, 5, 7}
     *                                 Evaluation: SHIFT, next_utxo: 8, omit last selected: {0, 5}
     *                                 Next Selection: {0, 5, 8}
     *
     * - CUT entire subtree:       We have exhausted the inclusion branch for the penultimately selected UTXO, both the
     *                             inclusion and the omission branch of the current prefix are barren. E.g. we have
     *                             reached the end of the UTXO pool, so neither further EXPLORING nor SHIFTING can find
     *                             any solutions. We designate our `next_utxo` the one after our penultimate selected,
     *                             then deselect both the last and penultimate selected.
     *
     *                             Example:
     *                                 Current Selection: {0, 5, 7}
     *                                 Evaluation: CUT, next_utxo: 6, omit two last selected: {0}
     *                                 Next Selection: {0, 6}
     */
    auto deselect_last = [&]() {
        OutputGroup& utxo = utxo_pool[curr_selection.back()];
        curr_amount -= utxo.GetSelectionAmount();
        curr_weight -= utxo.m_weight;
        curr_selection.pop_back();
    };

    SelectionResult result(selection_target, SelectionAlgorithm::CG);
    bool is_done = false;
    size_t curr_try = 0;
    while (!is_done) {
        bool should_shift{false}, should_cut{false};
        // Select `next_utxo`
        OutputGroup& utxo = utxo_pool[next_utxo];
        curr_amount += utxo.GetSelectionAmount();
        curr_weight += utxo.m_weight;
        curr_selection.push_back(next_utxo);
        ++next_utxo;
        ++curr_try;

        // EVALUATE current selection: check for solutions and see whether we can CUT or SHIFT before EXPLORING further
        auto curr_tail = curr_selection.back();
        if (curr_amount + lookahead[curr_tail] < total_target) {
            // Insufficient funds with lookahead: CUT
            should_cut = true;
        } else if (curr_weight > best_selection_weight) {
            // best_selection_weight is initialized to max_selection_weight
            if (curr_weight > max_selection_weight) max_tx_weight_exceeded = true;
            // Worse weight than best solution. More UTXOs only increase weight:
            // CUT if last selected group had minimal weight, else SHIFT
            if (utxo_pool[curr_tail].m_weight <= min_tail_weight[curr_tail]) {
                should_cut = true;
            } else {
                should_shift  = true;
            }
        } else if (curr_amount >= total_target) {
            // Success, adding more weight cannot be better: SHIFT
            should_shift  = true;
            if (curr_weight < best_selection_weight || (curr_weight == best_selection_weight && curr_amount < best_selection_amount)) {
                // New lowest weight, or same weight with fewer funds tied up
                best_selection = curr_selection;
                best_selection_weight = curr_weight;
                best_selection_amount = curr_amount;
            }
        } else if (!best_selection.empty() && curr_weight + int64_t{min_tail_weight[curr_tail]} * ((total_target - curr_amount + utxo_pool[curr_tail].GetSelectionAmount() - 1) / utxo_pool[curr_tail].GetSelectionAmount()) > best_selection_weight) {
            // Compare minimal tail weight and last selected amount with the amount missing to gauge whether a better weight is still possible.
            if (utxo_pool[curr_tail].m_weight <= min_tail_weight[curr_tail]) {
                should_cut = true;
            } else {
                should_shift = true;
            }
        }

        if (curr_try >= TOTAL_TRIES) {
            // Solution is not guaranteed to be optimal if `curr_try` hit TOTAL_TRIES
            result.SetAlgoCompleted(false);
            break;
        }

        if (next_utxo == utxo_pool.size()) {
            // Last added UTXO was end of UTXO pool, nothing left to add on inclusion or omission branch: CUT
            should_cut = true;
        }

        if (should_cut) {
            // Neither adding to the current selection nor exploring the omission branch of the last selected UTXO can
            // find any solutions. Redirect to exploring the Omission branch of the penultimate selected UTXO (i.e.
            // set `next_utxo` to one after the penultimate selected, then deselect the last two selected UTXOs)
            should_cut = false;
            deselect_last();
            should_shift  = true;
        }

        while (should_shift) {
            // Set `next_utxo` to one after last selected, then deselect last selected UTXO
            if (curr_selection.empty()) {
                // Exhausted search space before running into attempt limit
                is_done = true;
                result.SetAlgoCompleted(true);
                break;
            }
            next_utxo = curr_selection.back() + 1;
            deselect_last();
            should_shift  = false;

            // After SHIFTing to an omission branch, the `next_utxo` might have the same effective value as the UTXO we
            // just omitted. Since lower weight is our tiebreaker on UTXOs with equal effective value for sorting, if it
            // ties on the effective value, it _must_ have the same weight (i.e. be a "clone" of the prior UTXO) or a
            // higher weight. If so, selecting `next_utxo` would produce an equivalent or worse selection as one we
            // previously evaluated. In that case, increment `next_utxo` until we find a UTXO with a differing amount.
            while (utxo_pool[next_utxo - 1].GetSelectionAmount() == utxo_pool[next_utxo].GetSelectionAmount()) {
                if (next_utxo >= utxo_pool.size() - 1) {
                    // Reached end of UTXO pool skipping clones: SHIFT instead
                    should_shift = true;
                    break;
                }
                // Skip clone: previous UTXO is equivalent and unselected
                ++next_utxo;
            }
        }
    }

    result.SetSelectionsEvaluated(curr_try);

    if (best_selection.empty()) {
        return max_tx_weight_exceeded ? ErrorMaxWeightExceeded() : util::Error();
    }

    for (const size_t& i : best_selection) {
        result.AddInput(utxo_pool[i]);
    }

    return result;
}

class MinOutputGroupComparator
{
public:
    int operator() (const OutputGroup& group1, const OutputGroup& group2) const
    {
        return group1.GetSelectionAmount() > group2.GetSelectionAmount();
    }
};

util::Result<SelectionResult> SelectCoinsSRD(const std::vector<OutputGroup>& utxo_pool, CAmount target_value, CAmount change_fee, FastRandomContext& rng,
                                             int max_selection_weight)
{
    SelectionResult result(target_value, SelectionAlgorithm::SRD);
    std::priority_queue<OutputGroup, std::vector<OutputGroup>, MinOutputGroupComparator> heap;

    // Include change for SRD as we want to avoid making really small change if the selection just
    // barely meets the target. Just use the lower bound change target instead of the randomly
    // generated one, since SRD will result in a random change amount anyway; avoid making the
    // target needlessly large.
    target_value += CHANGE_LOWER + change_fee;

    std::vector<size_t> indexes;
    indexes.resize(utxo_pool.size());
    std::iota(indexes.begin(), indexes.end(), 0);
    std::shuffle(indexes.begin(), indexes.end(), rng);

    CAmount selected_eff_value = 0;
    int weight = 0;
    bool max_tx_weight_exceeded = false;
    for (const size_t i : indexes) {
        const OutputGroup& group = utxo_pool.at(i);
        Assume(group.GetSelectionAmount() > 0);

        // Add group to selection
        heap.push(group);
        selected_eff_value += group.GetSelectionAmount();
        weight += group.m_weight;

        // If the selection weight exceeds the maximum allowed size, remove the least valuable inputs until we
        // are below max weight.
        if (weight > max_selection_weight) {
            max_tx_weight_exceeded = true; // mark it in case we don't find any useful result.
            do {
                const OutputGroup& to_remove_group = heap.top();
                selected_eff_value -= to_remove_group.GetSelectionAmount();
                weight -= to_remove_group.m_weight;
                heap.pop();
            } while (!heap.empty() && weight > max_selection_weight);
        }

        // Now check if we are above the target
        if (selected_eff_value >= target_value) {
            // Result found, add it.
            while (!heap.empty()) {
                result.AddInput(heap.top());
                heap.pop();
            }
            return result;
        }
    }
    return max_tx_weight_exceeded ? ErrorMaxWeightExceeded() : util::Error();
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
 * paramp[in]   max_selection_weight  The maximum allowed weight for a selection result to be valid.
 * param@[in]   iterations      Maximum number of tries.
 */
static void ApproximateBestSubset(FastRandomContext& insecure_rand, const std::vector<OutputGroup>& groups,
                                  const CAmount& nTotalLower, const CAmount& nTargetValue,
                                  std::vector<char>& vfBest, CAmount& nBest, int max_selection_weight, int iterations = 1000)
{
    std::vector<char> vfIncluded;

    // Worst case "best" approximation is just all of the groups.
    vfBest.assign(groups.size(), true);
    nBest = nTotalLower;

    for (int nRep = 0; nRep < iterations && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(groups.size(), false);
        CAmount nTotal = 0;
        int selected_coins_weight{0};
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
                    selected_coins_weight += groups[i].m_weight;
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue && selected_coins_weight <= max_selection_weight) {
                        fReachedTarget = true;
                        // If the total is between nTargetValue and nBest, it's our new best
                        // approximation.
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= groups[i].GetSelectionAmount();
                        selected_coins_weight -= groups[i].m_weight;
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }
}

util::Result<SelectionResult> KnapsackSolver(std::vector<OutputGroup>& groups, const CAmount& nTargetValue,
                                             CAmount change_target, FastRandomContext& rng, int max_selection_weight)
{
    SelectionResult result(nTargetValue, SelectionAlgorithm::KNAPSACK);

    bool max_weight_exceeded{false};
    // List of values less than target
    std::optional<OutputGroup> lowest_larger;
    // Groups with selection amount smaller than the target and any change we might produce.
    // Don't include groups larger than this, because they will only cause us to overshoot.
    std::vector<OutputGroup> applicable_groups;
    CAmount nTotalLower = 0;

    std::shuffle(groups.begin(), groups.end(), rng);

    for (const OutputGroup& group : groups) {
        if (group.m_weight > max_selection_weight) {
            max_weight_exceeded = true;
            continue;
        }
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
        if (result.GetWeight() <= max_selection_weight) return result;
        else max_weight_exceeded = true;

        // Try something else
        result.Clear();
    }

    if (nTotalLower < nTargetValue) {
        if (!lowest_larger) {
            if (max_weight_exceeded) return ErrorMaxWeightExceeded();
            return util::Error();
        }
        result.AddInput(*lowest_larger);
        return result;
    }

    // Solve subset sum by stochastic approximation
    std::sort(applicable_groups.begin(), applicable_groups.end(), descending);
    std::vector<char> vfBest;
    CAmount nBest;

    ApproximateBestSubset(rng, applicable_groups, nTotalLower, nTargetValue, vfBest, nBest, max_selection_weight);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + change_target) {
        ApproximateBestSubset(rng, applicable_groups, nTotalLower, nTargetValue + change_target, vfBest, nBest, max_selection_weight);
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

        // If the result exceeds the maximum allowed size, return closest UTXO above the target
        if (result.GetWeight() > max_selection_weight) {
            // No coin above target, nothing to do.
            if (!lowest_larger) return ErrorMaxWeightExceeded();

            // Return closest UTXO above target
            result.Clear();
            result.AddInput(*lowest_larger);
        }

        if (LogAcceptCategory(BCLog::SELECTCOINS, BCLog::Level::Debug)) {
            std::string log_message{"Coin selection best subset: "};
            for (unsigned int i = 0; i < applicable_groups.size(); i++) {
                if (vfBest[i]) {
                    log_message += strprintf("%s ", FormatMoney(applicable_groups[i].m_value));
                }
            }
            LogDebug(BCLog::SELECTCOINS, "%stotal %s\n", log_message, FormatMoney(nBest));
        }
    }
    Assume(result.GetWeight() <= max_selection_weight);
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

void SelectionResult::SetBumpFeeDiscount(const CAmount discount)
{
    // Overlapping ancestry can only lower the fees, not increase them
    assert (discount >= 0);
    bump_fee_group_discount = discount;
}

void SelectionResult::RecalculateWaste(const CAmount min_viable_change, const CAmount change_cost, const CAmount change_fee)
{
    // This function should not be called with empty inputs as that would mean the selection failed
    assert(!m_selected_inputs.empty());

    // Always consider the cost of spending an input now vs in the future.
    CAmount waste = 0;
    for (const auto& coin_ptr : m_selected_inputs) {
        const COutput& coin = *coin_ptr;
        waste += coin.GetFee() - coin.long_term_fee;
    }
    // Bump fee of whole selection may diverge from sum of individual bump fees
    waste -= bump_fee_group_discount;

    if (GetChange(min_viable_change, change_fee)) {
        // if we have a minimum viable amount after deducting fees, account for
        // cost of creating and spending change
        waste += change_cost;
    } else {
        // When we are not making change (GetChange(…) == 0), consider the excess we are throwing away to fees
        CAmount selected_effective_value = m_use_effective ? GetSelectedEffectiveValue() : GetSelectedValue();
        assert(selected_effective_value >= m_target);
        waste += selected_effective_value - m_target;
    }

    m_waste = waste;
}

void SelectionResult::SetAlgoCompleted(bool algo_completed)
{
    m_algo_completed = algo_completed;
}

bool SelectionResult::GetAlgoCompleted() const
{
    return m_algo_completed;
}

void SelectionResult::SetSelectionsEvaluated(size_t attempts)
{
    m_selections_evaluated = attempts;
}

size_t SelectionResult::GetSelectionsEvaluated() const
{
    return m_selections_evaluated;
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
    return std::accumulate(m_selected_inputs.cbegin(), m_selected_inputs.cend(), CAmount{0}, [](CAmount sum, const auto& coin) { return sum + coin->GetEffectiveValue(); }) + bump_fee_group_discount;
}

CAmount SelectionResult::GetTotalBumpFees() const
{
    return std::accumulate(m_selected_inputs.cbegin(), m_selected_inputs.cend(), CAmount{0}, [](CAmount sum, const auto& coin) { return sum + coin->ancestor_bump_fees; }) - bump_fee_group_discount;
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
    std::shuffle(coins.begin(), coins.end(), FastRandomContext());
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
    case SelectionAlgorithm::CG: return "cg";
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
