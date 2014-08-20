// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _BITCOINALERT_H_
#define _BITCOINALERT_H_ 1

#include "serialize.h"
#include "sync.h"

#include <map>
#include <set>
#include <stdint.h>
#include <string>

class CAlert;
class CNode;
class uint256;

extern std::map<uint256, CAlert> mapAlerts;
extern CCriticalSection cs_mapAlerts;

/** Alerts are for notifying old versions if they become too obsolete and
 * need to upgrade.  The message is displayed in the status bar.
 * Alert messages are broadcast as a vector of signed data.  Unserializing may
 * not read the entire buffer if the alert is for a newer version, but older
 * versions can still relay the original data.
 */
class CUnsignedAlert
{
public:
    int nVersion;
    int64_t nRelayUntil;      // when newer nodes stop relaying to newer nodes
    int64_t nExpiration;
    int nID;
    int nCancel;
    std::set<int> setCancel;
    int nMinVer;            // lowest version inclusive
    int nMaxVer;            // highest version inclusive
    std::set<std::string> setSubVer;  // empty matches all
    int nPriority;

    // Actions
    std::string strComment;
    std::string strStatusBar;
    std::string strReserved;

    IMPLEMENT_SERIALIZE

    template <typename T, typename Stream, typename Operation>
    inline static size_t SerializationOp(T thisPtr, Stream& s, Operation ser_action, int nType, int nVersion) {
        size_t nSerSize = 0;
        READWRITE(thisPtr->nVersion);
        nVersion = thisPtr->nVersion;
        READWRITE(thisPtr->nRelayUntil);
        READWRITE(thisPtr->nExpiration);
        READWRITE(thisPtr->nID);
        READWRITE(thisPtr->nCancel);
        READWRITE(thisPtr->setCancel);
        READWRITE(thisPtr->nMinVer);
        READWRITE(thisPtr->nMaxVer);
        READWRITE(thisPtr->setSubVer);
        READWRITE(thisPtr->nPriority);

        READWRITE(LIMITED_STRING(thisPtr->strComment, 65536));
        READWRITE(LIMITED_STRING(thisPtr->strStatusBar, 256));
        READWRITE(LIMITED_STRING(thisPtr->strReserved, 256));
        return nSerSize;
    }

    void SetNull();

    std::string ToString() const;
};

/** An alert is a combination of a serialized CUnsignedAlert and a signature. */
class CAlert : public CUnsignedAlert
{
public:
    std::vector<unsigned char> vchMsg;
    std::vector<unsigned char> vchSig;

    CAlert()
    {
        SetNull();
    }

    IMPLEMENT_SERIALIZE

    template <typename T, typename Stream, typename Operation>
    inline static size_t SerializationOp(T thisPtr, Stream& s, Operation ser_action, int nType, int nVersion) {
        size_t nSerSize = 0;
        READWRITE(thisPtr->vchMsg);
        READWRITE(thisPtr->vchSig);
        return nSerSize;
    }

    void SetNull();
    bool IsNull() const;
    uint256 GetHash() const;
    bool IsInEffect() const;
    bool Cancels(const CAlert& alert) const;
    bool AppliesTo(int nVersion, std::string strSubVerIn) const;
    bool AppliesToMe() const;
    bool RelayTo(CNode* pnode) const;
    bool CheckSignature() const;
    bool ProcessAlert(bool fThread = true);

    /*
     * Get copy of (active) alert object by hash. Returns a null alert if it is not found.
     */
    static CAlert getAlertByHash(const uint256 &hash);
};

#endif
