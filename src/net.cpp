// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <net.h>
#include <netmessagemaker.h>

#include <addrdb.h>
#include <banman.h>
#include <clientversion.h>
#include <compat.h>
#include <consensus/consensus.h>
#include <crypto/sha256.h>
#include <i2p.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <netbase.h>
#include <node/ui_interface.h>
#include <protocol.h>
#include <random.h>
#include <scheduler.h>
#include <util/sock.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/thread.h>
#include <util/time.h>
#include <util/translation.h>
#include <util/wpipe.h>
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
#include <array>
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
    BF_REPORT_ERROR = (1U << 0),
    /**
     * Do not call AddLocal() for our special addresses, e.g., for incoming
     * Tor connections, to prevent gossiping them over the network.
     */
    BF_DONT_ADVERTISE = (1U << 1),
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
#endif /* USE_WAKEUP_PIPE */

const std::string NET_MESSAGE_TYPE_OTHER = "*other*";

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

void CConnman::AddAddrFetch(const std::string& strDest)
{
    LOCK(m_addr_fetches_mutex);
    m_addr_fetches.push_back(strDest);
}

uint16_t GetListenPort()
{
    // If -bind= is provided with ":port" part, use that (first one if multiple are provided).
    for (const std::string& bind_arg : gArgs.GetArgs("-bind")) {
        CService bind_addr;
        constexpr uint16_t dummy_port = 0;

        if (Lookup(bind_arg, bind_addr, dummy_port, /*fAllowLookup=*/false)) {
            if (bind_addr.GetPort() != dummy_port) {
                return bind_addr.GetPort();
            }
        }
    }

    // Otherwise, if -whitebind= without NetPermissionFlags::NoBan is provided, use that
    // (-whitebind= is required to have ":port").
    for (const std::string& whitebind_arg : gArgs.GetArgs("-whitebind")) {
        NetWhitebindPermissions whitebind;
        bilingual_str error;
        if (NetWhitebindPermissions::TryParse(whitebind_arg, whitebind, error)) {
            if (!NetPermissions::HasFlag(whitebind.m_flags, NetPermissionFlags::NoBan)) {
                return whitebind.m_service.GetPort();
            }
        }
    }

    // Otherwise, if -port= is provided, use that. Otherwise use the default port.
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
        if (pnode->IsInboundConn()) {
            // For inbound connections, assume both the address and the port
            // as seen from the peer.
            addrLocal = CAddress{pnode->GetAddrLocal(), addrLocal.nServices};
        } else {
            // For outbound connections, assume just the address as seen from
            // the peer and leave the port in `addrLocal` as returned by
            // `GetLocalAddress()` above. The peer has no way to observe our
            // listening port when we have initiated the connection.
            addrLocal.SetIP(pnode->GetAddrLocal());
        }
    }
    if (addrLocal.IsRoutable() || gArgs.GetBoolArg("-addrmantest", false))
    {
        LogPrint(BCLog::NET, "Advertising address %s to peer=%d\n", addrLocal.ToString(), pnode->GetId());
        return addrLocal;
    }
    // Address is unroutable. Don't advertise.
    return std::nullopt;
}

/**
 * If an IPv6 address belongs to the address range used by the CJDNS network and
 * the CJDNS network is reachable (-cjdnsreachable config is set), then change
 * the type from NET_IPV6 to NET_CJDNS.
 * @param[in] service Address to potentially convert.
 * @return a copy of `service` either unmodified or changed to CJDNS.
 */
CService MaybeFlipIPv6toCJDNS(const CService& service)
{
    CService ret{service};
    if (ret.m_net == NET_IPV6 && ret.m_addr[0] == 0xfc && IsReachable(NET_CJDNS)) {
        ret.m_net = NET_CJDNS;
    }
    return ret;
}

// learn a new local address
bool AddLocal(const CService& addr_, int nScore)
{
    CService addr{MaybeFlipIPv6toCJDNS(addr_)};

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
    LOCK(m_nodes_mutex);
    for (CNode* pnode : m_nodes) {
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
    LOCK(m_nodes_mutex);
    for (CNode* pnode : m_nodes) {
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
    LOCK(m_nodes_mutex);
    for (CNode* pnode : m_nodes) {
        if (fExcludeDisconnecting && pnode->fDisconnect) {
            continue;
        }
        if (pnode->m_addr_name == addrName) {
            return pnode;
        }
    }
    return nullptr;
}

CNode* CConnman::FindNode(const CService& addr, bool fExcludeDisconnecting)
{
    LOCK(m_nodes_mutex);
    for (CNode* pnode : m_nodes) {
        if (fExcludeDisconnecting && pnode->fDisconnect) {
            continue;
        }
        if (static_cast<CService>(pnode->addr) == addr) {
            return pnode;
        }
    }
    return nullptr;
}

bool CConnman::AlreadyConnectedToAddress(const CAddress& addr)
{
    return FindNode(addr.ToStringIPPort());
}

bool CConnman::CheckIncomingNonce(uint64_t nonce)
{
    LOCK(m_nodes_mutex);
    for (const CNode* pnode : m_nodes) {
        if (!pnode->fSuccessfullyConnected && !pnode->IsInboundConn() && pnode->GetLocalNonce() == nonce)
            return false;
    }
    return true;
}

/** Get the bind address for a socket as CAddress */
static CAddress GetBindAddress(const Sock& sock)
{
    CAddress addr_bind;
    struct sockaddr_storage sockaddr_bind;
    socklen_t sockaddr_bind_len = sizeof(sockaddr_bind);
    if (sock.Get() != INVALID_SOCKET) {
        if (!sock.GetSockName((struct sockaddr*)&sockaddr_bind, &sockaddr_bind_len)) {
            addr_bind.SetSockAddr((const struct sockaddr*)&sockaddr_bind);
        } else {
            LogPrint(BCLog::NET, "Warning: getsockname failed\n");
        }
    }
    return addr_bind;
}

CNode* CConnman::ConnectNode(CAddress addrConnect, const char *pszDest, bool fCountFailure, ConnectionType conn_type)
{
    AssertLockNotHeld(m_unused_i2p_sessions_mutex);
    assert(conn_type != ConnectionType::INBOUND);

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
            const CService rnd{resolved[GetRand(resolved.size())]};
            addrConnect = CAddress{MaybeFlipIPv6toCJDNS(rnd), NODE_NONE};
            if (!addrConnect.IsValid()) {
                LogPrint(BCLog::NET, "Resolver returned invalid address %s for %s\n", addrConnect.ToString(), pszDest);
                return nullptr;
            }
            // It is possible that we already have a connection to the IP/port pszDest resolved to.
            // In that case, drop the connection that was just created.
            LOCK(m_nodes_mutex);
            CNode* pnode = FindNode(static_cast<CService>(addrConnect));
            if (pnode) {
                LogPrintf("Failed to open new connection, already connected\n");
                return nullptr;
            }
        }
    }

    // Connect
    bool connected = false;
    std::unique_ptr<Sock> sock;
    Proxy proxy;
    CAddress addr_bind;
    assert(!addr_bind.IsValid());
    std::unique_ptr<i2p::sam::Session> i2p_transient_session;

    if (addrConnect.IsValid()) {
        const bool use_proxy{GetProxy(addrConnect.GetNetwork(), proxy)};
        bool proxyConnectionFailed = false;

        if (addrConnect.GetNetwork() == NET_I2P && use_proxy) {
            i2p::Connection conn;

            if (m_i2p_sam_session) {
                connected = m_i2p_sam_session->Connect(addrConnect, conn, proxyConnectionFailed);
            } else {
                {
                    LOCK(m_unused_i2p_sessions_mutex);
                    if (m_unused_i2p_sessions.empty()) {
                        i2p_transient_session =
                            std::make_unique<i2p::sam::Session>(proxy.proxy, &interruptNet);
                    } else {
                        i2p_transient_session.swap(m_unused_i2p_sessions.front());
                        m_unused_i2p_sessions.pop();
                    }
                }
                connected = i2p_transient_session->Connect(addrConnect, conn, proxyConnectionFailed);
                if (!connected) {
                    LOCK(m_unused_i2p_sessions_mutex);
                    if (m_unused_i2p_sessions.size() < MAX_UNUSED_I2P_SESSIONS_SIZE) {
                        m_unused_i2p_sessions.emplace(i2p_transient_session.release());
                    }
                }
            }

            if (connected) {
                sock = std::move(conn.sock);
                addr_bind = CAddress{conn.me, NODE_NONE};
            }
        } else if (use_proxy) {
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
            connected = ConnectSocketDirectly(addrConnect, *sock, nConnectTimeout, conn_type == ConnectionType::MANUAL);
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
        addr_bind = GetBindAddress(*sock);
    }
    CNode* pnode = new CNode(id,
                             nLocalServices,
                             std::move(sock),
                             addrConnect,
                             CalculateKeyedNetGroup(addrConnect),
                             nonce,
                             addr_bind,
                             pszDest ? pszDest : "",
                             conn_type,
                             /*inbound_onion=*/false,
                             std::move(i2p_transient_session));
    pnode->AddRef();
    statsClient.inc("peers.connect", 1.0f);

    // We're making a new connection, harvest entropy from the time (and our peer count)
    RandAddEvent((uint32_t)id);

    return pnode;
}

void CNode::CloseSocketDisconnect(CConnman* connman)
{
    AssertLockHeld(connman->m_nodes_mutex);

    fDisconnect = true;
    LOCK2(connman->cs_mapSocketToNode, m_sock_mutex);
    if (!m_sock) {
        return;
    }

    fHasRecvData = false;
    fCanSendData = false;

    connman->mapSocketToNode.erase(m_sock->Get());
    {
        LOCK(connman->cs_sendable_receivable_nodes);
        connman->mapReceivableNodes.erase(GetId());
        connman->mapSendableNodes.erase(GetId());
    }
    {
        LOCK(connman->cs_mapNodesWithDataToSend);
        if (connman->mapNodesWithDataToSend.erase(GetId()) != 0) {
            // See comment in PushMessage
            Release();
        }
    }

    if (connman->m_edge_trig_events && !connman->m_edge_trig_events->UnregisterEvents(m_sock->Get())) {
        LogPrint(BCLog::NET, "EdgeTriggeredEvents::UnregisterEvents() failed\n");
    }

    LogPrint(BCLog::NET, "disconnecting peer=%d\n", id);
    m_sock.reset();
    m_i2p_sam_session.reset();

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
    return (ignores_incoming_txs && !HasPermission(NetPermissionFlags::Relay)) || IsBlockOnlyConn();
}

std::string ConnectionTypeAsString(ConnectionType conn_type)
{
    switch (conn_type) {
    case ConnectionType::INBOUND:
        return "inbound";
    case ConnectionType::MANUAL:
        return "manual";
    case ConnectionType::FEELER:
        return "feeler";
    case ConnectionType::OUTBOUND_FULL_RELAY:
        return "outbound-full-relay";
    case ConnectionType::BLOCK_RELAY:
        return "block-relay-only";
    case ConnectionType::ADDR_FETCH:
        return "addr-fetch";
    } // no default case, so the compiler can warn about missing cases

    assert(false);
}

CService CNode::GetAddrLocal() const
{
    AssertLockNotHeld(m_addr_local_mutex);
    LOCK(m_addr_local_mutex);
    return addrLocal;
}

void CNode::SetAddrLocal(const CService& addrLocalIn) {
    AssertLockNotHeld(m_addr_local_mutex);
    LOCK(m_addr_local_mutex);
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
    return m_inbound_onion ? NET_ONION : addr.GetNetClass();
}

#undef X
#define X(name) stats.name = name
void CNode::CopyStats(CNodeStats& stats)
{
    stats.nodeid = this->GetId();
    X(nServices);
    X(addr);
    X(addrBind);
    stats.m_network = ConnectedThroughNetwork();
    X(m_last_send);
    X(m_last_recv);
    X(m_last_tx_time);
    X(m_last_block_time);
    X(m_connected);
    X(nTimeOffset);
    X(m_addr_name);
    X(nVersion);
    {
        LOCK(m_subver_mutex);
        X(cleanSubVer);
    }
    stats.fInbound = IsInboundConn();
    stats.m_manual_connection = IsManualConn();
    X(m_bip152_highbandwidth_to);
    X(m_bip152_highbandwidth_from);
    {
        LOCK(cs_vSend);
        X(mapSendBytesPerMsgType);
        X(nSendBytes);
    }
    {
        LOCK(cs_vRecv);
        X(mapRecvBytesPerMsgType);
        X(nRecvBytes);
    }
    X(m_legacyWhitelisted);
    X(m_permissionFlags);

    X(m_last_ping_time);
    X(m_min_ping_time);

    // Leave string empty if addrLocal invalid (not filled in yet)
    CService addrLocalUnlocked = GetAddrLocal();
    stats.addrLocal = addrLocalUnlocked.IsValid() ? addrLocalUnlocked.ToString() : "";

    {
        LOCK(cs_mnauth);
        X(verifiedProRegTxHash);
        X(verifiedPubKeyHash);
    }
    X(m_masternode_connection);
    X(m_conn_type);
}
#undef X

bool CNode::ReceiveMsgBytes(Span<const uint8_t> msg_bytes, bool& complete)
{
    complete = false;
    // TODO: use mocktime here after bitcoin#19499 is backported
    const auto time = std::chrono::microseconds(GetTimeMicros());
    LOCK(cs_vRecv);
    m_last_recv = std::chrono::duration_cast<std::chrono::seconds>(time);
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
            bool reject_message{false};
            CNetMessage msg = m_deserializer->GetMessage(time, reject_message);
            if (reject_message) {
                // Message deserialization failed. Drop the message but don't disconnect the peer.
                // store the size of the corrupt message
                mapRecvBytesPerMsgType.at(NET_MESSAGE_TYPE_OTHER) += msg.m_raw_message_size;
                continue;
            }

            // Store received bytes per message type.
            // To prevent a memory DOS, only allow known message types.
            auto i = mapRecvBytesPerMsgType.find(msg.m_type);
            if (i == mapRecvBytesPerMsgType.end()) {
                i = mapRecvBytesPerMsgType.find(NET_MESSAGE_TYPE_OTHER);
            }
            assert(i != mapRecvBytesPerMsgType.end());
            i->second += msg.m_raw_message_size;
            statsClient.count("bandwidth.message." + std::string(msg.m_type) + ".bytesReceived", msg.m_raw_message_size, 1.0f);

            // push the message to the process queue,
            vRecvMsg.push_back(std::move(msg));

            complete = true;
        }
    }

    return true;
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
        LogPrint(BCLog::NET, "Header error: Unable to deserialize, peer=%d\n", m_node_id);
        return -1;
    }

    // Check start string, network magic
    if (memcmp(hdr.pchMessageStart, m_chain_params.MessageStart(), CMessageHeader::MESSAGE_START_SIZE) != 0) {
        LogPrint(BCLog::NET, "Header error: Wrong MessageStart %s received, peer=%d\n", HexStr(hdr.pchMessageStart), m_node_id);
        return -1;
    }

    // reject messages larger than MAX_SIZE or MAX_PROTOCOL_MESSAGE_LENGTH
    if (hdr.nMessageSize > MAX_SIZE || hdr.nMessageSize > MAX_PROTOCOL_MESSAGE_LENGTH) {
        LogPrint(BCLog::NET, "Header error: Size too large (%s, %u bytes), peer=%d\n", SanitizeString(hdr.GetCommand()), hdr.nMessageSize, m_node_id);
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

CNetMessage V1TransportDeserializer::GetMessage(const std::chrono::microseconds time, bool& reject_message)
{
    // Initialize out parameter
    reject_message = false;
    // decompose a single CNetMessage from the TransportDeserializer
    CNetMessage msg(std::move(vRecv));

    // store message type string, time, and sizes
    msg.m_type = hdr.GetCommand();
    msg.m_time = time;
    msg.m_message_size = hdr.nMessageSize;
    msg.m_raw_message_size = hdr.nMessageSize + CMessageHeader::HEADER_SIZE;

    uint256 hash = GetMessageHash();

    // We just received a message off the wire, harvest entropy from the time (and the message checksum)
    RandAddEvent(ReadLE32(hash.begin()));

    // Check checksum and header message type string
    if (memcmp(hash.begin(), hdr.pchChecksum, CMessageHeader::CHECKSUM_SIZE) != 0) {
        LogPrint(BCLog::NET, "Header error: Wrong checksum (%s, %u bytes), expected %s was %s, peer=%d\n",
                 SanitizeString(msg.m_type), msg.m_message_size,
                 HexStr(Span{hash}.first(CMessageHeader::CHECKSUM_SIZE)),
                 HexStr(hdr.pchChecksum),
                 m_node_id);
        reject_message = true;
    } else if (!hdr.IsCommandValid()) {
        LogPrint(BCLog::NET, "Header error: Invalid message type (%s, %u bytes), peer=%d\n",
                 SanitizeString(hdr.GetCommand()), msg.m_message_size, m_node_id);
        reject_message = true;
    }

    // Always reset the network deserializer (prepare for the next message)
    Reset();
    return msg;
}

void V1TransportSerializer::prepareForTransport(CSerializedNetMsg& msg, std::vector<unsigned char>& header) const
{
    // create dbl-sha256 checksum
    uint256 hash = Hash(msg.data);

    // create header
    CMessageHeader hdr(Params().MessageStart(), msg.m_type.c_str(), msg.data.size());
    memcpy(hdr.pchChecksum, hash.begin(), CMessageHeader::CHECKSUM_SIZE);

    // serialize header
    header.reserve(CMessageHeader::HEADER_SIZE);
    CVectorWriter{SER_NETWORK, INIT_PROTO_VERSION, header, 0, hdr};
}

size_t CConnman::SocketSendData(CNode& node)
{
    auto it = node.vSendMsg.begin();
    size_t nSentSize = 0;

    while (it != node.vSendMsg.end()) {
        const auto& data = *it;
        assert(data.size() > node.nSendOffset);
        int nBytes = 0;
        {
            LOCK(node.m_sock_mutex);
            if (!node.m_sock) {
                break;
            }
            nBytes = node.m_sock->Send(reinterpret_cast<const char*>(data.data()) + node.nSendOffset, data.size() - node.nSendOffset, MSG_NOSIGNAL | MSG_DONTWAIT);
        }
        if (nBytes > 0) {
            node.m_last_send = GetTime<std::chrono::seconds>();
            node.nSendBytes += nBytes;
            node.nSendOffset += nBytes;
            nSentSize += nBytes;
            if (node.nSendOffset == data.size()) {
                node.nSendOffset = 0;
                node.nSendSize -= data.size();
                node.fPauseSend = node.nSendSize > nSendBufferMaxSize;
                it++;
            } else {
                // could not send full message; stop sending more
                node.fCanSendData = false;
                break;
            }
        } else {
            if (nBytes < 0) {
                // error
                int nErr = WSAGetLastError();
                if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS) {
                    LogPrint(BCLog::NET, "socket send error for peer=%d: %s\n", node.GetId(), NetworkErrorString(nErr));
                    node.fDisconnect = true;
                }
            }
            // couldn't send anything at all
            node.fCanSendData = false;
            break;
        }
    }

    if (it == node.vSendMsg.end()) {
        assert(node.nSendOffset == 0);
        assert(node.nSendSize == 0);
    }
    node.vSendMsg.erase(node.vSendMsg.begin(), it);
    node.nSendMsgSize = node.vSendMsg.size();
    return nSentSize;
}

static bool ReverseCompareNodeMinPingTime(const NodeEvictionCandidate& a, const NodeEvictionCandidate& b)
{
    return a.m_min_ping_time > b.m_min_ping_time;
}

static bool ReverseCompareNodeTimeConnected(const NodeEvictionCandidate& a, const NodeEvictionCandidate& b)
{
    return a.m_connected > b.m_connected;
}

static bool CompareNetGroupKeyed(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b) {
    return a.nKeyedNetGroup < b.nKeyedNetGroup;
}

static bool CompareNodeBlockTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    // There is a fall-through here because it is common for a node to have many peers which have not yet relayed a block.
    if (a.m_last_block_time != b.m_last_block_time) return a.m_last_block_time < b.m_last_block_time;
    if (a.fRelevantServices != b.fRelevantServices) return b.fRelevantServices;
    return a.m_connected > b.m_connected;
}

static bool CompareNodeTXTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    // There is a fall-through here because it is common for a node to have more than a few peers that have not yet relayed txn.
    if (a.m_last_tx_time != b.m_last_tx_time) return a.m_last_tx_time < b.m_last_tx_time;
    if (a.m_relay_txs != b.m_relay_txs) return b.m_relay_txs;
    if (a.fBloomFilter != b.fBloomFilter) return a.fBloomFilter;
    return a.m_connected > b.m_connected;
}

// Pick out the potential block-relay only peers, and sort them by last block time.
static bool CompareNodeBlockRelayOnlyTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    if (a.m_relay_txs != b.m_relay_txs) return a.m_relay_txs;
    if (a.m_last_block_time != b.m_last_block_time) return a.m_last_block_time < b.m_last_block_time;
    if (a.fRelevantServices != b.fRelevantServices) return b.fRelevantServices;
    return a.m_connected > b.m_connected;
}

/**
 * Sort eviction candidates by network/localhost and connection uptime.
 * Candidates near the beginning are more likely to be evicted, and those
 * near the end are more likely to be protected, e.g. less likely to be evicted.
 * - First, nodes that are not `is_local` and that do not belong to `network`,
 *   sorted by increasing uptime (from most recently connected to connected longer).
 * - Then, nodes that are `is_local` or belong to `network`, sorted by increasing uptime.
 */
struct CompareNodeNetworkTime {
    const bool m_is_local;
    const Network m_network;
    CompareNodeNetworkTime(bool is_local, Network network) : m_is_local(is_local), m_network(network) {}
    bool operator()(const NodeEvictionCandidate& a, const NodeEvictionCandidate& b) const
    {
        if (m_is_local && a.m_is_local != b.m_is_local) return b.m_is_local;
        if ((a.m_network == m_network) != (b.m_network == m_network)) return b.m_network == m_network;
        return a.m_connected > b.m_connected;
    };
};

//! Sort an array by the specified comparator, then erase the last K elements where predicate is true.
template <typename T, typename Comparator>
static void EraseLastKElements(
    std::vector<T>& elements, Comparator comparator, size_t k,
    std::function<bool(const NodeEvictionCandidate&)> predicate = [](const NodeEvictionCandidate& n) { return true; })
{
    std::sort(elements.begin(), elements.end(), comparator);
    size_t eraseSize = std::min(k, elements.size());
    elements.erase(std::remove_if(elements.end() - eraseSize, elements.end(), predicate), elements.end());
}

void ProtectEvictionCandidatesByRatio(std::vector<NodeEvictionCandidate>& eviction_candidates)
{
    // Protect the half of the remaining nodes which have been connected the longest.
    // This replicates the non-eviction implicit behavior, and precludes attacks that start later.
    // To favorise the diversity of our peer connections, reserve up to half of these protected
    // spots for Tor/onion, localhost, I2P, and CJDNS peers, even if they're not longest uptime
    // overall. This helps protect these higher-latency peers that tend to be otherwise
    // disadvantaged under our eviction criteria.
    const size_t initial_size = eviction_candidates.size();
    const size_t total_protect_size{initial_size / 2};

    // Disadvantaged networks to protect. In the case of equal counts, earlier array members
    // have the first opportunity to recover unused slots from the previous iteration.
    struct Net { bool is_local; Network id; size_t count; };
    std::array<Net, 4> networks{
        {{false, NET_CJDNS, 0}, {false, NET_I2P, 0}, {/*localhost=*/true, NET_MAX, 0}, {false, NET_ONION, 0}}};

    // Count and store the number of eviction candidates per network.
    for (Net& n : networks) {
        n.count = std::count_if(eviction_candidates.cbegin(), eviction_candidates.cend(),
                                [&n](const NodeEvictionCandidate& c) {
                                    return n.is_local ? c.m_is_local : c.m_network == n.id;
                                });
    }
    // Sort `networks` by ascending candidate count, to give networks having fewer candidates
    // the first opportunity to recover unused protected slots from the previous iteration.
    std::stable_sort(networks.begin(), networks.end(), [](Net a, Net b) { return a.count < b.count; });

    // Protect up to 25% of the eviction candidates by disadvantaged network.
    const size_t max_protect_by_network{total_protect_size / 2};
    size_t num_protected{0};

    while (num_protected < max_protect_by_network) {
        // Count the number of disadvantaged networks from which we have peers to protect.
        auto num_networks = std::count_if(networks.begin(), networks.end(), [](const Net& n) { return n.count; });
        if (num_networks == 0) {
            break;
        }
        const size_t disadvantaged_to_protect{max_protect_by_network - num_protected};
        const size_t protect_per_network{std::max(disadvantaged_to_protect / num_networks, static_cast<size_t>(1))};
        // Early exit flag if there are no remaining candidates by disadvantaged network.
        bool protected_at_least_one{false};

        for (Net& n : networks) {
            if (n.count == 0) continue;
            const size_t before = eviction_candidates.size();
            EraseLastKElements(eviction_candidates, CompareNodeNetworkTime(n.is_local, n.id),
                               protect_per_network, [&n](const NodeEvictionCandidate& c) {
                                   return n.is_local ? c.m_is_local : c.m_network == n.id;
                               });
            const size_t after = eviction_candidates.size();
            if (before > after) {
                protected_at_least_one = true;
                const size_t delta{before - after};
                num_protected += delta;
                if (num_protected >= max_protect_by_network) {
                    break;
                }
                n.count -= delta;
            }
        }
        if (!protected_at_least_one) {
            break;
        }
    }

    // Calculate how many we removed, and update our total number of peers that
    // we want to protect based on uptime accordingly.
    assert(num_protected == initial_size - eviction_candidates.size());
    const size_t remaining_to_protect{total_protect_size - num_protected};
    EraseLastKElements(eviction_candidates, ReverseCompareNodeTimeConnected, remaining_to_protect);
}

[[nodiscard]] std::optional<NodeId> SelectNodeToEvict(std::vector<NodeEvictionCandidate>&& vEvictionCandidates)
{
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
    // Protect up to 8 non-tx-relay peers that have sent us novel blocks.
    EraseLastKElements(vEvictionCandidates, CompareNodeBlockRelayOnlyTime, 8,
                       [](const NodeEvictionCandidate& n) { return !n.m_relay_txs && n.fRelevantServices; });

    // Protect 4 nodes that most recently sent us novel blocks.
    // An attacker cannot manipulate this metric without performing useful work.
    EraseLastKElements(vEvictionCandidates, CompareNodeBlockTime, 4);

    // Protect some of the remaining eviction candidates by ratios of desirable
    // or disadvantaged characteristics.
    ProtectEvictionCandidatesByRatio(vEvictionCandidates);

    if (vEvictionCandidates.empty()) return std::nullopt;

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
    std::chrono::seconds nMostConnectionsTime{0};
    std::map<uint64_t, std::vector<NodeEvictionCandidate> > mapNetGroupNodes;
    for (const NodeEvictionCandidate &node : vEvictionCandidates) {
        std::vector<NodeEvictionCandidate> &group = mapNetGroupNodes[node.nKeyedNetGroup];
        group.push_back(node);
        const auto grouptime{group[0].m_connected};

        if (group.size() > nMostConnections || (group.size() == nMostConnections && grouptime > nMostConnectionsTime)) {
            nMostConnections = group.size();
            nMostConnectionsTime = grouptime;
            naMostConnections = node.nKeyedNetGroup;
        }
    }

    // Reduce to the network group with the most connections
    vEvictionCandidates = std::move(mapNetGroupNodes[naMostConnections]);

    // Disconnect from the network group with the most connections
    return vEvictionCandidates.front().id;
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
        LOCK(m_nodes_mutex);

        for (const CNode* node : m_nodes) {
            if (node->HasPermission(NetPermissionFlags::NoBan))
                continue;
            if (!node->IsInboundConn())
                continue;
            if (node->fDisconnect)
                continue;

            if (fMasternodeMode) {
                // This handles eviction protected nodes. Nodes are always protected for a short time after the connection
                // was accepted. This short time is meant for the VERSION/VERACK exchange and the possible MNAUTH that might
                // follow when the incoming connection is from another masternode. When a message other than MNAUTH
                // is received after VERSION/VERACK, the protection is lifted immediately.
                bool isProtected = GetTime<std::chrono::seconds>() - node->m_connected < INBOUND_EVICTION_PROTECTION_TIME;
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

            NodeEvictionCandidate candidate = {node->GetId(), node->m_connected, node->m_min_ping_time,
                                               node->m_last_block_time, node->m_last_tx_time,
                                               HasAllDesirableServiceFlags(node->nServices),
                                               node->m_relays_txs.load(), node->m_bloom_filter_loaded.load(),
                                               node->nKeyedNetGroup, node->m_prefer_evict, node->addr.IsLocal(),
                                               node->ConnectedThroughNetwork()};
            vEvictionCandidates.push_back(candidate);
        }
    }
    const std::optional<NodeId> node_id_to_evict = SelectNodeToEvict(std::move(vEvictionCandidates));
    if (!node_id_to_evict) {
        return false;
    }
    LOCK(m_nodes_mutex);
    for (CNode* pnode : m_nodes) {
        if (pnode->GetId() == *node_id_to_evict) {
            LogPrint(BCLog::NET_NETCONN, "selected %s connection for eviction peer=%d; disconnecting\n", pnode->ConnectionTypeAsString(), pnode->GetId());
            pnode->fDisconnect = true;
            return true;
        }
    }
    return false;
}

void CConnman::AcceptConnection(const ListenSocket& hListenSocket, CMasternodeSync& mn_sync) {
    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);
    auto sock = hListenSocket.sock->Accept((struct sockaddr*)&sockaddr, &len);
    CAddress addr;

    if (!sock) {
        const int nErr = WSAGetLastError();
        if (nErr != WSAEWOULDBLOCK) {
            LogPrintf("socket error accept failed: %s\n", NetworkErrorString(nErr));
        }
        return;
    }

    if (!addr.SetSockAddr((const struct sockaddr*)&sockaddr)) {
        LogPrintf("Warning: Unknown socket family\n");
    } else {
        addr = CAddress{MaybeFlipIPv6toCJDNS(addr), NODE_NONE};
    }

    const CAddress addr_bind{MaybeFlipIPv6toCJDNS(GetBindAddress(*sock)), NODE_NONE};

    NetPermissionFlags permissionFlags = NetPermissionFlags::None;
    hListenSocket.AddSocketPermissionFlags(permissionFlags);

    CreateNodeFromAcceptedSocket(std::move(sock), permissionFlags, addr_bind, addr, mn_sync);
}

void CConnman::CreateNodeFromAcceptedSocket(std::unique_ptr<Sock>&& sock,
                                            NetPermissionFlags permissionFlags,
                                            const CAddress& addr_bind,
                                            const CAddress& addr,
                                            CMasternodeSync& mn_sync)
{
    int nInbound = 0;
    int nVerifiedInboundMasternodes = 0;
    int nMaxInbound = nMaxConnections - m_max_outbound;

    AddWhitelistPermissionFlags(permissionFlags, addr);
    bool legacyWhitelisted = false;
    if (NetPermissions::HasFlag(permissionFlags, NetPermissionFlags::Implicit)) {
        NetPermissions::ClearFlag(permissionFlags, NetPermissionFlags::Implicit);
        if (gArgs.GetBoolArg("-whitelistforcerelay", DEFAULT_WHITELISTFORCERELAY)) NetPermissions::AddFlag(permissionFlags, NetPermissionFlags::ForceRelay);
        if (gArgs.GetBoolArg("-whitelistrelay", DEFAULT_WHITELISTRELAY)) NetPermissions::AddFlag(permissionFlags, NetPermissionFlags::Relay);
        NetPermissions::AddFlag(permissionFlags, NetPermissionFlags::Mempool);
        NetPermissions::AddFlag(permissionFlags, NetPermissionFlags::NoBan);
        legacyWhitelisted = true;
    }

    {
        LOCK(m_nodes_mutex);
        for (const CNode* pnode : m_nodes) {
            if (pnode->IsInboundConn()) {
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
        LogPrint(BCLog::NET_NETCONN, "%s: not accepting new connections\n", strDropped);
        return;
    }

    if (!IsSelectableSocket(sock->Get()))
    {
        LogPrintf("%s: non-selectable socket\n", strDropped);
        return;
    }

    // According to the internet TCP_NODELAY is not carried into accepted sockets
    // on all platforms.  Set it again here just to be sure.
    const int on{1};
    if (sock->SetSockOpt(IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) == SOCKET_ERROR) {
        LogPrint(BCLog::NET, "connection from %s: unable to set TCP_NODELAY, continuing anyway\n",
                 addr.ToString());
    }

    // Don't accept connections from banned peers.
    bool banned = m_banman && m_banman->IsBanned(addr);
    if (!NetPermissions::HasFlag(permissionFlags, NetPermissionFlags::NoBan) && banned)
    {
        LogPrint(BCLog::NET, "%s (banned)\n", strDropped);
        return;
    }

    // Only accept connections from discouraged peers if our inbound slots aren't (almost) full.
    bool discouraged = m_banman && m_banman->IsDiscouraged(addr);
    if (!NetPermissions::HasFlag(permissionFlags, NetPermissionFlags::NoBan) && nInbound + 1 >= nMaxInbound && discouraged)
    {
        LogPrint(BCLog::NET, "connection from %s dropped (discouraged)\n", addr.ToString());
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
            return;
        }
        nInbound--;
    }

    // don't accept incoming connections until blockchain is synced
    if (fMasternodeMode && !mn_sync.IsBlockchainSynced()) {
        LogPrint(BCLog::NET, "AcceptConnection -- blockchain is not synced yet, skipping inbound connection attempt\n");
        return;
    }

    NodeId id = GetNewNodeId();
    uint64_t nonce = GetDeterministicRandomizer(RANDOMIZER_ID_LOCALHOSTNONCE).Write(id).Finalize();

    ServiceFlags nodeServices = nLocalServices;
    if (NetPermissions::HasFlag(permissionFlags, NetPermissionFlags::BloomFilter)) {
        nodeServices = static_cast<ServiceFlags>(nodeServices | NODE_BLOOM);
    }

    const bool inbound_onion = std::find(m_onion_binds.begin(), m_onion_binds.end(), addr_bind) != m_onion_binds.end();
    CNode* pnode = new CNode(id,
                             nodeServices,
                             std::move(sock),
                             addr,
                             CalculateKeyedNetGroup(addr),
                             nonce,
                             addr_bind,
                             /*addrNameIn=*/"",
                             ConnectionType::INBOUND,
                             inbound_onion);
    pnode->AddRef();
    pnode->m_permissionFlags = permissionFlags;
    // If this flag is present, the user probably expect that RPC and QT report it as whitelisted (backward compatibility)
    pnode->m_legacyWhitelisted = legacyWhitelisted;
    pnode->m_prefer_evict = discouraged;
    m_msgproc->InitializeNode(pnode);

    {
        LOCK(pnode->m_sock_mutex);
        if (fLogIPs) {
            LogPrint(BCLog::NET_NETCONN, "connection from %s accepted, sock=%d, peer=%d\n", addr.ToString(), pnode->m_sock->Get(), pnode->GetId());
        } else {
            LogPrint(BCLog::NET_NETCONN, "connection accepted, sock=%d, peer=%d\n", pnode->m_sock->Get(), pnode->GetId());
        }
    }

    {
        LOCK(m_nodes_mutex);
        m_nodes.push_back(pnode);
    }
    {
        LOCK2(cs_mapSocketToNode, pnode->m_sock_mutex);
        mapSocketToNode.emplace(pnode->m_sock->Get(), pnode);
        if (m_edge_trig_events) {
            if (!m_edge_trig_events->RegisterEvents(pnode->m_sock->Get())) {
                LogPrint(BCLog::NET, "EdgeTriggeredEvents::RegisterEvents() failed\n");
            }
        }
        if (m_wakeup_pipe) {
            m_wakeup_pipe->Write();
        }
    }

    // We received a new connection, harvest entropy from the time (and our peer count)
    RandAddEvent((uint32_t)id);
}

bool CConnman::AddConnection(const std::string& address, ConnectionType conn_type)
{
    AssertLockNotHeld(m_unused_i2p_sessions_mutex);
    std::optional<int> max_connections;
    switch (conn_type) {
    case ConnectionType::INBOUND:
    case ConnectionType::MANUAL:
    case ConnectionType::FEELER:
        return false;
    case ConnectionType::OUTBOUND_FULL_RELAY:
        max_connections = m_max_outbound_full_relay;
        break;
    case ConnectionType::BLOCK_RELAY:
        max_connections = m_max_outbound_block_relay;
        break;
    // no limit for ADDR_FETCH because -seednode has no limit either
    case ConnectionType::ADDR_FETCH:
        break;
    } // no default case, so the compiler can warn about missing cases

    // Count existing connections
    int existing_connections = WITH_LOCK(m_nodes_mutex,
                                         return std::count_if(m_nodes.begin(), m_nodes.end(), [conn_type](CNode* node) { return node->m_conn_type == conn_type; }););

    // Max connections of specified type already exist
    if (max_connections != std::nullopt && existing_connections >= max_connections) return false;

    // Max total outbound connections already exist
    CSemaphoreGrant grant(*semOutbound, true);
    if (!grant) return false;

    OpenNetworkConnection(CAddress(), false, &grant, address.c_str(), conn_type);
    return true;
}

void CConnman::DisconnectNodes()
{
    {
        LOCK(m_nodes_mutex);

        if (!fNetworkActive) {
            // Disconnect any connected nodes
            for (CNode* pnode : m_nodes) {
                if (!pnode->fDisconnect) {
                    LogPrint(BCLog::NET, "Network not active, dropping peer=%d\n", pnode->GetId());
                    pnode->fDisconnect = true;
                }
            }
        }

        // Disconnect unused nodes
        for (auto it = m_nodes.begin(); it != m_nodes.end(); )
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
                            LOCK(pnode->m_sock_mutex);
                            if (pnode->m_sock) {
                                // Give the other side a chance to detect the disconnect as early as possible (recv() will return 0)
                                ::shutdown(pnode->m_sock->Get(), SD_SEND);
                            }
                            pnode->fSocketShutdown = true;
                        }
                        ++it;
                        continue;
                    }
                }

                if (fLogIPs) {
                    LogPrintf("ThreadSocketHandler -- removing node: peer=%d addr=%s nRefCount=%d fInbound=%d m_masternode_connection=%d m_masternode_iqr_connection=%d\n",
                          pnode->GetId(), pnode->addr.ToString(), pnode->GetRefCount(), pnode->IsInboundConn(), pnode->m_masternode_connection, pnode->m_masternode_iqr_connection);
                } else {
                    LogPrintf("ThreadSocketHandler -- removing node: peer=%d nRefCount=%d fInbound=%d m_masternode_connection=%d m_masternode_iqr_connection=%d\n",
                          pnode->GetId(), pnode->GetRefCount(), pnode->IsInboundConn(), pnode->m_masternode_connection, pnode->m_masternode_iqr_connection);
                }

                // remove from m_nodes
                it = m_nodes.erase(it);

                // release outbound grant (if any)
                pnode->grantOutbound.Release();

                // close socket and cleanup
                pnode->CloseSocketDisconnect(this);

                // hold in disconnected pool until all refs are released
                pnode->Release();
                m_nodes_disconnected.push_back(pnode);
            } else {
                ++it;
            }
        }
    }
    {
        // Delete disconnected nodes
        std::list<CNode*> nodes_disconnected_copy = m_nodes_disconnected;
        for (auto it = m_nodes_disconnected.begin(); it != m_nodes_disconnected.end(); )
        {
            CNode* pnode = *it;
            // wait until threads are done using it
            bool fDelete = false;
            if (pnode->GetRefCount() <= 0) {
                {
                    TRY_LOCK(pnode->cs_vSend, lockSend);
                    if (lockSend) {
                        fDelete = true;
                    }
                }
                if (fDelete) {
                    it = m_nodes_disconnected.erase(it);
                    DeleteNode(pnode);
                }
            }
            if (!fDelete) {
                ++it;
            }
        }
    }
}

void CConnman::NotifyNumConnectionsChanged(CMasternodeSync& mn_sync)
{
    size_t nodes_size;
    {
        LOCK(m_nodes_mutex);
        nodes_size = m_nodes.size();
    }

    // If we had zero connections before and new connections now or if we just dropped
    // to zero connections reset the sync process if its outdated.
    if ((nodes_size > 0 && nPrevNodeCount == 0) || (nodes_size == 0 && nPrevNodeCount > 0)) {
        mn_sync.Reset();
    }

    if(nodes_size != nPrevNodeCount) {
        nPrevNodeCount = nodes_size;
        if(clientInterface)
            clientInterface->NotifyNumConnectionsChanged(nodes_size);

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
    mapMsgTypeSize mapRecvBytesMsgStats;
    mapMsgTypeSize mapSentBytesMsgStats;
    for (const std::string &msg : getAllNetMessageTypes()) {
        mapRecvBytesMsgStats[msg] = 0;
        mapSentBytesMsgStats[msg] = 0;
    }
    mapRecvBytesMsgStats[NET_MESSAGE_TYPE_OTHER] = 0;
    mapSentBytesMsgStats[NET_MESSAGE_TYPE_OTHER] = 0;
    const NodesSnapshot snap{*this, /* filter = */ CConnman::FullyConnectedOnly};
    for (auto pnode : snap.Nodes()) {
        {
            LOCK(pnode->cs_vRecv);
            for (const mapMsgTypeSize::value_type &i : pnode->mapRecvBytesPerMsgType)
                mapRecvBytesMsgStats[i.first] += i.second;
        }
        {
            LOCK(pnode->cs_vSend);
            for (const mapMsgTypeSize::value_type &i : pnode->mapSendBytesPerMsgType)
                mapSentBytesMsgStats[i.first] += i.second;
        }
        if(pnode->fClient)
            spvNodes++;
        else
            fullNodes++;
        if(pnode->IsInboundConn())
            inboundNodes++;
        else
            outboundNodes++;
        if(pnode->addr.IsIPv4())
            ipv4Nodes++;
        if(pnode->addr.IsIPv6())
            ipv6Nodes++;
        if(pnode->addr.IsTor())
            torNodes++;
        const auto last_ping_time = count_microseconds(pnode->m_last_ping_time);
        if (last_ping_time > 0)
            statsClient.timing("peers.ping_us", last_ping_time, 1.0f);
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

bool CConnman::ShouldRunInactivityChecks(const CNode& node, std::chrono::seconds now) const
{
    return node.m_connected + m_peer_connect_timeout < now;
}

bool CConnman::InactivityCheck(const CNode& node) const
{
    // Tests that see disconnects after using mocktime can start nodes with a
    // large timeout. For example, -peertimeout=999999999.
    const auto now{GetTime<std::chrono::seconds>()};
    const auto last_send{node.m_last_send.load()};
    const auto last_recv{node.m_last_recv.load()};

    if (!ShouldRunInactivityChecks(node, now)) return false;

    if (last_recv.count() == 0 || last_send.count() == 0) {
        LogPrint(BCLog::NET, "socket no message in first %i seconds, %d %d peer=%d\n", count_seconds(m_peer_connect_timeout), last_recv.count() != 0, last_send.count() != 0, node.GetId());
        return true;
    }

    if (now > last_send + TIMEOUT_INTERVAL) {
        LogPrint(BCLog::NET, "socket sending timeout: %is peer=%d\n", count_seconds(now - last_send), node.GetId());
        return true;
    }

    if (now > last_recv + TIMEOUT_INTERVAL) {
        LogPrint(BCLog::NET, "socket receive timeout: %is peer=%d\n", count_seconds(now - last_recv), node.GetId());
        return true;
    }

    if (!node.fSuccessfullyConnected) {
        LogPrint(BCLog::NET, "version handshake timeout peer=%d\n", node.GetId());
        return true;
    }

    return false;
}

bool CConnman::GenerateSelectSet(const std::vector<CNode*>& nodes,
                                 std::set<SOCKET>& recv_set,
                                 std::set<SOCKET>& send_set,
                                 std::set<SOCKET>& error_set)
{
    for (const ListenSocket& hListenSocket : vhListenSocket) {
        recv_set.insert(hListenSocket.sock->Get());
    }

    for (CNode* pnode : nodes)
    {
        bool select_recv = !pnode->fHasRecvData;
        bool select_send = !pnode->fCanSendData;

        LOCK(pnode->m_sock_mutex);
        if (!pnode->m_sock) {
            continue;
        }

        error_set.insert(pnode->m_sock->Get());
        if (select_send) {
            send_set.insert(pnode->m_sock->Get());
        }
        if (select_recv) {
            recv_set.insert(pnode->m_sock->Get());
        }
    }

    if (m_wakeup_pipe) {
        // We add a pipe to the read set so that the select() call can be woken up from the outside
        // This is done when data is added to send buffers (vSendMsg) or when new peers are added
        // This is currently only implemented for POSIX compliant systems. This means that Windows will fall back to
        // timing out after 50ms and then trying to send. This is ok as we assume that heavy-load daemons are usually
        // run on Linux and friends.
        recv_set.insert(m_wakeup_pipe->m_pipe[0]);
    }

    return !recv_set.empty() || !send_set.empty() || !error_set.empty();
}

#ifdef USE_KQUEUE
void CConnman::SocketEventsKqueue(std::set<SOCKET>& recv_set,
                                  std::set<SOCKET>& send_set,
                                  std::set<SOCKET>& error_set,
                                  bool only_poll)
{
    const size_t maxEvents = 64;
    struct kevent events[maxEvents];

    struct timespec timeout;
    timeout.tv_sec = only_poll ? 0 : SELECT_TIMEOUT_MILLISECONDS / 1000;
    timeout.tv_nsec = (only_poll ? 0 : SELECT_TIMEOUT_MILLISECONDS % 1000) * 1000 * 1000;

    int n{-1};
    ToggleWakeupPipe([&](){n = kevent(Assert(m_edge_trig_events)->GetFileDescriptor(), nullptr, 0, events, maxEvents, &timeout);});
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
void CConnman::SocketEventsEpoll(std::set<SOCKET>& recv_set,
                                 std::set<SOCKET>& send_set,
                                 std::set<SOCKET>& error_set,
                                 bool only_poll)
{
    const size_t maxEvents = 64;
    epoll_event events[maxEvents];

    int n{-1};
    ToggleWakeupPipe([&](){n = epoll_wait(Assert(m_edge_trig_events)->GetFileDescriptor(), events, maxEvents, only_poll ? 0 : SELECT_TIMEOUT_MILLISECONDS);});
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
void CConnman::SocketEventsPoll(const std::vector<CNode*>& nodes,
                                std::set<SOCKET>& recv_set,
                                std::set<SOCKET>& send_set,
                                std::set<SOCKET>& error_set,
                                bool only_poll)
{
    std::set<SOCKET> recv_select_set, send_select_set, error_select_set;
    if (!GenerateSelectSet(nodes, recv_select_set, send_select_set, error_select_set)) {
        if (!only_poll) interruptNet.sleep_for(std::chrono::milliseconds(SELECT_TIMEOUT_MILLISECONDS));
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

    int r{-1};
    ToggleWakeupPipe([&](){r = poll(vpollfds.data(), vpollfds.size(), only_poll ? 0 : SELECT_TIMEOUT_MILLISECONDS);});
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

void CConnman::SocketEventsSelect(const std::vector<CNode*>& nodes,
                                  std::set<SOCKET>& recv_set,
                                  std::set<SOCKET>& send_set,
                                  std::set<SOCKET>& error_set,
                                  bool only_poll)
{
    std::set<SOCKET> recv_select_set, send_select_set, error_select_set;
    if (!GenerateSelectSet(nodes, recv_select_set, send_select_set, error_select_set)) {
        interruptNet.sleep_for(std::chrono::milliseconds(SELECT_TIMEOUT_MILLISECONDS));
        return;
    }

    //
    // Find which sockets have data to receive
    //
    struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = only_poll ? 0 : SELECT_TIMEOUT_MILLISECONDS * 1000; // frequency to poll pnode->vSend

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

    int nSelect{-1};
    ToggleWakeupPipe([&](){nSelect = select(hSocketMax + 1, &fdsetRecv, &fdsetSend, &fdsetError, &timeout);});
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

void CConnman::SocketEvents(const std::vector<CNode*>& nodes,
                            std::set<SOCKET>& recv_set,
                            std::set<SOCKET>& send_set,
                            std::set<SOCKET>& error_set,
                            bool only_poll)
{
    switch (socketEventsMode) {
#ifdef USE_KQUEUE
        case SocketEventsMode::KQueue:
            SocketEventsKqueue(recv_set, send_set, error_set, only_poll);
            break;
#endif
#ifdef USE_EPOLL
        case SocketEventsMode::EPoll:
            SocketEventsEpoll(recv_set, send_set, error_set, only_poll);
            break;
#endif
#ifdef USE_POLL
        case SocketEventsMode::Poll:
            SocketEventsPoll(nodes, recv_set, send_set, error_set, only_poll);
            break;
#endif
        case SocketEventsMode::Select:
            SocketEventsSelect(nodes, recv_set, send_set, error_set, only_poll);
            break;
        default:
            assert(false);
    }
}

void CConnman::SocketHandler(CMasternodeSync& mn_sync)
{
    AssertLockNotHeld(m_total_bytes_sent_mutex);

    std::set<SOCKET> recv_set;
    std::set<SOCKET> send_set;
    std::set<SOCKET> error_set;

    bool only_poll = [this]() {
        // Check if we have work to do and thus should avoid waiting for events
        LOCK2(m_nodes_mutex, cs_sendable_receivable_nodes);
        if (!mapReceivableNodes.empty()) {
            return true;
        } else if (!mapSendableNodes.empty()) {
            if (LOCK(cs_mapNodesWithDataToSend); !mapNodesWithDataToSend.empty()) {
                // We must check if at least one of the nodes with pending messages is also
                // sendable, as otherwise a single node would be able to make the network
                // thread busy with polling.
                for (auto& p : mapNodesWithDataToSend) {
                    if (mapSendableNodes.count(p.first)) {
                        return true;
                        break;
                    }
                }
            }
        }
        return false;
    }();

    {
        const NodesSnapshot snap{*this, /* filter = */ CConnman::AllNodes, /* shuffle = */ false};

        // Check for the readiness of the already connected sockets and the
        // listening sockets in one call ("readiness" as in poll(2) or
        // select(2)). If none are ready, wait for a short while and return
        // empty sets.
        SocketEvents(snap.Nodes(), recv_set, send_set, error_set, only_poll);

    // Drain the wakeup pipe
    if (m_wakeup_pipe && recv_set.count(m_wakeup_pipe->m_pipe[0])) {
        m_wakeup_pipe->Drain();
    }

        // Service (send/receive) each of the already connected nodes.
        SocketHandlerConnected(recv_set, send_set, error_set);
    }

    // Accept new connections from listening sockets.
    SocketHandlerListening(recv_set, mn_sync);
}

void CConnman::SocketHandlerConnected(const std::set<SOCKET>& recv_set,
                                      const std::set<SOCKET>& send_set,
                                      const std::set<SOCKET>& error_set)
{
    AssertLockNotHeld(m_total_bytes_sent_mutex);

    if (interruptNet) return;

    std::vector<CNode*> vErrorNodes;
    std::vector<CNode*> vReceivableNodes;
    std::vector<CNode*> vSendableNodes;
    {
        LOCK(cs_mapSocketToNode);
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

            LOCK(cs_sendable_receivable_nodes);
            auto jt = mapReceivableNodes.emplace(it->second->GetId(), it->second);
            assert(jt.first->second == it->second);
            it->second->fHasRecvData = true;
        }
        for (auto hSocket : send_set) {
            auto it = mapSocketToNode.find(hSocket);
            if (it == mapSocketToNode.end()) {
                continue;
            }

            LOCK(cs_sendable_receivable_nodes);
            auto jt = mapSendableNodes.emplace(it->second->GetId(), it->second);
            assert(jt.first->second == it->second);
            it->second->fCanSendData = true;
        }

        // collect nodes that have a receivable socket
        // also clean up mapReceivableNodes from nodes that were receivable in the last iteration but aren't anymore
        {
            LOCK(cs_sendable_receivable_nodes);

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
        size_t bytes_sent = WITH_LOCK(pnode->cs_vSend, return SocketSendData(*pnode));
        if (bytes_sent) RecordBytesSent(bytes_sent);
    }

    for (auto& node : vErrorNodes) {
        node->Release();
    }
    for (auto& node : vReceivableNodes) {
        node->Release();
    }
    for (auto& node : vSendableNodes) {
        node->Release();
    }

    if (interruptNet) {
        return;
    }

    {
        LOCK(cs_sendable_receivable_nodes);
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

void CConnman::SocketHandlerListening(const std::set<SOCKET>& recv_set, CMasternodeSync& mn_sync)
{
    for (const ListenSocket& listen_socket : vhListenSocket) {
        if (interruptNet) {
            return;
        }
        if (recv_set.count(listen_socket.sock->Get()) > 0) {
            AcceptConnection(listen_socket, mn_sync);
        }
    }
}

size_t CConnman::SocketRecvData(CNode *pnode)
{
    // typical socket buffer is 8K-64K
    uint8_t pchBuf[0x10000];
    int nBytes = 0;
    {
        LOCK(pnode->m_sock_mutex);
        if (!pnode->m_sock)
            return 0;
        nBytes = recv(pnode->m_sock->Get(), (char*)pchBuf, sizeof(pchBuf), MSG_DONTWAIT);
        if (nBytes < (int)sizeof(pchBuf)) {
            pnode->fHasRecvData = false;
        }
    }
    if (nBytes > 0)
    {
        bool notify = false;
        if (!pnode->ReceiveMsgBytes(Span<const uint8_t>(pchBuf, nBytes), notify)) {
            LOCK(m_nodes_mutex);
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
        LOCK(m_nodes_mutex);
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
            LOCK(m_nodes_mutex);
            pnode->fOtherSideDisconnected = true; // avoid lingering
            pnode->CloseSocketDisconnect(this);
        }
    }
    if (nBytes < 0) {
        return 0;
    }
    return (size_t)nBytes;
}

void CConnman::ThreadSocketHandler(CMasternodeSync& mn_sync)
{
    AssertLockNotHeld(m_total_bytes_sent_mutex);

    int64_t nLastCleanupNodes = 0;

    while (!interruptNet)
    {
        // Handle sockets before we do the next round of disconnects. This allows us to flush send buffers one last time
        // before actually closing sockets. Receiving is however skipped in case a peer is pending to be disconnected
        SocketHandler(mn_sync);
        if (GetTimeMillis() - nLastCleanupNodes > 1000) {
            ForEachNode(AllNodes, [&](CNode* pnode) {
                if (InactivityCheck(*pnode)) pnode->fDisconnect = true;
            });
            nLastCleanupNodes = GetTimeMillis();
        }
        DisconnectNodes();
        NotifyNumConnectionsChanged(mn_sync);
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
                        LOCK(m_nodes_mutex);
                        for (const CNode* pnode : m_nodes) {
                            if (pnode->fSuccessfullyConnected && !pnode->IsFullOutboundConn() && !pnode->m_masternode_probe_connection) ++nRelevant;
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
            AddAddrFetch(seed);
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
                // instead using them as a addrfetch to get nodes with our desired service bits.
                AddAddrFetch(seed);
            }
        }
        --seeds_right_now;
    }
    LogPrintf("%d addresses found from DNS seeds\n", found);
}

void CConnman::DumpAddresses()
{
    int64_t nStart = GetTimeMillis();

    DumpPeerAddresses(::gArgs, addrman);

    LogPrint(BCLog::NET, "Flushed %d addresses to peers.dat  %dms\n",
           addrman.size(), GetTimeMillis() - nStart);
}

void CConnman::ProcessAddrFetch()
{
    AssertLockNotHeld(m_unused_i2p_sessions_mutex);
    std::string strDest;
    {
        LOCK(m_addr_fetches_mutex);
        if (m_addr_fetches.empty())
            return;
        strDest = m_addr_fetches.front();
        m_addr_fetches.pop_front();
    }
    CAddress addr;
    CSemaphoreGrant grant(*semOutbound, true);
    if (grant) {
        OpenNetworkConnection(addr, false, &grant, strDest.c_str(), ConnectionType::ADDR_FETCH);
    }
}

bool CConnman::GetTryNewOutboundPeer() const
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
// disconnected soon (eg ADDR_FETCH and FEELER)
// Also exclude peers that haven't finished initial connection handshake yet
// (so that we don't decide we're over our desired connection limit, and then
// evict some peer that has finished the handshake)
int CConnman::GetExtraFullOutboundCount() const
{
    int full_outbound_peers = 0;
    {
        LOCK(m_nodes_mutex);
        for (const CNode* pnode : m_nodes) {
            // don't count outbound masternodes
            if (pnode->m_masternode_connection) {
                continue;
            }
            if (pnode->fSuccessfullyConnected && !pnode->fDisconnect && pnode->IsFullOutboundConn() && !pnode->m_masternode_probe_connection) {
                ++full_outbound_peers;
            }
        }
    }
    return std::max(full_outbound_peers - m_max_outbound_full_relay, 0);
}

int CConnman::GetExtraBlockRelayCount() const
{
    int block_relay_peers = 0;
    {
        LOCK(m_nodes_mutex);
        for (const CNode* pnode : m_nodes) {
            if (pnode->fSuccessfullyConnected && !pnode->fDisconnect && pnode->IsBlockOnlyConn()) {
                ++block_relay_peers;
            }
        }
    }
    return std::max(block_relay_peers - m_max_outbound_block_relay, 0);
}

void CConnman::ThreadOpenConnections(const std::vector<std::string> connect, CDeterministicMNManager& dmnman)
{
    AssertLockNotHeld(m_unused_i2p_sessions_mutex);
    FastRandomContext rng;
    // Connect to specific addresses
    if (!connect.empty())
    {
        for (int64_t nLoop = 0;; nLoop++)
        {
            ProcessAddrFetch();
            for (const std::string& strAddr : connect)
            {
                CAddress addr(CService(), NODE_NONE);
                OpenNetworkConnection(addr, false, nullptr, strAddr.c_str(), ConnectionType::MANUAL);
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
    auto start = GetTime<std::chrono::microseconds>();

    // Minimum time before next feeler connection (in microseconds).
    auto next_feeler = PoissonNextSend(start, FEELER_INTERVAL);
    auto next_extra_block_relay = PoissonNextSend(start, EXTRA_BLOCK_RELAY_ONLY_PEER_INTERVAL);
    const bool dnsseed = gArgs.GetBoolArg("-dnsseed", DEFAULT_DNSSEED);
    bool add_fixed_seeds = gArgs.GetBoolArg("-fixedseeds", DEFAULT_FIXEDSEEDS);

    if (!add_fixed_seeds) {
        LogPrintf("Fixed seeds are disabled\n");
    }

    while (!interruptNet)
    {
        ProcessAddrFetch();

        if (!interruptNet.sleep_for(std::chrono::milliseconds(500)))
            return;

        CSemaphoreGrant grant(*semOutbound);
        if (interruptNet)
            return;

        if (add_fixed_seeds && addrman.size() == 0) {
            // When the node starts with an empty peers.dat, there are a few other sources of peers before
            // we fallback on to fixed seeds: -dnsseed, -seednode, -addnode
            // If none of those are available, we fallback on to fixed seeds immediately, else we allow
            // 60 seconds for any of those sources to populate addrman.
            bool add_fixed_seeds_now = false;
            // It is cheapest to check if enough time has passed first.
            if (GetTime<std::chrono::seconds>() > start + std::chrono::minutes{1}) {
                add_fixed_seeds_now = true;
                LogPrintf("Adding fixed seeds as 60 seconds have passed and addrman is empty\n");
            }

            // Checking !dnsseed is cheaper before locking 2 mutexes.
            if (!add_fixed_seeds_now && !dnsseed) {
                LOCK2(m_addr_fetches_mutex, m_added_nodes_mutex);
                if (m_addr_fetches.empty() && m_added_nodes.empty()) {
                    add_fixed_seeds_now = true;
                    LogPrintf("Adding fixed seeds as -dnsseed=0, -addnode is not provided and all -seednode(s) attempted\n");
                }
            }

            if (add_fixed_seeds_now) {
                CNetAddr local;
                local.SetInternal("fixedseeds");
                addrman.Add(ConvertSeeds(Params().FixedSeeds()), local);
                add_fixed_seeds = false;
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
            LOCK(m_nodes_mutex);
            for (const CNode* pnode : m_nodes) {
                if (pnode->IsFullOutboundConn() && !pnode->m_masternode_connection) nOutboundFullRelay++;
                if (pnode->IsBlockOnlyConn()) nOutboundBlockRelay++;

                // Netgroups for inbound and manual peers are not excluded because our goal here
                // is to not use multiple of our limited outbound slots on a single netgroup
                // but inbound and manual peers do not use our outbound slots. Inbound peers
                // also have the added issue that they could be attacker controlled and used
                // to prevent us from connecting to particular hosts if we used them here.
                switch (pnode->m_conn_type) {
                    case ConnectionType::INBOUND:
                    case ConnectionType::MANUAL:
                        break;
                    case ConnectionType::OUTBOUND_FULL_RELAY:
                    case ConnectionType::BLOCK_RELAY:
                    case ConnectionType::ADDR_FETCH:
                    case ConnectionType::FEELER:
                        setConnected.insert(pnode->addr.GetGroup(addrman.GetAsmap()));
                } // no default case, so the compiler can warn about missing cases
            }
        }

        std::set<uint256> setConnectedMasternodes;
        {
            LOCK(m_nodes_mutex);
            for (CNode* pnode : m_nodes) {
                auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
                if (!verifiedProRegTxHash.IsNull()) {
                    setConnectedMasternodes.emplace(verifiedProRegTxHash);
                }
            }
        }

        ConnectionType conn_type = ConnectionType::OUTBOUND_FULL_RELAY;
        auto now = GetTime<std::chrono::microseconds>();
        bool anchor = false;
        bool fFeeler = false;

        // Determine what type of connection to open. Opening
        // BLOCK_RELAY connections to addresses from anchors.dat gets the highest
        // priority. Then we open OUTBOUND_FULL_RELAY priority until we
        // meet our full-relay capacity. Then we open BLOCK_RELAY connection
        // until we hit our block-relay-only peer limit.
        // GetTryNewOutboundPeer() gets set when a stale tip is detected, so we
        // try opening an additional OUTBOUND_FULL_RELAY connection. If none of
        // these conditions are met, check to see if it's time to try an extra
        // block-relay-only peer (to confirm our tip is current, see below) or the next_feeler
        // timer to decide if we should open a FEELER.

        if (!m_anchors.empty() && (nOutboundBlockRelay < m_max_outbound_block_relay)) {
            conn_type = ConnectionType::BLOCK_RELAY;
            anchor = true;
        } else if (nOutboundFullRelay < m_max_outbound_full_relay) {
            // OUTBOUND_FULL_RELAY
        } else if (nOutboundBlockRelay < m_max_outbound_block_relay) {
            conn_type = ConnectionType::BLOCK_RELAY;
        } else if (GetTryNewOutboundPeer()) {
            // OUTBOUND_FULL_RELAY
        } else if (now > next_extra_block_relay && m_start_extra_block_relay_peers) {
            // Periodically connect to a peer (using regular outbound selection
            // methodology from addrman) and stay connected long enough to sync
            // headers, but not much else.
            //
            // Then disconnect the peer, if we haven't learned anything new.
            //
            // The idea is to make eclipse attacks very difficult to pull off,
            // because every few minutes we're finding a new peer to learn headers
            // from.
            //
            // This is similar to the logic for trying extra outbound (full-relay)
            // peers, except:
            // - we do this all the time on a poisson timer, rather than just when
            //   our tip is stale
            // - we potentially disconnect our next-youngest block-relay-only peer, if our
            //   newest block-relay-only peer delivers a block more recently.
            //   See the eviction logic in net_processing.cpp.
            //
            // Because we can promote these connections to block-relay-only
            // connections, they do not get their own ConnectionType enum
            // (similar to how we deal with extra outbound peers).
            next_extra_block_relay = PoissonNextSend(now, EXTRA_BLOCK_RELAY_ONLY_PEER_INTERVAL);
            conn_type = ConnectionType::BLOCK_RELAY;
        } else if (now > next_feeler) {
            next_feeler = PoissonNextSend(now, FEELER_INTERVAL);
            conn_type = ConnectionType::FEELER;
            fFeeler = true;
        } else {
            // skip to next iteration of while loop
            continue;
        }

        addrman.ResolveCollisions();

        auto mnList = dmnman.GetListAtChainTip();

        int64_t nANow = GetAdjustedTime();
        int nTries = 0;
        while (!interruptNet)
        {
            if (anchor && !m_anchors.empty()) {
                const CAddress addr = m_anchors.back();
                m_anchors.pop_back();
                if (!addr.IsValid() || IsLocal(addr) || !IsReachable(addr) ||
                    !HasAllDesirableServiceFlags(addr.nServices) ||
                    setConnected.count(addr.GetGroup(addrman.GetAsmap()))) continue;
                addrConnect = addr;
                LogPrint(BCLog::NET, "Trying to make an anchor connection to %s\n", addrConnect.ToString());
                break;
            }

            // If we didn't find an appropriate destination after trying 100 addresses fetched from addrman,
            // stop this loop, and let the outer loop run again (which sleeps, adds seed nodes, recalculates
            // already-connected network ranges, ...) before trying new addrman addresses.
            nTries++;
            if (nTries > 100)
                break;

            CAddress addr;
            int64_t addr_last_try{0};

            if (fFeeler) {
                // First, try to get a tried table collision address. This returns
                // an empty (invalid) address if there are no collisions to try.
                std::tie(addr, addr_last_try) = addrman.SelectTriedCollision();

                if (!addr.IsValid()) {
                    // No tried table collisions. Select a new table address
                    // for our feeler.
                    std::tie(addr, addr_last_try) = addrman.Select(true);
                } else if (AlreadyConnectedToAddress(addr)) {
                    // If test-before-evict logic would have us connect to a
                    // peer that we're already connected to, just mark that
                    // address as Good(). We won't be able to initiate the
                    // connection anyway, so this avoids inadvertently evicting
                    // a currently-connected peer.
                    addrman.Good(addr);
                    // Select a new table address for our feeler instead.
                    std::tie(addr, addr_last_try) = addrman.Select(true);
                }
            } else {
                // Not a feeler
                std::tie(addr, addr_last_try) = addrman.Select();
            }

            auto dmn = mnList.GetMNByService(addr);
            bool isMasternode = dmn != nullptr;

            // Require outbound connections, other than feelers, to be to distinct network groups
            if (!fFeeler && setConnected.count(addr.GetGroup(addrman.GetAsmap()))) {
                break;
            }

            // if we selected an invalid address, restart
            if (!addr.IsValid() || setConnected.count(addr.GetGroup(addrman.GetAsmap())))
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
            if (nANow - addr_last_try < 600 && nTries < 30)
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

            OpenNetworkConnection(addrConnect, (int)setConnected.size() >= std::min(nMaxConnections - 1, 2), &grant, nullptr, conn_type);
        }
    }
}

std::vector<CAddress> CConnman::GetCurrentBlockRelayOnlyConns() const
{
    std::vector<CAddress> ret;
    LOCK(m_nodes_mutex);
    for (const CNode* pnode : m_nodes) {
        if (pnode->IsBlockRelayOnly()) {
            ret.push_back(pnode->addr);
        }
    }

    return ret;
}

std::vector<AddedNodeInfo> CConnman::GetAddedNodeInfo() const
{
    std::vector<AddedNodeInfo> ret;

    std::list<std::string> lAddresses(0);
    {
        LOCK(m_added_nodes_mutex);
        ret.reserve(m_added_nodes.size());
        std::copy(m_added_nodes.cbegin(), m_added_nodes.cend(), std::back_inserter(lAddresses));
    }


    // Build a map of all already connected addresses (by IP:port and by name) to inbound/outbound and resolved CService
    std::map<CService, bool> mapConnected;
    std::map<std::string, std::pair<bool, CService>> mapConnectedByName;
    {
        LOCK(m_nodes_mutex);
        for (const CNode* pnode : m_nodes) {
            if (pnode->addr.IsValid()) {
                mapConnected[pnode->addr] = pnode->IsInboundConn();
            }
            std::string addrName{pnode->m_addr_name};
            if (!addrName.empty()) {
                mapConnectedByName[std::move(addrName)] = std::make_pair(pnode->IsInboundConn(), static_cast<const CService&>(pnode->addr));
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
    AssertLockNotHeld(m_unused_i2p_sessions_mutex);
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
                OpenNetworkConnection(addr, false, &grant, info.strAddedNode.c_str(), ConnectionType::MANUAL);
                if (!interruptNet.sleep_for(std::chrono::milliseconds(500)))
                    return;
            }
        }
        // Retry every 60 seconds if a connection was attempted, otherwise two seconds
        if (!interruptNet.sleep_for(std::chrono::seconds(tried ? 60 : 2)))
            return;
    }
}

void CConnman::ThreadOpenMasternodeConnections(CDeterministicMNManager& dmnman, CMasternodeMetaMan& mn_metaman,
                                               CMasternodeSync& mn_sync)
{
    // Connecting to specific addresses, no masternode connections available
    if (gArgs.IsArgSet("-connect") && gArgs.GetArgs("-connect").size() > 0)
        return;

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

        if (!fNetworkActive || !mn_sync.IsBlockchainSynced())
            continue;

        std::set<CService> connectedNodes;
        std::map<uint256 /*proTxHash*/, bool /*fInbound*/> connectedProRegTxHashes;
        ForEachNode([&](const CNode* pnode) {
            auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
            connectedNodes.emplace(pnode->addr);
            if (!verifiedProRegTxHash.IsNull()) {
                connectedProRegTxHashes.emplace(verifiedProRegTxHash, pnode->IsInboundConn());
            }
        });

        auto mnList = dmnman.GetListAtChainTip();

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
                            if (pnode->nTimeFirstMessageReceived != 0 && GetTimeSeconds() - pnode->nTimeFirstMessageReceived > 5) {
                                // clearly not expecting mnauth to take that long even if it wasn't the first message
                                // we received (as it should normally), disconnect
                                LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- dropping non-mnauth connection to %s, service=%s\n", _func_, proRegTxHash.ToString(), addr2.ToString());
                                pnode->fDisconnect = true;
                                return true;
                            }
                            return false;
                        });
                        // either way - it's not ready, skip it for now
                        continue;
                    }
                    if (!connectedNodes.count(addr2) && !IsMasternodeOrDisconnectRequested(addr2) && !connectedProRegTxHashes.count(proRegTxHash)) {
                        int64_t lastAttempt = mn_metaman.GetMetaInfo(dmn->proTxHash)->GetLastOutboundAttempt();
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
                    mn_metaman.GetMetaInfo(dmn->proTxHash)->SetLastOutboundSuccess(nANow);
                    it = masternodePendingProbes.erase(it);
                    continue;
                }

                ++it;

                int64_t lastAttempt = mn_metaman.GetMetaInfo(dmn->proTxHash)->GetLastOutboundAttempt();
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
            LOCK2(m_nodes_mutex, cs_vPendingMasternodes);

            if (!vPendingMasternodes.empty()) {
                auto dmn = mnList.GetValidMN(vPendingMasternodes.front());
                vPendingMasternodes.erase(vPendingMasternodes.begin());
                if (dmn && !connectedNodes.count(dmn->pdmnState->addr) && !IsMasternodeOrDisconnectRequested(dmn->pdmnState->addr)) {
                    LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- opening pending masternode connection to %s, service=%s\n", _func_, dmn->proTxHash.ToString(), dmn->pdmnState->addr.ToString());
                    return dmn;
                }
            }

            if (const auto pending = getPendingQuorumNodes(); !pending.empty()) {
                // not-null
                auto dmn = pending[GetRand(pending.size())];
                LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- opening quorum connection to %s, service=%s\n",
                         _func_, dmn->proTxHash.ToString(), dmn->pdmnState->addr.ToString());
                return dmn;
            }

            if (const auto pending = getPendingProbes(); !pending.empty()) {
                // not-null
                auto dmn = pending[GetRand(pending.size())];
                masternodePendingProbes.erase(dmn->proTxHash);
                isProbe = MasternodeProbeConn::IsConnection;

                LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- probing masternode %s, service=%s\n", _func_, dmn->proTxHash.ToString(), dmn->pdmnState->addr.ToString());
                return dmn;
            }
            return nullptr;
        };

        CDeterministicMNCPtr connectToDmn = getConnectToDmn();

        if (connectToDmn == nullptr) {
            continue;
        }

        didConnect = true;

        mn_metaman.GetMetaInfo(connectToDmn->proTxHash)->SetLastOutboundAttempt(nANow);

        OpenMasternodeConnection(CAddress(connectToDmn->pdmnState->addr, NODE_NETWORK), isProbe);
        // should be in the list now if connection was opened
        bool connected = ForNode(connectToDmn->pdmnState->addr, CConnman::AllNodes, [&](CNode* pnode) {
            if (pnode->fDisconnect) {
                return false;
            }
            return true;
        });
        if (!connected) {
            LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- connection failed for masternode  %s, service=%s\n", __func__, connectToDmn->proTxHash.ToString(), connectToDmn->pdmnState->addr.ToString());
            // Will take a few consequent failed attempts to PoSe-punish a MN.
            if (mn_metaman.GetMetaInfo(connectToDmn->proTxHash)->OutboundFailedTooManyTimes()) {
                LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- failed to connect to masternode %s too many times\n", __func__, connectToDmn->proTxHash.ToString());
            }
        }
    }
}

// if successful, this moves the passed grant to the constructed node
void CConnman::OpenNetworkConnection(const CAddress& addrConnect, bool fCountFailure, CSemaphoreGrant *grantOutbound, const char *pszDest, ConnectionType conn_type, MasternodeConn masternode_connection, MasternodeProbeConn masternode_probe_connection)
{
    AssertLockNotHeld(m_unused_i2p_sessions_mutex);
    assert(conn_type != ConnectionType::INBOUND);

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
            return addrConnect.ToString();
        } else {
            return std::string("new peer");
        }
    };

    if (!pszDest) {
        // banned, discouraged or exact match?
        if ((m_banman && (m_banman->IsDiscouraged(addrConnect) || m_banman->IsBanned(addrConnect))) || AlreadyConnectedToAddress(addrConnect))
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
    CNode* pnode = ConnectNode(addrConnect, pszDest, fCountFailure, conn_type);

    if (!pnode) {
        LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- ConnectNode failed for %s\n", __func__, getIpStr());
        return;
    }

    {
        LOCK(pnode->m_sock_mutex);
        LogPrint(BCLog::NET_NETCONN, "CConnman::%s -- successfully connected to %s, sock=%d, peer=%d\n", __func__, getIpStr(), pnode->m_sock->Get(), pnode->GetId());
    }

    if (grantOutbound)
        grantOutbound->MoveTo(pnode->grantOutbound);

    if (masternode_connection == MasternodeConn::IsConnection)
        pnode->m_masternode_connection = true;
    if (masternode_probe_connection == MasternodeProbeConn::IsConnection)
        pnode->m_masternode_probe_connection = true;

    {
        LOCK2(cs_mapSocketToNode, pnode->m_sock_mutex);
        mapSocketToNode.emplace(pnode->m_sock->Get(), pnode);
    }

    m_msgproc->InitializeNode(pnode);
    {
        LOCK(m_nodes_mutex);
        m_nodes.push_back(pnode);
    }
    {
        if (m_edge_trig_events) {
            LOCK(pnode->m_sock_mutex);
            if (!m_edge_trig_events->RegisterEvents(pnode->m_sock->Get())) {
                LogPrint(BCLog::NET, "EdgeTriggeredEvents::RegisterEvents() failed\n");
            }
        }
        if (m_wakeup_pipe) {
            m_wakeup_pipe->Write();
        }
    }
}

void CConnman::OpenMasternodeConnection(const CAddress &addrConnect, MasternodeProbeConn probe) {
    OpenNetworkConnection(addrConnect, false, nullptr, nullptr, ConnectionType::OUTBOUND_FULL_RELAY, MasternodeConn::IsConnection, probe);
}

void CConnman::ThreadMessageHandler()
{
    int64_t nLastSendMessagesTimeMasternodes = 0;

    FastRandomContext rng;
    while (!flagInterruptMsgProc)
    {
        bool fMoreWork = false;

        bool fSkipSendMessagesForMasternodes = true;
        if (GetTimeMillis() - nLastSendMessagesTimeMasternodes >= 100) {
            fSkipSendMessagesForMasternodes = false;
            nLastSendMessagesTimeMasternodes = GetTimeMillis();
        }

        // Randomize the order in which we process messages from/to our peers.
        // This prevents attacks in which an attacker exploits having multiple
        // consecutive connections in the m_nodes list.
        const NodesSnapshot snap{*this, /* filter = */ CConnman::AllNodes, /* shuffle = */ true};

        for (CNode* pnode : snap.Nodes()) {
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

        WAIT_LOCK(mutexMsgProc, lock);
        if (!fMoreWork) {
            condMsgProc.wait_until(lock, std::chrono::steady_clock::now() + std::chrono::milliseconds(100), [this]() EXCLUSIVE_LOCKS_REQUIRED(mutexMsgProc) { return fMsgProcWake; });
        }
        fMsgProcWake = false;
    }
}

void CConnman::ThreadI2PAcceptIncoming(CMasternodeSync& mn_sync)
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

        CreateNodeFromAcceptedSocket(std::move(conn.sock), NetPermissionFlags::None,
                                     CAddress{conn.me, NODE_NONE}, CAddress{conn.peer, NODE_NONE}, mn_sync);
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
    if (sock->SetSockOpt(SOL_SOCKET, SO_REUSEADDR, (sockopt_arg_type)&nOne, sizeof(int)) == SOCKET_ERROR) {
        strError = strprintf(Untranslated("Error setting SO_REUSEADDR on socket: %s, continuing anyway"), NetworkErrorString(WSAGetLastError()));
        LogPrintf("%s\n", strError.original);
    }

    // some systems don't have IPV6_V6ONLY but are always v6only; others do have the option
    // and enable it by default or not. Try to enable it, if possible.
    if (addrBind.IsIPv6()) {
#ifdef IPV6_V6ONLY
        if (sock->SetSockOpt(IPPROTO_IPV6, IPV6_V6ONLY, (sockopt_arg_type)&nOne, sizeof(int)) == SOCKET_ERROR) {
            strError = strprintf(Untranslated("Error setting IPV6_V6ONLY on socket: %s, continuing anyway"), NetworkErrorString(WSAGetLastError()));
            LogPrintf("%s\n", strError.original);
        }
#endif
#ifdef WIN32
        int nProtLevel = PROTECTION_LEVEL_UNRESTRICTED;
        if (sock->SetSockOpt(IPPROTO_IPV6, IPV6_PROTECTION_LEVEL, (const char*)&nProtLevel, sizeof(int)) == SOCKET_ERROR) {
            strError = strprintf(Untranslated("Error setting IPV6_PROTECTION_LEVEL on socket: %s, continuing anyway"), NetworkErrorString(WSAGetLastError()));
            LogPrintf("%s\n", strError.original);
        }
#endif
    }

    if (sock->Bind(reinterpret_cast<struct sockaddr*>(&sockaddr), len) == SOCKET_ERROR) {
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
    if (sock->Listen(SOMAXCONN) == SOCKET_ERROR)
    {
        strError = strprintf(_("Error: Listening for incoming connections failed (listen returned error %s)"), NetworkErrorString(WSAGetLastError()));
        LogPrintf("%s\n", strError.original);
        return false;
    }

    if (m_edge_trig_events && !m_edge_trig_events->AddSocket(sock->Get())) {
        LogPrintf("Error: EdgeTriggeredEvents::AddSocket() failed\n");
        return false;
    }

    vhListenSocket.emplace_back(std::move(sock), permissions);

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

void CConnman::SetNetworkActive(bool active, CMasternodeSync* const mn_sync)
{
    LogPrintf("%s: %s\n", __func__, active);

    if (fNetworkActive == active) {
        return;
    }

    fNetworkActive = active;

    // mn_sync is nullptr during app initialization with `-networkactive=`
    if (mn_sync) {
        // Always call the Reset() if the network gets enabled/disabled to make sure the sync process
        // gets a reset if its outdated..
        mn_sync->Reset();
    }

    uiInterface.NotifyNetworkActiveChanged(fNetworkActive);
}

CConnman::CConnman(uint64_t nSeed0In, uint64_t nSeed1In, AddrMan& addrman_in, bool network_active) :
        addrman(addrman_in), nSeed0(nSeed0In), nSeed1(nSeed1In)
{
    SetTryNewOutboundPeer(false);

    Options connOptions;
    Init(connOptions);
    SetNetworkActive(network_active, /* mn_sync = */ nullptr);
}

NodeId CConnman::GetNewNodeId()
{
    return nLastNodeId.fetch_add(1, std::memory_order_relaxed);
}


bool CConnman::Bind(const CService& addr_, unsigned int flags, NetPermissionFlags permissions)
{
    const CService addr{MaybeFlipIPv6toCJDNS(addr_)};

    bilingual_str strError;
    if (!BindListenPort(addr, strError, permissions)) {
        if ((flags & BF_REPORT_ERROR) && clientInterface) {
            clientInterface->ThreadSafeMessageBox(strError, "", CClientUIInterface::MSG_ERROR);
        }
        return false;
    }

    if (addr.IsRoutable() && fDiscover && !(flags & BF_DONT_ADVERTISE) && !NetPermissions::HasFlag(permissions, NetPermissionFlags::NoBan)) {
        AddLocal(addr, LOCAL_BIND);
    }

    return true;
}

bool CConnman::InitBinds(const Options& options)
{
    bool fBound = false;
    for (const auto& addrBind : options.vBinds) {
        fBound |= Bind(addrBind, BF_REPORT_ERROR, NetPermissionFlags::None);
    }
    for (const auto& addrBind : options.vWhiteBinds) {
        fBound |= Bind(addrBind.m_service, BF_REPORT_ERROR, addrBind.m_flags);
    }
    for (const auto& addr_bind : options.onion_binds) {
        fBound |= Bind(addr_bind, BF_DONT_ADVERTISE, NetPermissionFlags::None);
    }
    if (options.bind_on_any) {
        struct in_addr inaddr_any;
        inaddr_any.s_addr = htonl(INADDR_ANY);
        struct in6_addr inaddr6_any = IN6ADDR_ANY_INIT;
        fBound |= Bind(CService(inaddr6_any, GetListenPort()), BF_NONE, NetPermissionFlags::None);
        fBound |= Bind(CService(inaddr_any, GetListenPort()), !fBound ? BF_REPORT_ERROR : BF_NONE, NetPermissionFlags::None);
    }
    return fBound;
}

bool CConnman::Start(CDeterministicMNManager& dmnman, CMasternodeMetaMan& mn_metaman, CMasternodeSync& mn_sync,
                     CScheduler& scheduler, const Options& connOptions)
{
    AssertLockNotHeld(m_total_bytes_sent_mutex);
    Init(connOptions);

    if (socketEventsMode == SocketEventsMode::EPoll || socketEventsMode == SocketEventsMode::KQueue) {
        m_edge_trig_events = std::make_unique<EdgeTriggeredEvents>(socketEventsMode);
        if (!m_edge_trig_events->IsValid()) {
            LogPrintf("Unable to initialize EdgeTriggeredEvents instance\n");
            m_edge_trig_events.reset();
            return false;
        }
    }

    if (fListen && !InitBinds(connOptions)) {
        if (clientInterface) {
            clientInterface->ThreadSafeMessageBox(
                _("Failed to listen on any port. Use -listen=0 if you want this."),
                "", CClientUIInterface::MSG_ERROR);
        }
        return false;
    }

    Proxy i2p_sam;
    if (GetProxy(NET_I2P, i2p_sam) && connOptions.m_i2p_accept_incoming) {
        m_i2p_sam_session = std::make_unique<i2p::sam::Session>(GetDataDir() / "i2p_private_key",
                                                                i2p_sam.proxy, &interruptNet);
    }

    for (const auto& strDest : connOptions.vSeedNodes) {
        AddAddrFetch(strDest);
    }

    if (m_use_addrman_outgoing) {
        // Load addresses from anchors.dat
        m_anchors = ReadAnchors(GetDataDir() / ANCHORS_DATABASE_FILENAME);
        if (m_anchors.size() > MAX_BLOCK_RELAY_ONLY_ANCHORS) {
            m_anchors.resize(MAX_BLOCK_RELAY_ONLY_ANCHORS);
        }
        LogPrintf("%i block-relay-only anchors will be tried for connections.\n", m_anchors.size());
    }

    uiInterface.InitMessage(_("Starting network threads…").translated);

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
    m_wakeup_pipe = std::make_unique<WakeupPipe>(m_edge_trig_events.get());
    if (!m_wakeup_pipe->IsValid()) {
        /* We log the error but do not halt initialization */
        LogPrintf("Unable to initialize WakeupPipe instance\n");
        m_wakeup_pipe.reset();
    }
#endif /* USE_WAKEUP_PIPE */

    // Send and receive from sockets, accept connections
    threadSocketHandler = std::thread(&util::TraceThread, "net", [this, &mn_sync] { ThreadSocketHandler(mn_sync); });

    if (!gArgs.GetBoolArg("-dnsseed", DEFAULT_DNSSEED))
        LogPrintf("DNS seeding disabled\n");
    else
        threadDNSAddressSeed = std::thread(&util::TraceThread, "dnsseed", [this] { ThreadDNSAddressSeed(); });

    // Initiate manual connections
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
            [this, connect = connOptions.m_specified_outgoing, &dmnman] { ThreadOpenConnections(connect, dmnman); });
    }

    // Initiate masternode connections
    threadOpenMasternodeConnections = std::thread(&util::TraceThread, "mncon", [this, &dmnman, &mn_metaman, &mn_sync] {
        ThreadOpenMasternodeConnections(dmnman, mn_metaman, mn_sync);
    });

    // Process messages
    threadMessageHandler = std::thread(&util::TraceThread, "msghand", [this] { ThreadMessageHandler(); });

    if (m_i2p_sam_session) {
        threadI2PAcceptIncoming =
            std::thread(&util::TraceThread, "i2paccept", [this, &mn_sync] { ThreadI2PAcceptIncoming(mn_sync); });
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
        LOCK(m_nodes_mutex);

        // Close sockets
        for (CNode *pnode : m_nodes)
            pnode->CloseSocketDisconnect(this);
    }
    for (ListenSocket& hListenSocket : vhListenSocket) {
        if (hListenSocket.sock) {
            if (m_edge_trig_events && !m_edge_trig_events->RemoveSocket(hListenSocket.sock->Get())) {
                LogPrintf("EdgeTriggeredEvents::RemoveSocket() failed\n");
            }
        }
    }

    // clean up some globals (to help leak detection)
    std::vector<CNode*> nodes;
    WITH_LOCK(m_nodes_mutex, nodes.swap(m_nodes));
    for (CNode* pnode : nodes) {
        DeleteNode(pnode);
    }
    for (CNode* pnode : m_nodes_disconnected) {
        DeleteNode(pnode);
    }
    WITH_LOCK(cs_mapSocketToNode, mapSocketToNode.clear());
    {
        LOCK(cs_sendable_receivable_nodes);
        mapReceivableNodes.clear();
    }
    {
        LOCK(cs_mapNodesWithDataToSend);
        mapNodesWithDataToSend.clear();
    }
    m_nodes_disconnected.clear();
    vhListenSocket.clear();
    semOutbound.reset();
    semAddnode.reset();
    /**
     * m_wakeup_pipe must be reset *before* m_edge_trig_events as it may
     * attempt to call EdgeTriggeredEvents::UnregisterPipe() in its destructor
     */
    m_wakeup_pipe.reset();
    m_edge_trig_events.reset();
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

std::vector<CAddress> CConnman::GetAddresses(size_t max_addresses, size_t max_pct, std::optional<Network> network) const
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
        .Write(requestor.ConnectedThroughNetwork())
        .Write(local_socket_bytes.data(), local_socket_bytes.size())
        // For outbound connections, the port of the bound address is randomly
        // assigned by the OS and would therefore not be useful for seeding.
        .Write(requestor.IsInboundConn() ? requestor.addrBind.GetPort() : 0)
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
    LOCK(m_added_nodes_mutex);
    for (const std::string& it : m_added_nodes) {
        if (strNode == it) return false;
    }

    m_added_nodes.push_back(strNode);
    return true;
}

bool CConnman::RemoveAddedNode(const std::string& strNode)
{
    LOCK(m_added_nodes_mutex);
    for(std::vector<std::string>::iterator it = m_added_nodes.begin(); it != m_added_nodes.end(); ++it) {
        if (strNode == *it) {
            m_added_nodes.erase(it);
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
            const CNetMsgMaker msgMaker(pnode->GetCommonVersion());
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
    LOCK2(m_nodes_mutex, cs_vPendingMasternodes);
    auto it = masternodeQuorumNodes.find(std::make_pair(llmqType, quorumHash));
    if (it == masternodeQuorumNodes.end()) {
        return {};
    }
    const auto& proRegTxHashes = it->second;

    std::set<NodeId> nodes;
    for (const auto pnode : m_nodes) {
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

bool CConnman::IsMasternodeQuorumNode(const CNode* pnode, const CDeterministicMNList& tip_mn_list)
{
    // Let's see if this is an outgoing connection to an address that is known to be a masternode
    // We however only need to know this if the node did not authenticate itself as a MN yet
    uint256 assumedProTxHash;
    if (pnode->GetVerifiedProRegTxHash().IsNull() && !pnode->IsInboundConn()) {
        auto dmn = tip_mn_list.GetMNByService(pnode->addr);
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

size_t CConnman::GetNodeCount(ConnectionDirection flags) const
{
    LOCK(m_nodes_mutex);

    int nNum = 0;
    for (const auto& pnode : m_nodes) {
        if (pnode->fDisconnect) {
            continue;
        }
        if ((flags & ConnectionDirection::Verified) && pnode->GetVerifiedProRegTxHash().IsNull()) {
            continue;
        }
        if (flags & (pnode->IsInboundConn() ? ConnectionDirection::In : ConnectionDirection::Out)) {
            nNum++;
        } else if (flags == ConnectionDirection::Verified) {
            nNum++;
        }
    }

    return nNum;
}

size_t CConnman::GetMaxOutboundNodeCount()
{
    return m_max_outbound;
}

void CConnman::GetNodeStats(std::vector<CNodeStats>& vstats) const
{
    vstats.clear();
    LOCK(m_nodes_mutex);
    vstats.reserve(m_nodes.size());
    for (CNode* pnode : m_nodes) {
        if (pnode->fDisconnect) {
            continue;
        }
        vstats.emplace_back();
        pnode->CopyStats(vstats.back());
        vstats.back().m_mapped_as = pnode->addr.GetMappedAS(addrman.GetAsmap());
    }
}

bool CConnman::DisconnectNode(const std::string& strNode)
{
    LOCK(m_nodes_mutex);
    if (CNode* pnode = FindNode(strNode)) {
        LogPrint(BCLog::NET_NETCONN, "disconnect by address%s matched peer=%d; disconnecting\n", (fLogIPs ? strprintf("=%s", strNode) : ""), pnode->GetId());
        pnode->fDisconnect = true;
        return true;
    }
    return false;
}

bool CConnman::DisconnectNode(const CSubNet& subnet)
{
    bool disconnected = false;
    LOCK(m_nodes_mutex);
    for (CNode* pnode : m_nodes) {
        if (subnet.Match(pnode->addr)) {
            LogPrint(BCLog::NET_NETCONN, "disconnect by subnet%s matched peer=%d; disconnecting\n", (fLogIPs ? strprintf("=%s", subnet.ToString()) : ""), pnode->GetId());
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
    LOCK(m_nodes_mutex);
    for(CNode* pnode : m_nodes) {
        if (id == pnode->GetId()) {
            LogPrint(BCLog::NET_NETCONN, "disconnect by id peer=%d; disconnecting\n", pnode->GetId());
            pnode->fDisconnect = true;
            return true;
        }
    }
    return false;
}

void CConnman::RecordBytesRecv(uint64_t bytes)
{
    nTotalBytesRecv += bytes;
    statsClient.count("bandwidth.bytesReceived", bytes, 0.1f);
    statsClient.gauge("bandwidth.totalBytesReceived", nTotalBytesRecv, 0.01f);
}

void CConnman::RecordBytesSent(uint64_t bytes)
{
    AssertLockNotHeld(m_total_bytes_sent_mutex);
    LOCK(m_total_bytes_sent_mutex);

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

uint64_t CConnman::GetMaxOutboundTarget() const
{
    AssertLockNotHeld(m_total_bytes_sent_mutex);
    LOCK(m_total_bytes_sent_mutex);
    return nMaxOutboundLimit;
}

std::chrono::seconds CConnman::GetMaxOutboundTimeframe() const
{
    return MAX_UPLOAD_TIMEFRAME;
}

std::chrono::seconds CConnman::GetMaxOutboundTimeLeftInCycle() const
{
    AssertLockNotHeld(m_total_bytes_sent_mutex);
    LOCK(m_total_bytes_sent_mutex);
    return GetMaxOutboundTimeLeftInCycle_();
}

std::chrono::seconds CConnman::GetMaxOutboundTimeLeftInCycle_() const
{
    AssertLockHeld(m_total_bytes_sent_mutex);

    if (nMaxOutboundLimit == 0)
        return 0s;

    if (nMaxOutboundCycleStartTime.count() == 0)
        return MAX_UPLOAD_TIMEFRAME;

    const std::chrono::seconds cycleEndTime = nMaxOutboundCycleStartTime + MAX_UPLOAD_TIMEFRAME;
    const auto now = GetTime<std::chrono::seconds>();
    return (cycleEndTime < now) ? 0s : cycleEndTime - now;
}

bool CConnman::OutboundTargetReached(bool historicalBlockServingLimit) const
{
    AssertLockNotHeld(m_total_bytes_sent_mutex);
    LOCK(m_total_bytes_sent_mutex);
    if (nMaxOutboundLimit == 0)
        return false;

    if (historicalBlockServingLimit)
    {
        // keep a large enough buffer to at least relay each block once
        const std::chrono::seconds timeLeftInCycle = GetMaxOutboundTimeLeftInCycle_();
        const uint64_t buffer = timeLeftInCycle / std::chrono::minutes{10} * MaxBlockSize(fDIP0001ActiveAtTip);
        if (buffer >= nMaxOutboundLimit || nMaxOutboundTotalBytesSentInCycle >= nMaxOutboundLimit - buffer)
            return true;
    }
    else if (nMaxOutboundTotalBytesSentInCycle >= nMaxOutboundLimit)
        return true;

    return false;
}

uint64_t CConnman::GetOutboundTargetBytesLeft() const
{
    AssertLockNotHeld(m_total_bytes_sent_mutex);
    LOCK(m_total_bytes_sent_mutex);
    if (nMaxOutboundLimit == 0)
        return 0;

    return (nMaxOutboundTotalBytesSentInCycle >= nMaxOutboundLimit) ? 0 : nMaxOutboundLimit - nMaxOutboundTotalBytesSentInCycle;
}

uint64_t CConnman::GetTotalBytesRecv() const
{
    return nTotalBytesRecv;
}

uint64_t CConnman::GetTotalBytesSent() const
{
    AssertLockNotHeld(m_total_bytes_sent_mutex);
    LOCK(m_total_bytes_sent_mutex);
    return nTotalBytesSent;
}

ServiceFlags CConnman::GetLocalServices() const
{
    return nLocalServices;
}

unsigned int CConnman::GetReceiveFloodSize() const { return nReceiveFloodSize; }

CNode::CNode(NodeId idIn,
             ServiceFlags nLocalServicesIn,
             std::shared_ptr<Sock> sock,
             const CAddress& addrIn,
             uint64_t nKeyedNetGroupIn,
             uint64_t nLocalHostNonceIn,
             const CAddress& addrBindIn,
             const std::string& addrNameIn,
             ConnectionType conn_type_in,
             bool inbound_onion,
             std::unique_ptr<i2p::sam::Session>&& i2p_sam_session)
    : m_deserializer{std::make_unique<V1TransportDeserializer>(V1TransportDeserializer(Params(), idIn, SER_NETWORK, INIT_PROTO_VERSION))},
      m_serializer{std::make_unique<V1TransportSerializer>(V1TransportSerializer())},
      m_sock{sock},
      m_connected{GetTime<std::chrono::seconds>()},
      addr{addrIn},
      addrBind{addrBindIn},
      m_addr_name{addrNameIn.empty() ? addr.ToStringIPPort() : addrNameIn},
      m_inbound_onion{inbound_onion},
      nKeyedNetGroup{nKeyedNetGroupIn},
      id{idIn},
      nLocalHostNonce{nLocalHostNonceIn},
      m_conn_type{conn_type_in},
      nLocalServices{nLocalServicesIn},
      m_i2p_sam_session{std::move(i2p_sam_session)}
{
    if (inbound_onion) assert(conn_type_in == ConnectionType::INBOUND);

    for (const std::string &msg : getAllNetMessageTypes())
        mapRecvBytesPerMsgType[msg] = 0;
    mapRecvBytesPerMsgType[NET_MESSAGE_TYPE_OTHER] = 0;

    if (fLogIPs) {
        LogPrint(BCLog::NET, "Added connection to %s peer=%d\n", m_addr_name, id);
    } else {
        LogPrint(BCLog::NET, "Added connection peer=%d\n", id);
    }
}

bool CConnman::NodeFullyConnected(const CNode* pnode)
{
    return pnode && pnode->fSuccessfullyConnected && !pnode->fDisconnect;
}

void CConnman::PushMessage(CNode* pnode, CSerializedNetMsg&& msg)
{
    AssertLockNotHeld(m_total_bytes_sent_mutex);
    size_t nMessageSize = msg.data.size();
    LogPrint(BCLog::NET, "sending %s (%d bytes) peer=%d\n", SanitizeString(msg.m_type), nMessageSize, pnode->GetId());
    if (gArgs.GetBoolArg("-capturemessages", false)) {
        CaptureMessage(pnode->addr, msg.m_type, msg.data, /* incoming */ false);
    }

    // make sure we use the appropriate network transport format
    std::vector<unsigned char> serializedHeader;
    pnode->m_serializer->prepareForTransport(msg, serializedHeader);

    size_t nTotalSize = nMessageSize + serializedHeader.size();
    statsClient.count("bandwidth.message." + SanitizeString(msg.m_type.c_str()) + ".bytesSent", nTotalSize, 1.0f);
    statsClient.inc("message.sent." + SanitizeString(msg.m_type.c_str()), 1.0f);

    {
        LOCK(pnode->cs_vSend);
        bool hasPendingData = !pnode->vSendMsg.empty();

        //log total amount of bytes per message type
        pnode->mapSendBytesPerMsgType[msg.m_type] += nTotalSize;
        pnode->nSendSize += nTotalSize;

        if (pnode->nSendSize > nSendBufferMaxSize) pnode->fPauseSend = true;
        pnode->vSendMsg.push_back(std::move(serializedHeader));
        if (nMessageSize) pnode->vSendMsg.push_back(std::move(msg.data));
        pnode->nSendMsgSize = pnode->vSendMsg.size();

        {
            LOCK(cs_mapNodesWithDataToSend);
            // we're not holding m_nodes_mutex here, so there is a chance of this node being disconnected shortly before
            // we get here. Whoever called PushMessage still has a ref to CNode*, but will later Release() it, so we
            // might end up having an entry in mapNodesWithDataToSend that is not in m_nodes anymore. We need to
            // Add/Release refs when adding/erasing mapNodesWithDataToSend.
            if (mapNodesWithDataToSend.emplace(pnode->GetId(), pnode).second) {
                pnode->AddRef();
            }
        }

        // wake up select() call in case there was no pending data before (so it was not selecting this socket for sending)
        if (!hasPendingData && (m_wakeup_pipe && m_wakeup_pipe->m_need_wakeup.load()))
            m_wakeup_pipe->Write();
    }
}

bool CConnman::ForNode(const CService& addr, std::function<bool(const CNode* pnode)> cond, std::function<bool(CNode* pnode)> func)
{
    CNode* found = nullptr;
    LOCK(m_nodes_mutex);
    for (auto&& pnode : m_nodes) {
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
    LOCK(m_nodes_mutex);
    for (auto&& pnode : m_nodes) {
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

std::chrono::microseconds CConnman::PoissonNextSendInbound(std::chrono::microseconds now, std::chrono::seconds average_interval)
{
    if (m_next_send_inv_to_incoming.load() < now) {
        // If this function were called from multiple threads simultaneously
        // it would possible that both update the next send variable, and return a different result to their caller.
        // This is not possible in practice as only the net processing thread invokes this function.
        m_next_send_inv_to_incoming = PoissonNextSend(now, average_interval);
    }
    return m_next_send_inv_to_incoming;
}

std::chrono::microseconds PoissonNextSend(std::chrono::microseconds now, std::chrono::seconds average_interval)
{
    double unscaled = -log1p(GetRand(1ULL << 48) * -0.0000000000000035527136788 /* -1/2^48 */);
    return now + std::chrono::duration_cast<std::chrono::microseconds>(unscaled * average_interval + 0.5us);
}

CConnman::NodesSnapshot::NodesSnapshot(const CConnman& connman, std::function<bool(const CNode* pnode)> filter,
                                       bool shuffle)
{
    LOCK(connman.m_nodes_mutex);
    m_nodes_copy.reserve(connman.m_nodes.size());

    for (auto& node : connman.m_nodes) {
        if (!filter(node))
            continue;
        node->AddRef();
        m_nodes_copy.push_back(node);
    }

    if (shuffle) {
        Shuffle(m_nodes_copy.begin(), m_nodes_copy.end(), FastRandomContext{});
    }
}

CConnman::NodesSnapshot::~NodesSnapshot()
{
    for (auto& node : m_nodes_copy) {
        node->Release();
    }
}

CSipHasher CConnman::GetDeterministicRandomizer(uint64_t id) const
{
    return CSipHasher(nSeed0, nSeed1).Write(id);
}

uint64_t CConnman::CalculateKeyedNetGroup(const CAddress& ad) const
{
    std::vector<unsigned char> vchNetGroup(ad.GetGroup(addrman.GetAsmap()));

    return GetDeterministicRandomizer(RANDOMIZER_ID_NETGROUP).Write(vchNetGroup.data(), vchNetGroup.size()).Finalize();
}

void CaptureMessageToFile(const CAddress& addr,
                          const std::string& msg_type,
                          Span<const unsigned char> data,
                          bool is_incoming)
{
    // Note: This function captures the message at the time of processing,
    // not at socket receive/send time.
    // This ensures that the messages are always in order from an application
    // layer (processing) perspective.
    auto now = GetTime<std::chrono::microseconds>();

    // Windows folder names can not include a colon
    std::string clean_addr = addr.ToString();
    std::replace(clean_addr.begin(), clean_addr.end(), ':', '_');

    fs::path base_path = GetDataDir() / "message_capture" / clean_addr;
    fs::create_directories(base_path);

    fs::path path = base_path / (is_incoming ? "msgs_recv.dat" : "msgs_sent.dat");
    CAutoFile f(fsbridge::fopen(path, "ab"), SER_DISK, CLIENT_VERSION);

    ser_writedata64(f, now.count());
    f.write(MakeByteSpan(msg_type));
    for (auto i = msg_type.length(); i < CMessageHeader::COMMAND_SIZE; ++i) {
        f << uint8_t{'\0'};
    }
    uint32_t size = data.size();
    ser_writedata32(f, size);
    f.write(AsBytes(data));
}

std::function<void(const CAddress& addr,
                   const std::string& msg_type,
                   Span<const unsigned char> data,
                   bool is_incoming)>
    CaptureMessage = CaptureMessageToFile;
