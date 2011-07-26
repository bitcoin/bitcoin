// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_NET_H
#define BITCOIN_NET_H

#include <deque>
#include <boost/array.hpp>
#include <openssl/rand.h>

#ifndef __WXMSW__
#include <arpa/inet.h>
#else
#include <ws2tcpip.h>
#endif

class CMessageHeader;
class CAddress;
class CAddrDB;
class CInv;
class CRequestTracker;
class CNode;
class CBlockIndex;
extern int nBestHeight;
extern int nConnectTimeout;



inline unsigned int ReceiveBufferSize() { return 1000*GetArg("-maxreceivebuffer", 10*1000); }
inline unsigned int SendBufferSize() { return 1000*GetArg("-maxsendbuffer", 10*1000); }
inline unsigned short GetDefaultPort() { return fTestNet ? 18333 : 8333; }
static const unsigned int PUBLISH_HOPS = 5;
enum
{
    NODE_NETWORK = (1 << 0),
};




bool Lookup(const char *pszName, std::vector<CAddress>& vaddr, int nServices, int nMaxSolutions, bool fAllowLookup = false, int portDefault = 0, bool fAllowPort = false);
bool Lookup(const char *pszName, CAddress& addr, int nServices, bool fAllowLookup = false, int portDefault = 0, bool fAllowPort = false);
bool GetMyExternalIP(unsigned int& ipRet);
bool AddAddress(CAddress addr, int64 nTimePenalty=0, CAddrDB *pAddrDB=NULL);
bool AddLocalAddress(const CAddress &addr);
bool IsLocalAddress(const CAddress &addr);
const CAddress* GetLocalAddress(const CAddress &addrPartner);
void AddressCurrentlyConnected(const CAddress& addr);
CNode* FindNode(unsigned int ip);
CNode* ConnectNode(CAddress addrConnect, int64 nTimeout=0);
void AbandonRequests(void (*fn)(void*, CDataStream&), void* param1);
bool AnySubscribed(unsigned int nChannel);
void MapPort(bool fMapPort);
void DNSAddressSeed();
bool BindListenPort(std::string& strError=REF(std::string()));
void StartNode(void* parg);
bool StopNode();








//
// Message header
//  (4) message start
//  (12) command
//  (4) size
//  (4) checksum

extern char pchMessageStart[4];

class CMessageHeader
{
public:
    enum { COMMAND_SIZE=12 };
    char pchMessageStart[sizeof(::pchMessageStart)];
    char pchCommand[COMMAND_SIZE];
    unsigned int nMessageSize;
    unsigned int nChecksum;

    CMessageHeader()
    {
        memcpy(pchMessageStart, ::pchMessageStart, sizeof(pchMessageStart));
        memset(pchCommand, 0, sizeof(pchCommand));
        pchCommand[1] = 1;
        nMessageSize = -1;
        nChecksum = 0;
    }

    CMessageHeader(const char* pszCommand, unsigned int nMessageSizeIn)
    {
        memcpy(pchMessageStart, ::pchMessageStart, sizeof(pchMessageStart));
        strncpy(pchCommand, pszCommand, COMMAND_SIZE);
        nMessageSize = nMessageSizeIn;
        nChecksum = 0;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(FLATDATA(pchMessageStart));
        READWRITE(FLATDATA(pchCommand));
        READWRITE(nMessageSize);
        if (nVersion >= 209)
            READWRITE(nChecksum);
    )

    std::string GetCommand()
    {
        if (pchCommand[COMMAND_SIZE-1] == 0)
            return std::string(pchCommand, pchCommand + strlen(pchCommand));
        else
            return std::string(pchCommand, pchCommand + COMMAND_SIZE);
    }

    bool IsValid()
    {
        // Check start string
        if (memcmp(pchMessageStart, ::pchMessageStart, sizeof(pchMessageStart)) != 0)
            return false;

        // Check the command string for errors
        for (char* p1 = pchCommand; p1 < pchCommand + COMMAND_SIZE; p1++)
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
};






static const unsigned char pchIPv4[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };

class CAddress
{
private:
    // TCP/IP port in network byte order
    uint16_t port;

    // IPv6 address (possibly containing an IPv4 mapped address)
    unsigned char ip[16];

public:
    uint64 nServices;

    // disk and network only
    unsigned int nTime;

    // memory only
    unsigned int nLastTry;


    // portIn is in network byte order
    void Init(struct in_addr* pipv4Addr, unsigned short portIn, uint64 nServicesIn=NODE_NETWORK)
    {
        Init();
        if (pipv4Addr != NULL)
        {
            memcpy(ip,    pchIPv4, 12);
            memcpy(ip+12, pipv4Addr, 4);
        }
        port = portIn;
        nServices = nServicesIn;
    }

    void Init(struct sockaddr_in* paddr, uint64 nServicesIn=NODE_NETWORK)
    {
        Init(&paddr->sin_addr, paddr->sin_port, nServicesIn);
    }

    CAddress()
    {
        Init();
    }

    // construct an IPv4 address - portIn is in host byte order
    CAddress(struct in_addr* pipv4Addr, unsigned short portIn=0, uint64 nServicesIn=NODE_NETWORK)
    {
        if (portIn == 0)
            portIn = GetDefaultPort();
        Init(pipv4Addr, htons(portIn), nServicesIn);
    }

    // construct an IPv4 address - portIn is in host byte order
    CAddress(struct sockaddr_in* paddr, uint64 nServicesIn=NODE_NETWORK)
    {
        Init(paddr, nServicesIn);
    }

    explicit CAddress(const char* pszIn, int portIn, bool fNameLookup = false, uint64 nServicesIn=NODE_NETWORK)
    {
        Init();
        Lookup(pszIn, *this, nServicesIn, fNameLookup, portIn);
    }

    explicit CAddress(const char* pszIn, bool fNameLookup = false, uint64 nServicesIn=NODE_NETWORK)
    {
        Init();
        Lookup(pszIn, *this, nServicesIn, fNameLookup, 0, true);
    }

    explicit CAddress(std::string strIn, int portIn, bool fNameLookup = false, uint64 nServicesIn=NODE_NETWORK)
    {
        Init();
        Lookup(strIn.c_str(), *this, nServicesIn, fNameLookup, portIn);
    }

    explicit CAddress(std::string strIn, bool fNameLookup = false, uint64 nServicesIn=NODE_NETWORK)
    {
        Init();
        Lookup(strIn.c_str(), *this, nServicesIn, fNameLookup, 0, true);
    }

    void Init()
    {
        nServices = NODE_NETWORK;
        memset(ip, 0, 16);
        port = htons(GetDefaultPort());
        nTime = 100000000;
        nLastTry = 0;
    }

    IMPLEMENT_SERIALIZE
    (
        if (fRead)
            const_cast<CAddress*>(this)->Init();
        if (nType & SER_DISK)
            READWRITE(nVersion);
        if ((nType & SER_DISK) || (nVersion >= 31402 && !(nType & SER_GETHASH)))
            READWRITE(nTime);
        READWRITE(nServices);
        READWRITE(FLATDATA(ip));
        READWRITE(port);
    )

    bool ConnectSocket(SOCKET& hSocketRet, int nTimeout=nConnectTimeout) const;

    int inline GetPort() const
    {
        return ntohs(port);
    }

    void SetPort(unsigned short portIn)
    {
        port = htons(portIn);
    }

    friend inline bool operator==(const CAddress& a, const CAddress& b)
    {
        return (memcmp(a.ip, b.ip, sizeof(a.ip)) == 0 &&
                a.port == b.port);
    }

    friend inline int CompareIP(const CAddress& a, const CAddress& b)
    {
        return memcmp(a.ip, b.ip, sizeof(a.ip));
    }

    friend inline bool operator!=(const CAddress& a, const CAddress& b)
    {
        return (!(a == b));
    }

    friend inline bool operator<(const CAddress& a, const CAddress& b)
    {
        int ret = memcmp(a.ip, b.ip, sizeof(a.ip));
        if (ret < 0)
            return true;
        return false;
    }

    std::vector<unsigned char> GetKey() const
    {
        CDataStream ss;
        ss.reserve(18);
        ss << FLATDATA(ip) << port;

        #if defined(_MSC_VER) && _MSC_VER < 1300
        return std::vector<unsigned char>((unsigned char*)&ss.begin()[0], (unsigned char*)&ss.end()[0]);
        #else
        return std::vector<unsigned char>(ss.begin(), ss.end());
        #endif
    }
    uint256 GetIPAsUint256() const
    {
       uint256 uint256 = 0;
       memcpy(&uint256, ip, 16);
       return uint256;
    }

    struct sockaddr_in GetSockAddrIPv4() const
    {
        struct sockaddr_in sockaddr;
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sin_family = AF_INET;
        memcpy(&sockaddr.sin_addr.s_addr, ip+12, 4);
        sockaddr.sin_port = port;
        return sockaddr;
    }

    // IPv4-mapped addresses (::ffff:0:0/96)
    bool IsIPv4() const
    {
        return (memcmp(ip, pchIPv4, sizeof(pchIPv4)) == 0);
    }

    // IPv4 private networks (10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16)
    bool IsRFC1918() const
    {
        return IsIPv4() && (GetByte(3) == 10 ||
            (GetByte(3) == 192 && GetByte(2) == 168) ||
            (GetByte(3) == 172 &&
                (GetByte(2) >= 16 && GetByte(2) <= 31)));
    }

    // IPv4 auto configuration (169.254.x.x)
    bool IsRFC3927() const
    {
        return IsIPv4() && (GetByte(3) == 169 && GetByte(2) == 254);
    }

    // IPv6 documentation address (2001:db8::/32)
    bool IsRFC3849() const
    {
        return GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x0D && GetByte(12) == 0xB8;
    }

    // 6to4 tunneling (2002::/16)
    bool Is6to4() const
    {
        return (GetByte(15) == 0x20 && GetByte(14) == 0x02);
    }

    // IPv6 Well-known prefix (64:ff9b::/96)
    bool IsRFC6052() const
    {
        static const unsigned char pchRFC6052[] = {0,0x64,0xFF,0x9B,0,0,0,0,0,0,0,0};
        return (memcmp(ip, pchRFC6052, sizeof(pchRFC6052)) == 0);
    }

    // IPv6 Teredo tunneling (2001::/32)
    bool IsRFC4380() const
    {
        return (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0 && GetByte(12) == 0);
    }

    // IPv6 stateless auto configuration (fe80::/64)
    bool IsRFC4862() const
    {
        static const unsigned char pchRFC4862[] = {0xFE,0x80,0,0,0,0,0,0};
        return (memcmp(ip, pchRFC4862, sizeof(pchRFC4862)) == 0);
    }

    // IPv6 unique local address (fc00::/7)
    bool IsRFC4193() const
    {
        return ((GetByte(15) & 0xFE) == 0xFC);
    }

    // IPv4-translated addresses (::ffff:0:0:0/96)
    bool IsRFC6145() const
    {
        static const unsigned char pchRFC6145[] = {0,0,0,0,0,0,0,0,0xFF,0xFF,0,0};
        return (memcmp(ip, pchRFC6145, sizeof(pchRFC6145)) == 0);
    }

    // IPv6 and IPv4 loopback addresses
    bool IsLocal() const
    {
        // IPv4 loopback
       if (IsIPv4() && (GetByte(3) == 127 || GetByte(3) == 0))
           return true;

       // IPv6 loopback (::1/128)
       static const unsigned char pchLocal[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
       if (memcmp(ip, pchLocal, 16) == 0)
           return true;

       return false;
    }

    bool IsRoutable() const
    {
        return IsValid() && !(IsRFC1918() || IsRFC3927() || IsRFC4862() || IsRFC4193() || IsLocal());
    }

    bool IsMulticast() const
    {
        return    (IsIPv4() && (GetByte(3) & 0xF0) == 0xE0)
               || (GetByte(15) == 0xFF);
    }

    bool IsValid() const
    {
        if (port == htons(USHRT_MAX))
            return false;

        // Clean up 3-byte shifted addresses caused by garbage in size field
        // of addr messages from versions before 0.2.9 checksum.
        // Two consecutive addr messages look like this:
        // header20 vectorlen3 addr26 addr26 addr26 header20 vectorlen3 addr26 addr26 addr26...
        // so if the first length field is garbled, it reads the second batch
        // of addr misaligned by 3 bytes.
        if (memcmp(ip, pchIPv4+3, sizeof(pchIPv4)-3) == 0)
            return false;

        // unspecified IPv6 address (::/128)
        unsigned char ipNone[16] = {};
        if (memcmp(ip, ipNone, 16) == 0)
            return false;

        // documentation IPv6 address
        if (IsRFC3849())
            return false;

        if (IsIPv4())
        {
            // INADDR_NONE
            uint32_t ipNone = INADDR_NONE;
            if (memcmp(ip+12, &ipNone, 4) == 0)
                return false;

            // 0
            ipNone = 0;
            if (memcmp(ip+12, &ipNone, 4) == 0)
                return false;
        }

        return true;
    }

    unsigned char GetByte(int n) const
    {
        return ip[15-n];
    }

    std::vector<unsigned char> GetGroup() const;

    int64 GetRandomized() const
    {
        if (IsIPv4())
        {
            // reconstruct ip in reversed-byte order
            // (the original definition of the randomizer used network-order integers on little endian architecture)
            int64 ip = GetByte(0) << 24 + GetByte(1) << 16 + GetByte(2) << 8 + GetByte(3);
            return ip * 7789;
        }

        // for IPv6 addresses, use separate multipliers for each byte
        // these numbers are from the hexadecimal expansion of 3/Pi:
        static const int64 nByteMult[16] = 
            {0xF4764525, 0x75661FBE, 0xFA3B03BA, 0xEFCF4CA1, 0x4913E065, 0xDA655862, 0xFD7A1581, 0xCE19A812,
             0x92B6A557, 0x6374BC50, 0x096DC65F, 0x0EBA5B2B, 0x7D2CE0AB, 0x09BE7ADE, 0x5CC350EF, 0xC618E6C7};
        int64 nRet = 0;
        for (int n=0; n<16; n++)
            nRet += nByteMult[n]*GetByte(n);
        return nRet;
    }

    std::string ToStringIPPort() const
    {
        if (IsIPv4())
            return strprintf("%u.%u.%u.%u:%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0), ntohs(port));
        else
            return "[" + ToStringIP() + strprintf("]:%u", ntohs(port));
    }

    std::string ToStringIP() const
    {
        if (IsIPv4())
            return strprintf("%u.%u.%u.%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0));
        else
            return strprintf("%x:%x:%x:%x:%x:%x:%x:%x",
                             GetByte(15) << 8 | GetByte(14), GetByte(13) << 8 | GetByte(12),
                             GetByte(11) << 8 | GetByte(10), GetByte(9) << 8 | GetByte(8),
                             GetByte(7) << 8 | GetByte(6), GetByte(5) << 8 | GetByte(4),
                             GetByte(3) << 8 | GetByte(2), GetByte(1) << 8 | GetByte(0));
    }

    std::string ToStringPort() const
    {
        return strprintf("%u", ntohs(port));
    }

    std::string ToString() const
    {
        return ToStringIPPort();
    }

    void print() const
    {
        printf("CAddress(%s)\n", ToString().c_str());
    }

    // for IPv6 partners:        for unknown/Teredo partners:      for IPv4 partners:
    // 0 - unroutable            // 0 - unroutable                 // 0 - unroutable
    // 1 - teredo                // 1 - teredo                     // 1 - ipv4
    // 2 - tunneled ipv6         // 2 - tunneled ipv6
    // 3 - ipv4                  // 3 - ipv6
    // 4 - ipv6                  // 4 - ipv4
    int GetReachability(const CAddress *paddrPartner = NULL) const
    {
        if (!IsValid() || !IsRoutable())
            return 0;
        if (paddrPartner && paddrPartner->IsIPv4())
            return IsIPv4() ? 1 : 0;
        if (IsRFC4380())
            return 1;
        if (Is6to4() || IsRFC6052())
            return 2;
        bool fRealIPv6 = paddrPartner && !paddrPartner->IsRFC4380() && paddrPartner->IsValid() && paddrPartner->IsRoutable();
        if (fRealIPv6)
            return IsIPv4() ? 3 : 4;
        else
            return IsIPv4() ? 4 : 3;
    }

    const CAddress* SelectCompatible(const std::set<CAddress>& setAddr) const
    {
        const CAddress* paddrBest = NULL;
        int nBestScore = 0;
        for (std::set<CAddress>::const_iterator mi = setAddr.begin(); mi != setAddr.end(); mi++)
        {
            int nScore = (*mi).GetReachability(this);
            if (nScore > nBestScore)
            {
                paddrBest = &(*mi);
                nBestScore = nScore;
            }
        }
        return paddrBest;
    }



#ifdef USE_IPV6
    // portIn is in network byte order
    void Init(struct in6_addr* pipv6Addr, unsigned short portIn, uint64 nServicesIn=NODE_NETWORK)
    {
        Init();
        if (pipv6Addr != NULL)
            memcpy(ip, pipv6Addr, 16);
        port = portIn;
        nServices  = nServicesIn;
    }

    void Init(struct sockaddr_in6* paddr, uint64 nServicesIn=NODE_NETWORK)
    {
        Init(&paddr->sin6_addr, paddr->sin6_port, nServicesIn);
    }

    // construct an IPv6 address - portIn is in host byte order
    CAddress(struct in6_addr* pipv6Addr, unsigned short portIn=0, uint64 nServicesIn=NODE_NETWORK)
    {
        if (portIn == 0)
            portIn = GetDefaultPort();
        Init(pipv6Addr, htons(portIn), nServicesIn);
    }

    // construct an IPv6 address - portIn is in host byte order
    CAddress(struct sockaddr_in6* paddr, uint64 nServicesIn=NODE_NETWORK)
    {
        Init(paddr, nServicesIn);
    }

    struct sockaddr_in6 GetSockAddrIPv6() const
    {
        struct sockaddr_in6 sockaddr;
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sin6_family = AF_INET6;
        memcpy(&sockaddr.sin6_addr.s6_addr, ip, 16);
        sockaddr.sin6_port = port;
        return sockaddr;
    }
#endif


};







enum
{
    MSG_TX = 1,
    MSG_BLOCK,
};

static const char* ppszTypeName[] =
{
    "ERROR",
    "tx",
    "block",
};

class CInv
{
public:
    int type;
    uint256 hash;

    CInv()
    {
        type = 0;
        hash = 0;
    }

    CInv(int typeIn, const uint256& hashIn)
    {
        type = typeIn;
        hash = hashIn;
    }

    CInv(const std::string& strType, const uint256& hashIn)
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

    IMPLEMENT_SERIALIZE
    (
        READWRITE(type);
        READWRITE(hash);
    )

    friend inline bool operator<(const CInv& a, const CInv& b)
    {
        return (a.type < b.type || (a.type == b.type && a.hash < b.hash));
    }

    bool IsKnownType() const
    {
        return (type >= 1 && type < ARRAYLEN(ppszTypeName));
    }

    const char* GetCommand() const
    {
        if (!IsKnownType())
            throw std::out_of_range(strprintf("CInv::GetCommand() : type=%d unknown type", type));
        return ppszTypeName[type];
    }

    std::string ToString() const
    {
        return strprintf("%s %s", GetCommand(), hash.ToString().substr(0,20).c_str());
    }

    void print() const
    {
        printf("CInv(%s)\n", ToString().c_str());
    }
};





class CRequestTracker
{
public:
    void (*fn)(void*, CDataStream&);
    void* param1;

    explicit CRequestTracker(void (*fnIn)(void*, CDataStream&)=NULL, void* param1In=NULL)
    {
        fn = fnIn;
        param1 = param1In;
    }

    bool IsNull()
    {
        return fn == NULL;
    }
};





extern bool fClient;
extern bool fAllowDNS;
extern uint64 nLocalServices;
extern std::set<CAddress> vaddrLocalHost;
extern CNode* pnodeLocalHost;
extern uint64 nLocalHostNonce;
extern boost::array<int, 10> vnThreadsRunning;
extern SOCKET hListenSocket;

extern std::vector<CNode*> vNodes;
extern CCriticalSection cs_vNodes;
extern std::map<std::vector<unsigned char>, CAddress> mapAddresses;
extern CCriticalSection cs_mapAddresses;
extern std::map<CInv, CDataStream> mapRelay;
extern std::deque<std::pair<int64, CInv> > vRelayExpiration;
extern CCriticalSection cs_mapRelay;
extern std::map<CInv, int64> mapAlreadyAskedFor;

// Settings
extern int fUseProxy;
extern CAddress addrProxy;






class CNode
{
public:
    // socket
    uint64 nServices;
    SOCKET hSocket;
    CDataStream vSend;
    CDataStream vRecv;
    CCriticalSection cs_vSend;
    CCriticalSection cs_vRecv;
    int64 nLastSend;
    int64 nLastRecv;
    int64 nLastSendEmpty;
    int64 nTimeConnected;
    unsigned int nHeaderStart;
    unsigned int nMessageStart;
    CAddress addr;
    int nVersion;
    std::string strSubVer;
    bool fClient;
    bool fInbound;
    bool fNetworkNode;
    bool fSuccessfullyConnected;
    bool fDisconnect;
protected:
    int nRefCount;
public:
    int64 nReleaseTime;
    std::map<uint256, CRequestTracker> mapRequests;
    CCriticalSection cs_mapRequests;
    uint256 hashContinue;
    CBlockIndex* pindexLastGetBlocksBegin;
    uint256 hashLastGetBlocksEnd;
    int nStartingHeight;

    // flood relay
    std::vector<CAddress> vAddrToSend;
    std::set<CAddress> setAddrKnown;
    bool fGetAddr;
    std::set<uint256> setKnown;

    // inventory based relay
    std::set<CInv> setInventoryKnown;
    std::vector<CInv> vInventoryToSend;
    CCriticalSection cs_inventory;
    std::multimap<int64, CInv> mapAskFor;

    // publish and subscription
    std::vector<char> vfSubscribe;


    CNode(SOCKET hSocketIn, CAddress addrIn, bool fInboundIn=false)
    {
        nServices = 0;
        hSocket = hSocketIn;
        vSend.SetType(SER_NETWORK);
        vSend.SetVersion(0);
        vRecv.SetType(SER_NETWORK);
        vRecv.SetVersion(0);
        // Version 0.2 obsoletes 20 Feb 2012
        if (GetTime() > 1329696000)
        {
            vSend.SetVersion(209);
            vRecv.SetVersion(209);
        }
        nLastSend = 0;
        nLastRecv = 0;
        nLastSendEmpty = GetTime();
        nTimeConnected = GetTime();
        nHeaderStart = -1;
        nMessageStart = -1;
        addr = addrIn;
        nVersion = 0;
        strSubVer = "";
        fClient = false; // set by version message
        fInbound = fInboundIn;
        fNetworkNode = false;
        fSuccessfullyConnected = false;
        fDisconnect = false;
        nRefCount = 0;
        nReleaseTime = 0;
        hashContinue = 0;
        pindexLastGetBlocksBegin = 0;
        hashLastGetBlocksEnd = 0;
        nStartingHeight = -1;
        fGetAddr = false;
        vfSubscribe.assign(256, false);

        // Be shy and don't send version until we hear
        if (!fInbound)
            PushVersion();
    }

    ~CNode()
    {
        if (hSocket != INVALID_SOCKET)
        {
            closesocket(hSocket);
            hSocket = INVALID_SOCKET;
        }
    }

private:
    CNode(const CNode&);
    void operator=(const CNode&);
public:


    int GetRefCount()
    {
        return std::max(nRefCount, 0) + (GetTime() < nReleaseTime ? 1 : 0);
    }

    CNode* AddRef(int64 nTimeout=0)
    {
        if (nTimeout != 0)
            nReleaseTime = std::max(nReleaseTime, GetTime() + nTimeout);
        else
            nRefCount++;
        return this;
    }

    void Release()
    {
        nRefCount--;
    }



    void AddAddressKnown(const CAddress& addr)
    {
        setAddrKnown.insert(addr);
    }

    void PushAddress(const CAddress& addr)
    {
        // Known checking here is only to save space from duplicates.
        // SendMessages will filter it again for knowns that were added
        // after addresses were pushed.
        if (addr.IsValid() && !setAddrKnown.count(addr))
            vAddrToSend.push_back(addr);
    }


    void AddInventoryKnown(const CInv& inv)
    {
        CRITICAL_BLOCK(cs_inventory)
            setInventoryKnown.insert(inv);
    }

    void PushInventory(const CInv& inv)
    {
        CRITICAL_BLOCK(cs_inventory)
            if (!setInventoryKnown.count(inv))
                vInventoryToSend.push_back(inv);
    }

    void AskFor(const CInv& inv)
    {
        // We're using mapAskFor as a priority queue,
        // the key is the earliest time the request can be sent
        int64& nRequestTime = mapAlreadyAskedFor[inv];
        printf("askfor %s   %"PRI64d"\n", inv.ToString().c_str(), nRequestTime);

        // Make sure not to reuse time indexes to keep things in the same order
        int64 nNow = (GetTime() - 1) * 1000000;
        static int64 nLastTime;
        nLastTime = nNow = std::max(nNow, ++nLastTime);

        // Each retry is 2 minutes after the last
        nRequestTime = std::max(nRequestTime + 2 * 60 * 1000000, nNow);
        mapAskFor.insert(std::make_pair(nRequestTime, inv));
    }



    void BeginMessage(const char* pszCommand)
    {
        cs_vSend.Enter();
        if (nHeaderStart != -1)
            AbortMessage();
        nHeaderStart = vSend.size();
        vSend << CMessageHeader(pszCommand, 0);
        nMessageStart = vSend.size();
        if (fDebug)
            printf("%s ", DateTimeStrFormat("%x %H:%M:%S", GetTime()).c_str());
        printf("sending: %s ", pszCommand);
    }

    void AbortMessage()
    {
        if (nHeaderStart == -1)
            return;
        vSend.resize(nHeaderStart);
        nHeaderStart = -1;
        nMessageStart = -1;
        cs_vSend.Leave();
        printf("(aborted)\n");
    }

    void EndMessage()
    {
        if (mapArgs.count("-dropmessagestest") && GetRand(atoi(mapArgs["-dropmessagestest"])) == 0)
        {
            printf("dropmessages DROPPING SEND MESSAGE\n");
            AbortMessage();
            return;
        }

        if (nHeaderStart == -1)
            return;

        // Set the size
        unsigned int nSize = vSend.size() - nMessageStart;
        memcpy((char*)&vSend[nHeaderStart] + offsetof(CMessageHeader, nMessageSize), &nSize, sizeof(nSize));

        // Set the checksum
        if (vSend.GetVersion() >= 209)
        {
            uint256 hash = Hash(vSend.begin() + nMessageStart, vSend.end());
            unsigned int nChecksum = 0;
            memcpy(&nChecksum, &hash, sizeof(nChecksum));
            assert(nMessageStart - nHeaderStart >= offsetof(CMessageHeader, nChecksum) + sizeof(nChecksum));
            memcpy((char*)&vSend[nHeaderStart] + offsetof(CMessageHeader, nChecksum), &nChecksum, sizeof(nChecksum));
        }

        printf("(%d bytes) ", nSize);
        printf("\n");

        nHeaderStart = -1;
        nMessageStart = -1;
        cs_vSend.Leave();
    }

    void EndMessageAbortIfEmpty()
    {
        if (nHeaderStart == -1)
            return;
        int nSize = vSend.size() - nMessageStart;
        if (nSize > 0)
            EndMessage();
        else
            AbortMessage();
    }



    void PushVersion()
    {
        /// when NTP implemented, change to just nTime = GetAdjustedTime()
        int64 nTime = (fInbound ? GetAdjustedTime() : GetTime());
        CAddress addrYou = (fUseProxy ? CAddress("0.0.0.0") : addr);
        CAddress addrMe("0.0.0.0");
        if (!fUseProxy)
        {
            const CAddress *paddrMe = GetLocalAddress(addr);
            if (paddrMe)
                addrMe = *paddrMe;
        }
        RAND_bytes((unsigned char*)&nLocalHostNonce, sizeof(nLocalHostNonce));
        PushMessage("version", VERSION, nLocalServices, nTime, addrYou, addrMe,
                    nLocalHostNonce, std::string(pszSubVer), nBestHeight);
    }




    void PushMessage(const char* pszCommand)
    {
        try
        {
            BeginMessage(pszCommand);
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1>
    void PushMessage(const char* pszCommand, const T1& a1)
    {
        try
        {
            BeginMessage(pszCommand);
            vSend << a1;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2)
    {
        try
        {
            BeginMessage(pszCommand);
            vSend << a1 << a2;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3)
    {
        try
        {
            BeginMessage(pszCommand);
            vSend << a1 << a2 << a3;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4)
    {
        try
        {
            BeginMessage(pszCommand);
            vSend << a1 << a2 << a3 << a4;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5)
    {
        try
        {
            BeginMessage(pszCommand);
            vSend << a1 << a2 << a3 << a4 << a5;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6)
    {
        try
        {
            BeginMessage(pszCommand);
            vSend << a1 << a2 << a3 << a4 << a5 << a6;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7)
    {
        try
        {
            BeginMessage(pszCommand);
            vSend << a1 << a2 << a3 << a4 << a5 << a6 << a7;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8)
    {
        try
        {
            BeginMessage(pszCommand);
            vSend << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8, const T9& a9)
    {
        try
        {
            BeginMessage(pszCommand);
            vSend << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8 << a9;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }


    void PushRequest(const char* pszCommand,
                     void (*fn)(void*, CDataStream&), void* param1)
    {
        uint256 hashReply;
        RAND_bytes((unsigned char*)&hashReply, sizeof(hashReply));

        CRITICAL_BLOCK(cs_mapRequests)
            mapRequests[hashReply] = CRequestTracker(fn, param1);

        PushMessage(pszCommand, hashReply);
    }

    template<typename T1>
    void PushRequest(const char* pszCommand, const T1& a1,
                     void (*fn)(void*, CDataStream&), void* param1)
    {
        uint256 hashReply;
        RAND_bytes((unsigned char*)&hashReply, sizeof(hashReply));

        CRITICAL_BLOCK(cs_mapRequests)
            mapRequests[hashReply] = CRequestTracker(fn, param1);

        PushMessage(pszCommand, hashReply, a1);
    }

    template<typename T1, typename T2>
    void PushRequest(const char* pszCommand, const T1& a1, const T2& a2,
                     void (*fn)(void*, CDataStream&), void* param1)
    {
        uint256 hashReply;
        RAND_bytes((unsigned char*)&hashReply, sizeof(hashReply));

        CRITICAL_BLOCK(cs_mapRequests)
            mapRequests[hashReply] = CRequestTracker(fn, param1);

        PushMessage(pszCommand, hashReply, a1, a2);
    }



    void PushGetBlocks(CBlockIndex* pindexBegin, uint256 hashEnd);
    bool IsSubscribed(unsigned int nChannel);
    void Subscribe(unsigned int nChannel, unsigned int nHops=0);
    void CancelSubscribe(unsigned int nChannel);
    void CloseSocketDisconnect();
    void Cleanup();
};










inline void RelayInventory(const CInv& inv)
{
    // Put on lists to offer to the other nodes
    CRITICAL_BLOCK(cs_vNodes)
        BOOST_FOREACH(CNode* pnode, vNodes)
            pnode->PushInventory(inv);
}

template<typename T>
void RelayMessage(const CInv& inv, const T& a)
{
    CDataStream ss(SER_NETWORK);
    ss.reserve(10000);
    ss << a;
    RelayMessage(inv, ss);
}

template<>
inline void RelayMessage<>(const CInv& inv, const CDataStream& ss)
{
    CRITICAL_BLOCK(cs_mapRelay)
    {
        // Expire old relay messages
        while (!vRelayExpiration.empty() && vRelayExpiration.front().first < GetTime())
        {
            mapRelay.erase(vRelayExpiration.front().second);
            vRelayExpiration.pop_front();
        }

        // Save original serialized message so newer versions are preserved
        mapRelay[inv] = ss;
        vRelayExpiration.push_back(std::make_pair(GetTime() + 15 * 60, inv));
    }

    RelayInventory(inv);
}








//
// Templates for the publish and subscription system.
// The object being published as T& obj needs to have:
//   a set<CAddress> setSources member
//   specializations of AdvertInsert and AdvertErase
// Currently implemented for CTable and CProduct.
//

template<typename T>
void AdvertStartPublish(CNode* pfrom, unsigned int nChannel, unsigned int nHops, T& obj)
{
    // Add to sources
    obj.setSources.insert(pfrom->addr);

    if (!AdvertInsert(obj))
        return;

    // Relay
    CRITICAL_BLOCK(cs_vNodes)
        BOOST_FOREACH(CNode* pnode, vNodes)
            if (pnode != pfrom && (nHops < PUBLISH_HOPS || pnode->IsSubscribed(nChannel)))
                pnode->PushMessage("publish", nChannel, nHops, obj);
}

template<typename T>
void AdvertStopPublish(CNode* pfrom, unsigned int nChannel, unsigned int nHops, T& obj)
{
    uint256 hash = obj.GetHash();

    CRITICAL_BLOCK(cs_vNodes)
        BOOST_FOREACH(CNode* pnode, vNodes)
            if (pnode != pfrom && (nHops < PUBLISH_HOPS || pnode->IsSubscribed(nChannel)))
                pnode->PushMessage("pub-cancel", nChannel, nHops, hash);

    AdvertErase(obj);
}

template<typename T>
void AdvertRemoveSource(CNode* pfrom, unsigned int nChannel, unsigned int nHops, T& obj)
{
    // Remove a source
    obj.setSources.erase(pfrom->addr);

    // If no longer supported by any sources, cancel it
    if (obj.setSources.empty())
        AdvertStopPublish(pfrom, nChannel, nHops, obj);
}

#endif
