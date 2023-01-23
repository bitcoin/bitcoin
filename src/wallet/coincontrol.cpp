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
    return !m_selected.empty();
}

bool CCoinControl::IsSelected(const COutPoint& outpoint) const
{
    return m_selected.count(outpoint) > 0;
}

bool CCoinControl::IsExternalSelected(const COutPoint& outpoint) const
{
    const auto it = m_selected.find(outpoint);
    return it != m_selected.end() && it->second.HasTxOut();
}

std::optional<CTxOut> CCoinControl::GetExternalOutput(const COutPoint& outpoint) const
{
    const auto it = m_selected.find(outpoint);
    if (it == m_selected.end() || !it->second.HasTxOut()) {
        return std::nullopt;
    }
    return it->second.GetTxOut();
}

PreselectedInput& CCoinControl::Select(const COutPoint& outpoint)
{
    return m_selected[outpoint];
}

void CCoinControl::SelectExternal(const COutPoint& outpoint, const CTxOut& txout)
{
    m_selected[outpoint].SetTxOut(txout);
}

void CCoinControl::UnSelect(const COutPoint& outpoint)
{
    m_selected.erase(outpoint);
}

void CCoinControl::UnSelectAll()
{
    m_selected.clear();
}

std::vector<COutPoint> CCoinControl::ListSelected() const
{
    std::vector<COutPoint> outpoints;
    std::transform(m_selected.begin(), m_selected.end(), std::back_inserter(outpoints),
        [](const std::map<COutPoint, PreselectedInput>::value_type& pair) {
            return pair.first;
        });
    return outpoints;
}

void CCoinControl::SetInputWeight(const COutPoint& outpoint, int64_t weight)
{
    m_selected[outpoint].SetInputWeight(weight);
}

std::optional<int64_t> CCoinControl::GetInputWeight(const COutPoint& outpoint) const
{
    const auto it = m_selected.find(outpoint);
    return it != m_selected.end() ? it->second.GetInputWeight() : std::nullopt;
}

void PreselectedInput::SetTxOut(const CTxOut& txout)
{
    m_txout = txout;
}

CTxOut PreselectedInput::GetTxOut() const
{
    assert(m_txout.has_value());
    return m_txout.value();
}

bool PreselectedInput::HasTxOut() const
{
    return m_txout.has_value();
}

void PreselectedInput::SetInputWeight(int64_t weight)
{
    m_weight = weight;
}

std::optional<int64_t> PreselectedInput::GetInputWeight() const
{
    return m_weight;
}
} // namespace wallet
