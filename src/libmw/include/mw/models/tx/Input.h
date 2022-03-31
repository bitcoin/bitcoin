#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Traits.h>
#include <mw/crypto/Hasher.h>
#include <mw/models/crypto/Commitment.h>
#include <mw/models/crypto/PublicKey.h>
#include <mw/models/crypto/Signature.h>
#include <mw/models/crypto/SignedMessage.h>

////////////////////////////////////////
// INPUT
////////////////////////////////////////
class Input :
    public Traits::ICommitted,
    public Traits::IHashable,
    public Traits::IPrintable,
    public Traits::ISerializable
{
    enum FeatureBit {
        STEALTH_KEY_FEATURE_BIT = 0x01,
        EXTRA_DATA_FEATURE_BIT = 0x02
    };

public:
    //
    // Constructors
    //
    Input(uint8_t features, mw::Hash output_id, Commitment commitment, PublicKey input_pubkey, PublicKey output_pubkey, Signature signature)
        : m_features(features),
        m_outputID(std::move(output_id)),
        m_commitment(std::move(commitment)),
        m_inputPubKey(std::move(input_pubkey)),
        m_outputPubKey(std::move(output_pubkey)),
        m_signature(std::move(signature))
    {
        m_hash = Hashed(*this);
    }
    Input(const Input& input) = default;
    Input(Input&& input) noexcept = default;
    Input() = default;

    //
    // Factories
    //
    static Input Create(const mw::Hash& output_id, const Commitment& commitment, const SecretKey& input_key, const SecretKey& output_key);

    //
    // Destructor
    //
    virtual ~Input() = default;

    //
    // Operators
    //
    Input& operator=(const Input& input) = default;
    Input& operator=(Input&& input) noexcept = default;
    bool operator<(const Input& input) const noexcept { return m_hash < input.m_hash; }
    bool operator==(const Input& input) const noexcept { return m_hash == input.m_hash; }

    //
    // Getters
    //
    bool IsStandard() const noexcept { return m_features < FeatureBit::EXTRA_DATA_FEATURE_BIT; }
    const mw::Hash& GetOutputID() const noexcept { return m_outputID; }
    const Commitment& GetCommitment() const noexcept final { return m_commitment; }
    const PublicKey& GetOutputPubKey() const noexcept { return m_outputPubKey; }
    const boost::optional<PublicKey>& GetInputPubKey() const noexcept { return m_inputPubKey; }
    const std::vector<uint8_t>& GetExtraData() const noexcept { return m_extraData; }
    const Signature& GetSignature() const noexcept { return m_signature; }

    SignedMessage BuildSignedMsg() const noexcept;

    //
    // Serialization/Deserialization
    //
    IMPL_SERIALIZABLE(Input, obj)
    {
        READWRITE(obj.m_features);
        READWRITE(obj.m_outputID);
        READWRITE(obj.m_commitment);
        READWRITE(obj.m_outputPubKey);

        if (obj.m_features & STEALTH_KEY_FEATURE_BIT) {
            PublicKey input_pubkey;
            SER_WRITE(obj, input_pubkey = *obj.m_inputPubKey);
            READWRITE(input_pubkey);
            SER_READ(obj, obj.m_inputPubKey = boost::make_optional<PublicKey>(std::move(input_pubkey)));
        }

        if (obj.m_features & EXTRA_DATA_FEATURE_BIT) {
            READWRITE(obj.m_extraData);
        }

        READWRITE(obj.m_signature);
        SER_READ(obj, obj.m_hash = Hashed(obj));
    }

    //
    // Traits
    //
    std::string Format() const final { return "Input(ID=" + GetOutputID().Format() + ")"; }
    const mw::Hash& GetHash() const noexcept final { return m_hash; }

private:
    uint8_t m_features;

    // The ID of the output being spent.
    mw::Hash m_outputID;

    // The commit referencing the output being spent.
    Commitment m_commitment;

    // The input pubkey.
    boost::optional<PublicKey> m_inputPubKey;

    // The public key of the output being spent.
    PublicKey m_outputPubKey;

    std::vector<uint8_t> m_extraData;

    Signature m_signature;

    mw::Hash m_hash;
};

// Sorts by output ID (hash)
static const struct
{
    bool operator()(const Input& a, const Input& b) const
    {
        return a.GetOutputID() < b.GetOutputID();
    }
} InputSort;