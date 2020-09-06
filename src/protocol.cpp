// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <protocol.h>

#include <util/system.h>
#include <util/strencodings.h>

static std::atomic<bool> g_initial_block_download_completed(false);

namespace NetMsgType {
const char *VERSION="version";
const char *VERACK="verack";
const char *ADDR="addr";
const char *ADDRV2="addrv2";
const char *SENDADDRV2="sendaddrv2";
const char *INV="inv";
const char *GETDATA="getdata";
const char *MERKLEBLOCK="merkleblock";
const char *GETBLOCKS="getblocks";
const char *GETHEADERS="getheaders";
const char *TX="tx";
const char *HEADERS="headers";
const char *BLOCK="block";
const char *GETADDR="getaddr";
const char *MEMPOOL="mempool";
const char *PING="ping";
const char *PONG="pong";
const char *NOTFOUND="notfound";
const char *FILTERLOAD="filterload";
const char *FILTERADD="filteradd";
const char *FILTERCLEAR="filterclear";
const char *REJECT="reject";
const char *SENDHEADERS="sendheaders";
const char *SENDCMPCT="sendcmpct";
const char *CMPCTBLOCK="cmpctblock";
const char *GETBLOCKTXN="getblocktxn";
const char *BLOCKTXN="blocktxn";
// Dash message types
const char *LEGACYTXLOCKREQUEST="ix";
const char *SPORK="spork";
const char *GETSPORKS="getsporks";
const char *DSACCEPT="dsa";
const char *DSVIN="dsi";
const char *DSFINALTX="dsf";
const char *DSSIGNFINALTX="dss";
const char *DSCOMPLETE="dsc";
const char *DSSTATUSUPDATE="dssu";
const char *DSTX="dstx";
const char *DSQUEUE="dsq";
const char *SENDDSQUEUE="senddsq";
const char *SYNCSTATUSCOUNT="ssc";
const char *MNGOVERNANCESYNC="govsync";
const char *MNGOVERNANCEOBJECT="govobj";
const char *MNGOVERNANCEOBJECTVOTE="govobjvote";
const char *GETMNLISTDIFF="getmnlistd";
const char *MNLISTDIFF="mnlistdiff";
const char *QSENDRECSIGS="qsendrecsigs";
const char *QFCOMMITMENT="qfcommit";
const char *QCONTRIB="qcontrib";
const char *QCOMPLAINT="qcomplaint";
const char *QJUSTIFICATION="qjustify";
const char *QPCOMMITMENT="qpcommit";
const char *QWATCH="qwatch";
const char *QSIGSESANN="qsigsesann";
const char *QSIGSHARESINV="qsigsinv";
const char *QGETSIGSHARES="qgetsigs";
const char *QBSIGSHARES="qbsigs";
const char *QSIGREC="qsigrec";
const char *QSIGSHARE="qsigshare";
const char* QGETDATA = "qgetdata";
const char* QDATA = "qdata";
const char *CLSIG="clsig";
const char *ISLOCK="islock";
const char *MNAUTH="mnauth";
}; // namespace NetMsgType

/** All known message types. Keep this in the same order as the list of
 * messages above and in protocol.h.
 */
const static std::string allNetMessageTypes[] = {
    NetMsgType::VERSION,
    NetMsgType::VERACK,
    NetMsgType::ADDR,
    NetMsgType::ADDRV2,
    NetMsgType::SENDADDRV2,
    NetMsgType::INV,
    NetMsgType::GETDATA,
    NetMsgType::MERKLEBLOCK,
    NetMsgType::GETBLOCKS,
    NetMsgType::GETHEADERS,
    NetMsgType::TX,
    NetMsgType::HEADERS,
    NetMsgType::BLOCK,
    NetMsgType::GETADDR,
    NetMsgType::MEMPOOL,
    NetMsgType::PING,
    NetMsgType::PONG,
    NetMsgType::NOTFOUND,
    NetMsgType::FILTERLOAD,
    NetMsgType::FILTERADD,
    NetMsgType::FILTERCLEAR,
    NetMsgType::REJECT,
    NetMsgType::SENDHEADERS,
    NetMsgType::SENDCMPCT,
    NetMsgType::CMPCTBLOCK,
    NetMsgType::GETBLOCKTXN,
    NetMsgType::BLOCKTXN,
    // Dash message types
    // NOTE: do NOT include non-implmented here, we want them to be "Unknown command" in ProcessMessage()
    NetMsgType::LEGACYTXLOCKREQUEST,
    NetMsgType::SPORK,
    NetMsgType::GETSPORKS,
    NetMsgType::SENDDSQUEUE,
    NetMsgType::DSACCEPT,
    NetMsgType::DSVIN,
    NetMsgType::DSFINALTX,
    NetMsgType::DSSIGNFINALTX,
    NetMsgType::DSCOMPLETE,
    NetMsgType::DSSTATUSUPDATE,
    NetMsgType::DSTX,
    NetMsgType::DSQUEUE,
    NetMsgType::SYNCSTATUSCOUNT,
    NetMsgType::MNGOVERNANCESYNC,
    NetMsgType::MNGOVERNANCEOBJECT,
    NetMsgType::MNGOVERNANCEOBJECTVOTE,
    NetMsgType::GETMNLISTDIFF,
    NetMsgType::MNLISTDIFF,
    NetMsgType::QSENDRECSIGS,
    NetMsgType::QFCOMMITMENT,
    NetMsgType::QCONTRIB,
    NetMsgType::QCOMPLAINT,
    NetMsgType::QJUSTIFICATION,
    NetMsgType::QPCOMMITMENT,
    NetMsgType::QWATCH,
    NetMsgType::QSIGSESANN,
    NetMsgType::QSIGSHARESINV,
    NetMsgType::QGETSIGSHARES,
    NetMsgType::QBSIGSHARES,
    NetMsgType::QSIGREC,
    NetMsgType::QSIGSHARE,
    NetMsgType::QGETDATA,
    NetMsgType::QDATA,
    NetMsgType::CLSIG,
    NetMsgType::ISLOCK,
    NetMsgType::MNAUTH,
};
const static std::vector<std::string> allNetMessageTypesVec(allNetMessageTypes, allNetMessageTypes+ARRAYLEN(allNetMessageTypes));

CMessageHeader::CMessageHeader(const MessageStartChars& pchMessageStartIn)
{
    memcpy(pchMessageStart, pchMessageStartIn, MESSAGE_START_SIZE);
    memset(pchCommand, 0, sizeof(pchCommand));
    nMessageSize = -1;
    memset(pchChecksum, 0, CHECKSUM_SIZE);
}

CMessageHeader::CMessageHeader(const MessageStartChars& pchMessageStartIn, const char* pszCommand, unsigned int nMessageSizeIn)
{
    memcpy(pchMessageStart, pchMessageStartIn, MESSAGE_START_SIZE);
    memset(pchCommand, 0, sizeof(pchCommand));
    strncpy(pchCommand, pszCommand, COMMAND_SIZE);
    nMessageSize = nMessageSizeIn;
    memset(pchChecksum, 0, CHECKSUM_SIZE);
}

std::string CMessageHeader::GetCommand() const
{
    return std::string(pchCommand, pchCommand + strnlen(pchCommand, COMMAND_SIZE));
}

bool CMessageHeader::IsValid(const MessageStartChars& pchMessageStartIn) const
{
    // Check start string
    if (memcmp(pchMessageStart, pchMessageStartIn, MESSAGE_START_SIZE) != 0)
        return false;

    // Check the command string for errors
    for (const char* p1 = pchCommand; p1 < pchCommand + COMMAND_SIZE; p1++)
    {
        if (*p1 == 0)
        {
            // Must be all zeros after the first zero
            for (; p1 < pchCommand + COMMAND_SIZE; p1++)
                if (*p1 != 0)
                    return false;
        }
        else if (*p1 < ' ' || *p1 > 0x7E)
            return false;
    }

    // Message size
    if (nMessageSize > MAX_SIZE)
    {
        LogPrintf("CMessageHeader::IsValid(): (%s, %u bytes) nMessageSize > MAX_SIZE\n", GetCommand(), nMessageSize);
        return false;
    }

    return true;
}


ServiceFlags GetDesirableServiceFlags(ServiceFlags services) {
    if ((services & NODE_NETWORK_LIMITED) && g_initial_block_download_completed) {
        return ServiceFlags(NODE_NETWORK_LIMITED);
    }
    return ServiceFlags(NODE_NETWORK);
}

void SetServiceFlagsIBDCache(bool state) {
    g_initial_block_download_completed = state;
}


CAddress::CAddress() : CService()
{
    Init();
}

CAddress::CAddress(CService ipIn, ServiceFlags nServicesIn) : CService(ipIn)
{
    Init();
    nServices = nServicesIn;
}

CAddress::CAddress(CService ipIn, ServiceFlags nServicesIn, uint32_t nTimeIn) : CService(ipIn)
{
    Init();
    nServices = nServicesIn;
    nTime = nTimeIn;
}

void CAddress::Init()
{
    nServices = NODE_NONE;
    nTime = 100000000;
}

CInv::CInv()
{
    type = 0;
    hash.SetNull();
}

CInv::CInv(int typeIn, const uint256& hashIn) : type(typeIn), hash(hashIn) {}

bool operator<(const CInv& a, const CInv& b)
{
    return (a.type < b.type || (a.type == b.type && a.hash < b.hash));
}

bool CInv::IsKnownType() const
{
    return GetCommandInternal() != nullptr;
}

const char* CInv::GetCommandInternal() const
{
    switch (type)
    {
        case MSG_TX:                            return NetMsgType::TX;
        case MSG_BLOCK:                         return NetMsgType::BLOCK;
        case MSG_FILTERED_BLOCK:                return NetMsgType::MERKLEBLOCK;
        case MSG_LEGACY_TXLOCK_REQUEST:         return NetMsgType::LEGACYTXLOCKREQUEST;
        case MSG_CMPCT_BLOCK:                   return NetMsgType::CMPCTBLOCK;
        case MSG_SPORK:                         return NetMsgType::SPORK;
        case MSG_DSTX:                          return NetMsgType::DSTX;
        case MSG_GOVERNANCE_OBJECT:             return NetMsgType::MNGOVERNANCEOBJECT;
        case MSG_GOVERNANCE_OBJECT_VOTE:        return NetMsgType::MNGOVERNANCEOBJECTVOTE;
        case MSG_QUORUM_FINAL_COMMITMENT:       return NetMsgType::QFCOMMITMENT;
        case MSG_QUORUM_CONTRIB:                return NetMsgType::QCONTRIB;
        case MSG_QUORUM_COMPLAINT:              return NetMsgType::QCOMPLAINT;
        case MSG_QUORUM_JUSTIFICATION:          return NetMsgType::QJUSTIFICATION;
        case MSG_QUORUM_PREMATURE_COMMITMENT:   return NetMsgType::QPCOMMITMENT;
        case MSG_QUORUM_RECOVERED_SIG:          return NetMsgType::QSIGREC;
        case MSG_CLSIG:                         return NetMsgType::CLSIG;
        case MSG_ISLOCK:                        return NetMsgType::ISLOCK;
        default:
            return nullptr;
    }
}

std::string CInv::GetCommand() const
{
    auto cmd = GetCommandInternal();
    if (cmd == nullptr) {
        throw std::out_of_range(strprintf("CInv::GetCommand(): type=%d unknown type", type));
    }
    return cmd;
}

std::string CInv::ToString() const
{
    auto cmd = GetCommandInternal();
    if (!cmd) {
        return strprintf("0x%08x %s", type, hash.ToString());
    } else {
        return strprintf("%s %s", cmd, hash.ToString());
    }
}

const std::vector<std::string> &getAllNetMessageTypes()
{
    return allNetMessageTypesVec;
}
