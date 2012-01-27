// Copyright (c) 2009-2011 Satoshi Nakamoto & Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KEYSTORE_H
#define BITCOIN_KEYSTORE_H

class CKeyStore
{
public:
    std::map<std::vector<unsigned char>, CPrivKey> mapKeys;
    mutable CCriticalSection cs_mapKeys;
    virtual bool AddKey(const CKey& key);
    bool HaveKey(const std::vector<unsigned char> &vchPubKey) const
    {
        return (mapKeys.count(vchPubKey) > 0);
    }
    bool GetPrivKey(const std::vector<unsigned char> &vchPubKey, CPrivKey& keyOut) const
    {
        std::map<std::vector<unsigned char>, CPrivKey>::const_iterator mi = mapKeys.find(vchPubKey);
        if (mi != mapKeys.end())
        {
            keyOut = (*mi).second;
            return true;
        }
        return false;
    }
    std::vector<unsigned char> GenerateNewKey();
};

#endif
