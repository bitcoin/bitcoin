// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <protocol.h>

#include <util/system.h>

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
const char *SENDHEADERS="sendheaders";
const char *FEEFILTER="feefilter";
const char *SENDCMPCT="sendcmpct";
const char *CMPCTBLOCK="cmpctblock";
const char *GETBLOCKTXN="getblocktxn";
const char *BLOCKTXN="blocktxn";
const char *GETCFILTERS="getcfilters";
const char *CFILTER="cfilter";
const char *GETCFHEADERS="getcfheaders";
const char *CFHEADERS="cfheaders";
const char *GETCFCHECKPT="getcfcheckpt";
const char *CFCHECKPT="cfcheckpt";
const char *WTXIDRELAY="wtxidrelay";
const char *SENDTXRCNCL="sendtxrcncl";
} // namespace NetMsgType

/** All known message types including the short-ID (as initially defined in BIP324).
 *  Keep this in the same order as the list of messages above and in protocol.h.
 */
const static std::map<std::string, uint8_t> allNetMessageTypes = {
    {NetMsgType::VERSION, NO_BIP324_SHORT_ID},
    {NetMsgType::VERACK, NO_BIP324_SHORT_ID},
    {NetMsgType::ADDR, 1},
    {NetMsgType::ADDRV2, 28},
    {NetMsgType::SENDADDRV2, NO_BIP324_SHORT_ID},
    {NetMsgType::INV, 14},
    {NetMsgType::GETDATA, 11},
    {NetMsgType::MERKLEBLOCK, 16},
    {NetMsgType::GETBLOCKS, 9},
    {NetMsgType::GETHEADERS, 12},
    {NetMsgType::TX, 21},
    {NetMsgType::HEADERS, 13},
    {NetMsgType::BLOCK, 2},
    {NetMsgType::GETADDR, NO_BIP324_SHORT_ID},
    {NetMsgType::MEMPOOL, 15},
    {NetMsgType::PING, 18},
    {NetMsgType::PONG, 19},
    {NetMsgType::NOTFOUND, 17},
    {NetMsgType::FILTERLOAD, 8},
    {NetMsgType::FILTERADD, 6},
    {NetMsgType::FILTERCLEAR, 7},
    {NetMsgType::SENDHEADERS, NO_BIP324_SHORT_ID},
    {NetMsgType::FEEFILTER, 5},
    {NetMsgType::SENDCMPCT, 20},
    {NetMsgType::CMPCTBLOCK, 4},
    {NetMsgType::GETBLOCKTXN, 10},
    {NetMsgType::BLOCKTXN, 3},
    {NetMsgType::GETCFILTERS, 22},
    {NetMsgType::CFILTER, 23},
    {NetMsgType::GETCFHEADERS, 24},
    {NetMsgType::CFHEADERS, 25},
    {NetMsgType::GETCFCHECKPT, 26},
    {NetMsgType::CFCHECKPT, 27},
    {NetMsgType::WTXIDRELAY, NO_BIP324_SHORT_ID},
    {NetMsgType::SENDTXRCNCL, NO_BIP324_SHORT_ID}};

static std::map<uint8_t, std::string> shortIDMsgTypes;

CMessageHeader::CMessageHeader(const MessageStartChars& pchMessageStartIn, const char* pszCommand, unsigned int nMessageSizeIn)
{
    memcpy(pchMessageStart, pchMessageStartIn, MESSAGE_START_SIZE);

    // Copy the command name
    size_t i = 0;
    for (; i < COMMAND_SIZE && pszCommand[i] != 0; ++i) pchCommand[i] = pszCommand[i];
    assert(pszCommand[i] == 0); // Assert that the command name passed in is not longer than COMMAND_SIZE

    nMessageSize = nMessageSizeIn;
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
        return ServiceFlags(NODE_NETWORK_LIMITED | NODE_WITNESS);
    }
    return ServiceFlags(NODE_NETWORK | NODE_WITNESS);
}

void SetServiceFlagsIBDCache(bool state) {
    g_initial_block_download_completed = state;
}

CInv::CInv()
{
    type = 0;
    hash.SetNull();
}

CInv::CInv(uint32_t typeIn, const uint256& hashIn) : type(typeIn), hash(hashIn) {}

bool operator<(const CInv& a, const CInv& b)
{
    return (a.type < b.type || (a.type == b.type && a.hash < b.hash));
}

std::string CInv::GetCommand() const
{
    std::string cmd;
    if (type & MSG_WITNESS_FLAG)
        cmd.append("witness-");
    int masked = type & MSG_TYPE_MASK;
    switch (masked)
    {
    case MSG_TX:             return cmd.append(NetMsgType::TX);
    // WTX is not a message type, just an inv type
    case MSG_WTX:            return cmd.append("wtx");
    case MSG_BLOCK:          return cmd.append(NetMsgType::BLOCK);
    case MSG_FILTERED_BLOCK: return cmd.append(NetMsgType::MERKLEBLOCK);
    case MSG_CMPCT_BLOCK:    return cmd.append(NetMsgType::CMPCTBLOCK);
    default:
        throw std::out_of_range(strprintf("CInv::GetCommand(): type=%d unknown type", type));
    }
}

std::string CInv::ToString() const
{
    try {
        return strprintf("%s %s", GetCommand(), hash.ToString());
    } catch(const std::out_of_range &) {
        return strprintf("0x%08x %s", type, hash.ToString());
    }
}

const std::map<std::string, uint8_t>& getAllNetMessageTypes()
{
    return allNetMessageTypes;
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
    case NODE_WITNESS:         return "WITNESS";
    case NODE_COMPACT_FILTERS: return "COMPACT_FILTERS";
    case NODE_NETWORK_LIMITED: return "NETWORK_LIMITED";
    // Not using default, so we get warned when a case is missing
    }

    return strprintf("UNKNOWN[2^%u]", bit);
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

GenTxid ToGenTxid(const CInv& inv)
{
    assert(inv.IsGenTxMsg());
    return inv.IsMsgWtx() ? GenTxid::Wtxid(inv.hash) : GenTxid::Txid(inv.hash);
}

uint8_t GetShortIDFromMessageType(const std::string& message_type)
{
    auto it = allNetMessageTypes.find(message_type);
    if (it != allNetMessageTypes.end()) {
        return it->second;
    }
    return NO_BIP324_SHORT_ID;
}

std::optional<std::string> GetMessageTypeFromShortID(const uint8_t shortID)
{
    if (shortIDMsgTypes.empty()) {
        for (const std::pair<std::string, uint8_t> entry : allNetMessageTypes) {
            if (entry.second != NO_BIP324_SHORT_ID) {
                shortIDMsgTypes[entry.second] = entry.first;
            }
        }
    }

    auto it = shortIDMsgTypes.find(shortID);
    if (it != shortIDMsgTypes.end()) {
        return it->second;
    }
    return {};
}
