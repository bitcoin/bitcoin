// Copyright (c) 2018 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <smsg/keystore.h>

namespace smsg
{

bool SecMsgKeyStore::AddKey(const CKeyID &idk, const SecMsgKey &key)
{
    LOCK(cs_KeyStore);
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

bool SecMsgKeyStore::Clear()
{
    LOCK(cs_KeyStore);
    mapKeys.clear();
    return true;
};

} // namespace smsg
