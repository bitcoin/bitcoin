// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <net.h>
#include <netmessagemaker.h>

#include <banman.h>
#include <clientversion.h>
#include <compat.h>
#include <consensus/consensus.h>
#include <crypto/sha256.h>
#include <i2p.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <netbase.h>
#include <protocol.h>
#include <random.h>
#include <scheduler.h>
#include <ui_interface.h>
#include <util/sock.h>
#include <util/strencodings.h>
#include <util/thread.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h> // for fDIP0001ActiveAtTip

#include <masternode/meta.h>
#include <masternode/sync.h>
#include <coinjoin/coinjoin.h>
#include <evo/deterministicmns.h>

#include <statsd_client.h>

#ifdef WIN32
#include <string.h>
#else
#include <fcntl.h>
#endif

#if HAVE_DECL_GETIFADDRS && HAVE_DECL_FREEIFADDRS
#include <ifaddrs.h>
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

#include <algorithm>
#include <cstdint>
#include <functional>
#include <unordered_map>

#include <math.h>

/** Maximum number of block-relay-only anchor connections */
static constexpr size_t MAX_BLOCK_RELAY_ONLY_ANCHORS = 2;
static_assert (MAX_BLOCK_RELAY_ONLY_ANCHORS <= static_cast<size_t>(MAX_BLOCK_RELAY_ONLY_CONNECTIONS), "MAX_BLOCK_RELAY_ONLY_ANCHORS must not exceed MAX_BLOCK_RELAY_ONLY_CONNECTIONS.");
/** Anchor IP address database file name */
const char* const ANCHORS_DATABASE_FILENAME = "anchors.dat";

// How often to dump addresses to peers.dat
static constexpr std::chrono::minutes DUMP_PEERS_INTERVAL{15};

/** Number of DNS seeds to query when the number of connections is low. */
static constexpr int DNSSEEDS_TO_QUERY_AT_ONCE = 3;

/** How long to delay before querying DNS seeds
 *
 * If we have more than THRESHOLD entries in addrman, then it's likely
 * that we got those addresses from having previously connected to the P2P
 * network, and that we'll be able to successfully reconnect to the P2P
 * network via contacting one of them. So if that's the case, spend a
 * little longer trying to connect to known peers before querying the
 * DNS seeds.
 */
static constexpr std::chrono::seconds DNSSEEDS_DELAY_FEW_PEERS{11};
static constexpr std::chrono::minutes DNSSEEDS_DELAY_MANY_PEERS{5};
static constexpr int DNSSEEDS_DELAY_PEER_THRESHOLD = 1000; // "many" vs "few" peers

/** The default timeframe for -maxuploadtarget. 1 day. */
static constexpr std::chrono::seconds MAX_UPLOAD_TIMEFRAME{60 * 60 * 24};

// A random time period (0 to 1 seconds) is added to feeler connections to prevent synchronization.
static constexpr auto FEELER_SLEEP_WINDOW{1s};

/** Used to pass flags to the Bind() function */
enum BindFlags {
    BF_NONE         = 0,
    BF_EXPLICIT     = (1U << 0),
    BF_REPORT_ERROR = (1U << 1),
    /**
     * Do not call AddLocal() for our special addresses, e.g., for incoming
     * Tor connections, to prevent gossiping them over the network.
     */
    BF_DONT_ADVERTISE = (1U << 2),
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

const std::string NET_MESSAGE_COMMAND_OTHER = "*other*";

constexpr const CConnman::CFullyConnectedOnly CConnman::FullyConnectedOnly;
constexpr const CConnman::CAllNodes CConnman::AllNodes;

static const uint64_t RANDOMIZER_ID_NETGROUP = 0x6c0edd8036ef4036ULL; // SHA256("netgroup")[0:8]
static const uint64_t RANDOMIZER_ID_LOCALHOSTNONCE = 0xd93e69e2bbfa5735ULL; // SHA256("localhostnonce")[0:8]
static const uint64_t RANDOMIZER_ID_ADDRCACHE = 0x1cf2e4ddd306dda9ULL; // SHA256("addrcache")[0:8]
//
// Global state variables
//
bool fDiscover = true;
bool fListen = true;
Mutex g_maplocalhost_mutex;
std::map<CNetAddr, LocalServiceInfo> mapLocalHost GUARDED_BY(g_maplocalhost_mutex);
static bool vfLimited[NET_MAX] GUARDED_BY(g_maplocalhost_mutex) = {};
std::string strSubVersion;

void CConnman::AddOneShot(const std::string& strDest)
{
    LOCK(cs_vOneShots);
    vOneShots.push_back(strDest);
}

uint16_t GetListenPort()
{
    return static_cast<uint16_t>(gArgs.GetArg("-port", Params().GetDefaultPort()));
}

// find 'best' local address for a particular peer
bool GetLocal(CService& addr, const CNetAddr *paddrPeer)
{
    if (!fListen)
        return false;

    int nBestScore = -1;
    int nBestReachability = -1;
    {
        LOCK(g_maplocalhost_mutex);
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

//! Convert the serialized seeds into usable address objects.
static std::vector<CAddress> ConvertSeeds(const std::vector<uint8_t> &vSeedsIn)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    std::vector<CAddress> vSeedsOut;
    FastRandomContext rng;
    CDataStream s(vSeedsIn, SER_NETWORK, PROTOCOL_VERSION | ADDRV2_FORMAT);
    while (!s.eof()) {
        CService endpoint;
        s >> endpoint;
        CAddress addr{endpoint, GetDesirableServiceFlags(NODE_NONE)};
        addr.nTime = GetTime() - rng.randrange(nOneWeek) - nOneWeek;
        LogPrint(BCLog::NET, "Added hardcoded seed: %s\n", addr.ToString());
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
    LOCK(g_maplocalhost_mutex);
    const auto it = mapLocalHost.find(addr);
    return (it != mapLocalHost.end()) ? it->second.nScore : 0;
}

// Is our peer's addrLocal potentially useful as an external IP source?
bool IsPeerAddrLocalGood(CNode *pnode)
{
    CService addrLocal = pnode->GetAddrLocal();
    return fDiscover && pnode->addr.IsRoutable() && addrLocal.IsRoutable() &&
           IsReachable(addrLocal.GetNetwork());
}

std::optional<CAddress> GetLocalAddrForPeer(CNode *pnode)
{
    CAddress addrLocal = GetLocalAddress(&pnode->addr, pnode->GetLocalServices());
    if (gArgs.GetBoolArg("-addrmantest", false)) {
        // use IPv4 loopback during addrmantest
        addrLocal = CAddress(CService(LookupNumeric("127.0.0.1", GetListenPort())), pnode->GetLocalServices());
    }
    // If discovery is enabled, sometimes give our peer the address it
    // tells us that it sees us as in case it has a better idea of our
    // address than we do.
    FastRandomContext rng;
    if (IsPeerAddrLocalGood(pnode) && (!addrLocal.IsRoutable() ||
         rng.randbits((GetnScore(addrLocal) > LOCAL_MANUAL) ? 3 : 1) == 0))
    {
        addrLocal.SetIP(pnode->GetAddrLocal());
    }
    if (addrLocal.IsRoutable() || gArgs.GetBoolArg("-addrmantest", false))
    {
        LogPrint(BCLog::NET, "Advertising address %s to peer=%d\n", addrLocal.ToString(), pnode->GetId());
        return addrLocal;
    }
    // Address is unroutable. Don't advertise.
    return std::nullopt;
}

// learn a new local address
bool AddLocal(const CService& addr, int nScore)
{
    if (!addr.IsRoutable() && Params().RequireRoutableExternalIP())
        return false;

    if (!fDiscover && nScore < LOCAL_MANUAL)
        return false;

    if (!IsReachable(addr))
        return false;

    LogPrintf("AddLocal(%s,%i)\n", addr.ToString(), nScore);

    {
        LOCK(g_maplocalhost_mutex);
        const auto [it, is_newly_added] = mapLocalHost.emplace(addr, LocalServiceInfo());
        LocalServiceInfo &info = it->second;
        if (is_newly_added || nScore >= info.nScore) {
            info.nScore = nScore + (is_newly_added ? 0 : 1);
            info.nPort = addr.GetPort();
        }
    }

    return true;
}

bool AddLocal(const CNetAddr &addr, int nScore)
{
    return AddLocal(CService(addr, GetListenPort()), nScore);
}

void RemoveLocal(const CService& addr)
{
    LOCK(g_maplocalhost_mutex);
    LogPrintf("RemoveLocal(%s)\n", addr.ToString());
    mapLocalHost.erase(addr);
}

void SetReachable(enum Network net, bool reachable)
{
    if (net == NET_UNROUTABLE || net == NET_INTERNAL)
        return;
    LOCK(g_maplocalhost_mutex);
    vfLimited[net] = !reachable;
}

bool IsReachable(enum Network net)
{
    LOCK(g_maplocalhost_mutex);
    return !vfLimited[net];
}

bool IsReachable(const CNetAddr &addr)
{
    return IsReachable(addr.GetNetwork());
}

/** vote for a local address */
bool SeenLocal(const CService& addr)
{
    LOCK(g_maplocalhost_mutex);
    const auto it = mapLocalHost.find(addr);
    if (it == mapLocalHost.end()) return false;
    ++it->second.nScore;
    return true;
}


/** check whether a given address is potentially local */
bool IsLocal(const CService& addr)
{
    LOCK(g_maplocalhost_mutex);
    return mapLocalHost.count(addr) > 0;
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
    for (const CNode* pnode : vNodes) {
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

CNode* CConnman::ConnectNode(CAddress addrConnect, const char *pszDest, bool fCountFailure, bool manual_connection, bool block_relay_only)
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
    const uint16_t default_port{pszDest != nullptr ? Params().GetDefaultPort(pszDest) :
                                                     Params().GetDefaultPort()};
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
    std::unique_ptr<Sock> sock;
    proxyType proxy;
    CAddress addr_bind;
    assert(!addr_bind.IsValid());

    if (addrConnect.IsValid()) {
        bool proxyConnectionFailed = false;

        if (addrConnect.GetNetwork() == NET_I2P && m_i2p_sam_session.get() != nullptr) {
            i2p::Connection conn;
            if (m_i2p_sam_session->Connect(addrConnect, conn, proxyConnectionFailed)) {
                connected = true;
                sock = std::move(conn.sock);
                addr_bind = CAddress{conn.me, NODE_NONE};
            }
        } else if (GetProxy(addrConnect.GetNetwork(), proxy)) {
            sock = CreateSock(proxy.proxy);
            if (!sock) {
                return nullptr;
            }
            connected = ConnectThroughProxy(proxy, addrConnect.ToStringIP(), addrConnect.GetPort(),
                                            *sock, nConnectTimeout, proxyConnectionFailed);
        } else {
            // no proxy needed (none set for target network)
            sock = CreateSock(addrConnect);
            if (!sock) {
                return nullptr;
            }
            connected = ConnectSocketDirectly(addrConnect, *sock, nConnectTimeout, manual_connection);
        }
        if (!proxyConnectionFailed) {
            // If a connection to the node was attempted, and failure (if any) is not caused by a problem connecting to
            // the proxy, mark this as an attempt.
            addrman.Attempt(addrConnect, fCountFailure);
        }
    } else if (pszDest && GetNameProxy(proxy)) {
        sock = CreateSock(proxy.proxy);
        if (!sock) {
            return nullptr;
        }
        std::string host;
        uint16_t port{default_port};
        SplitHostPort(std::string(pszDest), port, host);
        bool proxyConnectionFailed;
        connected = ConnectThroughProxy(proxy, host, port, *sock, nConnectTimeout,
                                        proxyConnectionFailed);
    }
    if (!connected) {
        return nullptr;
    }

    // Add node
    NodeId id = GetNewNodeId();
    uint64_t nonce = GetDeterministicRandomizer(RANDOMIZER_ID_LOCALHOSTNONCE).Write(id).Finalize();
    if (!addr_bind.IsValid()) {
        addr_bind = GetBindAddress(sock->Get());
    }
    CNode* pnode = new CNode(id, nLocalServices, sock->Release(), addrConnect, CalculateKeyedNetGroup(addrConnect), nonce, addr_bind, pszDest ? pszDest : "", false, block_relay_only);
    pnode->AddRef();
    statsClient.inc("peers.connect", 1.0f);

    // We're making a new connection, harvest entropy from the time (and our peer count)
    RandAddEvent((uint32_t)id);

    return pnode;
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

void CConnman::AddWhitelistPermissionFlags(NetPermissionFlags& flags, const CNetAddr &addr) const {
    for (const auto& subnet : vWhitelistedRange) {
        if (subnet.m_subnet.Match(addr)) NetPermissions::AddFlag(flags, subnet.m_flags);
    }
}

bool CNode::IsBlockRelayOnly() const {
    bool ignores_incoming_txs{gArgs.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY)};
    // Stop processing non-block data early if
    // 1) We are in blocks only mode and peer has no relay permission
    // 2) This peer is a block-relay-only peer
    return (ignores_incoming_txs && !HasPermission(PF_RELAY)) || !IsAddrRelayPeer();
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

Network CNode::ConnectedThroughNetwork() const
{
    return fInbound && m_inbound_onion ? NET_ONION : addr.GetNetClass();
}

#undef X
#define X(name) stats.name = name
void CNode::copyStats(CNodeStats &stats, const std::vector<bool> &m_asmap)
{
    stats.nodeid = this->GetId();
    X(nServices);
    X(addr);
    X(addrBind);
    stats.m_network = ConnectedThroughNetwork();
    stats.m_mapped_as = addr.GetMappedAS(m_asmap);
    if (IsAddrRelayPeer()) {
        LOCK(m_tx_relay->cs_filter);
        stats.fRelayTxes = m_tx_relay->fRelayTxes;
    } else {
        stats.fRelayTxes = false;
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
    X(m_legacyWhitelisted);
    X(m_permissionFlags);

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
    stats.m_ping_usec = nPingUsecTime;
    stats.m_min_ping_usec  = nMinPingUsecTime;
    stats.m_ping_wait_usec = nPingUsecWait;

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

bool CNode::ReceiveMsgBytes(Span<const uint8_t> msg_bytes, bool& complete)
{
    complete = false;
    int64_t nTimeMicros = GetTimeMicros();
    LOCK(cs_vRecv);
    nLastRecv = nTimeMicros / 1000000;
    nRecvBytes += msg_bytes.size();
    while (msg_bytes.size() > 0) {
        // absorb network data
        int handled = m_deserializer->Read(msg_bytes);
        if (handled < 0) {
            // Serious header problem, disconnect from the peer.
            return false;
        }

        if (m_deserializer->Complete()) {
            // decompose a transport agnostic CNetMessage from the deserializer
            uint32_t out_err_raw_size{0};
            std::optional<CNetMessage> result{m_deserializer->GetMessage(nTimeMicros, out_err_raw_size)};
            if (!result) {
                // Message deserialization failed.  Drop the message but don't disconnect the peer.
                // store the size of the corrupt message
                mapRecvBytesPerMsgCmd.find(NET_MESSAGE_COMMAND_OTHER)->second += out_err_raw_size;
                continue;
            }

            //store received bytes per message command
            //to prevent a memory DOS, only allow valid commands
            mapMsgCmdSize::iterator i = mapRecvBytesPerMsgCmd.find(result->m_command);
            if (i == mapRecvBytesPerMsgCmd.end())
                i = mapRecvBytesPerMsgCmd.find(NET_MESSAGE_COMMAND_OTHER);
            assert(i != mapRecvBytesPerMsgCmd.end());
            i->second += result->m_raw_message_size;
            statsClient.count("bandwidth.message." + std::string(result->m_command) + ".bytesReceived", result->m_raw_message_size, 1.0f);

            // push the message to the process queue,
            vRecvMsg.push_back(std::move(*result));

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

int V1TransportDeserializer::readHeader(Span<const uint8_t> msg_bytes)
{
    // copy data to temporary parsing buffer
    unsigned int nRemaining = CMessageHeader::HEADER_SIZE - nHdrPos;
    unsigned int nCopy = std::min<unsigned int>(nRemaining, msg_bytes.size());

    memcpy(&hdrbuf[nHdrPos], msg_bytes.data(), nCopy);
    nHdrPos += nCopy;

    // if header incomplete, exit
    if (nHdrPos < CMessageHeader::HEADER_SIZE)
        return nCopy;

    // deserialize to CMessageHeader
    try {
        hdrbuf >> hdr;
    }
    catch (const std::exception&) {
        LogPrint(BCLog::NET, "HEADER ERROR - UNABLE TO DESERIALIZE, peer=%d\n", m_node_id);
        return -1;
    }

    // Check start string, network magic
    if (memcmp(hdr.pchMessageStart, m_chain_params.MessageStart(), CMessageHeader::MESSAGE_START_SIZE) != 0) {
        LogPrint(BCLog::NET, "HEADER ERROR - MESSAGESTART (%s, %u bytes), received %s, peer=%d\n", hdr.GetCommand(), hdr.nMessageSize, HexStr(hdr.pchMessageStart), m_node_id);
        return -1;
    }

    // reject messages larger than MAX_SIZE or MAX_PROTOCOL_MESSAGE_LENGTH
    if (hdr.nMessageSize > MAX_SIZE || hdr.nMessageSize > MAX_PROTOCOL_MESSAGE_LENGTH) {
        LogPrint(BCLog::NET, "HEADER ERROR - SIZE (%s, %u bytes), peer=%d\n", hdr.GetCommand(), hdr.nMessageSize, m_node_id);
        return -1;
    }

    // switch state to reading message data
    in_data = true;

    return nCopy;
}

int V1TransportDeserializer::readData(Span<const uint8_t> msg_bytes)
{
    unsigned int nRemaining = hdr.nMessageSize - nDataPos;
    unsigned int nCopy = std::min<unsigned int>(nRemaining, msg_bytes.size());

    if (vRecv.size() < nDataPos + nCopy) {
        // Allocate up to 256 KiB ahead, but never more than the total message size.
        vRecv.resize(std::min(hdr.nMessageSize, nDataPos + nCopy + 256 * 1024));
    }

    hasher.Write(msg_bytes.first(nCopy));
    memcpy(&vRecv[nDataPos], msg_bytes.data(), nCopy);
    nDataPos += nCopy;

    return nCopy;
}

const uint256& V1TransportDeserializer::GetMessageHash() const
{
    assert(Complete());
    if (data_hash.IsNull())
        hasher.Finalize(data_hash);
    return data_hash;
}

std::optional<CNetMessage> V1TransportDeserializer::GetMessage(int64_t time, uint32_t& out_err_raw_size)
{
    // decompose a single CNetMessage from the TransportDeserializer
    std::optional<CNetMessage> msg(std::move(vRecv));

    // store command string, time, and sizes
    msg->m_command = hdr.GetCommand();
    msg->m_time = time;
    msg->m_message_size = hdr.nMessageSize;
    msg->m_raw_message_size = hdr.nMessageSize + CMessageHeader::HEADER_SIZE;

    uint256 hash = GetMessageHash();

    // We just received a message off the wire, harvest entropy from the time (and the message checksum)
    RandAddEvent(ReadLE32(hash.begin()));

    // Check checksum and header command string
    if (memcmp(hash.begin(), hdr.pchChecksum, CMessageHeader::CHECKSUM_SIZE) != 0) {
        LogPrint(BCLog::NET, "CHECKSUM ERROR (%s, %u bytes), expected %s was %s, peer=%d\n",
                 SanitizeString(msg->m_command), msg->m_message_size,
                 HexStr(Span{hash}.first(CMessageHeader::CHECKSUM_SIZE)),
                 HexStr(hdr.pchChecksum),
                 m_node_id);
        out_err_raw_size = msg->m_raw_message_size;
        msg.reset();
    } else if (!hdr.IsCommandValid()) {
        LogPrint(BCLog::NET, "HEADER ERROR - COMMAND (%s, %u bytes), peer=%d\n",
                 hdr.GetCommand(), msg->m_message_size, m_node_id);
        out_err_raw_size = msg->m_raw_message_size;
        msg.reset();
    }

    // Always reset the network deserializer (prepare for the next message)
    Reset();
    return msg;
}

void V1TransportSerializer::prepareForTransport(CSerializedNetMsg& msg, std::vector<unsigned char>& header) {
    // create dbl-sha256 checksum
    uint256 hash = Hash(msg.data);

    // create header
    CMessageHeader hdr(Params().MessageStart(), msg.command.c_str(), msg.data.size());
    memcpy(hdr.pchChecksum, hash.begin(), CMessageHeader::CHECKSUM_SIZE);

    // serialize header
    header.reserve(CMessageHeader::HEADER_SIZE);
    CVectorWriter{SER_NETWORK, INIT_PROTO_VERSION, header, 0, hdr};
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
    bool prefer_evict;
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
            if (node->HasPermission(PF_NOBAN))
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
                if (!node->GetVerifiedProRegTxHash().IsNull()) {
                    isProtected = true;
                }
                if (isProtected) {
                    continue;
                }
            }

            bool peer_relay_txes = false;
            bool peer_filter_not_null = false;
            if (node->IsAddrRelayPeer()) {
                LOCK(node->m_tx_relay->cs_filter);
                peer_relay_txes = node->m_tx_relay->fRelayTxes;
                peer_filter_not_null = node->m_tx_relay->pfilter != nullptr;
            }
            NodeEvictionCandidate candidate = {node->GetId(), node->nTimeConnected, node->nMinPingUsecTime,
                                               node->nLastBlockTime, node->nLastTXTime,
                                               HasAllDesirableServiceFlags(node->nServices),
                                               peer_relay_txes, peer_filter_not_null, node->nKeyedNetGroup,
                                               node->m_prefer_evict};
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
    // Protect 4 nodes that most recently sent us novel transactions accepted into our mempool.
    // An attacker cannot manipulate this metric without performing useful work.
    EraseLastKElements(vEvictionCandidates, CompareNodeTXTime, 4);
    // Protect 4 nodes that most recently sent us novel blocks.
    // An attacker cannot manipulate this metric without performing useful work.
    EraseLastKElements(vEvictionCandidates, CompareNodeBlockTime, 4);
    // Protect the half of the remaining nodes which have been connected the longest.
    // This replicates the non-eviction implicit behavior, and precludes attacks that start later.
    EraseLastKElements(vEvictionCandidates, ReverseCompareNodeTimeConnected, vEvictionCandidates.size() / 2);

    if (vEvictionCandidates.empty()) return false;

    // If any remaining peers are preferred for eviction consider only them.
    // This happens after the other preferences since if a peer is really the best by other criteria (esp relaying blocks)
    //  then we probably don't want to evict it no matter what.
    if (std::any_of(vEvictionCandidates.begin(),vEvictionCandidates.end(),[](NodeEvictionCandidate const &n){return n.prefer_evict;})) {
        vEvictionCandidates.erase(std::remove_if(vEvictionCandidates.begin(),vEvictionCandidates.end(),
                                  [](NodeEvictionCandidate const &n){return !n.prefer_evict;}),vEvictionCandidates.end());
    }

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
    if (hSocket == INVALID_SOCKET) {
        const int nErr = WSAGetLastError();
        if (nErr != WSAEWOULDBLOCK) {
            LogPrintf("socket error accept failed: %s\n", NetworkErrorString(nErr));
        }
        return;
    }

    if (!addr.SetSockAddr((const struct sockaddr*)&sockaddr)) {
        LogPrintf("Warning: Unknown socket family\n");
    }

    const CAddress addr_bind = GetBindAddress(hSocket);

    NetPermissionFlags permissionFlags = NetPermissionFlags::PF_NONE;
    hListenSocket.AddSocketPermissionFlags(permissionFlags);

    CreateNodeFromAcceptedSocket(hSocket, permissionFlags, addr_bind, addr);
}

void CConnman::CreateNodeFromAcceptedSocket(SOCKET hSocket,
                                            NetPermissionFlags permissionFlags,
                                            const CAddress& addr_bind,
                                            const CAddress& addr)
{
    int nInbound = 0;
    int nVerifiedInboundMasternodes = 0;
    int nMaxInbound = nMaxConnections - m_max_outbound;

    AddWhitelistPermissionFlags(permissionFlags, addr);
    bool legacyWhitelisted = false;
    if (NetPermissions::HasFlag(permissionFlags, NetPermissionFlags::PF_ISIMPLICIT)) {
        NetPermissions::ClearFlag(permissionFlags, PF_ISIMPLICIT);
        if (gArgs.GetBoolArg("-whitelistforcerelay", DEFAULT_WHITELISTFORCERELAY)) NetPermissions::AddFlag(permissionFlags, PF_FORCERELAY);
        if (gArgs.GetBoolArg("-whitelistrelay", DEFAULT_WHITELISTRELAY)) NetPermissions::AddFlag(permissionFlags, PF_RELAY);
        NetPermissions::AddFlag(permissionFlags, PF_MEMPOOL);
        NetPermissions::AddFlag(permissionFlags, PF_NOBAN);
        legacyWhitelisted = true;
    }

    {
        LOCK(cs_vNodes);
        for (const CNode* pnode : vNodes) {
            if (pnode->fInbound) {
                nInbound++;
                if (!pnode->GetVerifiedProRegTxHash().IsNull()) {
                    nVerifiedInboundMasternodes++;
                }
            }
        }

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

    // Don't accept connections from banned peers.
    bool banned = m_banman->IsBanned(addr);
    if (!NetPermissions::HasFlag(permissionFlags, NetPermissionFlags::PF_NOBAN) && banned)
    {
        LogPrint(BCLog::NET, "%s (banned)\n", strDropped);
        CloseSocket(hSocket);
        return;
    }

    // Only accept connections from discouraged peers if our inbound slots aren't (almost) full.
    bool discouraged = m_banman->IsDiscouraged(addr);
    if (!NetPermissions::HasFlag(permissionFlags, NetPermissionFlags::PF_NOBAN) && nInbound + 1 >= nMaxInbound && discouraged)
    {
        LogPrint(BCLog::NET, "connection from %s dropped (discouraged)\n", addr.ToString());
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

    // don't accept incoming connections until blockchain is synced
    if(fMasternodeMode && !::masternodeSync->IsBlockchainSynced()) {
        LogPrint(BCLog::NET, "AcceptConnection -- blockchain is not synced yet, skipping inbound connection attempt\n");
        CloseSocket(hSocket);
        return;
    }

    NodeId id = GetNewNodeId();
    uint64_t nonce = GetDeterministicRandomizer(RANDOMIZER_ID_LOCALHOSTNONCE).Write(id).Finalize();

    ServiceFlags nodeServices = nLocalServices;
    if (NetPermissions::HasFlag(permissionFlags, PF_BLOOMFILTER)) {
        nodeServices = static_cast<ServiceFlags>(nodeServices | NODE_BLOOM);
    }

    const bool inbound_onion = std::find(m_onion_binds.begin(), m_onion_binds.end(), addr_bind) != m_onion_binds.end();
    CNode* pnode = new CNode(id, nodeServices, hSocket, addr, CalculateKeyedNetGroup(addr), nonce, addr_bind, "", true, inbound_onion);
    pnode->AddRef();
    pnode->m_permissionFlags = permissionFlags;
    // If this flag is present, the user probably expect that RPC and QT report it as whitelisted (backward compatibility)
    pnode->m_legacyWhitelisted = legacyWhitelisted;
    pnode->m_prefer_evict = discouraged;
    m_msgproc->InitializeNode(pnode);

    if (fLogIPs) {
        LogPrint(BCLog::NET_NETCONN, "connection from %s accepted, sock=%d, peer=%d\n", addr.ToString(), hSocket, pnode->GetId());
    } else {
        LogPrint(BCLog::NET_NETCONN, "connection accepted, sock=%d, peer=%d\n", hSocket, pnode->GetId());
    }

    {
        LOCK(cs_vNodes);
        vNodes.push_back(pnode);
        mapSocketToNode.emplace(hSocket, pnode);
        RegisterEvents(pnode);
        WakeSelect();
    }

    // We received a new connection, harvest entropy from the time (and our peer count)
    RandAddEvent((uint32_t)id);
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
        ::masternodeSync->Reset();
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
    auto vNodesCopy = CopyNodeVector(CConnman::FullyConnectedOnly);
    for (auto pnode : vNodesCopy) {
        {
            LOCK(pnode->cs_vRecv);
            for (const mapMsgCmdSize::value_type &i : pnode->mapRecvBytesPerMsgCmd)
                mapRecvBytesMsgStats[i.first] += i.second;
        }
        {
            LOCK(pnode->cs_vSend);
            for (const mapMsgCmdSize::value_type &i : pnode->mapSendBytesPerMsgCmd)
                mapSentBytesMsgStats[i.first] += i.second;
        }
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
    ReleaseNodeVector(vNodesCopy);
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

void CConnman::InactivityCheck(CNode *pnode) const
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
        if (recv_set.count(hListenSocket.socket) > 0)
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

        // Send data
        size_t bytes_sent = WITH_LOCK(pnode->cs_vSend, return SocketSendData(pnode));
        if (bytes_sent) RecordBytesSent(bytes_sent);
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
    uint8_t pchBuf[0x10000];
    int nBytes = 0;
    {
        LOCK(pnode->cs_hSocket);
        if (pnode->hSocket == INVALID_SOCKET)
            return 0;
        nBytes = recv(pnode->hSocket, (char*)pchBuf, sizeof(pchBuf), MSG_DONTWAIT);
        if (nBytes < (int)sizeof(pchBuf)) {
            pnode->fHasRecvData = false;
        }
    }
    if (nBytes > 0)
    {
        bool notify = false;
        if (!pnode->ReceiveMsgBytes(Span<const uint8_t>(pchBuf, nBytes), notify)) {
            LOCK(cs_vNodes);
            pnode->CloseSocketDisconnect(this);
        }
        RecordBytesRecv(nBytes);
        if (notify) {
            size_t nSizeAdded = 0;
            auto it(pnode->vRecvMsg.begin());
            for (; it != pnode->vRecvMsg.end(); ++it) {
                // vRecvMsg contains only completed CNetMessage
                // the single possible partially deserialized message are held by TransportDeserializer
                nSizeAdded += it->m_raw_message_size;
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
            LogPrint(BCLog::NET, "socket closed for peer=%d\n", pnode->GetId());
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
            if (!pnode->fDisconnect){
                LogPrint(BCLog::NET, "socket recv error for peer=%d: %s\n", pnode->GetId(), NetworkErrorString(nErr));
            }
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
    } else if (addrman.size() == 0) {
        // If we have no known peers, query all.
        // This will occur on the first run, or if peers.dat has been
        // deleted.
        seeds_right_now = seeds.size();
    }

    // goal: only query DNS seed if address need is acute
    // * If we have a reasonable number of peers in addrman, spend
    //   some time trying them first. This improves user privacy by
    //   creating fewer identifying DNS requests, reduces trust by
    //   giving seeds less influence on the network topology, and
    //   reduces traffic to the seeds.
    // * When querying DNS seeds query a few at once, this ensures
    //   that we don't give DNS seeds the ability to eclipse nodes
    //   that query them.
    // * If we continue having problems, eventually query all the
    //   DNS seeds, and if that fails too, also try the fixed seeds.
    //   (done in ThreadOpenConnections)
    const std::chrono::seconds seeds_wait_time = (addrman.size() >= DNSSEEDS_DELAY_PEER_THRESHOLD ? DNSSEEDS_DELAY_MANY_PEERS : DNSSEEDS_DELAY_FEW_PEERS);

    for (const std::string& seed : seeds) {
        if (seeds_right_now == 0) {
            seeds_right_now += DNSSEEDS_TO_QUERY_AT_ONCE;

            if (addrman.size() > 0) {
                LogPrintf("Waiting %d seconds before querying DNS seeds.\n", seeds_wait_time.count());
                std::chrono::seconds to_wait = seeds_wait_time;
                while (to_wait.count() > 0) {
                    // if sleeping for the MANY_PEERS interval, wake up
                    // early to see if we have enough peers and can stop
                    // this thread entirely freeing up its resources
                    std::chrono::seconds w = std::min(DNSSEEDS_DELAY_FEW_PEERS, to_wait);
                    if (!interruptNet.sleep_for(w)) return;
                    to_wait -= w;

                    int nRelevant = 0;
                    {
                        LOCK(cs_vNodes);
                        for (const CNode* pnode : vNodes) {
                            nRelevant += pnode->fSuccessfullyConnected && !pnode->fFeeler && !pnode->fOneShot && !pnode->m_manual_connection && !pnode->fInbound && !pnode->m_masternode_probe_connection;
                        }
                    }
                    if (nRelevant >= 2) {
                        if (found > 0) {
                            LogPrintf("%d addresses found from DNS seeds\n", found);
                            LogPrintf("P2P peers available. Finished DNS seeding.\n");
                        } else {
                            LogPrintf("P2P peers available. Skipped DNS seeding.\n");
                        }
                        return;
                    }
                }
            }
        }

        if (interruptNet) return;

        // hold off on querying seeds if P2P network deactivated
        if (!fNetworkActive) {
            LogPrintf("Waiting for network to be reactivated before querying DNS seeds.\n");
            do {
                if (!interruptNet.sleep_for(std::chrono::seconds{1})) return;
            } while (!fNetworkActive);
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
            if (LookupHost(host, vIPs, nMaxIPs, true)) {
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
        for (const CNode* pnode : vNodes) {
            // don't count outbound masternodes
            if (pnode->m_masternode_connection) {
                continue;
            }
            if (!pnode->fInbound && !pnode->m_manual_connection && !pnode->fFeeler && !pnode->fDisconnect && !pnode->fOneShot && pnode->fSuccessfullyConnected && !pnode->m_masternode_probe_connection) {
                ++nOutbound;
            }
        }
    }
    return std::max(nOutbound - m_max_outbound_full_relay - m_max_outbound_block_relay, 0);
}

void CConnman::ThreadOpenConnections(const std::vector<std::string> connect)
{
    FastRandomContext rng;
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
        // Note that we only do this if we started with an empty peers.dat,
        // (in which case we will query DNS seeds immediately) *and* the DNS
        // seeds have not returned any results.
        if (addrman.size() == 0 && (GetTime() - nStart > 60)) {
            static bool done = false;
            if (!done) {
                LogPrintf("Adding fixed seed nodes as DNS doesn't seem to be available.\n");
                CNetAddr local;
                local.SetInternal("fixedseeds");
                addrman.Add(ConvertSeeds(Params().FixedSeeds()), local);
                done = true;
            }
        }

        //
        // Choose an address to connect to based on most recently seen
        //
        CAddress addrConnect;

        // Only connect out to one peer per network group (/16 for IPv4).
        // This is only done for mainnet and testnet
        int nOutboundFullRelay = 0;
        int nOutboundBlockRelay = 0;
        std::set<std::vector<unsigned char> > setConnected;
        if (!Params().AllowMultipleAddressesFromGroup()) {
            LOCK(cs_vNodes);
            for (const CNode* pnode : vNodes) {
                if (!pnode->fInbound && !pnode->m_masternode_connection && !pnode->m_manual_connection) {
                    // Netgroups for inbound and addnode peers are not excluded because our goal here
                    // is to not use multiple of our limited outbound slots on a single netgroup
                    // but inbound and addnode peers do not use our outbound slots.  Inbound peers
                    // also have the added issue that they're attacker controlled and could be used
                    // to prevent us from connecting to particular hosts if we used them here.
                    setConnected.insert(pnode->addr.GetGroup(addrman.m_asmap));
                    if (!pnode->IsAddrRelayPeer()) {
                        nOutboundBlockRelay++;
                    } else if (!pnode->fFeeler) {
                        nOutboundFullRelay++;
                    }
                }
            }
        }

        std::set<uint256> setConnectedMasternodes;
        {
            LOCK(cs_vNodes);
            for (CNode* pnode : vNodes) {
                auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
                if (!verifiedProRegTxHash.IsNull()) {
                    setConnectedMasternodes.emplace(verifiedProRegTxHash);
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

        if (nOutboundFullRelay >= m_max_outbound_full_relay && nOutboundBlockRelay >= m_max_outbound_block_relay && !GetTryNewOutboundPeer()) {
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
            // If we didn't find an appropriate destination after trying 100 addresses fetched from addrman,
            // stop this loop, and let the outer loop run again (which sleeps, adds seed nodes, recalculates
            // already-connected network ranges, ...) before trying new addrman addresses.
            nTries++;
            if (nTries > 100)
                break;

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

            if (!IsReachable(addr))
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

            // Do not allow non-default ports, unless after 50 invalid
            // addresses selected already. This is to prevent malicious peers
            // from advertising themselves as a service on another host and
            // port, causing a DoS attack as nodes around the network attempt
            // to connect to it fruitlessly.
            if ((!isMasternode || !Params().AllowMultiplePorts()) && addr.GetPort() != Params().GetDefaultPort(addr.GetNetwork()) && addr.GetPort() != GetListenPort() && nTries < 50) {
                continue;
            }

            addrConnect = addr;
            break;
        }

        if (addrConnect.IsValid()) {
            if (fFeeler) {
                // Add small amount of random noise before connection to avoid synchronization.
                if (!interruptNet.sleep_for(rng.rand_uniform_duration<CThreadInterrupt::Clock>(FEELER_SLEEP_WINDOW))) {
                    return;
                }
                if (fLogIPs) {
                    LogPrint(BCLog::NET, "Making feeler connection to %s\n", addrConnect.ToString());
                } else {
                    LogPrint(BCLog::NET, "Making feeler connection\n");
                }
            }

            // Open this connection as block-relay-only if we're already at our
            // full-relay capacity, but not yet at our block-relay peer limit.
            // (It should not be possible for fFeeler to be set if we're not
            // also at our block-relay peer limit, but check against that as
            // well for sanity.)
            bool block_relay_only = nOutboundBlockRelay < m_max_outbound_block_relay && !fFeeler && nOutboundFullRelay >= m_max_outbound_full_relay;

            OpenNetworkConnection(addrConnect, (int)setConnected.size() >= std::min(nMaxConnections - 1, 2), &grant, nullptr, false, fFeeler, false, block_relay_only);
        }
    }
}

std::vector<CAddress> CConnman::GetCurrentBlockRelayOnlyConns() const
{
    std::vector<CAddress> ret;
    LOCK(cs_vNodes);
    for (const CNode* pnode : vNodes) {
        if (pnode->IsBlockRelayOnly()) {
            ret.push_back(pnode->addr);
        }
    }

    return ret;
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
        CService service(LookupNumeric(strAddNode, Params().GetDefaultPort(strAddNode)));
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

    assert(::mmetaman != nullptr);

    auto& chainParams = Params();

    bool didConnect = false;
    while (!interruptNet)
    {
        auto sleepTime = std::chrono::milliseconds(1000);
        if (didConnect) {
            sleepTime = std::chrono::milliseconds(100);
        }
        if (!interruptNet.sleep_for(sleepTime))
            return;

        didConnect = false;

        if (!fNetworkActive || !::masternodeSync->IsBlockchainSynced())
            continue;

        std::set<CService> connectedNodes;
        std::map<uint256 /*proTxHash*/, bool /*fInbound*/> connectedProRegTxHashes;
        ForEachNode([&](const CNode* pnode) {
            auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
            connectedNodes.emplace(pnode->addr);
            if (!verifiedProRegTxHash.IsNull()) {
                connectedProRegTxHashes.emplace(verifiedProRegTxHash, pnode->fInbound);
            }
        });

        auto mnList = deterministicMNManager->GetListAtChainTip();

        if (interruptNet)
            return;

        int64_t nANow = GetTime<std::chrono::seconds>().count();
        constexpr const auto &_func_ = __func__;

        // NOTE: Process only one pending masternode at a time

        MasternodeProbeConn isProbe = MasternodeProbeConn::IsNotConnection;

        const auto getPendingQuorumNodes = [&]() {
            LockAssertion lock(cs_vPendingMasternodes);
            std::vector<CDeterministicMNCPtr> ret;
            for (const auto& group : masternodeQuorumNodes) {
                for (const auto& proRegTxHash : group.second) {
                    auto dmn = mnList.GetMN(proRegTxHash);
                    if (!dmn) {
                        continue;
                    }
                    const auto& addr2 = dmn->pdmnState->addr;
                    if (connectedNodes.count(addr2) && !connectedProRegTxHashes.count(proRegTxHash)) {
                        // we probably connected to it before it became a masternode
                        // or maybe we are still waiting for mnauth
                        (void)ForNode(addr2, [&](CNode* pnode) {
                            if (pnode->nTimeFirstMessageReceived != 0 && GetSystemTimeInSeconds() - pnode->nTimeFirstMessageReceived > 5) {
                                // clearly not expecting mnauth to take that long even if it wasn't the first message
                                // we received (as it should normally), disconnect
                                LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- dropping non-mnauth connection to %s, service=%s\n", _func_, proRegTxHash.ToString(), addr2.ToString(false));
                                pnode->fDisconnect = true;
                                return true;
                            }
                            return false;
                        });
                        // either way - it's not ready, skip it for now
                        continue;
                    }
                    if (!connectedNodes.count(addr2) && !IsMasternodeOrDisconnectRequested(addr2) && !connectedProRegTxHashes.count(proRegTxHash)) {
                        int64_t lastAttempt = mmetaman->GetMetaInfo(dmn->proTxHash)->GetLastOutboundAttempt();
                        // back off trying connecting to an address if we already tried recently
                        if (nANow - lastAttempt < chainParams.LLMQConnectionRetryTimeout()) {
                            continue;
                        }
                        ret.emplace_back(dmn);
                    }
                }
            }
            return ret;
        };

        const auto getPendingProbes = [&]() {
            LockAssertion lock(cs_vPendingMasternodes);
            std::vector<CDeterministicMNCPtr> ret;
            for (auto it = masternodePendingProbes.begin(); it != masternodePendingProbes.end(); ) {
                auto dmn = mnList.GetMN(*it);
                if (!dmn) {
                    it = masternodePendingProbes.erase(it);
                    continue;
                }
                bool connectedAndOutbound = connectedProRegTxHashes.count(dmn->proTxHash) && !connectedProRegTxHashes[dmn->proTxHash];
                if (connectedAndOutbound) {
                    // we already have an outbound connection to this MN so there is no theed to probe it again
                    mmetaman->GetMetaInfo(dmn->proTxHash)->SetLastOutboundSuccess(nANow);
                    it = masternodePendingProbes.erase(it);
                    continue;
                }

                ++it;

                int64_t lastAttempt = mmetaman->GetMetaInfo(dmn->proTxHash)->GetLastOutboundAttempt();
                // back off trying connecting to an address if we already tried recently
                if (nANow - lastAttempt < chainParams.LLMQConnectionRetryTimeout()) {
                    continue;
                }
                ret.emplace_back(dmn);
            }
            return ret;
        };

        auto getConnectToDmn = [&]() -> CDeterministicMNCPtr {
            // don't hold lock while calling OpenMasternodeConnection as cs_main is locked deep inside
            LOCK2(cs_vNodes, cs_vPendingMasternodes);

            if (!vPendingMasternodes.empty()) {
                auto dmn = mnList.GetValidMN(vPendingMasternodes.front());
                vPendingMasternodes.erase(vPendingMasternodes.begin());
                if (dmn && !connectedNodes.count(dmn->pdmnState->addr) && !IsMasternodeOrDisconnectRequested(dmn->pdmnState->addr)) {
                    LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- opening pending masternode connection to %s, service=%s\n", _func_, dmn->proTxHash.ToString(), dmn->pdmnState->addr.ToString(false));
                    return dmn;
                }
            }

            if (const auto pending = getPendingQuorumNodes(); !pending.empty()) {
                // not-null
                auto dmn = pending[GetRand(pending.size())];
                LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- opening quorum connection to %s, service=%s\n",
                         _func_, dmn->proTxHash.ToString(), dmn->pdmnState->addr.ToString(false));
                return dmn;
            }

            if (const auto pending = getPendingProbes(); !pending.empty()) {
                // not-null
                auto dmn = pending[GetRand(pending.size())];
                masternodePendingProbes.erase(dmn->proTxHash);
                isProbe = MasternodeProbeConn::IsConnection;

                LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- probing masternode %s, service=%s\n", _func_, dmn->proTxHash.ToString(), dmn->pdmnState->addr.ToString(false));
                return dmn;
            }
            return nullptr;
        };

        CDeterministicMNCPtr connectToDmn = getConnectToDmn();

        if (connectToDmn == nullptr) {
            continue;
        }

        didConnect = true;

        mmetaman->GetMetaInfo(connectToDmn->proTxHash)->SetLastOutboundAttempt(nANow);

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
            // Will take a few consequent failed attempts to PoSe-punish a MN.
            if (mmetaman->GetMetaInfo(connectToDmn->proTxHash)->OutboundFailedTooManyTimes()) {
                LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- failed to connect to masternode %s too many times\n", __func__, connectToDmn->proTxHash.ToString());
            }
        }
    }
}

// if successful, this moves the passed grant to the constructed node
void CConnman::OpenNetworkConnection(const CAddress& addrConnect, bool fCountFailure, CSemaphoreGrant *grantOutbound, const char *pszDest, bool fOneShot, bool fFeeler, bool manual_connection, bool block_relay_only, MasternodeConn masternode_connection, MasternodeProbeConn masternode_probe_connection)
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

    auto getIpStr = [&]() {
        if (fLogIPs) {
            return addrConnect.ToString(false);
        } else {
            return std::string("new peer");
        }
    };

    if (!pszDest) {
        // banned, discouraged or exact match?
        if ((m_banman && (m_banman->IsDiscouraged(addrConnect) || m_banman->IsBanned(addrConnect))) || FindNode(addrConnect.ToStringIPPort()))
            return;
        // local and not a connection to itself?
        bool fAllowLocal = Params().AllowMultiplePorts() && addrConnect.GetPort() != GetListenPort();
        if (!fAllowLocal && IsLocal(addrConnect))
            return;
        // Search for IP:PORT match:
        //  - if multiple ports for the same IP are allowed,
        //  - for probe connections
        // Search for IP-only match otherwise
        bool searchIPPort = Params().AllowMultiplePorts() || masternode_probe_connection == MasternodeProbeConn::IsConnection;
        bool skip = searchIPPort ?
                FindNode(static_cast<CService>(addrConnect)) :
                FindNode(static_cast<CNetAddr>(addrConnect));
        if (skip) {
            LogPrintf("CConnman::%s -- Failed to open new connection to %s, already connected\n", __func__, getIpStr());
            return;
        }
    } else if (FindNode(std::string(pszDest)))
        return;

    LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- connecting to %s\n", __func__, getIpStr());
    CNode* pnode = ConnectNode(addrConnect, pszDest, fCountFailure, manual_connection, block_relay_only);

    if (!pnode) {
        LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- ConnectNode failed for %s\n", __func__, getIpStr());
        return;
    }

    {
        LOCK(pnode->cs_hSocket);
        LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- successfully connected to %s, sock=%d, peer=%d\n", __func__, getIpStr(), pnode->hSocket, pnode->GetId());
    }

    if (grantOutbound)
        grantOutbound->MoveTo(pnode->grantOutbound);
    if (fOneShot)
        pnode->fOneShot = true;
    if (fFeeler)
        pnode->fFeeler = true;
    if (manual_connection)
        pnode->m_manual_connection = true;
    if (masternode_connection == MasternodeConn::IsConnection)
        pnode->m_masternode_connection = true;
    if (masternode_probe_connection == MasternodeProbeConn::IsConnection)
        pnode->m_masternode_probe_connection = true;

    {
        LOCK2(cs_vNodes, pnode->cs_hSocket);
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

void CConnman::OpenMasternodeConnection(const CAddress &addrConnect, MasternodeProbeConn probe) {
    OpenNetworkConnection(addrConnect, false, nullptr, nullptr, false, false, false, false, MasternodeConn::IsConnection, probe);
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

void CConnman::ThreadI2PAcceptIncoming()
{
    static constexpr auto err_wait_begin = 1s;
    static constexpr auto err_wait_cap = 5min;
    auto err_wait = err_wait_begin;

    bool advertising_listen_addr = false;
    i2p::Connection conn;

    while (!interruptNet) {

        if (!m_i2p_sam_session->Listen(conn)) {
            if (advertising_listen_addr && conn.me.IsValid()) {
                RemoveLocal(conn.me);
                advertising_listen_addr = false;
            }

            interruptNet.sleep_for(err_wait);
            if (err_wait < err_wait_cap) {
                err_wait *= 2;
            }

            continue;
        }

        if (!advertising_listen_addr) {
            AddLocal(conn.me, LOCAL_MANUAL);
            advertising_listen_addr = true;
        }

        if (!m_i2p_sam_session->Accept(conn)) {
            continue;
        }

        CreateNodeFromAcceptedSocket(conn.sock->Release(), NetPermissionFlags::PF_NONE,
                                     CAddress{conn.me, NODE_NONE}, CAddress{conn.peer, NODE_NONE});
    }
}

bool CConnman::BindListenPort(const CService& addrBind, bilingual_str& strError, NetPermissionFlags permissions)
{
    int nOne = 1;

    // Create socket for listening for incoming connections
    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);
    if (!addrBind.GetSockAddr((struct sockaddr*)&sockaddr, &len))
    {
        strError = strprintf(Untranslated("Error: Bind address family for %s not supported"), addrBind.ToString());
        LogPrintf("%s\n", strError.original);
        return false;
    }

    std::unique_ptr<Sock> sock = CreateSock(addrBind);
    if (!sock) {
        strError = strprintf(Untranslated("Error: Couldn't open socket for incoming connections (socket returned error %s)"), NetworkErrorString(WSAGetLastError()));
        LogPrintf("%s\n", strError.original);
        return false;
    }

    // Allow binding if the port is still in TIME_WAIT state after
    // the program was closed and restarted.
    setsockopt(sock->Get(), SOL_SOCKET, SO_REUSEADDR, (sockopt_arg_type)&nOne, sizeof(int));

    // some systems don't have IPV6_V6ONLY but are always v6only; others do have the option
    // and enable it by default or not. Try to enable it, if possible.
    if (addrBind.IsIPv6()) {
#ifdef IPV6_V6ONLY
        setsockopt(sock->Get(), IPPROTO_IPV6, IPV6_V6ONLY, (sockopt_arg_type)&nOne, sizeof(int));
#endif
#ifdef WIN32
        int nProtLevel = PROTECTION_LEVEL_UNRESTRICTED;
        setsockopt(sock->Get(), IPPROTO_IPV6, IPV6_PROTECTION_LEVEL, (const char*)&nProtLevel, sizeof(int));
#endif
    }

    if (::bind(sock->Get(), (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
    {
        int nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE)
            strError = strprintf(_("Unable to bind to %s on this computer. %s is probably already running."), addrBind.ToString(), PACKAGE_NAME);
        else
            strError = strprintf(_("Unable to bind to %s on this computer (bind returned error %s)"), addrBind.ToString(), NetworkErrorString(nErr));
        LogPrintf("%s\n", strError.original);
        return false;
    }
    LogPrintf("Bound to %s\n", addrBind.ToString());

    // Listen for incoming connections
    if (listen(sock->Get(), SOMAXCONN) == SOCKET_ERROR)
    {
        strError = strprintf(_("Error: Listening for incoming connections failed (listen returned error %s)"), NetworkErrorString(WSAGetLastError()));
        LogPrintf("%s\n", strError.original);
        return false;
    }

#ifdef USE_KQUEUE
    if (socketEventsMode == SOCKETEVENTS_KQUEUE) {
        struct kevent event;
        EV_SET(&event, sock->Get(), EVFILT_READ, EV_ADD, 0, 0, nullptr);
        if (kevent(kqueuefd, &event, 1, nullptr, 0, nullptr) != 0) {
            strError = strprintf(_("Error: failed to add socket to kqueuefd (kevent returned error %s)"), NetworkErrorString(WSAGetLastError()));
            LogPrintf("%s\n", strError.original);
            return false;
        }
    }
#endif

#ifdef USE_EPOLL
    if (socketEventsMode == SOCKETEVENTS_EPOLL) {
        epoll_event event;
        event.data.fd = sock->Get();
        event.events = EPOLLIN;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock->Get(), &event) != 0) {
            strError = strprintf(_("Error: failed to add socket to epollfd (epoll_ctl returned error %s)"), NetworkErrorString(WSAGetLastError()));
            LogPrintf("%s\n", strError.original);
            return false;
        }
    }
#endif

    vhListenSocket.push_back(ListenSocket(sock->Release(), permissions));

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
#elif (HAVE_DECL_GETIFADDRS && HAVE_DECL_FREEIFADDRS)
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
    ::masternodeSync->Reset();

    uiInterface.NotifyNetworkActiveChanged(fNetworkActive);
}

CConnman::CConnman(uint64_t nSeed0In, uint64_t nSeed1In, CAddrMan& addrman_in) :
        addrman(addrman_in), nSeed0(nSeed0In), nSeed1(nSeed1In)
{
    SetTryNewOutboundPeer(false);

    Options connOptions;
    Init(connOptions);
}

NodeId CConnman::GetNewNodeId()
{
    return nLastNodeId.fetch_add(1, std::memory_order_relaxed);
}


bool CConnman::Bind(const CService &addr, unsigned int flags, NetPermissionFlags permissions) {
    if (!(flags & BF_EXPLICIT) && !IsReachable(addr)) {
        return false;
    }
    bilingual_str strError;
    if (!BindListenPort(addr, strError, permissions)) {
        if ((flags & BF_REPORT_ERROR) && clientInterface) {
            clientInterface->ThreadSafeMessageBox(strError, "", CClientUIInterface::MSG_ERROR);
        }
        return false;
    }

    if (addr.IsRoutable() && fDiscover && !(flags & BF_DONT_ADVERTISE) && !NetPermissions::HasFlag(permissions, NetPermissionFlags::PF_NOBAN)) {
        AddLocal(addr, LOCAL_BIND);
    }

    return true;
}

bool CConnman::InitBinds(
    const std::vector<CService>& binds,
    const std::vector<NetWhitebindPermissions>& whiteBinds,
    const std::vector<CService>& onion_binds)
{
    bool fBound = false;
    for (const auto& addrBind : binds) {
        fBound |= Bind(addrBind, (BF_EXPLICIT | BF_REPORT_ERROR), NetPermissionFlags::PF_NONE);
    }
    for (const auto& addrBind : whiteBinds) {
        fBound |= Bind(addrBind.m_service, (BF_EXPLICIT | BF_REPORT_ERROR), addrBind.m_flags);
    }
    if (binds.empty() && whiteBinds.empty()) {
        struct in_addr inaddr_any;
        inaddr_any.s_addr = htonl(INADDR_ANY);
        struct in6_addr inaddr6_any = IN6ADDR_ANY_INIT;
        fBound |= Bind(CService(inaddr6_any, GetListenPort()), BF_NONE, NetPermissionFlags::PF_NONE);
        fBound |= Bind(CService(inaddr_any, GetListenPort()), !fBound ? BF_REPORT_ERROR : BF_NONE, NetPermissionFlags::PF_NONE);
    }

    for (const auto& addr_bind : onion_binds) {
        fBound |= Bind(addr_bind, BF_EXPLICIT | BF_DONT_ADVERTISE, NetPermissionFlags::PF_NONE);
    }

    return fBound;
}

bool CConnman::Start(CScheduler& scheduler, const Options& connOptions)
{
    Init(connOptions);

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

    if (fListen && !InitBinds(connOptions.vBinds, connOptions.vWhiteBinds, connOptions.onion_binds)) {
        if (clientInterface) {
            clientInterface->ThreadSafeMessageBox(
                _("Failed to listen on any port. Use -listen=0 if you want this."),
                "", CClientUIInterface::MSG_ERROR);
        }
        return false;
    }

    proxyType i2p_sam;
    if (GetProxy(NET_I2P, i2p_sam)) {
        m_i2p_sam_session = std::make_unique<i2p::sam::Session>(GetDataDir() / "i2p_private_key",
                                                                i2p_sam.proxy, &interruptNet);
    }

    for (const auto& strDest : connOptions.vSeedNodes) {
        AddOneShot(strDest);
    }

    if (clientInterface) {
        clientInterface->InitMessage(_("Loading P2P addresses...").translated);
    }
    // Load addresses from peers.dat
    int64_t nStart = GetTimeMillis();
    {
        CAddrDB adb;
        if (adb.Read(addrman))
            LogPrintf("Loaded %i addresses from peers.dat  %dms\n", addrman.size(), GetTimeMillis() - nStart);
        else {
            addrman.Clear(); // Addrman can be in an inconsistent state after failure, reset it
            LogPrintf("Recreating peers.dat\n");
            DumpAddresses();
        }
    }

    if (m_use_addrman_outgoing) {
        // Load addresses from anchors.dat
        m_anchors = ReadAnchors(GetDataDir() / ANCHORS_DATABASE_FILENAME);
        if (m_anchors.size() > MAX_BLOCK_RELAY_ONLY_ANCHORS) {
            m_anchors.resize(MAX_BLOCK_RELAY_ONLY_ANCHORS);
        }
        LogPrintf("%i block-relay-only anchors will be tried for connections.\n", m_anchors.size());
    }

    uiInterface.InitMessage(_("Starting network threads...").translated);

    fAddressesInitialized = true;

    if (semOutbound == nullptr) {
        // initialize semaphore
        semOutbound = std::make_unique<CSemaphore>(std::min(m_max_outbound, nMaxConnections));
    }
    if (semAddnode == nullptr) {
        // initialize semaphore
        semAddnode = std::make_unique<CSemaphore>(nMaxAddnode);
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
    threadSocketHandler = std::thread(&util::TraceThread, "net", [this] { ThreadSocketHandler(); });

    if (!gArgs.GetBoolArg("-dnsseed", true))
        LogPrintf("DNS seeding disabled\n");
    else
        threadDNSAddressSeed = std::thread(&util::TraceThread, "dnsseed", [this] { ThreadDNSAddressSeed(); });

    // Initiate outbound connections from -addnode
    threadOpenAddedConnections = std::thread(&util::TraceThread, "addcon", [this] { ThreadOpenAddedConnections(); });

    if (connOptions.m_use_addrman_outgoing && !connOptions.m_specified_outgoing.empty()) {
        if (clientInterface) {
            clientInterface->ThreadSafeMessageBox(
                _("Cannot provide specific connections and have addrman find outgoing connections at the same."),
                "", CClientUIInterface::MSG_ERROR);
        }
        return false;
    }
    if (connOptions.m_use_addrman_outgoing || !connOptions.m_specified_outgoing.empty()) {
        threadOpenConnections = std::thread(
            &util::TraceThread, "opencon",
            [this, connect = connOptions.m_specified_outgoing] { ThreadOpenConnections(connect); });
    }

    // Initiate masternode connections
    threadOpenMasternodeConnections = std::thread(&util::TraceThread, "mncon", [this] { ThreadOpenMasternodeConnections(); });

    // Process messages
    threadMessageHandler = std::thread(&util::TraceThread, "msghand", [this] { ThreadMessageHandler(); });

    if (connOptions.m_i2p_accept_incoming && m_i2p_sam_session.get() != nullptr) {
        threadI2PAcceptIncoming =
            std::thread(&util::TraceThread, "i2paccept", [this] { ThreadI2PAcceptIncoming(); });
    }

    // Dump network addresses
    scheduler.scheduleEvery([this] { DumpAddresses(); }, DUMP_PEERS_INTERVAL);

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
};
static CNetCleanup instance_of_cnetcleanup;

void CExplicitNetCleanup::callCleanup()
{
    // Explicit call to destructor of CNetCleanup because it's not implicitly called
    // when the wallet is restarted from within the wallet itself.
    CNetCleanup tmp;
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
        for (int i=0; i<m_max_outbound; i++) {
            semOutbound->post();
        }
    }

    if (semAddnode) {
        for (int i=0; i<nMaxAddnode; i++) {
            semAddnode->post();
        }
    }
}

void CConnman::StopThreads()
{
    if (threadI2PAcceptIncoming.joinable()) {
        threadI2PAcceptIncoming.join();
    }
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
}

void CConnman::StopNodes()
{
    if (fAddressesInitialized) {
        DumpAddresses();
        fAddressesInitialized = false;

        if (m_use_addrman_outgoing) {
            // Anchor connections are only dumped during clean shutdown.
            std::vector<CAddress> anchors_to_dump = GetCurrentBlockRelayOnlyConns();
            if (anchors_to_dump.size() > MAX_BLOCK_RELAY_ONLY_ANCHORS) {
                anchors_to_dump.resize(MAX_BLOCK_RELAY_ONLY_ANCHORS);
            }
            DumpAnchors(GetDataDir() / ANCHORS_DATABASE_FILENAME, anchors_to_dump);
        }
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
    std::vector<CNode*> nodes;
    WITH_LOCK(cs_vNodes, nodes.swap(vNodes));
    for (CNode* pnode : nodes) {
        DeleteNode(pnode);
    }
    for (CNode* pnode : vNodesDisconnected) {
        DeleteNode(pnode);
    }
    mapSocketToNode.clear();
    {
        LOCK(cs_vNodes);
        mapReceivableNodes.clear();
    }
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
    m_msgproc->FinalizeNode(*pnode);
    delete pnode;
}

CConnman::~CConnman()
{
    Interrupt();
    Stop();
}

std::vector<CAddress> CConnman::GetAddresses(size_t max_addresses, size_t max_pct, std::optional<Network> network)
{
    std::vector<CAddress> addresses = addrman.GetAddr(max_addresses, max_pct, network);
    if (m_banman) {
        addresses.erase(std::remove_if(addresses.begin(), addresses.end(),
                        [this](const CAddress& addr){return m_banman->IsDiscouraged(addr) || m_banman->IsBanned(addr);}),
                        addresses.end());
    }
    return addresses;
}

std::vector<CAddress> CConnman::GetAddresses(CNode& requestor, size_t max_addresses, size_t max_pct)
{
    auto local_socket_bytes = requestor.addrBind.GetAddrBytes();
    uint64_t cache_id = GetDeterministicRandomizer(RANDOMIZER_ID_ADDRCACHE)
        .Write(requestor.addr.GetNetwork())
        .Write(local_socket_bytes.data(), local_socket_bytes.size())
        .Finalize();
    const auto current_time = GetTime<std::chrono::microseconds>();
    auto r = m_addr_response_caches.emplace(cache_id, CachedAddrResponse{});
    CachedAddrResponse& cache_entry = r.first->second;
    if (cache_entry.m_cache_entry_expiration < current_time) { // If emplace() added new one it has expiration 0.
        cache_entry.m_addrs_response_cache = GetAddresses(max_addresses, max_pct, /* network */ std::nullopt);
        // Choosing a proper cache lifetime is a trade-off between the privacy leak minimization
        // and the usefulness of ADDR responses to honest users.
        //
        // Longer cache lifetime makes it more difficult for an attacker to scrape
        // enough AddrMan data to maliciously infer something useful.
        // By the time an attacker scraped enough AddrMan records, most of
        // the records should be old enough to not leak topology info by
        // e.g. analyzing real-time changes in timestamps.
        //
        // It takes only several hundred requests to scrape everything from an AddrMan containing 100,000 nodes,
        // so ~24 hours of cache lifetime indeed makes the data less inferable by the time
        // most of it could be scraped (considering that timestamps are updated via
        // ADDR self-announcements and when nodes communicate).
        // We also should be robust to those attacks which may not require scraping *full* victim's AddrMan
        // (because even several timestamps of the same handful of nodes may leak privacy).
        //
        // On the other hand, longer cache lifetime makes ADDR responses
        // outdated and less useful for an honest requestor, e.g. if most nodes
        // in the ADDR response are no longer active.
        //
        // However, the churn in the network is known to be rather low. Since we consider
        // nodes to be "terrible" (see IsTerrible()) if the timestamps are older than 30 days,
        // max. 24 hours of "penalty" due to cache shouldn't make any meaningful difference
        // in terms of the freshness of the response.
        cache_entry.m_cache_entry_expiration = current_time + std::chrono::hours(21) + GetRandMillis(std::chrono::hours(6));
    }
    return cache_entry.m_addrs_response_cache;
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
        auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
        if (!verifiedProRegTxHash.IsNull() && !pnode->m_masternode_iqr_connection && IsMasternodeQuorumRelayMember(verifiedProRegTxHash)) {
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
        auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
        if (!pnode->qwatch && (verifiedProRegTxHash.IsNull() || !proRegTxHashes.count(verifiedProRegTxHash))) {
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
    if (pnode->GetVerifiedProRegTxHash().IsNull() && !pnode->fInbound) {
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
        if (!pnode->GetVerifiedProRegTxHash().IsNull()) {
            if (p.second.count(pnode->GetVerifiedProRegTxHash())) {
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
        if ((flags & CONNECTIONS_VERIFIED) && pnode->GetVerifiedProRegTxHash().IsNull()) {
            continue;
        }
        if (flags & (pnode->fInbound ? CONNECTIONS_IN : CONNECTIONS_OUT)) {
            nNum++;
        } else if (flags == CONNECTIONS_VERIFIED) {
            nNum++;
        }
    }

    return nNum;
}

size_t CConnman::GetMaxOutboundNodeCount()
{
    return m_max_outbound;
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

bool CConnman::DisconnectNode(const CSubNet& subnet)
{
    bool disconnected = false;
    LOCK(cs_vNodes);
    for (CNode* pnode : vNodes) {
        if (subnet.Match(pnode->addr)) {
            pnode->fDisconnect = true;
            disconnected = true;
        }
    }
    return disconnected;
}

bool CConnman::DisconnectNode(const CNetAddr& addr)
{
    return DisconnectNode(CSubNet(addr));
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
        if (pnode->nVersion < minProtoVersion || !pnode->CanRelay() || !pnode->IsAddrRelayPeer()) {
            continue;
        }
        {
            LOCK(pnode->m_tx_relay->cs_filter);
            if (!pnode->m_tx_relay->fRelayTxes) {
                continue;
            }
            if (pnode->m_tx_relay->pfilter && !pnode->m_tx_relay->pfilter->IsRelevantAndUpdate(relatedTx)) {
                continue;
            }
        }
        pnode->PushInventory(inv);
    }
}

void CConnman::RelayInvFiltered(CInv &inv, const uint256& relatedTxHash, const int minProtoVersion)
{
    LOCK(cs_vNodes);
    for (const auto& pnode : vNodes) {
        if (pnode->nVersion < minProtoVersion || !pnode->CanRelay() || !pnode->IsAddrRelayPeer()) {
            continue;
        }
        {
            LOCK(pnode->m_tx_relay->cs_filter);
            if (!pnode->m_tx_relay->fRelayTxes) {
                continue;
            }
            if (pnode->m_tx_relay->pfilter && !pnode->m_tx_relay->pfilter->contains(relatedTxHash)) {
                continue;
            }
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

    const auto now = GetTime<std::chrono::seconds>();
    if (nMaxOutboundCycleStartTime + MAX_UPLOAD_TIMEFRAME < now)
    {
        // timeframe expired, reset cycle
        nMaxOutboundCycleStartTime = now;
        nMaxOutboundTotalBytesSentInCycle = 0;
    }

    nMaxOutboundTotalBytesSentInCycle += bytes;
}

uint64_t CConnman::GetMaxOutboundTarget()
{
    LOCK(cs_totalBytesSent);
    return nMaxOutboundLimit;
}

std::chrono::seconds CConnman::GetMaxOutboundTimeframe()
{
    return MAX_UPLOAD_TIMEFRAME;
}

std::chrono::seconds CConnman::GetMaxOutboundTimeLeftInCycle()
{
    LOCK(cs_totalBytesSent);
    if (nMaxOutboundLimit == 0)
        return 0s;

    if (nMaxOutboundCycleStartTime.count() == 0)
        return MAX_UPLOAD_TIMEFRAME;

    const std::chrono::seconds cycleEndTime = nMaxOutboundCycleStartTime + MAX_UPLOAD_TIMEFRAME;
    const auto now = GetTime<std::chrono::seconds>();
    return (cycleEndTime < now) ? 0s : cycleEndTime - now;
}

bool CConnman::OutboundTargetReached(bool historicalBlockServingLimit)
{
    LOCK(cs_totalBytesSent);
    if (nMaxOutboundLimit == 0)
        return false;

    if (historicalBlockServingLimit)
    {
        // keep a large enough buffer to at least relay each block once
        const std::chrono::seconds timeLeftInCycle = GetMaxOutboundTimeLeftInCycle();
        const uint64_t buffer = timeLeftInCycle / std::chrono::minutes{10} * MaxBlockSize(fDIP0001ActiveAtTip);
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

unsigned int CConnman::GetReceiveFloodSize() const { return nReceiveFloodSize; }

CNode::CNode(NodeId idIn, ServiceFlags nLocalServicesIn, SOCKET hSocketIn, const CAddress& addrIn, uint64_t nKeyedNetGroupIn, uint64_t nLocalHostNonceIn, const CAddress& addrBindIn, const std::string& addrNameIn, bool fInboundIn, bool block_relay_only, bool inbound_onion)
    : nTimeConnected(GetSystemTimeInSeconds()),
    addr(addrIn),
    addrBind(addrBindIn),
    fInbound(fInboundIn),
    nKeyedNetGroup(nKeyedNetGroupIn),
    m_addr_known{block_relay_only ? nullptr : std::make_unique<CRollingBloomFilter>(5000, 0.001)},
    id(idIn),
    nLocalHostNonce(nLocalHostNonceIn),
    nLocalServices(nLocalServicesIn),
    m_inbound_onion(inbound_onion)
{
    hSocket = hSocketIn;
    addrName = addrNameIn == "" ? addr.ToStringIPPort() : addrNameIn;
    hashContinue = uint256();

    for (const std::string &msg : getAllNetMessageTypes())
        mapRecvBytesPerMsgCmd[msg] = 0;
    mapRecvBytesPerMsgCmd[NET_MESSAGE_COMMAND_OTHER] = 0;

    if (fLogIPs) {
        LogPrint(BCLog::NET, "Added connection to %s peer=%d\n", addrName, id);
    } else {
        LogPrint(BCLog::NET, "Added connection peer=%d\n", id);
    }

    m_deserializer = std::make_unique<V1TransportDeserializer>(V1TransportDeserializer(Params(), GetId(), SER_NETWORK, INIT_PROTO_VERSION));
    m_serializer = std::make_unique<V1TransportSerializer>(V1TransportSerializer());
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
    LogPrint(BCLog::NET, "sending %s (%d bytes) peer=%d\n", SanitizeString(msg.command), nMessageSize, pnode->GetId());

    // make sure we use the appropriate network transport format
    std::vector<unsigned char> serializedHeader;
    pnode->m_serializer->prepareForTransport(msg, serializedHeader);

    size_t nTotalSize = nMessageSize + serializedHeader.size();
    statsClient.count("bandwidth.message." + SanitizeString(msg.command.c_str()) + ".bytesSent", nTotalSize, 1.0f);
    statsClient.inc("message.sent." + SanitizeString(msg.command.c_str()), 1.0f);

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
