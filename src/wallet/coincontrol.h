// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_WALLET_COINCONTROL_H
#define SYSCOIN_WALLET_COINCONTROL_H

#include <outputtype.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <primitives/transaction.h>
#include <script/keyorigin.h>
#include <script/signingprovider.h>
#include <script/standard.h>

#include <optional>
#include <algorithm>
#include <map>
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
    // SYSCOIN
    //! Custom transaction version
    int m_version = CTransaction::CURRENT_VERSION;
    //! Custom poda data
    std::vector<uint8_t> m_nevmdata;
    CCoinControl();

    bool HasSelected() const
    {
        return (setSelected.size() > 0);
    }

    bool IsSelected(const COutPoint& output) const
    {
        return (setSelected.count(output) > 0);
    }

    bool IsExternalSelected(const COutPoint& output) const
    {
        return (m_external_txouts.count(output) > 0);
    }

    bool GetExternalOutput(const COutPoint& outpoint, CTxOut& txout) const
    {
        const auto ext_it = m_external_txouts.find(outpoint);
        if (ext_it == m_external_txouts.end()) {
            return false;
        }
        txout = ext_it->second;
        return true;
    }

    void Select(const COutPoint& output)
    {
        setSelected.insert(output);
    }

    void SelectExternal(const COutPoint& outpoint, const CTxOut& txout)
    {
        setSelected.insert(outpoint);
        m_external_txouts.emplace(outpoint, txout);
    }

    void UnSelect(const COutPoint& output)
    {
        setSelected.erase(output);
    }

    void UnSelectAll()
    {
        setSelected.clear();
    }

    void ListSelected(std::vector<COutPoint>& vOutpoints) const
    {
        vOutpoints.assign(setSelected.begin(), setSelected.end());
    }

    void SetInputWeight(const COutPoint& outpoint, int64_t weight)
    {
        m_input_weights[outpoint] = weight;
    }

    bool HasInputWeight(const COutPoint& outpoint) const
    {
        return m_input_weights.count(outpoint) > 0;
    }

    int64_t GetInputWeight(const COutPoint& outpoint) const
    {
        auto it = m_input_weights.find(outpoint);
        assert(it != m_input_weights.end());
        return it->second;
    }

private:
    std::set<COutPoint> setSelected;
    std::map<COutPoint, CTxOut> m_external_txouts;
    //! Map of COutPoints to the maximum weight for that input
    std::map<COutPoint, int64_t> m_input_weights;
};
} // namespace wallet

#endif // SYSCOIN_WALLET_COINCONTROL_H
