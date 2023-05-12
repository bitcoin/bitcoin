// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/keyring.h>

namespace blsct {
bool KeyRing::AddKeyPubKey(const PrivateKey& key, const PublicKey &pubkey)
{
    LOCK(cs_KeyStore);
    mapKeys[pubkey.GetID()] = key;
    return true;
}

bool KeyRing::AddViewKey(const PrivateKey& key, const PublicKey &pubkey)
{
    LOCK(cs_KeyStore);
    mapKeys[pubkey.GetID()] = key;
    viewKey = key;
    viewPublicKey = key.GetPublicKey();
    return true;
}

bool KeyRing::AddSpendKey(const PublicKey &pubkey)
{
    LOCK(cs_KeyStore);
    spendPublicKey = pubkey;
    return true;
}
}
