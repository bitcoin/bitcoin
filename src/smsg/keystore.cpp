// Copyright (c) 2018 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <smsg/keystore.h>

namespace smsg
{

bool SecMsgKeyStore::AddKey(const CKeyID &idk, SecMsgKey &key)
{
    LOCK(cs_KeyStore);
    key.pubkey = key.key.GetPubKey();
    mapKeys[idk] = key;
    return true;
};

bool SecMsgKeyStore::HaveKey(const CKeyID &idk) const
{
    LOCK(cs_KeyStore);
    return mapKeys.count(idk) > 0;
}

bool SecMsgKeyStore::EraseKey(const CKeyID &idk)
{
    LOCK(cs_KeyStore);
    mapKeys.erase(idk);
    return true;
};

bool SecMsgKeyStore::GetPubKey(const CKeyID &idk, CPubKey &pk)
{
    LOCK(cs_KeyStore);
    std::map<CKeyID, SecMsgKey>::const_iterator it = mapKeys.find(idk);

    if (it != mapKeys.end())
    {
        pk = it->second.pubkey;
        return true;
    };

    return false;
};

bool SecMsgKeyStore::Clear()
{
    LOCK(cs_KeyStore);
    mapKeys.clear();
    return true;
};

} // namespace smsg
