// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_H
#define BITCOIN_NET_H

#include <addrdb.h>
#include <addrman.h>
#include <bloom.h>
#include <chainparams.h>
#include <compat.h>
#include <fs.h>
#include <crypto/siphash.h>
#include <hash.h>
#include <i2p.h>
#include <limitedmap.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <policy/feerate.h>
#include <protocol.h>
#include <random.h>
#include <saltedhasher.h>
#include <streams.h>
#include <sync.h>
#include <threadinterrupt.h>
#include <uint256.h>
#include <util/system.h>
#include <consensus/params.h>

#include <atomic>
#include <cstdint>
#include <deque>
#include <map>
#include <thread>
#include <memory>
#include <condition_variable>
#include <optional>
#include <queue>

#ifndef WIN32
#define USE_WAKEUP_PIPE
#endif

class CScheduler;
class CNode;
class BanMan;
struct bilingual_str;

/** Default for -whitelistrelay. */
static const bool DEFAULT_WHITELISTRELAY = true;
/** Default for -whitelistforcerelay. */
static const bool DEFAULT_WHITELISTFORCERELAY = false;

/** Time after which to disconnect, after waiting for a ping response (or inactivity). */
static const int TIMEOUT_INTERVAL = 20 * 60;
/** Minimum time between warnings printed to log. */
static const int WARNING_INTERVAL = 10 * 60;
/** Run the feeler connection loop once every 2 minutes or 120 seconds. **/
static const int FEELER_INTERVAL = 120;
/** The maximum number of entries in an 'inv' protocol message */
static const unsigned int MAX_INV_SZ = 50000;
/** The maximum number of addresses from our addrman to return in response to a getaddr message. */
static constexpr size_t MAX_ADDR_TO_SEND = 1000;
/** Maximum length of incoming protocol messages (no message over 3 MiB is currently acceptable). */
static const unsigned int MAX_PROTOCOL_MESSAGE_LENGTH = 3 * 1024 * 1024;
/** Maximum length of the user agent string in `version` message */
static const unsigned int MAX_SUBVERSION_LENGTH = 256;
/** Maximum number of automatic outgoing nodes over which we'll relay everything (blocks, tx, addrs, etc) */
static const int MAX_OUTBOUND_FULL_RELAY_CONNECTIONS = 8;
/** Maximum number of addnode outgoing nodes */
static const int MAX_ADDNODE_CONNECTIONS = 8;
/** Eviction protection time for incoming connections  */
static const int INBOUND_EVICTION_PROTECTION_TIME = 1;
/** Maximum number of block-relay-only outgoing connections */
static const int MAX_BLOCK_RELAY_ONLY_CONNECTIONS = 2;
/** Maximum number of feeler connections */
static const int MAX_FEELER_CONNECTIONS = 1;
/** -listen default */
static const bool DEFAULT_LISTEN = true;
/** The maximum number of peer connections to maintain.
 *  Masternodes are forced to accept at least this many connections
 */
static const unsigned int DEFAULT_MAX_PEER_CONNECTIONS = 125;
/** The default for -maxuploadtarget. 0 = Unlimited */
static constexpr uint64_t DEFAULT_MAX_UPLOAD_TARGET = 0;
/** Default for blocks only*/
static const bool DEFAULT_BLOCKSONLY = false;
/** -peertimeout default */
static const int64_t DEFAULT_PEER_CONNECT_TIMEOUT = 60;

static const bool DEFAULT_FORCEDNSSEED = false;
static const size_t DEFAULT_MAXRECEIVEBUFFER = 5 * 1000;
static const size_t DEFAULT_MAXSENDBUFFER    = 1 * 1000;

#if defined USE_KQUEUE
#define DEFAULT_SOCKETEVENTS "kqueue"
#elif defined USE_EPOLL
#define DEFAULT_SOCKETEVENTS "epoll"
#elif defined USE_POLL
#define DEFAULT_SOCKETEVENTS "poll"
#else
#define DEFAULT_SOCKETEVENTS "select"
#endif

typedef int64_t NodeId;

struct AddedNodeInfo
{
    std::string strAddedNode;
    CService resolvedAddress;
    bool fConnected;
    bool fInbound;
};

class CNodeStats;
class CClientUIInterface;

struct CSerializedNetMsg
{
    CSerializedNetMsg() = default;
    CSerializedNetMsg(CSerializedNetMsg&&) = default;
    CSerializedNetMsg& operator=(CSerializedNetMsg&&) = default;
    // No copying, only moves.
    CSerializedNetMsg(const CSerializedNetMsg& msg) = delete;
    CSerializedNetMsg& operator=(const CSerializedNetMsg&) = delete;

    std::vector<unsigned char> data;
    std::string command;
};


class NetEventsInterface;
class CConnman
{
friend class CNode;
public:

    enum NumConnections {
        CONNECTIONS_NONE = 0,
        CONNECTIONS_IN = (1U << 0),
        CONNECTIONS_OUT = (1U << 1),
        CONNECTIONS_ALL = (CONNECTIONS_IN | CONNECTIONS_OUT),
        CONNECTIONS_VERIFIED = (1U << 2),
        CONNECTIONS_VERIFIED_IN = (CONNECTIONS_VERIFIED | CONNECTIONS_IN),
        CONNECTIONS_VERIFIED_OUT = (CONNECTIONS_VERIFIED | CONNECTIONS_OUT),
    };

    enum SocketEventsMode {
        SOCKETEVENTS_SELECT = 0,
        SOCKETEVENTS_POLL = 1,
        SOCKETEVENTS_EPOLL = 2,
        SOCKETEVENTS_KQUEUE = 3,
    };

    struct Options
    {
        ServiceFlags nLocalServices = NODE_NONE;
        int nMaxConnections = 0;
        int m_max_outbound_full_relay = 0;
        int m_max_outbound_block_relay = 0;
        int nMaxAddnode = 0;
        int nMaxFeeler = 0;
        CClientUIInterface* uiInterface = nullptr;
        NetEventsInterface* m_msgproc = nullptr;
        BanMan* m_banman = nullptr;
        unsigned int nSendBufferMaxSize = 0;
        unsigned int nReceiveFloodSize = 0;
        uint64_t nMaxOutboundLimit = 0;
        int64_t m_peer_connect_timeout = DEFAULT_PEER_CONNECT_TIMEOUT;
        std::vector<std::string> vSeedNodes;
        std::vector<NetWhitelistPermissions> vWhitelistedRange;
        std::vector<NetWhitebindPermissions> vWhiteBinds;
        std::vector<CService> vBinds;
        std::vector<CService> onion_binds;
        bool m_use_addrman_outgoing = true;
        std::vector<std::string> m_specified_outgoing;
        std::vector<std::string> m_added_nodes;
        SocketEventsMode socketEventsMode = SOCKETEVENTS_SELECT;
        std::vector<bool> m_asmap;
        bool m_i2p_accept_incoming;
    };

    void Init(const Options& connOptions) {
        nLocalServices = connOptions.nLocalServices;
        nMaxConnections = connOptions.nMaxConnections;
        m_max_outbound_full_relay = std::min(connOptions.m_max_outbound_full_relay, connOptions.nMaxConnections);
        m_max_outbound_block_relay = connOptions.m_max_outbound_block_relay;
        m_use_addrman_outgoing = connOptions.m_use_addrman_outgoing;
        nMaxAddnode = connOptions.nMaxAddnode;
        nMaxFeeler = connOptions.nMaxFeeler;
        m_max_outbound = m_max_outbound_full_relay + m_max_outbound_block_relay + nMaxFeeler;
        clientInterface = connOptions.uiInterface;
        m_banman = connOptions.m_banman;
        m_msgproc = connOptions.m_msgproc;
        nSendBufferMaxSize = connOptions.nSendBufferMaxSize;
        nReceiveFloodSize = connOptions.nReceiveFloodSize;
        m_peer_connect_timeout = connOptions.m_peer_connect_timeout;
        {
            LOCK(cs_totalBytesSent);
            nMaxOutboundLimit = connOptions.nMaxOutboundLimit;
        }
        vWhitelistedRange = connOptions.vWhitelistedRange;
        {
            LOCK(cs_vAddedNodes);
            vAddedNodes = connOptions.m_added_nodes;
        }
        socketEventsMode = connOptions.socketEventsMode;
        m_onion_binds = connOptions.onion_binds;
    }

    CConnman(uint64_t seed0, uint64_t seed1, CAddrMan& addrman);
    ~CConnman();
    bool Start(CScheduler& scheduler, const Options& options);

    void StopThreads();
    void StopNodes();
    void Stop()
    {
        StopThreads();
        StopNodes();
    };

    void Interrupt();
    bool GetNetworkActive() const { return fNetworkActive; };
    bool GetUseAddrmanOutgoing() const { return m_use_addrman_outgoing; };
    void SetNetworkActive(bool active);
    SocketEventsMode GetSocketEventsMode() const { return socketEventsMode; }

    enum class MasternodeConn {
        IsNotConnection,
        IsConnection,
    };

    enum class MasternodeProbeConn {
        IsNotConnection,
        IsConnection,
    };

    void OpenNetworkConnection(const CAddress& addrConnect, bool fCountFailure, CSemaphoreGrant *grantOutbound = nullptr, const char *strDest = nullptr, bool fOneShot = false, bool fFeeler = false, bool manual_connection = false, bool block_relay_only = false, MasternodeConn masternode_connection = MasternodeConn::IsNotConnection, MasternodeProbeConn masternode_probe_connection = MasternodeProbeConn::IsNotConnection);
    void OpenMasternodeConnection(const CAddress& addrConnect, MasternodeProbeConn probe = MasternodeProbeConn::IsConnection);
    bool CheckIncomingNonce(uint64_t nonce);

    struct CFullyConnectedOnly {
        bool operator() (const CNode* pnode) const {
            return NodeFullyConnected(pnode);
        }
    };

    constexpr static const CFullyConnectedOnly FullyConnectedOnly{};

    struct CAllNodes {
        bool operator() (const CNode*) const {return true;}
    };

    constexpr static const CAllNodes AllNodes{};

    bool ForNode(NodeId id, std::function<bool(const CNode* pnode)> cond, std::function<bool(CNode* pnode)> func);
    bool ForNode(const CService& addr, std::function<bool(const CNode* pnode)> cond, std::function<bool(CNode* pnode)> func);

    template<typename Callable>
    bool ForNode(const CService& addr, Callable&& func)
    {
        return ForNode(addr, FullyConnectedOnly, func);
    }

    template<typename Callable>
    bool ForNode(NodeId id, Callable&& func)
    {
        return ForNode(id, FullyConnectedOnly, func);
    }

    bool IsConnected(const CService& addr, std::function<bool(const CNode* pnode)> cond)
    {
        return ForNode(addr, cond, [](CNode* pnode){
            return true;
        });
    }

    bool IsMasternodeOrDisconnectRequested(const CService& addr);

    void PushMessage(CNode* pnode, CSerializedNetMsg&& msg);

    template<typename Condition, typename Callable>
    bool ForEachNodeContinueIf(const Condition& cond, Callable&& func)
    {
        LOCK(cs_vNodes);
        for (auto&& node : vNodes)
            if (cond(node))
                if(!func(node))
                    return false;
        return true;
    };

    template<typename Callable>
    bool ForEachNodeContinueIf(Callable&& func)
    {
        return ForEachNodeContinueIf(FullyConnectedOnly, func);
    }

    template<typename Condition, typename Callable>
    bool ForEachNodeContinueIf(const Condition& cond, Callable&& func) const
    {
        LOCK(cs_vNodes);
        for (const auto& node : vNodes)
            if (cond(node))
                if(!func(node))
                    return false;
        return true;
    };

    template<typename Callable>
    bool ForEachNodeContinueIf(Callable&& func) const
    {
        return ForEachNodeContinueIf(FullyConnectedOnly, func);
    }

    template<typename Condition, typename Callable>
    void ForEachNode(const Condition& cond, Callable&& func)
    {
        LOCK(cs_vNodes);
        for (auto&& node : vNodes) {
            if (cond(node))
                func(node);
        }
    };

    template<typename Callable>
    void ForEachNode(Callable&& func)
    {
        ForEachNode(FullyConnectedOnly, func);
    }

    template<typename Condition, typename Callable>
    void ForEachNode(const Condition& cond, Callable&& func) const
    {
        LOCK(cs_vNodes);
        for (auto&& node : vNodes) {
            if (cond(node))
                func(node);
        }
    };

    template<typename Callable>
    void ForEachNode(Callable&& func) const
    {
        ForEachNode(FullyConnectedOnly, func);
    }

    template<typename Condition, typename Callable, typename CallableAfter>
    void ForEachNodeThen(const Condition& cond, Callable&& pre, CallableAfter&& post)
    {
        LOCK(cs_vNodes);
        for (auto&& node : vNodes) {
            if (cond(node))
                pre(node);
        }
        post();
    };

    template<typename Callable, typename CallableAfter>
    void ForEachNodeThen(Callable&& pre, CallableAfter&& post)
    {
        ForEachNodeThen(FullyConnectedOnly, pre, post);
    }

    template<typename Condition, typename Callable, typename CallableAfter>
    void ForEachNodeThen(const Condition& cond, Callable&& pre, CallableAfter&& post) const
    {
        LOCK(cs_vNodes);
        for (auto&& node : vNodes) {
            if (cond(node))
                pre(node);
        }
        post();
    };

    template<typename Callable, typename CallableAfter>
    void ForEachNodeThen(Callable&& pre, CallableAfter&& post) const
    {
        ForEachNodeThen(FullyConnectedOnly, pre, post);
    }

    std::vector<CNode*> CopyNodeVector(std::function<bool(const CNode* pnode)> cond);
    std::vector<CNode*> CopyNodeVector();
    void ReleaseNodeVector(const std::vector<CNode*>& vecNodes);

    void RelayTransaction(const CTransaction& tx);
    void RelayInv(CInv &inv, const int minProtoVersion = MIN_PEER_PROTO_VERSION);
    void RelayInvFiltered(CInv &inv, const CTransaction &relatedTx, const int minProtoVersion = MIN_PEER_PROTO_VERSION);
    // This overload will not update node filters,  so use it only for the cases when other messages will update related transaction data in filters
    void RelayInvFiltered(CInv &inv, const uint256 &relatedTxHash, const int minProtoVersion = MIN_PEER_PROTO_VERSION);

    // Addrman functions
    /**
     * Return all or many randomly selected addresses, optionally by network.
     *
     * @param[in] max_addresses  Maximum number of addresses to return (0 = all).
     * @param[in] max_pct        Maximum percentage of addresses to return (0 = all).
     * @param[in] network        Select only addresses of this network (nullopt = all).
     */
    std::vector<CAddress> GetAddresses(size_t max_addresses, size_t max_pct, std::optional<Network> network);

    /**
     * Cache is used to minimize topology leaks, so it should
     * be used for all non-trusted calls, for example, p2p.
     * A non-malicious call (from RPC or a peer with addr permission) should
     * call the function without a parameter to avoid using the cache.
     */
    std::vector<CAddress> GetAddresses(CNode& requestor, size_t max_addresses, size_t max_pct);

    // This allows temporarily exceeding m_max_outbound_full_relay, with the goal of finding
    // a peer that is better than all our current peers.
    void SetTryNewOutboundPeer(bool flag);
    bool GetTryNewOutboundPeer();

    // Return the number of outbound peers we have in excess of our target (eg,
    // if we previously called SetTryNewOutboundPeer(true), and have since set
    // to false, we may have extra peers that we wish to disconnect). This may
    // return a value less than (num_outbound_connections - num_outbound_slots)
    // in cases where some outbound connections are not yet fully connected, or
    // not yet fully disconnected.
    int GetExtraOutboundCount();

    bool AddNode(const std::string& node);
    bool RemoveAddedNode(const std::string& node);
    std::vector<AddedNodeInfo> GetAddedNodeInfo();

    bool AddPendingMasternode(const uint256& proTxHash);
    void SetMasternodeQuorumNodes(Consensus::LLMQType llmqType, const uint256& quorumHash, const std::set<uint256>& proTxHashes);
    void SetMasternodeQuorumRelayMembers(Consensus::LLMQType llmqType, const uint256& quorumHash, const std::set<uint256>& proTxHashes);
    bool HasMasternodeQuorumNodes(Consensus::LLMQType llmqType, const uint256& quorumHash);
    std::set<uint256> GetMasternodeQuorums(Consensus::LLMQType llmqType);
    // also returns QWATCH nodes
    std::set<NodeId> GetMasternodeQuorumNodes(Consensus::LLMQType llmqType, const uint256& quorumHash) const;
    void RemoveMasternodeQuorumNodes(Consensus::LLMQType llmqType, const uint256& quorumHash);
    bool IsMasternodeQuorumNode(const CNode* pnode);
    bool IsMasternodeQuorumRelayMember(const uint256& protxHash);
    void AddPendingProbeConnections(const std::set<uint256>& proTxHashes);

    size_t GetNodeCount(NumConnections num);
    size_t GetMaxOutboundNodeCount();
    void GetNodeStats(std::vector<CNodeStats>& vstats);
    bool DisconnectNode(const std::string& node);
    bool DisconnectNode(const CSubNet& subnet);
    bool DisconnectNode(const CNetAddr& addr);
    bool DisconnectNode(NodeId id);

    //! Used to convey which local services we are offering peers during node
    //! connection.
    //!
    //! The data returned by this is used in CNode construction,
    //! which is used to advertise which services we are offering
    //! that peer during `net_processing.cpp:PushNodeVersion()`.
    ServiceFlags GetLocalServices() const;

    uint64_t GetMaxOutboundTarget();
    std::chrono::seconds GetMaxOutboundTimeframe();

    //! check if the outbound target is reached
    //! if param historicalBlockServingLimit is set true, the function will
    //! response true if the limit for serving historical blocks has been reached
    bool OutboundTargetReached(bool historicalBlockServingLimit);

    //! response the bytes left in the current max outbound cycle
    //! in case of no limit, it will always response 0
    uint64_t GetOutboundTargetBytesLeft();

    //! returns the time left in the current max outbound cycle
    //! in case of no limit, it will always return 0
    std::chrono::seconds GetMaxOutboundTimeLeftInCycle();

    uint64_t GetTotalBytesRecv();
    uint64_t GetTotalBytesSent();

    /** Get a unique deterministic randomizer. */
    CSipHasher GetDeterministicRandomizer(uint64_t id) const;

    unsigned int GetReceiveFloodSize() const;

    void WakeMessageHandler();
    void WakeSelect();

    /** Attempts to obfuscate tx time through exponentially distributed emitting.
        Works assuming that a single interval is used.
        Variable intervals will result in privacy decrease.
    */
    int64_t PoissonNextSendInbound(int64_t now, int average_interval_seconds);

    void SetAsmap(std::vector<bool> asmap) { addrman.m_asmap = std::move(asmap); }

private:
    struct ListenSocket {
    public:
        SOCKET socket;
        inline void AddSocketPermissionFlags(NetPermissionFlags& flags) const { NetPermissions::AddFlag(flags, m_permissions); }
        ListenSocket(SOCKET socket_, NetPermissionFlags permissions_) : socket(socket_), m_permissions(permissions_) {}
    private:
        NetPermissionFlags m_permissions;
    };

    bool BindListenPort(const CService& bindAddr, bilingual_str& strError, NetPermissionFlags permissions);
    bool Bind(const CService& addr, unsigned int flags, NetPermissionFlags permissions);
    bool InitBinds(
        const std::vector<CService>& binds,
        const std::vector<NetWhitebindPermissions>& whiteBinds,
        const std::vector<CService>& onion_binds);

    void ThreadOpenAddedConnections();
    void AddOneShot(const std::string& strDest);
    void ProcessOneShot();
    void ThreadOpenConnections(std::vector<std::string> connect);
    void ThreadMessageHandler();
    void ThreadI2PAcceptIncoming();
    void AcceptConnection(const ListenSocket& hListenSocket);

    /**
     * Create a `CNode` object from a socket that has just been accepted and add the node to
     * the `vNodes` member.
     * @param[in] hSocket Connected socket to communicate with the peer.
     * @param[in] permissionFlags The peer's permissions.
     * @param[in] addr_bind The address and port at our side of the connection.
     * @param[in] addr The address and port at the peer's side of the connection.
     */
    void CreateNodeFromAcceptedSocket(SOCKET hSocket,
                                      NetPermissionFlags permissionFlags,
                                      const CAddress& addr_bind,
                                      const CAddress& addr);

    void DisconnectNodes();
    void NotifyNumConnectionsChanged();
    void CalculateNumConnectionsChangedStats();
    void InactivityCheck(CNode *pnode) const;
    bool GenerateSelectSet(std::set<SOCKET> &recv_set, std::set<SOCKET> &send_set, std::set<SOCKET> &error_set);
#ifdef USE_KQUEUE
    void SocketEventsKqueue(std::set<SOCKET> &recv_set, std::set<SOCKET> &send_set, std::set<SOCKET> &error_set, bool fOnlyPoll);
#endif
#ifdef USE_EPOLL
    void SocketEventsEpoll(std::set<SOCKET> &recv_set, std::set<SOCKET> &send_set, std::set<SOCKET> &error_set, bool fOnlyPoll);
#endif
#ifdef USE_POLL
    void SocketEventsPoll(std::set<SOCKET> &recv_set, std::set<SOCKET> &send_set, std::set<SOCKET> &error_set, bool fOnlyPoll);
#endif
    void SocketEventsSelect(std::set<SOCKET> &recv_set, std::set<SOCKET> &send_set, std::set<SOCKET> &error_set, bool fOnlyPoll);
    void SocketEvents(std::set<SOCKET> &recv_set, std::set<SOCKET> &send_set, std::set<SOCKET> &error_set, bool fOnlyPoll);
    void SocketHandler();
    void ThreadSocketHandler();
    void ThreadDNSAddressSeed();
    void ThreadOpenMasternodeConnections();

    uint64_t CalculateKeyedNetGroup(const CAddress& ad) const;

    CNode* FindNode(const CNetAddr& ip, bool fExcludeDisconnecting = true);
    CNode* FindNode(const CSubNet& subNet, bool fExcludeDisconnecting = true);
    CNode* FindNode(const std::string& addrName, bool fExcludeDisconnecting = true);
    CNode* FindNode(const CService& addr, bool fExcludeDisconnecting = true);

    bool AttemptToEvictConnection();
    CNode* ConnectNode(CAddress addrConnect, const char *pszDest = nullptr, bool fCountFailure = false, bool manual_connection = false, bool block_relay_only = false);
    void AddWhitelistPermissionFlags(NetPermissionFlags& flags, const CNetAddr &addr) const;

    void DeleteNode(CNode* pnode);

    NodeId GetNewNodeId();

    size_t SocketSendData(CNode *pnode);
    size_t SocketRecvData(CNode* pnode);
    void DumpAddresses();

    // Network stats
    void RecordBytesRecv(uint64_t bytes);
    void RecordBytesSent(uint64_t bytes);

    // Whether the node should be passed out in ForEach* callbacks
    static bool NodeFullyConnected(const CNode* pnode);

    void RegisterEvents(CNode* pnode);
    void UnregisterEvents(CNode* pnode);

    // Network usage totals
    RecursiveMutex cs_totalBytesRecv;
    RecursiveMutex cs_totalBytesSent;
    uint64_t nTotalBytesRecv GUARDED_BY(cs_totalBytesRecv) {0};
    uint64_t nTotalBytesSent GUARDED_BY(cs_totalBytesSent) {0};

    // outbound limit & stats
    uint64_t nMaxOutboundTotalBytesSentInCycle GUARDED_BY(cs_totalBytesSent) {0};
    std::chrono::seconds nMaxOutboundCycleStartTime GUARDED_BY(cs_totalBytesSent) {0};
    uint64_t nMaxOutboundLimit GUARDED_BY(cs_totalBytesSent);

    // P2P timeout in seconds
    int64_t m_peer_connect_timeout;

    // Whitelisted ranges. Any node connecting from these is automatically
    // whitelisted (as well as those connecting to whitelisted binds).
    std::vector<NetWhitelistPermissions> vWhitelistedRange;

    unsigned int nSendBufferMaxSize{0};
    unsigned int nReceiveFloodSize{0};

    std::vector<ListenSocket> vhListenSocket;
    std::atomic<bool> fNetworkActive{true};
    bool fAddressesInitialized{false};
    CAddrMan& addrman;
    std::deque<std::string> vOneShots GUARDED_BY(cs_vOneShots);
    RecursiveMutex cs_vOneShots;
    std::vector<std::string> vAddedNodes GUARDED_BY(cs_vAddedNodes);
    RecursiveMutex cs_vAddedNodes;
    std::vector<uint256> vPendingMasternodes;
    mutable RecursiveMutex cs_vPendingMasternodes;
    std::map<std::pair<Consensus::LLMQType, uint256>, std::set<uint256>> masternodeQuorumNodes GUARDED_BY(cs_vPendingMasternodes);
    std::map<std::pair<Consensus::LLMQType, uint256>, std::set<uint256>> masternodeQuorumRelayMembers GUARDED_BY(cs_vPendingMasternodes);
    std::set<uint256> masternodePendingProbes GUARDED_BY(cs_vPendingMasternodes);
    std::vector<CNode*> vNodes GUARDED_BY(cs_vNodes);
    std::list<CNode*> vNodesDisconnected;
    std::unordered_map<SOCKET, CNode*> mapSocketToNode;
    mutable RecursiveMutex cs_vNodes;
    std::atomic<NodeId> nLastNodeId{0};
    unsigned int nPrevNodeCount{0};

    /**
     * Cache responses to addr requests to minimize privacy leak.
     * Attack example: scraping addrs in real-time may allow an attacker
     * to infer new connections of the victim by detecting new records
     * with fresh timestamps (per self-announcement).
     */
    struct CachedAddrResponse {
        std::vector<CAddress> m_addrs_response_cache;
        std::chrono::microseconds m_cache_entry_expiration{0};
    };

    /**
     * Addr responses stored in different caches
     * per (network, local socket) prevent cross-network node identification.
     * If a node for example is multi-homed under Tor and IPv6,
     * a single cache (or no cache at all) would let an attacker
     * to easily detect that it is the same node by comparing responses.
     * Indexing by local socket prevents leakage when a node has multiple
     * listening addresses on the same network.
     *
     * The used memory equals to 1000 CAddress records (or around 40 bytes) per
     * distinct Network (up to 5) we have/had an inbound peer from,
     * resulting in at most ~196 KB. Every separate local socket may
     * add up to ~196 KB extra.
     */
    std::map<uint64_t, CachedAddrResponse> m_addr_response_caches;

    /**
     * Services this instance offers.
     *
     * This data is replicated in each CNode instance we create during peer
     * connection (in ConnectNode()) under a member also called
     * nLocalServices.
     *
     * This data is not marked const, but after being set it should not
     * change. See the note in CNode::nLocalServices documentation.
     *
     * \sa CNode::nLocalServices
     */
    ServiceFlags nLocalServices;

    std::unique_ptr<CSemaphore> semOutbound;
    std::unique_ptr<CSemaphore> semAddnode;
    int nMaxConnections;

    // How many full-relay (tx, block, addr) outbound peers we want
    int m_max_outbound_full_relay;

    // How many block-relay only outbound peers we want
    // We do not relay tx or addr messages with these peers
    int m_max_outbound_block_relay;

    int nMaxAddnode;
    int nMaxFeeler;
    int m_max_outbound;
    bool m_use_addrman_outgoing;
    CClientUIInterface* clientInterface;
    NetEventsInterface* m_msgproc;
    BanMan* m_banman;

    /** SipHasher seeds for deterministic randomness */
    const uint64_t nSeed0, nSeed1;

    /** flag for waking the message processor. */
    bool fMsgProcWake GUARDED_BY(mutexMsgProc);

    std::condition_variable condMsgProc;
    Mutex mutexMsgProc;
    std::atomic<bool> flagInterruptMsgProc{false};

    /**
     * This is signaled when network activity should cease.
     * A pointer to it is saved in `m_i2p_sam_session`, so make sure that
     * the lifetime of `interruptNet` is not shorter than
     * the lifetime of `m_i2p_sam_session`.
     */
    CThreadInterrupt interruptNet;

    /**
     * I2P SAM session.
     * Used to accept incoming and make outgoing I2P connections.
     */
    std::unique_ptr<i2p::sam::Session> m_i2p_sam_session;

#ifdef USE_WAKEUP_PIPE
    /** a pipe which is added to select() calls to wakeup before the timeout */
    int wakeupPipe[2]{-1,-1};
#endif
    std::atomic<bool> wakeupSelectNeeded{false};

    SocketEventsMode socketEventsMode;
#ifdef USE_KQUEUE
    int kqueuefd{-1};
#endif
#ifdef USE_EPOLL
    int epollfd{-1};
#endif

    /** Protected by cs_vNodes */
    std::unordered_map<NodeId, CNode*> mapReceivableNodes GUARDED_BY(cs_vNodes);
    std::unordered_map<NodeId, CNode*> mapSendableNodes GUARDED_BY(cs_vNodes);
    /** Protected by cs_mapNodesWithDataToSend */
    std::unordered_map<NodeId, CNode*> mapNodesWithDataToSend GUARDED_BY(cs_mapNodesWithDataToSend);
    mutable RecursiveMutex cs_mapNodesWithDataToSend;

    std::thread threadDNSAddressSeed;
    std::thread threadSocketHandler;
    std::thread threadOpenAddedConnections;
    std::thread threadOpenConnections;
    std::thread threadOpenMasternodeConnections;
    std::thread threadMessageHandler;
    std::thread threadI2PAcceptIncoming;

    /** flag for deciding to connect to an extra outbound peer,
     *  in excess of m_max_outbound_full_relay
     *  This takes the place of a feeler connection */
    std::atomic_bool m_try_another_outbound_peer;

    std::atomic<int64_t> m_next_send_inv_to_incoming{0};

    /**
     * A vector of -bind=<address>:<port>=onion arguments each of which is
     * an address and port that are designated for incoming Tor connections.
     */
    std::vector<CService> m_onion_binds;

    friend struct CConnmanTest;
    friend struct ConnmanTestMsg;
};
void Discover();
uint16_t GetListenPort();

struct CombinerAll
{
    typedef bool result_type;

    template<typename I>
    bool operator()(I first, I last) const
    {
        while (first != last) {
            if (!(*first)) return false;
            ++first;
        }
        return true;
    }
};

/**
 * Interface for message handling
 */
class NetEventsInterface
{
public:
    /** Initialize a peer (setup state, queue any initial messages) */
    virtual void InitializeNode(CNode* pnode) = 0;

    /** Handle removal of a peer (clear state) */
    virtual void FinalizeNode(const CNode& node) = 0;

    /**
    * Process protocol messages received from a given node
    *
    * @param[in]   pnode           The node which we have received messages from.
    * @param[in]   interrupt       Interrupt condition for processing threads
    * @return                      True if there is more work to be done
    */
    virtual bool ProcessMessages(CNode* pnode, std::atomic<bool>& interrupt) = 0;

    /**
    * Send queued protocol messages to a given node.
    *
    * @param[in]   pnode           The node which we are sending messages to.
    * @return                      True if there is more work to be done
    */
    virtual bool SendMessages(CNode* pnode) = 0;


protected:
    /**
     * Protected destructor so that instances can only be deleted by derived classes.
     * If that restriction is no longer desired, this should be made public and virtual.
     */
    ~NetEventsInterface() = default;
};

enum
{
    LOCAL_NONE,   // unknown
    LOCAL_IF,     // address a local interface listens on
    LOCAL_BIND,   // address explicit bound to
    LOCAL_MAPPED, // address reported by UPnP or NAT-PMP
    LOCAL_MANUAL, // address explicitly specified (-externalip=)

    LOCAL_MAX
};

bool IsPeerAddrLocalGood(CNode *pnode);
/** Returns a local address that we should advertise to this peer */
std::optional<CAddress> GetLocalAddrForPeer(CNode *pnode);

/**
 * Mark a network as reachable or unreachable (no automatic connects to it)
 * @note Networks are reachable by default
 */
void SetReachable(enum Network net, bool reachable);
/** @returns true if the network is reachable, false otherwise */
bool IsReachable(enum Network net);
/** @returns true if the address is in a reachable network, false otherwise */
bool IsReachable(const CNetAddr& addr);

bool AddLocal(const CService& addr, int nScore = LOCAL_NONE);
bool AddLocal(const CNetAddr& addr, int nScore = LOCAL_NONE);
void RemoveLocal(const CService& addr);
bool SeenLocal(const CService& addr);
bool IsLocal(const CService& addr);
bool GetLocal(CService &addr, const CNetAddr *paddrPeer = nullptr);
CAddress GetLocalAddress(const CNetAddr *paddrPeer, ServiceFlags nLocalServices);


extern bool fDiscover;
extern bool fListen;

/** Subversion as sent to the P2P network in `version` messages */
extern std::string strSubVersion;

struct LocalServiceInfo {
    int nScore;
    uint16_t nPort;
};

extern Mutex g_maplocalhost_mutex;
extern std::map<CNetAddr, LocalServiceInfo> mapLocalHost GUARDED_BY(g_maplocalhost_mutex);

extern const std::string NET_MESSAGE_COMMAND_OTHER;
typedef std::map<std::string, uint64_t> mapMsgCmdSize; //command, total bytes

class CNodeStats
{
public:
    NodeId nodeid;
    ServiceFlags nServices;
    bool fRelayTxes;
    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nTimeConnected;
    int64_t nTimeOffset;
    std::string addrName;
    int nVersion;
    std::string cleanSubVer;
    bool fInbound;
    bool m_manual_connection;
    int nStartingHeight;
    uint64_t nSendBytes;
    mapMsgCmdSize mapSendBytesPerMsgCmd;
    uint64_t nRecvBytes;
    mapMsgCmdSize mapRecvBytesPerMsgCmd;
    NetPermissionFlags m_permissionFlags;
    bool m_legacyWhitelisted;
    int64_t m_ping_usec;
    int64_t m_ping_wait_usec;
    int64_t m_min_ping_usec;
    // Our address, as reported by the peer
    std::string addrLocal;
    // Address of this peer
    CAddress addr;
    // Bind address of our side of the connection
    CAddress addrBind;
    // Name of the network the peer connected through
    std::string m_network;
    uint32_t m_mapped_as;
    // In case this is a verified MN, this value is the proTx of the MN
    uint256 verifiedProRegTxHash;
    // In case this is a verified MN, this value is the hashed operator pubkey of the MN
    uint256 verifiedPubKeyHash;
    bool m_masternode_connection;
};



/** Transport protocol agnostic message container.
 * Ideally it should only contain receive time, payload,
 * command and size.
 */
class CNetMessage {
public:
    CDataStream m_recv;                  // received message data
    int64_t m_time = 0;                  // time (in microseconds) of message receipt.
    uint32_t m_message_size = 0;         // size of the payload
    uint32_t m_raw_message_size = 0;     // used wire size of the message (including header/checksum)
    std::string m_command;

    CNetMessage(CDataStream&& recv_in) : m_recv(std::move(recv_in)) {}

    void SetVersion(int nVersionIn)
    {
        m_recv.SetVersion(nVersionIn);
    }
};

/** The TransportDeserializer takes care of holding and deserializing the
 * network receive buffer. It can deserialize the network buffer into a
 * transport protocol agnostic CNetMessage (command & payload)
 */
class TransportDeserializer {
public:
    // returns true if the current deserialization is complete
    virtual bool Complete() const = 0;
    // set the serialization context version
    virtual void SetVersion(int version) = 0;
    /** read and deserialize data, advances msg_bytes data pointer */
    virtual int Read(Span<const uint8_t>& msg_bytes) = 0;
    // decomposes a message from the context
    virtual std::optional<CNetMessage> GetMessage(int64_t time, uint32_t& out_err) = 0;
    virtual ~TransportDeserializer() {}
};

class V1TransportDeserializer final : public TransportDeserializer
{
private:
    const CChainParams& m_chain_params;
    const NodeId m_node_id; // Only for logging
    mutable CHash256 hasher;
    mutable uint256 data_hash;
    bool in_data;                   // parsing header (false) or data (true)
    CDataStream hdrbuf;             // partially received header
    CMessageHeader hdr;             // complete header
    CDataStream vRecv;              // received message data
    unsigned int nHdrPos;
    unsigned int nDataPos;

    const uint256& GetMessageHash() const;
    int readHeader(Span<const uint8_t> msg_bytes);
    int readData(Span<const uint8_t> msg_bytes);

    void Reset() {
        vRecv.clear();
        hdrbuf.clear();
        hdrbuf.resize(24);
        in_data = false;
        nHdrPos = 0;
        nDataPos = 0;
        data_hash.SetNull();
        hasher.Reset();
    }

public:
    V1TransportDeserializer(const CChainParams& chain_params, const NodeId node_id, int nTypeIn, int nVersionIn)
        : m_chain_params(chain_params),
          m_node_id(node_id),
          hdrbuf(nTypeIn, nVersionIn),
          vRecv(nTypeIn, nVersionIn)
    {
        Reset();
    }

    bool Complete() const override
    {
        if (!in_data)
            return false;
        return (hdr.nMessageSize == nDataPos);
    }
    void SetVersion(int nVersionIn) override
    {
        hdrbuf.SetVersion(nVersionIn);
        vRecv.SetVersion(nVersionIn);
    }
    int Read(Span<const uint8_t>& msg_bytes) override
    {
        int ret = in_data ? readData(msg_bytes) : readHeader(msg_bytes);
        if (ret < 0) {
            Reset();
        } else {
            msg_bytes = msg_bytes.subspan(ret);
        }
        return ret;
    }
    std::optional<CNetMessage> GetMessage(int64_t time, uint32_t& out_err_raw_size) override;
};

/** The TransportSerializer prepares messages for the network transport
 */
class TransportSerializer {
public:
    // prepare message for transport (header construction, error-correction computation, payload encryption, etc.)
    virtual void prepareForTransport(CSerializedNetMsg& msg, std::vector<unsigned char>& header) = 0;
    virtual ~TransportSerializer() {}
};

class V1TransportSerializer  : public TransportSerializer {
public:
    void prepareForTransport(CSerializedNetMsg& msg, std::vector<unsigned char>& header) override;
};

/** Information about a peer */
class CNode
{
    friend class CConnman;
    friend struct ConnmanTestMsg;

public:
    std::unique_ptr<TransportDeserializer> m_deserializer;
    std::unique_ptr<TransportSerializer> m_serializer;

    NetPermissionFlags m_permissionFlags{ PF_NONE };
    std::atomic<ServiceFlags> nServices{NODE_NONE};
    SOCKET hSocket GUARDED_BY(cs_hSocket);
    /** Total size of all vSendMsg entries */
    size_t nSendSize GUARDED_BY(cs_vSend){0};
    /** Offset inside the first vSendMsg already sent */
    size_t nSendOffset GUARDED_BY(cs_vSend){0};
    uint64_t nSendBytes GUARDED_BY(cs_vSend){0};
    std::list<std::vector<unsigned char>> vSendMsg GUARDED_BY(cs_vSend);
    std::atomic<size_t> nSendMsgSize{0};
    RecursiveMutex cs_vSend;
    RecursiveMutex cs_hSocket;
    RecursiveMutex cs_vRecv;

    RecursiveMutex cs_vProcessMsg;
    std::list<CNetMessage> vProcessMsg GUARDED_BY(cs_vProcessMsg);
    size_t nProcessQueueSize{0};

    RecursiveMutex cs_sendProcessing;

    uint64_t nRecvBytes GUARDED_BY(cs_vRecv){0};
    std::atomic<int> nRecvVersion{INIT_PROTO_VERSION};

    std::atomic<int64_t> nLastSend{0};
    std::atomic<int64_t> nLastRecv{0};
    const int64_t nTimeConnected;
    std::atomic<int64_t> nTimeOffset{0};
    std::atomic<int64_t> nLastWarningTime{0};
    std::atomic<int64_t> nTimeFirstMessageReceived{0};
    std::atomic<bool> fFirstMessageIsMNAUTH{false};
    // Address of this peer
    const CAddress addr;
    // Bind address of our side of the connection
    const CAddress addrBind;
    std::atomic<int> nNumWarningsSkipped{0};
    std::atomic<int> nVersion{0};
    /**
     * cleanSubVer is a sanitized string of the user agent byte array we read
     * from the wire. This cleaned string can safely be logged or displayed.
     */
    std::string cleanSubVer GUARDED_BY(cs_SubVer){};
    RecursiveMutex cs_SubVer; // used for both cleanSubVer and strSubVer
    bool m_prefer_evict{false}; // This peer is preferred for eviction.
    bool HasPermission(NetPermissionFlags permission) const {
        return NetPermissions::HasFlag(m_permissionFlags, permission);
    }
    // This boolean is unusued in actual processing, only present for backward compatibility at RPC/QT level
    bool m_legacyWhitelisted{false};
    bool fFeeler{false}; // If true this node is being used as a short lived feeler.
    bool fOneShot{false};
    bool m_manual_connection{false};
    bool fClient{false}; // set by version message
    bool m_limited_node{false}; //after BIP159, set by version message
    const bool fInbound;

    /**
     * Whether the peer has signaled support for receiving ADDRv2 (BIP155)
     * messages, implying a preference to receive ADDRv2 instead of ADDR ones.
     */
    std::atomic_bool m_wants_addrv2{false};
    std::atomic_bool fSuccessfullyConnected{false};
    // Setting fDisconnect to true will cause the node to be disconnected the
    // next time DisconnectNodes() runs
    std::atomic_bool fDisconnect{false};
    std::atomic<int64_t> nDisconnectLingerTime{0};
    std::atomic_bool fSocketShutdown{false};
    std::atomic_bool fOtherSideDisconnected { false };
    bool fSentAddr{false};
    // If 'true' this node will be disconnected on CMasternodeMan::ProcessMasternodeConnections()
    std::atomic<bool> m_masternode_connection{false};
    // If 'true' this node will be disconnected after MNAUTH
    std::atomic<bool> m_masternode_probe_connection{false};
    // If 'true', we identified it as an intra-quorum relay connection
    std::atomic<bool> m_masternode_iqr_connection{false};
    CSemaphoreGrant grantOutbound;
    std::atomic<int> nRefCount{0};

    const uint64_t nKeyedNetGroup;

    std::atomic_bool fPauseRecv{false};
    std::atomic_bool fPauseSend{false};

    std::atomic_bool fHasRecvData{false};
    std::atomic_bool fCanSendData{false};

    /**
     * Get network the peer connected through.
     *
     * Returns Network::NET_ONION for *inbound* onion connections,
     * and CNetAddr::GetNetClass() otherwise. The latter cannot be used directly
     * because it doesn't detect the former, and it's not the responsibility of
     * the CNetAddr class to know the actual network a peer is connected through.
     *
     * @return network the peer connected through.
     */
    Network ConnectedThroughNetwork() const;

protected:
    mapMsgCmdSize mapSendBytesPerMsgCmd GUARDED_BY(cs_vSend);
    mapMsgCmdSize mapRecvBytesPerMsgCmd GUARDED_BY(cs_vRecv);

public:
    uint256 hashContinue;
    std::atomic<int> nStartingHeight{-1};

    // flood relay
    std::vector<CAddress> vAddrToSend;
    const std::unique_ptr<CRollingBloomFilter> m_addr_known;
    bool fGetAddr{false};
    std::chrono::microseconds m_next_addr_send GUARDED_BY(cs_sendProcessing){0};
    std::chrono::microseconds m_next_local_addr_send GUARDED_BY(cs_sendProcessing){0};

    // Don't relay addr messages to peers that we connect to as block-relay-only
    // peers (to prevent adversaries from inferring these links from addr
    // traffic).
    bool IsAddrRelayPeer() const { return m_addr_known != nullptr; }

    bool IsBlockRelayOnly() const;

    // List of block ids we still have announce.
    // There is no final sorting before sending, as they are always sent immediately
    // and in the order requested.
    std::vector<uint256> vInventoryBlockToSend GUARDED_BY(cs_inventory);
    RecursiveMutex cs_inventory;
    /** UNIX epoch time of the last block received from this peer that we had
     * not yet seen (e.g. not already received from another peer), that passed
     * preliminary validity checks and was saved to disk, even if we don't
     * connect the block or it eventually fails connection. Used as an inbound
     * peer eviction criterium in CConnman::AttemptToEvictConnection. */
    std::atomic<int64_t> nLastBlockTime{0};

    /** UNIX epoch time of the last transaction received from this peer that we
     * had not yet seen (e.g. not already received from another peer) and that
     * was accepted into our mempool. Used as an inbound peer eviction criterium
     * in CConnman::AttemptToEvictConnection. */
    std::atomic<int64_t> nLastTXTime{0};

    struct TxRelay {
        mutable RecursiveMutex cs_filter;
        // We use fRelayTxes for two purposes -
        // a) it allows us to not relay tx invs before receiving the peer's version message
        // b) the peer may tell us in its version message that we should not relay tx invs
        //    unless it loads a bloom filter.
        bool fRelayTxes GUARDED_BY(cs_filter){false};
        std::unique_ptr<CBloomFilter> pfilter PT_GUARDED_BY(cs_filter) GUARDED_BY(cs_filter){nullptr};

        mutable RecursiveMutex cs_tx_inventory;
        // inventory based relay
        CRollingBloomFilter filterInventoryKnown GUARDED_BY(cs_tx_inventory){50000, 0.000001};
        // Set of transaction ids we still have to announce.
        // They are sorted by the mempool before relay, so the order is not important.
        std::set<uint256> setInventoryTxToSend GUARDED_BY(cs_tx_inventory);
        // List of non-tx/non-block inventory items
        std::vector<CInv> vInventoryOtherToSend GUARDED_BY(cs_tx_inventory);
        // Used for BIP35 mempool sending, also protected by cs_tx_inventory
        bool fSendMempool GUARDED_BY(cs_tx_inventory){false};
        // Last time a "MEMPOOL" request was serviced.
        std::atomic<std::chrono::seconds> m_last_mempool_req{0s};
        std::chrono::microseconds nNextInvSend{0};
    };

    // in bitcoin: m_tx_relay == nullptr if we're not relaying transactions with this peer
    // in dash: m_tx_relay should never be nullptr, use `IsAddrRelayPeer() == false` instead
    std::unique_ptr<TxRelay> m_tx_relay{std::make_unique<TxRelay>()};

    // Used for headers announcements - unfiltered blocks to relay
    std::vector<uint256> vBlockHashesToAnnounce GUARDED_BY(cs_inventory);

    // Ping time measurement:
    // The pong reply we're expecting, or 0 if no pong expected.
    std::atomic<uint64_t> nPingNonceSent{0};
    // Time (in usec) the last ping was sent, or 0 if no ping was ever sent.
    std::atomic<int64_t> nPingUsecStart{0};
    // Last measured round-trip time.
    std::atomic<int64_t> nPingUsecTime{0};
    // Best measured round-trip time.
    std::atomic<int64_t> nMinPingUsecTime{std::numeric_limits<int64_t>::max()};
    // Whether a ping is requested.
    std::atomic<bool> fPingQueued{false};

    // If true, we will send him CoinJoin queue messages
    std::atomic<bool> fSendDSQueue{false};

    // If true, we will announce/send him plain recovered sigs (usually true for full nodes)
    std::atomic<bool> fSendRecSigs{false};
    // If true, we will send him all quorum related messages, even if he is not a member of our quorums
    std::atomic<bool> qwatch{false};

    CNode(NodeId id, ServiceFlags nLocalServicesIn, SOCKET hSocketIn, const CAddress &addrIn, uint64_t nKeyedNetGroupIn, uint64_t nLocalHostNonceIn, const CAddress &addrBindIn, const std::string &addrNameIn = "", bool fInboundIn = false, bool block_relay_only = false, bool inbound_onion = false);
    ~CNode();
    CNode(const CNode&) = delete;
    CNode& operator=(const CNode&) = delete;

private:
    const NodeId id;
    const uint64_t nLocalHostNonce;

    //! Services offered to this peer.
    //!
    //! This is supplied by the parent CConnman during peer connection
    //! (CConnman::ConnectNode()) from its attribute of the same name.
    //!
    //! This is const because there is no protocol defined for renegotiating
    //! services initially offered to a peer. The set of local services we
    //! offer should not change after initialization.
    //!
    //! An interesting example of this is NODE_NETWORK and initial block
    //! download: a node which starts up from scratch doesn't have any blocks
    //! to serve, but still advertises NODE_NETWORK because it will eventually
    //! fulfill this role after IBD completes. P2P code is written in such a
    //! way that it can gracefully handle peers who don't make good on their
    //! service advertisements.
    const ServiceFlags nLocalServices;

    int nSendVersion {0};
    std::list<CNetMessage> vRecvMsg;  // Used only by SocketHandler thread

    mutable RecursiveMutex cs_addrName;
    std::string addrName GUARDED_BY(cs_addrName);

    // Our address, as reported by the peer
    CService addrLocal GUARDED_BY(cs_addrLocal);
    mutable RecursiveMutex cs_addrLocal;

    //! Whether this peer connected via our Tor onion service.
    const bool m_inbound_onion{false};

    // Challenge sent in VERSION to be answered with MNAUTH (only happens between MNs)
    mutable RecursiveMutex cs_mnauth;
    uint256 sentMNAuthChallenge GUARDED_BY(cs_mnauth);
    uint256 receivedMNAuthChallenge GUARDED_BY(cs_mnauth);
    uint256 verifiedProRegTxHash GUARDED_BY(cs_mnauth);
    uint256 verifiedPubKeyHash GUARDED_BY(cs_mnauth);

public:

    NodeId GetId() const {
        return id;
    }

    uint64_t GetLocalNonce() const {
        return nLocalHostNonce;
    }

    int GetRefCount() const
    {
        assert(nRefCount >= 0);
        return nRefCount;
    }

    /**
     * Receive bytes from the buffer and deserialize them into messages.
     *
     * @param[in]   msg_bytes   The raw data
     * @param[out]  complete    Set True if at least one message has been
     *                          deserialized and is ready to be processed
     * @return  True if the peer should stay connected,
     *          False if the peer should be disconnected from.
     */
    bool ReceiveMsgBytes(Span<const uint8_t> msg_bytes, bool& complete);

    void SetRecvVersion(int nVersionIn)
    {
        nRecvVersion = nVersionIn;
    }
    int GetRecvVersion() const
    {
        return nRecvVersion;
    }
    void SetSendVersion(int nVersionIn);
    int GetSendVersion() const;

    CService GetAddrLocal() const;
    //! May not be called more than once
    void SetAddrLocal(const CService& addrLocalIn);

    CNode* AddRef()
    {
        nRefCount++;
        return this;
    }

    void Release()
    {
        nRefCount--;
    }



    void AddAddressKnown(const CAddress& _addr)
    {
        assert(m_addr_known);
        m_addr_known->insert(_addr.GetKey());
    }

    /**
     * Whether the peer supports the address. For example, a peer that does not
     * implement BIP155 cannot receive Tor v3 addresses because it requires
     * ADDRv2 (BIP155) encoding.
     */
    bool IsAddrCompatible(const CAddress& addr) const
    {
        return m_wants_addrv2 || addr.IsAddrV1Compatible();
    }

    void PushAddress(const CAddress& _addr, FastRandomContext &insecure_rand)
    {
        // Known checking here is only to save space from duplicates.
        // SendMessages will filter it again for knowns that were added
        // after addresses were pushed.
        assert(m_addr_known);
        if (_addr.IsValid() && !m_addr_known->contains(_addr.GetKey()) && IsAddrCompatible(_addr)) {
            if (vAddrToSend.size() >= MAX_ADDR_TO_SEND) {
                vAddrToSend[insecure_rand.randrange(vAddrToSend.size())] = _addr;
            } else {
                vAddrToSend.push_back(_addr);
            }
        }
    }


    void AddInventoryKnown(const CInv& inv)
    {
        AddInventoryKnown(inv.hash);
    }

    void AddInventoryKnown(const uint256& hash)
    {
        LOCK(m_tx_relay->cs_tx_inventory);
        m_tx_relay->filterInventoryKnown.insert(hash);
    }

    void PushInventory(const CInv& inv)
    {
        if (inv.type == MSG_BLOCK) {
            LogPrint(BCLog::NET, "%s -- adding new inv: %s peer=%d\n", __func__, inv.ToString(), id);
            LOCK(cs_inventory);
            vInventoryBlockToSend.push_back(inv.hash);
            return;
        }
        LOCK(m_tx_relay->cs_tx_inventory);
        if (m_tx_relay->filterInventoryKnown.contains(inv.hash)) {
            LogPrint(BCLog::NET, "%s -- skipping known inv: %s peer=%d\n", __func__, inv.ToString(), id);
            return;
        }
        LogPrint(BCLog::NET, "%s -- adding new inv: %s peer=%d\n", __func__, inv.ToString(), id);
        if (inv.type == MSG_TX || inv.type == MSG_DSTX) {
            m_tx_relay->setInventoryTxToSend.insert(inv.hash);
            return;
        }
        m_tx_relay->vInventoryOtherToSend.push_back(inv);
    }

    void PushBlockHash(const uint256 &hash)
    {
        LOCK(cs_inventory);
        vBlockHashesToAnnounce.push_back(hash);
    }

    void CloseSocketDisconnect(CConnman* connman);

    void copyStats(CNodeStats &stats, const std::vector<bool> &m_asmap);

    ServiceFlags GetLocalServices() const
    {
        return nLocalServices;
    }

    std::string GetAddrName() const;
    //! Sets the addrName only if it was not previously set
    void MaybeSetAddrName(const std::string& addrNameIn);

    std::string GetLogString() const;

    bool CanRelay() const { return !m_masternode_connection || m_masternode_iqr_connection; }

    uint256 GetSentMNAuthChallenge() const {
        LOCK(cs_mnauth);
        return sentMNAuthChallenge;
    }

    uint256 GetReceivedMNAuthChallenge() const {
        LOCK(cs_mnauth);
        return receivedMNAuthChallenge;
    }

    uint256 GetVerifiedProRegTxHash() const {
        LOCK(cs_mnauth);
        return verifiedProRegTxHash;
    }

    uint256 GetVerifiedPubKeyHash() const {
        LOCK(cs_mnauth);
        return verifiedPubKeyHash;
    }

    void SetSentMNAuthChallenge(const uint256& newSentMNAuthChallenge) {
        LOCK(cs_mnauth);
        sentMNAuthChallenge = newSentMNAuthChallenge;
    }

    void SetReceivedMNAuthChallenge(const uint256& newReceivedMNAuthChallenge) {
        LOCK(cs_mnauth);
        receivedMNAuthChallenge = newReceivedMNAuthChallenge;
    }

    void SetVerifiedProRegTxHash(const uint256& newVerifiedProRegTxHash) {
        LOCK(cs_mnauth);
        verifiedProRegTxHash = newVerifiedProRegTxHash;
    }

    void SetVerifiedPubKeyHash(const uint256& newVerifiedPubKeyHash) {
        LOCK(cs_mnauth);
        verifiedPubKeyHash = newVerifiedPubKeyHash;
    }
};

class CExplicitNetCleanup
{
public:
    static void callCleanup();
};

/** Return a timestamp in the future (in microseconds) for exponentially distributed events. */
int64_t PoissonNextSend(int64_t now, int average_interval_seconds);

/** Wrapper to return mockable type */
inline std::chrono::microseconds PoissonNextSend(std::chrono::microseconds now, std::chrono::seconds average_interval)
{
    return std::chrono::microseconds{PoissonNextSend(now.count(), average_interval.count())};
}

#endif // BITCOIN_NET_H
