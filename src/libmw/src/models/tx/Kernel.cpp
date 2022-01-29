#include <mw/models/tx/Kernel.h>
#include <mw/crypto/Schnorr.h>
#include <mw/crypto/SecretKeys.h>

Kernel Kernel::Create(
    const BlindingFactor& blind,
    const boost::optional<SecretKey>& stealth_blind,
    const boost::optional<CAmount>& fee,
    const boost::optional<CAmount>& pegin_amount,
    const std::vector<PegOutCoin>& pegouts,
    const boost::optional<int32_t>& lock_height)
{
    const uint8_t features_byte =
        (fee ? FEE_FEATURE_BIT : 0) |
        (pegin_amount ? PEGIN_FEATURE_BIT : 0) |
        (pegouts.empty() ? 0 : PEGOUT_FEATURE_BIT) |
        (lock_height ? HEIGHT_LOCK_FEATURE_BIT : 0) |
        (stealth_blind ? STEALTH_EXCESS_FEATURE_BIT : 0);

    SecretKey sig_key = blind.data();
    Commitment excess_commit = Commitment::Blinded(blind, 0);
    boost::optional<PublicKey> stealth_excess = boost::none;

    if (stealth_blind) {
        stealth_excess = PublicKey::From(stealth_blind.value());

        Hasher h;
        h << PublicKey::From(excess_commit) << stealth_excess.value();

        sig_key = SecretKeys::From(sig_key)
            .Mul(h.hash())
            .Add(stealth_blind.value())
            .Total();
    }

    mw::Hash sig_message = Kernel::GetSignatureMessage(
        features_byte,
        excess_commit,
        stealth_excess,
        fee,
        pegin_amount,
        pegouts,
        lock_height,
        {}
    );
    Signature sig = Schnorr::Sign(sig_key.data(), sig_message);
    return Kernel(
        features_byte,
        fee,
        pegin_amount,
        pegouts,
        lock_height,
        std::move(stealth_excess),
        std::vector<uint8_t>{},
        std::move(excess_commit),
        std::move(sig)
    );
}

SignedMessage Kernel::BuildSignedMsg() const
{
    PublicKey public_key = PublicKey::From(GetExcess());
    if (HasStealthExcess()) {
        PublicKey stealth_excess = GetStealthExcess();

        Hasher h;
        h << public_key << stealth_excess;

        public_key = public_key
            .Mul(h.hash())
            .Add(stealth_excess);
    }

    mw::Hash sig_message = Kernel::GetSignatureMessage(
        m_features,
        m_excess,
        m_stealthExcess,
        m_fee,
        m_pegin,
        m_pegouts,
        m_lockHeight,
        m_extraData
    );
    return SignedMessage{sig_message, public_key, GetSignature()};
}

mw::Hash Kernel::GetSignatureMessage(
    const uint8_t features,
    const Commitment& excess_commitment,
    const boost::optional<PublicKey>& stealth_commitment,
    const boost::optional<CAmount>& fee,
    const boost::optional<CAmount>& pegin_amount,
    const std::vector<PegOutCoin>& pegouts,
    const boost::optional<int32_t>& lock_height,
    const std::vector<uint8_t>& extra_data)
{
    Hasher s;
    s << features << excess_commitment;

    if (fee) {
        ::WriteVarInt<Hasher, VarIntMode::NONNEGATIVE_SIGNED, CAmount>(s, fee.value());
    }

    if (pegin_amount) {
        ::WriteVarInt<Hasher, VarIntMode::NONNEGATIVE_SIGNED, CAmount>(s, pegin_amount.value());
    }

    if (!pegouts.empty()) {
        s << pegouts;
    }

    if (lock_height) {
        ::WriteVarInt<Hasher, VarIntMode::NONNEGATIVE_SIGNED, int32_t>(s, lock_height.value());
    }

    if (stealth_commitment) {
        s << stealth_commitment.value();
    }

    if (!extra_data.empty()) {
        s << extra_data;
    }

    return s.hash();
}