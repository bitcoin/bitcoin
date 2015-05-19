// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_H
#define BITCOIN_NET_H

#include "bloom.h"
#include "compat.h"
#include "hash.h"
#include "limitedmap.h"
#include "mruset.h"
#include "netbase.h"
#include "protocol.h"
#include "sync.h"
#include "uint256.h"
#include "util.h"
#include "addrman.h"

#include <deque>
#include <stdint.h>

#ifndef WIN32
#include <arpa/inet.h>
#endif

#include <boost/foreach.hpp>
#include <boost/signals2/signal.hpp>
#include <openssl/rand.h>

class CAddrMan;
class CNetParams;
class Credits_CBlockIndex;
class CNode;

namespace boost {
    class thread_group;
}

/** The maximum number of entries in an 'inv' protocol message */
static const unsigned int MAX_INV_SZ = 50000;
/** -upnp default */
#ifdef USE_UPNP
static const bool DEFAULT_UPNP = USE_UPNP;
#else
static const bool DEFAULT_UPNP = false;
#endif

inline unsigned int ReceiveFloodSize() { return 1000*GetArg("-maxreceivebuffer", 5*1000); }
inline unsigned int SendBufferSize() { return 1000*GetArg("-maxsendbuffer", 1*1000); }

void AddOneShot(std::string strDest, CNetParams * netParams);
bool RecvLine(SOCKET hSocket, std::string& strLine, CNetParams * netParams);
bool GetMyExternalIP(CNetAddr& ipRet, CNetParams * netParams);
CNode* FindNode(const CNetAddr& ip, CNetParams * netParams);
CNode* FindNode(const CService& ip, CNetParams * netParams);
CNode* ConnectNode(CAddress addrConnect, const char *strDest, CNetParams * netParams);
void MapPort(bool fUseUPnP, CNetParams * netParams);
bool BindListenPort(const CService &bindAddr, std::string& strError, CNetParams * netParams);
void StartNode(boost::thread_group& threadGroup);
bool StopNode();
void SocketSendData(CNode *pnode);

typedef int NodeId;

// Signals for message handling
struct CNodeSignals
{
    boost::signals2::signal<int ()> GetHeight;
    boost::signals2::signal<bool (CNode*)> ProcessMessages;
    boost::signals2::signal<bool (CNode*, bool)> SendMessages;
    boost::signals2::signal<void (NodeId, const CNode*)> InitializeNode;
    boost::signals2::signal<void (NodeId)> FinalizeNode;
};


enum
{
    LOCAL_NONE,   // unknown
    LOCAL_IF,     // address a local interface listens on
    LOCAL_BIND,   // address explicit bound to
    LOCAL_UPNP,   // address reported by UPnP
    LOCAL_HTTP,   // address reported by whatismyip.com and similar
    LOCAL_MANUAL, // address explicitly specified (-externalip=)

    LOCAL_MAX
};

void SetLimited(enum Network net, bool fLimited, CNetParams * netParams);
bool IsLimited(enum Network net, CNetParams * netParams);
bool IsLimited(const CService& addr, CNetParams * netParams);
bool AddLocal(const CService& addr, int nScore, CNetParams * netParams);
bool SeenLocal(const CService& addr, CNetParams * netParams);
bool IsLocal(const CService& addr, CNetParams * netParams);
bool GetLocal(CService &addr, const CNetAddr *paddrPeer, CNetParams * netParams);
bool IsReachable(const CService &addr, CNetParams * netParams);
void SetReachable(enum Network net, bool fFlag, CNetParams * netParams);
CAddress GetLocalAddress(const CService *paddrPeer, CNetParams * netParams);

struct LocalServiceInfo {
    int nScore;
    int nPort;
};

class CNodeStats
{
public:
    NodeId nodeid;
    uint64_t nServices;
    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nTimeConnected;
    std::string addrName;
    int nVersion;
    std::string cleanSubVer;
    bool fInbound;
    int nStartingHeight;
    uint64_t nSendBytes;
    uint64_t nRecvBytes;
    bool fSyncNode;
    double dPingTime;
    double dPingWait;
    std::string addrLocal;
};


class CNetParams{
public:
	CNetParams() : mapAlreadyAskedFor(MAX_INV_SZ){
		pnodeSync = NULL;
		semOutbound = NULL;
		nLastNodeId = 0;
		fDiscover = true;
		fListen = true;
		nLocalServices = NODE_NETWORK;
		vfReachable[NET_UNROUTABLE] = false;
		vfReachable[NET_IPV4] = false;
		vfReachable[NET_IPV6] = false;
		vfReachable[NET_TOR] = false;
		vfLimited[NET_UNROUTABLE] = false;
		vfLimited[NET_IPV4] = false;
		vfLimited[NET_IPV6] = false;
		vfLimited[NET_TOR] = false;
		pnodeLocalHost = NULL;
		nLocalHostNonce = 0;
		nMaxConnections = 125;
    }

	CAddrMan addrman;

	vector<CNode*> vNodes;
	CCriticalSection cs_vNodes;

	deque<string> vOneShots;
	CCriticalSection cs_vOneShots;

	CNode* pnodeSync;

	CSemaphore* semOutbound;

	std::vector<SOCKET> vhListenSocket;
	list<CNode*> vNodesDisconnected;

	map<CInv, CDataStream> mapRelay;
	deque<pair<int64_t, CInv> > vRelayExpiration;
	CCriticalSection cs_mapRelay;
	limitedmap<CInv, int64_t> mapAlreadyAskedFor;

	set<CNetAddr> setservAddNodeAddresses;
	CCriticalSection cs_setservAddNodeAddresses;

	vector<std::string> vAddedNodes;
	CCriticalSection cs_vAddedNodes;

	NodeId nLastNodeId;
	CCriticalSection cs_nLastNodeId;

	CCriticalSection cs_mapLocalHost;
	map<CNetAddr, LocalServiceInfo> mapLocalHost;

	bool fDiscover;
	bool fListen;
	uint64_t nLocalServices;
	bool vfReachable[NET_MAX];
	bool vfLimited[NET_MAX];
	CNode* pnodeLocalHost;
	uint64_t nLocalHostNonce;
	int nMaxConnections;

	// Signals for message handling
	CNodeSignals g_signals;

	virtual const CChainParams& Params() = 0;
	virtual const std::string LogPrefix() = 0;
	virtual const char * DebugCategory() = 0;
	virtual const std::string ClientName() = 0;
	virtual const int& ProtocolVersion() = 0;
	virtual unsigned short GetListenPort() = 0;
	CNodeSignals& GetNodeSignals() { return g_signals; }
	virtual void RegisterNodeSignals() = 0;
	virtual void UnregisterNodeSignals() = 0;
	unsigned short GetDefaultPort() {
		return Params().GetDefaultPort();
	}
	unsigned short GetRPCPort() {
		return Params().RPCPort();
	}
	const vector<CDNSSeedData>& GetDNSSeeds() {
		return Params().DNSSeeds();
	}
	const vector<CAddress>& GetFixedSeeds() {
		return Params().FixedSeeds();
	}
	const std::string AddrFileName() {
		return Params().AddrFileName();
	}
	const MessageStartChars& MessageStart() {
		return Params().MessageStart();
	}
	int ClientVersion() {
		return Params().ClientVersion();
	}
};

CNetParams * Credits_NetParams() ;
CNetParams * Bitcoin_NetParams();


class CNetMessage {
public:
    bool in_data;                   // parsing header (false) or data (true)

    CDataStream hdrbuf;             // partially received header
    CMessageHeader hdr;             // complete header
    unsigned int nHdrPos;

    CDataStream vRecv;              // received message data
    unsigned int nDataPos;

    CNetMessage(int nTypeIn, int nVersionIn) : hdrbuf(nTypeIn, nVersionIn), vRecv(nTypeIn, nVersionIn) {
        hdrbuf.resize(24);
        in_data = false;
        nHdrPos = 0;
        nDataPos = 0;
    }

    bool complete() const
    {
        if (!in_data)
            return false;
        return (hdr.nMessageSize == nDataPos);
    }

    void SetVersion(int nVersionIn)
    {
        hdrbuf.SetVersion(nVersionIn);
        vRecv.SetVersion(nVersionIn);
    }

    int readHeader(const char *pch, unsigned int nBytes);
    int readData(const char *pch, unsigned int nBytes);
};





/** Information about a peer */
class CNode
{
public:
    CNetParams * netParams;
    // socket
    uint64_t nServices;
    SOCKET hSocket;
    CDataStream ssSend;
    size_t nSendSize; // total size of all vSendMsg entries
    size_t nSendOffset; // offset inside the first vSendMsg already sent
    uint64_t nSendBytes;
    std::deque<CSerializeData> vSendMsg;
    CCriticalSection cs_vSend;

    std::deque<CInv> vRecvGetData;
    std::deque<CNetMessage> vRecvMsg;
    CCriticalSection cs_vRecvMsg;
    uint64_t nRecvBytes;
    int nRecvVersion;

    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nLastSendEmpty;
    int64_t nTimeConnected;
    CAddress addr;
    std::string addrName;
    CService addrLocal;
    int nVersion;
    // strSubVer is whatever byte array we read from the wire. However, this field is intended
    // to be printed out, displayed to humans in various forms and so on. So we sanitize it and
    // store the sanitized version in cleanSubVer. The original should be used when dealing with
    // the network or wire types and the cleaned string used when displayed or logged.
    std::string strSubVer, cleanSubVer;
    bool fOneShot;
    bool fClient;
    bool fInbound;
    bool fNetworkNode;
    bool fSuccessfullyConnected;
    bool fDisconnect;
    // We use fRelayTxes for two purposes -
    // a) it allows us to not relay tx invs before receiving the peer's version message
    // b) the peer may tell us in their version message that we should not relay tx invs
    //    until they have initialized their bloom filter.
    bool fRelayTxes;
    CSemaphoreGrant grantOutbound;
    CCriticalSection cs_filter;
    CBloomFilter* pfilter;
    int nRefCount;
    NodeId id;
protected:

    // Denial-of-service detection/prevention
    // Key is IP address, value is banned-until-time
    static std::map<CNetAddr, int64_t> setBanned;
    static CCriticalSection cs_setBanned;

    // Basic fuzz-testing
    void Fuzz(int nChance); // modifies ssSend

public:
    uint256 hashContinue;
    CBlockIndexBase* pindexLastGetBlocksBegin;
    uint256 hashLastGetBlocksEnd;
    int nStartingHeight;
    bool fStartSync;

    // flood relay
    std::vector<CAddress> vAddrToSend;
    mruset<CAddress> setAddrKnown;
    bool fGetAddr;
    std::set<uint256> setKnown;

    // inventory based relay
    mruset<CInv> setInventoryKnown;
    std::vector<CInv> vInventoryToSend;
    CCriticalSection cs_inventory;
    std::multimap<int64_t, CInv> mapAskFor;

    // Ping time measurement
    uint64_t nPingNonceSent;
    int64_t nPingUsecStart;
    int64_t nPingUsecTime;
    bool fPingQueued;

    CNode(SOCKET hSocketIn, CAddress addrIn, std::string addrNameIn, bool fInboundIn, CNetParams * netParamsIn) : ssSend(SER_NETWORK, INIT_PROTO_VERSION), setAddrKnown(5000)
    {
    	netParams = netParamsIn;
        nServices = 0;
        hSocket = hSocketIn;
        nRecvVersion = INIT_PROTO_VERSION;
        nLastSend = 0;
        nLastRecv = 0;
        nSendBytes = 0;
        nRecvBytes = 0;
        nLastSendEmpty = GetTime();
        nTimeConnected = GetTime();
        addr = addrIn;
        addrName = addrNameIn == "" ? addr.ToStringIPPort() : addrNameIn;
        nVersion = 0;
        strSubVer = "";
        fOneShot = false;
        fClient = false; // set by version message
        fInbound = fInboundIn;
        fNetworkNode = false;
        fSuccessfullyConnected = false;
        fDisconnect = false;
        nRefCount = 0;
        nSendSize = 0;
        nSendOffset = 0;
        hashContinue = 0;
        pindexLastGetBlocksBegin = 0;
        hashLastGetBlocksEnd = 0;
        nStartingHeight = -1;
        fStartSync = false;
        fGetAddr = false;
        fRelayTxes = false;
        setInventoryKnown.max_size(SendBufferSize() / 1000);
        pfilter = new CBloomFilter();
        nPingNonceSent = 0;
        nPingUsecStart = 0;
        nPingUsecTime = 0;
        fPingQueued = false;

        {
            LOCK(netParams->cs_nLastNodeId);
            id = netParams->nLastNodeId++;
        }

        // Be shy and don't send version until we hear
        if (hSocket != INVALID_SOCKET && !fInbound)
            PushVersion();

        netParams->GetNodeSignals().InitializeNode(GetId(), this);
    }

    ~CNode()
    {
        if (hSocket != INVALID_SOCKET)
        {
            closesocket(hSocket);
            hSocket = INVALID_SOCKET;
        }
        if (pfilter)
            delete pfilter;
        netParams->GetNodeSignals().FinalizeNode(GetId());
    }

private:
    // Network usage totals
    static CCriticalSection bitcredit_cs_totalBytesRecv;
    static CCriticalSection bitcredit_cs_totalBytesSent;
    static uint64_t bitcredit_nTotalBytesRecv;
    static uint64_t bitcredit_nTotalBytesSent;
    static CCriticalSection bitcoin_cs_totalBytesRecv;
    static CCriticalSection bitcoin_cs_totalBytesSent;
    static uint64_t bitcoin_nTotalBytesRecv;
    static uint64_t bitcoin_nTotalBytesSent;

    CNode(const CNode&);
    void operator=(const CNode&);

public:

    CNetParams * GetNetParams() const {
      return netParams;
    }
    NodeId GetId() const {
      return id;
    }

    int GetRefCount()
    {
        assert(nRefCount >= 0);
        return nRefCount;
    }

    // requires LOCK(cs_vRecvMsg)
    unsigned int GetTotalRecvSize()
    {
        unsigned int total = 0;
        BOOST_FOREACH(const CNetMessage &msg, vRecvMsg)
            total += msg.vRecv.size() + 24;
        return total;
    }

    // requires LOCK(cs_vRecvMsg)
    bool ReceiveMsgBytes(const char *pch, unsigned int nBytes);

    // requires LOCK(cs_vRecvMsg)
    void SetRecvVersion(int nVersionIn)
    {
        nRecvVersion = nVersionIn;
        BOOST_FOREACH(CNetMessage &msg, vRecvMsg)
            msg.SetVersion(nVersionIn);
    }

    CNode* AddRef()
    {
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
        {
            LOCK(cs_inventory);
            setInventoryKnown.insert(inv);
        }
    }

    void PushInventory(const CInv& inv)
    {
        {
            LOCK(cs_inventory);
            if (!setInventoryKnown.count(inv))
                vInventoryToSend.push_back(inv);
        }
    }

    void AskFor(const CInv& inv)
    {
        // We're using mapAskFor as a priority queue,
        // the key is the earliest time the request can be sent
        int64_t nRequestTime;
        limitedmap<CInv, int64_t>::const_iterator it = netParams->mapAlreadyAskedFor.find(inv);
        if (it != netParams->mapAlreadyAskedFor.end())
            nRequestTime = it->second;
        else
            nRequestTime = 0;
        LogPrint(netParams->DebugCategory(), "%s askfor %s   %d (%s)\n", netParams->LogPrefix(), inv.ToString(), nRequestTime, DateTimeStrFormat("%H:%M:%S", nRequestTime/1000000).c_str());

        // Make sure not to reuse time indexes to keep things in the same order
        int64_t nNow = GetTimeMicros() - 1000000;
        static int64_t nLastTime;
        ++nLastTime;
        nNow = std::max(nNow, nLastTime);
        nLastTime = nNow;

        // Each retry is 2 minutes after the last
        nRequestTime = std::max(nRequestTime + 2 * 60 * 1000000, nNow);
        if (it != netParams->mapAlreadyAskedFor.end())
        	netParams->mapAlreadyAskedFor.update(it, nRequestTime);
        else
        	netParams->mapAlreadyAskedFor.insert(std::make_pair(inv, nRequestTime));
        mapAskFor.insert(std::make_pair(nRequestTime, inv));
    }



    // TODO: Document the postcondition of this function.  Is cs_vSend locked?
    void BeginMessage(const char* pszCommand) EXCLUSIVE_LOCK_FUNCTION(cs_vSend)
    {
        ENTER_CRITICAL_SECTION(cs_vSend);
        assert(ssSend.size() == 0);
        ssSend << CMessageHeader(pszCommand, 0, netParams->MessageStart());
        LogPrint(netParams->DebugCategory(), "%s sending: %s ", netParams->LogPrefix(), pszCommand);
    }

    // TODO: Document the precondition of this function.  Is cs_vSend locked?
    void AbortMessage() UNLOCK_FUNCTION(cs_vSend)
    {
        ssSend.clear();

        LEAVE_CRITICAL_SECTION(cs_vSend);

        LogPrint(netParams->DebugCategory(), "(aborted)\n");
    }

    // TODO: Document the precondition of this function.  Is cs_vSend locked?
    void EndMessage() UNLOCK_FUNCTION(cs_vSend)
    {
        // The -*messagestest options are intentionally not documented in the help message,
        // since they are only used during development to debug the networking code and are
        // not intended for end-users.
        if (mapArgs.count("-dropmessagestest") && GetRand(GetArg("-dropmessagestest", 2)) == 0)
        {
            LogPrint(netParams->DebugCategory(), "%s dropmessages DROPPING SEND MESSAGE\n", netParams->LogPrefix());
            AbortMessage();
            return;
        }
        if (mapArgs.count("-fuzzmessagestest"))
            Fuzz(GetArg("-fuzzmessagestest", 10));

        if (ssSend.size() == 0)
            return;

        // Set the size
        unsigned int nSize = ssSend.size() - CMessageHeader::HEADER_SIZE;
        memcpy((char*)&ssSend[CMessageHeader::MESSAGE_SIZE_OFFSET], &nSize, sizeof(nSize));

        // Set the checksum
        uint256 hash = Hash(ssSend.begin() + CMessageHeader::HEADER_SIZE, ssSend.end());
        unsigned int nChecksum = 0;
        memcpy(&nChecksum, &hash, sizeof(nChecksum));
        assert(ssSend.size () >= CMessageHeader::CHECKSUM_OFFSET + sizeof(nChecksum));
        memcpy((char*)&ssSend[CMessageHeader::CHECKSUM_OFFSET], &nChecksum, sizeof(nChecksum));

        LogPrint(netParams->DebugCategory(), "(%d bytes)\n", nSize);

        std::deque<CSerializeData>::iterator it = vSendMsg.insert(vSendMsg.end(), CSerializeData());
        ssSend.GetAndClear(*it);
        nSendSize += (*it).size();

        // If write queue empty, attempt "optimistic write"
        if (it == vSendMsg.begin())
            SocketSendData(this);

        LEAVE_CRITICAL_SECTION(cs_vSend);
    }

    void PushVersion();


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
            ssSend << a1;
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
            ssSend << a1 << a2;
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
            ssSend << a1 << a2 << a3;
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
            ssSend << a1 << a2 << a3 << a4;
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
            ssSend << a1 << a2 << a3 << a4 << a5;
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
            ssSend << a1 << a2 << a3 << a4 << a5 << a6;
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
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7;
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
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8;
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
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8 << a9;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    bool IsSubscribed(unsigned int nChannel);
    void Subscribe(unsigned int nChannel, unsigned int nHops=0);
    void CancelSubscribe(unsigned int nChannel);
    void CloseSocketDisconnect();
    void Cleanup();


    // Denial-of-service detection/prevention
    // The idea is to detect peers that are behaving
    // badly and disconnect/ban them, but do it in a
    // one-coding-mistake-won't-shatter-the-entire-network
    // way.
    // IMPORTANT:  There should be nothing I can give a
    // node that it will forward on that will make that
    // node's peers drop it. If there is, an attacker
    // can isolate a node and/or try to split the network.
    // Dropping a node for sending stuff that is invalid
    // now but might be valid in a later version is also
    // dangerous, because it can cause a network split
    // between nodes running old code and nodes running
    // new code.
    static void ClearBanned(); // needed for unit testing
    static bool IsBanned(CNetAddr ip);
    static bool Ban(const CNetAddr &ip);
    void copyStats(CNodeStats &stats);

    // Network stats
    static void bitcredit_RecordBytesRecv(uint64_t bytes);
    static void bitcoin_RecordBytesRecv(uint64_t bytes);
    static void bitcredit_RecordBytesSent(uint64_t bytes);
    static void bitcoin_RecordBytesSent(uint64_t bytes);

    static uint64_t bitcredit_GetTotalBytesRecv();
    static uint64_t bitcoin_GetTotalBytesRecv();
    static uint64_t bitcredit_GetTotalBytesSent();
    static uint64_t bitcoin_GetTotalBytesSent();
};


class Credits_CTransaction;
void Credits_RelayTransaction(const Credits_CTransaction& tx, const uint256& hash, CNetParams * netParams);
void Credits_RelayTransaction(const Credits_CTransaction& txWrap, const uint256& hash, const CDataStream& ss, CNetParams * netParams);

class Bitcoin_CTransaction;
void Bitcoin_RelayTransaction(const Bitcoin_CTransaction& tx, const uint256& hash, CNetParams * netParams);
void Bitcoin_RelayTransaction(const Bitcoin_CTransaction& txWrap, const uint256& hash, const CDataStream& ss, CNetParams * netParams);

/** Access to the (IP) address database (xxx_peers.dat) */
class CAddrDB
{
private:
    std::string fileName;
	MessageStartChars messageStart;
	int clientVersion;
    boost::filesystem::path pathAddr;
public:
    CAddrDB(std::string fileName, const MessageStartChars& messageStart, int clientVersion);
    bool Write(const CAddrMan& addr);
    bool Read(CAddrMan& addr);
};

#endif
