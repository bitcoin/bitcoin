// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_COINSELECTION_H
#define BITCOIN_WALLET_COINSELECTION_H

#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <outputtype.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <random.h>
#include <util/check.h>
#include <util/insert.h>
#include <util/result.h>

#include <optional>


namespace wallet {
//! lower bound for randomly-chosen target change amount
static constexpr CAmount CHANGE_LOWER{50000};
//! upper bound for randomly-chosen target change amount
static constexpr CAmount CHANGE_UPPER{1000000};

/** A UTXO under consideration for use in funding a new transaction. */
struct COutput {
private:
    /** The output's value minus fees required to spend it and bump its unconfirmed ancestors to the target feerate. */
    std::optional<CAmount> effective_value;

    /** The fee required to spend this output at the transaction's target feerate and to bump its unconfirmed ancestors to the target feerate. */
    std::optional<CAmount> fee;

public:
    /** The outpoint identifying this UTXO */
    COutPoint outpoint;

    /** The output itself */
    CTxOut txout;

    /**
     * Depth in block chain.
     * If > 0: the tx is on chain and has this many confirmations.
     * If = 0: the tx is waiting confirmation.
     * If < 0: a conflicting tx is on chain and has this many confirmations. */
    int depth;

    /** Pre-computed estimated size of this output as a fully-signed input in a transaction. Can be -1 if it could not be calculated */
    int input_bytes;

    /** Whether we have the private keys to spend this output */
    bool spendable;

    /** Whether we know how to spend this output, ignoring the lack of keys */
    bool solvable;

    /**
     * Whether this output is considered safe to spend. Unconfirmed transactions
     * from outside keys and unconfirmed replacement transactions are considered
     * unsafe and will not be used to fund new spending transactions.
     */
    bool safe;

    /** The time of the transaction containing this output as determined by CWalletTx::nTimeSmart */
    int64_t time;

    /** Whether the transaction containing this output is sent from the owning wallet */
    bool from_me;

    /** The fee required to spend this output at the consolidation feerate. */
    CAmount long_term_fee{0};

    /** The fee necessary to bump this UTXO's ancestor transactions to the target feerate */
    CAmount ancestor_bump_fees{0};

    COutput(const COutPoint& outpoint, const CTxOut& txout, int depth, int input_bytes, bool spendable, bool solvable, bool safe, int64_t time, bool from_me, const std::optional<CFeeRate> feerate = std::nullopt)
        : outpoint{outpoint},
          txout{txout},
          depth{depth},
          input_bytes{input_bytes},
          spendable{spendable},
          solvable{solvable},
          safe{safe},
          time{time},
          from_me{from_me}
    {
        if (feerate) {
            // base fee without considering potential unconfirmed ancestors
            fee = input_bytes < 0 ? 0 : feerate.value().GetFee(input_bytes);
            effective_value = txout.nValue - fee.value();
        }
    }

    COutput(const COutPoint& outpoint, const CTxOut& txout, int depth, int input_bytes, bool spendable, bool solvable, bool safe, int64_t time, bool from_me, const CAmount fees)
        : COutput(outpoint, txout, depth, input_bytes, spendable, solvable, safe, time, from_me)
    {
        // if input_bytes is unknown, then fees should be 0, if input_bytes is known, then the fees should be a positive integer or 0 (input_bytes known and fees = 0 only happens in the tests)
        assert((input_bytes < 0 && fees == 0) || (input_bytes > 0 && fees >= 0));
        fee = fees;
        effective_value = txout.nValue - fee.value();
    }

    std::string ToString() const;

    bool operator<(const COutput& rhs) const
    {
        return outpoint < rhs.outpoint;
    }

    void ApplyBumpFee(CAmount bump_fee)
    {
        assert(bump_fee >= 0);
        ancestor_bump_fees = bump_fee;
        assert(fee);
        *fee += bump_fee;
        // Note: assert(effective_value - bump_fee == nValue - fee.value());
        effective_value = txout.nValue - fee.value();
    }

    CAmount GetFee() const
    {
        assert(fee.has_value());
        return fee.value();
    }

    CAmount GetEffectiveValue() const
    {
        assert(effective_value.has_value());
        return effective_value.value();
    }

    bool HasEffectiveValue() const { return effective_value.has_value(); }
};

/** Parameters for one iteration of Coin Selection. */
struct CoinSelectionParams {
    /** Randomness to use in the context of coin selection. */
    FastRandomContext& rng_fast;
    /** Size of a change output in bytes, determined by the output type. */
    int change_output_size = 0;
    /** Size of the input to spend a change output in virtual bytes. */
    int change_spend_size = 0;
    /** Mininmum change to target in Knapsack solver and CoinGrinder:
     * select coins to cover the payment and at least this value of change. */
    CAmount m_min_change_target{0};
    /** Minimum amount for creating a change output.
     * If change budget is smaller than min_change then we forgo creation of change output.
     */
    CAmount min_viable_change{0};
    /** Cost of creating the change output. */
    CAmount m_change_fee{0};
    /** Cost of creating the change output + cost of spending the change output in the future. */
    CAmount m_cost_of_change{0};
    /** The targeted feerate of the transaction being built. */
    CFeeRate m_effective_feerate;
    /** The feerate estimate used to estimate an upper bound on what should be sufficient to spend
     * the change output sometime in the future. */
    CFeeRate m_long_term_feerate;
    /** If the cost to spend a change output at the discard feerate exceeds its value, drop it to fees. */
    CFeeRate m_discard_feerate;
    /** Size of the transaction before coin selection, consisting of the header and recipient
     * output(s), excluding the inputs and change output(s). */
    int tx_noinputs_size = 0;
    /** Indicate that we are subtracting the fee from outputs */
    bool m_subtract_fee_outputs = false;
    /** When true, always spend all (up to OUTPUT_GROUP_MAX_ENTRIES) or none of the outputs
     * associated with the same address. This helps reduce privacy leaks resulting from address
     * reuse. Dust outputs are not eligible to be added to output groups and thus not considered. */
    bool m_avoid_partial_spends = false;
    /**
     * When true, allow unsafe coins to be selected during Coin Selection. This may spend unconfirmed outputs:
     * 1) Received from other wallets, 2) replacing other txs, 3) that have been replaced.
     */
    bool m_include_unsafe_inputs = false;
    /** The maximum weight for this transaction. */
    std::optional<int> m_max_tx_weight{std::nullopt};

    CoinSelectionParams(FastRandomContext& rng_fast, int change_output_size, int change_spend_size,
                        CAmount min_change_target, CFeeRate effective_feerate,
                        CFeeRate long_term_feerate, CFeeRate discard_feerate, int tx_noinputs_size, bool avoid_partial,
                        std::optional<int> max_tx_weight = std::nullopt)
        : rng_fast{rng_fast},
          change_output_size(change_output_size),
          change_spend_size(change_spend_size),
          m_min_change_target(min_change_target),
          m_effective_feerate(effective_feerate),
          m_long_term_feerate(long_term_feerate),
          m_discard_feerate(discard_feerate),
          tx_noinputs_size(tx_noinputs_size),
          m_avoid_partial_spends(avoid_partial),
          m_max_tx_weight(max_tx_weight)
    {
    }
    CoinSelectionParams(FastRandomContext& rng_fast)
        : rng_fast{rng_fast} {}
};

/** Parameters for filtering which OutputGroups we may use in coin selection.
 * We start by being very selective and requiring multiple confirmations and
 * then get more permissive if we cannot fund the transaction. */
struct CoinEligibilityFilter
{
    /** Minimum number of confirmations for outputs that we sent to ourselves.
     * We may use unconfirmed UTXOs sent from ourselves, e.g. change outputs. */
    const int conf_mine;
    /** Minimum number of confirmations for outputs received from a different wallet. */
    const int conf_theirs;
    /** Maximum number of unconfirmed ancestors aggregated across all UTXOs in an OutputGroup. */
    const uint64_t max_ancestors;
    /** Maximum number of descendants that a single UTXO in the OutputGroup may have. */
    const uint64_t max_descendants;
    /** When avoid_reuse=true and there are full groups (OUTPUT_GROUP_MAX_ENTRIES), whether or not to use any partial groups.*/
    const bool m_include_partial_groups{false};

    CoinEligibilityFilter() = delete;
    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_ancestors) {}
    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors, uint64_t max_descendants) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_descendants) {}
    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors, uint64_t max_descendants, bool include_partial) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_descendants), m_include_partial_groups(include_partial) {}

    bool operator<(const CoinEligibilityFilter& other) const {
        return std::tie(conf_mine, conf_theirs, max_ancestors, max_descendants, m_include_partial_groups)
               < std::tie(other.conf_mine, other.conf_theirs, other.max_ancestors, other.max_descendants, other.m_include_partial_groups);
    }
};

/** A group of UTXOs paid to the same output script. */
struct OutputGroup
{
    /** The list of UTXOs contained in this output group. */
    std::vector<std::shared_ptr<COutput>> m_outputs;
    /** Whether the UTXOs were sent by the wallet to itself. This is relevant because we may want at
     * least a certain number of confirmations on UTXOs received from outside wallets while trusting
     * our own UTXOs more. */
    bool m_from_me{true};
    /** The total value of the UTXOs in sum. */
    CAmount m_value{0};
    /** The minimum number of confirmations the UTXOs in the group have. Unconfirmed is 0. */
    int m_depth{999};
    /** The aggregated count of unconfirmed ancestors of all UTXOs in this
     * group. Not deduplicated and may overestimate when ancestors are shared. */
    size_t m_ancestors{0};
    /** The maximum count of descendants of a single UTXO in this output group. */
    size_t m_descendants{0};
    /** The value of the UTXOs after deducting the cost of spending them at the effective feerate. */
    CAmount effective_value{0};
    /** The fee to spend these UTXOs at the effective feerate. */
    CAmount fee{0};
    /** The fee to spend these UTXOs at the long term feerate. */
    CAmount long_term_fee{0};
    /** The feerate for spending a created change output eventually (i.e. not urgently, and thus at
     * a lower feerate). Calculated using long term fee estimate. This is used to decide whether
     * it could be economical to create a change output. */
    CFeeRate m_long_term_feerate{0};
    /** Indicate that we are subtracting the fee from outputs.
     * When true, the value that is used for coin selection is the UTXO's real value rather than effective value */
    bool m_subtract_fee_outputs{false};
    /** Total weight of the UTXOs in this group. */
    int m_weight{0};

    OutputGroup() = default;
    OutputGroup(const CoinSelectionParams& params) :
        m_long_term_feerate(params.m_long_term_feerate),
        m_subtract_fee_outputs(params.m_subtract_fee_outputs)
    {}

    void Insert(const std::shared_ptr<COutput>& output, size_t ancestors, size_t descendants);
    bool EligibleForSpending(const CoinEligibilityFilter& eligibility_filter) const;
    CAmount GetSelectionAmount() const;
};

struct Groups {
    // Stores 'OutputGroup' containing only positive UTXOs (value > 0).
    std::vector<OutputGroup> positive_group;
    // Stores 'OutputGroup' which may contain both positive and negative UTXOs.
    std::vector<OutputGroup> mixed_group;
};

/** Stores several 'Groups' whose were mapped by output type. */
struct OutputGroupTypeMap
{
    // Maps output type to output groups.
    std::map<OutputType, Groups> groups_by_type;
    // All inserted groups, no type distinction.
    Groups all_groups;

    // Based on the insert flag; appends group to the 'mixed_group' and, if value > 0, to the 'positive_group'.
    // This affects both; the groups filtered by type and the overall groups container.
    void Push(const OutputGroup& group, OutputType type, bool insert_positive, bool insert_mixed);
    // Different output types count
    size_t TypesCount() { return groups_by_type.size(); }
};

typedef std::map<CoinEligibilityFilter, OutputGroupTypeMap> FilteredOutputGroups;

/** Choose a random change target for each transaction to make it harder to fingerprint the Core
 * wallet based on the change output values of transactions it creates.
 * Change target covers at least change fees and adds a random value on top of it.
 * The random value is between 50ksat and min(2 * payment_value, 1milsat)
 * When payment_value <= 25ksat, the value is just 50ksat.
 *
 * Making change amounts similar to the payment value may help disguise which output(s) are payments
 * are which ones are change. Using double the payment value may increase the number of inputs
 * needed (and thus be more expensive in fees), but breaks analysis techniques which assume the
 * coins selected are just sufficient to cover the payment amount ("unnecessary input" heuristic).
 *
 * @param[in]   payment_value   Average payment value of the transaction output(s).
 * @param[in]   change_fee      Fee for creating a change output.
 */
[[nodiscard]] CAmount GenerateChangeTarget(const CAmount payment_value, const CAmount change_fee, FastRandomContext& rng);

enum class SelectionAlgorithm : uint8_t
{
    BNB = 0,
    KNAPSACK = 1,
    SRD = 2,
    CG = 3,
    MANUAL = 4,
};

std::string GetAlgorithmName(const SelectionAlgorithm algo);

struct SelectionResult
{
private:
    /** Set of inputs selected by the algorithm to use in the transaction */
    std::set<std::shared_ptr<COutput>> m_selected_inputs;
    /** The target the algorithm selected for. Equal to the recipient amount plus non-input fees */
    CAmount m_target;
    /** The algorithm used to produce this result */
    SelectionAlgorithm m_algo;
    /** Whether the input values for calculations should be the effective value (true) or normal value (false) */
    bool m_use_effective{false};
    /** The computed waste */
    std::optional<CAmount> m_waste;
    /** False if algorithm was cut short by hitting limit of attempts and solution is non-optimal */
    bool m_algo_completed{true};
    /** The count of selections that were evaluated by this coin selection attempt */
    size_t m_selections_evaluated;
    /** Total weight of the selected inputs */
    int m_weight{0};
    /** How much individual inputs overestimated the bump fees for the shared ancestry */
    CAmount bump_fee_group_discount{0};

    template<typename T>
    void InsertInputs(const T& inputs)
    {
        // Store sum of combined input sets to check that the results have no shared UTXOs
        const size_t expected_count = m_selected_inputs.size() + inputs.size();
        util::insert(m_selected_inputs, inputs);
        if (m_selected_inputs.size() != expected_count) {
            throw std::runtime_error(STR_INTERNAL_BUG("Shared UTXOs among selection results"));
        }
    }

public:
    explicit SelectionResult(const CAmount target, SelectionAlgorithm algo)
        : m_target(target), m_algo(algo) {}

    SelectionResult() = delete;

    /** Get the sum of the input values */
    [[nodiscard]] CAmount GetSelectedValue() const;

    [[nodiscard]] CAmount GetSelectedEffectiveValue() const;

    [[nodiscard]] CAmount GetTotalBumpFees() const;

    void Clear();

    void AddInput(const OutputGroup& group);
    void AddInputs(const std::set<std::shared_ptr<COutput>>& inputs, bool subtract_fee_outputs);

    /** How much individual inputs overestimated the bump fees for shared ancestries */
    void SetBumpFeeDiscount(const CAmount discount);

    /** Calculates and stores the waste for this result given the cost of change
     * and the opportunity cost of spending these inputs now vs in the future.
     * If change exists, waste = change_cost + inputs * (effective_feerate - long_term_feerate) - bump_fee_group_discount
     * If no change, waste = excess + inputs * (effective_feerate - long_term_feerate) - bump_fee_group_discount
     * where excess = selected_effective_value - target
     * change_cost = effective_feerate * change_output_size + long_term_feerate * change_spend_size
     *
     * @param[in] min_viable_change The minimum amount necessary to make a change output economic
     * @param[in] change_cost       The cost of creating a change output and spending it in the future. Only
     *                              used if there is change, in which case it must be non-negative.
     * @param[in] change_fee        The fee for creating a change output
     */
    void RecalculateWaste(const CAmount min_viable_change, const CAmount change_cost, const CAmount change_fee);
    [[nodiscard]] CAmount GetWaste() const;

    /** Tracks that algorithm was able to exhaustively search the entire combination space before hitting limit of tries */
    void SetAlgoCompleted(bool algo_completed);

    /** Get m_algo_completed */
    bool GetAlgoCompleted() const;

    /** Record the number of selections that were evaluated */
    void SetSelectionsEvaluated(size_t attempts);

    /** Get selections_evaluated */
    size_t GetSelectionsEvaluated() const ;

    /**
     * Combines the @param[in] other selection result into 'this' selection result.
     *
     * Important note:
     * There must be no shared 'COutput' among the two selection results being combined.
     */
    void Merge(const SelectionResult& other);

    /** Get m_selected_inputs */
    const std::set<std::shared_ptr<COutput>>& GetInputSet() const;
    /** Get the vector of COutputs that will be used to fill in a CTransaction's vin */
    std::vector<std::shared_ptr<COutput>> GetShuffledInputVector() const;

    bool operator<(SelectionResult other) const;

    /** Get the amount for the change output after paying needed fees.
     *
     * The change amount is not 100% precise due to discrepancies in fee calculation.
     * The final change amount (if any) should be corrected after calculating the final tx fees.
     * When there is a discrepancy, most of the time the final change would be slightly bigger than estimated.
     *
     * Following are the possible factors of discrepancy:
     *  + non-input fees always include segwit flags
     *  + input fee estimation always include segwit stack size
     *  + input fees are rounded individually and not collectively, which leads to small rounding errors
     *  - input counter size is always assumed to be 1vbyte
     *
     * @param[in]  min_viable_change  Minimum amount for change output, if change would be less then we forgo change
     * @param[in]  change_fee         Fees to include change output in the tx
     * @returns Amount for change output, 0 when there is no change.
     *
     */
    CAmount GetChange(const CAmount min_viable_change, const CAmount change_fee) const;

    CAmount GetTarget() const { return m_target; }

    SelectionAlgorithm GetAlgo() const { return m_algo; }

    int GetWeight() const { return m_weight; }
};

util::Result<SelectionResult> SelectCoinsBnB(std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, const CAmount& cost_of_change,
                                             int max_selection_weight);

util::Result<SelectionResult> CoinGrinder(std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, CAmount change_target, int max_selection_weight);

/** Select coins by Single Random Draw. OutputGroups are selected randomly from the eligible
 * outputs until the target is satisfied
 *
 * @param[in]  utxo_pool    The positive effective value OutputGroups eligible for selection
 * @param[in]  target_value The target value to select for
 * @param[in]  rng The randomness source to shuffle coins
 * @param[in]  max_selection_weight The maximum allowed weight for a selection result to be valid
 * @returns If successful, a valid SelectionResult, otherwise, util::Error
 */
util::Result<SelectionResult> SelectCoinsSRD(const std::vector<OutputGroup>& utxo_pool, CAmount target_value, CAmount change_fee, FastRandomContext& rng,
                                             int max_selection_weight);

// Original coin selection algorithm as a fallback
util::Result<SelectionResult> KnapsackSolver(std::vector<OutputGroup>& groups, const CAmount& nTargetValue,
                                             CAmount change_target, FastRandomContext& rng, int max_selection_weight);
} // namespace wallet

#endif // BITCOIN_WALLET_COINSELECTION_H
