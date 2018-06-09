// Copyright (c) 2014-2015 The ShadowCoin developers
// Copyright (c) 2017 The Particl developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key/extkey.h>

#include <key_io.h>
#include <crypto/hmac_sha512.h>

#include <stdint.h>

CCriticalSection cs_extKey;

const char *ExtKeyGetString(int ind)
{
    switch (ind)
    {
        case 2:     return "Path string too long";
        case 3:     return "Path string empty";
        case 4:     return "Integer conversion invalid character";
        case 5:     return "Integer conversion out of range";
        case 7:     return "Malformed path";
        case 8:     return "Offset is hardened already";
        case 9:     return "Can't use BIP44 key as master";
        case 10:    return "Ext key not found in wallet";
        case 11:    return "This key is already the master key";
        case 12:    return "Derived key already exists in wallet";
        case 13:    return "Failed to unlock";
        case 14:    return "Account already exists in db.";
        case 15:    return "Account not found in wallet";
        case 16:    return "Invalid params pointer";
        default:    return "Unknown error, check log";
    };

};

std::vector<uint8_t> &SetCompressedInt64(std::vector<uint8_t> &v, uint64_t n)
{
    int b = GetNumBytesReqForInt(n);
    v.resize(b);
    if (b > 0)
        memcpy(&v[0], (uint8_t*) &n, b);

    return v;
};

int64_t GetCompressedInt64(const std::vector<uint8_t> &v, uint64_t &n)
{
    int b = v.size();
    n = 0;
    if (b < 1)
        n = 0;
    else
    if (b < 9)
        memcpy((uint8_t*) &n, &v[0], b);

    return (int64_t)n;
};

std::vector<uint8_t> &SetCKeyID(std::vector<uint8_t> &v, CKeyID n)
{
    v.resize(20);
    memcpy(&v[0], (uint8_t*) &n, 20);
    return v;
};

bool GetCKeyID(const std::vector<uint8_t> &v, CKeyID &n)
{
    if (v.size() != 20)
        return false;

    memcpy((uint8_t*) &n, &v[0], 20);

    return true;
};

std::vector<uint8_t> &SetString(std::vector<uint8_t> &v, const char *s)
{
    size_t len = strlen(s);
    v.resize(len);
    memcpy(&v[0], (uint8_t*) &s, len);

    return v;
};

std::vector<uint8_t> &SetChar(std::vector<uint8_t> &v, const uint8_t c)
{
    v.resize(1);
    v[0] = c;

    return v;
};

std::vector<uint8_t> &PushUInt32(std::vector<uint8_t> &v, const uint32_t i)
{
    size_t o = v.size();
    v.resize(o+4);

    memcpy(&v[o], (uint8_t*) &i, 4);
    return v;
};

#define _UINT32_MAX  (0xffffffff)
static uint32_t strtou32max(const char *nptr, int base)
{
    const char *s;
    uintmax_t acc;
    char c;
    uintmax_t cutoff;
    int neg, any, cutlim;

    s = nptr;
    do {
        c = *s++;
    } while (isspace((unsigned char)c));

    if (c == '-') {
        neg = 1;
        c = *s++;
    } else {
        neg = 0;
        if (c == '+')
            c = *s++;
    }
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X') &&
        ((s[1] >= '0' && s[1] <= '9') ||
        (s[1] >= 'A' && s[1] <= 'F') ||
        (s[1] >= 'a' && s[1] <= 'f'))) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;
    acc = any = 0;
    if (base < 2 || base > 36)
        goto noconv;

    cutoff = _UINT32_MAX / base;
    cutlim = _UINT32_MAX % base;
    for ( ; ; c = *s++) {
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'A' && c <= 'Z')
            c -= 'A' - 10;
        else if (c >= 'a' && c <= 'z')
            c -= 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
            any = -1;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0) {
        acc = _UINT32_MAX;
        errno = ERANGE;
    } else if (!any) {
noconv:
        errno = EINVAL;
    } else if (neg)
        acc = -acc;
    return (acc);
};

static inline int validDigit(char c, int base)
{
    switch(base)
    {
        case 2:  return c == '0' || c == '1';
        case 10: return c >= '0' && c <= '9';
        case 16: return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        default: errno = EINVAL;
    };
    return 0;
};

int ExtractExtKeyPath(const std::string &sPath, std::vector<uint32_t> &vPath)
{
    char data[512];

    vPath.clear();

    if (sPath.length() > sizeof(data) -2)
        return 2;
    if (sPath.length() < 1)
        return 3;

    size_t nStart = 0;
    size_t nLen = sPath.length();
    if (tolower(sPath[0]) == 'm')
    {
        nStart+=2;
        nLen-=2;
    };
    if (nLen < 1)
        return 3;

    memcpy(data, sPath.data()+nStart, nLen);
    data[nLen] = '\0';

    int nSlashes = 0;
    for (size_t k = 0; k < nLen; ++k)
    {
        if (data[k] == '/')
        {
            nSlashes++;

            // Catch start or end '/', and '//'
            if (k == 0
                || k == nLen-1
                || (k < nLen-1 && data[k+1] == '/'))
                return 7;
        };
    };

    vPath.reserve(nSlashes + 1);

    char *p = strtok(data, "/");

    while (p)
    {
        uint32_t nChild;
        bool fHarden = false;

        // Don't allow octal, only hex and binary
        int nBase = *p == '0' && (*(p+1) == 'b' || *(p+1) == 'B') ? 2
            : *p == '0' && (*(p+1) == 'x' || *(p+1) == 'X') ? 16 : 10;
        if (nBase != 10)
            p += 2; // step over 0b / 0x
        char *ps = p;
        for (; *p; ++p)
        {
            // Last char can be (h, H ,')
            if (!*(p+1) && (tolower(*p) == 'h' || *p == '\''))
            {
                fHarden = true;
                *p = '\0';
            } else
            if (!validDigit(*p, nBase))
                return 4;
        };

        errno = 0;
        nChild = strtou32max(ps, nBase);
        if (errno != 0)
            return 5;

        if (fHarden)
        {
            if ((nChild >> 31) == 0)
            {
                nChild |= 1 << 31;
            } else
            {
                return 8;
            };
        };

        vPath.push_back(nChild);

        p = strtok(nullptr, "/");
    };

    if (vPath.size() < 1)
        return 3;

    return 0;
};

void CExtPubKey::Encode(unsigned char code[74]) const
{
    code[0] = nDepth;
    memcpy(code+1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF; code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >>  8) & 0xFF; code[8] = (nChild >>  0) & 0xFF;
    memcpy(code+9, vchChainCode, 32);
    assert(pubkey.size() == 33);
    memcpy(code+41, pubkey.begin(), 33);
};

void CExtPubKey::Decode(const unsigned char code[74])
{
    nDepth = code[0];
    memcpy(vchFingerprint, code+1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(vchChainCode, code+9, 32);
    pubkey.Set(code+41, code+74);
};

bool CExtPubKey::Derive(CExtPubKey &out, unsigned int nChild) const
{
    out.nDepth = nDepth + 1;
    CKeyID id = pubkey.GetID();

    memcpy(&out.vchFingerprint[0], &id, 4);
    out.nChild = nChild;
    return pubkey.Derive(out.pubkey, out.vchChainCode, nChild, vchChainCode);
};



bool CExtKey::Derive(CExtKey &out, unsigned int nChild) const
{
    out.nDepth = nDepth + 1;
    CKeyID id = key.GetPubKey().GetID();
    memcpy(&out.vchFingerprint[0], &id, 4);
    out.nChild = nChild;
    return key.Derive(out.key, out.vchChainCode, nChild, vchChainCode);
};

void CExtKey::SetSeed(const unsigned char *seed, unsigned int nSeedLen)
{
    static const unsigned char hashkey[] = {'B','i','t','c','o','i','n',' ','s','e','e','d'};

    std::vector<unsigned char, secure_allocator<unsigned char>> vout(64);
    CHMAC_SHA512(hashkey, sizeof(hashkey)).Write(seed, nSeedLen).Finalize(vout.data());

    key.Set(&vout[0], &vout[32], true);
    memcpy(vchChainCode, &vout[32], 32);
    nDepth = 0;
    nChild = 0;
    memset(vchFingerprint, 0, sizeof(vchFingerprint));
};

int CExtKey::SetKeyCode(const unsigned char *pkey, const unsigned char *pcode)
{
    key.Set(pkey, true);
    memcpy(vchChainCode, pcode, 32);
    nDepth = 0;
    nChild = 0;
    memset(vchFingerprint, 0, sizeof(vchFingerprint));
    return 0;
};

CExtPubKey CExtKey::Neutered() const
{
    CExtPubKey ret;
    ret.nDepth = nDepth;
    memcpy(&ret.vchFingerprint[0], &vchFingerprint[0], 4);
    ret.nChild = nChild;
    ret.pubkey = key.GetPubKey();
    memcpy(&ret.vchChainCode[0], &vchChainCode[0], 32);
    return ret;
};

void CExtKey::Encode(unsigned char code[74]) const
{
    code[0] = nDepth;
    memcpy(code+1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF; code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >>  8) & 0xFF; code[8] = (nChild >>  0) & 0xFF;
    memcpy(code+9, vchChainCode, 32);
    code[41] = 0;
    assert(key.size() == 32);
    memcpy(code+42, key.begin(), 32);
};

void CExtKey::Decode(const unsigned char code[74])
{
    nDepth = code[0];
    memcpy(vchFingerprint, code+1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(vchChainCode, code+9, 32);
    key.Set(code+42, code+74, true);
};


void CExtKeyPair::EncodeV(unsigned char code[74]) const
{
    code[0] = nDepth;
    memcpy(code+1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF; code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >>  8) & 0xFF; code[8] = (nChild >>  0) & 0xFF;
    memcpy(code+9, vchChainCode, 32);
    code[41] = 0;
    assert(key.size() == 32);
    memcpy(code+42, key.begin(), 32);
};

void CExtKeyPair::DecodeV(const unsigned char code[74])
{
    nDepth = code[0];
    memcpy(vchFingerprint, code+1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(vchChainCode, code+9, 32);
    key.Set(code+42, code+74, true);
    pubkey = key.GetPubKey();
};

void CExtKeyPair::EncodeP(unsigned char code[74]) const
{
    code[0] = nDepth;
    memcpy(code+1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF; code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >>  8) & 0xFF; code[8] = (nChild >>  0) & 0xFF;
    memcpy(code+9, vchChainCode, 32);
    assert(pubkey.size() == 33);
    memcpy(code+41, pubkey.begin(), 33);
};

void CExtKeyPair::DecodeP(const unsigned char code[74])
{
    nDepth = code[0];
    memcpy(vchFingerprint, code+1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(vchChainCode, code+9, 32);
    pubkey.Set(code+41, code+74);
    key.Clear();
};

bool CExtKeyPair::Derive(CExtKey &out, unsigned int nChild) const
{
    if (!key.IsValid())
        return false;
    out.nDepth = nDepth + 1;
    CKeyID id = key.GetPubKey().GetID();
    memcpy(&out.vchFingerprint[0], &id, 4);
    out.nChild = nChild;
    return key.Derive(out.key, out.vchChainCode, nChild, vchChainCode);
};

bool CExtKeyPair::Derive(CExtPubKey &out, unsigned int nChild) const
{
    if ((nChild >> 31) == 0)
    {
        out.nDepth = nDepth + 1;
        CKeyID id = pubkey.GetID();
        memcpy(&out.vchFingerprint[0], &id, 4);
        out.nChild = nChild;
        return pubkey.Derive(out.pubkey, out.vchChainCode, nChild, vchChainCode);
    };
    if (!key.IsValid())
        return false;

    out.nDepth = nDepth + 1;
    CKeyID id = pubkey.GetID();
    memcpy(&out.vchFingerprint[0], &id, 4);
    out.nChild = nChild;
    CKey tkey;
    if (!key.Derive(tkey, out.vchChainCode, nChild, vchChainCode))
        return false;
    out.pubkey = tkey.GetPubKey();
    return true;
};

bool CExtKeyPair::Derive(CKey &out, unsigned int nChild) const
{
    if (!key.IsValid())
        return false;

    unsigned char temp[32];
    return key.Derive(out, temp, nChild, vchChainCode);
};

bool CExtKeyPair::Derive(CPubKey &out, unsigned int nChild) const
{
    unsigned char temp[32];
    if ((nChild >> 31) == 0)
    {
        return pubkey.Derive(out, temp, nChild, vchChainCode);
    };
    if (!key.IsValid())
        return false;

    CKey tkey;
    if (!key.Derive(tkey, temp, nChild, vchChainCode))
        return false;
    out = tkey.GetPubKey();
    return true;
};

CExtPubKey CExtKeyPair::GetExtPubKey() const
{
    CExtPubKey ret;
    ret.nDepth = nDepth;
    memcpy(&ret.vchFingerprint[0], &vchFingerprint[0], 4);
    ret.nChild = nChild;
    ret.pubkey = pubkey;
    memcpy(&ret.vchChainCode[0], &vchChainCode[0], 32);
    return ret;
};

CExtKeyPair CExtKeyPair::Neutered() const
{
    CExtKeyPair ret;
    ret.nDepth = nDepth;
    memcpy(&ret.vchFingerprint[0], &vchFingerprint[0], 4);
    ret.nChild = nChild;
    ret.pubkey = pubkey;
    ret.key.Clear();
    memcpy(&ret.vchChainCode[0], &vchChainCode[0], 32);
    return ret;
}

void CExtKeyPair::SetSeed(const unsigned char *seed, unsigned int nSeedLen)
{
    static const unsigned char hashkey[] = {'B','i','t','c','o','i','n',' ','s','e','e','d'};

    std::vector<unsigned char, secure_allocator<unsigned char>> vout(64);
    CHMAC_SHA512(hashkey, sizeof(hashkey)).Write(seed, nSeedLen).Finalize(vout.data());

    key.Set(&vout[0], &vout[32], true);
    pubkey = key.GetPubKey();
    memcpy(vchChainCode, &vout[32], 32);
    nDepth = 0;
    nChild = 0;
    memset(vchFingerprint, 0, sizeof(vchFingerprint));
};

int CExtKeyPair::SetKeyCode(const unsigned char *pkey, const unsigned char *pcode)
{
    key.Set(pkey, true);
    pubkey = key.GetPubKey();
    memcpy(vchChainCode, pcode, 32);
    nDepth = 0;
    nChild = 0;
    memset(vchFingerprint, 0, sizeof(vchFingerprint));
    return 0;
};


std::string CEKAStealthKey::ToStealthAddress() const
{
    // Return base58 encoded public stealth address

    CStealthAddress sxAddr;
    SetSxAddr(sxAddr);
    return sxAddr.Encoded();
};


int CEKAStealthKey::SetSxAddr(CStealthAddress &sxAddr) const
{
    sxAddr.prefix.number_bits = nPrefixBits;
    sxAddr.prefix.bitfield = nPrefix;
    sxAddr.scan_pubkey = pkScan;
    sxAddr.spend_pubkey = pkSpend;
    sxAddr.scan_secret.Set(skScan.begin(), true);

    return 0;
};

int CEKAStealthKey::ToRaw(std::vector<uint8_t> &raw) const
{
    // inefficient
    CStealthAddress sxAddr;
    SetSxAddr(sxAddr);
    return sxAddr.ToRaw(raw);
};

std::string CStoredExtKey::GetIDString58() const
{
    return HDKeyIDToString(kp.GetID());
};

int CStoredExtKey::SetPath(const std::vector<uint32_t> &vPath_)
{
    if (vPath_.size() < 1)
        return 1;
    std::vector<uint8_t> vPath(4 * vPath_.size());
    memcpy(vPath.data(), vPath_.data(), vPath.size());
    mapValue[EKVT_PATH] = vPath;
    return 0;
};

std::string CExtKeyAccount::GetIDString58() const
{
    // 0th chain is always account chain
    if (vExtKeyIDs.size() < 1)
        return "Not Set";

    return HDAccIDToString(vExtKeyIDs[0]);
};

int CExtKeyAccount::HaveSavedKey(const CKeyID &id)
{
    LOCK(cs_account);

    AccKeyMap::const_iterator mi = mapKeys.find(id);
    if (mi != mapKeys.end())
        return HK_YES;
    return HK_NO;
};

int CExtKeyAccount::HaveKey(const CKeyID &id, bool fUpdate, const CEKAKey *&pak, const CEKASCKey *&pasc, isminetype &ismine)
{
    // If fUpdate, promote key if found in look ahead
    LOCK(cs_account);

    pasc = nullptr;
    AccKeyMap::const_iterator mi = mapKeys.find(id);
    if (mi != mapKeys.end())
    {
        pak = &mi->second;
        ismine = IsMine(pak->nParent);
        return HK_YES;
    };

    mi = mapLookAhead.find(id);
    if (mi != mapLookAhead.end())
    {
        pak = &mi->second;
        ismine = IsMine(pak->nParent);
        if (LogAcceptCategory(BCLog::HDWALLET))
            LogPrintf("HaveKey in lookAhead %s\n", CBitcoinAddress(mi->first).ToString());
        return fUpdate ? HK_LOOKAHEAD_DO_UPDATE : HK_LOOKAHEAD;
    };

    pak = nullptr;
    return HaveStealthKey(id, pasc, ismine);
};

int CExtKeyAccount::HaveStealthKey(const CKeyID &id, const CEKASCKey *&pasc, isminetype &ismine)
{
    LOCK(cs_account);

    AccKeySCMap::const_iterator miSck = mapStealthChildKeys.find(id);
    if (miSck != mapStealthChildKeys.end())
    {
        pasc = &miSck->second;
        AccStealthKeyMap::const_iterator miSk = mapStealthKeys.find(pasc->idStealthKey);
        if (miSk == mapStealthKeys.end())
            ismine = ISMINE_NO;
        else
            ismine = IsMine(miSk->second.akSpend.nParent);
        return HK_YES;
    };
    ismine = ISMINE_NO;
    return HK_NO;
};

bool CExtKeyAccount::GetKey(const CKeyID &id, CKey &keyOut) const
{
    CEKAKey ak;
    CKeyID idStealth;
    return GetKey(id, keyOut, ak, idStealth);
};

bool CExtKeyAccount::GetKey(const CEKAKey &ak, CKey &keyOut) const
{
    LOCK(cs_account);

    if (ak.nParent >= vExtKeys.size())
        return error("%s: Account key invalid parent ext key %d, account %s.", __func__, ak.nParent, GetIDString58());

    const CStoredExtKey *chain = vExtKeys[ak.nParent];

    if (chain->fLocked)
        return error("%s: Chain locked, account %s.", __func__, GetIDString58());

    if (!chain->kp.Derive(keyOut, ak.nKey))
        return false;

    return true;
};

bool CExtKeyAccount::GetKey(const CEKASCKey &asck, CKey &keyOut) const
{
    LOCK(cs_account);

    AccStealthKeyMap::const_iterator miSk = mapStealthKeys.find(asck.idStealthKey);
    if (miSk == mapStealthKeys.end())
        return error("%s: CEKASCKey Stealth key not found.", __func__);

    return (0 == ExpandStealthChildKey(&miSk->second, asck.sShared, keyOut));
};

int CExtKeyAccount::GetKey(const CKeyID &id, CKey &keyOut, CEKAKey &ak, CKeyID &idStealth) const
{
    LOCK(cs_account);

    int rv = 0;
    AccKeyMap::const_iterator mi;
    AccKeySCMap::const_iterator miSck;
    if ((mi = mapKeys.find(id)) != mapKeys.end())
    {
        ak = mi->second;
        if (!GetKey(ak, keyOut))
            return 0;
        rv = 1;
    } else
    if ((miSck = mapStealthChildKeys.find(id)) != mapStealthChildKeys.end())
    {
        idStealth = miSck->second.idStealthKey;
        if (!GetKey(miSck->second, keyOut))
            return 0;
        rv = 2;
    } else
    {
        return 0;
    };

    // [rm] necessary?
    if (LogAcceptCategory(BCLog::HDWALLET) && keyOut.GetPubKey().GetID() != id)
        return error("Stored key mismatch.");
    return rv;
};

bool CExtKeyAccount::GetPubKey(const CKeyID &id, CPubKey &pkOut) const
{
    LOCK(cs_account);
    AccKeyMap::const_iterator mi;
    AccKeySCMap::const_iterator miSck;
    if ((mi = mapKeys.find(id)) != mapKeys.end())
    {
        if (!GetPubKey(mi->second, pkOut))
            return false;
    } else
    if ((miSck = mapStealthChildKeys.find(id)) != mapStealthChildKeys.end())
    {
        if (!GetPubKey(miSck->second, pkOut))
            return false;
    } else
    {
        return false;
    };

    if (LogAcceptCategory(BCLog::HDWALLET)) // [rm] necessary?
    {
        if (pkOut.GetID() != id)
            return errorN(1, "%s: Extracted public key mismatch.", __func__);
    };

    return true;
};

bool CExtKeyAccount::GetPubKey(const CEKAKey &ak, CPubKey &pkOut) const
{
    LOCK(cs_account);

    if (ak.nParent >= vExtKeys.size())
        return error("%s: Account key invalid parent ext key %d, account %s.", __func__, ak.nParent, GetIDString58());

    const CStoredExtKey *chain = GetChain(ak.nParent);

    if (!chain)
        return error("%s: Chain unknown, account %s.", __func__, GetIDString58());

    if (!chain->kp.Derive(pkOut, ak.nKey))
        return false;

    return true;
};

bool CExtKeyAccount::GetPubKey(const CEKASCKey &asck, CPubKey &pkOut) const
{
    LOCK(cs_account);

    AccStealthKeyMap::const_iterator miSk = mapStealthKeys.find(asck.idStealthKey);
    if (miSk == mapStealthKeys.end())
        return error("%s: CEKASCKey Stealth key not in this account!", __func__);

    return (0 == ExpandStealthChildPubKey(&miSk->second, asck.sShared, pkOut));
};

bool CExtKeyAccount::SaveKey(const CKeyID &id, const CEKAKey &keyIn)
{
    // TODO: rename? this is taking a key from lookahead and saving it
    LOCK(cs_account);
    AccKeyMap::const_iterator mi = mapKeys.find(id);
    if (mi != mapKeys.end())
        return false; // already saved

    if (mapLookAhead.erase(id) != 1)
        LogPrintf("Warning: SaveKey %s key not found in look ahead %s.\n", GetIDString58(), CBitcoinAddress(id).ToString());

    mapKeys[id] = keyIn;


    CStoredExtKey *pc;
    if (!IsHardened(keyIn.nKey)
        && (pc = GetChain(keyIn.nParent)) != nullptr)
    {
        // TODO: gaps?
        if (keyIn.nKey == pc->nGenerated)
            pc->nGenerated++;
        else
        if (keyIn.nKey > pc->nGenerated)
        {
            // Incase keys have been processed out of order, go back and check for received keys
            for (uint32_t i = pc->nGenerated; i <= keyIn.nKey; ++i)
            {
                uint32_t nChildOut = 0;
                CPubKey pk;
                if (0 != pc->DeriveKey(pk, i, nChildOut, false))
                {
                    LogPrintf("%s DeriveKey failed %d.\n", __func__, i);
                    break;
                };

                const CEKAKey *pak = nullptr;
                const CEKASCKey *pasc = nullptr;
                isminetype ismine;
                if (HK_YES != HaveKey(pk.GetID(), false, pak, pasc, ismine))
                    break;

                pc->nGenerated = i;
            };
        };

        if (pc->nFlags & EAF_ACTIVE
            && pc->nFlags & EAF_RECEIVE_ON)
            AddLookAhead(keyIn.nParent, 1);
    };

    if (LogAcceptCategory(BCLog::HDWALLET))
    {
        LogPrintf("Saved key %s %d, %s.\n", GetIDString58(), keyIn.nParent, CBitcoinAddress(id).ToString());

        // Check match
        CStoredExtKey *pa;
        if ((pa = GetChain(keyIn.nParent)) != nullptr)
        {
            if ((keyIn.nKey >> 31) == 1
                && pa->fLocked == 1)
            {
                LogPrintf("Can't check hardened key when wallet locked.\n");
                return true;
            };

            CPubKey pk;
            if (!GetPubKey(keyIn, pk))
                return error("GetPubKey failed.");

            if (pk.GetID() != id)
                return error("Key mismatch!");

            LogPrintf("Key match verified.\n");
        } else
        {
            return error("Unknown chain.");
        };
    };

    return true;
};

bool CExtKeyAccount::SaveKey(const CKeyID &id, const CEKASCKey &keyIn)
{
    LOCK(cs_account);
    AccKeySCMap::const_iterator mi = mapStealthChildKeys.find(id);
    if (mi != mapStealthChildKeys.end())
        return false; // already saved

    AccStealthKeyMap::const_iterator miSk = mapStealthKeys.find(keyIn.idStealthKey);
    if (miSk == mapStealthKeys.end())
        return error("SaveKey(): CEKASCKey Stealth key not in this account!");

    mapStealthChildKeys[id] = keyIn;

    if (LogAcceptCategory(BCLog::HDWALLET))
        LogPrintf("SaveKey(): CEKASCKey %s, %s.\n", GetIDString58(), CBitcoinAddress(id).ToString());

    return true;
};

bool CExtKeyAccount::IsLocked(const CEKAStealthKey &aks)
{
    // TODO: check aks belongs to account??
    CStoredExtKey *pc = GetChain(aks.akSpend.nParent);
    if (pc && !pc->fLocked)
        return false;
    return true;
};


int CExtKeyAccount::AddLookBehind(uint32_t nChain, uint32_t nKeys)
{
    CStoredExtKey *pc = GetChain(nChain);
    if (!pc)
        return errorN(1, "%s: Unknown chain, %d.", __func__, nChain);

    if (LogAcceptCategory(BCLog::HDWALLET))
        LogPrintf("%s: chain %s, keys %d.\n", __func__, pc->GetIDString58(), nKeys);

    AccKeyMap::const_iterator mi;
    uint32_t nChild = pc->nGenerated;
    uint32_t nChildOut = nChild;

    CKeyID keyId;
    CPubKey pk;
    for (uint32_t k = 0; k < nKeys; ++k)
    {
        bool fGotKey = false;

        if (nChild == 0)
        {
            LogPrint(BCLog::HDWALLET, "%s: chain %s, at key 0.\n", __func__, pc->GetIDString58());
            break;
        }

        nChild -=1;

        uint32_t nMaxTries = 32; // TODO: link to lookahead size
        for (uint32_t i = 0; i < nMaxTries; ++i) // nMaxTries > lookahead pool
        {
            if (pc->DeriveKey(pk, nChild, nChildOut, false) != 0)
            {
                LogPrintf("Error: %s - DeriveKey failed, chain %d, child %d.\n", __func__, nChain, nChild);
                nChild = nChildOut-1;
                continue;
            };
            nChild = nChildOut-1;

            keyId = pk.GetID();
            if ((mi = mapKeys.find(keyId)) != mapKeys.end())
            {
                if (LogAcceptCategory(BCLog::HDWALLET))
                    LogPrintf("%s: key exists in map skipping %s.\n", __func__, CBitcoinAddress(keyId).ToString());
                continue;
            };

            if ((mi = mapLookAhead.find(keyId)) != mapLookAhead.end())
                continue;

            fGotKey = true;
            break;
        };

        if (!fGotKey)
        {
            LogPrintf("Error: %s - DeriveKey loop failed, chain %d, child %d.\n", __func__, nChain, nChild);
            continue;
        };

        mapLookAhead[keyId] = CEKAKey(nChain, nChildOut);

        if (LogAcceptCategory(BCLog::HDWALLET))
            LogPrintf("%s: Added %s, look-ahead size %u.\n", __func__, CBitcoinAddress(keyId).ToString(), mapLookAhead.size());
    };

    return 0;
};

int CExtKeyAccount::AddLookAhead(uint32_t nChain, uint32_t nKeys)
{
    // Must start from key 0
    CStoredExtKey *pc = GetChain(nChain);
    if (!pc)
        return errorN(1, "%s: Unknown chain, %d.", __func__, nChain);

    AccKeyMap::const_iterator mi;
    uint32_t nChild = std::max(pc->nGenerated, pc->nLastLookAhead);
    uint32_t nChildOut = nChild;

    if (LogAcceptCategory(BCLog::HDWALLET))
        LogPrintf("%s: chain %s, keys %d, from %d.\n", __func__, pc->GetIDString58(), nKeys, nChildOut);

    CKeyID keyId;
    CPubKey pk;
    for (uint32_t k = 0; k < nKeys; ++k)
    {
        bool fGotKey = false;

        uint32_t nMaxTries = 1000; // TODO: link to lookahead size
        for (uint32_t i = 0; i < nMaxTries; ++i) // nMaxTries > lookahead pool
        {
            if (pc->DeriveKey(pk, nChild, nChildOut, false) != 0)
            {
                LogPrintf("Error: %s - DeriveKey failed, chain %d, child %d.\n", __func__, nChain, nChild);
                nChild = nChildOut+1;
                continue;
            };
            nChild = nChildOut+1;

            keyId = pk.GetID();
            if ((mi = mapKeys.find(keyId)) != mapKeys.end())
            {
                if (LogAcceptCategory(BCLog::HDWALLET))
                    LogPrintf("%s: key exists in map skipping %s.\n", __func__, CBitcoinAddress(keyId).ToString());
                continue;
            };

            if ((mi = mapLookAhead.find(keyId)) != mapLookAhead.end())
                continue;

            fGotKey = true;
            break;
        };

        if (!fGotKey)
        {
            LogPrintf("Error: %s - DeriveKey loop failed, chain %d, child %d.\n", __func__, nChain, nChild);
            continue;
        };

        mapLookAhead[keyId] = CEKAKey(nChain, nChildOut);
        pc->nLastLookAhead = nChildOut;

        if (LogAcceptCategory(BCLog::HDWALLET))
            LogPrintf("%s: Added %s, look-ahead size %u.\n", __func__, CBitcoinAddress(keyId).ToString(), mapLookAhead.size());
    };

    return 0;
};


int CExtKeyAccount::ExpandStealthChildKey(const CEKAStealthKey *aks, const CKey &sShared, CKey &kOut) const
{
    // Derive the secret key of the stealth address and then the secret key out

    LOCK(cs_account);

    if (!aks)
        return errorN(1, "%s: Sanity checks failed.", __func__);

    CKey kSpend;
    if (!GetKey(aks->akSpend, kSpend))
        return errorN(1, "%s: GetKey() failed.", __func__);

    if (StealthSharedToSecretSpend(sShared, kSpend, kOut) != 0)
        return errorN(1, "%s: StealthSharedToSecretSpend() failed.", __func__);

    return 0;
};

int CExtKeyAccount::ExpandStealthChildPubKey(const CEKAStealthKey *aks, const CKey &sShared, CPubKey &pkOut) const
{
    // Works with locked wallet

    LOCK(cs_account);

    if (!aks)
        return errorN(1, "%s: Sanity checks failed.", __func__);

    ec_point pkExtract;

    if (StealthSharedToPublicKey(aks->pkSpend, sShared, pkExtract) != 0)
        return errorN(1, "%s: StealthSharedToPublicKey() failed.", __func__);

    pkOut = CPubKey(pkExtract);

    if (!pkOut.IsValid())
        return errorN(1, "%s: Invalid public key.", __func__);

    return 0;
};

int CExtKeyAccount::WipeEncryption()
{
    std::vector<CStoredExtKey*>::iterator it;
    for (it = vExtKeys.begin(); it != vExtKeys.end(); ++it)
    {
        if (!((*it)->nFlags & EAF_IS_CRYPTED))
            continue;

        if ((*it)->fLocked)
            return errorN(1, "Attempting to undo encryption of a locked key.");

        (*it)->nFlags &= ~EAF_IS_CRYPTED;
        (*it)->vchCryptedSecret.clear();
    };

    return 0;
};


inline void AppendPathLink(std::string &s, uint32_t n, char cH)
{
    s += "/";
    bool fHardened = false;
    if ((n >> 31) == 1)
    {
        n &= ~(1 << 31);
        fHardened = true;
    };
    s += strprintf("%u", n);
    if (fHardened)
        s += cH;
};

int PathToString(const std::vector<uint8_t> &vPath, std::string &sPath, char cH, size_t nStart)
{
    sPath = "";
    if (vPath.size() % 4 != 0)
        return 1;

    sPath = "m";
    for (size_t o = nStart; o < vPath.size(); o+=4)
    {
        uint32_t n;
        memcpy(&n, &vPath[o], 4);
        AppendPathLink(sPath, n, cH);
    };

    return 0;
};

int PathToString(const std::vector<uint32_t> &vPath, std::string &sPath, char cH, size_t nStart)
{
    sPath = "m";
    for (size_t o = nStart; o < vPath.size(); ++o)
        AppendPathLink(sPath, vPath[o], cH);
    return 0;
};

bool IsBIP32(const char *base58)
{
    std::vector<uint8_t> vchBytes;
    if (!DecodeBase58(base58, vchBytes))
        return false;

    if (vchBytes.size() != BIP32_KEY_LEN)
        return false;

    if (0 == memcmp(&vchBytes[0], &Params().Base58Prefix(CChainParams::EXT_SECRET_KEY)[0], 4)
        || 0 == memcmp(&vchBytes[0], &Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY)[0], 4))
        return true;

    if (!VerifyChecksum(vchBytes))
        return false;

    return false;
};

int AppendChainPath(const CStoredExtKey *pc, std::vector<uint32_t> &vPath)
{
    mapEKValue_t::const_iterator mvi = pc->mapValue.find(EKVT_PATH);
    if (mvi == pc->mapValue.end())
        return 1;

    assert(mvi->second.size() % 4 == 0);

    // Path on pc is relative to master key, get path relative to account by starting at 1
    for (size_t i = 4; i < mvi->second.size(); i+=4)
    {
        uint32_t tmp;
        memcpy(&tmp, &mvi->second[i], 4);
        vPath.push_back(tmp);
    };

    return 0;
};

int AppendChainPath(const CStoredExtKey *pc, std::vector<uint8_t> &vPath)
{
    mapEKValue_t::const_iterator mvi = pc->mapValue.find(EKVT_PATH);
    if (mvi == pc->mapValue.end())
        return 1;

    assert(mvi->second.size() % 4 == 0);

    // Path on pc is relative to master key, get path relative to account by starting at 1
    for (size_t i = 4; i < mvi->second.size(); i+=4)
    {
        uint32_t tmp;
        memcpy(&tmp, &mvi->second[i], 4);
        PushUInt32(vPath, tmp);
    };

    return 0;
};

int AppendPath(const CStoredExtKey *pc, std::vector<uint32_t> &vPath)
{
    // Path on account chain is relative to master key
    mapEKValue_t::const_iterator mvi = pc->mapValue.find(EKVT_PATH);
    if (mvi == pc->mapValue.end())
        return 1;

    assert(mvi->second.size() % 4 == 0);
    for (size_t i = 0; i < mvi->second.size(); i+=4)
    {
        uint32_t tmp;
        memcpy(&tmp, &mvi->second[i], 4);
        vPath.push_back(tmp);
    };

    return 0;
};

std::string HDAccIDToString(const CKeyID &id)
{
    CBitcoinAddress addr;
    addr.Set(id, CChainParams::EXT_ACC_HASH);
    return addr.ToString();
};

std::string HDKeyIDToString(const CKeyID &id)
{
    CBitcoinAddress addr;
    addr.Set(id, CChainParams::EXT_KEY_HASH);
    return addr.ToString();
};

std::string GetDefaultAccountPath()
{
    // Return path of default account: 44'/44'/0'
    std::vector<uint32_t> vPath;
    vPath.push_back(WithHardenedBit(44)); // purpose
    vPath.push_back(WithHardenedBit(Params().BIP44ID())); // coin
    vPath.push_back(WithHardenedBit(0)); // account
    std::string rv;
    if (0 == PathToString(vPath, rv, 'h'))
        return rv;
    return "";
};
