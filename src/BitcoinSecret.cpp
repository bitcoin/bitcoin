// Copyright (c) 2014-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "BitcoinSecret.h"

void CBitcoinSecret::SetKey(const CKey& vchSecret)
{
    assert(vchSecret.IsValid());
    m_data.SetData(Params().Base58Prefix(CChainParams::SECRET_KEY), vchSecret.begin(), vchSecret.size());
    if (vchSecret.IsCompressed())
        m_data.vchData.push_back(1);
}

CKey CBitcoinSecret::GetKey()
{
    CKey ret;
    assert(m_data.vchData.size() >= 32);
    ret.Set(m_data.vchData.begin(), m_data.vchData.begin() + 32, m_data.vchData.size() > 32 && m_data.vchData[32] == 1);
    return ret;
}

bool CBitcoinSecret::IsValid() const
{
    bool fExpectedFormat = m_data.vchData.size() == 32 || (m_data.vchData.size() == 33 && m_data.vchData[32] == 1);
    bool fCorrectVersion = m_data.vchVersion == Params().Base58Prefix(CChainParams::SECRET_KEY);
    return fExpectedFormat && fCorrectVersion;
}

bool CBitcoinSecret::SetBase58string(const base58string& strSecret)
{
    return m_data.SetBase58string(strSecret) && IsValid();
}
