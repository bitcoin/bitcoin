// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_GOVERNANCE_GOVERNANCECOMMON_H
#define SYSCOIN_GOVERNANCE_GOVERNANCECOMMON_H

#include <primitives/transaction.h>
#include <uint256.h>

#include <string>
#include <vector>

/**
 * This module is a public interface of governance module that can be used
 * in other components such as wallet
 */

class UniValue;

class GovernanceObject {
    int nValue;
    public:
        GovernanceObject(const int& value) {
            nValue = value;
        }
        int GetValue() const {
            return nValue;
        }
        SERIALIZE_METHODS(GovernanceObject, obj)
        {
            READWRITE(obj.nValue);
        }
};
static constexpr int GOVERNANCE_OBJECT_UNKNOWN = 0;
static constexpr int GOVERNANCE_OBJECT_PROPOSAL = 1;
static constexpr int GOVERNANCE_OBJECT_TRIGGER = 2;
namespace Governance
{
class Object
{
public:
    Object() = default;

    Object(const uint256& nHashParent, int nRevision, int64_t nTime, const uint256& nCollateralHash, const std::string& strDataHex);

    UniValue ToJson() const;

    uint256 GetHash() const;

    std::string GetDataAsHexString() const;
    std::string GetDataAsPlainString() const;

    /// Object typecode
    GovernanceObject type{GOVERNANCE_OBJECT_UNKNOWN};

    /// parent object, 0 is root
    uint256 hashParent{};

    /// object revision in the system
    int revision{0};

    /// time this object was created
    int64_t time{0};

    /// fee-tx
    uint256 collateralHash{};

    /// Masternode info for signed objects
    COutPoint masternodeOutpoint;
    std::vector<unsigned char> vchSig{};

    /// Data field - can be used for anything
    std::vector<unsigned char> vchData;

    SERIALIZE_METHODS(Object, obj)
    {
        READWRITE(
                obj.hashParent,
                obj.revision,
                obj.time,
                obj.collateralHash,
                obj.vchData,
                obj.type,
                obj.masternodeOutpoint
                );
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(obj.vchSig);
        }
    }
};

} // namespace Governance
#endif
