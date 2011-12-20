// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "protocol.h"
#include "util.h"

#ifndef WIN32
# include <arpa/inet.h>
#endif

// Prototypes from net.h, but that header (currently) stinks, can't #include it without breaking things
bool Lookup(const char *pszName, std::vector<CAddress>& vaddr, int nServices, int nMaxSolutions, bool fAllowLookup = false, int portDefault = 0, bool fAllowPort = false);
bool Lookup(const char *pszName, CAddress& addr, int nServices, bool fAllowLookup = false, int portDefault = 0, bool fAllowPort = false);

static const unsigned char pchIPv4[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };
static const char* ppszTypeName[] =
{
    "ERROR",
    "tx",
    "block",
};

CMessageHeader::CMessageHeader()
{
    memcpy(pchMessageStart, ::pchMessageStart, sizeof(pchMessageStart));
    memset(pchCommand, 0, sizeof(pchCommand));
    pchCommand[1] = 1;
    nMessageSize = -1;
    nChecksum = 0;
}

CMessageHeader::CMessageHeader(const char* pszCommand, unsigned int nMessageSizeIn)
{
    memcpy(pchMessageStart, ::pchMessageStart, sizeof(pchMessageStart));
    strncpy(pchCommand, pszCommand, COMMAND_SIZE);
    nMessageSize = nMessageSizeIn;
    nChecksum = 0;
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

CAddress::CAddress()
{
    Init();
}

CAddress::CAddress(unsigned int ipIn, unsigned short portIn, uint64 nServicesIn)
{
    Init();
    ip = ipIn;
    port = htons(portIn == 0 ? GetDefaultPort() : portIn);
    nServices = nServicesIn;
}

CAddress::CAddress(const struct sockaddr_in& sockaddr, uint64 nServicesIn)
{
    Init();
    ip = sockaddr.sin_addr.s_addr;
    port = sockaddr.sin_port;
    nServices = nServicesIn;
}

CAddress::CAddress(const char* pszIn, int portIn, bool fNameLookup, uint64 nServicesIn)
{
    Init();
    Lookup(pszIn, *this, nServicesIn, fNameLookup, portIn);
}

CAddress::CAddress(const char* pszIn, bool fNameLookup, uint64 nServicesIn)
{
    Init();
    Lookup(pszIn, *this, nServicesIn, fNameLookup, 0, true);
}

CAddress::CAddress(std::string strIn, int portIn, bool fNameLookup, uint64 nServicesIn)
{
    Init();
    Lookup(strIn.c_str(), *this, nServicesIn, fNameLookup, portIn);
}

CAddress::CAddress(std::string strIn, bool fNameLookup, uint64 nServicesIn)
{
    Init();
    Lookup(strIn.c_str(), *this, nServicesIn, fNameLookup, 0, true);
}

void CAddress::Init()
{
    nServices = NODE_NETWORK;
    memcpy(pchReserved, pchIPv4, sizeof(pchReserved));
    ip = INADDR_NONE;
    port = htons(GetDefaultPort());
    nTime = 100000000;
    nLastTry = 0;
}

bool operator==(const CAddress& a, const CAddress& b)
{
    return (memcmp(a.pchReserved, b.pchReserved, sizeof(a.pchReserved)) == 0 &&
            a.ip   == b.ip &&
            a.port == b.port);
}

bool operator!=(const CAddress& a, const CAddress& b)
{
    return (!(a == b));
}

bool operator<(const CAddress& a, const CAddress& b)
{
    int ret = memcmp(a.pchReserved, b.pchReserved, sizeof(a.pchReserved));
    if (ret < 0)
        return true;
    else if (ret == 0)
    {
        if (ntohl(a.ip) < ntohl(b.ip))
            return true;
        else if (a.ip == b.ip)
            return ntohs(a.port) < ntohs(b.port);
    }
    return false;
}

std::vector<unsigned char> CAddress::GetKey() const
{
    CDataStream ss;
    ss.reserve(18);
    ss << FLATDATA(pchReserved) << ip << port;

    #if defined(_MSC_VER) && _MSC_VER < 1300
    return std::vector<unsigned char>((unsigned char*)&ss.begin()[0], (unsigned char*)&ss.end()[0]);
    #else
    return std::vector<unsigned char>(ss.begin(), ss.end());
    #endif
}

struct sockaddr_in CAddress::GetSockAddr() const
{
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = ip;
    sockaddr.sin_port = port;
    return sockaddr;
}

bool CAddress::IsIPv4() const
{
    return (memcmp(pchReserved, pchIPv4, sizeof(pchIPv4)) == 0);
}

bool CAddress::IsRFC1918() const
{
  return IsIPv4() && (GetByte(3) == 10 ||
    (GetByte(3) == 192 && GetByte(2) == 168) ||
    (GetByte(3) == 172 &&
      (GetByte(2) >= 16 && GetByte(2) <= 31)));
}

bool CAddress::IsRFC3927() const
{
  return IsIPv4() && (GetByte(3) == 169 && GetByte(2) == 254);
}

bool CAddress::IsLocal() const
{
  return IsIPv4() && (GetByte(3) == 127 ||
      GetByte(3) == 0);
}

bool CAddress::IsRoutable() const
{
    return IsValid() &&
        !(IsRFC1918() || IsRFC3927() || IsLocal());
}

bool CAddress::IsValid() const
{
    // Clean up 3-byte shifted addresses caused by garbage in size field
    // of addr messages from versions before 0.2.9 checksum.
    // Two consecutive addr messages look like this:
    // header20 vectorlen3 addr26 addr26 addr26 header20 vectorlen3 addr26 addr26 addr26...
    // so if the first length field is garbled, it reads the second batch
    // of addr misaligned by 3 bytes.
    if (memcmp(pchReserved, pchIPv4+3, sizeof(pchIPv4)-3) == 0)
        return false;

    return (ip != 0 && ip != INADDR_NONE && port != htons(USHRT_MAX));
}

unsigned char CAddress::GetByte(int n) const
{
    return ((unsigned char*)&ip)[3-n];
}

std::string CAddress::ToStringIPPort() const
{
    return strprintf("%u.%u.%u.%u:%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0), ntohs(port));
}

std::string CAddress::ToStringIP() const
{
    return strprintf("%u.%u.%u.%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0));
}

std::string CAddress::ToStringPort() const
{
    return strprintf("%u", ntohs(port));
}

std::string CAddress::ToString() const
{
    return strprintf("%u.%u.%u.%u:%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0), ntohs(port));
}

void CAddress::print() const
{
    printf("CAddress(%s)\n", ToString().c_str());
}

CInv::CInv()
{
    type = 0;
    hash = 0;
}

CInv::CInv(int typeIn, const uint256& hashIn)
{
    type = typeIn;
    hash = hashIn;
}

CInv::CInv(const std::string& strType, const uint256& hashIn)
{
    int i;
    for (i = 1; i < ARRAYLEN(ppszTypeName); i++)
    {
        if (strType == ppszTypeName[i])
        {
            type = i;
            break;
        }
    }
    if (i == ARRAYLEN(ppszTypeName))
        throw std::out_of_range(strprintf("CInv::CInv(string, uint256) : unknown type '%s'", strType.c_str()));
    hash = hashIn;
}

bool operator<(const CInv& a, const CInv& b)
{
    return (a.type < b.type || (a.type == b.type && a.hash < b.hash));
}

bool CInv::IsKnownType() const
{
    return (type >= 1 && type < ARRAYLEN(ppszTypeName));
}

const char* CInv::GetCommand() const
{
    if (!IsKnownType())
        throw std::out_of_range(strprintf("CInv::GetCommand() : type=%d unknown type", type));
    return ppszTypeName[type];
}

std::string CInv::ToString() const
{
    return strprintf("%s %s", GetCommand(), hash.ToString().substr(0,20).c_str());
}

void CInv::print() const
{
    printf("CInv(%s)\n", ToString().c_str());
}
