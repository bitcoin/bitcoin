// Copyright (c) 2014-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"

#include "hash.h"
#include "uint256.h"
#include "utilstrencodings.h"
#include "addressindex.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <string>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

/** All alphanumeric characters except for "0", "I", "O", and "l" */
static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

bool DecodeBase58(const char* psz, std::vector<unsigned char>& vch)
{
    // Skip leading spaces.
    while (*psz && isspace(*psz))
        psz++;
    // Skip and count leading '1's.
    int zeroes = 0;
    int length = 0;
    while (*psz == '1') {
        zeroes++;
        psz++;
    }
    // Allocate enough space in big-endian base256 representation.
    int size = strlen(psz) * 733 /1000 + 1; // log(58) / log(256), rounded up.
    std::vector<unsigned char> b256(size);
    // Process the characters.
    while (*psz && !isspace(*psz)) {
        // Decode base58 character
        const char* ch = strchr(pszBase58, *psz);
        if (ch == nullptr)
            return false;
        // Apply "b256 = b256 * 58 + ch".
        int carry = ch - pszBase58;
        int i = 0;
        for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        assert(carry == 0);
        length = i;
        psz++;
    }
    // Skip trailing spaces.
    while (isspace(*psz))
        psz++;
    if (*psz != 0)
        return false;
    // Skip leading zeroes in b256.
    std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
    while (it != b256.end() && *it == 0)
        it++;
    // Copy result into output vector.
    vch.reserve(zeroes + (b256.end() - it));
    vch.assign(zeroes, 0x00);
    while (it != b256.end())
        vch.push_back(*(it++));
    return true;
}

std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend)
{
    // Skip & count leading zeroes.
    int zeroes = 0;
    int length = 0;
    while (pbegin != pend && *pbegin == 0) {
        pbegin++;
        zeroes++;
    }
    // Allocate enough space in big-endian base58 representation.
    int size = (pend - pbegin) * 138 / 100 + 1; // log(256) / log(58), rounded up.
    std::vector<unsigned char> b58(size);
    // Process the bytes.
    while (pbegin != pend) {
        int carry = *pbegin;
        int i = 0;
        // Apply "b58 = b58 * 256 + ch".
        for (std::vector<unsigned char>::reverse_iterator it = b58.rbegin(); (carry != 0 || i < length) && (it != b58.rend()); it++, i++) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }

        assert(carry == 0);
        length = i;
        pbegin++;
    }
    // Skip leading zeroes in base58 result.
    std::vector<unsigned char>::iterator it = b58.begin() + (size - length);
    while (it != b58.end() && *it == 0)
        it++;
    // Translate the result into a string.
    std::string str;
    str.reserve(zeroes + (b58.end() - it));
    str.assign(zeroes, '1');
    while (it != b58.end())
        str += pszBase58[*(it++)];
    return str;
}

std::string EncodeBase58(const std::vector<unsigned char>& vch)
{
    return EncodeBase58(vch.data(), vch.data() + vch.size());
}

bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet)
{
    return DecodeBase58(str.c_str(), vchRet);
}

std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn)
{
    // add 4-byte hash check to the end
    std::vector<unsigned char> vch(vchIn);
    uint256 hash = Hash(vch.begin(), vch.end());
    vch.insert(vch.end(), (unsigned char*)&hash, (unsigned char*)&hash + 4);
    return EncodeBase58(vch);
}

bool DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet)
{
    if (!DecodeBase58(psz, vchRet) ||
        (vchRet.size() < 4)) {
        vchRet.clear();
        return false;
    }
    // re-calculate the checksum, ensure it matches the included 4-byte checksum
    uint256 hash = Hash(vchRet.begin(), vchRet.end() - 4);
    if (memcmp(&hash, &vchRet.end()[-4], 4) != 0) {
        vchRet.clear();
        return false;
    }
    vchRet.resize(vchRet.size() - 4);
    return true;
}

bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet)
{
    return DecodeBase58Check(str.c_str(), vchRet);
}

CBase58Data::CBase58Data()
{
    vchVersion.clear();
    vchData.clear();
    fBech32 = false;
}

void CBase58Data::SetData(const std::vector<unsigned char>& vchVersionIn, const void* pdata, size_t nSize)
{
    vchVersion = vchVersionIn;

    fBech32 = pParams() && pParams()->IsBech32Prefix(vchVersionIn);

    vchData.resize(nSize);
    if (!vchData.empty())
        memcpy(vchData.data(), pdata, nSize);
}

void CBase58Data::SetData(const std::vector<unsigned char>& vchVersionIn, const unsigned char* pbegin, const unsigned char* pend)
{
    SetData(vchVersionIn, (void*)pbegin, pend - pbegin);
}

bool CBase58Data::SetString(const char* psz, unsigned int nVersionBytes)
{
    CChainParams::Base58Type prefixType;
    fBech32 = pParams() && pParams()->IsBech32Prefix(psz, strlen(psz), prefixType);
    if (fBech32)
    {
        vchVersion = Params().Bech32Prefix(prefixType);
        std::string s(psz);
        auto ret = bech32::Decode(s);
        if (ret.second.size() == 0)
            return false;
        std::vector<uint8_t> data;
        if (!ConvertBits<5, 8, false>(data, ret.second.begin(), ret.second.end()))
          return false;
        vchData.assign(data.begin(), data.end());
        return true;
    };

    std::vector<unsigned char> vchTemp;
    bool rc58 = DecodeBase58Check(psz, vchTemp);

    if (rc58
        && nVersionBytes != 4
        && vchTemp.size() == BIP32_KEY_N_BYTES + 4) // no point checking smaller keys
    {
        if (0 == memcmp(&vchTemp[0], &Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY)[0], 4))
            nVersionBytes = 4;
        else
        if (0 == memcmp(&vchTemp[0], &Params().Base58Prefix(CChainParams::EXT_SECRET_KEY)[0], 4))
        {
            nVersionBytes = 4;

            // Never display secret in a CBitcoinAddress

            // Length already checked
            vchVersion = Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY);
            CExtKeyPair ekp;
            ekp.DecodeV(&vchTemp[4]);
            vchData.resize(74);
            ekp.EncodeP(&vchData[0]);
            memory_cleanse(&vchTemp[0], vchData.size());
            return true;
        };
    };

    if ((!rc58) || (vchTemp.size() < nVersionBytes)) {
        vchData.clear();
        vchVersion.clear();
        return false;
    }
    vchVersion.assign(vchTemp.begin(), vchTemp.begin() + nVersionBytes);
    vchData.resize(vchTemp.size() - nVersionBytes);
    if (!vchData.empty())
        memcpy(vchData.data(), vchTemp.data() + nVersionBytes, vchData.size());
    memory_cleanse(vchTemp.data(), vchTemp.size());
    return true;
}

bool CBase58Data::SetString(const std::string& str)
{
    return SetString(str.c_str());
}

std::string CBase58Data::ToString() const
{
    if (fBech32)
    {
        std::string sHrp(vchVersion.begin(), vchVersion.end());
        std::vector<uint8_t> data;
        ConvertBits<8, 5, true>(data, vchData.begin(), vchData.end());
        std::string rv = bech32::Encode(sHrp, data);
        if (rv.empty())
          return _("bech32 encode failed.");
        return rv;
    };

    std::vector<unsigned char> vch = vchVersion;
    vch.insert(vch.end(), vchData.begin(), vchData.end());
    return EncodeBase58Check(vch);
}

int CBase58Data::CompareTo(const CBase58Data& b58) const
{
    if (vchVersion < b58.vchVersion)
        return -1;
    if (vchVersion > b58.vchVersion)
        return 1;
    if (vchData < b58.vchData)
        return -1;
    if (vchData > b58.vchData)
        return 1;
    return 0;
}

namespace
{
class CBitcoinAddressVisitor : public boost::static_visitor<bool>
{
private:
    CBitcoinAddress* addr;
    bool fBech32;

public:
    CBitcoinAddressVisitor(CBitcoinAddress* addrIn, bool fBech32_ = false) : addr(addrIn), fBech32(fBech32_) {}

    bool operator()(const CKeyID& id) const { return addr->Set(id, fBech32); }
    bool operator()(const CScriptID& id) const { return addr->Set(id, fBech32); }
    bool operator()(const CExtKeyPair &ek) const { return addr->Set(ek, fBech32); }
    bool operator()(const CStealthAddress &sxAddr) const { return addr->Set(sxAddr, fBech32); }
    bool operator()(const CKeyID256& id) const { return addr->Set(id, fBech32); }
    bool operator()(const CScriptID256& id) const { return addr->Set(id, fBech32); }
    bool operator()(const CNoDestination& no) const { return false; }
};

} // namespace

bool CBitcoinAddress::Set(const CKeyID& id, bool fBech32)
{
    SetData(fBech32 ? Params().Bech32Prefix(CChainParams::PUBKEY_ADDRESS)
        : Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS), &id, 20);
    return true;
}

bool CBitcoinAddress::Set(const CScriptID& id, bool fBech32)
{
    SetData(fBech32 ? Params().Bech32Prefix(CChainParams::SCRIPT_ADDRESS)
        : Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS), &id, 20);
    return true;
}

bool CBitcoinAddress::Set(const CKeyID256 &id, bool fBech32)
{
    SetData(fBech32 ? Params().Bech32Prefix(CChainParams::PUBKEY_ADDRESS_256)
        : Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS_256), &id, 32);
    return true;
};

bool CBitcoinAddress::Set(const CScriptID256 &id, bool fBech32)
{
    SetData(fBech32 ? Params().Bech32Prefix(CChainParams::SCRIPT_ADDRESS_256)
        : Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS_256), &id, 32);
    return true;
};

bool CBitcoinAddress::Set(const CKeyID &id, CChainParams::Base58Type prefix, bool fBech32)
{
    SetData(fBech32 ? Params().Bech32Prefix(prefix) : Params().Base58Prefix(prefix), &id, 20);
    return true;
}

bool CBitcoinAddress::Set(const CStealthAddress &sx, bool fBech32)
{
    std::vector<uint8_t> raw;
    if (0 != sx.ToRaw(raw))
        return false;

    SetData(fBech32 ? Params().Bech32Prefix(CChainParams::STEALTH_ADDRESS)
        : Params().Base58Prefix(CChainParams::STEALTH_ADDRESS), &raw[0], raw.size());
    return true;
};

bool CBitcoinAddress::Set(const CExtKeyPair &ek, bool fBech32)
{
    std::vector<unsigned char> vchVersion;
    uint8_t data[74];

    // Use public key only, should never need to reveal the secret key in an address

    /*
    if (ek.IsValidV())
    {
        vchVersion = Params().Base58Prefix(CChainParams::EXT_SECRET_KEY);
        ek.EncodeV(data);
    } else
    */

    vchVersion = fBech32 ? Params().Bech32Prefix(CChainParams::EXT_PUBLIC_KEY)
        : Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY);
    ek.EncodeP(data);

    SetData(vchVersion, data, 74);
    return true;
};

bool CBitcoinAddress::Set(const CTxDestination& dest, bool fBech32)
{
    return boost::apply_visitor(CBitcoinAddressVisitor(this, fBech32), dest);
}

bool CBitcoinAddress::IsValidStealthAddress() const
{
    return IsValidStealthAddress(Params());
};

bool CBitcoinAddress::IsValidStealthAddress(const CChainParams &params) const
{
    if (vchVersion != params.Base58Prefix(CChainParams::STEALTH_ADDRESS)
        && vchVersion != params.Bech32Prefix(CChainParams::STEALTH_ADDRESS))
        return false;

    if (vchData.size() < MIN_STEALTH_RAW_SIZE)
        return false;

    size_t nPkSpend = vchData[34];

    if (nPkSpend != 1) // TODO: allow multi
        return false;

    size_t nBits = vchData[35 + EC_COMPRESSED_SIZE * nPkSpend + 1];
    if (nBits > 32)
        return false;

    size_t nPrefixBytes = std::ceil((float)nBits / 8.0);

    if (vchData.size() != MIN_STEALTH_RAW_SIZE + EC_COMPRESSED_SIZE * (nPkSpend-1) + nPrefixBytes)
        return false;
    return true;
};

bool CBitcoinAddress::IsValid() const
{
    return IsValid(Params());
}

bool CBitcoinAddress::IsValid(const CChainParams& params) const
{
    if (fBech32)
    {
        CChainParams::Base58Type prefix;
        if (!params.IsBech32Prefix(vchVersion, prefix))
            return false;

        switch (prefix)
        {
            case CChainParams::PUBKEY_ADDRESS:
            case CChainParams::SCRIPT_ADDRESS:
            case CChainParams::EXT_KEY_HASH:
            case CChainParams::EXT_ACC_HASH:
                return vchData.size() == 20;
            case CChainParams::PUBKEY_ADDRESS_256:
            case CChainParams::SCRIPT_ADDRESS_256:
                return vchData.size() == 32;
            case CChainParams::EXT_PUBLIC_KEY:
            case CChainParams::EXT_SECRET_KEY:
                return vchData.size() == BIP32_KEY_N_BYTES;
            case CChainParams::STEALTH_ADDRESS:
                return IsValidStealthAddress(params);
            default:
                return false;
        };

        return false;
    };

    bool fCorrectSize = vchData.size() == 20;
    bool fKnownVersion = vchVersion == params.Base58Prefix(CChainParams::PUBKEY_ADDRESS) ||
                         vchVersion == params.Base58Prefix(CChainParams::SCRIPT_ADDRESS);
    if (fCorrectSize && fKnownVersion)
        return true;

    if (IsValidStealthAddress(params))
        return true;

    if (vchVersion.size() == 4
        && (vchVersion == params.Base58Prefix(CChainParams::EXT_PUBLIC_KEY)
            || vchVersion == params.Base58Prefix(CChainParams::EXT_SECRET_KEY)))
        return vchData.size() == BIP32_KEY_N_BYTES;

    bool fKnownVersion256 = vchVersion == params.Base58Prefix(CChainParams::PUBKEY_ADDRESS_256) ||
                            vchVersion == params.Base58Prefix(CChainParams::SCRIPT_ADDRESS_256);
    if (fKnownVersion256 && vchData.size() == 32)
        return true;
    return false;
}

bool CBitcoinAddress::IsValid(CChainParams::Base58Type prefix) const
{
    if (fBech32)
    {
        CChainParams::Base58Type prefixOut;
        if (!Params().IsBech32Prefix(vchVersion, prefixOut)
            || prefix != prefixOut)
            return false;

        switch (prefix)
        {
            case CChainParams::PUBKEY_ADDRESS:
            case CChainParams::SCRIPT_ADDRESS:
            case CChainParams::EXT_KEY_HASH:
            case CChainParams::EXT_ACC_HASH:
                return vchData.size() == 20;
            case CChainParams::PUBKEY_ADDRESS_256:
            case CChainParams::SCRIPT_ADDRESS_256:
                return vchData.size() == 32;
            case CChainParams::EXT_PUBLIC_KEY:
            case CChainParams::EXT_SECRET_KEY:
                return vchData.size() == BIP32_KEY_N_BYTES;
            case CChainParams::STEALTH_ADDRESS:
                return IsValidStealthAddress(Params());
            default:
                return false;
        };

        return false;
        return false;
    };

    bool fKnownVersion = vchVersion == Params().Base58Prefix(prefix);
    if (prefix == CChainParams::EXT_PUBLIC_KEY
        || prefix == CChainParams::EXT_SECRET_KEY)
        return fKnownVersion && vchData.size() == BIP32_KEY_N_BYTES;

    if (prefix == CChainParams::PUBKEY_ADDRESS_256
        || prefix == CChainParams::SCRIPT_ADDRESS_256)
        return fKnownVersion && vchData.size() == 32;

    bool fCorrectSize = vchData.size() == 20;
    return fCorrectSize && fKnownVersion;
}

CTxDestination CBitcoinAddress::Get() const
{
    if (!IsValid())
        return CNoDestination();
    uint160 id;

    if (vchVersion == Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS)
        || vchVersion == Params().Bech32Prefix(CChainParams::PUBKEY_ADDRESS))
    {
        memcpy(&id, vchData.data(), 20);
        return CKeyID(id);
    } else
    if (vchVersion == Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS)
        || vchVersion == Params().Bech32Prefix(CChainParams::SCRIPT_ADDRESS))
    {
        memcpy(&id, vchData.data(), 20);
        return CScriptID(id);
    } else
    if (vchVersion == Params().Base58Prefix(CChainParams::EXT_SECRET_KEY)
        || vchVersion == Params().Bech32Prefix(CChainParams::EXT_SECRET_KEY))
    {
        CExtKeyPair kp;
        kp.DecodeV(vchData.data());
        return kp;
    } else
    if (vchVersion == Params().Base58Prefix(CChainParams::STEALTH_ADDRESS)
        || vchVersion == Params().Bech32Prefix(CChainParams::STEALTH_ADDRESS))
    {
        CStealthAddress sx;
        if (0 == sx.FromRaw(vchData.data(), vchData.size()))
            return sx;
        return CNoDestination();
    } else
    if (vchVersion == Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY)
        || vchVersion == Params().Bech32Prefix(CChainParams::EXT_PUBLIC_KEY))
    {
        CExtKeyPair kp;
        kp.DecodeP(vchData.data());
        return kp;
    }
    else
    if (vchVersion == Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS_256)
        || vchVersion == Params().Bech32Prefix(CChainParams::PUBKEY_ADDRESS_256))
    {
        return CKeyID256(*((uint256*)vchData.data()));
    } else
    if (vchVersion == Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS_256)
        || vchVersion == Params().Bech32Prefix(CChainParams::SCRIPT_ADDRESS_256))
    {
        //uint256 id;
        //memcpy(&id, vchData.data(), 32);
        //return CScriptID256(*((uint256*)vchData.data()));
    };


    return CNoDestination();
}

bool CBitcoinAddress::GetKeyID(CKeyID& keyID) const
{
    if (!IsValid() || vchVersion != Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS))
        return false;
    uint160 id;
    memcpy(&id, vchData.data(), 20);
    keyID = CKeyID(id);
    return true;
}

bool CBitcoinAddress::GetKeyID(CKeyID256& keyID) const
{
    if (!IsValid() || vchVersion != Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS_256))
        return false;
    uint256 id;
    memcpy(&id, vchData.data(), 32);
    keyID = CKeyID256(id);
    return true;
}

bool CBitcoinAddress::GetKeyID(CKeyID &keyID, CChainParams::Base58Type prefix) const
{
    if (!IsValid(prefix))
        return false;
    uint160 id;
    memcpy(&id, &vchData[0], 20);
    keyID = CKeyID(id);
    return true;
}

bool CBitcoinAddress::GetIndexKey(uint256 &hashBytes, int &type) const
{
    if (!IsValid())
        return false;

    memset(&hashBytes, 0, 32);
    if (vchVersion == Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS))
    {
        memcpy(&hashBytes, &vchData[0], 20);
        type = ADDR_INDT_PUBKEY_ADDRESS;
        return true;
    };

    if (vchVersion == Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS))
    {
        memcpy(&hashBytes, &vchData[0], 20);
        type = ADDR_INDT_SCRIPT_ADDRESS;
        return true;
    };

    if (vchVersion == Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS_256))
    {
        memcpy(&hashBytes, &vchData[0], 32);
        type = ADDR_INDT_PUBKEY_ADDRESS_256;
        return true;
    };

    if (vchVersion == Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS_256))
    {
        memcpy(&hashBytes, &vchData[0], 32);
        type = ADDR_INDT_SCRIPT_ADDRESS_256;
        return true;
    };

    return false;
};

bool CBitcoinAddress::IsScript() const
{
    return IsValid() && vchVersion == Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS);
}

void CBitcoinSecret::SetKey(const CKey& vchSecret)
{
    assert(vchSecret.IsValid());
    SetData(Params().Base58Prefix(CChainParams::SECRET_KEY), vchSecret.begin(), vchSecret.size());
    if (vchSecret.IsCompressed())
        vchData.push_back(1);
}

CKey CBitcoinSecret::GetKey()
{
    CKey ret;
    assert(vchData.size() >= 32);
    ret.Set(vchData.begin(), vchData.begin() + 32, vchData.size() > 32 && vchData[32] == 1);
    return ret;
}

bool CBitcoinSecret::IsValid() const
{
    bool fExpectedFormat = vchData.size() == 32 || (vchData.size() == 33 && vchData[32] == 1);
    bool fCorrectVersion = vchVersion == Params().Base58Prefix(CChainParams::SECRET_KEY);
    return fExpectedFormat && fCorrectVersion;
}

bool CBitcoinSecret::SetString(const char* pszSecret)
{
    return CBase58Data::SetString(pszSecret) && IsValid();
}

bool CBitcoinSecret::SetString(const std::string& strSecret)
{
    return SetString(strSecret.c_str());
}


int CExtKey58::Set58(const char *base58)
{
    std::vector<uint8_t> vchBytes;
    if (!DecodeBase58(base58, vchBytes))
        return 1;

    if (vchBytes.size() != BIP32_KEY_LEN)
        return 2;

    if (!VerifyChecksum(vchBytes))
        return 3;

    const CChainParams *pparams = &Params();
    CChainParams::Base58Type type;
    if (0 == memcmp(&vchBytes[0], &pparams->Base58Prefix(CChainParams::EXT_SECRET_KEY)[0], 4))
        type = CChainParams::EXT_SECRET_KEY;
    else
    if (0 == memcmp(&vchBytes[0], &pparams->Base58Prefix(CChainParams::EXT_PUBLIC_KEY)[0], 4))
        type = CChainParams::EXT_PUBLIC_KEY;
    else
    if (0 == memcmp(&vchBytes[0], &pparams->Base58Prefix(CChainParams::EXT_SECRET_KEY_BTC)[0], 4))
        type = CChainParams::EXT_SECRET_KEY_BTC;
    else
    if (0 == memcmp(&vchBytes[0], &pparams->Base58Prefix(CChainParams::EXT_PUBLIC_KEY_BTC)[0], 4))
        type = CChainParams::EXT_PUBLIC_KEY_BTC;
    else
        return 4;

    SetData(pparams->Base58Prefix(type), &vchBytes[4], &vchBytes[4]+74);
    return 0;
};

int CExtKey58::Set58(const char *base58, CChainParams::Base58Type type, const CChainParams *pparams)
{
    if (!pparams)
        return 16;

    std::vector<uint8_t> vchBytes;
    if (!DecodeBase58(base58, vchBytes))
        return 1;

    if (vchBytes.size() != BIP32_KEY_LEN)
        return 2;

    if (!VerifyChecksum(vchBytes))
        return 3;

    if (0 != memcmp(&vchBytes[0], &pparams->Base58Prefix(type)[0], 4))
        return 4;

    SetData(pparams->Base58Prefix(type), &vchBytes[4], &vchBytes[4]+74);
    return 0;
};

bool CExtKey58::IsValid(CChainParams::Base58Type prefix) const
{
    return vchVersion == Params().Base58Prefix(prefix)
        && vchData.size() == BIP32_KEY_N_BYTES;
};

std::string CExtKey58::ToStringVersion(CChainParams::Base58Type prefix)
{
    vchVersion = Params().Base58Prefix(prefix);
    return ToString();
};
