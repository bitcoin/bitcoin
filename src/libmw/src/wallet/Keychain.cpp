#include <mw/wallet/Keychain.h>
#include <mw/crypto/Hasher.h>
#include <mw/crypto/SecretKeys.h>
#include <mw/models/tx/OutputMask.h>
#include <wallet/scriptpubkeyman.h>

MW_NAMESPACE

bool Keychain::RewindOutput(const Output& output, mw::Coin& coin) const
{
    if (!output.HasStandardFields()) {
        return false;
    }

    assert(!GetScanSecret().IsNull());
    PublicKey shared_secret = output.Ke().Mul(GetScanSecret());
    uint8_t view_tag = Hashed(EHashTag::TAG, shared_secret)[0];
    if (view_tag != output.GetViewTag()) {
        return false;
    }

    SecretKey t = Hashed(EHashTag::DERIVE, shared_secret);
    PublicKey B_i = output.Ko().Div(Hashed(EHashTag::OUT_KEY, t));

    // Check if B_i belongs to wallet
    StealthAddress address(B_i.Mul(m_scanSecret), B_i);
    auto pMetadata = m_spk_man.GetMetadata(address);
    if (!pMetadata || !pMetadata->mweb_index) {
        return false;
    }

    uint32_t index = *pMetadata->mweb_index;

    // Calc blinding factor and unmask nonce and amount.
    OutputMask mask = OutputMask::FromShared(t);
    uint64_t value = mask.MaskValue(output.GetMaskedValue());
    BigInt<16> n = mask.MaskNonce(output.GetMaskedNonce());

    if (mask.SwitchCommit(value) != output.GetCommitment()) {
        return false;
    }

    // Calculate Carol's sending key 's' and check that s*B ?= Ke
    SecretKey s = Hasher(EHashTag::SEND_KEY)
        .Append(address.A())
        .Append(address.B())
        .Append(value)
        .Append(n)
        .hash();
    if (output.Ke() != address.B().Mul(s)) {
        return false;
    }

    // Spend secret will be null for locked or watch-only wallets.
    if (!GetSpendSecret().IsNull()) {
        coin.spend_key = boost::make_optional(CalculateOutputKey(index, t));
    }

    coin.address_index = index;
    coin.blind = boost::make_optional(mask.GetRawBlind());
    coin.amount = value;
    coin.output_id = output.GetOutputID();
    coin.address = address;
    coin.shared_secret = boost::make_optional(std::move(t));

    return true;
}

SecretKey Keychain::CalculateOutputKey(const uint32_t index, const SecretKey& shared_secret) const
{
    assert(!m_spendSecret.IsNull());

    return SecretKeys::From(GetSpendKey(index))
        .Mul(Hashed(EHashTag::OUT_KEY, shared_secret))
        .Total();
}

StealthAddress Keychain::GetStealthAddress(const uint32_t index) const
{
    assert(!m_spendSecret.IsNull());

    PublicKey Bi = PublicKey::From(GetSpendKey(index));
    PublicKey Ai = Bi.Mul(m_scanSecret);

    return StealthAddress(Ai, Bi);
}

SecretKey Keychain::GetSpendKey(const uint32_t index) const
{
    assert(!m_spendSecret.IsNull());

    SecretKey mi = Hasher(EHashTag::ADDRESS)
        .Append<uint32_t>(index)
        .Append(m_scanSecret)
        .hash();

    return SecretKeys::From(m_spendSecret).Add(mi).Total();
}

END_NAMESPACE