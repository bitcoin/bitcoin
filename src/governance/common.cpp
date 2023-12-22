// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/common.h>

#include <util/strencodings.h>
#include <util/underlying.h>
#include <hash.h>
#include <univalue.h>

namespace Governance
{

Object::Object(const uint256& nHashParent, int nRevision, int64_t nTime, const uint256& nCollateralHash, const std::string& strDataHex) :
    hashParent{nHashParent},
    revision{nRevision},
    time{nTime},
    collateralHash{nCollateralHash},
    masternodeOutpoint{},
    vchSig{},
    vchData{ParseHex(strDataHex)}
{
}

uint256 Object::GetHash() const
{
    // Note: doesn't match serialization

    // CREATE HASH OF ALL IMPORTANT PIECES OF DATA

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << hashParent;
    ss << revision;
    ss << time;
    ss << HexStr(vchData);
    ss << masternodeOutpoint << uint8_t{} << 0xffffffff; // adding dummy values here to match old hashing
    ss << vchSig;
    // fee_tx is left out on purpose

    return ss.GetHash();
}

UniValue Object::ToJson() const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("objectHash", GetHash().ToString());
    obj.pushKV("parentHash", hashParent.ToString());
    obj.pushKV("collateralHash", collateralHash.ToString());
    obj.pushKV("createdAt", time);
    obj.pushKV("revision", revision);
    UniValue data;
    if (!data.read(GetDataAsPlainString())) {
        data.clear();
        data.setObject();
        data.pushKV("plain", GetDataAsPlainString());
    }
    data.pushKV("hex", GetDataAsHexString());
    obj.pushKV("data", data);
    return obj;
}

std::string Object::GetDataAsHexString() const
{
    return HexStr(vchData);
}

std::string Object::GetDataAsPlainString() const
{
    return std::string(vchData.begin(), vchData.end());
}

} // namespace Governance
