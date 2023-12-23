// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_COINCONTROL_H
#define BITCOIN_WALLET_COINCONTROL_H

#include <outputtype.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <primitives/transaction.h>
#include <script/keyorigin.h>
#include <script/signingprovider.h>

#include <algorithm>
#include <map>
#include <optional>
#include <set>

namespace wallet {
const int DEFAULT_MIN_DEPTH = 0;
const int DEFAULT_MAX_DEPTH = 9999999;

//! Default for -avoidpartialspends
static constexpr bool DEFAULT_AVOIDPARTIALSPENDS = false;

/** Coin Control Features. */
class CCoinControl
{
public:
    //! Custom change destination, if not set an address is generated
    CTxDestination destChange = CNoDestination();
    //! Override the default change type if set, ignored if destChange is set
    std::optional<OutputType> m_change_type;
    //! If false, only safe inputs will be used
    bool m_include_unsafe_inputs = false;
    //! If true, the selection process can add extra unselected inputs from the wallet
    //! while requires all selected inputs be used
    bool m_allow_other_inputs = true;
    //! Includes watch only addresses which are solvable
    bool fAllowWatchOnly = false;
    //! Override automatic min/max checks on fee, m_feerate must be set if true
    bool fOverrideFeeRate = false;
    //! Override the wallet's m_pay_tx_fee if set
    std::optional<CFeeRate> m_feerate;
    //! Override the default confirmation target if set
    std::optional<unsigned int> m_confirm_target;
    //! Override the wallet's m_signal_rbf if set
    std::optional<bool> m_signal_bip125_rbf;
    //! Avoid partial use of funds sent to a given address
    bool m_avoid_partial_spends = DEFAULT_AVOIDPARTIALSPENDS;
    //! Forbids inclusion of dirty (previously used) addresses
    bool m_avoid_address_reuse = false;
    //! Fee estimation mode to control arguments to estimateSmartFee
    FeeEstimateMode m_fee_mode = FeeEstimateMode::UNSET;
    //! Minimum chain depth value for coin availability
    int m_min_depth = DEFAULT_MIN_DEPTH;
    //! Maximum chain depth value for coin availability
    int m_max_depth = DEFAULT_MAX_DEPTH;
    //! SigningProvider that has pubkeys and scripts to do spend size estimation for external inputs
    FlatSigningProvider m_external_provider;

    CCoinControl();

    /**
     * Returns true if there are pre-selected inputs.
     */
    bool HasSelected() const;
    /**
     * Returns true if the given output is pre-selected.
     */
    bool IsSelected(const COutPoint& output) const;
    /**
     * Returns true if the given output is selected as an external input.
     */
    bool IsExternalSelected(const COutPoint& output) const;
    /**
     * Returns the external output for the given outpoint if it exists.
     */
    std::optional<CTxOut> GetExternalOutput(const COutPoint& outpoint) const;
    /**
     * Lock-in the given output for spending.
     * The output will be included in the transaction even if it's not the most optimal choice.
     */
    void Select(const COutPoint& output);
    /**
     * Lock-in the given output as an external input for spending because it is not in the wallet.
     * The output will be included in the transaction even if it's not the most optimal choice.
     */
    void SelectExternal(const COutPoint& outpoint, const CTxOut& txout);
    /**
     * Unselects the given output.
     */
    void UnSelect(const COutPoint& output);
    /**
     * Unselects all outputs.
     */
    void UnSelectAll();
    /**
     * List the selected inputs.
     */
    std::vector<COutPoint> ListSelected() const;
    /**
     * Set an input's weight.
     */
    void SetInputWeight(const COutPoint& outpoint, int64_t weight);
    /**
     * Returns true if the input weight is set.
     */
    bool HasInputWeight(const COutPoint& outpoint) const;
    /**
     * Returns the input weight.
     */
    int64_t GetInputWeight(const COutPoint& outpoint) const;

private:
    //! Selected inputs (inputs that will be used, regardless of whether they're optimal or not)
    std::set<COutPoint> m_selected_inputs;
    //! Map of external inputs to include in the transaction
    //! These are not in the wallet, so we need to track them separately
    std::map<COutPoint, CTxOut> m_external_txouts;
    //! Map of COutPoints to the maximum weight for that input
    std::map<COutPoint, int64_t> m_input_weights;
};
} // namespace wallet

#endif // BITCOIN_WALLET_COINCONTROL_H
