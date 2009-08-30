// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

class CMessageHeader;
class CAddress;
class CInv;
class CRequestTracker;
class CNode;



static const unsigned short DEFAULT_PORT = htons(8333);
static const unsigned int PUBLISH_HOPS = 5;
enum
{
    NODE_NETWORK = (1 << 0),
};






bool ConnectSocket(const CAddress& addrConnect, SOCKET& hSocketRet);
bool GetMyExternalIP(unsigned int& ipRet);
bool AddAddress(CAddrDB& addrdb, const CAddress& addr);
CNode* FindNode(unsigned int ip);
CNode* ConnectNode(CAddress addrConnect, int64 nTimeout=0);
void AbandonRequests(void (*fn)(void*, CDataStream&), void* param1);
bool AnySubscribed(unsigned int nChannel);
void ThreadBitcoinMiner(void* parg);
bool StartNode(string& strError=REF(string()));
bool StopNode();
void CheckForShutdown(int n);









//
// Message header
//  (4) message start
//  (12) command
//  (4) size

// The message start string is designed to be unlikely to occur in normal data.
// The characters are rarely used upper ascii, not valid as UTF-8, and produce
// a large 4-byte int at any alignment.
static const char pchMessageStart[4] = { 0xf9, 0xbe, 0xb4, 0xd9 };

class CMessageHeader
{
public:
    enum { COMMAND_SIZE=12 };
    char pchMessageStart[sizeof(::pchMessageStart)];
    char pchCommand[COMMAND_SIZE];
    unsigned int nMessageSize;

    CMessageHeader()
    {
        memcpy(pchMessageStart, ::pchMessageStart, sizeof(pchMessageStart));
        memset(pchCommand, 0, sizeof(pchCommand));
        pchCommand[1] = 1;
        nMessageSize = -1;
    }

    CMessageHeader(const char* pszCommand, unsigned int nMessageSizeIn)
    {
        memcpy(pchMessageStart, ::pchMessageStart, sizeof(pchMessageStart));
        strncpy(pchCommand, pszCommand, COMMAND_SIZE);
        nMessageSize = nMessageSizeIn;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(FLATDATA(pchMessageStart));
        READWRITE(FLATDATA(pchCommand));
        READWRITE(nMessageSize);
    )

    string GetCommand()
    {
        if (pchCommand[COMMAND_SIZE-1] == 0)
            return string(pchCommand, pchCommand + strlen(pchCommand));
        else
            return string(pchCommand, pchCommand + COMMAND_SIZE);
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
        if (nMessageSize > 0x10000000)
        {
            printf("CMessageHeader::IsValid() : nMessageSize too large %u\n", nMessageSize);
            return false;
        }

        return true;
    }
};






static const unsigned char pchIPv4[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };

class CAddress
{
public:
    uint64 nServices;
    unsigned char pchReserved[12];
    unsigned int ip;
    unsigned short port;

    // disk only
    unsigned int nTime;

    // memory only
    unsigned int nLastFailed;

    CAddress()
    {
        nServices = 0;
        memcpy(pchReserved, pchIPv4, sizeof(pchReserved));
        ip = 0;
        port = DEFAULT_PORT;
        nTime = GetAdjustedTime();
        nLastFailed = 0;
    }

    CAddress(unsigned int ipIn, unsigned short portIn=DEFAULT_PORT, uint64 nServicesIn=0)
    {
        nServices = nServicesIn;
        memcpy(pchReserved, pchIPv4, sizeof(pchReserved));
        ip = ipIn;
        port = portIn;
        nTime = GetAdjustedTime();
        nLastFailed = 0;
    }

    explicit CAddress(const struct sockaddr_in& sockaddr, uint64 nServicesIn=0)
    {
        nServices = nServicesIn;
        memcpy(pchReserved, pchIPv4, sizeof(pchReserved));
        ip = sockaddr.sin_addr.s_addr;
        port = sockaddr.sin_port;
        nTime = GetAdjustedTime();
        nLastFailed = 0;
    }

    explicit CAddress(const char* pszIn, uint64 nServicesIn=0)
    {
        nServices = nServicesIn;
        memcpy(pchReserved, pchIPv4, sizeof(pchReserved));
        ip = 0;
        port = DEFAULT_PORT;
        nTime = GetAdjustedTime();
        nLastFailed = 0;

        char psz[100];
        if (strlen(pszIn) > ARRAYLEN(psz)-1)
            return;
        strcpy(psz, pszIn);
        unsigned int a, b, c, d, e;
        if (sscanf(psz, "%u.%u.%u.%u:%u", &a, &b, &c, &d, &e) < 4)
            return;
        char* pszPort = strchr(psz, ':');
        if (pszPort)
        {
            *pszPort++ = '\0';
            port = htons(atoi(pszPort));
        }
        ip = inet_addr(psz);
    }

    IMPLEMENT_SERIALIZE
    (
        if (nType & SER_DISK)
        {
            READWRITE(nVersion);
            READWRITE(nTime);
        }
        READWRITE(nServices);
        READWRITE(FLATDATA(pchReserved));
        READWRITE(ip);
        READWRITE(port);
    )

    friend inline bool operator==(const CAddress& a, const CAddress& b)
    {
        return (memcmp(a.pchReserved, b.pchReserved, sizeof(a.pchReserved)) == 0 &&
                a.ip   == b.ip &&
                a.port == b.port);
    }

    friend inline bool operator<(const CAddress& a, const CAddress& b)
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

    vector<unsigned char> GetKey() const
    {
        CDataStream ss;
        ss.reserve(18);
        ss << FLATDATA(pchReserved) << ip << port;

        #if defined(_MSC_VER) && _MSC_VER < 1300
        return vector<unsigned char>((unsigned char*)&ss.begin()[0], (unsigned char*)&ss.end()[0]);
        #else
        return vector<unsigned char>(ss.begin(), ss.end());
        #endif
    }

    struct sockaddr_in GetSockAddr() const
    {
        struct sockaddr_in sockaddr;
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = ip;
        sockaddr.sin_port = port;
        return sockaddr;
    }

    bool IsIPv4() const
    {
        return (memcmp(pchReserved, pchIPv4, sizeof(pchIPv4)) == 0);
    }

    bool IsRoutable() const
    {
        return !(GetByte(3) == 10 || (GetByte(3) == 192 && GetByte(2) == 168) || GetByte(3) == 127 || GetByte(3) == 0);
    }

    unsigned char GetByte(int n) const
    {
        return ((unsigned char*)&ip)[3-n];
    }

    string ToStringIPPort() const
    {
        return strprintf("%u.%u.%u.%u:%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0), ntohs(port));
    }

    string ToStringIP() const
    {
        return strprintf("%u.%u.%u.%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0));
    }

    string ToString() const
    {
        return strprintf("%u.%u.%u.%u:%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0), ntohs(port));
        //return strprintf("%u.%u.%u.%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0));
    }

    void print() const
    {
        printf("CAddress(%s)\n", ToString().c_str());
    }
};







enum
{
    MSG_TX = 1,
    MSG_BLOCK,
    MSG_REVIEW,
    MSG_PRODUCT,
    MSG_TABLE,
};

static const char* ppszTypeName[] =
{
    "ERROR",
    "tx",
    "block",
    "review",
    "product",
    "table",
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

    CInv(const string& strType, const uint256& hashIn)
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
            throw std::out_of_range(strprintf("CInv::GetCommand() : type=% unknown type", type));
        return ppszTypeName[type];
    }

    string ToString() const
    {
        return strprintf("%s %s", GetCommand(), hash.ToString().substr(0,14).c_str());
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
extern uint64 nLocalServices;
extern CAddress addrLocalHost;
extern CNode* pnodeLocalHost;
extern bool fShutdown;
extern array<bool, 10> vfThreadRunning;
extern vector<CNode*> vNodes;
extern CCriticalSection cs_vNodes;
extern map<vector<unsigned char>, CAddress> mapAddresses;
extern CCriticalSection cs_mapAddresses;
extern map<CInv, CDataStream> mapRelay;
extern deque<pair<int64, CInv> > vRelayExpiration;
extern CCriticalSection cs_mapRelay;
extern map<CInv, int64> mapAlreadyAskedFor;
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
    unsigned int nPushPos;
    CAddress addr;
    int nVersion;
    bool fClient;
    bool fInbound;
    bool fNetworkNode;
    bool fDisconnect;
protected:
    int nRefCount;
public:
    int64 nReleaseTime;
    map<uint256, CRequestTracker> mapRequests;
    CCriticalSection cs_mapRequests;

    // flood
    vector<CAddress> vAddrToSend;
    set<CAddress> setAddrKnown;

    // inventory based relay
    set<CInv> setInventoryKnown;
    set<CInv> setInventoryKnown2;
    vector<CInv> vInventoryToSend;
    CCriticalSection cs_inventory;
    multimap<int64, CInv> mapAskFor;

    // publish and subscription
    vector<char> vfSubscribe;


    CNode(SOCKET hSocketIn, CAddress addrIn, bool fInboundIn=false)
    {
        nServices = 0;
        hSocket = hSocketIn;
        vSend.SetType(SER_NETWORK);
        vRecv.SetType(SER_NETWORK);
        nPushPos = -1;
        addr = addrIn;
        nVersion = 0;
        fClient = false; // set by version message
        fInbound = fInboundIn;
        fNetworkNode = false;
        fDisconnect = false;
        nRefCount = 0;
        nReleaseTime = 0;
        vfSubscribe.assign(256, false);

        // Push a version message
        /// when NTP implemented, change to just nTime = GetAdjustedTime()
        int64 nTime = (fInbound ? GetAdjustedTime() : GetTime());
        PushMessage("version", VERSION, nLocalServices, nTime, addr);
    }

    ~CNode()
    {
        if (hSocket != INVALID_SOCKET)
            closesocket(hSocket);
    }

private:
    CNode(const CNode&);
    void operator=(const CNode&);
public:


    bool ReadyToDisconnect()
    {
        return fDisconnect || GetRefCount() <= 0;
    }

    int GetRefCount()
    {
        return max(nRefCount, 0) + (GetTime() < nReleaseTime ? 1 : 0);
    }

    void AddRef(int64 nTimeout=0)
    {
        if (nTimeout != 0)
            nReleaseTime = max(nReleaseTime, GetTime() + nTimeout);
        else
            nRefCount++;
    }

    void Release()
    {
        nRefCount--;
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
        printf("askfor %s  %I64d\n", inv.ToString().c_str(), nRequestTime);

        // Make sure not to reuse time indexes to keep things in the same order
        int64 nNow = (GetTime() - 1) * 1000000;
        static int64 nLastTime;
        nLastTime = nNow = max(nNow, ++nLastTime);

        // Each retry is 2 minutes after the last
        nRequestTime = max(nRequestTime + 2 * 60 * 1000000, nNow);
        mapAskFor.insert(make_pair(nRequestTime, inv));
    }



    void BeginMessage(const char* pszCommand)
    {
        EnterCriticalSection(&cs_vSend);
        if (nPushPos != -1)
            AbortMessage();
        nPushPos = vSend.size();
        vSend << CMessageHeader(pszCommand, 0);
        printf("sending: %-12s ", pszCommand);
    }

    void AbortMessage()
    {
        if (nPushPos == -1)
            return;
        vSend.resize(nPushPos);
        nPushPos = -1;
        LeaveCriticalSection(&cs_vSend);
        printf("(aborted)\n");
    }

    void EndMessage()
    {
        extern int nDropMessagesTest;
        if (nDropMessagesTest > 0 && GetRand(nDropMessagesTest) == 0)
        {
            printf("dropmessages DROPPING SEND MESSAGE\n");
            AbortMessage();
            return;
        }

        if (nPushPos == -1)
            return;

        // Patch in the size
        unsigned int nSize = vSend.size() - nPushPos - sizeof(CMessageHeader);
        memcpy((char*)&vSend[nPushPos] + offsetof(CMessageHeader, nMessageSize), &nSize, sizeof(nSize));

        printf("(%d bytes)  ", nSize);
        //for (int i = nPushPos+sizeof(CMessageHeader); i < min(vSend.size(), nPushPos+sizeof(CMessageHeader)+20U); i++)
        //    printf("%02x ", vSend[i] & 0xff);
        printf("\n");

        nPushPos = -1;
        LeaveCriticalSection(&cs_vSend);
    }

    void EndMessageAbortIfEmpty()
    {
        if (nPushPos == -1)
            return;
        int nSize = vSend.size() - nPushPos - sizeof(CMessageHeader);
        if (nSize > 0)
            EndMessage();
        else
            AbortMessage();
    }

    const char* GetMessageCommand() const
    {
        if (nPushPos == -1)
            return "";
        return &vSend[nPushPos] + offsetof(CMessageHeader, pchCommand);
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



    bool IsSubscribed(unsigned int nChannel);
    void Subscribe(unsigned int nChannel, unsigned int nHops=0);
    void CancelSubscribe(unsigned int nChannel);
    void Disconnect();
};










inline void RelayInventory(const CInv& inv)
{
    // Put on lists to offer to the other nodes
    CRITICAL_BLOCK(cs_vNodes)
        foreach(CNode* pnode, vNodes)
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
        vRelayExpiration.push_back(make_pair(GetTime() + 15 * 60, inv));
    }

    RelayInventory(inv);
}








//
// Templates for the publish and subscription system.
// The object being published as T& obj needs to have:
//   a set<unsigned int> setSources member
//   specializations of AdvertInsert and AdvertErase
// Currently implemented for CTable and CProduct.
//

template<typename T>
void AdvertStartPublish(CNode* pfrom, unsigned int nChannel, unsigned int nHops, T& obj)
{
    // Add to sources
    obj.setSources.insert(pfrom->addr.ip);

    if (!AdvertInsert(obj))
        return;

    // Relay
    CRITICAL_BLOCK(cs_vNodes)
        foreach(CNode* pnode, vNodes)
            if (pnode != pfrom && (nHops < PUBLISH_HOPS || pnode->IsSubscribed(nChannel)))
                pnode->PushMessage("publish", nChannel, nHops, obj);
}

template<typename T>
void AdvertStopPublish(CNode* pfrom, unsigned int nChannel, unsigned int nHops, T& obj)
{
    uint256 hash = obj.GetHash();

    CRITICAL_BLOCK(cs_vNodes)
        foreach(CNode* pnode, vNodes)
            if (pnode != pfrom && (nHops < PUBLISH_HOPS || pnode->IsSubscribed(nChannel)))
                pnode->PushMessage("pub-cancel", nChannel, nHops, hash);

    AdvertErase(obj);
}

template<typename T>
void AdvertRemoveSource(CNode* pfrom, unsigned int nChannel, unsigned int nHops, T& obj)
{
    // Remove a source
    obj.setSources.erase(pfrom->addr.ip);

    // If no longer supported by any sources, cancel it
    if (obj.setSources.empty())
        AdvertStopPublish(pfrom, nChannel, nHops, obj);
}
