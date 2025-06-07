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

class PreselectedInput
{
private:
    //! The previous output being spent by this input
    std::optional<CTxOut> m_txout;
    //! The input weight for spending this input
    std::optional<int64_t> m_weight;
    //! The sequence number for this input
    std::optional<uint32_t> m_sequence;
    //! The scriptSig for this input
    std::optional<CScript> m_script_sig;
    //! The scriptWitness for this input
    std::optional<CScriptWitness> m_script_witness;
    //! The position in the inputs vector for this input
    std::optional<unsigned int> m_pos;

public:
    /**
     * Set the previous output for this input.
     * Only necessary if the input is expected to be an external input.
     */
    void SetTxOut(const CTxOut& txout);
    /** Retrieve the previous output for this input. */
    CTxOut GetTxOut() const;
    /** Return whether the previous output is set for this input. */
    bool HasTxOut() const;

    /** Set the weight for this input. */
    void SetInputWeight(int64_t weight);
    /** Retrieve the input weight for this input. */
    std::optional<int64_t> GetInputWeight() const;

    /** Set the sequence for this input. */
    void SetSequence(uint32_t sequence);
    /** Retrieve the sequence for this input. */
    std::optional<uint32_t> GetSequence() const;

    /** Set the scriptSig for this input. */
    void SetScriptSig(const CScript& script);
    /** Set the scriptWitness for this input. */
    void SetScriptWitness(const CScriptWitness& script_wit);
    /** Return whether either the scriptSig or scriptWitness are set for this input. */
    bool HasScripts() const;
    /** Retrieve both the scriptSig and the scriptWitness. */
    std::pair<std::optional<CScript>, std::optional<CScriptWitness>> GetScripts() const;

    /** Store the position of this input. */
    void SetPosition(unsigned int pos);
    /** Retrieve the position of this input. */
    std::optional<unsigned int> GetPosition() const;
};

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
    //! Locktime
    std::optional<uint32_t> m_locktime;
    //! Version
    std::optional<uint32_t> m_version;
    //! Caps weight of resulting tx
    std::optional<int> m_max_tx_weight{std::nullopt};

    CCoinControl();

    /**
     * Returns true if there are pre-selected inputs.
     */
    bool HasSelected() const;
    /**
     * Returns true if the given output is pre-selected.
     */
    bool IsSelected(const COutPoint& outpoint) const;
    /**
     * Returns true if the given output is selected as an external input.
     */
    bool IsExternalSelected(const COutPoint& outpoint) const;
    /**
     * Returns the external output for the given outpoint if it exists.
     */
    std::optional<CTxOut> GetExternalOutput(const COutPoint& outpoint) const;
    /**
     * Lock-in the given output for spending.
     * The output will be included in the transaction even if it's not the most optimal choice.
     */
    PreselectedInput& Select(const COutPoint& outpoint);
    /**
     * Unselects the given output.
     */
    void UnSelect(const COutPoint& outpoint);
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
     * Returns the input weight.
     */
    std::optional<int64_t> GetInputWeight(const COutPoint& outpoint) const;
    /** Retrieve the sequence for an input */
    std::optional<uint32_t> GetSequence(const COutPoint& outpoint) const;
    /** Retrieves the scriptSig and scriptWitness for an input. */
    std::pair<std::optional<CScript>, std::optional<CScriptWitness>> GetScripts(const COutPoint& outpoint) const;

    bool HasSelectedOrder() const
    {
        return m_selection_pos > 0;
    }

    std::optional<unsigned int> GetSelectionPos(const COutPoint& outpoint) const
    {
        const auto it = m_selected.find(outpoint);
        if (it == m_selected.end()) {
            return std::nullopt;
        }
        return it->second.GetPosition();
    }

private:
    //! Selected inputs (inputs that will be used, regardless of whether they're optimal or not)
    std::map<COutPoint, PreselectedInput> m_selected;
    unsigned int m_selection_pos{0};
};
} // namespace wallet

#endif // BITCOIN_WALLET_COINCONTROL_H
