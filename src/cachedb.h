// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CACHEDB_H
#define BITCOIN_CACHEDB_H

#include <fs.h>
#include <serialize.h>

#include <string>
#include <map>

class CSubNet;
class CAddrMan;
class CMasternodeMan;
class CGovernanceManager;
class CNetFulfilledRequestManager;
class CMasternodePayments;

class CDataStream;

typedef enum BanReason
{
    BanReasonUnknown          = 0,
    BanReasonNodeMisbehaving  = 1,
    BanReasonManuallyAdded    = 2
} BanReason;

class CBanEntry
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    int64_t nCreateTime;
    int64_t nBanUntil;
    uint8_t banReason;

    CBanEntry()
    {
        SetNull();
    }

    CBanEntry(int64_t nCreateTimeIn)
    {
        SetNull();
        nCreateTime = nCreateTimeIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(this->nVersion);
        READWRITE(nCreateTime);
        READWRITE(nBanUntil);
        READWRITE(banReason);
    }

    void SetNull()
    {
        nVersion = CBanEntry::CURRENT_VERSION;
        nCreateTime = 0;
        nBanUntil = 0;
        banReason = BanReasonUnknown;
    }

    std::string banReasonToString() const
    {
        switch (banReason) {
        case BanReasonNodeMisbehaving:
            return "node misbehaving";
        case BanReasonManuallyAdded:
            return "manually added";
        default:
            return "unknown";
        }
    }
};

typedef std::map<CSubNet, CBanEntry> banmap_t;

/** Access to the (IP) address database (peers.dat) */
class CAddrDB
{
private:
    fs::path pathAddr;
public:
    CAddrDB();
    bool Write(const CAddrMan& addr);
    bool Read(CAddrMan& addr);
    static bool Read(CAddrMan& addr, CDataStream& ssPeers);
};

/** Access to the banlist database (banlist.dat) */
class CBanDB
{
private:
    fs::path pathBanlist;
public:
    CBanDB();
    bool Write(const banmap_t& banSet);
    bool Read(banmap_t& banSet);
};

// Chaincoin specific cache files

/** Access to the mncache database (mncache.dat) */
class CMNCacheDB
{
private:
    fs::path pathMNCache;
public:
    CMNCacheDB();
    bool Write(const CMasternodeMan& mncache);
    bool Read(CMasternodeMan& mncache);
};

/** Access to the mnpayments database (mnpayments.dat) */
class CMNPayDB
{
private:
    fs::path pathMNPay;
public:
    CMNPayDB();
    bool Write(const CMasternodePayments& mnpayments);
    bool Read(CMasternodePayments& mnpayments);
};

/** Access to the governance database (governance.dat) */
class CGovDB
{
private:
    fs::path pathGovernance;
public:
    CGovDB();
    bool Write(const CGovernanceManager& governance);
    bool Read(CGovernanceManager& governance);
};

/** Access to the netfulfilled database (netfulfilled.dat) */
class CNetFulDB
{
private:
    fs::path pathNetfulfilled;
public:
    CNetFulDB();
    bool Write(const CNetFulfilledRequestManager& netfulfilled);
    bool Read(CNetFulfilledRequestManager& netfulfilled);
};

#endif // BITCOIN_CACHEDB_H
