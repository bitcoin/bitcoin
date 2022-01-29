#include <mw/node/CoinsView.h>
#include <mw/exceptions/ValidationException.h>
#include <mw/consensus/Aggregation.h>
#include <mw/consensus/KernelSumValidator.h>
#include <mw/common/Logger.h>
#include <mw/db/MMRInfoDB.h>

#include "CoinActions.h"

using namespace mw;

CoinsViewCache::CoinsViewCache(const ICoinsView::Ptr& pBase)
    : ICoinsView(pBase->GetBestHeader(), pBase->GetDatabase()),
      m_pBase(pBase),
      m_pLeafSet(std::make_unique<LeafSetCache>(pBase->GetLeafSet())),
      m_pOutputPMMR(std::make_unique<PMMRCache>(pBase->GetOutputPMMR())),
      m_pUpdates(std::make_shared<CoinsViewUpdates>()) {}

std::vector<UTXO::CPtr> CoinsViewCache::GetUTXOs(const mw::Hash& output_id) const noexcept
{
    std::vector<UTXO::CPtr> utxos = m_pBase->GetUTXOs(output_id);

    std::vector<CoinAction> actions = m_pUpdates->GetActions(output_id);
    for (const CoinAction& action : actions) {
        if (action.pUTXO != nullptr) {
            utxos.push_back(action.pUTXO);
        } else {
            assert(!utxos.empty());
            utxos.pop_back();
        }
    }

    return utxos;
}

mw::BlockUndo::CPtr CoinsViewCache::ApplyBlock(const mw::Block::CPtr& pBlock)
{
    assert(pBlock != nullptr);

    auto pPreviousHeader = GetBestHeader();
    SetBestHeader(pBlock->GetHeader());

    BlindingFactor prev_offset = pPreviousHeader != nullptr ? pPreviousHeader->GetKernelOffset() : BlindingFactor();
    KernelSumValidator::ValidateForBlock(pBlock->GetTxBody(), pBlock->GetKernelOffset(), prev_offset);

    std::vector<UTXO> coinsSpent;
    std::for_each(
        pBlock->GetInputs().cbegin(), pBlock->GetInputs().cend(),
        [this, &coinsSpent](const Input& input) {
            UTXO spentUTXO = SpendUTXO(input.GetOutputID());
            coinsSpent.push_back(std::move(spentUTXO));
        }
    );

    std::vector<mw::Hash> coinsAdded;
    std::for_each(
        pBlock->GetOutputs().cbegin(), pBlock->GetOutputs().cend(),
        [this, &pBlock, &coinsAdded](const Output& output) {
            AddUTXO(pBlock->GetHeight(), output);
            coinsAdded.push_back(output.GetOutputID());
        }
    );

    auto pHeader = pBlock->GetHeader();
    if (pHeader->GetOutputRoot() != GetOutputPMMR()->Root()
        || pHeader->GetNumTXOs() != GetOutputPMMR()->GetNumLeaves()
        || pHeader->GetLeafsetRoot() != GetLeafSet()->Root())
    {
        ThrowValidation(EConsensusError::MMR_MISMATCH);
    }

    return std::make_shared<mw::BlockUndo>(pPreviousHeader, std::move(coinsSpent), std::move(coinsAdded));
}

void CoinsViewCache::UndoBlock(const mw::BlockUndo::CPtr& pUndo)
{
    assert(pUndo != nullptr);

    for (const mw::Hash& coinToRemove : pUndo->GetCoinsAdded()) {
        m_pUpdates->SpendUTXO(coinToRemove);
    }

    std::vector<mmr::LeafIndex> leavesToAdd;
    for (const UTXO& coinToAdd : pUndo->GetCoinsSpent()) {
        leavesToAdd.push_back(coinToAdd.GetLeafIndex());
        m_pUpdates->AddUTXO(std::make_shared<UTXO>(coinToAdd));
    }

    auto pHeader = pUndo->GetPreviousHeader();
    if (pHeader == nullptr) {
        m_pLeafSet->Rewind(0, {});
        m_pOutputPMMR->Rewind(0);
        SetBestHeader(nullptr);
        return;
    }

    m_pLeafSet->Rewind(pHeader->GetNumTXOs(), leavesToAdd);
    m_pOutputPMMR->Rewind(pHeader->GetNumTXOs());
    SetBestHeader(pHeader);

    // Sanity check to make sure rewind applied successfully
    if (pHeader->GetOutputRoot() != GetOutputPMMR()->Root()
        || pHeader->GetNumTXOs() != GetOutputPMMR()->GetNumLeaves()
        || pHeader->GetLeafsetRoot() != GetLeafSet()->Root())
    {
        ThrowValidation(EConsensusError::MMR_MISMATCH);
    }
}

mw::Block::Ptr CoinsViewCache::BuildNextBlock(const uint64_t height, const std::vector<mw::Transaction::CPtr>& transactions)
{
    LOG_TRACE_F("Building block with {} transactions", transactions.size());

    auto pTransaction = Aggregation::Aggregate(transactions);

    MemMMR::Ptr pKernelMMR = std::make_shared<MemMMR>();
    std::for_each(
        pTransaction->GetKernels().cbegin(), pTransaction->GetKernels().cend(),
        [&pKernelMMR](const Kernel& kernel) { pKernelMMR->Add(kernel); }
    );

    std::for_each(
        pTransaction->GetOutputs().cbegin(), pTransaction->GetOutputs().cend(),
        [this, height](const Output& output) { AddUTXO(height, output); }
    );

    std::for_each(
        pTransaction->GetInputs().cbegin(), pTransaction->GetInputs().cend(),
        [this](const Input& input) { SpendUTXO(input.GetOutputID()); } // MW: TODO - Do we need to check the UTXO's receiver public key? Presumably, that was already done.
    );

    const uint64_t output_mmr_size = m_pOutputPMMR->GetNumLeaves();
    const uint64_t kernel_mmr_size = pKernelMMR->GetNumLeaves();

    mw::Hash output_root = m_pOutputPMMR->Root();
    mw::Hash kernel_root = pKernelMMR->Root();
    mw::Hash leafset_root = m_pLeafSet->Root();

    BlindingFactor kernel_offset = pTransaction->GetKernelOffset();
    if (GetBestHeader() != nullptr) {
        kernel_offset = Pedersen::AddBlindingFactors({
            GetBestHeader()->GetKernelOffset(),
            pTransaction->GetKernelOffset()
        });
    }

    BlindingFactor stealth_offset = pTransaction->GetStealthOffset();

    auto pHeader = std::make_shared<mw::Header>(
        height,
        std::move(output_root),
        std::move(kernel_root),
        std::move(leafset_root),
        std::move(kernel_offset),
        std::move(stealth_offset),
        output_mmr_size,
        kernel_mmr_size
    );

    return std::make_shared<mw::Block>(pHeader, pTransaction->GetBody());
}

bool CoinsViewCache::HasCoinInCache(const mw::Hash& output_id) const noexcept
{
    std::vector<CoinAction> actions = m_pUpdates->GetActions(output_id);
    if (!actions.empty()) {
        return actions.back().pUTXO != nullptr;
    }

    return false;
}

void CoinsViewCache::AddUTXO(const uint64_t header_height, const Output& output)
{
    mmr::LeafIndex leafIdx = m_pOutputPMMR->Add(output.GetOutputID());
    m_pLeafSet->Add(leafIdx);

    auto pUTXO = std::make_shared<UTXO>(header_height, std::move(leafIdx), output);

    m_pUpdates->AddUTXO(pUTXO);
}

UTXO CoinsViewCache::SpendUTXO(const mw::Hash& output_id)
{
    std::vector<UTXO::CPtr> utxos = GetUTXOs(output_id);
    if (utxos.empty() || !m_pLeafSet->Contains(utxos.back()->GetLeafIndex())) {
        ThrowValidation(EConsensusError::UTXO_MISSING);
    }

    m_pLeafSet->Remove(utxos.back()->GetLeafIndex());
    m_pUpdates->SpendUTXO(output_id);

    return *utxos.back();
}

void CoinsViewCache::WriteBatch(const std::unique_ptr<mw::DBBatch>&, const CoinsViewUpdates& updates, const mw::Header::CPtr& pHeader)
{
    SetBestHeader(pHeader);

    for (const auto& actions : updates.GetActions()) {
        const mw::Hash& output_id = actions.first;
        for (const auto& action : actions.second) {
            if (action.IsSpend()) {
                m_pUpdates->SpendUTXO(output_id);
            } else {
                m_pUpdates->AddUTXO(action.pUTXO);
            }
        }
    }
}

void CoinsViewCache::Flush(const std::unique_ptr<mw::DBBatch>& pBatch)
{
    if (GetBestHeader() == nullptr) {
        return;
    }
    
    m_pBase->WriteBatch(pBatch, *m_pUpdates, GetBestHeader());

    MMRInfo mmr_info;
    if (!m_pBase->IsCache()) {
        auto current_mmr_info = MMRInfoDB(GetDatabase().get(), pBatch.get())
            .GetLatest();
        if (current_mmr_info) {
            mmr_info = *current_mmr_info;
        }

        ++mmr_info.index;
    }

    m_pLeafSet->Flush(mmr_info.index);
    m_pOutputPMMR->Flush(mmr_info.index, pBatch);

    if (!m_pBase->IsCache()) {
        MMRInfoDB(GetDatabase().get(), pBatch.get())
            .Save(mmr_info);
    }

    m_pUpdates->Clear();
}