// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "base58.h"

class base58string {
public:
    base58string() {}
    explicit base58string(const std::string& str, unsigned int nVersionBytes = 1) {
        CBase58Data data;
        data._SetString(str.c_str(), nVersionBytes);
        m_string = data._ToString();
    }
    explicit base58string(const char* sz, unsigned int nVersionBytes = 1) {
        CBase58Data data;
        data._SetString(sz, nVersionBytes);
        m_string = data._ToString();
    }

    void SetData(const std::vector<unsigned char>& vchVersionIn, const void* pdata, size_t nSize)
    {
        CBase58Data data;
        data.SetData(vchVersionIn, pdata, nSize);
        m_string = data._ToString();
    }

    void SetData(const std::vector<unsigned char>& vchVersionIn, const unsigned char* pbegin, const unsigned char* pend)
    {
        CBase58Data data;
        data.SetData(vchVersionIn, pbegin, pend);
        m_string = data._ToString();
    }

    const char* c_str() const { return m_string.c_str(); }
    size_t length() const { return m_string.length(); }

    int CompareTo(const base58string& str) const { return m_string.compare(str.m_string); }
    bool operator==(const base58string& str) const { return CompareTo(str) == 0; }
    bool operator<=(const base58string& str) const { return CompareTo(str) <= 0; }
    bool operator>=(const base58string& str) const { return CompareTo(str) >= 0; }
    bool operator< (const base58string& str) const { return CompareTo(str) <  0; }
    bool operator> (const base58string& str) const { return CompareTo(str) >  0; }

private:
    std::string m_string;
};
