// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/coincontrol.h>

#include <common/args.h>

namespace wallet {
CCoinControl::CCoinControl()
{
    m_avoid_partial_spends = gArgs.GetBoolArg("-avoidpartialspends", DEFAULT_AVOIDPARTIALSPENDS);
}

bool CCoinControl::HasSelected() const
{
    return !m_selected_inputs.empty();
}

bool CCoinControl::IsSelected(const COutPoint& output) const
{
    return m_selected_inputs.count(output) > 0;
}

bool CCoinControl::IsExternalSelected(const COutPoint& output) const
{
    return m_external_txouts.count(output) > 0;
}

std::optional<CTxOut> CCoinControl::GetExternalOutput(const COutPoint& outpoint) const
{
    const auto ext_it = m_external_txouts.find(outpoint);
    if (ext_it == m_external_txouts.end()) {
        return std::nullopt;
    }

    return std::make_optional(ext_it->second);
}

void CCoinControl::Select(const COutPoint& output)
{
    m_selected_inputs.insert(output);
}

void CCoinControl::SelectExternal(const COutPoint& outpoint, const CTxOut& txout)
{
    m_selected_inputs.insert(outpoint);
    m_external_txouts.emplace(outpoint, txout);
}

void CCoinControl::UnSelect(const COutPoint& output)
{
    m_selected_inputs.erase(output);
}

void CCoinControl::UnSelectAll()
{
    m_selected_inputs.clear();
}

std::vector<COutPoint> CCoinControl::ListSelected() const
{
    return {m_selected_inputs.begin(), m_selected_inputs.end()};
}

void CCoinControl::SetInputWeight(const COutPoint& outpoint, int64_t weight)
{
    m_input_weights[outpoint] = weight;
}

bool CCoinControl::HasInputWeight(const COutPoint& outpoint) const
{
    return m_input_weights.count(outpoint) > 0;
}

int64_t CCoinControl::GetInputWeight(const COutPoint& outpoint) const
{
    auto it = m_input_weights.find(outpoint);
    assert(it != m_input_weights.end());
    return it->second;
}
} // namespace wallet
