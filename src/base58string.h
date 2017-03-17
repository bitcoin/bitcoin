// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

class base58string {
public:
    base58string() {}
    explicit base58string(const std::string& str, unsigned int nVersionBytes = 1) { m_data._SetString(str.c_str(), nVersionBytes); }
    explicit base58string(const char* sz, unsigned int nVersionBytes = 1) { m_data._SetString(sz, nVersionBytes); }

    // std::string ToString() const { return m_data.ToString(); }
    const char* c_str() const { return m_data.ToBase58string().c_str(); }

    int CompareTo(const base58string& str) const { return m_data.CompareTo(str.m_data); }
    bool operator==(const base58string& str) const { return CompareTo(str) == 0; }
    bool operator<=(const base58string& str) const { return CompareTo(str) <= 0; }
    bool operator>=(const base58string& str) const { return CompareTo(str) >= 0; }
    bool operator< (const base58string& str) const { return CompareTo(str) <  0; }
    bool operator> (const base58string& str) const { return CompareTo(str) >  0; }

protected:
    friend class CBase58Data;
private:
    CBase58Data m_data;
};
