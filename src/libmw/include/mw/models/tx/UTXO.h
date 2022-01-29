#pragma once

#include <mw/common/Macros.h>
#include <mw/common/Traits.h>
#include <mw/models/tx/Output.h>
#include <mw/mmr/LeafIndex.h>
#include <serialize.h>

class UTXO : public Traits::ISerializable
{
public:
    using CPtr = std::shared_ptr<const UTXO>;

    UTXO() : m_blockHeight(0), m_leafIdx(), m_output() { }
    UTXO(const int32_t blockHeight, mmr::LeafIndex leafIdx, Output output)
        : m_blockHeight(blockHeight), m_leafIdx(std::move(leafIdx)), m_output(std::move(output)) { }

    int32_t GetBlockHeight() const noexcept { return m_blockHeight; }
    const mmr::LeafIndex& GetLeafIndex() const noexcept { return m_leafIdx; }
    const Output& GetOutput() const noexcept { return m_output; }

    const mw::Hash& GetOutputID() const noexcept { return m_output.GetOutputID(); }
    const Commitment& GetCommitment() const noexcept { return m_output.GetCommitment(); }
    const PublicKey& GetReceiverPubKey() const noexcept { return m_output.GetReceiverPubKey(); }
    const RangeProof::CPtr& GetRangeProof() const noexcept { return m_output.GetRangeProof(); }
    ProofData BuildProofData() const noexcept { return m_output.BuildProofData(); }

    IMPL_SERIALIZABLE(UTXO, obj)
    {
        READWRITE(obj.m_blockHeight, obj.m_leafIdx, obj.m_output);
    }

private:
    int32_t m_blockHeight;
    mmr::LeafIndex m_leafIdx;
    Output m_output;
};