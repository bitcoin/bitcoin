// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cisa.h>

#include <hash.h>
#include <key.h>
#include <random.h>
#include <support/allocators/secure.h>
#include <support/cleanse.h>

#include <secp256k1_extrakeys.h>
#include <secp256k1_fullagg.h>
#include <secp256k1_schnorrsig_halfagg.h>

#include <array>
#include <cstring>

class FullAggSecNonceImpl
{
private:
    secure_unique_ptr<secp256k1_fullagg_secnonce> m_nonce;

public:
    FullAggSecNonceImpl() : m_nonce{make_secure_unique<secp256k1_fullagg_secnonce>()} {}

    FullAggSecNonceImpl(const FullAggSecNonceImpl&) = delete;
    FullAggSecNonceImpl& operator=(const FullAggSecNonceImpl&) = delete;

    secp256k1_fullagg_secnonce* Get() const { return m_nonce.get(); }
    void Invalidate() { m_nonce.reset(); }
    bool IsValid() { return m_nonce != nullptr; }
};

FullAggSecNonce::FullAggSecNonce() : m_impl{std::make_unique<FullAggSecNonceImpl>()} {}
FullAggSecNonce::FullAggSecNonce(FullAggSecNonce&&) noexcept = default;
FullAggSecNonce& FullAggSecNonce::operator=(FullAggSecNonce&&) noexcept = default;
FullAggSecNonce::~FullAggSecNonce() = default;

secp256k1_fullagg_secnonce* FullAggSecNonce::Get() const { return m_impl->Get(); }
void FullAggSecNonce::Invalidate() { m_impl->Invalidate(); }
bool FullAggSecNonce::IsValid() { return m_impl->IsValid(); }

uint256 CISASessionID(const XOnlyPubKey& pubkey, const std::vector<uint8_t>& pubnonce)
{
    HashWriter hasher;
    hasher << pubkey << pubnonce;
    return hasher.GetSHA256();
}

namespace {
//! The group's keys, messages and pubnonces in the layout the secp256k1 fullagg API takes.
struct FullAggGroup {
    std::vector<secp256k1_xonly_pubkey> pubkeys;
    std::vector<secp256k1_fullagg_pubnonce> pubnonces;
    std::vector<const secp256k1_xonly_pubkey*> pubkey_ptrs;
    std::vector<const unsigned char*> msg_ptrs;
    std::vector<const secp256k1_fullagg_pubnonce*> pubnonce_ptrs;
    bool valid{false};

    FullAggGroup(const std::vector<XOnlyPubKey>& keys, const std::vector<uint256>& msgs, const std::vector<std::vector<uint8_t>>& nonces)
    {
        const size_t n{keys.size()};
        if (n == 0 || msgs.size() != n || nonces.size() != n) return;
        pubkeys.resize(n);
        pubnonces.resize(n);
        for (size_t i = 0; i < n; i++) {
            if (!secp256k1_xonly_pubkey_parse(secp256k1_context_static, &pubkeys[i], keys[i].data())) return;
            if (nonces[i].size() != FULLAGG_PUBNONCE_SIZE || !secp256k1_fullagg_pubnonce_parse(secp256k1_context_static, &pubnonces[i], nonces[i].data())) return;
            pubkey_ptrs.push_back(&pubkeys[i]);
            msg_ptrs.push_back(msgs[i].begin());
            pubnonce_ptrs.push_back(&pubnonces[i]);
        }
        valid = true;
    }

    size_t size() const { return pubkeys.size(); }

    bool InitSession(secp256k1_fullagg_session& session) const
    {
        secp256k1_fullagg_aggnonce aggnonce;
        if (!secp256k1_fullagg_nonce_agg(secp256k1_context_static, &aggnonce, pubnonce_ptrs.data(), size())) return false;
        return secp256k1_fullagg_session_init(secp256k1_context_static, &session, &aggnonce, pubkey_ptrs.data(), msg_ptrs.data(), pubnonce_ptrs.data(), size());
    }

    bool VerifyPartialSig(const secp256k1_fullagg_partial_sig& psig, size_t signer_index) const
    {
        return secp256k1_fullagg_partial_sig_verify(secp256k1_context_static, &psig, pubnonce_ptrs.data(), pubkey_ptrs.data(), msg_ptrs.data(), size(), signer_index);
    }
};
} // namespace

std::vector<uint8_t> CreateFullAggNonce(FullAggSecNonce& secnonce, const KeyPair& keypair)
{
    const secp256k1_keypair* kp{keypair.Get()};
    if (!kp) return {};
    secp256k1_xonly_pubkey pubkey;
    std::array<unsigned char, 32> seckey;
    if (!secp256k1_keypair_xonly_pub(secp256k1_context_static, &pubkey, nullptr, kp) || !secp256k1_keypair_sec(secp256k1_context_static, seckey.data(), kp)) return {};

    uint256 rand;
    GetStrongRandBytes(rand);
    secp256k1_fullagg_pubnonce pubnonce;
    const bool ok{secp256k1_fullagg_nonce_gen(GetSecp256k1SignContext(), secnonce.Get(), &pubnonce, rand.data(), seckey.data(), &pubkey, /*extra_input32=*/nullptr) != 0};
    memory_cleanse(seckey.data(), seckey.size());
    if (!ok) return {};

    std::vector<uint8_t> out(FULLAGG_PUBNONCE_SIZE);
    secp256k1_fullagg_pubnonce_serialize(secp256k1_context_static, out.data(), &pubnonce);
    return out;
}

std::optional<uint256> CreateFullAggPartialSig(const KeyPair& keypair, FullAggSecNonce& secnonce, const std::vector<XOnlyPubKey>& pubkeys, const std::vector<uint256>& msgs, const std::vector<std::vector<uint8_t>>& pubnonces, size_t signer_index)
{
    const secp256k1_keypair* kp{keypair.Get()};
    const FullAggGroup group{pubkeys, msgs, pubnonces};
    secp256k1_fullagg_session session;
    if (!kp || !group.valid || signer_index >= group.size() || !group.InitSession(session)) return std::nullopt;

    secp256k1_fullagg_partial_sig psig;
    const bool ok{secp256k1_fullagg_partial_sign(GetSecp256k1SignContext(), &psig, secnonce.Get(), kp, msgs[signer_index].data(), &session, group.pubkey_ptrs.data(), group.msg_ptrs.data(), group.pubnonce_ptrs.data(), group.size(), signer_index) != 0};
    // The secnonce must not be used again, whether or not signing succeeded.
    secnonce.Invalidate();
    if (!ok || !group.VerifyPartialSig(psig, signer_index)) return std::nullopt;

    uint256 out;
    secp256k1_fullagg_partial_sig_serialize(secp256k1_context_static, out.data(), &psig);
    return out;
}

bool VerifyFullAggPartialSig(const uint256& partial_sig, const std::vector<XOnlyPubKey>& pubkeys, const std::vector<uint256>& msgs, const std::vector<std::vector<uint8_t>>& pubnonces, size_t signer_index)
{
    const FullAggGroup group{pubkeys, msgs, pubnonces};
    secp256k1_fullagg_partial_sig psig;
    if (!group.valid || signer_index >= group.size() || !secp256k1_fullagg_partial_sig_parse(secp256k1_context_static, &psig, partial_sig.data())) return false;
    return group.VerifyPartialSig(psig, signer_index);
}

std::optional<std::vector<uint8_t>> AggregateFullAggSigs(const std::vector<XOnlyPubKey>& pubkeys, const std::vector<uint256>& msgs, const std::vector<std::vector<uint8_t>>& pubnonces, const std::vector<uint256>& partial_sigs)
{
    const FullAggGroup group{pubkeys, msgs, pubnonces};
    secp256k1_fullagg_session session;
    if (!group.valid || partial_sigs.size() != group.size() || !group.InitSession(session)) return std::nullopt;

    std::vector<secp256k1_fullagg_partial_sig> psigs(group.size());
    std::vector<const secp256k1_fullagg_partial_sig*> psig_ptrs;
    for (size_t i = 0; i < group.size(); i++) {
        if (!secp256k1_fullagg_partial_sig_parse(secp256k1_context_static, &psigs[i], partial_sigs[i].data()) || !group.VerifyPartialSig(psigs[i], i)) return std::nullopt;
        psig_ptrs.push_back(&psigs[i]);
    }

    std::vector<uint8_t> sig(64);
    if (!secp256k1_fullagg_partial_sig_agg(secp256k1_context_static, sig.data(), &session, psig_ptrs.data(), psig_ptrs.size())) return std::nullopt;
    return sig;
}

std::optional<std::vector<uint8_t>> AggregateHalfAggSigs(const std::vector<XOnlyPubKey>& pubkeys, const std::vector<uint256>& msgs, const std::vector<std::vector<uint8_t>>& sigs)
{
    const size_t n{pubkeys.size()};
    if (n == 0 || msgs.size() != n || sigs.size() != n) return std::nullopt;

    std::vector<secp256k1_xonly_pubkey> secp_pubkeys(n);
    std::vector<unsigned char> flat_msgs(n * 32);
    std::vector<unsigned char> flat_sigs(n * 64);
    for (size_t i = 0; i < n; i++) {
        if (sigs[i].size() != 64 || !pubkeys[i].VerifySchnorr(msgs[i], sigs[i])) return std::nullopt;
        if (!secp256k1_xonly_pubkey_parse(secp256k1_context_static, &secp_pubkeys[i], pubkeys[i].data())) return std::nullopt;
        memcpy(flat_msgs.data() + i * 32, msgs[i].begin(), 32);
        memcpy(flat_sigs.data() + i * 64, sigs[i].data(), 64);
    }

    std::vector<uint8_t> aggsig((n + 1) * 32);
    size_t aggsig_len{aggsig.size()};
    if (!secp256k1_schnorrsig_aggregate(secp256k1_context_static, aggsig.data(), &aggsig_len, secp_pubkeys.data(), flat_msgs.data(), flat_sigs.data(), n)) return std::nullopt;
    return aggsig;
}
