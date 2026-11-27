// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <wallet/descriptor_analyzer.h>

#include <key.h>
#include <pubkey.h>
#include <script/descriptor.h>
#include <script/keyorigin.h>
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

//! Prefer the most useful answer when several wallet keys are candidates. A
//! locked wallet may still hold the private key, so UNKNOWN beats WATCH_ONLY.
KeyMatch Best(KeyMatch a, KeyMatch b)
{
    for (const KeyMatch match : {KeyMatch::MINE, KeyMatch::UNKNOWN, KeyMatch::WATCH_ONLY}) {
        if (a == match || b == match) return match;
    }
    return KeyMatch::NOT_MINE;
}

//! The wallet's HD keys, and the origins its descriptors give them.
struct HDKeys {
    CWallet::HDPubKeyMap keys;
    //! E.g. an account xprv imported with its master's origin.
    std::vector<std::pair<KeyOriginInfo, CExtPubKey>> origins;
};

/** Match a key against what deriving `path` from the wallet's HD key `hd_key`
 *  gives. `same` tells whether an extended public key is the key. */
template <typename Same>
KeyMatch MatchPath(const CWallet& wallet, const CExtPubKey& hd_key, const std::set<DescriptorScriptPubKeyMan*>& spkms, const std::vector<uint32_t>& path, Same same)
    EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    // Only the descriptors with this key count, as in gethdkeys: the wallet
    // signs with each descriptor's own keys, so a private key held alone in
    // another descriptor can't sign for it.
    const bool has_private{HasPrivKey(spkms, hd_key.pubkey.GetID())};
    std::optional<CExtPubKey> derived;
    if (!HasHardenedDerivation(path)) {
        // Public derivation needs no private key, even when locked.
        derived = hd_key;
        for (const uint32_t step : path) {
            if (!derived->Derive(*derived, step)) return KeyMatch::NOT_MINE;
        }
    } else if (!has_private) {
        // Without the private key this candidate can't be derived; unlocking wouldn't help.
        return KeyMatch::NOT_MINE;
    } else if (wallet.IsLocked()) {
        return KeyMatch::UNKNOWN;
    } else if (const auto xprv{wallet.GetExtKey(hd_key)}) {
        // Decrypts only this candidate's key.
        if (const auto child{DeriveExtKey(*xprv, path)}) derived = child->first.Neuter();
    }
    if (derived && same(*derived)) return has_private ? KeyMatch::MINE : KeyMatch::WATCH_ONLY;
    return KeyMatch::NOT_MINE;
}

/** Match a key against the wallet's HD keys: the key is one of them, or
 *  deriving the rest of `origin`'s path from one of them gives the key. The
 *  path starts at the HD key with the origin's fingerprint, or after an origin
 *  the wallet's descriptors give an HD key. `same` tells whether an extended
 *  public key is the key. */
template <typename Same>
KeyMatch MatchHDKeys(const CWallet& wallet, const HDKeys& hd_keys, const std::optional<KeyOriginInfo>& origin, Same same)
    EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    // The key itself needs no private key, so try that first: deriving a path
    // may decrypt a private key, which a key already MINE doesn't need.
    KeyMatch result{KeyMatch::NOT_MINE};
    for (const auto& [hd_key, spkms] : hd_keys.keys) result = Best(result, MatchPath(wallet, hd_key, spkms, {}, same));
    if (!origin || result == KeyMatch::MINE) return result;
    for (const auto& [hd_key, spkms] : hd_keys.keys) {
        if (origin->fingerprint == hd_key.id_key_fingerprint()) result = Best(result, MatchPath(wallet, hd_key, spkms, origin->path, same));
    }
    if (result == KeyMatch::MINE) return result;
    for (const auto& [hd_origin, hd_key] : hd_keys.origins) {
        const auto& prefix{hd_origin.path};
        if (hd_origin.fingerprint != origin->fingerprint || origin->path.size() < prefix.size() ||
            !std::equal(prefix.begin(), prefix.end(), origin->path.begin())) continue;
        const auto it{hd_keys.keys.find(hd_key)};
        if (it == hd_keys.keys.end()) continue;
        const std::vector<uint32_t> rest(origin->path.begin() + prefix.size(), origin->path.end());
        result = Best(result, MatchPath(wallet, hd_key, it->second, rest, same));
    }
    return result;
}

KeyMatch MatchExtKey(const CWallet& wallet, const HDKeys& hd_keys, const CExtPubKey& key, const std::optional<KeyOriginInfo>& origin)
    EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    return MatchHDKeys(wallet, hd_keys, origin, [&](const CExtPubKey& candidate) { return SameKey(candidate, key); });
}

/** Match a constant key: the wallet stores its private key, or it is derived
 *  from one of the wallet's HD keys, with the origin given in the descriptor
 *  or the one the wallet recorded when it derived the key. A key the wallet
 *  has derived is at least WATCH_ONLY. */
KeyMatch MatchConstKey(const CWallet& wallet, const HDKeys& hd_keys, const DescriptorKeyInfo::ConstKey& key,
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
    bool derived{false};
    for (const auto* spkm : wallet.GetAllScriptPubKeyMans()) {
        const auto* desc_spkm{dynamic_cast<const DescriptorScriptPubKeyMan*>(spkm)};
        if (!desc_spkm) continue;
        for (const CPubKey& pubkey : pubkeys) {
            if (WITH_LOCK(desc_spkm->cs_desc_man, return desc_spkm->HasPrivKey(pubkey.GetID()))) return KeyMatch::MINE;
            if (auto recorded{desc_spkm->GetPubKeyOrigin(pubkey)}) {
                derived = true;
                origins.push_back(std::move(*recorded));
            }
        }
    }

    const auto same = [&](const CExtPubKey& candidate) {
        return xonly ? XOnlyPubKey{candidate.pubkey} == XOnlyPubKey{key.pubkey} : candidate.pubkey == key.pubkey;
    };
    KeyMatch result{derived ? KeyMatch::WATCH_ONLY : KeyMatch::NOT_MINE};
    for (const KeyOriginInfo& candidate : origins) result = Best(result, MatchHDKeys(wallet, hd_keys, candidate, same));
    return result;
}

/** The wallet's HD keys, plus those it holds only as musig() participants,
 *  which GetHDPubKeys() leaves out because they have no single key to list,
 *  and the origins its descriptors give them. */
HDKeys WalletHDKeys(const CWallet& wallet) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    HDKeys hd_keys{.keys = wallet.GetHDPubKeys(CWallet::HDKeyFilter::All), .origins = {}};
    for (auto* spkm : wallet.GetAllScriptPubKeyMans()) {
        auto* desc_spkm{dynamic_cast<DescriptorScriptPubKeyMan*>(spkm)};
        if (!desc_spkm) continue;
        const auto inventory{WITH_LOCK(desc_spkm->cs_desc_man, return desc_spkm->GetWalletDescriptor().descriptor->GetKeyInventory())};
        for (const auto& occurrence : inventory.occurrences) {
            const auto* key{std::get_if<DescriptorKeyInfo::BIP32Key>(&occurrence.expression.key)};
            if (!key) continue;
            if (occurrence.role == DescriptorKeyInventory::Role::MUSIG_PARTICIPANT) hd_keys.keys[key->root].insert(desc_spkm);
            if (const auto& origin{occurrence.expression.origin}) hd_keys.origins.emplace_back(*origin, key->root);
        }
    }
    return hd_keys;
}

std::optional<CTxDestination> FirstAddress(const Descriptor& descriptor, const SigningProvider& provider)
{
    std::vector<CScript> scripts;
    FlatSigningProvider out;
    if (!descriptor.Expand(/*pos=*/0, provider, scripts, out)) return std::nullopt;
    // The first script with an address, as deriveaddresses lists: combo() starts with P2PK, which has none.
    for (const CScript& script : scripts) {
        if (CTxDestination dest; ExtractDestination(script, dest)) return dest;
    }
    return std::nullopt;
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

WalletDescriptorAnalysis AnalyzeDescriptor(const CWallet& wallet, std::span<const std::unique_ptr<Descriptor>> descriptors, const FlatSigningProvider& provider)
{
    WalletDescriptorAnalysis result;
    result.analysis = AnalyzeDescriptor(descriptors);
    result.has_private_keys = !provider.keys.empty();
    if (descriptors.empty()) return result;
    for (const auto& descriptor : descriptors) {
        const auto address{FirstAddress(*descriptor, provider)};
        if (!address) {
            result.first_addresses.clear();
            break;
        }
        result.first_addresses.push_back(*address);
    }

    LOCK(wallet.cs_wallet);
    const HDKeys hd_keys{WalletHDKeys(wallet)};
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
