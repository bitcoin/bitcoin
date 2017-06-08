// Copyright (c) 2017 Pieter Wuille
// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BECH32_H
#define BITCOIN_BECH32_H

#include "chainparams.h"
#include "key.h"
#include "key/extkey.h"
#include "key/stealth.h"
#include "pubkey.h"
#include "script/script.h"
#include "script/standard.h"
#include "support/allocators/zeroafterfree.h"

#include <string>
#include <vector>

class CBech32Address
{
protected:
    //! the version byte(s)
    std::vector<unsigned char> vchVersion;

    //! the actually encoded data
    typedef std::vector<unsigned char, zero_after_free_allocator<unsigned char> > vector_uchar;
    vector_uchar vchData;

    void SetData(const std::vector<unsigned char> &vchVersionIn, const void* pdata, size_t nSize);
    void SetData(const std::vector<unsigned char> &vchVersionIn, const unsigned char *pbegin, const unsigned char *pend);

public:
    bool SetString(const char* psz, unsigned int nVersionBytes = 1);
    bool SetString(const std::string& str);
    std::string ToString() const;
    /*
    int CompareTo(const CBech32Address& b58) const;

    bool operator==(const CBech32Address& b58) const { return CompareTo(b58) == 0; }
    bool operator<=(const CBech32Address& b58) const { return CompareTo(b58) <= 0; }
    bool operator>=(const CBech32Address& b58) const { return CompareTo(b58) >= 0; }
    bool operator< (const CBech32Address& b58) const { return CompareTo(b58) <  0; }
    bool operator> (const CBech32Address& b58) const { return CompareTo(b58) >  0; }
    */
    
    bool Set(const CKeyID &id);
    bool Set(const CScriptID &id);
    bool Set(const CKeyID &id, const char *hrp);
    bool Set(const CStealthAddress &sx);
    bool Set(const CExtKeyPair &ek);
    bool Set(const CTxDestination &dest);
    
    bool IsValidStealthAddress() const;
    bool IsValidStealthAddress(const CChainParams &params) const;
    bool IsValid() const;
    bool IsValid(const CChainParams &params) const;
    bool IsValid(const char *hrp) const;

    CBech32Address();
    CBech32Address(const CTxDestination &dest) { Set(dest); };
    CBech32Address(const std::string& strAddress) { SetString(strAddress); };
    CBech32Address(const char* pszAddress) { SetString(pszAddress); };

    CTxDestination Get() const;
    bool GetKeyID(CKeyID &keyID) const;
    bool GetKeyID(CKeyID &keyID, const char *hrp) const;
    bool GetIndexKey(uint160 &hashBytes, int &type) const;
    bool IsScript() const;
}






#endif // BITCOIN_BECH32_H
