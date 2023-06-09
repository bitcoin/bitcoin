// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/keyring.h>

namespace blsct {
bool KeyRing::AddKeyPubKey(const PrivateKey& key, const PublicKey& pubkey)
{
    LOCK(cs_KeyStore);
    mapKeys[pubkey.GetID()] = key;
    return true;
}

bool KeyRing::AddViewKey(const PrivateKey& key, const PublicKey& pubkey)
{
    LOCK(cs_KeyStore);
    viewKey = key;
    viewPublicKey = key.GetPublicKey();
    fViewKeyDefined = true;
    return true;
}

bool KeyRing::AddSpendKey(const PublicKey& pubkey)
{
    LOCK(cs_KeyStore);
    spendPublicKey = pubkey;
    fSpendKeyDefined = true;
    return true;
}

bool KeyRing::HaveKey(const CKeyID& id) const
{
    LOCK(cs_KeyStore);
    return mapKeys.count(id) > 0;
}

bool KeyRing::GetKey(const CKeyID& address, PrivateKey& keyOut) const
{
    LOCK(cs_KeyStore);
    KeyMap::const_iterator mi = mapKeys.find(address);
    if (mi != mapKeys.end()) {
        keyOut = mi->second;
        return true;
    }
    return false;
}
} // namespace blsct
