// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "base58.h"

/**
* A base58-encoded secret key
* ※ rpcdump.cpp でしか使われてないっぽい。CKey を base58 エンコードする用途
*/
class CBitcoinSecret
{
public:
    void SetKey(const CKey& vchSecret);
    CKey GetKey();
    bool IsValid() const;
    bool SetBase58string(const base58string& strSecret);

    CBitcoinSecret(const CKey& vchSecret) { this->SetKey(vchSecret); }
    CBitcoinSecret() {}

public: // ### 仮.
    CBase58Data m_data;
};
