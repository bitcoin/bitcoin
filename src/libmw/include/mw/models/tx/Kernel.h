#pragma once

#include <mw/common/Macros.h>
#include <mw/common/Traits.h>
#include <mw/crypto/Hasher.h>
#include <mw/models/crypto/BlindingFactor.h>
#include <mw/models/crypto/Commitment.h>
#include <mw/models/crypto/SecretKey.h>
#include <mw/models/crypto/Signature.h>
#include <mw/models/crypto/SignedMessage.h>
#include <mw/models/tx/PegOutCoin.h>
#include <amount.h>
#include <boost/optional.hpp>
#include <numeric>

class Kernel :
    public Traits::ICommitted,
    public Traits::IHashable,
    public Traits::ISerializable
{
public:
    Kernel() = default;
    Kernel(
        const uint8_t features,
        boost::optional<CAmount> fee,
        boost::optional<CAmount> pegin,
        std::vector<PegOutCoin> pegouts,
        boost::optional<int32_t> lockHeight,
        boost::optional<PublicKey> stealthExcess,
        std::vector<uint8_t> extraData,
        Commitment excess,
        Signature signature
    ) : m_features(features),
        m_fee(fee),
        m_pegin(pegin),
        m_pegouts(std::move(pegouts)),
        m_lockHeight(lockHeight),
        m_stealthExcess(std::move(stealthExcess)),
        m_extraData(std::move(extraData)),
        m_excess(std::move(excess)),
        m_signature(std::move(signature))
    {
        m_hash = Hashed(*this);
    }

    enum FeatureBit {
        FEE_FEATURE_BIT = 0x01,
        PEGIN_FEATURE_BIT = 0x02,
        PEGOUT_FEATURE_BIT = 0x04,
        HEIGHT_LOCK_FEATURE_BIT = 0x08,
        STEALTH_EXCESS_FEATURE_BIT = 0x10,
        EXTRA_DATA_FEATURE_BIT = 0x20,
        ALL_FEATURE_BITS = FEE_FEATURE_BIT | PEGIN_FEATURE_BIT | PEGOUT_FEATURE_BIT | HEIGHT_LOCK_FEATURE_BIT | STEALTH_EXCESS_FEATURE_BIT | EXTRA_DATA_FEATURE_BIT
    };

    //
    // Factories
    //
    static Kernel Create(
        const BlindingFactor& blind,
        const boost::optional<SecretKey>& stealth_blind,
        const boost::optional<CAmount>& fee,
        const boost::optional<CAmount>& pegin_amount,
        const std::vector<PegOutCoin>& pegouts,
        const boost::optional<int32_t>& lock_height
    );

    //
    // Operators
    //
    bool operator<(const Kernel& rhs) const { return GetKernelID() < rhs.GetKernelID(); }
    bool operator==(const Kernel& rhs) const { return GetKernelID() == rhs.GetKernelID(); }
    bool operator!=(const Kernel& rhs) const { return GetKernelID() != rhs.GetKernelID(); }

    //
    // Getters
    //
    const mw::Hash& GetKernelID() const noexcept { return m_hash; }
    uint8_t GetFeatures() const noexcept { return m_features; }
    CAmount GetFee() const noexcept { return m_fee.value_or(0); }
    int32_t GetLockHeight() const noexcept { return m_lockHeight.value_or(0); }
    const Commitment& GetExcess() const noexcept { return m_excess; }
    const PublicKey& GetStealthExcess() const noexcept { return m_stealthExcess.value(); }
    const Signature& GetSignature() const noexcept { return m_signature; }
    const std::vector<uint8_t>& GetExtraData() const noexcept { return m_extraData; }

    bool IsStandard() const noexcept { return m_features < EXTRA_DATA_FEATURE_BIT; }

    SignedMessage BuildSignedMsg() const;
    static mw::Hash GetSignatureMessage(
        const uint8_t features,
        const Commitment& excess_commitment,
        const boost::optional<PublicKey>& stealth_commitment,
        const boost::optional<CAmount>& fee,
        const boost::optional<CAmount>& pegin_amount,
        const std::vector<PegOutCoin>& pegouts,
        const boost::optional<int32_t>& lock_height,
        const std::vector<uint8_t>& extra_data
    );

    bool HasPegIn() const noexcept { return !!m_pegin; }
    bool HasPegOut() const noexcept { return !m_pegouts.empty(); }
    bool HasStealthExcess() const noexcept { return !!m_stealthExcess; }

    CAmount GetPegIn() const noexcept { return m_pegin.value_or(0); }
    const std::vector<PegOutCoin>& GetPegOuts() const noexcept { return m_pegouts; }

    CAmount GetPegOutAmount() const noexcept
    {
        return std::accumulate(
            m_pegouts.cbegin(), m_pegouts.cend(), (CAmount)0,
            [](CAmount sum, const PegOutCoin& pegout) { return sum + pegout.GetAmount(); }
        );
    }

    CAmount GetSupplyChange() const noexcept
    {
        return (m_pegin.value_or(0) - m_fee.value_or(0)) - GetPegOutAmount();
    }

    //
    // Serialization/Deserialization
    //
    IMPL_SERIALIZED(Kernel);

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << m_features;

        if (m_fee) {
            ::WriteVarInt<Stream, VarIntMode::NONNEGATIVE_SIGNED, CAmount>(s, m_fee.value());
        }

        if (m_pegin) {
            ::WriteVarInt<Stream, VarIntMode::NONNEGATIVE_SIGNED, CAmount>(s, m_pegin.value());
        }

        if (!m_pegouts.empty()) {
            s << m_pegouts;
        }

        if (m_lockHeight) {
            ::WriteVarInt<Stream, VarIntMode::NONNEGATIVE_SIGNED, int32_t>(s, m_lockHeight.value());
        }

        if (m_stealthExcess) {
            s << m_stealthExcess.value();
        }

        if (!m_extraData.empty()) {
            s << m_extraData;
        }

        s << m_excess << m_signature;
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> m_features;

        if (m_features & FEE_FEATURE_BIT) {
            m_fee = ::ReadVarInt<Stream, VarIntMode::NONNEGATIVE_SIGNED, CAmount>(s);
        }

        if (m_features & PEGIN_FEATURE_BIT) {
            m_pegin = ::ReadVarInt<Stream, VarIntMode::NONNEGATIVE_SIGNED, CAmount>(s);
        }

        if (m_features & PEGOUT_FEATURE_BIT) {
            s >>m_pegouts;
        }

        if (m_features & HEIGHT_LOCK_FEATURE_BIT) {
            m_lockHeight = ::ReadVarInt<Stream, VarIntMode::NONNEGATIVE_SIGNED, int32_t>(s);
        }

        if (m_features & STEALTH_EXCESS_FEATURE_BIT) {
            PublicKey stealth_excess;
            s >> stealth_excess;
            m_stealthExcess = boost::make_optional(std::move(stealth_excess));
        }

        if (m_features & EXTRA_DATA_FEATURE_BIT) {
            s >> m_extraData;
        }

        s >> m_excess >> m_signature;

        m_hash = Hashed(*this);
    }

    //
    // Traits
    //
    const mw::Hash& GetHash() const noexcept final { return m_hash; }
    const Commitment& GetCommitment() const noexcept final { return m_excess; }

private:
    uint8_t m_features;
    boost::optional<CAmount> m_fee;
    boost::optional<CAmount> m_pegin;
    std::vector<PegOutCoin> m_pegouts;
    boost::optional<int32_t> m_lockHeight;
    boost::optional<PublicKey> m_stealthExcess;
    std::vector<uint8_t> m_extraData;

    // Remainder of the sum of all transaction commitments. 
    // If the transaction is well formed, amounts components should sum to zero and the excess is hence a valid public key.
    Commitment m_excess;

    // The signature proving the excess is a valid public key, which signs the transaction fee.
    Signature m_signature;

    mw::Hash m_hash;
};

// Sorts by net supply increase [pegin - (fee + pegout)] with highest increase first, then sorts by hash.
static const struct
{
    bool operator()(const Kernel& a, const Kernel& b) const
    {
        CAmount a_pegin = a.GetSupplyChange();
        CAmount b_pegin = b.GetSupplyChange();
        return (a_pegin > b_pegin) || (a_pegin == b_pegin && a.GetHash() < b.GetHash());
    }
} KernelSort;