// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <wallet/descriptor_analyzer.h>

#include <key.h>
#include <pubkey.h>
#include <script/descriptor.h>
#include <script/signingprovider.h>
#include <sync.h>
#include <util/bip32.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <set>
#include <utility>
#include <variant>
#include <vector>

namespace wallet {
namespace {

using KeyMatch = WalletDescriptorAnalysis::KeyMatch;

//! Same key material, ignoring depth, parent fingerprint and child number metadata.
bool SameKey(const CExtPubKey& a, const CExtPubKey& b)
{
    return a.pubkey == b.pubkey && a.chaincode == b.chaincode;
}

bool HasPrivKey(const std::set<DescriptorScriptPubKeyMan*>& spkms, const CKeyID& id)
{
    return std::ranges::any_of(spkms, [&](const auto* spkm) { LOCK(spkm->cs_desc_man); return spkm->HasPrivKey(id); });
}

//! Prefer the most useful answer when several wallet keys are candidates.
KeyMatch Best(KeyMatch a, KeyMatch b)
{
    for (const KeyMatch match : {KeyMatch::MINE, KeyMatch::WATCH_ONLY, KeyMatch::UNKNOWN}) {
        if (a == match || b == match) return match;
    }
    return KeyMatch::NOT_MINE;
}

/** Match a key against the wallet's HD keys: the key is one of them, or
 *  deriving `origin`'s path from the one with its fingerprint gives the key.
 *  `same` tells whether an extended public key is the key. */
template <typename Same>
KeyMatch MatchHDKeys(const CWallet& wallet, const CWallet::HDPubKeyMap& hd_keys, const std::optional<KeyOriginInfo>& origin, Same same)
    EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    KeyMatch result{KeyMatch::NOT_MINE};
    for (const auto& [hd_key, spkms] : hd_keys) {
        const bool has_private{HasPrivKey(spkms, hd_key.pubkey.GetID())};
        if (same(hd_key)) {
            result = Best(result, has_private ? KeyMatch::MINE : KeyMatch::WATCH_ONLY);
            continue;
        }
        if (!origin || origin->fingerprint != hd_key.id_key_fingerprint()) continue;

        std::optional<CExtPubKey> derived;
        if (!HasHardenedDerivation(origin->path)) {
            // Public derivation needs no private key, even when locked.
            derived = hd_key;
            for (const uint32_t step : origin->path) {
                if (!derived->Derive(*derived, step)) {
                    derived.reset();
                    break;
                }
            }
        } else if (!has_private) {
            // Without the private key this candidate can't be derived; unlocking wouldn't help.
            continue;
        } else if (wallet.IsLocked()) {
            result = Best(result, KeyMatch::UNKNOWN);
            continue;
        } else if (const auto xprv{wallet.GetExtKey(hd_key)}) {
            // Decrypts only this candidate's key.
            if (const auto child{DeriveExtKey(*xprv, origin->path)}) derived = child->first.Neuter();
        }
        if (derived && same(*derived)) result = Best(result, has_private ? KeyMatch::MINE : KeyMatch::WATCH_ONLY);
    }
    return result;
}

KeyMatch MatchExtKey(const CWallet& wallet, const CWallet::HDPubKeyMap& hd_keys, const CExtPubKey& key, const std::optional<KeyOriginInfo>& origin)
    EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    return MatchHDKeys(wallet, hd_keys, origin, [&](const CExtPubKey& candidate) { return SameKey(candidate, key); });
}

/** Match a constant key: the wallet stores its private key, or it is derived
 *  from one of the wallet's HD keys, with the origin given in the descriptor
 *  or the one the wallet recorded when it derived the key. */
KeyMatch MatchConstKey(const CWallet& wallet, const CWallet::HDPubKeyMap& hd_keys, const DescriptorKeyInfo::ConstKey& key,
                       const std::optional<KeyOriginInfo>& origin, DescriptorKeyInventory::Role role)
    EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    // Taproot signing uses the x-only key, so either parity is the same key there,
    // even when written as a full key.
    using Role = DescriptorKeyInventory::Role;
    const bool xonly{key.xonly || role == Role::TAPROOT_INTERNAL || role == Role::TAPROOT_OUTPUT || role == Role::TAPSCRIPT};
    const std::vector<CPubKey> pubkeys{xonly ? XOnlyPubKey{key.pubkey}.GetCPubKeys() : std::vector<CPubKey>{key.pubkey}};
    std::vector<KeyOriginInfo> origins;
    if (origin) origins.push_back(*origin);
    for (const auto* spkm : wallet.GetAllScriptPubKeyMans()) {
        const auto* desc_spkm{dynamic_cast<const DescriptorScriptPubKeyMan*>(spkm)};
        if (!desc_spkm) continue;
        for (const CPubKey& pubkey : pubkeys) {
            if (WITH_LOCK(desc_spkm->cs_desc_man, return desc_spkm->HasPrivKey(pubkey.GetID()))) return KeyMatch::MINE;
            if (auto recorded{desc_spkm->GetPubKeyOrigin(pubkey)}) origins.push_back(std::move(*recorded));
        }
    }

    const auto same = [&](const CExtPubKey& candidate) {
        return xonly ? XOnlyPubKey{candidate.pubkey} == XOnlyPubKey{key.pubkey} : candidate.pubkey == key.pubkey;
    };
    KeyMatch result{KeyMatch::NOT_MINE};
    for (const KeyOriginInfo& candidate : origins) result = Best(result, MatchHDKeys(wallet, hd_keys, candidate, same));
    return result;
}

/** The wallet's HD keys, plus those it holds only as musig() participants,
 *  which GetHDPubKeys() leaves out because they have no single key to list. */
CWallet::HDPubKeyMap WalletHDKeys(const CWallet& wallet) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    CWallet::HDPubKeyMap hd_keys{wallet.GetHDPubKeys(CWallet::HDKeyFilter::All)};
    for (auto* spkm : wallet.GetAllScriptPubKeyMans()) {
        auto* desc_spkm{dynamic_cast<DescriptorScriptPubKeyMan*>(spkm)};
        if (!desc_spkm) continue;
        const auto inventory{WITH_LOCK(desc_spkm->cs_desc_man, return desc_spkm->GetWalletDescriptor().descriptor->GetKeyInventory())};
        for (const auto& occurrence : inventory.occurrences) {
            if (occurrence.role != DescriptorKeyInventory::Role::MUSIG_PARTICIPANT) continue;
            if (const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&occurrence.expression.key)}) hd_keys[key->root].insert(desc_spkm);
        }
    }
    return hd_keys;
}

std::optional<CTxDestination> FirstAddress(const Descriptor& descriptor, const SigningProvider& provider)
{
    std::vector<CScript> scripts;
    FlatSigningProvider out;
    if (!descriptor.Expand(/*pos=*/0, provider, scripts, out) || scripts.empty()) return std::nullopt;
    CTxDestination dest;
    if (!ExtractDestination(scripts.front(), dest)) return std::nullopt;
    return dest;
}

} // namespace

std::optional<bool> WalletDescriptorAnalysis::CanSign() const
{
    // MuSig aggregates are never MINE; their participants are matched instead.
    bool unknown{false};
    for (const KeyMatch match : keys) {
        if (match == KeyMatch::MINE) return true;
        unknown |= match == KeyMatch::UNKNOWN;
    }
    if (unknown) return std::nullopt;
    return false;
}

WalletDescriptorAnalysis AnalyzeDescriptor(const CWallet& wallet, std::span<const std::unique_ptr<Descriptor>> descriptors, const SigningProvider& provider)
{
    WalletDescriptorAnalysis result;
    result.analysis = AnalyzeDescriptor(descriptors);
    if (descriptors.empty()) return result;
    result.first_address = FirstAddress(*descriptors.front(), provider);

    LOCK(wallet.cs_wallet);
    const CWallet::HDPubKeyMap hd_keys{WalletHDKeys(wallet)};
    for (const auto& occurrence : result.analysis.policy.keys.occurrences) {
        const auto& node{occurrence.expression};
        if (const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&node.key)}) {
            result.keys.push_back(MatchExtKey(wallet, hd_keys, key->root, node.origin));
        } else if (const auto* key{std::get_if<DescriptorKeyInfo::ConstKey>(&node.key)}) {
            result.keys.push_back(MatchConstKey(wallet, hd_keys, *key, node.origin, occurrence.role));
        } else {
            result.keys.push_back(KeyMatch::NOT_MINE);
        }
    }
    return result;
}

} // namespace wallet
