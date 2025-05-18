// Copyright (c) 2018-2021 The Tortoisecoin Core developers
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
    auto& input = m_selected[outpoint];
    input.SetPosition(m_selection_pos);
    ++m_selection_pos;
    return input;
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

std::optional<uint32_t> CCoinControl::GetSequence(const COutPoint& outpoint) const
{
    const auto it = m_selected.find(outpoint);
    return it != m_selected.end() ? it->second.GetSequence() : std::nullopt;
}

std::pair<std::optional<CScript>, std::optional<CScriptWitness>> CCoinControl::GetScripts(const COutPoint& outpoint) const
{
    const auto it = m_selected.find(outpoint);
    return it != m_selected.end() ? m_selected.at(outpoint).GetScripts() : std::make_pair(std::nullopt, std::nullopt);
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

void PreselectedInput::SetSequence(uint32_t sequence)
{
    m_sequence = sequence;
}

std::optional<uint32_t> PreselectedInput::GetSequence() const
{
    return m_sequence;
}

void PreselectedInput::SetScriptSig(const CScript& script)
{
    m_script_sig = script;
}

void PreselectedInput::SetScriptWitness(const CScriptWitness& script_wit)
{
    m_script_witness = script_wit;
}

bool PreselectedInput::HasScripts() const
{
    return m_script_sig.has_value() || m_script_witness.has_value();
}

std::pair<std::optional<CScript>, std::optional<CScriptWitness>> PreselectedInput::GetScripts() const
{
    return {m_script_sig, m_script_witness};
}

void PreselectedInput::SetPosition(unsigned int pos)
{
    m_pos = pos;
}

std::optional<unsigned int> PreselectedInput::GetPosition() const
{
    return m_pos;
}
} // namespace wallet
