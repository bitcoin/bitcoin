// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"

#include "hash.h"
#include "uint256.h"

#include <assert.h>
#include <libbase58.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <string>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

static
bool DecodeBase58_internal(const char *&psz, size_t &len, std::vector<unsigned char>& vch) {
    // Skip leading spaces.
    while (*psz && isspace(*psz))
        psz++;

    // Skip trailing spaces.
    len = strlen(psz);
    while (len && isspace(psz[len-1]))
        --len;
    if (!len)
    {
        vch.clear();
        return true;
    }

    // Allocate enough space in big-endian base256 representation.
    size_t binlen = len * 733 / 1000 + 2;  // log(58) / log(256), rounded up.
    vch.resize(binlen);
    bool rv = b58tobin(vch.data(), &binlen, psz, len);
    if (rv)
        vch.erase(vch.begin(), vch.begin() + (vch.size() - binlen));
    else
        vch.clear();
    return rv;
}

bool DecodeBase58(const char *psz, std::vector<unsigned char>& vch) {
    size_t dummy;
    return DecodeBase58_internal(psz, dummy, vch);
}

std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend) {
    // Allocate enough space in big-endian base58 representation.
    std::vector<char> b58((pend - pbegin) * 138 / 100 + 2);  // log(256) / log(58), rounded up.
    size_t len = b58.size();
    if (!b58enc(b58.data(), &len, pbegin, pend - pbegin))
        return std::string();
    // Translate the result into a string.
    return std::string(b58.data(), len - 1);
}

std::string EncodeBase58(const std::vector<unsigned char>& vch) {
    return EncodeBase58(&vch[0], &vch[0] + vch.size());
}

bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet) {
    return DecodeBase58(str.c_str(), vchRet);
}

static bool my_sha256_impl(void *digest, const void *data, size_t datasz)
{
    CSHA256().Write((unsigned char *)data, datasz).Finalize((unsigned char*)digest);
    return true;
}

std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn) {
    if (vchIn.size() < 1)
        return std::string();
    b58_sha256_impl = my_sha256_impl;
    std::vector<char> b58c((vchIn.size() + 4) * 138 / 100 + 2);
    size_t len = b58c.size();
    if (!b58check_enc(b58c.data(), &len, vchIn[0], &vchIn.data()[1], vchIn.size()-1))
        return std::string();
    return std::string(b58c.data(), len - 1);
}

bool DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet) {
    size_t b58len;
    if (!DecodeBase58_internal(psz, b58len, vchRet))
    {
        vchRet.clear();
        return false;
    }
    b58_sha256_impl = my_sha256_impl;
    if (b58check(vchRet.data(), vchRet.size(), psz, b58len) < 0)
    {
        vchRet.clear();
        return false;
    }
    vchRet.resize(vchRet.size()-4);
    return true;
}

bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet) {
    return DecodeBase58Check(str.c_str(), vchRet);
}

CBase58Data::CBase58Data() {
    vchVersion.clear();
    vchData.clear();
}

void CBase58Data::SetData(const std::vector<unsigned char> &vchVersionIn, const void* pdata, size_t nSize) {
    vchVersion = vchVersionIn;
    vchData.resize(nSize);
    if (!vchData.empty())
        memcpy(&vchData[0], pdata, nSize);
}

void CBase58Data::SetData(const std::vector<unsigned char> &vchVersionIn, const unsigned char *pbegin, const unsigned char *pend) {
    SetData(vchVersionIn, (void*)pbegin, pend - pbegin);
}

bool CBase58Data::SetString(const char* psz, unsigned int nVersionBytes) {
    std::vector<unsigned char> vchTemp;
    bool rc58 = DecodeBase58Check(psz, vchTemp);
    if ((!rc58) || (vchTemp.size() < nVersionBytes)) {
        vchData.clear();
        vchVersion.clear();
        return false;
    }
    vchVersion.assign(vchTemp.begin(), vchTemp.begin() + nVersionBytes);
    vchData.resize(vchTemp.size() - nVersionBytes);
    if (!vchData.empty())
        memcpy(&vchData[0], &vchTemp[nVersionBytes], vchData.size());
    OPENSSL_cleanse(&vchTemp[0], vchData.size());
    return true;
}

bool CBase58Data::SetString(const std::string& str) {
    return SetString(str.c_str());
}

std::string CBase58Data::ToString() const {
    std::vector<unsigned char> vch = vchVersion;
    vch.insert(vch.end(), vchData.begin(), vchData.end());
    return EncodeBase58Check(vch);
}

int CBase58Data::CompareTo(const CBase58Data& b58) const {
    if (vchVersion < b58.vchVersion) return -1;
    if (vchVersion > b58.vchVersion) return  1;
    if (vchData < b58.vchData)   return -1;
    if (vchData > b58.vchData)   return  1;
    return 0;
}

namespace {

    class CBitcoinAddressVisitor : public boost::static_visitor<bool> {
    private:
        CBitcoinAddress *addr;
    public:
        CBitcoinAddressVisitor(CBitcoinAddress *addrIn) : addr(addrIn) { }

        bool operator()(const CKeyID &id) const { return addr->Set(id); }
        bool operator()(const CScriptID &id) const { return addr->Set(id); }
        bool operator()(const CNoDestination &no) const { return false; }
    };

} // anon namespace

bool CBitcoinAddress::Set(const CKeyID &id) {
    SetData(Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS), &id, 20);
    return true;
}

bool CBitcoinAddress::Set(const CScriptID &id) {
    SetData(Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS), &id, 20);
    return true;
}

bool CBitcoinAddress::Set(const CTxDestination &dest) {
    return boost::apply_visitor(CBitcoinAddressVisitor(this), dest);
}

bool CBitcoinAddress::IsValid() const {
    bool fCorrectSize = vchData.size() == 20;
    bool fKnownVersion = vchVersion == Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS) ||
                         vchVersion == Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS);
    return fCorrectSize && fKnownVersion;
}

CTxDestination CBitcoinAddress::Get() const {
    if (!IsValid())
        return CNoDestination();
    uint160 id;
    memcpy(&id, &vchData[0], 20);
    if (vchVersion == Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS))
        return CKeyID(id);
    else if (vchVersion == Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS))
        return CScriptID(id);
    else
        return CNoDestination();
}

bool CBitcoinAddress::GetKeyID(CKeyID &keyID) const {
    if (!IsValid() || vchVersion != Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS))
        return false;
    uint160 id;
    memcpy(&id, &vchData[0], 20);
    keyID = CKeyID(id);
    return true;
}

bool CBitcoinAddress::IsScript() const {
    return IsValid() && vchVersion == Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS);
}

void CBitcoinSecret::SetKey(const CKey& vchSecret) {
    assert(vchSecret.IsValid());
    SetData(Params().Base58Prefix(CChainParams::SECRET_KEY), vchSecret.begin(), vchSecret.size());
    if (vchSecret.IsCompressed())
        vchData.push_back(1);
}

CKey CBitcoinSecret::GetKey() {
    CKey ret;
    ret.Set(&vchData[0], &vchData[32], vchData.size() > 32 && vchData[32] == 1);
    return ret;
}

bool CBitcoinSecret::IsValid() const {
    bool fExpectedFormat = vchData.size() == 32 || (vchData.size() == 33 && vchData[32] == 1);
    bool fCorrectVersion = vchVersion == Params().Base58Prefix(CChainParams::SECRET_KEY);
    return fExpectedFormat && fCorrectVersion;
}

bool CBitcoinSecret::SetString(const char* pszSecret) {
    return CBase58Data::SetString(pszSecret) && IsValid();
}

bool CBitcoinSecret::SetString(const std::string& strSecret) {
    return SetString(strSecret.c_str());
}
