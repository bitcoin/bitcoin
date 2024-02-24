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
    const PublicKey& GetSenderPubKey() const noexcept { return m_output.GetSenderPubKey(); }
    const PublicKey& GetReceiverPubKey() const noexcept { return m_output.GetReceiverPubKey(); }
    const OutputMessage& GetOutputMessage() const noexcept { return m_output.GetOutputMessage(); }
    const RangeProof::CPtr& GetRangeProof() const noexcept { return m_output.GetRangeProof(); }
    const Signature& GetSignature() const noexcept { return m_output.GetSignature(); }
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

/// <summary>
/// MWEB UTXO wrapper that supports serialization into multiple formats.
/// </summary>
class NetUTXO
{
public:
    static const uint8_t FULL_UTXO = 0x00;
    static const uint8_t HASH_ONLY = 0x01;
    static const uint8_t COMPACT_UTXO = 0x02;

    NetUTXO() = default;
    NetUTXO(const uint8_t format, const UTXO::CPtr& utxo)
        : m_format(format), m_utxo(utxo) { }

    template <typename Stream>
    inline void Serialize(Stream& s) const
    {
        s << COMPACTSIZE(m_utxo->GetLeafIndex().Get());

        if (m_format == FULL_UTXO) {
            s << m_utxo->GetOutput();
        } else if (m_format == HASH_ONLY) {
            s << m_utxo->GetOutputID();
        } else if (m_format == COMPACT_UTXO) {
            s << m_utxo->GetCommitment();
            s << m_utxo->GetSenderPubKey();
            s << m_utxo->GetReceiverPubKey();
            s << m_utxo->GetOutputMessage();
            s << m_utxo->GetRangeProof()->GetHash();
            s << m_utxo->GetSignature();
        } else {
            throw std::ios_base::failure("Unsupported MWEB UTXO serialization format");
        }
    }

private:
    uint8_t m_format;
    UTXO::CPtr m_utxo;
};