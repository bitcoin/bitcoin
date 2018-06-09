// Copyright (c) 2014-2015 The ShadowCoin developers
// Copyright (c) 2017 The Particl developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_KEY_EXTKEY_H
#define PARTICL_KEY_EXTKEY_H

#include <util.h>
#include <key.h>
#include <key/stealth.h>
#include <key/types.h>
#include <key/keyutil.h>
#include <sync.h>
#include <script/ismine.h>

static const uint32_t MAX_DERIVE_TRIES = 16;
static const uint32_t BIP32_KEY_LEN = 82;       // raw, 74 + 4 bytes id + 4 checksum
static const uint32_t BIP32_KEY_N_BYTES = 74;   // raw without id and checksum

static const uint32_t MAX_KEY_PACK_SIZE = 128;
static const uint32_t N_DEFAULT_LOOKAHEAD = 64;

static const uint32_t BIP44_PURPOSE = (((uint32_t)44) | (1 << 31));

typedef std::map<uint8_t, std::vector<uint8_t> > mapEKValue_t;


enum EKAddonValueTypes
{
    EKVT_CREATED_AT             = 1,    // up to 8 bytes of int64_t
    EKVT_KEY_TYPE               = 2,    // 1 uint8 of MainExtKeyTypes
    EKVT_STRING_PAIR            = 3,    // str1 null str2 null
    EKVT_ROOT_ID                = 4,    // packed keyid of the root key in the path eg: for key of path m/44'/44'/0, EKVT_ROOT_ID is the id of m
    EKVT_PATH                   = 5,    // pack 4bytes no separators
    EKVT_ADDED_SECRET_AT        = 6,
    EKVT_N_LOOKAHEAD            = 7,
    EKVT_INDEX                  = 8,    // 4byte index to full identifier in local wallet db
    EKVT_CONFIDENTIAL_CHAIN     = 9,
    EKVT_HARDWARE_DEVICE        = 10,   // 4bytes nVendorId, 4bytes nProductId
    EKVT_STEALTH_SCAN_CHAIN     = 11,
    EKVT_STEALTH_SPEND_CHAIN    = 12,
};

extern CCriticalSection cs_extKey;

enum MainExtKeyTypes
{
    EKT_MASTER,
    EKT_BIP44_MASTER, // display with btc prefix (xprv)
    EKT_INTERNAL,
    EKT_EXTERNAL,
    EKT_STEALTH,      // legacy v1 stealth addresses
    EKT_CONFIDENTIAL,
    EKT_STEALTH_SCAN,
    EKT_STEALTH_SPEND,
    EKT_MAX_TYPES,
};

enum AccountFlagTypes
{
    EAF_ACTIVE           = (1 << 0),
    EAF_HAVE_SECRET      = (1 << 1),
    EAF_IS_CRYPTED       = (1 << 2),
    EAF_RECEIVE_ON       = (1 << 3), // CStoredExtKey with this flag set generate look ahead keys
    EAF_IN_ACCOUNT       = (1 << 4), // CStoredExtKey is part of an account
    EAF_HARDWARE_DEVICE  = (1 << 5), // Have private key in hardware device.
};

enum {HK_NO = 0, HK_YES, HK_LOOKAHEAD, HK_LOOKAHEAD_DO_UPDATE};

struct CExtPubKey {
    unsigned char nDepth;
    unsigned char vchFingerprint[4];
    unsigned int nChild;
    unsigned char vchChainCode[32];
    CPubKey pubkey;

    friend bool operator==(const CExtPubKey &a, const CExtPubKey &b) {
        return a.nDepth == b.nDepth && memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0], 4) == 0 && a.nChild == b.nChild &&
               memcmp(&a.vchChainCode[0], &b.vchChainCode[0], 32) == 0 && a.pubkey == b.pubkey;
    }

    bool IsValid() const { return pubkey.IsValid(); }

    CKeyID GetID() const {
        return pubkey.GetID();
    }

    void Encode(unsigned char code[74]) const;
    void Decode(const unsigned char code[74]);
    bool Derive(CExtPubKey &out, unsigned int nChild) const;

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s.write((char*)&nDepth, 1);
        s.write((char*)vchFingerprint, 4);
        s.write((char*)&nChild, 4);
        s.write((char*)vchChainCode, 32);

        pubkey.Serialize(s);
    }
    template<typename Stream>
    void Unserialize(Stream &s)
    {
        s.read((char*)&nDepth, 1);
        s.read((char*)vchFingerprint, 4);
        s.read((char*)&nChild, 4);
        s.read((char*)vchChainCode, 32);

        pubkey.Unserialize(s);
    }
};

struct CExtKey {
    unsigned char nDepth;
    unsigned char vchFingerprint[4];
    unsigned int nChild;
    unsigned char vchChainCode[32];
    CKey key;

    friend bool operator==(const CExtKey &a, const CExtKey &b) {
        return a.nDepth == b.nDepth && memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0], 4) == 0 && a.nChild == b.nChild &&
               memcmp(&a.vchChainCode[0], &b.vchChainCode[0], 32) == 0 && a.key == b.key;
    }

    bool IsValid() const { return key.IsValid(); }

    void Encode(unsigned char code[74]) const;
    void Decode(const unsigned char code[74]);
    bool Derive(CExtKey &out, unsigned int nChild) const;
    CExtPubKey Neutered() const;
    void SetSeed(const unsigned char *seed, unsigned int nSeedLen);
    int SetKeyCode(const unsigned char *pkey, const unsigned char *pcode);

    size_t GetSerializeSize(int nType, int nVersion) const
    {
        return 42 + (key.IsValid() ? 32 : 0);
    }

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s.write((char*)&nDepth, 1);
        s.write((char*)vchFingerprint, 4);
        s.write((char*)&nChild, 4);
        s.write((char*)vchChainCode, 32);

        char fValid = key.IsValid();
        s.write((char*)&fValid, 1);
        if (fValid)
            s.write((char*)key.begin(), 32);
    }
    template<typename Stream>
    void Unserialize(Stream &s)
    {
        s.read((char*)&nDepth, 1);
        s.read((char*)vchFingerprint, 4);
        s.read((char*)&nChild, 4);
        s.read((char*)vchChainCode, 32);

        char tmp[33];
        s.read((char*)tmp, 1); // key.IsValid()
        if (tmp[0])
        {
            s.read((char*)tmp+1, 32);
            key.Set((uint8_t*)tmp+1, 1);
        };
    };
};

class CExtKeyPair
{
public:
    //uint8_t nFlags; ? crypted
    uint8_t nDepth;
    uint8_t vchFingerprint[4];
    uint32_t nChild;
    uint8_t vchChainCode[32];
    CKey key;
    CPubKey pubkey;

    CExtKeyPair() {};
    CExtKeyPair(CExtKey &vk)
    {
        nDepth = vk.nDepth;
        memcpy(vchFingerprint, vk.vchFingerprint, sizeof(vchFingerprint));
        nChild = vk.nChild;
        memcpy(vchChainCode, vk.vchChainCode, sizeof(vchChainCode));
        key = vk.key;
        pubkey = key.GetPubKey();
    };

    CExtKeyPair(CExtPubKey &pk)
    {
        nDepth = pk.nDepth;
        memcpy(vchFingerprint, pk.vchFingerprint, sizeof(vchFingerprint));
        nChild = pk.nChild;
        memcpy(vchChainCode, pk.vchChainCode, sizeof(vchChainCode));
        key.Clear();
        pubkey = pk.pubkey;
    };

    CExtKey GetExtKey() const
    {
        CExtKey vk;
        vk.nDepth = nDepth;
        memcpy(vk.vchFingerprint, vchFingerprint, sizeof(vchFingerprint));
        vk.nChild = nChild;
        memcpy(vk.vchChainCode, vchChainCode, sizeof(vchChainCode));
        vk.key = key;
        return vk;
    };

    CKeyID GetID() const {
        return pubkey.GetID();
    };

    friend bool operator==(const CExtKeyPair &a, const CExtKeyPair &b)
    {
        return a.nDepth == b.nDepth && memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0], 4) == 0 && a.nChild == b.nChild &&
               memcmp(&a.vchChainCode[0], &b.vchChainCode[0], 32) == 0 && a.key == b.key && a.pubkey == b.pubkey ;
    }

    friend bool operator < (const CExtKeyPair &a, const CExtKeyPair &b)
    {
        return a.nDepth < b.nDepth || memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0], 4) < 0 || a.nChild < b.nChild
            || memcmp(&a.vchChainCode[0], &b.vchChainCode[0], 32) < 0 || a.key < b.key || a.pubkey < b.pubkey ;
    }

    bool IsValidV() const { return key.IsValid(); }
    bool IsValidP() const { return pubkey.IsValid(); }

    void EncodeV(unsigned char code[74]) const;
    void DecodeV(const unsigned char code[74]);

    void EncodeP(unsigned char code[74]) const;
    void DecodeP(const unsigned char code[74]);

    bool Derive(CExtKey &out, unsigned int nChild) const;
    bool Derive(CExtPubKey &out, unsigned int nChild) const;
    bool Derive(CKey &out, unsigned int nChild) const;
    bool Derive(CPubKey &out, unsigned int nChild) const;

    CExtPubKey GetExtPubKey() const;
    CExtKeyPair Neutered() const;
    void SetSeed(const unsigned char *seed, unsigned int nSeedLen);
    int SetKeyCode(const unsigned char *pkey, const unsigned char *pcode);

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s.write((char*)&nDepth, 1);
        s.write((char*)vchFingerprint, 4);
        s.write((char*)&nChild, 4);
        s.write((char*)vchChainCode, 32);

        char fValid = key.IsValid();
        s.write((char*)&fValid, 1);
        if (fValid)
            s.write((char*)key.begin(), 32);

        pubkey.Serialize(s);
    }
    template<typename Stream>
    void Unserialize(Stream &s)
    {
        s.read((char*)&nDepth, 1);
        s.read((char*)vchFingerprint, 4);
        s.read((char*)&nChild, 4);
        s.read((char*)vchChainCode, 32);

        char tmp[33];
        s.read((char*)tmp, 1); // key.IsValid()
        if (tmp[0])
        {
            s.read((char*)tmp+1, 32);
            key.Set((uint8_t*)tmp+1, 1);
        } else
        {
            key.Clear();
        };
        pubkey.Unserialize(s);
    }
};

class CStoredExtKey
{
public:
    CStoredExtKey()
    {
        fLocked = 0;
        nFlags = 0;
        nGenerated = 0;
        nHGenerated = 0;
        nLastLookAhead = 0;
    };

    std::string GetIDString58() const;

    CKeyID GetID() const
    {
        return kp.GetID();
    };

    bool operator <(const CStoredExtKey& y) const
    {
        return kp < y.kp;
    };

    bool operator ==(const CStoredExtKey& y) const
    {
        // Compare pubkeys instead of CExtKeyPair for speed
        return kp.pubkey == y.kp.pubkey;
    };

    template<typename T>
    int DeriveKey(T &keyOut, uint32_t nChildIn, uint32_t &nChildOut, bool fHardened = false) const
    {
        if (fHardened && !kp.IsValidV())
            return errorN(1, "Ext key does not contain a secret.");

        for (uint32_t i = 0; i < MAX_DERIVE_TRIES; ++i)
        {
            if ((nChildIn >> 31) == 1)
            {
                // TODO: auto spawn new master key
                return errorN(1, "No more %skeys can be derived from master.", fHardened ? "hardened " : "");
            };

            uint32_t nNum = fHardened ? nChildIn | 1 << 31 : nChildIn;

            if (kp.Derive(keyOut, nNum))
            {
                nChildOut = nNum; // nChildOut has bit 31 set for harnened keys
                return 0;
            };

            nChildIn++;
        };
        return 1;
    };

    template<typename T>
    int DeriveNextKey(T &keyOut, uint32_t &nChildOut, bool fHardened = false, bool fUpdate = true)
    {
        uint32_t nChild = fHardened ? nHGenerated : nGenerated;
        nChildOut = 0; // Silence compiler warning, uninitialised
        int rv;
        if ((rv = DeriveKey(keyOut, nChild, nChildOut, fHardened)) != 0)
            return rv;

        nChild = nChildOut & ~(1 << 31); // Clear the hardened bit
        if (fUpdate)
            SetCounter(nChild+1, fHardened);

        return 0;
    };

    int SetCounter(uint32_t nC, bool fHardened)
    {
        if (fHardened)
            nHGenerated = nC;
        else
            nGenerated = nC;
        return 0;
    };

    uint32_t GetCounter(bool fHardened) const
    {
        return fHardened ? nHGenerated : nGenerated;
    };

    int SetPath(const std::vector<uint32_t> &vPath_);

    isminetype IsMine() const
    {
        if (kp.key.IsValid() || (nFlags & EAF_IS_CRYPTED))
            return ISMINE_SPENDABLE;
        if ((nFlags & EAF_HARDWARE_DEVICE))
            return (isminetype)((int)ISMINE_SPENDABLE | (int)ISMINE_HARDWARE_DEVICE);
        return ISMINE_WATCH_ONLY_;
    };

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        // Never save secret data when key is encrypted
        if (vchCryptedSecret.size() > 0)
        {
            CExtKeyPair kpt = kp.Neutered();
            s << kpt;
        } else
        {
            s << kp;
        };

        s << vchCryptedSecret;
        s << sLabel;
        s << nFlags;
        s << nGenerated;
        s << nHGenerated;
        s << mapValue;
    };
    template <typename Stream>
    void Unserialize(Stream &s)
    {
        s >> kp;

        s >> vchCryptedSecret;
        s >> sLabel;
        s >> nFlags;
        s >> nGenerated;
        s >> nHGenerated;
        s >> mapValue;
    };

    // When encrypted, pk can't be derived from vk
    CExtKeyPair kp;
    std::vector<uint8_t> vchCryptedSecret;

    std::string sLabel;

    uint8_t fLocked; // not part of nFlags so not saved
    uint32_t nFlags;
    uint32_t nGenerated;
    uint32_t nHGenerated;
    uint32_t nLastLookAhead; // in memory only

    mapEKValue_t mapValue;
};


class CEKAKey
{
public:
    CEKAKey() : nParent(0), nKey(0) {};
    CEKAKey(uint32_t nParent_, uint32_t nKey_) : nParent(nParent_), nKey(nKey_) {};

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << nParent;
        s << nKey;
        s << sLabel;
    };
    template <typename Stream>
    void Unserialize(Stream &s)
    {
        s >> nParent;
        s >> nKey;
        s >> sLabel;
    };

    uint32_t nParent; // chain identifier, vExtKeys
    uint32_t nKey;

    //uint32_t nChecksum; // TODO: is it worth storing 4 bytes of the id (160 hash here)
    std::string sLabel; // TODO: remove
};


class CEKASCKey
{
// Key derived from stealth key
public:
    CEKASCKey() {};
    CEKASCKey(CKeyID &idStealthKey_, CKey &sShared_) : idStealthKey(idStealthKey_), sShared(sShared_) {};

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << idStealthKey;
        s.write((char*)sShared.begin(), EC_SECRET_SIZE);
        s << sLabel;
    };
    template <typename Stream>
    void Unserialize(Stream &s)
    {
        s >> idStealthKey;
        s.read((char*)sShared.begin(), EC_SECRET_SIZE);
        sShared.SetFlags(true, true);
        s >> sLabel;
    };
    // TODO: store an offset instead of the full id of the stealth address
    CKeyID idStealthKey; // id of parent stealth key (received on)
    CKey sShared;

    //uint32_t nChecksum; // TODO: is it worth storing 4 bytes of the id (160 hash here)
    std::string sLabel; // TODO: remove
};

class CEKAStealthKey
{
public:
    CEKAStealthKey() {};
    CEKAStealthKey(uint32_t nScanParent_, uint32_t nScanKey_, const CKey &scanSecret_,
        uint32_t nSpendParent_, uint32_t nSpendKey_, const CPubKey &pkSpendSecret,
        uint8_t nPrefixBits_, uint32_t nPrefix_)
    {
        // Spend secret is not stored
        nFlags = 0;
        nScanParent = nScanParent_;
        nScanKey = nScanKey_;
        skScan = scanSecret_;
        CPubKey pk = skScan.GetPubKey();
        pkScan.resize(pk.size());
        memcpy(&pkScan[0], pk.begin(), pk.size());

        akSpend = CEKAKey(nSpendParent_, nSpendKey_);
        pk = pkSpendSecret;
        pkSpend.resize(pk.size());
        memcpy(&pkSpend[0], pk.begin(), pk.size());

        nPrefixBits = nPrefixBits_;
        nPrefix = nPrefix_;
    };

    std::string ToStealthAddress() const;
    int SetSxAddr(CStealthAddress &sxAddr) const;

    int ToRaw(std::vector<uint8_t> &raw) const;

    CKeyID GetID() const
    {
        // Not likely to be called very often
        return skScan.GetPubKey().GetID();
    };

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << nFlags;
        s << sLabel;
        s << nScanParent;
        s << nScanKey;
        s << skScan;
        s << akSpend;
        s << pkScan;
        s << pkSpend;
        s << nPrefixBits;
        s << nPrefix;
    };
    template <typename Stream>
    void Unserialize(Stream &s)
    {
        s >> nFlags;
        s >> sLabel;
        s >> nScanParent;
        s >> nScanKey;
        s >> skScan;
        s >> akSpend;
        s >> pkScan;
        s >> pkSpend;
        s >> nPrefixBits;
        s >> nPrefix;
    };

    uint8_t nFlags; // options of CStealthAddress
    std::string sLabel;
    uint32_t nScanParent; // vExtKeys
    uint32_t nScanKey;
    CKey skScan;
    CEKAKey akSpend;

    ec_point pkScan;
    ec_point pkSpend;

    uint8_t nPrefixBits;
    uint32_t nPrefix;
};

class CEKAKeyPack
{
public:
    CEKAKeyPack() {};
    CEKAKeyPack(CKeyID id_, const CEKAKey &ak_) : id(id_), ak(ak_) {};

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << id;
        s << ak;
    };
    template <typename Stream>
    void Unserialize(Stream &s)
    {
        s >> id;
        s >> ak;
    };

    CKeyID id;
    CEKAKey ak;
};

class CEKASCKeyPack
{
public:
    CEKASCKeyPack() {};
    CEKASCKeyPack(CKeyID id_, const CEKASCKey &asck_) : id(id_), asck(asck_) {};

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << id;
        s << asck;
    };
    template <typename Stream>
    void Unserialize(Stream &s)
    {
        s >> id;
        s >> asck;
    };

    CKeyID id;
    CEKASCKey asck;
};

class CEKAStealthKeyPack
{
public:
    CEKAStealthKeyPack() {};
    CEKAStealthKeyPack(CKeyID id_, const CEKAStealthKey &aks_) : id(id_), aks(aks_) {};

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << id;
        s << aks;
    };
    template <typename Stream>
    void Unserialize(Stream &s)
    {
        s >> id;
        s >> aks;
    };

    CKeyID id;
    CEKAStealthKey aks;
};


typedef std::map<CKeyID, CEKAKey> AccKeyMap;
typedef std::map<CKeyID, CEKASCKey> AccKeySCMap;
typedef std::map<CKeyID, CEKAStealthKey> AccStealthKeyMap;

class CExtKeyAccount
{ // stored by idAccount
public:
    CExtKeyAccount()
    {
        nActiveExternal = 0xFFFFFFFF;
        nActiveInternal = 0xFFFFFFFF;
        nActiveStealth = 0xFFFFFFFF;
        nHeightCheckedUncrypted = 0;
        nFlags = 0;
        nPack = 0;
        nPackStealth = 0;
        nPackStealthKeys = 0;
    };

    int FreeChains()
    {
        // Keys are normally freed by the wallet
        std::vector<CStoredExtKey*>::iterator it;
        for (it = vExtKeys.begin(); it != vExtKeys.end(); ++it)
        {
            delete *it;
            *it = nullptr;
        };
        return 0;
    };

    std::string GetIDString58() const;

    CKeyID GetID() const
    {
        if (vExtKeyIDs.size() < 1)
            return CKeyID(); // CKeyID inits to 0
        return vExtKeyIDs[0];
    };

    int HaveSavedKey(const CKeyID &id);
    int HaveKey(const CKeyID &id, bool fUpdate, const CEKAKey *&pak, const CEKASCKey *&pasc, isminetype &ismine);
    int HaveStealthKey(const CKeyID &id, const CEKASCKey *&pasc, isminetype &ismine);
    bool GetKey(const CKeyID &id, CKey &keyOut) const;
    bool GetKey(const CEKAKey &ak, CKey &keyOut) const;
    bool GetKey(const CEKASCKey &asck, CKey &keyOut) const;

    int GetKey(const CKeyID &id, CKey &keyOut, CEKAKey &ak, CKeyID &idStealth) const; // retuurns 1: chain, 2: stealth address


    bool GetPubKey(const CKeyID &id, CPubKey &pkOut) const;
    bool GetPubKey(const CEKAKey &ak, CPubKey &pkOut) const;
    bool GetPubKey(const CEKASCKey &asck, CPubKey &pkOut) const;

    bool SaveKey(const CKeyID &id, const CEKAKey &keyIn);
    bool SaveKey(const CKeyID &id, const CEKASCKey &keyIn);

    bool IsLocked(const CEKAStealthKey &aks);

    bool GetChainNum(CStoredExtKey *p, uint32_t &nChain) const
    {
        for (size_t i = 0; i < vExtKeys.size(); ++i)
        {
            if (vExtKeys[i] != p)
                continue;
            nChain = i;
            return true;
        };
        return false;
    };

    CStoredExtKey *GetChain(uint32_t nChain) const
    {
        if (nChain >= vExtKeys.size())
            return nullptr;
        return vExtKeys[nChain];
    };

    CStoredExtKey *ChainExternal()
    {
        return GetChain(nActiveExternal);
    };

    CStoredExtKey *ChainInternal()
    {
        return GetChain(nActiveInternal);
    };

    CStoredExtKey *ChainStealth()
    {
        return GetChain(nActiveStealth);
    };

    CStoredExtKey *ChainAccount()
    {
        return vExtKeys.size() < 1 ? nullptr : vExtKeys[0];
    };

    const CStoredExtKey *ChainAccount() const
    {
        return vExtKeys.size() < 1 ? nullptr : vExtKeys[0];
    };

    void InsertChain(CStoredExtKey *sekChain)
    {
        vExtKeyIDs.push_back(sekChain->GetID());
        vExtKeys.push_back(sekChain);
    };

    size_t NumChains() const
    {
        if (vExtKeys.size() < 1) // vExtKeys[0] is account key
            return 0;
        return vExtKeys.size() - 1;
    };

    isminetype IsMine(uint32_t nChain) const
    {
        CStoredExtKey *p = GetChain(nChain);
        return p ? p->IsMine() : ISMINE_NO;
    };

    int AddLookBehind(uint32_t nChain, uint32_t nKeys);
    int AddLookAhead(uint32_t nChain, uint32_t nKeys);

    int AddLookAheadInternal(uint32_t nKeys)
    {
        return AddLookAhead(nActiveInternal, nKeys);
    };

    int AddLookAheadExternal(uint32_t nKeys)
    {
        return AddLookAhead(nActiveExternal, nKeys);
    };

    int ExpandStealthChildKey(const CEKAStealthKey *aks, const CKey &sShared, CKey &kOut) const;
    int ExpandStealthChildPubKey(const CEKAStealthKey *aks, const CKey &sShared, CPubKey &pkOut) const;

    int WipeEncryption();

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << sLabel;
        s << idMaster;

        s << nActiveExternal;
        s << nActiveInternal;
        s << nActiveStealth;

        s << vExtKeyIDs;
        s << nHeightCheckedUncrypted;
        s << nFlags;
        s << nPack;
        s << nPackStealth;
        s << nPackStealthKeys;
        s << mapValue;
    };
    template <typename Stream>
    void Unserialize(Stream &s)
    {
        s >> sLabel;
        s >> idMaster;

        s >> nActiveExternal;
        s >> nActiveInternal;
        s >> nActiveStealth;

        s >> vExtKeyIDs;
        s >> nHeightCheckedUncrypted;
        s >> nFlags;
        s >> nPack;
        s >> nPackStealth;
        s >> nPackStealthKeys;
        s >> mapValue;
    };

    // TODO: Could store used keys in archived packs, which don't get loaded into memory
    AccKeyMap mapKeys;
    AccKeyMap mapLookAhead;

    AccKeySCMap mapStealthChildKeys; // keys derived from stealth addresses

    AccStealthKeyMap mapStealthKeys;
    AccStealthKeyMap mapLookAheadStealth;

    std::string sLabel; // account name
    CKeyID idMaster;

    uint32_t nActiveExternal;
    uint32_t nActiveInternal;
    uint32_t nActiveStealth;

    // Note: Stealth addresses consist of 2 secret keys, one of which (scan secret) must remain unencrypted while wallet locked
    // store a separate child key used only to derive secret keys
    // Stealth addresses must only ever be generated as hardened keys

    mutable CCriticalSection cs_account;

    // 0th key is always the account key
    std::vector<CStoredExtKey*> vExtKeys;
    std::vector<CKeyID> vExtKeyIDs;

    int nHeightCheckedUncrypted; // last block checked while uncrypted

    uint32_t nFlags;
    uint32_t nPack;
    uint32_t nPackStealth;
    uint32_t nPackStealthKeys;
    mapEKValue_t mapValue;
};



const char *ExtKeyGetString(int ind);

inline int GetNumBytesReqForInt(uint64_t v)
{
    int n = 0;
    while (v != 0)
    {
        v >>= 8;
        n ++;
    };
    return n;
};

std::vector<uint8_t> &SetCompressedInt64(std::vector<uint8_t> &v, uint64_t n);
int64_t GetCompressedInt64(const std::vector<uint8_t> &v, uint64_t &n);

std::vector<uint8_t> &SetCKeyID(std::vector<uint8_t> &v, CKeyID n);
bool GetCKeyID(const std::vector<uint8_t> &v, CKeyID &n);

std::vector<uint8_t> &SetString(std::vector<uint8_t> &v, const char *s);
std::vector<uint8_t> &SetChar(std::vector<uint8_t> &v, const uint8_t c);
std::vector<uint8_t> &PushUInt32(std::vector<uint8_t> &v, const uint32_t i);


inline bool IsHardened(uint32_t n)              { return (n & (1 << 31));};
inline uint32_t &SetHardenedBit(uint32_t &n)    { return (n |= (1 << 31));};
inline uint32_t &ClearHardenedBit(uint32_t &n)  { return (n &= ~(1 << 31));};
inline uint32_t WithHardenedBit(uint32_t n)     { return (n |= (1 << 31));};

int ExtractExtKeyPath(const std::string &sPath, std::vector<uint32_t> &vPath);

int PathToString(const std::vector<uint8_t> &vPath, std::string &sPath, char cH='\'', size_t nStart = 0);
int PathToString(const std::vector<uint32_t> &vPath, std::string &sPath, char cH='\'', size_t nStart = 0);

bool IsBIP32(const char *base58);

int AppendChainPath(const CStoredExtKey *pc, std::vector<uint32_t> &vPath);
int AppendChainPath(const CStoredExtKey *pc, std::vector<uint8_t> &vPath);
int AppendPath(const CStoredExtKey *pc, std::vector<uint32_t> &vPath);

std::string HDAccIDToString(const CKeyID &id);
std::string HDKeyIDToString(const CKeyID &id);

std::string GetDefaultAccountPath();

#endif // PARTICL_KEY_EXTKEY_H

