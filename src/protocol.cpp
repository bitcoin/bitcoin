// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "protocol.h"
#include "util.h"
#include "netbase.h"

#ifndef WIN32
# include <arpa/inet.h>
#endif

static const std::string forfill[] = { "ERROR", "tx", "block" }; //TODO: Replace with initializer list constructor when c++11 comes
static const std::vector<std::string> vpszTypeName(forfill, forfill + 3);

CMessageHeader::CMessageHeader() : nMessageSize(std::numeric_limits<uint32_t>::max()), nChecksum(0)
{
    memcpy(pchMessageStart, ::pchMessageStart, sizeof(pchMessageStart));
    memset(pchCommand, 0, sizeof(pchCommand));
    pchCommand[1] = 1;
}

CMessageHeader::CMessageHeader(const char* pszCommand, unsigned int nMessageSizeIn) : nMessageSize(nMessageSizeIn), nChecksum(0)
{
    memcpy(pchMessageStart, ::pchMessageStart, sizeof(pchMessageStart));
    strncpy(pchCommand, pszCommand, COMMAND_SIZE);
}

std::string CMessageHeader::GetCommand() const
{
    if (pchCommand[COMMAND_SIZE-1] == 0)
        return std::string(pchCommand, pchCommand + strlen(pchCommand));
    else
        return std::string(pchCommand, pchCommand + COMMAND_SIZE);
}

bool CMessageHeader::IsValid() const
{
    // Check start string
    if (memcmp(pchMessageStart, ::pchMessageStart, sizeof(pchMessageStart)) != 0)
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
        printf("CMessageHeader::IsValid() : (%s, %u bytes) nMessageSize > MAX_SIZE\n", GetCommand().c_str(), nMessageSize);
        return false;
    }

    return true;
}

CAddress::CAddress() : CService(), nServices(NODE_NETWORK), nTime(100000000), nLastTry(0) { }
CAddress::CAddress(CService ipIn, uint64_t nServicesIn) : CService(ipIn), nServices(nServicesIn), nTime(100000000), nLastTry(0) { }
CInv::CInv() : type(0), hash(0) { }
CInv::CInv(int typeIn, const uint256& hashIn) : type(typeIn), hash(hashIn) { }
CInv::CInv(const std::string& strType, const uint256& hashIn) : hash(hashIn)
{
    unsigned int i;
    for (i = 1; i < vpszTypeName.size(); ++i)
    {
        if (strType.compare(vpszTypeName[i]) == 0) {
            type = i;
            break;
        }
    }
    if (i == vpszTypeName.size())
        throw std::out_of_range(strprintf("CInv::CInv(string, uint256) : unknown type '%s'", strType.c_str()));
}

bool operator<(const CInv& a, const CInv& b)
{
    return (a.type < b.type || (a.type == b.type && a.hash < b.hash));
}

bool CInv::IsKnownType() const
{
    return (type >= 1 && type < (int)vpszTypeName.size());
}

const char* CInv::GetCommand() const
{
    if (!IsKnownType())
        throw std::out_of_range(strprintf("CInv::GetCommand() : type=%d unknown type", type));
    return vpszTypeName[type].c_str();
}

std::string CInv::ToString() const
{
    return strprintf("%s %s", GetCommand(), hash.ToString().substr(0,20).c_str());
}