// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <protocol.h>

#include <util/system.h>

#include <atomic>

static std::atomic<bool> g_initial_block_download_completed(false);

#define MAKE_MSG(var_name, p2p_name_str)   \
        const char* var_name=p2p_name_str; \
        static_assert(std::size(p2p_name_str) <= CMessageHeader::COMMAND_SIZE + 1, "p2p_name_str cannot be greater than COMMAND_SIZE"); // Includes +1 for null termination character.

namespace NetMsgType {
MAKE_MSG(VERSION, "version");
MAKE_MSG(VERACK, "verack");
MAKE_MSG(ADDR, "addr");
MAKE_MSG(ADDRV2, "addrv2");
MAKE_MSG(SENDADDRV2, "sendaddrv2");
MAKE_MSG(INV, "inv");
MAKE_MSG(GETDATA, "getdata");
MAKE_MSG(MERKLEBLOCK, "merkleblock");
MAKE_MSG(GETBLOCKS, "getblocks");
MAKE_MSG(GETHEADERS, "getheaders");
MAKE_MSG(TX, "tx");
MAKE_MSG(HEADERS, "headers");
MAKE_MSG(BLOCK, "block");
MAKE_MSG(GETADDR, "getaddr");
MAKE_MSG(MEMPOOL, "mempool");
MAKE_MSG(PING, "ping");
MAKE_MSG(PONG, "pong");
MAKE_MSG(NOTFOUND, "notfound");
MAKE_MSG(FILTERLOAD, "filterload");
MAKE_MSG(FILTERADD, "filteradd");
MAKE_MSG(FILTERCLEAR, "filterclear");
MAKE_MSG(SENDHEADERS, "sendheaders");
MAKE_MSG(SENDCMPCT, "sendcmpct");
MAKE_MSG(CMPCTBLOCK, "cmpctblock");
MAKE_MSG(GETBLOCKTXN, "getblocktxn");
MAKE_MSG(BLOCKTXN, "blocktxn");
MAKE_MSG(GETCFILTERS, "getcfilters");
MAKE_MSG(CFILTER, "cfilter");
MAKE_MSG(GETCFHEADERS, "getcfheaders");
MAKE_MSG(CFHEADERS, "cfheaders");
MAKE_MSG(GETCFCHECKPT, "getcfcheckpt");
MAKE_MSG(CFCHECKPT, "cfcheckpt");
// Dash message types
MAKE_MSG(SPORK, "spork");
MAKE_MSG(GETSPORKS, "getsporks");
MAKE_MSG(DSACCEPT, "dsa");
MAKE_MSG(DSVIN, "dsi");
MAKE_MSG(DSFINALTX, "dsf");
MAKE_MSG(DSSIGNFINALTX, "dss");
MAKE_MSG(DSCOMPLETE, "dsc");
MAKE_MSG(DSSTATUSUPDATE, "dssu");
MAKE_MSG(DSTX, "dstx");
MAKE_MSG(DSQUEUE, "dsq");
MAKE_MSG(SENDDSQUEUE, "senddsq");
MAKE_MSG(SYNCSTATUSCOUNT, "ssc");
MAKE_MSG(MNGOVERNANCESYNC, "govsync");
MAKE_MSG(MNGOVERNANCEOBJECT, "govobj");
MAKE_MSG(MNGOVERNANCEOBJECTVOTE, "govobjvote");
MAKE_MSG(GETMNLISTDIFF, "getmnlistd");
MAKE_MSG(MNLISTDIFF, "mnlistdiff");
MAKE_MSG(QSENDRECSIGS, "qsendrecsigs");
MAKE_MSG(QFCOMMITMENT, "qfcommit");
MAKE_MSG(QCONTRIB, "qcontrib");
MAKE_MSG(QCOMPLAINT, "qcomplaint");
MAKE_MSG(QJUSTIFICATION, "qjustify");
MAKE_MSG(QPCOMMITMENT, "qpcommit");
MAKE_MSG(QWATCH, "qwatch");
MAKE_MSG(QSIGSESANN, "qsigsesann");
MAKE_MSG(QSIGSHARESINV, "qsigsinv");
MAKE_MSG(QGETSIGSHARES, "qgetsigs");
MAKE_MSG(QBSIGSHARES, "qbsigs");
MAKE_MSG(QSIGREC, "qsigrec");
MAKE_MSG(QSIGSHARE, "qsigshare");
MAKE_MSG(QGETDATA, "qgetdata");
MAKE_MSG(QDATA, "qdata");
MAKE_MSG(CLSIG, "clsig");
MAKE_MSG(ISLOCK, "islock");
MAKE_MSG(ISDLOCK, "isdlock");
MAKE_MSG(MNAUTH, "mnauth");
MAKE_MSG(GETHEADERS2, "getheaders2");
MAKE_MSG(SENDHEADERS2, "sendheaders2");
MAKE_MSG(HEADERS2, "headers2");
MAKE_MSG(GETQUORUMROTATIONINFO, "getqrinfo");
MAKE_MSG(QUORUMROTATIONINFO, "qrinfo");
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
    NetMsgType::SENDHEADERS,
    NetMsgType::SENDCMPCT,
    NetMsgType::CMPCTBLOCK,
    NetMsgType::GETBLOCKTXN,
    NetMsgType::BLOCKTXN,
    NetMsgType::GETCFILTERS,
    NetMsgType::CFILTER,
    NetMsgType::GETCFHEADERS,
    NetMsgType::CFHEADERS,
    NetMsgType::GETCFCHECKPT,
    NetMsgType::CFCHECKPT,
    // Dash message types
    // NOTE: do NOT include non-implmented here, we want them to be "Unknown command" in ProcessMessage()
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
    NetMsgType::ISDLOCK,
    NetMsgType::MNAUTH,
    NetMsgType::GETHEADERS2,
    NetMsgType::SENDHEADERS2,
    NetMsgType::HEADERS2};
const static std::vector<std::string> allNetMessageTypesVec(std::begin(allNetMessageTypes), std::end(allNetMessageTypes));

/** Message types that are not allowed by blocks-relay-only policy.
 *  We do not want most of CoinJoin, DKG or LLMQ signing messages to be relayed
 *  to/from nodes via connections that were established in this mode.
 *  Make sure to keep this list up to date whenever a new message type is added.
 *  NOTE: Unlike the list above, this list is sorted alphabetically.
 */
const static std::string netMessageTypesViolateBlocksOnly[] = {
    NetMsgType::DSACCEPT,
    NetMsgType::DSCOMPLETE,
    NetMsgType::DSFINALTX,
    NetMsgType::DSQUEUE,
    NetMsgType::DSSIGNFINALTX,
    NetMsgType::DSSTATUSUPDATE,
    NetMsgType::DSTX,
    NetMsgType::DSVIN,
    NetMsgType::QBSIGSHARES,
    NetMsgType::QCOMPLAINT,
    NetMsgType::QCONTRIB,
    NetMsgType::QDATA,
    NetMsgType::QGETDATA,
    NetMsgType::QGETSIGSHARES,
    NetMsgType::QJUSTIFICATION,
    NetMsgType::QPCOMMITMENT,
    NetMsgType::QSENDRECSIGS,
    NetMsgType::QSIGREC,
    NetMsgType::QSIGSESANN,
    NetMsgType::QSIGSHARE,
    NetMsgType::QSIGSHARESINV,
    NetMsgType::QWATCH,
    NetMsgType::TX,
};
const static std::set<std::string> netMessageTypesViolateBlocksOnlySet(std::begin(netMessageTypesViolateBlocksOnly), std::end(netMessageTypesViolateBlocksOnly));

CMessageHeader::CMessageHeader()
{
    memset(pchMessageStart, 0, MESSAGE_START_SIZE);
    memset(pchCommand, 0, sizeof(pchCommand));
    memset(pchChecksum, 0, CHECKSUM_SIZE);
}

CMessageHeader::CMessageHeader(const MessageStartChars& pchMessageStartIn, const char* pszCommand, unsigned int nMessageSizeIn)
{
    memcpy(pchMessageStart, pchMessageStartIn, MESSAGE_START_SIZE);

    // Copy the command name, zero-padding to COMMAND_SIZE bytes
    size_t i = 0;
    for (; i < COMMAND_SIZE && pszCommand[i] != 0; ++i) pchCommand[i] = pszCommand[i];
    assert(pszCommand[i] == 0); // Assert that the command name passed in is not longer than COMMAND_SIZE
    for (; i < COMMAND_SIZE; ++i) pchCommand[i] = 0;

    nMessageSize = nMessageSizeIn;
    memset(pchChecksum, 0, CHECKSUM_SIZE);
}

std::string CMessageHeader::GetCommand() const
{
    return std::string(pchCommand, pchCommand + strnlen(pchCommand, COMMAND_SIZE));
}

bool CMessageHeader::IsCommandValid() const
{
    // Check the command string for errors
    for (const char* p1 = pchCommand; p1 < pchCommand + COMMAND_SIZE; ++p1) {
        if (*p1 == 0) {
            // Must be all zeros after the first zero
            for (; p1 < pchCommand + COMMAND_SIZE; ++p1) {
                if (*p1 != 0) {
                    return false;
                }
            }
        } else if (*p1 < ' ' || *p1 > 0x7E) {
            return false;
        }
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
        case MSG_ISDLOCK:                       return NetMsgType::ISDLOCK;
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

bool NetMessageViolatesBlocksOnly(const std::string& msg_type)
{
    return netMessageTypesViolateBlocksOnlySet.find(msg_type) != netMessageTypesViolateBlocksOnlySet.end();
}

/**
 * Convert a service flag (NODE_*) to a human readable string.
 * It supports unknown service flags which will be returned as "UNKNOWN[...]".
 * @param[in] bit the service flag is calculated as (1 << bit)
 */
static std::string serviceFlagToStr(size_t bit)
{
    const uint64_t service_flag = 1ULL << bit;
    switch ((ServiceFlags)service_flag) {
    case NODE_NONE: abort();  // impossible
    case NODE_NETWORK:         return "NETWORK";
    case NODE_BLOOM:           return "BLOOM";
    case NODE_COMPACT_FILTERS: return "COMPACT_FILTERS";
    case NODE_NETWORK_LIMITED: return "NETWORK_LIMITED";
    case NODE_HEADERS_COMPRESSED: return "HEADERS_COMPRESSED";
    // Not using default, so we get warned when a case is missing
    }

    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << "UNKNOWN[";
    stream << "2^" << bit;
    stream << "]";
    return stream.str();
}

std::vector<std::string> serviceFlagsToStr(uint64_t flags)
{
    std::vector<std::string> str_flags;

    for (size_t i = 0; i < sizeof(flags) * 8; ++i) {
        if (flags & (1ULL << i)) {
            str_flags.emplace_back(serviceFlagToStr(i));
        }
    }

    return str_flags;
}
