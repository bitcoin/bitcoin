// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/dash-config.h>
#endif

#include <net.h>
#include <netmessagemaker.h>

#include <chainparams.h>
#include <clientversion.h>
#include <consensus/consensus.h>
#include <crypto/common.h>
#include <crypto/sha256.h>
#include <primitives/transaction.h>
#include <netbase.h>
#include <scheduler.h>
#include <ui_interface.h>
#include <utilstrencodings.h>
#include <validation.h>

#include <masternode/masternode-meta.h>
#include <masternode/masternode-sync.h>
#include <coinjoin/coinjoin.h>
#include <evo/deterministicmns.h>

#include <statsd_client.h>

#ifdef WIN32
#include <string.h>
#else
#include <fcntl.h>
#endif

#ifdef USE_POLL
#include <poll.h>
#endif

#ifdef USE_EPOLL
#include <sys/epoll.h>
#endif

#ifdef USE_KQUEUE
#include <sys/event.h>
#endif

#ifdef USE_UPNP
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/miniwget.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
#endif

#include <unordered_map>

#include <math.h>

// Dump addresses to peers.dat and banlist.dat every 15 minutes (900s)
#define DUMP_ADDRESSES_INTERVAL 900

/** Number of DNS seeds to query when the number of connections is low. */
static constexpr int DNSSEEDS_TO_QUERY_AT_ONCE = 3;

// We add a random period time (0 to 1 seconds) to feeler connections to prevent synchronization.
#define FEELER_SLEEP_WINDOW 1

// MSG_NOSIGNAL is not available on some platforms, if it doesn't exist define it as 0
#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

// MSG_DONTWAIT is not available on some platforms, if it doesn't exist define it as 0
#if !defined(MSG_DONTWAIT)
#define MSG_DONTWAIT 0
#endif

// Fix for ancient MinGW versions, that don't have defined these in ws2tcpip.h.
// Todo: Can be removed when our pull-tester is upgraded to a modern MinGW version.
#ifdef WIN32
#ifndef PROTECTION_LEVEL_UNRESTRICTED
#define PROTECTION_LEVEL_UNRESTRICTED 10
#endif
#ifndef IPV6_PROTECTION_LEVEL
#define IPV6_PROTECTION_LEVEL 23
#endif
#endif

/** Used to pass flags to the Bind() function */
enum BindFlags {
    BF_NONE         = 0,
    BF_EXPLICIT     = (1U << 0),
    BF_REPORT_ERROR = (1U << 1),
    BF_WHITELIST    = (1U << 2),
};

#ifndef USE_WAKEUP_PIPE
// The set of sockets cannot be modified while waiting
// The sleep time needs to be small to avoid new sockets stalling
static const uint64_t SELECT_TIMEOUT_MILLISECONDS = 50;
#else
// select() is woken up through the wakeup pipe whenever a new node is added, so we can wait much longer.
// We are however still somewhat limited in how long we can sleep as there is periodic work (cleanup) to be done in
// the socket handler thread
static const uint64_t SELECT_TIMEOUT_MILLISECONDS = 500;
#endif

const static std::string NET_MESSAGE_COMMAND_OTHER = "*other*";

constexpr const CConnman::CFullyConnectedOnly CConnman::FullyConnectedOnly;
constexpr const CConnman::CAllNodes CConnman::AllNodes;

static const uint64_t RANDOMIZER_ID_NETGROUP = 0x6c0edd8036ef4036ULL; // SHA256("netgroup")[0:8]
static const uint64_t RANDOMIZER_ID_LOCALHOSTNONCE = 0xd93e69e2bbfa5735ULL; // SHA256("localhostnonce")[0:8]
//
// Global state variables
//
bool fDiscover = true;
bool fListen = true;
bool fRelayTxes = true;
CCriticalSection cs_mapLocalHost;
std::map<CNetAddr, LocalServiceInfo> mapLocalHost GUARDED_BY(cs_mapLocalHost);
static bool vfLimited[NET_MAX] GUARDED_BY(cs_mapLocalHost) = {};
std::string strSubVersion;

void CConnman::AddOneShot(const std::string& strDest)
{
    LOCK(cs_vOneShots);
    vOneShots.push_back(strDest);
}

unsigned short GetListenPort()
{
    return (unsigned short)(gArgs.GetArg("-port", Params().GetDefaultPort()));
}

// find 'best' local address for a particular peer
bool GetLocal(CService& addr, const CNetAddr *paddrPeer)
{
    if (!fListen)
        return false;

    int nBestScore = -1;
    int nBestReachability = -1;
    {
        LOCK(cs_mapLocalHost);
        for (const auto& entry : mapLocalHost)
        {
            int nScore = entry.second.nScore;
            int nReachability = entry.first.GetReachabilityFrom(paddrPeer);
            if (nReachability > nBestReachability || (nReachability == nBestReachability && nScore > nBestScore))
            {
                addr = CService(entry.first, entry.second.nPort);
                nBestReachability = nReachability;
                nBestScore = nScore;
            }
        }
    }
    return nBestScore >= 0;
}

//! Convert the pnSeed6 array into usable address objects.
static std::vector<CAddress> convertSeed6(const std::vector<SeedSpec6> &vSeedsIn)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    std::vector<CAddress> vSeedsOut;
    vSeedsOut.reserve(vSeedsIn.size());
    for (const auto& seed_in : vSeedsIn) {
        struct in6_addr ip;
        memcpy(&ip, seed_in.addr, sizeof(ip));
        CAddress addr(CService(ip, seed_in.port), GetDesirableServiceFlags(NODE_NONE));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
    return vSeedsOut;
}

// get best local address for a particular peer as a CAddress
// Otherwise, return the unroutable 0.0.0.0 but filled in with
// the normal parameters, since the IP may be changed to a useful
// one by discovery.
CAddress GetLocalAddress(const CNetAddr *paddrPeer, ServiceFlags nLocalServices)
{
    CAddress ret(CService(CNetAddr(),GetListenPort()), nLocalServices);
    CService addr;
    if (GetLocal(addr, paddrPeer))
    {
        ret = CAddress(addr, nLocalServices);
    }
    ret.nTime = GetAdjustedTime();
    return ret;
}

static int GetnScore(const CService& addr)
{
    LOCK(cs_mapLocalHost);
    if (mapLocalHost.count(addr) == 0) return 0;
    return mapLocalHost[addr].nScore;
}

// Is our peer's addrLocal potentially useful as an external IP source?
bool IsPeerAddrLocalGood(CNode *pnode)
{
    CService addrLocal = pnode->GetAddrLocal();
    return fDiscover && pnode->addr.IsRoutable() && addrLocal.IsRoutable() &&
           !IsLimited(addrLocal.GetNetwork());
}

// pushes our own address to a peer
void AdvertiseLocal(CNode *pnode)
{
    if (fListen && pnode->fSuccessfullyConnected)
    {
        CAddress addrLocal = GetLocalAddress(&pnode->addr, pnode->GetLocalServices());
        if (gArgs.GetBoolArg("-addrmantest", false)) {
            // use IPv4 loopback during addrmantest
            addrLocal = CAddress(CService(LookupNumeric("127.0.0.1", GetListenPort())), pnode->GetLocalServices());
        }
        // If discovery is enabled, sometimes give our peer the address it
        // tells us that it sees us as in case it has a better idea of our
        // address than we do.
        if (IsPeerAddrLocalGood(pnode) && (!addrLocal.IsRoutable() ||
             GetRand((GetnScore(addrLocal) > LOCAL_MANUAL) ? 8:2) == 0))
        {
            addrLocal.SetIP(pnode->GetAddrLocal());
        }
        if (addrLocal.IsRoutable() || gArgs.GetBoolArg("-addrmantest", false))
        {
            LogPrint(BCLog::NET, "AdvertiseLocal: advertising address %s\n", addrLocal.ToString());
            FastRandomContext insecure_rand;
            pnode->PushAddress(addrLocal, insecure_rand);
        }
    }
}

// learn a new local address
bool AddLocal(const CService& addr, int nScore)
{
    if (!addr.IsRoutable() && Params().RequireRoutableExternalIP())
        return false;

    if (!fDiscover && nScore < LOCAL_MANUAL)
        return false;

    if (IsLimited(addr))
        return false;

    LogPrintf("AddLocal(%s,%i)\n", addr.ToString(), nScore);

    {
        LOCK(cs_mapLocalHost);
        bool fAlready = mapLocalHost.count(addr) > 0;
        LocalServiceInfo &info = mapLocalHost[addr];
        if (!fAlready || nScore >= info.nScore) {
            info.nScore = nScore + (fAlready ? 1 : 0);
            info.nPort = addr.GetPort();
        }
    }

    return true;
}

bool AddLocal(const CNetAddr &addr, int nScore)
{
    return AddLocal(CService(addr, GetListenPort()), nScore);
}

bool RemoveLocal(const CService& addr)
{
    LOCK(cs_mapLocalHost);
    LogPrintf("RemoveLocal(%s)\n", addr.ToString());
    mapLocalHost.erase(addr);
    return true;
}

/** Make a particular network entirely off-limits (no automatic connects to it) */
void SetLimited(enum Network net, bool fLimited)
{
    if (net == NET_UNROUTABLE || net == NET_INTERNAL)
        return;
    LOCK(cs_mapLocalHost);
    vfLimited[net] = fLimited;
}

bool IsLimited(enum Network net)
{
    LOCK(cs_mapLocalHost);
    return vfLimited[net];
}

bool IsLimited(const CNetAddr &addr)
{
    return IsLimited(addr.GetNetwork());
}

/** vote for a local address */
bool SeenLocal(const CService& addr)
{
    {
        LOCK(cs_mapLocalHost);
        if (mapLocalHost.count(addr) == 0)
            return false;
        mapLocalHost[addr].nScore++;
    }
    return true;
}


/** check whether a given address is potentially local */
bool IsLocal(const CService& addr)
{
    LOCK(cs_mapLocalHost);
    return mapLocalHost.count(addr) > 0;
}

/** check whether a given network is one we can probably connect to */
bool IsReachable(enum Network net)
{
    LOCK(cs_mapLocalHost);
    return !vfLimited[net];
}

/** check whether a given address is in a network we can probably connect to */
bool IsReachable(const CNetAddr& addr)
{
    enum Network net = addr.GetNetwork();
    return IsReachable(net);
}


CNode* CConnman::FindNode(const CNetAddr& ip, bool fExcludeDisconnecting)
{
    LOCK(cs_vNodes);
    for (CNode* pnode : vNodes) {
        if (fExcludeDisconnecting && pnode->fDisconnect) {
            continue;
        }
        if (static_cast<CNetAddr>(pnode->addr) == ip) {
            return pnode;
        }
    }
    return nullptr;
}

CNode* CConnman::FindNode(const CSubNet& subNet, bool fExcludeDisconnecting)
{
    LOCK(cs_vNodes);
    for (CNode* pnode : vNodes) {
        if (fExcludeDisconnecting && pnode->fDisconnect) {
            continue;
        }
        if (subNet.Match(static_cast<CNetAddr>(pnode->addr))) {
            return pnode;
        }
    }
    return nullptr;
}

CNode* CConnman::FindNode(const std::string& addrName, bool fExcludeDisconnecting)
{
    LOCK(cs_vNodes);
    for (CNode* pnode : vNodes) {
        if (fExcludeDisconnecting && pnode->fDisconnect) {
            continue;
        }
        if (pnode->GetAddrName() == addrName) {
            return pnode;
        }
    }
    return nullptr;
}

CNode* CConnman::FindNode(const CService& addr, bool fExcludeDisconnecting)
{
    LOCK(cs_vNodes);
    for (CNode* pnode : vNodes) {
        if (fExcludeDisconnecting && pnode->fDisconnect) {
            continue;
        }
        if (static_cast<CService>(pnode->addr) == addr) {
            return pnode;
        }
    }
    return nullptr;
}

bool CConnman::CheckIncomingNonce(uint64_t nonce)
{
    LOCK(cs_vNodes);
    for (CNode* pnode : vNodes) {
        if (!pnode->fSuccessfullyConnected && !pnode->fInbound && pnode->GetLocalNonce() == nonce)
            return false;
    }
    return true;
}

/** Get the bind address for a socket as CAddress */
static CAddress GetBindAddress(SOCKET sock)
{
    CAddress addr_bind;
    struct sockaddr_storage sockaddr_bind;
    socklen_t sockaddr_bind_len = sizeof(sockaddr_bind);
    if (sock != INVALID_SOCKET) {
        if (!getsockname(sock, (struct sockaddr*)&sockaddr_bind, &sockaddr_bind_len)) {
            addr_bind.SetSockAddr((const struct sockaddr*)&sockaddr_bind);
        } else {
            LogPrint(BCLog::NET, "Warning: getsockname failed\n");
        }
    }
    return addr_bind;
}

CNode* CConnman::ConnectNode(CAddress addrConnect, const char *pszDest, bool fCountFailure, bool manual_connection)
{
    if (pszDest == nullptr) {
        bool fAllowLocal = Params().AllowMultiplePorts() && addrConnect.GetPort() != GetListenPort();
        if (!fAllowLocal && IsLocal(addrConnect)) {
            return nullptr;
        }

        // Look for an existing connection
        CNode* pnode = FindNode(static_cast<CService>(addrConnect));
        if (pnode)
        {
            LogPrintf("Failed to open new connection, already connected\n");
            return nullptr;
        }
    }

    /// debug print
    if (fLogIPs) {
        LogPrint(BCLog::NET, "trying connection %s lastseen=%.1fhrs\n",
            pszDest ? pszDest : addrConnect.ToString(),
            pszDest ? 0.0 : (double)(GetAdjustedTime() - addrConnect.nTime)/3600.0);
    } else {
        LogPrint(BCLog::NET, "trying connection lastseen=%.1fhrs\n",
            pszDest ? 0.0 : (double)(GetAdjustedTime() - addrConnect.nTime)/3600.0);
    }

    // Resolve
    const int default_port = Params().GetDefaultPort();
    if (pszDest) {
        std::vector<CService> resolved;
        if (Lookup(pszDest, resolved,  default_port, fNameLookup && !HaveNameProxy(), 256) && !resolved.empty()) {
            addrConnect = CAddress(resolved[GetRand(resolved.size())], NODE_NONE);
            if (!addrConnect.IsValid()) {
                LogPrint(BCLog::NET, "Resolver returned invalid address %s for %s\n", addrConnect.ToString(), pszDest);
                return nullptr;
            }
            // It is possible that we already have a connection to the IP/port pszDest resolved to.
            // In that case, drop the connection that was just created, and return the existing CNode instead.
            // Also store the name we used to connect in that CNode, so that future FindNode() calls to that
            // name catch this early.
            LOCK(cs_vNodes);
            CNode* pnode = FindNode(static_cast<CService>(addrConnect));
            if (pnode)
            {
                pnode->MaybeSetAddrName(std::string(pszDest));
                LogPrintf("Failed to open new connection, already connected\n");
                return nullptr;
            }
        }
    }

    // Connect
    bool connected = false;
    SOCKET hSocket = INVALID_SOCKET;
    proxyType proxy;
    if (addrConnect.IsValid()) {
        bool proxyConnectionFailed = false;

        if (GetProxy(addrConnect.GetNetwork(), proxy)) {
            hSocket = CreateSocket(proxy.proxy);
            if (hSocket == INVALID_SOCKET) {
                return nullptr;
            }
            connected = ConnectThroughProxy(proxy, addrConnect.ToStringIP(), addrConnect.GetPort(), hSocket, nConnectTimeout, &proxyConnectionFailed);
        } else {
            // no proxy needed (none set for target network)
            hSocket = CreateSocket(addrConnect);
            if (hSocket == INVALID_SOCKET) {
                return nullptr;
            }
            connected = ConnectSocketDirectly(addrConnect, hSocket, nConnectTimeout, manual_connection);
        }
        if (!proxyConnectionFailed) {
            // If a connection to the node was attempted, and failure (if any) is not caused by a problem connecting to
            // the proxy, mark this as an attempt.
            addrman.Attempt(addrConnect, fCountFailure);
        }
    } else if (pszDest && GetNameProxy(proxy)) {
        hSocket = CreateSocket(proxy.proxy);
        if (hSocket == INVALID_SOCKET) {
            return nullptr;
        }
        std::string host;
        int port = default_port;
        SplitHostPort(std::string(pszDest), port, host);
        connected = ConnectThroughProxy(proxy, host, port, hSocket, nConnectTimeout, nullptr);
    }
    if (!connected) {
        CloseSocket(hSocket);
        return nullptr;
    }

    // Add node
    NodeId id = GetNewNodeId();
    uint64_t nonce = GetDeterministicRandomizer(RANDOMIZER_ID_LOCALHOSTNONCE).Write(id).Finalize();
    CAddress addr_bind = GetBindAddress(hSocket);
    CNode* pnode = new CNode(id, nLocalServices, GetBestHeight(), hSocket, addrConnect, CalculateKeyedNetGroup(addrConnect), nonce, addr_bind, pszDest ? pszDest : "", false);
    pnode->AddRef();
    statsClient.inc("peers.connect", 1.0f);

    return pnode;
}

void CConnman::DumpBanlist()
{
    SweepBanned(); // clean unused entries (if bantime has expired)

    if (!BannedSetIsDirty())
        return;

    int64_t nStart = GetTimeMillis();

    CBanDB bandb;
    banmap_t banmap;
    GetBanned(banmap);
    if (bandb.Write(banmap)) {
        SetBannedSetDirty(false);
    }

    LogPrint(BCLog::NET, "Flushed %d banned node ips/subnets to banlist.dat  %dms\n",
        banmap.size(), GetTimeMillis() - nStart);
}

void CNode::CloseSocketDisconnect(CConnman* connman)
{
    AssertLockHeld(connman->cs_vNodes);

    fDisconnect = true;
    LOCK(cs_hSocket);
    if (hSocket == INVALID_SOCKET) {
        return;
    }

    fHasRecvData = false;
    fCanSendData = false;

    connman->mapSocketToNode.erase(hSocket);
    connman->mapReceivableNodes.erase(GetId());
    connman->mapSendableNodes.erase(GetId());
    {
        LOCK(connman->cs_mapNodesWithDataToSend);
        if (connman->mapNodesWithDataToSend.erase(GetId()) != 0) {
            // See comment in PushMessage
            Release();
        }
    }

    connman->UnregisterEvents(this);

    LogPrint(BCLog::NET, "disconnecting peer=%d\n", id);
    CloseSocket(hSocket);
    statsClient.inc("peers.disconnect", 1.0f);
}

void CConnman::ClearBanned()
{
    {
        LOCK(cs_setBanned);
        setBanned.clear();
        setBannedIsDirty = true;
    }
    DumpBanlist(); //store banlist to disk
    if(clientInterface)
        clientInterface->BannedListChanged();
}

bool CConnman::IsBanned(CNetAddr ip)
{
    LOCK(cs_setBanned);
    for (const auto& it : setBanned) {
        CSubNet subNet = it.first;
        CBanEntry banEntry = it.second;

        if (subNet.Match(ip) && GetTime() < banEntry.nBanUntil) {
            return true;
        }
    }
    return false;
}

bool CConnman::IsBanned(CSubNet subnet)
{
    LOCK(cs_setBanned);
    banmap_t::iterator i = setBanned.find(subnet);
    if (i != setBanned.end())
    {
        CBanEntry banEntry = (*i).second;
        if (GetTime() < banEntry.nBanUntil) {
            return true;
        }
    }
    return false;
}

void CConnman::Ban(const CNetAddr& addr, const BanReason &banReason, int64_t bantimeoffset, bool sinceUnixEpoch) {
    CSubNet subNet(addr);
    Ban(subNet, banReason, bantimeoffset, sinceUnixEpoch);
}

void CConnman::Ban(const CSubNet& subNet, const BanReason &banReason, int64_t bantimeoffset, bool sinceUnixEpoch) {
    CBanEntry banEntry(GetTime());
    banEntry.banReason = banReason;
    if (bantimeoffset <= 0)
    {
        bantimeoffset = gArgs.GetArg("-bantime", DEFAULT_MISBEHAVING_BANTIME);
        sinceUnixEpoch = false;
    }
    banEntry.nBanUntil = (sinceUnixEpoch ? 0 : GetTime() )+bantimeoffset;

    {
        LOCK(cs_setBanned);
        if (setBanned[subNet].nBanUntil < banEntry.nBanUntil) {
            setBanned[subNet] = banEntry;
            setBannedIsDirty = true;
        }
        else
            return;
    }
    if(clientInterface)
        clientInterface->BannedListChanged();
    {
        LOCK(cs_vNodes);
        for (CNode* pnode : vNodes) {
            if (subNet.Match(static_cast<CNetAddr>(pnode->addr)))
                pnode->fDisconnect = true;
        }
    }
    if(banReason == BanReasonManuallyAdded)
        DumpBanlist(); //store banlist to disk immediately if user requested ban
}

bool CConnman::Unban(const CNetAddr &addr) {
    CSubNet subNet(addr);
    return Unban(subNet);
}

bool CConnman::Unban(const CSubNet &subNet) {
    {
        LOCK(cs_setBanned);
        if (!setBanned.erase(subNet))
            return false;
        setBannedIsDirty = true;
    }
    if(clientInterface)
        clientInterface->BannedListChanged();
    DumpBanlist(); //store banlist to disk immediately
    return true;
}

void CConnman::GetBanned(banmap_t &banMap)
{
    LOCK(cs_setBanned);
    // Sweep the banlist so expired bans are not returned
    SweepBanned();
    banMap = setBanned; //create a thread safe copy
}

void CConnman::SetBanned(const banmap_t &banMap)
{
    LOCK(cs_setBanned);
    setBanned = banMap;
    setBannedIsDirty = true;
}

void CConnman::SweepBanned()
{
    int64_t now = GetTime();
    bool notifyUI = false;
    {
        LOCK(cs_setBanned);
        banmap_t::iterator it = setBanned.begin();
        while(it != setBanned.end())
        {
            CSubNet subNet = (*it).first;
            CBanEntry banEntry = (*it).second;
            if(now > banEntry.nBanUntil)
            {
                setBanned.erase(it++);
                setBannedIsDirty = true;
                notifyUI = true;
                LogPrint(BCLog::NET, "%s: Removed banned node ip/subnet from banlist.dat: %s\n", __func__, subNet.ToString());
            }
            else
                ++it;
        }
    }
    // update UI
    if(notifyUI && clientInterface) {
        clientInterface->BannedListChanged();
    }
}

bool CConnman::BannedSetIsDirty()
{
    LOCK(cs_setBanned);
    return setBannedIsDirty;
}

void CConnman::SetBannedSetDirty(bool dirty)
{
    LOCK(cs_setBanned); //reuse setBanned lock for the isDirty flag
    setBannedIsDirty = dirty;
}


bool CConnman::IsWhitelistedRange(const CNetAddr &addr) {
    for (const CSubNet& subnet : vWhitelistedRange) {
        if (subnet.Match(addr))
            return true;
    }
    return false;
}

std::string CNode::GetAddrName() const {
    LOCK(cs_addrName);
    return addrName;
}

void CNode::MaybeSetAddrName(const std::string& addrNameIn) {
    LOCK(cs_addrName);
    if (addrName.empty()) {
        addrName = addrNameIn;
    }
}

CService CNode::GetAddrLocal() const {
    LOCK(cs_addrLocal);
    return addrLocal;
}

void CNode::SetAddrLocal(const CService& addrLocalIn) {
    LOCK(cs_addrLocal);
    if (addrLocal.IsValid()) {
        error("Addr local already set for node: %i. Refusing to change from %s to %s", id, addrLocal.ToString(), addrLocalIn.ToString());
    } else {
        addrLocal = addrLocalIn;
    }
}

std::string CNode::GetLogString() const
{
    return fLogIPs ? addr.ToString() : strprintf("%d", id);
}

#undef X
#define X(name) stats.name = name
void CNode::copyStats(CNodeStats &stats, const std::vector<bool> &m_asmap)
{
    stats.nodeid = this->GetId();
    X(nServices);
    X(addr);
    X(addrBind);
    stats.m_mapped_as = addr.GetMappedAS(m_asmap);
    {
        LOCK(cs_filter);
        X(fRelayTxes);
    }
    X(nLastSend);
    X(nLastRecv);
    X(nTimeConnected);
    X(nTimeOffset);
    stats.addrName = GetAddrName();
    X(nVersion);
    {
        LOCK(cs_SubVer);
        X(cleanSubVer);
    }
    X(fInbound);
    X(m_manual_connection);
    X(nStartingHeight);
    {
        LOCK(cs_vSend);
        X(mapSendBytesPerMsgCmd);
        X(nSendBytes);
    }
    {
        LOCK(cs_vRecv);
        X(mapRecvBytesPerMsgCmd);
        X(nRecvBytes);
    }
    X(fWhitelisted);

    // It is common for nodes with good ping times to suddenly become lagged,
    // due to a new block arriving or other large transfer.
    // Merely reporting pingtime might fool the caller into thinking the node was still responsive,
    // since pingtime does not update until the ping is complete, which might take a while.
    // So, if a ping is taking an unusually long time in flight,
    // the caller can immediately detect that this is happening.
    int64_t nPingUsecWait = 0;
    if ((0 != nPingNonceSent) && (0 != nPingUsecStart)) {
        nPingUsecWait = GetTimeMicros() - nPingUsecStart;
    }

    // Raw ping time is in microseconds, but show it to user as whole seconds (Dash users should be well used to small numbers with many decimal places by now :)
    stats.dPingTime = (((double)nPingUsecTime) / 1e6);
    stats.dMinPing  = (((double)nMinPingUsecTime) / 1e6);
    stats.dPingWait = (((double)nPingUsecWait) / 1e6);

    // Leave string empty if addrLocal invalid (not filled in yet)
    CService addrLocalUnlocked = GetAddrLocal();
    stats.addrLocal = addrLocalUnlocked.IsValid() ? addrLocalUnlocked.ToString() : "";

    {
        LOCK(cs_mnauth);
        X(verifiedProRegTxHash);
        X(verifiedPubKeyHash);
    }
    X(m_masternode_connection);
}
#undef X

bool CNode::ReceiveMsgBytes(const char *pch, unsigned int nBytes, bool& complete)
{
    complete = false;
    int64_t nTimeMicros = GetTimeMicros();
    LOCK(cs_vRecv);
    nLastRecv = nTimeMicros / 1000000;
    nRecvBytes += nBytes;
    while (nBytes > 0) {

        // get current incomplete message, or create a new one
        if (vRecvMsg.empty() ||
            vRecvMsg.back().complete())
            vRecvMsg.push_back(CNetMessage(Params().MessageStart(), SER_NETWORK, INIT_PROTO_VERSION));

        CNetMessage& msg = vRecvMsg.back();

        // absorb network data
        int handled;
        if (!msg.in_data) {
            handled = msg.readHeader(pch, nBytes);
        } else {
            handled = msg.readData(pch, nBytes);
        }

        if (handled < 0)
            return false;

        if (msg.in_data && msg.hdr.nMessageSize > MAX_PROTOCOL_MESSAGE_LENGTH) {
            LogPrint(BCLog::NET, "Oversized message from peer=%i, disconnecting\n", GetId());
            return false;
        }

        pch += handled;
        nBytes -= handled;

        if (msg.complete()) {

            //store received bytes per message command
            //to prevent a memory DOS, only allow valid commands
            mapMsgCmdSize::iterator i = mapRecvBytesPerMsgCmd.find(msg.hdr.pchCommand);
            if (i == mapRecvBytesPerMsgCmd.end())
                i = mapRecvBytesPerMsgCmd.find(NET_MESSAGE_COMMAND_OTHER);
            assert(i != mapRecvBytesPerMsgCmd.end());
            i->second += msg.hdr.nMessageSize + CMessageHeader::HEADER_SIZE;
            statsClient.count("bandwidth.message." + std::string(msg.hdr.pchCommand) + ".bytesReceived", msg.hdr.nMessageSize + CMessageHeader::HEADER_SIZE, 1.0f);

            msg.nTime = nTimeMicros;
            complete = true;
        }
    }

    return true;
}

void CNode::SetSendVersion(int nVersionIn)
{
    // Send version may only be changed in the version message, and
    // only one version message is allowed per session. We can therefore
    // treat this value as const and even atomic as long as it's only used
    // once a version message has been successfully processed. Any attempt to
    // set this twice is an error.
    if (nSendVersion != 0) {
        error("Send version already set for node: %i. Refusing to change from %i to %i", id, nSendVersion, nVersionIn);
    } else {
        nSendVersion = nVersionIn;
    }
}

int CNode::GetSendVersion() const
{
    // The send version should always be explicitly set to
    // INIT_PROTO_VERSION rather than using this value until SetSendVersion
    // has been called.
    if (nSendVersion == 0) {
        error("Requesting unset send version for node: %i. Using %i", id, INIT_PROTO_VERSION);
        return INIT_PROTO_VERSION;
    }
    return nSendVersion;
}


int CNetMessage::readHeader(const char *pch, unsigned int nBytes)
{
    // copy data to temporary parsing buffer
    unsigned int nRemaining = 24 - nHdrPos;
    unsigned int nCopy = std::min(nRemaining, nBytes);

    memcpy(&hdrbuf[nHdrPos], pch, nCopy);
    nHdrPos += nCopy;

    // if header incomplete, exit
    if (nHdrPos < 24)
        return nCopy;

    // deserialize to CMessageHeader
    try {
        hdrbuf >> hdr;
    }
    catch (const std::exception&) {
        return -1;
    }

    // reject messages larger than MAX_SIZE
    if (hdr.nMessageSize > MAX_SIZE)
        return -1;

    // switch state to reading message data
    in_data = true;

    return nCopy;
}

int CNetMessage::readData(const char *pch, unsigned int nBytes)
{
    unsigned int nRemaining = hdr.nMessageSize - nDataPos;
    unsigned int nCopy = std::min(nRemaining, nBytes);

    if (vRecv.size() < nDataPos + nCopy) {
        // Allocate up to 256 KiB ahead, but never more than the total message size.
        vRecv.resize(std::min(hdr.nMessageSize, nDataPos + nCopy + 256 * 1024));
    }

    hasher.Write({(const unsigned char*)pch, nCopy});
    memcpy(&vRecv[nDataPos], pch, nCopy);
    nDataPos += nCopy;

    return nCopy;
}

const uint256& CNetMessage::GetMessageHash() const
{
    assert(complete());
    if (data_hash.IsNull())
        hasher.Finalize(data_hash);
    return data_hash;
}

size_t CConnman::SocketSendData(CNode *pnode) EXCLUSIVE_LOCKS_REQUIRED(pnode->cs_vSend)
{
    auto it = pnode->vSendMsg.begin();
    size_t nSentSize = 0;

    while (it != pnode->vSendMsg.end()) {
        const auto &data = *it;
        assert(data.size() > pnode->nSendOffset);
        int nBytes = 0;
        {
            LOCK(pnode->cs_hSocket);
            if (pnode->hSocket == INVALID_SOCKET)
                break;
            nBytes = send(pnode->hSocket, reinterpret_cast<const char*>(data.data()) + pnode->nSendOffset, data.size() - pnode->nSendOffset, MSG_NOSIGNAL | MSG_DONTWAIT);
        }
        if (nBytes > 0) {
            pnode->nLastSend = GetSystemTimeInSeconds();
            pnode->nSendBytes += nBytes;
            pnode->nSendOffset += nBytes;
            nSentSize += nBytes;
            if (pnode->nSendOffset == data.size()) {
                pnode->nSendOffset = 0;
                pnode->nSendSize -= data.size();
                pnode->fPauseSend = pnode->nSendSize > nSendBufferMaxSize;
                it++;
            } else {
                // could not send full message; stop sending more
                pnode->fCanSendData = false;
                break;
            }
        } else {
            if (nBytes < 0) {
                // error
                int nErr = WSAGetLastError();
                if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
                {
                    LogPrintf("socket send error %s (peer=%d)\n", NetworkErrorString(nErr), pnode->GetId());
                    pnode->fDisconnect = true;
                }
            }
            // couldn't send anything at all
            pnode->fCanSendData = false;
            break;
        }
    }

    if (it == pnode->vSendMsg.end()) {
        assert(pnode->nSendOffset == 0);
        assert(pnode->nSendSize == 0);
    }
    pnode->vSendMsg.erase(pnode->vSendMsg.begin(), it);
    pnode->nSendMsgSize = pnode->vSendMsg.size();
    return nSentSize;
}

struct NodeEvictionCandidate
{
    NodeId id;
    int64_t nTimeConnected;
    int64_t nMinPingUsecTime;
    int64_t nLastBlockTime;
    int64_t nLastTXTime;
    bool fRelevantServices;
    bool fRelayTxes;
    bool fBloomFilter;
    uint64_t nKeyedNetGroup;
};

static bool ReverseCompareNodeMinPingTime(const NodeEvictionCandidate& a, const NodeEvictionCandidate& b)
{
    return a.nMinPingUsecTime > b.nMinPingUsecTime;
}

static bool ReverseCompareNodeTimeConnected(const NodeEvictionCandidate& a, const NodeEvictionCandidate& b)
{
    return a.nTimeConnected > b.nTimeConnected;
}

static bool CompareNetGroupKeyed(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b) {
    return a.nKeyedNetGroup < b.nKeyedNetGroup;
}

static bool CompareNodeBlockTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    // There is a fall-through here because it is common for a node to have many peers which have not yet relayed a block.
    if (a.nLastBlockTime != b.nLastBlockTime) return a.nLastBlockTime < b.nLastBlockTime;
    if (a.fRelevantServices != b.fRelevantServices) return b.fRelevantServices;
    return a.nTimeConnected > b.nTimeConnected;
}

static bool CompareNodeTXTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    // There is a fall-through here because it is common for a node to have more than a few peers that have not yet relayed txn.
    if (a.nLastTXTime != b.nLastTXTime) return a.nLastTXTime < b.nLastTXTime;
    if (a.fRelayTxes != b.fRelayTxes) return b.fRelayTxes;
    if (a.fBloomFilter != b.fBloomFilter) return a.fBloomFilter;
    return a.nTimeConnected > b.nTimeConnected;
}


//! Sort an array by the specified comparator, then erase the last K elements.
template<typename T, typename Comparator>
static void EraseLastKElements(std::vector<T> &elements, Comparator comparator, size_t k)
{
    std::sort(elements.begin(), elements.end(), comparator);
    size_t eraseSize = std::min(k, elements.size());
    elements.erase(elements.end() - eraseSize, elements.end());
}

/** Try to find a connection to evict when the node is full.
 *  Extreme care must be taken to avoid opening the node to attacker
 *   triggered network partitioning.
 *  The strategy used here is to protect a small number of peers
 *   for each of several distinct characteristics which are difficult
 *   to forge.  In order to partition a node the attacker must be
 *   simultaneously better at all of them than honest peers.
 */
bool CConnman::AttemptToEvictConnection()
{
    std::vector<NodeEvictionCandidate> vEvictionCandidates;
    {
        LOCK(cs_vNodes);

        for (const CNode* node : vNodes) {
            if (node->fWhitelisted)
                continue;
            if (!node->fInbound)
                continue;
            if (node->fDisconnect)
                continue;

            if (fMasternodeMode) {
                // This handles eviction protected nodes. Nodes are always protected for a short time after the connection
                // was accepted. This short time is meant for the VERSION/VERACK exchange and the possible MNAUTH that might
                // follow when the incoming connection is from another masternode. When a message other than MNAUTH
                // is received after VERSION/VERACK, the protection is lifted immediately.
                bool isProtected = GetSystemTimeInSeconds() - node->nTimeConnected < INBOUND_EVICTION_PROTECTION_TIME;
                if (node->nTimeFirstMessageReceived != 0 && !node->fFirstMessageIsMNAUTH) {
                    isProtected = false;
                }
                // if MNAUTH was valid, the node is always protected (and at the same time not accounted when
                // checking incoming connection limits)
                if (!node->verifiedProRegTxHash.IsNull()) {
                    isProtected = true;
                }
                if (isProtected) {
                    continue;
                }
            }

            NodeEvictionCandidate candidate = {node->GetId(), node->nTimeConnected, node->nMinPingUsecTime,
                                               node->nLastBlockTime, node->nLastTXTime,
                                               HasAllDesirableServiceFlags(node->nServices),
                                               node->fRelayTxes, node->pfilter != nullptr, node->nKeyedNetGroup};
            vEvictionCandidates.push_back(candidate);
        }
    }

    // Protect connections with certain characteristics

    // Deterministically select 4 peers to protect by netgroup.
    // An attacker cannot predict which netgroups will be protected
    EraseLastKElements(vEvictionCandidates, CompareNetGroupKeyed, 4);
    // Protect the 8 nodes with the lowest minimum ping time.
    // An attacker cannot manipulate this metric without physically moving nodes closer to the target.
    EraseLastKElements(vEvictionCandidates, ReverseCompareNodeMinPingTime, 8);
    // Protect 4 nodes that most recently sent us transactions.
    // An attacker cannot manipulate this metric without performing useful work.
    EraseLastKElements(vEvictionCandidates, CompareNodeTXTime, 4);
    // Protect 4 nodes that most recently sent us blocks.
    // An attacker cannot manipulate this metric without performing useful work.
    EraseLastKElements(vEvictionCandidates, CompareNodeBlockTime, 4);
    // Protect the half of the remaining nodes which have been connected the longest.
    // This replicates the non-eviction implicit behavior, and precludes attacks that start later.
    EraseLastKElements(vEvictionCandidates, ReverseCompareNodeTimeConnected, vEvictionCandidates.size() / 2);

    if (vEvictionCandidates.empty()) return false;

    // Identify the network group with the most connections and youngest member.
    // (vEvictionCandidates is already sorted by reverse connect time)
    uint64_t naMostConnections;
    unsigned int nMostConnections = 0;
    int64_t nMostConnectionsTime = 0;
    std::map<uint64_t, std::vector<NodeEvictionCandidate> > mapNetGroupNodes;
    for (const NodeEvictionCandidate &node : vEvictionCandidates) {
        std::vector<NodeEvictionCandidate> &group = mapNetGroupNodes[node.nKeyedNetGroup];
        group.push_back(node);
        int64_t grouptime = group[0].nTimeConnected;

        if (group.size() > nMostConnections || (group.size() == nMostConnections && grouptime > nMostConnectionsTime)) {
            nMostConnections = group.size();
            nMostConnectionsTime = grouptime;
            naMostConnections = node.nKeyedNetGroup;
        }
    }

    // Reduce to the network group with the most connections
    vEvictionCandidates = std::move(mapNetGroupNodes[naMostConnections]);

    // Disconnect from the network group with the most connections
    NodeId evicted = vEvictionCandidates.front().id;
    LOCK(cs_vNodes);
    for (CNode* pnode : vNodes) {
        if (pnode->GetId() == evicted) {
            pnode->fDisconnect = true;
            return true;
        }
    }
    return false;
}

void CConnman::AcceptConnection(const ListenSocket& hListenSocket) {
    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);
    SOCKET hSocket = accept(hListenSocket.socket, (struct sockaddr*)&sockaddr, &len);
    CAddress addr;
    int nInbound = 0;
    int nVerifiedInboundMasternodes = 0;
    int nMaxInbound = nMaxConnections - (nMaxOutbound + nMaxFeeler);

    if (hSocket != INVALID_SOCKET) {
        if (!addr.SetSockAddr((const struct sockaddr*)&sockaddr)) {
            LogPrintf("Warning: Unknown socket family\n");
        }
    }

    bool whitelisted = hListenSocket.whitelisted || IsWhitelistedRange(addr);
    {
        LOCK(cs_vNodes);
        for (const CNode* pnode : vNodes) {
            if (pnode->fInbound) {
                nInbound++;
                if (!pnode->verifiedProRegTxHash.IsNull()) {
                    nVerifiedInboundMasternodes++;
                }
            }
        }

    }

    if (hSocket == INVALID_SOCKET)
    {
        int nErr = WSAGetLastError();
        if (nErr != WSAEWOULDBLOCK)
            LogPrintf("socket error accept failed: %s\n", NetworkErrorString(nErr));
        return;
    }

    std::string strDropped;
    if (fLogIPs) {
        strDropped = strprintf("connection from %s dropped", addr.ToString());
    } else {
        strDropped = "connection dropped";
    }

    if (!fNetworkActive) {
        LogPrintf("%s: not accepting new connections\n", strDropped);
        CloseSocket(hSocket);
        return;
    }

    if (!IsSelectableSocket(hSocket))
    {
        LogPrintf("%s: non-selectable socket\n", strDropped);
        CloseSocket(hSocket);
        return;
    }

    // According to the internet TCP_NODELAY is not carried into accepted sockets
    // on all platforms.  Set it again here just to be sure.
    SetSocketNoDelay(hSocket);

    if (IsBanned(addr) && !whitelisted)
    {
        LogPrint(BCLog::NET, "%s (banned)\n", strDropped);
        CloseSocket(hSocket);
        return;
    }

    // Evict connections until we are below nMaxInbound. In case eviction protection resulted in nodes to not be evicted
    // before, they might get evicted in batches now (after the protection timeout).
    // We don't evict verified MN connections and also don't take them into account when checking limits. We can do this
    // because we know that such connections are naturally limited by the total number of MNs, so this is not usable
    // for attacks.
    while (nInbound - nVerifiedInboundMasternodes >= nMaxInbound)
    {
        if (!AttemptToEvictConnection()) {
            // No connection to evict, disconnect the new connection
            LogPrint(BCLog::NET, "failed to find an eviction candidate - connection dropped (full)\n");
            CloseSocket(hSocket);
            return;
        }
        nInbound--;
    }

    // don't accept incoming connections until fully synced
    if(fMasternodeMode && !masternodeSync.IsSynced()) {
        LogPrint(BCLog::NET, "AcceptConnection -- masternode is not synced yet, skipping inbound connection attempt\n");
        CloseSocket(hSocket);
        return;
    }

    NodeId id = GetNewNodeId();
    uint64_t nonce = GetDeterministicRandomizer(RANDOMIZER_ID_LOCALHOSTNONCE).Write(id).Finalize();
    CAddress addr_bind = GetBindAddress(hSocket);

    CNode* pnode = new CNode(id, nLocalServices, GetBestHeight(), hSocket, addr, CalculateKeyedNetGroup(addr), nonce, addr_bind, "", true);
    pnode->AddRef();
    pnode->fWhitelisted = whitelisted;
    m_msgproc->InitializeNode(pnode);

    if (fLogIPs) {
        LogPrint(BCLog::NET_NETCONN, "connection from %s accepted, sock=%d, peer=%d\n", addr.ToString(), pnode->hSocket, pnode->GetId());
    } else {
        LogPrint(BCLog::NET_NETCONN, "connection accepted, sock=%d, peer=%d\n", pnode->hSocket, pnode->GetId());
    }

    {
        LOCK(cs_vNodes);
        vNodes.push_back(pnode);
        mapSocketToNode.emplace(pnode->hSocket, pnode);
        RegisterEvents(pnode);
        WakeSelect();
    }
}

void CConnman::DisconnectNodes()
{
    {
        LOCK(cs_vNodes);

        if (!fNetworkActive) {
            // Disconnect any connected nodes
            for (CNode* pnode : vNodes) {
                if (!pnode->fDisconnect) {
                    LogPrint(BCLog::NET, "Network not active, dropping peer=%d\n", pnode->GetId());
                    pnode->fDisconnect = true;
                }
            }
        }

        // Disconnect unused nodes
        for (auto it = vNodes.begin(); it != vNodes.end(); )
        {
            CNode* pnode = *it;
            if (pnode->fDisconnect)
            {
                // If we were the ones who initiated the disconnect, we must assume that the other side wants to see
                // pending messages. If the other side initiated the disconnect (or disconnected after we've shutdown
                // the socket), we can be pretty sure that they are not interested in any pending messages anymore and
                // thus can immediately close the socket.
                if (!pnode->fOtherSideDisconnected) {
                    if (pnode->nDisconnectLingerTime == 0) {
                        // let's not immediately close the socket but instead wait for at least 100ms so that there is a
                        // chance to flush all/some pending data. Otherwise the other side might not receive REJECT messages
                        // that were pushed right before setting fDisconnect=true
                        // Flushing must happen in two places to ensure data can be received by the other side:
                        //   1. vSendMsg must be empty and all messages sent via send(). This is ensured by SocketHandler()
                        //      being called before DisconnectNodes and also by the linger time
                        //   2. Internal socket send buffers must be flushed. This is ensured solely by the linger time
                        pnode->nDisconnectLingerTime = GetTimeMillis() + 100;
                    }
                    if (GetTimeMillis() < pnode->nDisconnectLingerTime) {
                        // everything flushed to the kernel?
                        if (!pnode->fSocketShutdown && pnode->nSendMsgSize == 0) {
                            LOCK(pnode->cs_hSocket);
                            if (pnode->hSocket != INVALID_SOCKET) {
                                // Give the other side a chance to detect the disconnect as early as possible (recv() will return 0)
                                ::shutdown(pnode->hSocket, SD_SEND);
                            }
                            pnode->fSocketShutdown = true;
                        }
                        ++it;
                        continue;
                    }
                }

                if (fLogIPs) {
                    LogPrintf("ThreadSocketHandler -- removing node: peer=%d addr=%s nRefCount=%d fInbound=%d m_masternode_connection=%d m_masternode_iqr_connection=%d\n",
                          pnode->GetId(), pnode->addr.ToString(), pnode->GetRefCount(), pnode->fInbound, pnode->m_masternode_connection, pnode->m_masternode_iqr_connection);
                } else {
                    LogPrintf("ThreadSocketHandler -- removing node: peer=%d nRefCount=%d fInbound=%d m_masternode_connection=%d m_masternode_iqr_connection=%d\n",
                          pnode->GetId(), pnode->GetRefCount(), pnode->fInbound, pnode->m_masternode_connection, pnode->m_masternode_iqr_connection);
                }

                // remove from vNodes
                it = vNodes.erase(it);

                // release outbound grant (if any)
                pnode->grantOutbound.Release();

                // close socket and cleanup
                pnode->CloseSocketDisconnect(this);

                // hold in disconnected pool until all refs are released
                pnode->Release();
                vNodesDisconnected.push_back(pnode);
            } else {
                ++it;
            }
        }
    }
    {
        // Delete disconnected nodes
        std::list<CNode*> vNodesDisconnectedCopy = vNodesDisconnected;
        for (auto it = vNodesDisconnected.begin(); it != vNodesDisconnected.end(); )
        {
            CNode* pnode = *it;
            // wait until threads are done using it
            bool fDelete = false;
            if (pnode->GetRefCount() <= 0) {
                {
                    TRY_LOCK(pnode->cs_inventory, lockInv);
                    if (lockInv) {
                        TRY_LOCK(pnode->cs_vSend, lockSend);
                        if (lockSend) {
                            fDelete = true;
                        }
                    }
                }
                if (fDelete) {
                    it = vNodesDisconnected.erase(it);
                    DeleteNode(pnode);
                }
            }
            if (!fDelete) {
                ++it;
            }
        }
    }
}

void CConnman::NotifyNumConnectionsChanged()
{
    size_t vNodesSize;
    {
        LOCK(cs_vNodes);
        vNodesSize = vNodes.size();
    }

    // If we had zero connections before and new connections now or if we just dropped
    // to zero connections reset the sync process if its outdated.
    if ((vNodesSize > 0 && nPrevNodeCount == 0) || (vNodesSize == 0 && nPrevNodeCount > 0)) {
        masternodeSync.Reset();
    }

    if(vNodesSize != nPrevNodeCount) {
        nPrevNodeCount = vNodesSize;
        if(clientInterface)
            clientInterface->NotifyNumConnectionsChanged(vNodesSize);

        CalculateNumConnectionsChangedStats();
    }
}

void CConnman::CalculateNumConnectionsChangedStats()
{
    if (!gArgs.GetBoolArg("-statsenabled", DEFAULT_STATSD_ENABLE)) {
        return;
    }

    // count various node attributes for statsD
    int fullNodes = 0;
    int spvNodes = 0;
    int inboundNodes = 0;
    int outboundNodes = 0;
    int ipv4Nodes = 0;
    int ipv6Nodes = 0;
    int torNodes = 0;
    mapMsgCmdSize mapRecvBytesMsgStats;
    mapMsgCmdSize mapSentBytesMsgStats;
    for (const std::string &msg : getAllNetMessageTypes()) {
        mapRecvBytesMsgStats[msg] = 0;
        mapSentBytesMsgStats[msg] = 0;
    }
    mapRecvBytesMsgStats[NET_MESSAGE_COMMAND_OTHER] = 0;
    mapSentBytesMsgStats[NET_MESSAGE_COMMAND_OTHER] = 0;
    LOCK(cs_vNodes);
    for (const CNode* pnode : vNodes) {
        for (const mapMsgCmdSize::value_type &i : pnode->mapRecvBytesPerMsgCmd)
            mapRecvBytesMsgStats[i.first] += i.second;
        for (const mapMsgCmdSize::value_type &i : pnode->mapSendBytesPerMsgCmd)
            mapSentBytesMsgStats[i.first] += i.second;
        if(pnode->fClient)
            spvNodes++;
        else
            fullNodes++;
        if(pnode->fInbound)
            inboundNodes++;
        else
            outboundNodes++;
        if(pnode->addr.IsIPv4())
            ipv4Nodes++;
        if(pnode->addr.IsIPv6())
            ipv6Nodes++;
        if(pnode->addr.IsTor())
            torNodes++;
        if(pnode->nPingUsecTime > 0)
            statsClient.timing("peers.ping_us", pnode->nPingUsecTime, 1.0f);
    }
    for (const std::string &msg : getAllNetMessageTypes()) {
        statsClient.gauge("bandwidth.message." + msg + ".totalBytesReceived", mapRecvBytesMsgStats[msg], 1.0f);
        statsClient.gauge("bandwidth.message." + msg + ".totalBytesSent", mapSentBytesMsgStats[msg], 1.0f);
    }
    statsClient.gauge("peers.totalConnections", nPrevNodeCount, 1.0f);
    statsClient.gauge("peers.spvNodeConnections", spvNodes, 1.0f);
    statsClient.gauge("peers.fullNodeConnections", fullNodes, 1.0f);
    statsClient.gauge("peers.inboundConnections", inboundNodes, 1.0f);
    statsClient.gauge("peers.outboundConnections", outboundNodes, 1.0f);
    statsClient.gauge("peers.ipv4Connections", ipv4Nodes, 1.0f);
    statsClient.gauge("peers.ipv6Connections", ipv6Nodes, 1.0f);
    statsClient.gauge("peers.torConnections", torNodes, 1.0f);
}

void CConnman::InactivityCheck(CNode *pnode)
{
    int64_t nTime = GetSystemTimeInSeconds();
    if (nTime - pnode->nTimeConnected > m_peer_connect_timeout)
    {
        if (pnode->nLastRecv == 0 || pnode->nLastSend == 0)
        {
            LogPrint(BCLog::NET, "socket no message in first %i seconds, %d %d from %d\n", m_peer_connect_timeout, pnode->nLastRecv != 0, pnode->nLastSend != 0, pnode->GetId());
            pnode->fDisconnect = true;
        }
        else if (nTime - pnode->nLastSend > TIMEOUT_INTERVAL)
        {
            LogPrintf("socket sending timeout: %is\n", nTime - pnode->nLastSend);
            pnode->fDisconnect = true;
        }
        else if (nTime - pnode->nLastRecv > TIMEOUT_INTERVAL)
        {
            LogPrintf("socket receive timeout: %is\n", nTime - pnode->nLastRecv);
            pnode->fDisconnect = true;
        }
        else if (pnode->nPingNonceSent && pnode->nPingUsecStart + TIMEOUT_INTERVAL * 1000000 < GetTimeMicros())
        {
            LogPrintf("ping timeout: %fs\n", 0.000001 * (GetTimeMicros() - pnode->nPingUsecStart));
            pnode->fDisconnect = true;
        }
        else if (!pnode->fSuccessfullyConnected)
        {
            LogPrint(BCLog::NET, "version handshake timeout from %d\n", pnode->GetId());
            pnode->fDisconnect = true;
        }
    }
}

bool CConnman::GenerateSelectSet(std::set<SOCKET> &recv_set, std::set<SOCKET> &send_set, std::set<SOCKET> &error_set)
{
    for (const ListenSocket& hListenSocket : vhListenSocket) {
        recv_set.insert(hListenSocket.socket);
    }

    {
        LOCK(cs_vNodes);
        for (CNode* pnode : vNodes)
        {
            bool select_recv = !pnode->fHasRecvData;
            bool select_send = !pnode->fCanSendData;

            LOCK(pnode->cs_hSocket);
            if (pnode->hSocket == INVALID_SOCKET)
                continue;

            error_set.insert(pnode->hSocket);
            if (select_send) {
                send_set.insert(pnode->hSocket);
            }
            if (select_recv) {
                recv_set.insert(pnode->hSocket);
            }
        }
    }

#ifdef USE_WAKEUP_PIPE
    // We add a pipe to the read set so that the select() call can be woken up from the outside
    // This is done when data is added to send buffers (vSendMsg) or when new peers are added
    // This is currently only implemented for POSIX compliant systems. This means that Windows will fall back to
    // timing out after 50ms and then trying to send. This is ok as we assume that heavy-load daemons are usually
    // run on Linux and friends.
    recv_set.insert(wakeupPipe[0]);
#endif

    return !recv_set.empty() || !send_set.empty() || !error_set.empty();
}

#ifdef USE_KQUEUE
void CConnman::SocketEventsKqueue(std::set<SOCKET> &recv_set, std::set<SOCKET> &send_set, std::set<SOCKET> &error_set, bool fOnlyPoll)
{
    const size_t maxEvents = 64;
    struct kevent events[maxEvents];

    struct timespec timeout;
    timeout.tv_sec = fOnlyPoll ? 0 : SELECT_TIMEOUT_MILLISECONDS / 1000;
    timeout.tv_nsec = (fOnlyPoll ? 0 : SELECT_TIMEOUT_MILLISECONDS % 1000) * 1000 * 1000;

    wakeupSelectNeeded = true;
    int n = kevent(kqueuefd, nullptr, 0, events, maxEvents, &timeout);
    wakeupSelectNeeded = false;
    if (n == -1) {
        LogPrintf("kevent wait error\n");
    } else if (n > 0) {
        for (int i = 0; i < n; i++) {
            auto& event = events[i];
            if ((event.flags & EV_ERROR) || (event.flags & EV_EOF)) {
                error_set.insert((SOCKET)event.ident);
                continue;
            }

            if (event.filter == EVFILT_READ) {
                recv_set.insert((SOCKET)event.ident);
            }

            if (event.filter == EVFILT_WRITE) {
                send_set.insert((SOCKET)event.ident);
            }
        }
    }
}
#endif

#ifdef USE_EPOLL
void CConnman::SocketEventsEpoll(std::set<SOCKET> &recv_set, std::set<SOCKET> &send_set, std::set<SOCKET> &error_set, bool fOnlyPoll)
{
    const size_t maxEvents = 64;
    epoll_event events[maxEvents];

    wakeupSelectNeeded = true;
    int n = epoll_wait(epollfd, events, maxEvents, fOnlyPoll ? 0 : SELECT_TIMEOUT_MILLISECONDS);
    wakeupSelectNeeded = false;
    for (int i = 0; i < n; i++) {
        auto& e = events[i];
        if((e.events & EPOLLERR) || (e.events & EPOLLHUP)) {
            error_set.insert((SOCKET)e.data.fd);
            continue;
        }

        if (e.events & EPOLLIN) {
            recv_set.insert((SOCKET)e.data.fd);
        }

        if (e.events & EPOLLOUT) {
            send_set.insert((SOCKET)e.data.fd);
        }
    }
}
#endif

#ifdef USE_POLL
void CConnman::SocketEventsPoll(std::set<SOCKET> &recv_set, std::set<SOCKET> &send_set, std::set<SOCKET> &error_set, bool fOnlyPoll)
{
    std::set<SOCKET> recv_select_set, send_select_set, error_select_set;
    if (!GenerateSelectSet(recv_select_set, send_select_set, error_select_set)) {
        if (!fOnlyPoll) interruptNet.sleep_for(std::chrono::milliseconds(SELECT_TIMEOUT_MILLISECONDS));
        return;
    }

    std::unordered_map<SOCKET, struct pollfd> pollfds;
    for (SOCKET socket_id : recv_select_set) {
        pollfds[socket_id].fd = socket_id;
        pollfds[socket_id].events |= POLLIN;
    }

    for (SOCKET socket_id : send_select_set) {
        pollfds[socket_id].fd = socket_id;
        pollfds[socket_id].events |= POLLOUT;
    }

    for (SOCKET socket_id : error_select_set) {
        pollfds[socket_id].fd = socket_id;
        // These flags are ignored, but we set them for clarity
        pollfds[socket_id].events |= POLLERR|POLLHUP;
    }

    std::vector<struct pollfd> vpollfds;
    vpollfds.reserve(pollfds.size());
    for (auto it : pollfds) {
        vpollfds.push_back(std::move(it.second));
    }

    wakeupSelectNeeded = true;
    int r = poll(vpollfds.data(), vpollfds.size(), fOnlyPoll ? 0 : SELECT_TIMEOUT_MILLISECONDS);
    wakeupSelectNeeded = false;
    if (r < 0) {
        return;
    }

    if (interruptNet) return;

    for (struct pollfd pollfd_entry : vpollfds) {
        if (pollfd_entry.revents & POLLIN)            recv_set.insert(pollfd_entry.fd);
        if (pollfd_entry.revents & POLLOUT)           send_set.insert(pollfd_entry.fd);
        if (pollfd_entry.revents & (POLLERR|POLLHUP)) error_set.insert(pollfd_entry.fd);
    }
}
#endif

void CConnman::SocketEventsSelect(std::set<SOCKET> &recv_set, std::set<SOCKET> &send_set, std::set<SOCKET> &error_set, bool fOnlyPoll)
{
    std::set<SOCKET> recv_select_set, send_select_set, error_select_set;
    if (!GenerateSelectSet(recv_select_set, send_select_set, error_select_set)) {
        interruptNet.sleep_for(std::chrono::milliseconds(SELECT_TIMEOUT_MILLISECONDS));
        return;
    }

    //
    // Find which sockets have data to receive
    //
    struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = fOnlyPoll ? 0 : SELECT_TIMEOUT_MILLISECONDS * 1000; // frequency to poll pnode->vSend

    fd_set fdsetRecv;
    fd_set fdsetSend;
    fd_set fdsetError;
    FD_ZERO(&fdsetRecv);
    FD_ZERO(&fdsetSend);
    FD_ZERO(&fdsetError);
    SOCKET hSocketMax = 0;

    for (SOCKET hSocket : recv_select_set) {
        FD_SET(hSocket, &fdsetRecv);
        hSocketMax = std::max(hSocketMax, hSocket);
    }

    for (SOCKET hSocket : send_select_set) {
        FD_SET(hSocket, &fdsetSend);
        hSocketMax = std::max(hSocketMax, hSocket);
    }

    for (SOCKET hSocket : error_select_set) {
        FD_SET(hSocket, &fdsetError);
        hSocketMax = std::max(hSocketMax, hSocket);
    }

    wakeupSelectNeeded = true;
    int nSelect = select(hSocketMax + 1, &fdsetRecv, &fdsetSend, &fdsetError, &timeout);
    wakeupSelectNeeded = false;
    if (interruptNet)
        return;

    if (nSelect == SOCKET_ERROR)
    {
        int nErr = WSAGetLastError();
        LogPrintf("socket select error %s\n", NetworkErrorString(nErr));
        for (unsigned int i = 0; i <= hSocketMax; i++)
            FD_SET(i, &fdsetRecv);
        FD_ZERO(&fdsetSend);
        FD_ZERO(&fdsetError);
        if (!interruptNet.sleep_for(std::chrono::milliseconds(SELECT_TIMEOUT_MILLISECONDS)))
            return;
    }

    for (SOCKET hSocket : recv_select_set) {
        if (FD_ISSET(hSocket, &fdsetRecv)) {
            recv_set.insert(hSocket);
        }
    }

    for (SOCKET hSocket : send_select_set) {
        if (FD_ISSET(hSocket, &fdsetSend)) {
            send_set.insert(hSocket);
        }
    }

    for (SOCKET hSocket : error_select_set) {
        if (FD_ISSET(hSocket, &fdsetError)) {
            error_set.insert(hSocket);
        }
    }
}

void CConnman::SocketEvents(std::set<SOCKET> &recv_set, std::set<SOCKET> &send_set, std::set<SOCKET> &error_set, bool fOnlyPoll)
{
    switch (socketEventsMode) {
#ifdef USE_KQUEUE
        case SOCKETEVENTS_KQUEUE:
            SocketEventsKqueue(recv_set, send_set, error_set, fOnlyPoll);
            break;
#endif
#ifdef USE_EPOLL
        case SOCKETEVENTS_EPOLL:
            SocketEventsEpoll(recv_set, send_set, error_set, fOnlyPoll);
            break;
#endif
#ifdef USE_POLL
        case SOCKETEVENTS_POLL:
            SocketEventsPoll(recv_set, send_set, error_set, fOnlyPoll);
            break;
#endif
        case SOCKETEVENTS_SELECT:
            SocketEventsSelect(recv_set, send_set, error_set, fOnlyPoll);
            break;
        default:
            assert(false);
    }
}

void CConnman::SocketHandler()
{
    bool fOnlyPoll = false;
    {
        // check if we have work to do and thus should avoid waiting for events
        LOCK2(cs_vNodes, cs_mapNodesWithDataToSend);
        if (!mapReceivableNodes.empty()) {
            fOnlyPoll = true;
        } else if (!mapSendableNodes.empty() && !mapNodesWithDataToSend.empty()) {
            // we must check if at least one of the nodes with pending messages is also sendable, as otherwise a single
            // node would be able to make the network thread busy with polling
            for (auto& p : mapNodesWithDataToSend) {
                if (mapSendableNodes.count(p.first)) {
                    fOnlyPoll = true;
                    break;
                }
            }
        }
    }

    std::set<SOCKET> recv_set, send_set, error_set;
    SocketEvents(recv_set, send_set, error_set, fOnlyPoll);

#ifdef USE_WAKEUP_PIPE
    // drain the wakeup pipe
    if (recv_set.count(wakeupPipe[0])) {
        char buf[128];
        while (true) {
            int r = read(wakeupPipe[0], buf, sizeof(buf));
            if (r <= 0) {
                break;
            }
        }
    }
#endif

    if (interruptNet) return;

    //
    // Accept new connections
    //
    for (const ListenSocket& hListenSocket : vhListenSocket)
    {
        if (hListenSocket.socket != INVALID_SOCKET && recv_set.count(hListenSocket.socket) > 0)
        {
            AcceptConnection(hListenSocket);
        }
    }

    std::vector<CNode*> vErrorNodes;
    std::vector<CNode*> vReceivableNodes;
    std::vector<CNode*> vSendableNodes;
    {
        LOCK(cs_vNodes);
        for (auto hSocket : error_set) {
            auto it = mapSocketToNode.find(hSocket);
            if (it == mapSocketToNode.end()) {
                continue;
            }
            it->second->AddRef();
            vErrorNodes.emplace_back(it->second);
        }
        for (auto hSocket : recv_set) {
            if (error_set.count(hSocket)) {
                // no need to handle it twice
                continue;
            }

            auto it = mapSocketToNode.find(hSocket);
            if (it == mapSocketToNode.end()) {
                continue;
            }

            auto jt = mapReceivableNodes.emplace(it->second->GetId(), it->second);
            assert(jt.first->second == it->second);
            it->second->fHasRecvData = true;
        }
        for (auto hSocket : send_set) {
            auto it = mapSocketToNode.find(hSocket);
            if (it == mapSocketToNode.end()) {
                continue;
            }

            auto jt = mapSendableNodes.emplace(it->second->GetId(), it->second);
            assert(jt.first->second == it->second);
            it->second->fCanSendData = true;
        }

        // collect nodes that have a receivable socket
        // also clean up mapReceivableNodes from nodes that were receivable in the last iteration but aren't anymore
        vReceivableNodes.reserve(mapReceivableNodes.size());
        for (auto it = mapReceivableNodes.begin(); it != mapReceivableNodes.end(); ) {
            if (!it->second->fHasRecvData) {
                it = mapReceivableNodes.erase(it);
            } else {
                // Implement the following logic:
                // * If there is data to send, try sending data. As this only
                //   happens when optimistic write failed, we choose to first drain the
                //   write buffer in this case before receiving more. This avoids
                //   needlessly queueing received data, if the remote peer is not themselves
                //   receiving data. This means properly utilizing TCP flow control signalling.
                // * Otherwise, if there is space left in the receive buffer (!fPauseRecv), try
                //   receiving data (which should succeed as the socket signalled as receivable).
                if (!it->second->fPauseRecv && it->second->nSendMsgSize == 0 && !it->second->fDisconnect) {
                    it->second->AddRef();
                    vReceivableNodes.emplace_back(it->second);
                }
                ++it;
            }
        }

        // collect nodes that have data to send and have a socket with non-empty write buffers
        // also clean up mapNodesWithDataToSend from nodes that had messages to send in the last iteration
        // but don't have any in this iteration
        LOCK(cs_mapNodesWithDataToSend);
        vSendableNodes.reserve(mapNodesWithDataToSend.size());
        for (auto it = mapNodesWithDataToSend.begin(); it != mapNodesWithDataToSend.end(); ) {
            if (it->second->nSendMsgSize == 0) {
                // See comment in PushMessage
                it->second->Release();
                it = mapNodesWithDataToSend.erase(it);
            } else {
                if (it->second->fCanSendData) {
                    it->second->AddRef();
                    vSendableNodes.emplace_back(it->second);
                }
                ++it;
            }
        }
    }

    for (CNode* pnode : vErrorNodes)
    {
        if (interruptNet) {
            break;
        }
        // let recv() return errors and then handle it
        SocketRecvData(pnode);
    }

    for (CNode* pnode : vReceivableNodes)
    {
        if (interruptNet) {
            break;
        }
        if (pnode->fPauseRecv) {
            continue;
        }

        SocketRecvData(pnode);
    }

    for (CNode* pnode : vSendableNodes) {
        if (interruptNet) {
            break;
        }

        LOCK(pnode->cs_vSend);
        size_t nBytes = SocketSendData(pnode);
        if (nBytes) {
            RecordBytesSent(nBytes);
        }
    }

    ReleaseNodeVector(vErrorNodes);
    ReleaseNodeVector(vReceivableNodes);
    ReleaseNodeVector(vSendableNodes);

    if (interruptNet) {
        return;
    }

    {
        LOCK(cs_vNodes);
        // remove nodes from mapSendableNodes, so that the next iteration knows that there is no work to do
        // (even if there are pending messages to be sent)
        for (auto it = mapSendableNodes.begin(); it != mapSendableNodes.end(); ) {
            if (!it->second->fCanSendData) {
                LogPrint(BCLog::NET, "%s -- remove mapSendableNodes, peer=%d\n", __func__, it->second->GetId());
                it = mapSendableNodes.erase(it);
            } else {
                ++it;
            }
        }
    }
}

size_t CConnman::SocketRecvData(CNode *pnode)
{
    // typical socket buffer is 8K-64K
    char pchBuf[0x10000];
    int nBytes = 0;
    {
        LOCK(pnode->cs_hSocket);
        if (pnode->hSocket == INVALID_SOCKET)
            return 0;
        nBytes = recv(pnode->hSocket, pchBuf, sizeof(pchBuf), MSG_DONTWAIT);
        if (nBytes < (int)sizeof(pchBuf)) {
            pnode->fHasRecvData = false;
        }
    }
    if (nBytes > 0)
    {
        bool notify = false;
        if (!pnode->ReceiveMsgBytes(pchBuf, nBytes, notify)) {
            LOCK(cs_vNodes);
            pnode->CloseSocketDisconnect(this);
        }
        RecordBytesRecv(nBytes);
        if (notify) {
            size_t nSizeAdded = 0;
            auto it(pnode->vRecvMsg.begin());
            for (; it != pnode->vRecvMsg.end(); ++it) {
                if (!it->complete())
                    break;
                nSizeAdded += it->vRecv.size() + CMessageHeader::HEADER_SIZE;
            }
            {
                LOCK(pnode->cs_vProcessMsg);
                pnode->vProcessMsg.splice(pnode->vProcessMsg.end(), pnode->vRecvMsg, pnode->vRecvMsg.begin(), it);
                pnode->nProcessQueueSize += nSizeAdded;
                pnode->fPauseRecv = pnode->nProcessQueueSize > nReceiveFloodSize;
            }
            WakeMessageHandler();
        }
    }
    else if (nBytes == 0)
    {
        // socket closed gracefully
        if (!pnode->fDisconnect) {
            LogPrint(BCLog::NET, "socket closed\n");
        }
        LOCK(cs_vNodes);
        pnode->fOtherSideDisconnected = true; // avoid lingering
        pnode->CloseSocketDisconnect(this);
    }
    else if (nBytes < 0)
    {
        // error
        int nErr = WSAGetLastError();
        if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
        {
            if (!pnode->fDisconnect)
                LogPrintf("socket recv error %s\n", NetworkErrorString(nErr));
            LOCK(cs_vNodes);
            pnode->fOtherSideDisconnected = true; // avoid lingering
            pnode->CloseSocketDisconnect(this);
        }
    }
    if (nBytes < 0) {
        return 0;
    }
    return (size_t)nBytes;
}

void CConnman::ThreadSocketHandler()
{
    int64_t nLastCleanupNodes = 0;

    while (!interruptNet)
    {
        // Handle sockets before we do the next round of disconnects. This allows us to flush send buffers one last time
        // before actually closing sockets. Receiving is however skipped in case a peer is pending to be disconnected
        SocketHandler();
        if (GetTimeMillis() - nLastCleanupNodes > 1000) {
            ForEachNode(AllNodes, [&](CNode* pnode) {
                InactivityCheck(pnode);
            });
            nLastCleanupNodes = GetTimeMillis();
        }
        DisconnectNodes();
        NotifyNumConnectionsChanged();
    }
}

void CConnman::WakeMessageHandler()
{
    {
        LOCK(mutexMsgProc);
        fMsgProcWake = true;
    }
    condMsgProc.notify_one();
}

void CConnman::WakeSelect()
{
#ifdef USE_WAKEUP_PIPE
    if (wakeupPipe[1] == -1) {
        return;
    }

    char buf{0};
    if (write(wakeupPipe[1], &buf, sizeof(buf)) != 1) {
        LogPrint(BCLog::NET, "write to wakeupPipe failed\n");
    }
#endif

    wakeupSelectNeeded = false;
}





#ifdef USE_UPNP
static CThreadInterrupt g_upnp_interrupt;
static std::thread g_upnp_thread;
static void ThreadMapPort()
{
    std::string port = strprintf("%u", GetListenPort());
    const char * multicastif = nullptr;
    const char * minissdpdpath = nullptr;
    struct UPNPDev * devlist = nullptr;
    char lanaddr[64];

#ifndef UPNPDISCOVER_SUCCESS
    /* miniupnpc 1.5 */
    devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0);
#elif MINIUPNPC_API_VERSION < 14
    /* miniupnpc 1.6 */
    int error = 0;
    devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0, 0, &error);
#else
    /* miniupnpc 1.9.20150730 */
    int error = 0;
    devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0, 0, 2, &error);
#endif

    struct UPNPUrls urls;
    struct IGDdatas data;
    int r;

    r = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
    if (r == 1)
    {
        if (fDiscover) {
            char externalIPAddress[40];
            r = UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIPAddress);
            if(r != UPNPCOMMAND_SUCCESS)
                LogPrintf("UPnP: GetExternalIPAddress() returned %d\n", r);
            else
            {
                if(externalIPAddress[0])
                {
                    CNetAddr resolved;
                    if(LookupHost(externalIPAddress, resolved, false)) {
                        LogPrintf("UPnP: ExternalIPAddress = %s\n", resolved.ToString().c_str());
                        AddLocal(resolved, LOCAL_UPNP);
                    }
                }
                else
                    LogPrintf("UPnP: GetExternalIPAddress failed.\n");
            }
        }

        std::string strDesc = "Dash Core " + FormatFullVersion();

        do {
#ifndef UPNPDISCOVER_SUCCESS
            /* miniupnpc 1.5 */
            r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                port.c_str(), port.c_str(), lanaddr, strDesc.c_str(), "TCP", 0);
#else
            /* miniupnpc 1.6 */
            r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                port.c_str(), port.c_str(), lanaddr, strDesc.c_str(), "TCP", 0, "0");
#endif

            if(r!=UPNPCOMMAND_SUCCESS)
                LogPrintf("AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
                    port, port, lanaddr, r, strupnperror(r));
            else
                LogPrintf("UPnP Port Mapping successful.\n");
        }
        while(g_upnp_interrupt.sleep_for(std::chrono::minutes(20)));

        r = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port.c_str(), "TCP", 0);
        LogPrintf("UPNP_DeletePortMapping() returned: %d\n", r);
        freeUPNPDevlist(devlist); devlist = nullptr;
        FreeUPNPUrls(&urls);
    } else {
        LogPrintf("No valid UPnP IGDs found\n");
        freeUPNPDevlist(devlist); devlist = nullptr;
        if (r != 0)
            FreeUPNPUrls(&urls);
    }
}

void StartMapPort()
{
    if (!g_upnp_thread.joinable()) {
        assert(!g_upnp_interrupt);
        g_upnp_thread = std::thread((std::bind(&TraceThread<void (*)()>, "upnp", &ThreadMapPort)));
    }
}

void InterruptMapPort()
{
    if(g_upnp_thread.joinable()) {
        g_upnp_interrupt();
    }
}

void StopMapPort()
{
    if(g_upnp_thread.joinable()) {
        g_upnp_thread.join();
        g_upnp_interrupt.reset();
    }
}

#else
void StartMapPort()
{
    // Intentionally left blank.
}
void InterruptMapPort()
{
    // Intentionally left blank.
}
void StopMapPort()
{
    // Intentionally left blank.
}
#endif






void CConnman::ThreadDNSAddressSeed()
{
    FastRandomContext rng;
    std::vector<std::string> seeds = Params().DNSSeeds();
    Shuffle(seeds.begin(), seeds.end(), rng);
    int seeds_right_now = 0; // Number of seeds left before testing if we have enough connections
    int found = 0;

    if (gArgs.GetBoolArg("-forcednsseed", DEFAULT_FORCEDNSSEED)) {
        // When -forcednsseed is provided, query all.
        seeds_right_now = seeds.size();
    }

    for (const std::string& seed : seeds) {
        // goal: only query DNS seed if address need is acute
        // Avoiding DNS seeds when we don't need them improves user privacy by
        // creating fewer identifying DNS requests, reduces trust by giving seeds
        // less influence on the network topology, and reduces traffic to the seeds.
        if (addrman.size() > 0 && seeds_right_now == 0) {
            if (!interruptNet.sleep_for(std::chrono::seconds(11))) return;

            LOCK(cs_vNodes);
            int nRelevant = 0;
            for (auto pnode : vNodes) {
                nRelevant += pnode->fSuccessfullyConnected && !pnode->fFeeler && !pnode->fOneShot && !pnode->m_manual_connection && !pnode->fInbound && !pnode->m_masternode_probe_connection;
            }
            if (nRelevant >= 2) {
                LogPrintf("P2P peers available. Skipped DNS seeding.\n");
                return;
            }
            seeds_right_now += DNSSEEDS_TO_QUERY_AT_ONCE;
        }

        if (interruptNet) {
            return;
        }
        LogPrintf("Loading addresses from DNS seed %s\n", seed);
        if (HaveNameProxy()) {
            AddOneShot(seed);
        } else {
            std::vector<CNetAddr> vIPs;
            std::vector<CAddress> vAdd;
            ServiceFlags requiredServiceBits = GetDesirableServiceFlags(NODE_NONE);
            std::string host = strprintf("x%x.%s", requiredServiceBits, seed);
            CNetAddr resolveSource;
            if (!resolveSource.SetInternal(host)) {
                continue;
            }
            unsigned int nMaxIPs = 256; // Limits number of IPs learned from a DNS seed
            if (LookupHost(host.c_str(), vIPs, nMaxIPs, true)) {
                for (const CNetAddr& ip : vIPs) {
                    int nOneDay = 24*3600;
                    CAddress addr = CAddress(CService(ip, Params().GetDefaultPort()), requiredServiceBits);
                    addr.nTime = GetTime() - 3*nOneDay - rng.randrange(4*nOneDay); // use a random age between 3 and 7 days old
                    vAdd.push_back(addr);
                    found++;
                }
                addrman.Add(vAdd, resolveSource);
            } else {
                // We now avoid directly using results from DNS Seeds which do not support service bit filtering,
                // instead using them as a oneshot to get nodes with our desired service bits.
                AddOneShot(seed);
            }
        }
        --seeds_right_now;
    }
    LogPrintf("%d addresses found from DNS seeds\n", found);
}












void CConnman::DumpAddresses()
{
    int64_t nStart = GetTimeMillis();

    CAddrDB adb;
    adb.Write(addrman);

    LogPrint(BCLog::NET, "Flushed %d addresses to peers.dat  %dms\n",
           addrman.size(), GetTimeMillis() - nStart);
}

void CConnman::DumpData()
{
    DumpAddresses();
    DumpBanlist();
}

void CConnman::ProcessOneShot()
{
    std::string strDest;
    {
        LOCK(cs_vOneShots);
        if (vOneShots.empty())
            return;
        strDest = vOneShots.front();
        vOneShots.pop_front();
    }
    CAddress addr;
    CSemaphoreGrant grant(*semOutbound, true);
    if (grant) {
        OpenNetworkConnection(addr, false, &grant, strDest.c_str(), true);
    }
}

bool CConnman::GetTryNewOutboundPeer()
{
    return m_try_another_outbound_peer;
}

void CConnman::SetTryNewOutboundPeer(bool flag)
{
    m_try_another_outbound_peer = flag;
    LogPrint(BCLog::NET, "net: setting try another outbound peer=%s\n", flag ? "true" : "false");
}

// Return the number of peers we have over our outbound connection limit
// Exclude peers that are marked for disconnect, or are going to be
// disconnected soon (eg one-shots and feelers)
// Also exclude peers that haven't finished initial connection handshake yet
// (so that we don't decide we're over our desired connection limit, and then
// evict some peer that has finished the handshake)
int CConnman::GetExtraOutboundCount()
{
    int nOutbound = 0;
    {
        LOCK(cs_vNodes);
        for (CNode* pnode : vNodes) {
            // don't count outbound masternodes
            if (pnode->m_masternode_connection) {
                continue;
            }
            if (!pnode->fInbound && !pnode->m_manual_connection && !pnode->fFeeler && !pnode->fDisconnect && !pnode->fOneShot && pnode->fSuccessfullyConnected && !pnode->m_masternode_probe_connection) {
                ++nOutbound;
            }
        }
    }
    return std::max(nOutbound - nMaxOutbound, 0);
}

void CConnman::ThreadOpenConnections(const std::vector<std::string> connect)
{
    // Connect to specific addresses
    if (!connect.empty())
    {
        for (int64_t nLoop = 0;; nLoop++)
        {
            ProcessOneShot();
            for (const std::string& strAddr : connect)
            {
                CAddress addr(CService(), NODE_NONE);
                OpenNetworkConnection(addr, false, nullptr, strAddr.c_str(), false, false, true);
                for (int i = 0; i < 10 && i < nLoop; i++)
                {
                    if (!interruptNet.sleep_for(std::chrono::milliseconds(500)))
                        return;
                }
            }
            if (!interruptNet.sleep_for(std::chrono::milliseconds(500)))
                return;
        }
    }

    // Initiate network connections
    int64_t nStart = GetTime();

    // Minimum time before next feeler connection (in microseconds).
    int64_t nNextFeeler = PoissonNextSend(nStart*1000*1000, FEELER_INTERVAL);
    while (!interruptNet)
    {
        ProcessOneShot();

        if (!interruptNet.sleep_for(std::chrono::milliseconds(500)))
            return;

        CSemaphoreGrant grant(*semOutbound);
        if (interruptNet)
            return;

        // Add seed nodes if DNS seeds are all down (an infrastructure attack?).
        if (addrman.size() == 0 && (GetTime() - nStart > 60)) {
            static bool done = false;
            if (!done) {
                LogPrintf("Adding fixed seed nodes as DNS doesn't seem to be available.\n");
                CNetAddr local;
                local.SetInternal("fixedseeds");
                addrman.Add(convertSeed6(Params().FixedSeeds()), local);
                done = true;
            }
        }

        //
        // Choose an address to connect to based on most recently seen
        //
        CAddress addrConnect;

        // Only connect out to one peer per network group (/16 for IPv4).
        // This is only done for mainnet and testnet
        int nOutbound = 0;
        std::set<std::vector<unsigned char> > setConnected;
        if (!Params().AllowMultipleAddressesFromGroup()) {
            LOCK(cs_vNodes);
            for (CNode* pnode : vNodes) {
                if (!pnode->fInbound && !pnode->m_masternode_connection && !pnode->m_manual_connection) {
                    // Netgroups for inbound and addnode peers are not excluded because our goal here
                    // is to not use multiple of our limited outbound slots on a single netgroup
                    // but inbound and addnode peers do not use our outbound slots.  Inbound peers
                    // also have the added issue that they're attacker controlled and could be used
                    // to prevent us from connecting to particular hosts if we used them here.
                    setConnected.insert(pnode->addr.GetGroup(addrman.m_asmap));
                    nOutbound++;
                }
            }
        }

        std::set<uint256> setConnectedMasternodes;
        {
            LOCK(cs_vNodes);
            for (CNode* pnode : vNodes) {
                if (!pnode->verifiedProRegTxHash.IsNull()) {
                    setConnectedMasternodes.emplace(pnode->verifiedProRegTxHash);
                }
            }
        }

        // Feeler Connections
        //
        // Design goals:
        //  * Increase the number of connectable addresses in the tried table.
        //
        // Method:
        //  * Choose a random address from new and attempt to connect to it if we can connect
        //    successfully it is added to tried.
        //  * Start attempting feeler connections only after node finishes making outbound
        //    connections.
        //  * Only make a feeler connection once every few minutes.
        //
        bool fFeeler = false;

        if (nOutbound >= nMaxOutbound && !GetTryNewOutboundPeer()) {
            int64_t nTime = GetTimeMicros(); // The current time right now (in microseconds).
            if (nTime > nNextFeeler) {
                nNextFeeler = PoissonNextSend(nTime, FEELER_INTERVAL);
                fFeeler = true;
            } else {
                continue;
            }
        }

        addrman.ResolveCollisions();

        auto mnList = deterministicMNManager->GetListAtChainTip();

        int64_t nANow = GetAdjustedTime();
        int nTries = 0;
        while (!interruptNet)
        {
            CAddrInfo addr = addrman.SelectTriedCollision();

            // SelectTriedCollision returns an invalid address if it is empty.
            if (!fFeeler || !addr.IsValid()) {
                addr = addrman.Select(fFeeler);
            }

            auto dmn = mnList.GetMNByService(addr);
            bool isMasternode = dmn != nullptr;

            // Require outbound connections, other than feelers, to be to distinct network groups
            if (!fFeeler && setConnected.count(addr.GetGroup(addrman.m_asmap))) {
                break;
            }

            // if we selected an invalid address, restart
            if (!addr.IsValid() || setConnected.count(addr.GetGroup(addrman.m_asmap)))
                break;

            // don't try to connect to masternodes that we already have a connection to (most likely inbound)
            if (isMasternode && setConnectedMasternodes.count(dmn->proTxHash))
                break;

            // if we selected a local address, restart (local addresses are allowed in regtest and devnet)
            bool fAllowLocal = Params().AllowMultiplePorts() && addrConnect.GetPort() != GetListenPort();
            if (!fAllowLocal && IsLocal(addrConnect)) {
                break;
            }

            // If we didn't find an appropriate destination after trying 100 addresses fetched from addrman,
            // stop this loop, and let the outer loop run again (which sleeps, adds seed nodes, recalculates
            // already-connected network ranges, ...) before trying new addrman addresses.
            nTries++;
            if (nTries > 100)
                break;

            if (IsLimited(addr))
                continue;

            // only consider very recently tried nodes after 30 failed attempts
            if (nANow - addr.nLastTry < 600 && nTries < 30)
                continue;

            // for non-feelers, require all the services we'll want,
            // for feelers, only require they be a full node (only because most
            // SPV clients don't have a good address DB available)
            if (!isMasternode && !fFeeler && !HasAllDesirableServiceFlags(addr.nServices)) {
                continue;
            } else if (!isMasternode && fFeeler && !MayHaveUsefulAddressDB(addr.nServices)) {
                continue;
            }

            // do not allow non-default ports, unless after 50 invalid addresses selected already
            if ((!isMasternode || !Params().AllowMultiplePorts()) && addr.GetPort() != Params().GetDefaultPort() && addr.GetPort() != GetListenPort() && nTries < 50)
                continue;

            addrConnect = addr;
            break;
        }

        if (addrConnect.IsValid()) {

            if (fFeeler) {
                // Add small amount of random noise before connection to avoid synchronization.
                int randsleep = GetRandInt(FEELER_SLEEP_WINDOW * 1000);
                if (!interruptNet.sleep_for(std::chrono::milliseconds(randsleep)))
                    return;
                if (fLogIPs) {
                    LogPrint(BCLog::NET, "Making feeler connection to %s\n", addrConnect.ToString());
                } else {
                    LogPrint(BCLog::NET, "Making feeler connection\n");
                }
            }

            OpenNetworkConnection(addrConnect, (int)setConnected.size() >= std::min(nMaxConnections - 1, 2), &grant, nullptr, false, fFeeler);
        }
    }
}

std::vector<AddedNodeInfo> CConnman::GetAddedNodeInfo()
{
    std::vector<AddedNodeInfo> ret;

    std::list<std::string> lAddresses(0);
    {
        LOCK(cs_vAddedNodes);
        ret.reserve(vAddedNodes.size());
        std::copy(vAddedNodes.cbegin(), vAddedNodes.cend(), std::back_inserter(lAddresses));
    }


    // Build a map of all already connected addresses (by IP:port and by name) to inbound/outbound and resolved CService
    std::map<CService, bool> mapConnected;
    std::map<std::string, std::pair<bool, CService>> mapConnectedByName;
    {
        LOCK(cs_vNodes);
        for (const CNode* pnode : vNodes) {
            if (pnode->addr.IsValid()) {
                mapConnected[pnode->addr] = pnode->fInbound;
            }
            std::string addrName = pnode->GetAddrName();
            if (!addrName.empty()) {
                mapConnectedByName[std::move(addrName)] = std::make_pair(pnode->fInbound, static_cast<const CService&>(pnode->addr));
            }
        }
    }

    for (const std::string& strAddNode : lAddresses) {
        CService service(LookupNumeric(strAddNode.c_str(), Params().GetDefaultPort()));
        AddedNodeInfo addedNode{strAddNode, CService(), false, false};
        if (service.IsValid()) {
            // strAddNode is an IP:port
            auto it = mapConnected.find(service);
            if (it != mapConnected.end()) {
                addedNode.resolvedAddress = service;
                addedNode.fConnected = true;
                addedNode.fInbound = it->second;
            }
        } else {
            // strAddNode is a name
            auto it = mapConnectedByName.find(strAddNode);
            if (it != mapConnectedByName.end()) {
                addedNode.resolvedAddress = it->second.second;
                addedNode.fConnected = true;
                addedNode.fInbound = it->second.first;
            }
        }
        ret.emplace_back(std::move(addedNode));
    }

    return ret;
}

void CConnman::ThreadOpenAddedConnections()
{
    while (true)
    {
        CSemaphoreGrant grant(*semAddnode);
        std::vector<AddedNodeInfo> vInfo = GetAddedNodeInfo();
        bool tried = false;
        for (const AddedNodeInfo& info : vInfo) {
            if (!info.fConnected) {
                if (!grant.TryAcquire()) {
                    // If we've used up our semaphore and need a new one, let's not wait here since while we are waiting
                    // the addednodeinfo state might change.
                    break;
                }
                tried = true;
                CAddress addr(CService(), NODE_NONE);
                OpenNetworkConnection(addr, false, &grant, info.strAddedNode.c_str(), false, false, true);
                if (!interruptNet.sleep_for(std::chrono::milliseconds(500)))
                    return;
            }
        }
        // Retry every 60 seconds if a connection was attempted, otherwise two seconds
        if (!interruptNet.sleep_for(std::chrono::seconds(tried ? 60 : 2)))
            return;
    }
}

void CConnman::ThreadOpenMasternodeConnections()
{
    // Connecting to specific addresses, no masternode connections available
    if (gArgs.IsArgSet("-connect") && gArgs.GetArgs("-connect").size() > 0)
        return;

    auto& chainParams = Params();

    bool didConnect = false;
    while (!interruptNet)
    {
        int sleepTime = 1000;
        if (didConnect) {
            sleepTime = 100;
        }
        if (!interruptNet.sleep_for(std::chrono::milliseconds(sleepTime)))
            return;

        didConnect = false;

        if (!fNetworkActive || !masternodeSync.IsBlockchainSynced())
            continue;

        std::set<CService> connectedNodes;
        std::map<uint256, bool> connectedProRegTxHashes;
        ForEachNode([&](const CNode* pnode) {
            connectedNodes.emplace(pnode->addr);
            if (!pnode->verifiedProRegTxHash.IsNull()) {
                connectedProRegTxHashes.emplace(pnode->verifiedProRegTxHash, pnode->fInbound);
            }
        });

        auto mnList = deterministicMNManager->GetListAtChainTip();

        if (interruptNet)
            return;

        int64_t nANow = GetAdjustedTime();

        // NOTE: Process only one pending masternode at a time

        CDeterministicMNCPtr connectToDmn;
        bool isProbe = false;
        { // don't hold lock while calling OpenMasternodeConnection as cs_main is locked deep inside
            LOCK2(cs_vNodes, cs_vPendingMasternodes);

            if (!vPendingMasternodes.empty()) {
                auto dmn = mnList.GetValidMN(vPendingMasternodes.front());
                vPendingMasternodes.erase(vPendingMasternodes.begin());
                if (dmn && !connectedNodes.count(dmn->pdmnState->addr) && !IsMasternodeOrDisconnectRequested(dmn->pdmnState->addr)) {
                    connectToDmn = dmn;
                    LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- opening pending masternode connection to %s, service=%s\n", __func__, dmn->proTxHash.ToString(), dmn->pdmnState->addr.ToString(false));
                }
            }

            if (!connectToDmn) {
                std::vector<CDeterministicMNCPtr> pending;
                for (const auto& group : masternodeQuorumNodes) {
                    for (const auto& proRegTxHash : group.second) {
                        auto dmn = mnList.GetMN(proRegTxHash);
                        if (!dmn) {
                            continue;
                        }
                        const auto& addr2 = dmn->pdmnState->addr;
                        if (!connectedNodes.count(addr2) && !IsMasternodeOrDisconnectRequested(addr2) && !connectedProRegTxHashes.count(proRegTxHash)) {
                            int64_t lastAttempt = mmetaman.GetMetaInfo(dmn->proTxHash)->GetLastOutboundAttempt();
                            // back off trying connecting to an address if we already tried recently
                            if (nANow - lastAttempt < chainParams.LLMQConnectionRetryTimeout()) {
                                continue;
                            }
                            pending.emplace_back(dmn);
                        }
                    }
                }

                if (!pending.empty()) {
                    connectToDmn = pending[GetRandInt(pending.size())];
                    LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- opening quorum connection to %s, service=%s\n", __func__, connectToDmn->proTxHash.ToString(), connectToDmn->pdmnState->addr.ToString(false));
                }
            }

            if (!connectToDmn) {
                std::vector<CDeterministicMNCPtr> pending;
                for (auto it = masternodePendingProbes.begin(); it != masternodePendingProbes.end(); ) {
                    auto dmn = mnList.GetMN(*it);
                    if (!dmn) {
                        it = masternodePendingProbes.erase(it);
                        continue;
                    }
                    bool connectedAndOutbound = connectedProRegTxHashes.count(dmn->proTxHash) && !connectedProRegTxHashes[dmn->proTxHash];
                    if (connectedAndOutbound) {
                        // we already have an outbound connection to this MN so there is no theed to probe it again
                        mmetaman.GetMetaInfo(dmn->proTxHash)->SetLastOutboundSuccess(nANow);
                        it = masternodePendingProbes.erase(it);
                        continue;
                    }

                    ++it;

                    int64_t lastAttempt = mmetaman.GetMetaInfo(dmn->proTxHash)->GetLastOutboundAttempt();
                    // back off trying connecting to an address if we already tried recently
                    if (nANow - lastAttempt < chainParams.LLMQConnectionRetryTimeout()) {
                        continue;
                    }
                    pending.emplace_back(dmn);
                }

                if (!pending.empty()) {
                    connectToDmn = pending[GetRandInt(pending.size())];
                    masternodePendingProbes.erase(connectToDmn->proTxHash);
                    isProbe = true;

                    LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- probing masternode %s, service=%s\n", __func__, connectToDmn->proTxHash.ToString(), connectToDmn->pdmnState->addr.ToString(false));
                }
            }
        }

        if (!connectToDmn) {
            continue;
        }

        didConnect = true;

        mmetaman.GetMetaInfo(connectToDmn->proTxHash)->SetLastOutboundAttempt(nANow);

        OpenMasternodeConnection(CAddress(connectToDmn->pdmnState->addr, NODE_NETWORK), isProbe);
        // should be in the list now if connection was opened
        bool connected = ForNode(connectToDmn->pdmnState->addr, CConnman::AllNodes, [&](CNode* pnode) {
            if (pnode->fDisconnect) {
                return false;
            }
            return true;
        });
        if (!connected) {
            LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- connection failed for masternode  %s, service=%s\n", __func__, connectToDmn->proTxHash.ToString(), connectToDmn->pdmnState->addr.ToString(false));
            // reset last outbound success
            mmetaman.GetMetaInfo(connectToDmn->proTxHash)->SetLastOutboundSuccess(0);
        }
    }
}

// if successful, this moves the passed grant to the constructed node
void CConnman::OpenNetworkConnection(const CAddress& addrConnect, bool fCountFailure, CSemaphoreGrant *grantOutbound, const char *pszDest, bool fOneShot, bool fFeeler, bool manual_connection, bool masternode_connection, bool masternode_probe_connection)
{
    //
    // Initiate outbound network connection
    //
    if (interruptNet) {
        return;
    }
    if (!fNetworkActive) {
        return;
    }
    if (!pszDest) {
        // banned or exact match?
        if (IsBanned(addrConnect) || FindNode(addrConnect.ToStringIPPort()))
            return;
        // local and not a connection to itself?
        bool fAllowLocal = Params().AllowMultiplePorts() && addrConnect.GetPort() != GetListenPort();
        if (!fAllowLocal && IsLocal(addrConnect))
            return;
        // if multiple ports for same IP are allowed, search for IP:PORT match, otherwise search for IP-only match
        if ((!Params().AllowMultiplePorts() && FindNode(static_cast<CNetAddr>(addrConnect))) ||
            (Params().AllowMultiplePorts() && FindNode(static_cast<CService>(addrConnect))))
            return;
    } else if (FindNode(std::string(pszDest)))
        return;

    auto getIpStr = [&]() {
        if (fLogIPs) {
            return addrConnect.ToString(false);
        } else {
            return std::string("new peer");
        }
    };

    LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- connecting to %s\n", __func__, getIpStr());
    CNode* pnode = ConnectNode(addrConnect, pszDest, fCountFailure, manual_connection);

    if (!pnode) {
        LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- ConnectNode failed for %s\n", __func__, getIpStr());
        return;
    }
    LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- succesfully connected to %s, sock=%d, peer=%d\n", __func__, getIpStr(), pnode->hSocket, pnode->GetId());
    if (grantOutbound)
        grantOutbound->MoveTo(pnode->grantOutbound);
    if (fOneShot)
        pnode->fOneShot = true;
    if (fFeeler)
        pnode->fFeeler = true;
    if (manual_connection)
        pnode->m_manual_connection = true;
    if (masternode_connection)
        pnode->m_masternode_connection = true;
    if (masternode_probe_connection)
        pnode->m_masternode_probe_connection = true;

    {
        LOCK(cs_vNodes);
        mapSocketToNode.emplace(pnode->hSocket, pnode);
    }

    m_msgproc->InitializeNode(pnode);
    {
        LOCK(cs_vNodes);
        vNodes.push_back(pnode);
        RegisterEvents(pnode);
        WakeSelect();
    }
}

void CConnman::OpenMasternodeConnection(const CAddress &addrConnect, bool probe) {
    OpenNetworkConnection(addrConnect, false, nullptr, nullptr, false, false, false, true, probe);
}

void CConnman::ThreadMessageHandler()
{
    int64_t nLastSendMessagesTimeMasternodes = 0;

    while (!flagInterruptMsgProc)
    {
        std::vector<CNode*> vNodesCopy = CopyNodeVector();

        bool fMoreWork = false;

        bool fSkipSendMessagesForMasternodes = true;
        if (GetTimeMillis() - nLastSendMessagesTimeMasternodes >= 100) {
            fSkipSendMessagesForMasternodes = false;
            nLastSendMessagesTimeMasternodes = GetTimeMillis();
        }

        for (CNode* pnode : vNodesCopy)
        {
            if (pnode->fDisconnect)
                continue;

            // Receive messages
            bool fMoreNodeWork = m_msgproc->ProcessMessages(pnode, flagInterruptMsgProc);
            fMoreWork |= (fMoreNodeWork && !pnode->fPauseSend);
            if (flagInterruptMsgProc)
                return;
            // Send messages
            if (!fSkipSendMessagesForMasternodes || !pnode->m_masternode_connection) {
                LOCK(pnode->cs_sendProcessing);
                m_msgproc->SendMessages(pnode);
            }

            if (flagInterruptMsgProc)
                return;
        }

        ReleaseNodeVector(vNodesCopy);

        WAIT_LOCK(mutexMsgProc, lock);
        if (!fMoreWork) {
            condMsgProc.wait_until(lock, std::chrono::steady_clock::now() + std::chrono::milliseconds(100), [this]() EXCLUSIVE_LOCKS_REQUIRED(mutexMsgProc) { return fMsgProcWake; });
        }
        fMsgProcWake = false;
    }
}






bool CConnman::BindListenPort(const CService &addrBind, std::string& strError, bool fWhitelisted)
{
    strError = "";
    int nOne = 1;

    // Create socket for listening for incoming connections
    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);
    if (!addrBind.GetSockAddr((struct sockaddr*)&sockaddr, &len))
    {
        strError = strprintf("Error: Bind address family for %s not supported", addrBind.ToString());
        LogPrintf("%s\n", strError);
        return false;
    }

    SOCKET hListenSocket = CreateSocket(addrBind);
    if (hListenSocket == INVALID_SOCKET)
    {
        strError = strprintf("Error: Couldn't open socket for incoming connections (socket returned error %s)", NetworkErrorString(WSAGetLastError()));
        LogPrintf("%s\n", strError);
        return false;
    }

    // Allow binding if the port is still in TIME_WAIT state after
    // the program was closed and restarted.
    setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (sockopt_arg_type)&nOne, sizeof(int));

    // some systems don't have IPV6_V6ONLY but are always v6only; others do have the option
    // and enable it by default or not. Try to enable it, if possible.
    if (addrBind.IsIPv6()) {
#ifdef IPV6_V6ONLY
        setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (sockopt_arg_type)&nOne, sizeof(int));
#endif
#ifdef WIN32
        int nProtLevel = PROTECTION_LEVEL_UNRESTRICTED;
        setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_PROTECTION_LEVEL, (const char*)&nProtLevel, sizeof(int));
#endif
    }

    if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
    {
        int nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE)
            strError = strprintf(_("Unable to bind to %s on this computer. %s is probably already running."), addrBind.ToString(), _(PACKAGE_NAME));
        else
            strError = strprintf(_("Unable to bind to %s on this computer (bind returned error %s)"), addrBind.ToString(), NetworkErrorString(nErr));
        LogPrintf("%s\n", strError);
        CloseSocket(hListenSocket);
        return false;
    }
    LogPrintf("Bound to %s\n", addrBind.ToString());

    // Listen for incoming connections
    if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        strError = strprintf(_("Error: Listening for incoming connections failed (listen returned error %s)"), NetworkErrorString(WSAGetLastError()));
        LogPrintf("%s\n", strError);
        CloseSocket(hListenSocket);
        return false;
    }

#ifdef USE_KQUEUE
    if (socketEventsMode == SOCKETEVENTS_KQUEUE) {
        struct kevent event;
        EV_SET(&event, hListenSocket, EVFILT_READ, EV_ADD, 0, 0, nullptr);
        if (kevent(kqueuefd, &event, 1, nullptr, 0, nullptr) != 0) {
            strError = strprintf(_("Error: failed to add socket to kqueuefd (kevent returned error %s)"), NetworkErrorString(WSAGetLastError()));
            LogPrintf("%s\n", strError);
            CloseSocket(hListenSocket);
            return false;
        }
    }
#endif

#ifdef USE_EPOLL
    if (socketEventsMode == SOCKETEVENTS_EPOLL) {
        epoll_event event;
        event.data.fd = hListenSocket;
        event.events = EPOLLIN;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, hListenSocket, &event) != 0) {
            strError = strprintf(_("Error: failed to add socket to epollfd (epoll_ctl returned error %s)"), NetworkErrorString(WSAGetLastError()));
            LogPrintf("%s\n", strError);
            CloseSocket(hListenSocket);
            return false;
        }
    }
#endif

    vhListenSocket.push_back(ListenSocket(hListenSocket, fWhitelisted));

    if (addrBind.IsRoutable() && fDiscover && !fWhitelisted)
        AddLocal(addrBind, LOCAL_BIND);

    return true;
}

void Discover()
{
    if (!fDiscover)
        return;

#ifdef WIN32
    // Get local host IP
    char pszHostName[256] = "";
    if (gethostname(pszHostName, sizeof(pszHostName)) != SOCKET_ERROR)
    {
        std::vector<CNetAddr> vaddr;
        if (LookupHost(pszHostName, vaddr, 0, true))
        {
            for (const CNetAddr &addr : vaddr)
            {
                if (AddLocal(addr, LOCAL_IF))
                    LogPrintf("%s: %s - %s\n", __func__, pszHostName, addr.ToString());
            }
        }
    }
#else
    // Get local host ip
    struct ifaddrs* myaddrs;
    if (getifaddrs(&myaddrs) == 0)
    {
        for (struct ifaddrs* ifa = myaddrs; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == nullptr) continue;
            if ((ifa->ifa_flags & IFF_UP) == 0) continue;
            if (strcmp(ifa->ifa_name, "lo") == 0) continue;
            if (strcmp(ifa->ifa_name, "lo0") == 0) continue;
            if (ifa->ifa_addr->sa_family == AF_INET)
            {
                struct sockaddr_in* s4 = (struct sockaddr_in*)(ifa->ifa_addr);
                CNetAddr addr(s4->sin_addr);
                if (AddLocal(addr, LOCAL_IF))
                    LogPrintf("%s: IPv4 %s: %s\n", __func__, ifa->ifa_name, addr.ToString());
            }
            else if (ifa->ifa_addr->sa_family == AF_INET6)
            {
                struct sockaddr_in6* s6 = (struct sockaddr_in6*)(ifa->ifa_addr);
                CNetAddr addr(s6->sin6_addr);
                if (AddLocal(addr, LOCAL_IF))
                    LogPrintf("%s: IPv6 %s: %s\n", __func__, ifa->ifa_name, addr.ToString());
            }
        }
        freeifaddrs(myaddrs);
    }
#endif
}

void CConnman::SetNetworkActive(bool active)
{
    LogPrint(BCLog::NET, "SetNetworkActive: %s\n", active);

    if (fNetworkActive == active) {
        return;
    }

    fNetworkActive = active;

    // Always call the Reset() if the network gets enabled/disabled to make sure the sync process
    // gets a reset if its outdated..
    masternodeSync.Reset();

    uiInterface.NotifyNetworkActiveChanged(fNetworkActive);
}

CConnman::CConnman(uint64_t nSeed0In, uint64_t nSeed1In) :
        addrman(Params().AllowMultiplePorts()),
        nSeed0(nSeed0In), nSeed1(nSeed1In)
{
    fNetworkActive = true;
    setBannedIsDirty = false;
    fAddressesInitialized = false;
    nLastNodeId = 0;
    nPrevNodeCount = 0;
    nSendBufferMaxSize = 0;
    nReceiveFloodSize = 0;
    flagInterruptMsgProc = false;
    SetTryNewOutboundPeer(false);

    Options connOptions;
    Init(connOptions);
}

NodeId CConnman::GetNewNodeId()
{
    return nLastNodeId.fetch_add(1, std::memory_order_relaxed);
}


bool CConnman::Bind(const CService &addr, unsigned int flags) {
    if (!(flags & BF_EXPLICIT) && IsLimited(addr))
        return false;
    std::string strError;
    if (!BindListenPort(addr, strError, (flags & BF_WHITELIST) != 0)) {
        if ((flags & BF_REPORT_ERROR) && clientInterface) {
            clientInterface->ThreadSafeMessageBox(strError, "", CClientUIInterface::MSG_ERROR);
        }
        return false;
    }
    return true;
}

bool CConnman::InitBinds(const std::vector<CService>& binds, const std::vector<CService>& whiteBinds) {
    bool fBound = false;
    for (const auto& addrBind : binds) {
        fBound |= Bind(addrBind, (BF_EXPLICIT | BF_REPORT_ERROR));
    }
    for (const auto& addrBind : whiteBinds) {
        fBound |= Bind(addrBind, (BF_EXPLICIT | BF_REPORT_ERROR | BF_WHITELIST));
    }
    if (binds.empty() && whiteBinds.empty()) {
        struct in_addr inaddr_any;
        inaddr_any.s_addr = INADDR_ANY;
        struct in6_addr inaddr6_any = IN6ADDR_ANY_INIT;
        fBound |= Bind(CService(inaddr6_any, GetListenPort()), BF_NONE);
        fBound |= Bind(CService(inaddr_any, GetListenPort()), !fBound ? BF_REPORT_ERROR : BF_NONE);
    }
    return fBound;
}

bool CConnman::Start(CScheduler& scheduler, const Options& connOptions)
{
    Init(connOptions);

    {
        LOCK(cs_totalBytesRecv);
        nTotalBytesRecv = 0;
    }
    {
        LOCK(cs_totalBytesSent);
        nTotalBytesSent = 0;
        nMaxOutboundTotalBytesSentInCycle = 0;
        nMaxOutboundCycleStartTime = 0;
    }

#ifdef USE_KQUEUE
    if (socketEventsMode == SOCKETEVENTS_KQUEUE) {
        kqueuefd = kqueue();
        if (kqueuefd == -1) {
            LogPrintf("kqueue failed\n");
            return false;
        }
    }
#endif

#ifdef USE_EPOLL
    if (socketEventsMode == SOCKETEVENTS_EPOLL) {
        epollfd = epoll_create1(0);
        if (epollfd == -1) {
            LogPrintf("epoll_create1 failed\n");
            return false;
        }
    }
#endif

    if (fListen && !InitBinds(connOptions.vBinds, connOptions.vWhiteBinds)) {
        if (clientInterface) {
            clientInterface->ThreadSafeMessageBox(
                _("Failed to listen on any port. Use -listen=0 if you want this."),
                "", CClientUIInterface::MSG_ERROR);
        }
        return false;
    }

    for (const auto& strDest : connOptions.vSeedNodes) {
        AddOneShot(strDest);
    }

    if (clientInterface) {
        clientInterface->InitMessage(_("Loading P2P addresses..."));
    }
    // Load addresses from peers.dat
    int64_t nStart = GetTimeMillis();
    {
        CAddrDB adb;
        if (adb.Read(addrman))
            LogPrintf("Loaded %i addresses from peers.dat  %dms\n", addrman.size(), GetTimeMillis() - nStart);
        else {
            addrman.Clear(); // Addrman can be in an inconsistent state after failure, reset it
            LogPrintf("Invalid or missing peers.dat; recreating\n");
            DumpAddresses();
        }
    }
    if (clientInterface)
        clientInterface->InitMessage(_("Loading banlist..."));
    // Load addresses from banlist.dat
    nStart = GetTimeMillis();
    CBanDB bandb;
    banmap_t banmap;
    if (bandb.Read(banmap)) {
        SetBanned(banmap); // thread save setter
        SetBannedSetDirty(false); // no need to write down, just read data
        SweepBanned(); // sweep out unused entries

        LogPrint(BCLog::NET, "Loaded %d banned node ips/subnets from banlist.dat  %dms\n",
            banmap.size(), GetTimeMillis() - nStart);
    } else {
        LogPrintf("Invalid or missing banlist.dat; recreating\n");
        SetBannedSetDirty(true); // force write
        DumpBanlist();
    }

    uiInterface.InitMessage(_("Starting network threads..."));

    fAddressesInitialized = true;

    if (semOutbound == nullptr) {
        // initialize semaphore
        semOutbound = MakeUnique<CSemaphore>(std::min((nMaxOutbound + nMaxFeeler), nMaxConnections));
    }
    if (semAddnode == nullptr) {
        // initialize semaphore
        semAddnode = MakeUnique<CSemaphore>(nMaxAddnode);
    }

    //
    // Start threads
    //
    assert(m_msgproc);
    InterruptSocks5(false);
    interruptNet.reset();
    flagInterruptMsgProc = false;

    {
        LOCK(mutexMsgProc);
        fMsgProcWake = false;
    }

#ifdef USE_WAKEUP_PIPE
    if (pipe(wakeupPipe) != 0) {
        wakeupPipe[0] = wakeupPipe[1] = -1;
        LogPrint(BCLog::NET, "pipe() for wakeupPipe failed\n");
    } else {
        int fFlags = fcntl(wakeupPipe[0], F_GETFL, 0);
        if (fcntl(wakeupPipe[0], F_SETFL, fFlags | O_NONBLOCK) == -1) {
            LogPrint(BCLog::NET, "fcntl for O_NONBLOCK on wakeupPipe failed\n");
        }
        fFlags = fcntl(wakeupPipe[1], F_GETFL, 0);
        if (fcntl(wakeupPipe[1], F_SETFL, fFlags | O_NONBLOCK) == -1) {
            LogPrint(BCLog::NET, "fcntl for O_NONBLOCK on wakeupPipe failed\n");
        }
#ifdef USE_KQUEUE
        if (socketEventsMode == SOCKETEVENTS_KQUEUE) {
            struct kevent event;
            EV_SET(&event, wakeupPipe[0], EVFILT_READ, EV_ADD, 0, 0, nullptr);
            int r = kevent(kqueuefd, &event, 1, nullptr, 0, nullptr);
            if (r != 0) {
                LogPrint(BCLog::NET, "%s -- kevent(%d, %d, %d, ...) failed. error: %s\n", __func__,
                         kqueuefd, EV_ADD, wakeupPipe[0], NetworkErrorString(WSAGetLastError()));
                return false;
            }
        }
#endif
#ifdef USE_EPOLL
        if (socketEventsMode == SOCKETEVENTS_EPOLL) {
            epoll_event event;
            event.events = EPOLLIN;
            event.data.fd = wakeupPipe[0];
            int r = epoll_ctl(epollfd, EPOLL_CTL_ADD, wakeupPipe[0], &event);
            if (r != 0) {
                LogPrint(BCLog::NET, "%s -- epoll_ctl(%d, %d, %d, ...) failed. error: %s\n", __func__,
                         epollfd, EPOLL_CTL_ADD, wakeupPipe[0], NetworkErrorString(WSAGetLastError()));
                return false;
            }
        }
#endif
    }
#endif

    // Send and receive from sockets, accept connections
    threadSocketHandler = std::thread(&TraceThread<std::function<void()> >, "net", std::function<void()>(std::bind(&CConnman::ThreadSocketHandler, this)));

    if (!gArgs.GetBoolArg("-dnsseed", true))
        LogPrintf("DNS seeding disabled\n");
    else
        threadDNSAddressSeed = std::thread(&TraceThread<std::function<void()> >, "dnsseed", std::function<void()>(std::bind(&CConnman::ThreadDNSAddressSeed, this)));

    // Initiate outbound connections from -addnode
    threadOpenAddedConnections = std::thread(&TraceThread<std::function<void()> >, "addcon", std::function<void()>(std::bind(&CConnman::ThreadOpenAddedConnections, this)));

    if (connOptions.m_use_addrman_outgoing && !connOptions.m_specified_outgoing.empty()) {
        if (clientInterface) {
            clientInterface->ThreadSafeMessageBox(
                _("Cannot provide specific connections and have addrman find outgoing connections at the same."),
                "", CClientUIInterface::MSG_ERROR);
        }
        return false;
    }
    if (connOptions.m_use_addrman_outgoing || !connOptions.m_specified_outgoing.empty())
        threadOpenConnections = std::thread(&TraceThread<std::function<void()> >, "opencon", std::function<void()>(std::bind(&CConnman::ThreadOpenConnections, this, connOptions.m_specified_outgoing)));

    // Initiate masternode connections
    threadOpenMasternodeConnections = std::thread(&TraceThread<std::function<void()> >, "mncon", std::function<void()>(std::bind(&CConnman::ThreadOpenMasternodeConnections, this)));

    // Process messages
    threadMessageHandler = std::thread(&TraceThread<std::function<void()> >, "msghand", std::function<void()>(std::bind(&CConnman::ThreadMessageHandler, this)));

    // Dump network addresses
    scheduler.scheduleEvery(std::bind(&CConnman::DumpData, this), DUMP_ADDRESSES_INTERVAL * 1000);

    return true;
}

class CNetCleanup
{
public:
    CNetCleanup() {}

    ~CNetCleanup()
    {
#ifdef WIN32
        // Shutdown Windows Sockets
        WSACleanup();
#endif
    }
}
instance_of_cnetcleanup;

void CExplicitNetCleanup::callCleanup()
{
    // Explicit call to destructor of CNetCleanup because it's not implicitly called
    // when the wallet is restarted from within the wallet itself.
    CNetCleanup *tmp = new CNetCleanup();
    delete tmp; // Stroustrup's gonna kill me for that
}

void CConnman::Interrupt()
{
    {
        LOCK(mutexMsgProc);
        flagInterruptMsgProc = true;
    }
    condMsgProc.notify_all();

    interruptNet();
    InterruptSocks5(true);

    if (semOutbound) {
        for (int i=0; i<(nMaxOutbound + nMaxFeeler); i++) {
            semOutbound->post();
        }
    }

    if (semAddnode) {
        for (int i=0; i<nMaxAddnode; i++) {
            semAddnode->post();
        }
    }
}

void CConnman::Stop()
{
    if (threadMessageHandler.joinable())
        threadMessageHandler.join();
    if (threadOpenMasternodeConnections.joinable())
        threadOpenMasternodeConnections.join();
    if (threadOpenConnections.joinable())
        threadOpenConnections.join();
    if (threadOpenAddedConnections.joinable())
        threadOpenAddedConnections.join();
    if (threadDNSAddressSeed.joinable())
        threadDNSAddressSeed.join();
    if (threadSocketHandler.joinable())
        threadSocketHandler.join();

    if (fAddressesInitialized)
    {
        DumpData();
        fAddressesInitialized = false;
    }

    {
        LOCK(cs_vNodes);

        // Close sockets
        for (CNode *pnode : vNodes)
            pnode->CloseSocketDisconnect(this);
    }
    for (ListenSocket& hListenSocket : vhListenSocket)
        if (hListenSocket.socket != INVALID_SOCKET) {
#ifdef USE_KQUEUE
            if (socketEventsMode == SOCKETEVENTS_KQUEUE) {
                struct kevent event;
                EV_SET(&event, hListenSocket.socket, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
                kevent(kqueuefd, &event, 1, nullptr, 0, nullptr);
            }
#endif
#ifdef USE_EPOLL
            if (socketEventsMode == SOCKETEVENTS_EPOLL) {
                epoll_ctl(epollfd, EPOLL_CTL_DEL, hListenSocket.socket, nullptr);
            }
#endif
            if (!CloseSocket(hListenSocket.socket))
                LogPrintf("CloseSocket(hListenSocket) failed with error %s\n", NetworkErrorString(WSAGetLastError()));
        }

    // clean up some globals (to help leak detection)
    for (CNode *pnode : vNodes) {
        DeleteNode(pnode);
    }
    for (CNode *pnode : vNodesDisconnected) {
        DeleteNode(pnode);
    }
    vNodes.clear();
    mapSocketToNode.clear();
    mapReceivableNodes.clear();
    {
        LOCK(cs_mapNodesWithDataToSend);
        mapNodesWithDataToSend.clear();
    }
    vNodesDisconnected.clear();
    vhListenSocket.clear();
    semOutbound.reset();
    semAddnode.reset();

#ifdef USE_KQUEUE
    if (socketEventsMode == SOCKETEVENTS_KQUEUE && kqueuefd != -1) {
#ifdef USE_WAKEUP_PIPE
        struct kevent event;
        EV_SET(&event, wakeupPipe[0], EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        kevent(kqueuefd, &event, 1, nullptr, 0, nullptr);
#endif
        close(kqueuefd);
    }
    kqueuefd = -1;
#endif
#ifdef USE_EPOLL
    if (socketEventsMode == SOCKETEVENTS_EPOLL && epollfd != -1) {
#ifdef USE_WAKEUP_PIPE
        epoll_ctl(epollfd, EPOLL_CTL_DEL, wakeupPipe[0], nullptr);
#endif
        close(epollfd);
    }
    epollfd = -1;
#endif

#ifdef USE_WAKEUP_PIPE
    if (wakeupPipe[0] != -1) close(wakeupPipe[0]);
    if (wakeupPipe[1] != -1) close(wakeupPipe[1]);
    wakeupPipe[0] = wakeupPipe[1] = -1;
#endif
}

void CConnman::DeleteNode(CNode* pnode)
{
    assert(pnode);
    bool fUpdateConnectionTime = false;
    m_msgproc->FinalizeNode(pnode->GetId(), fUpdateConnectionTime);
    if(fUpdateConnectionTime) {
        addrman.Connected(pnode->addr);
    }
    delete pnode;
}

CConnman::~CConnman()
{
    Interrupt();
    Stop();
}

size_t CConnman::GetAddressCount() const
{
    return addrman.size();
}

void CConnman::SetServices(const CService &addr, ServiceFlags nServices)
{
    addrman.SetServices(addr, nServices);
}

void CConnman::MarkAddressGood(const CAddress& addr)
{
    addrman.Good(addr);
}

void CConnman::AddNewAddresses(const std::vector<CAddress>& vAddr, const CAddress& addrFrom, int64_t nTimePenalty)
{
    addrman.Add(vAddr, addrFrom, nTimePenalty);
}

std::vector<CAddress> CConnman::GetAddresses()
{
    return addrman.GetAddr();
}

bool CConnman::AddNode(const std::string& strNode)
{
    LOCK(cs_vAddedNodes);
    for (const std::string& it : vAddedNodes) {
        if (strNode == it) return false;
    }

    vAddedNodes.push_back(strNode);
    return true;
}

bool CConnman::RemoveAddedNode(const std::string& strNode)
{
    LOCK(cs_vAddedNodes);
    for(std::vector<std::string>::iterator it = vAddedNodes.begin(); it != vAddedNodes.end(); ++it) {
        if (strNode == *it) {
            vAddedNodes.erase(it);
            return true;
        }
    }
    return false;
}

bool CConnman::AddPendingMasternode(const uint256& proTxHash)
{
    LOCK(cs_vPendingMasternodes);
    if (std::find(vPendingMasternodes.begin(), vPendingMasternodes.end(), proTxHash) != vPendingMasternodes.end()) {
        return false;
    }

    vPendingMasternodes.push_back(proTxHash);
    return true;
}

void CConnman::SetMasternodeQuorumNodes(Consensus::LLMQType llmqType, const uint256& quorumHash, const std::set<uint256>& proTxHashes)
{
    LOCK(cs_vPendingMasternodes);
    auto it = masternodeQuorumNodes.emplace(std::make_pair(llmqType, quorumHash), proTxHashes);
    if (!it.second) {
        it.first->second = proTxHashes;
    }
}

void CConnman::SetMasternodeQuorumRelayMembers(Consensus::LLMQType llmqType, const uint256& quorumHash, const std::set<uint256>& proTxHashes)
{
    {
        LOCK(cs_vPendingMasternodes);
        auto it = masternodeQuorumRelayMembers.emplace(std::make_pair(llmqType, quorumHash), proTxHashes);
        if (!it.second) {
            it.first->second = proTxHashes;
        }
    }

    // Update existing connections
    ForEachNode([&](CNode* pnode) {
        if (!pnode->verifiedProRegTxHash.IsNull() && !pnode->m_masternode_iqr_connection && IsMasternodeQuorumRelayMember(pnode->verifiedProRegTxHash)) {
            // Tell our peer that we're interested in plain LLMQ recovered signatures.
            // Otherwise the peer would only announce/send messages resulting from QRECSIG,
            // e.g. InstantSend locks or ChainLocks. SPV and regular full nodes should not send
            // this message as they are usually only interested in the higher level messages.
            const CNetMsgMaker msgMaker(pnode->GetSendVersion());
            PushMessage(pnode, msgMaker.Make(NetMsgType::QSENDRECSIGS, true));
            pnode->m_masternode_iqr_connection = true;
        }
    });
}

bool CConnman::HasMasternodeQuorumNodes(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    LOCK(cs_vPendingMasternodes);
    return masternodeQuorumNodes.count(std::make_pair(llmqType, quorumHash));
}

std::set<uint256> CConnman::GetMasternodeQuorums(Consensus::LLMQType llmqType)
{
    LOCK(cs_vPendingMasternodes);
    std::set<uint256> result;
    for (const auto& p : masternodeQuorumNodes) {
        if (p.first.first != llmqType) {
            continue;
        }
        result.emplace(p.first.second);
    }
    return result;
}

std::set<NodeId> CConnman::GetMasternodeQuorumNodes(Consensus::LLMQType llmqType, const uint256& quorumHash) const
{
    LOCK2(cs_vNodes, cs_vPendingMasternodes);
    auto it = masternodeQuorumNodes.find(std::make_pair(llmqType, quorumHash));
    if (it == masternodeQuorumNodes.end()) {
        return {};
    }
    const auto& proRegTxHashes = it->second;

    std::set<NodeId> nodes;
    for (const auto pnode : vNodes) {
        if (pnode->fDisconnect) {
            continue;
        }
        if (!pnode->qwatch && (pnode->verifiedProRegTxHash.IsNull() || !proRegTxHashes.count(pnode->verifiedProRegTxHash))) {
            continue;
        }
        nodes.emplace(pnode->GetId());
    }
    return nodes;
}

void CConnman::RemoveMasternodeQuorumNodes(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    LOCK(cs_vPendingMasternodes);
    masternodeQuorumNodes.erase(std::make_pair(llmqType, quorumHash));
    masternodeQuorumRelayMembers.erase(std::make_pair(llmqType, quorumHash));
}

bool CConnman::IsMasternodeQuorumNode(const CNode* pnode)
{
    // Let's see if this is an outgoing connection to an address that is known to be a masternode
    // We however only need to know this if the node did not authenticate itself as a MN yet
    uint256 assumedProTxHash;
    if (pnode->verifiedProRegTxHash.IsNull() && !pnode->fInbound) {
        auto mnList = deterministicMNManager->GetListAtChainTip();
        auto dmn = mnList.GetMNByService(pnode->addr);
        if (dmn == nullptr) {
            // This is definitely not a masternode
            return false;
        }
        assumedProTxHash = dmn->proTxHash;
    }

    LOCK(cs_vPendingMasternodes);
    for (const auto& p : masternodeQuorumNodes) {
        if (!pnode->verifiedProRegTxHash.IsNull()) {
            if (p.second.count(pnode->verifiedProRegTxHash)) {
                return true;
            }
        } else if (!assumedProTxHash.IsNull()) {
            if (p.second.count(assumedProTxHash)) {
                return true;
            }
        }
    }
    return false;
}

bool CConnman::IsMasternodeQuorumRelayMember(const uint256& protxHash)
{
    if (protxHash.IsNull()) {
        return false;
    }
    LOCK(cs_vPendingMasternodes);
    for (const auto& p : masternodeQuorumRelayMembers) {
        if (p.second.count(protxHash)) {
            return true;
        }
    }
    return false;
}

void CConnman::AddPendingProbeConnections(const std::set<uint256> &proTxHashes)
{
    LOCK(cs_vPendingMasternodes);
    masternodePendingProbes.insert(proTxHashes.begin(), proTxHashes.end());
}

size_t CConnman::GetNodeCount(NumConnections flags)
{
    LOCK(cs_vNodes);

    int nNum = 0;
    for (const auto& pnode : vNodes) {
        if (pnode->fDisconnect) {
            continue;
        }
        if (flags & (pnode->fInbound ? CONNECTIONS_IN : CONNECTIONS_OUT)) {
            nNum++;
        }
    }

    return nNum;
}

size_t CConnman::GetMaxOutboundNodeCount()
{
    return nMaxOutbound;
}

void CConnman::GetNodeStats(std::vector<CNodeStats>& vstats)
{
    vstats.clear();
    LOCK(cs_vNodes);
    vstats.reserve(vNodes.size());
    for (CNode* pnode : vNodes) {
        if (pnode->fDisconnect) {
            continue;
        }
        vstats.emplace_back();
        pnode->copyStats(vstats.back(), addrman.m_asmap);
    }
}

bool CConnman::DisconnectNode(const std::string& strNode)
{
    LOCK(cs_vNodes);
    if (CNode* pnode = FindNode(strNode)) {
        pnode->fDisconnect = true;
        return true;
    }
    return false;
}
bool CConnman::DisconnectNode(NodeId id)
{
    LOCK(cs_vNodes);
    for(CNode* pnode : vNodes) {
        if (id == pnode->GetId()) {
            pnode->fDisconnect = true;
            return true;
        }
    }
    return false;
}

void CConnman::RelayTransaction(const CTransaction& tx)
{
    uint256 hash = tx.GetHash();
    int nInv = MSG_TX;
    if (CCoinJoin::GetDSTX(hash)) {
        nInv = MSG_DSTX;
    }
    CInv inv(nInv, hash);
    RelayInv(inv);
}

void CConnman::RelayInv(CInv &inv, const int minProtoVersion) {
    LOCK(cs_vNodes);
    for (const auto& pnode : vNodes) {
        if (pnode->nVersion < minProtoVersion || !pnode->CanRelay())
            continue;
        pnode->PushInventory(inv);
    }
}

void CConnman::RelayInvFiltered(CInv &inv, const CTransaction& relatedTx, const int minProtoVersion)
{
    LOCK(cs_vNodes);
    for (const auto& pnode : vNodes) {
        if (pnode->nVersion < minProtoVersion || !pnode->CanRelay())
            continue;
        {
            LOCK(pnode->cs_filter);
            if(pnode->pfilter && !pnode->pfilter->IsRelevantAndUpdate(relatedTx))
                continue;
        }
        pnode->PushInventory(inv);
    }
}

void CConnman::RelayInvFiltered(CInv &inv, const uint256& relatedTxHash, const int minProtoVersion)
{
    LOCK(cs_vNodes);
    for (const auto& pnode : vNodes) {
        if (pnode->nVersion < minProtoVersion || !pnode->CanRelay())
            continue;
        {
            LOCK(pnode->cs_filter);
            if(pnode->pfilter && !pnode->pfilter->contains(relatedTxHash)) continue;
        }
        pnode->PushInventory(inv);
    }
}

void CConnman::RecordBytesRecv(uint64_t bytes)
{
    LOCK(cs_totalBytesRecv);
    nTotalBytesRecv += bytes;
    statsClient.count("bandwidth.bytesReceived", bytes, 0.1f);
    statsClient.gauge("bandwidth.totalBytesReceived", nTotalBytesRecv, 0.01f);
}

void CConnman::RecordBytesSent(uint64_t bytes)
{
    LOCK(cs_totalBytesSent);
    nTotalBytesSent += bytes;
    statsClient.count("bandwidth.bytesSent", bytes, 0.01f);
    statsClient.gauge("bandwidth.totalBytesSent", nTotalBytesSent, 0.01f);

    uint64_t now = GetTime();
    if (nMaxOutboundCycleStartTime + nMaxOutboundTimeframe < now)
    {
        // timeframe expired, reset cycle
        nMaxOutboundCycleStartTime = now;
        nMaxOutboundTotalBytesSentInCycle = 0;
    }

    // TODO, exclude whitebind peers
    nMaxOutboundTotalBytesSentInCycle += bytes;
}

void CConnman::SetMaxOutboundTarget(uint64_t limit)
{
    LOCK(cs_totalBytesSent);
    nMaxOutboundLimit = limit;
}

uint64_t CConnman::GetMaxOutboundTarget()
{
    LOCK(cs_totalBytesSent);
    return nMaxOutboundLimit;
}

uint64_t CConnman::GetMaxOutboundTimeframe()
{
    LOCK(cs_totalBytesSent);
    return nMaxOutboundTimeframe;
}

uint64_t CConnman::GetMaxOutboundTimeLeftInCycle()
{
    LOCK(cs_totalBytesSent);
    if (nMaxOutboundLimit == 0)
        return 0;

    if (nMaxOutboundCycleStartTime == 0)
        return nMaxOutboundTimeframe;

    uint64_t cycleEndTime = nMaxOutboundCycleStartTime + nMaxOutboundTimeframe;
    uint64_t now = GetTime();
    return (cycleEndTime < now) ? 0 : cycleEndTime - GetTime();
}

void CConnman::SetMaxOutboundTimeframe(uint64_t timeframe)
{
    LOCK(cs_totalBytesSent);
    if (nMaxOutboundTimeframe != timeframe)
    {
        // reset measure-cycle in case of changing
        // the timeframe
        nMaxOutboundCycleStartTime = GetTime();
    }
    nMaxOutboundTimeframe = timeframe;
}

bool CConnman::OutboundTargetReached(bool historicalBlockServingLimit)
{
    LOCK(cs_totalBytesSent);
    if (nMaxOutboundLimit == 0)
        return false;

    if (historicalBlockServingLimit)
    {
        // keep a large enough buffer to at least relay each block once
        uint64_t timeLeftInCycle = GetMaxOutboundTimeLeftInCycle();
        uint64_t buffer = timeLeftInCycle / 600 * MaxBlockSize(fDIP0001ActiveAtTip);
        if (buffer >= nMaxOutboundLimit || nMaxOutboundTotalBytesSentInCycle >= nMaxOutboundLimit - buffer)
            return true;
    }
    else if (nMaxOutboundTotalBytesSentInCycle >= nMaxOutboundLimit)
        return true;

    return false;
}

uint64_t CConnman::GetOutboundTargetBytesLeft()
{
    LOCK(cs_totalBytesSent);
    if (nMaxOutboundLimit == 0)
        return 0;

    return (nMaxOutboundTotalBytesSentInCycle >= nMaxOutboundLimit) ? 0 : nMaxOutboundLimit - nMaxOutboundTotalBytesSentInCycle;
}

uint64_t CConnman::GetTotalBytesRecv()
{
    LOCK(cs_totalBytesRecv);
    return nTotalBytesRecv;
}

uint64_t CConnman::GetTotalBytesSent()
{
    LOCK(cs_totalBytesSent);
    return nTotalBytesSent;
}

ServiceFlags CConnman::GetLocalServices() const
{
    return nLocalServices;
}

void CConnman::SetBestHeight(int height)
{
    nBestHeight.store(height, std::memory_order_release);
}

int CConnman::GetBestHeight() const
{
    return nBestHeight.load(std::memory_order_acquire);
}

unsigned int CConnman::GetReceiveFloodSize() const { return nReceiveFloodSize; }

CNode::CNode(NodeId idIn, ServiceFlags nLocalServicesIn, int nMyStartingHeightIn, SOCKET hSocketIn, const CAddress& addrIn, uint64_t nKeyedNetGroupIn, uint64_t nLocalHostNonceIn, const CAddress &addrBindIn, const std::string& addrNameIn, bool fInboundIn) :
    nTimeConnected(GetSystemTimeInSeconds()),
    nTimeFirstMessageReceived(0),
    fFirstMessageIsMNAUTH(false),
    addr(addrIn),
    addrBind(addrBindIn),
    fInbound(fInboundIn),
    nKeyedNetGroup(nKeyedNetGroupIn),
    addrKnown(5000, 0.001),
    filterInventoryKnown(50000, 0.000001),
    id(idIn),
    nLocalHostNonce(nLocalHostNonceIn),
    nLocalServices(nLocalServicesIn),
    nMyStartingHeight(nMyStartingHeightIn),
    nSendVersion(0)
{
    nServices = NODE_NONE;
    hSocket = hSocketIn;
    nRecvVersion = INIT_PROTO_VERSION;
    nLastSend = 0;
    nLastRecv = 0;
    nSendBytes = 0;
    nRecvBytes = 0;
    nTimeOffset = 0;
    addrName = addrNameIn == "" ? addr.ToStringIPPort() : addrNameIn;
    nVersion = 0;
    nNumWarningsSkipped = 0;
    nLastWarningTime = 0;
    strSubVer = "";
    fWhitelisted = false;
    fOneShot = false;
    m_manual_connection = false;
    fClient = false; // set by version message
    m_limited_node = false; // set by version message
    fFeeler = false;
    fSuccessfullyConnected = false;
    fDisconnect = false;
    nRefCount = 0;
    nSendSize = 0;
    nSendOffset = 0;
    hashContinue = uint256();
    nStartingHeight = -1;
    filterInventoryKnown.reset();
    fSendMempool = false;
    fGetAddr = false;
    nNextLocalAddrSend = 0;
    nNextAddrSend = 0;
    fRelayTxes = false;
    fSentAddr = false;
    timeLastMempoolReq = 0;
    nLastBlockTime = 0;
    nLastTXTime = 0;
    nPingNonceSent = 0;
    nPingUsecStart = 0;
    nPingUsecTime = 0;
    fPingQueued = false;
    m_masternode_connection = false;
    m_masternode_probe_connection = false;
    nMinPingUsecTime = std::numeric_limits<int64_t>::max();
    fPauseRecv = false;
    fPauseSend = false;
    fHasRecvData = false;
    fCanSendData = false;
    nProcessQueueSize = 0;
    nSendMsgSize = 0;

    for (const std::string &msg : getAllNetMessageTypes())
        mapRecvBytesPerMsgCmd[msg] = 0;
    mapRecvBytesPerMsgCmd[NET_MESSAGE_COMMAND_OTHER] = 0;

    if (fLogIPs) {
        LogPrint(BCLog::NET, "Added connection to %s peer=%d\n", addrName, id);
    } else {
        LogPrint(BCLog::NET, "Added connection peer=%d\n", id);
    }
}

CNode::~CNode()
{
    CloseSocket(hSocket);
}

bool CConnman::NodeFullyConnected(const CNode* pnode)
{
    return pnode && pnode->fSuccessfullyConnected && !pnode->fDisconnect;
}

void CConnman::PushMessage(CNode* pnode, CSerializedNetMsg&& msg)
{
    size_t nMessageSize = msg.data.size();
    size_t nTotalSize = nMessageSize + CMessageHeader::HEADER_SIZE;
    LogPrint(BCLog::NET, "sending %s (%d bytes) peer=%d\n",  SanitizeString(msg.command.c_str()), nMessageSize, pnode->GetId());
    statsClient.count("bandwidth.message." + SanitizeString(msg.command.c_str()) + ".bytesSent", nTotalSize, 1.0f);
    statsClient.inc("message.sent." + SanitizeString(msg.command.c_str()), 1.0f);

    std::vector<unsigned char> serializedHeader;
    serializedHeader.reserve(CMessageHeader::HEADER_SIZE);
    uint256 hash = Hash(msg.data.data(), msg.data.data() + nMessageSize);
    CMessageHeader hdr(Params().MessageStart(), msg.command.c_str(), nMessageSize);
    memcpy(hdr.pchChecksum, hash.begin(), CMessageHeader::CHECKSUM_SIZE);

    CVectorWriter{SER_NETWORK, INIT_PROTO_VERSION, serializedHeader, 0, hdr};

    size_t nBytesSent = 0;
    {
        LOCK(pnode->cs_vSend);
        bool hasPendingData = !pnode->vSendMsg.empty();

        //log total amount of bytes per command
        pnode->mapSendBytesPerMsgCmd[msg.command] += nTotalSize;
        pnode->nSendSize += nTotalSize;

        if (pnode->nSendSize > nSendBufferMaxSize)
            pnode->fPauseSend = true;
        pnode->vSendMsg.push_back(std::move(serializedHeader));
        if (nMessageSize)
            pnode->vSendMsg.push_back(std::move(msg.data));
        pnode->nSendMsgSize = pnode->vSendMsg.size();

        {
            LOCK(cs_mapNodesWithDataToSend);
            // we're not holding cs_vNodes here, so there is a chance of this node being disconnected shortly before
            // we get here. Whoever called PushMessage still has a ref to CNode*, but will later Release() it, so we
            // might end up having an entry in mapNodesWithDataToSend that is not in vNodes anymore. We need to
            // Add/Release refs when adding/erasing mapNodesWithDataToSend.
            if (mapNodesWithDataToSend.emplace(pnode->GetId(), pnode).second) {
                pnode->AddRef();
            }
        }

        // wake up select() call in case there was no pending data before (so it was not selecting this socket for sending)
        if (!hasPendingData && wakeupSelectNeeded)
            WakeSelect();
    }
    if (nBytesSent)
        RecordBytesSent(nBytesSent);
}

bool CConnman::ForNode(const CService& addr, std::function<bool(const CNode* pnode)> cond, std::function<bool(CNode* pnode)> func)
{
    CNode* found = nullptr;
    LOCK(cs_vNodes);
    for (auto&& pnode : vNodes) {
        if((CService)pnode->addr == addr) {
            found = pnode;
            break;
        }
    }
    return found != nullptr && cond(found) && func(found);
}

bool CConnman::ForNode(NodeId id, std::function<bool(const CNode* pnode)> cond, std::function<bool(CNode* pnode)> func)
{
    CNode* found = nullptr;
    LOCK(cs_vNodes);
    for (auto&& pnode : vNodes) {
        if(pnode->GetId() == id) {
            found = pnode;
            break;
        }
    }
    return found != nullptr && cond(found) && func(found);
}

bool CConnman::IsMasternodeOrDisconnectRequested(const CService& addr) {
    return ForNode(addr, AllNodes, [](CNode* pnode){
        return pnode->m_masternode_connection || pnode->fDisconnect;
    });
}

int64_t CConnman::PoissonNextSendInbound(int64_t now, int average_interval_seconds)
{
    if (m_next_send_inv_to_incoming < now) {
        // If this function were called from multiple threads simultaneously
        // it would possible that both update the next send variable, and return a different result to their caller.
        // This is not possible in practice as only the net processing thread invokes this function.
        m_next_send_inv_to_incoming = PoissonNextSend(now, average_interval_seconds);
    }
    return m_next_send_inv_to_incoming;
}

int64_t PoissonNextSend(int64_t now, int average_interval_seconds)
{
    return now + (int64_t)(log1p(GetRand(1ULL << 48) * -0.0000000000000035527136788 /* -1/2^48 */) * average_interval_seconds * -1000000.0 + 0.5);
}

std::vector<CNode*> CConnman::CopyNodeVector(std::function<bool(const CNode* pnode)> cond)
{
    std::vector<CNode*> vecNodesCopy;
    LOCK(cs_vNodes);
    vecNodesCopy.reserve(vNodes.size());
    for(size_t i = 0; i < vNodes.size(); ++i) {
        CNode* pnode = vNodes[i];
        if (!cond(pnode))
            continue;
        pnode->AddRef();
        vecNodesCopy.push_back(pnode);
    }
    return vecNodesCopy;
}

std::vector<CNode*> CConnman::CopyNodeVector()
{
    return CopyNodeVector(AllNodes);
}

void CConnman::ReleaseNodeVector(const std::vector<CNode*>& vecNodes)
{
    for(size_t i = 0; i < vecNodes.size(); ++i) {
        CNode* pnode = vecNodes[i];
        pnode->Release();
    }
}

CSipHasher CConnman::GetDeterministicRandomizer(uint64_t id) const
{
    return CSipHasher(nSeed0, nSeed1).Write(id);
}

uint64_t CConnman::CalculateKeyedNetGroup(const CAddress& ad) const
{
    std::vector<unsigned char> vchNetGroup(ad.GetGroup(addrman.m_asmap));

    return GetDeterministicRandomizer(RANDOMIZER_ID_NETGROUP).Write(vchNetGroup.data(), vchNetGroup.size()).Finalize();
}

void CConnman::RegisterEvents(CNode *pnode)
{
#ifdef USE_KQUEUE
    if (socketEventsMode != SOCKETEVENTS_KQUEUE) {
        return;
    }

    LOCK(pnode->cs_hSocket);
    assert(pnode->hSocket != INVALID_SOCKET);

    struct kevent events[2];
    EV_SET(&events[0], pnode->hSocket, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    EV_SET(&events[1], pnode->hSocket, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, nullptr);

    int r = kevent(kqueuefd, events, 2, nullptr, 0, nullptr);
    if (r != 0) {
        LogPrint(BCLog::NET, "%s -- kevent(%d, %d, %d, ...) failed. error: %s\n", __func__,
                kqueuefd, EV_ADD, pnode->hSocket, NetworkErrorString(WSAGetLastError()));
    }
#endif
#ifdef USE_EPOLL
    if (socketEventsMode != SOCKETEVENTS_EPOLL) {
        return;
    }

    LOCK(pnode->cs_hSocket);
    assert(pnode->hSocket != INVALID_SOCKET);

    epoll_event e;
    // We're using edge-triggered mode, so it's important that we drain sockets even if no signals come in
    e.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP;
    e.data.fd = pnode->hSocket;

    int r = epoll_ctl(epollfd, EPOLL_CTL_ADD, pnode->hSocket, &e);
    if (r != 0) {
        LogPrint(BCLog::NET, "%s -- epoll_ctl(%d, %d, %d, ...) failed. error: %s\n", __func__,
                epollfd, EPOLL_CTL_ADD, pnode->hSocket, NetworkErrorString(WSAGetLastError()));
    }
#endif
}

void CConnman::UnregisterEvents(CNode *pnode)
{
#ifdef USE_KQUEUE
    if (socketEventsMode != SOCKETEVENTS_KQUEUE) {
        return;
    }

    LOCK(pnode->cs_hSocket);
    if (pnode->hSocket == INVALID_SOCKET) {
        return;
    }

    struct kevent events[2];
    EV_SET(&events[0], pnode->hSocket, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    EV_SET(&events[1], pnode->hSocket, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);

    int r = kevent(kqueuefd, events, 2, nullptr, 0, nullptr);
    if (r != 0) {
        LogPrint(BCLog::NET, "%s -- kevent(%d, %d, %d, ...) failed. error: %s\n", __func__,
                kqueuefd, EV_DELETE, pnode->hSocket, NetworkErrorString(WSAGetLastError()));
    }
#endif
#ifdef USE_EPOLL
    if (socketEventsMode != SOCKETEVENTS_EPOLL) {
        return;
    }

    LOCK(pnode->cs_hSocket);
    if (pnode->hSocket == INVALID_SOCKET) {
        return;
    }

    int r = epoll_ctl(epollfd, EPOLL_CTL_DEL, pnode->hSocket, nullptr);
    if (r != 0) {
        LogPrint(BCLog::NET, "%s -- epoll_ctl(%d, %d, %d, ...) failed. error: %s\n", __func__,
                epollfd, EPOLL_CTL_DEL, pnode->hSocket, NetworkErrorString(WSAGetLastError()));
    }
#endif
}
