// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_H
#define BITCOIN_NET_H

#include <chainparams.h>
#include <compat/compat.h>
#include <crypto/siphash.h>
#include <i2p.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <netbase.h>
#include <netgroup.h>
#include <node/connection.h> // IWYU pragma: export
#include <node/connection_types.h>
#include <protocol.h>
#include <random.h>
#include <span.h>
#include <sync.h>
#include <util/sock.h>
#include <util/threadinterrupt.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

class AddrMan;
class BanMan;
class CScheduler;
struct bilingual_str;

/** Default for -whitelistrelay. */
static const bool DEFAULT_WHITELISTRELAY = true;
/** Default for -whitelistforcerelay. */
static const bool DEFAULT_WHITELISTFORCERELAY = false;

/** Time after which to disconnect, after waiting for a ping response (or inactivity). */
static constexpr std::chrono::minutes TIMEOUT_INTERVAL{20};
/** Run the feeler connection loop once every 2 minutes. **/
static constexpr auto FEELER_INTERVAL = 2min;
/** Run the extra block-relay-only connection loop once every 5 minutes. **/
static constexpr auto EXTRA_BLOCK_RELAY_ONLY_PEER_INTERVAL = 5min;
/** Maximum number of automatic outgoing nodes over which we'll relay everything (blocks, tx, addrs, etc) */
static const int MAX_OUTBOUND_FULL_RELAY_CONNECTIONS = 8;
/** Maximum number of addnode outgoing nodes */
static const int MAX_ADDNODE_CONNECTIONS = 8;
/** Maximum number of block-relay-only outgoing connections */
static const int MAX_BLOCK_RELAY_ONLY_CONNECTIONS = 2;
/** Maximum number of feeler connections */
static const int MAX_FEELER_CONNECTIONS = 1;
/** -listen default */
static const bool DEFAULT_LISTEN = true;
/** The maximum number of peer connections to maintain. */
static const unsigned int DEFAULT_MAX_PEER_CONNECTIONS = 125;
/** The default for -maxuploadtarget. 0 = Unlimited */
static const std::string DEFAULT_MAX_UPLOAD_TARGET{"0M"};
/** Default for blocks only*/
static const bool DEFAULT_BLOCKSONLY = false;
/** -peertimeout default */
static const int64_t DEFAULT_PEER_CONNECT_TIMEOUT = 60;
/** Number of file descriptors required for message capture **/
static const int NUM_FDS_MESSAGE_CAPTURE = 1;

static constexpr bool DEFAULT_FORCEDNSSEED{false};
static constexpr bool DEFAULT_DNSSEED{true};
static constexpr bool DEFAULT_FIXEDSEEDS{true};

static constexpr bool DEFAULT_V2_TRANSPORT{false};

struct AddedNodeParams {
    std::string m_added_node;
    bool m_use_v2transport;
};

struct AddedNodeInfo {
    AddedNodeParams m_params;
    CService resolvedAddress;
    bool fConnected;
    bool fInbound;
};

class CClientUIInterface;

/**
 * Look up IP addresses from all interfaces on the machine and add them to the
 * list of local addresses to self-advertise.
 * The loopback interface is skipped and only the first address from each
 * interface is used.
 */
void Discover();

uint16_t GetListenPort();

enum
{
    LOCAL_NONE,   // unknown
    LOCAL_IF,     // address a local interface listens on
    LOCAL_BIND,   // address explicit bound to
    LOCAL_MAPPED, // address reported by UPnP or NAT-PMP
    LOCAL_MANUAL, // address explicitly specified (-externalip=)

    LOCAL_MAX
};

/** Returns a local address that we should advertise to this peer. */
std::optional<CService> GetLocalAddrForPeer(CNode& node);

bool AddLocal(const CService& addr, int nScore = LOCAL_NONE);
bool AddLocal(const CNetAddr& addr, int nScore = LOCAL_NONE);
void RemoveLocal(const CService& addr);
bool SeenLocal(const CService& addr);
bool IsLocal(const CService& addr);
CService GetLocalAddress(const CNode& peer);

extern bool fDiscover;
extern bool fListen;

/** Subversion as sent to the P2P network in `version` messages */
extern std::string strSubVersion;

struct LocalServiceInfo {
    int nScore;
    uint16_t nPort;
};

extern GlobalMutex g_maplocalhost_mutex;
extern std::map<CNetAddr, LocalServiceInfo> mapLocalHost GUARDED_BY(g_maplocalhost_mutex);

/**
 * Interface for message handling
 */
class NetEventsInterface
{
public:
    /** Mutex for anything that is only accessed via the msg processing thread */
    static Mutex g_msgproc_mutex;

    /** Initialize a peer (setup state, queue any initial messages) */
    virtual void InitializeNode(CNode& node, ServiceFlags our_services) = 0;

    /** Handle removal of a peer (clear state) */
    virtual void FinalizeNode(const CNode& node) = 0;

    /**
    * Process protocol messages received from a given node
    *
    * @param[in]   pnode           The node which we have received messages from.
    * @param[in]   interrupt       Interrupt condition for processing threads
    * @return                      True if there is more work to be done
    */
    virtual bool ProcessMessages(CNode* pnode, std::atomic<bool>& interrupt) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex) = 0;

    /**
    * Send queued protocol messages to a given node.
    *
    * @param[in]   pnode           The node which we are sending messages to.
    * @return                      True if there is more work to be done
    */
    virtual bool SendMessages(CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex) = 0;


protected:
    /**
     * Protected destructor so that instances can only be deleted by derived classes.
     * If that restriction is no longer desired, this should be made public and virtual.
     */
    ~NetEventsInterface() = default;
};

class CConnman
{
public:

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
        /// True if the user did not specify -bind= or -whitebind= and thus
        /// we should bind on `0.0.0.0` (IPv4) and `::` (IPv6).
        bool bind_on_any;
        bool m_use_addrman_outgoing = true;
        std::vector<std::string> m_specified_outgoing;
        std::vector<std::string> m_added_nodes;
        bool m_i2p_accept_incoming;
    };

    void Init(const Options& connOptions) EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex, !m_total_bytes_sent_mutex)
    {
        AssertLockNotHeld(m_total_bytes_sent_mutex);

        nLocalServices = connOptions.nLocalServices;
        nMaxConnections = connOptions.nMaxConnections;
        m_max_outbound_full_relay = std::min(connOptions.m_max_outbound_full_relay, connOptions.nMaxConnections);
        m_max_outbound_block_relay = connOptions.m_max_outbound_block_relay;
        m_use_addrman_outgoing = connOptions.m_use_addrman_outgoing;
        nMaxAddnode = connOptions.nMaxAddnode;
        nMaxFeeler = connOptions.nMaxFeeler;
        m_max_outbound = m_max_outbound_full_relay + m_max_outbound_block_relay + nMaxFeeler;
        m_client_interface = connOptions.uiInterface;
        m_banman = connOptions.m_banman;
        m_msgproc = connOptions.m_msgproc;
        nSendBufferMaxSize = connOptions.nSendBufferMaxSize;
        nReceiveFloodSize = connOptions.nReceiveFloodSize;
        m_peer_connect_timeout = std::chrono::seconds{connOptions.m_peer_connect_timeout};
        {
            LOCK(m_total_bytes_sent_mutex);
            nMaxOutboundLimit = connOptions.nMaxOutboundLimit;
        }
        vWhitelistedRange = connOptions.vWhitelistedRange;
        {
            LOCK(m_added_nodes_mutex);

            for (const std::string& added_node : connOptions.m_added_nodes) {
                // -addnode cli arg does not currently have a way to signal BIP324 support
                m_added_node_params.push_back({added_node, false});
            }
        }
        m_onion_binds = connOptions.onion_binds;
    }

    CConnman(uint64_t seed0, uint64_t seed1, AddrMan& addrman, const NetGroupManager& netgroupman,
             const CChainParams& params, bool network_active = true);

    ~CConnman();

    bool Start(CScheduler& scheduler, const Options& options) EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex, !m_added_nodes_mutex, !m_addr_fetches_mutex, !mutexMsgProc);

    void StopThreads();
    void StopNodes();
    void Stop()
    {
        StopThreads();
        StopNodes();
    };

    void Interrupt() EXCLUSIVE_LOCKS_REQUIRED(!mutexMsgProc);
    bool GetNetworkActive() const { return fNetworkActive; };
    bool GetUseAddrmanOutgoing() const { return m_use_addrman_outgoing; };
    void SetNetworkActive(bool active);
    void OpenNetworkConnection(const CAddress& addrConnect, bool fCountFailure, CSemaphoreGrant&& grant_outbound, const char* strDest, ConnectionType conn_type, bool use_v2transport) EXCLUSIVE_LOCKS_REQUIRED(!m_unused_i2p_sessions_mutex);
    bool CheckIncomingNonce(uint64_t nonce);

    // alias for thread safety annotations only, not defined
    RecursiveMutex& GetNodesMutex() const LOCK_RETURNED(m_nodes_mutex);

    bool ForNode(NodeId id, std::function<bool(CNode* pnode)> func);

    void PushMessage(CNode* pnode, CSerializedNetMsg&& msg) EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);

    using NodeFn = std::function<void(CNode*)>;
    void ForEachNode(const NodeFn& func)
    {
        LOCK(m_nodes_mutex);
        for (auto&& node : m_nodes) {
            if (NodeFullyConnected(node))
                func(node);
        }
    };

    void ForEachNode(const NodeFn& func) const
    {
        LOCK(m_nodes_mutex);
        for (auto&& node : m_nodes) {
            if (NodeFullyConnected(node))
                func(node);
        }
    };

    // Addrman functions
    /**
     * Return all or many randomly selected addresses, optionally by network.
     *
     * @param[in] max_addresses  Maximum number of addresses to return (0 = all).
     * @param[in] max_pct        Maximum percentage of addresses to return (0 = all).
     * @param[in] network        Select only addresses of this network (nullopt = all).
     */
    std::vector<CAddress> GetAddresses(size_t max_addresses, size_t max_pct, std::optional<Network> network) const;
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
    bool GetTryNewOutboundPeer() const;

    void StartExtraBlockRelayPeers();

    // Return the number of outbound peers we have in excess of our target (eg,
    // if we previously called SetTryNewOutboundPeer(true), and have since set
    // to false, we may have extra peers that we wish to disconnect). This may
    // return a value less than (num_outbound_connections - num_outbound_slots)
    // in cases where some outbound connections are not yet fully connected, or
    // not yet fully disconnected.
    int GetExtraFullOutboundCount() const;
    // Count the number of block-relay-only peers we have over our limit.
    int GetExtraBlockRelayCount() const;

    bool AddNode(const AddedNodeParams& add) EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex);
    bool RemoveAddedNode(const std::string& node) EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex);
    std::vector<AddedNodeInfo> GetAddedNodeInfo() const EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex);

    /**
     * Attempts to open a connection. Currently only used from tests.
     *
     * @param[in]   address     Address of node to try connecting to
     * @param[in]   conn_type   ConnectionType::OUTBOUND, ConnectionType::BLOCK_RELAY,
     *                          ConnectionType::ADDR_FETCH or ConnectionType::FEELER
     * @return      bool        Returns false if there are no available
     *                          slots for this connection:
     *                          - conn_type not a supported ConnectionType
     *                          - Max total outbound connection capacity filled
     *                          - Max connection capacity for type is filled
     */
    bool AddConnection(const std::string& address, ConnectionType conn_type) EXCLUSIVE_LOCKS_REQUIRED(!m_unused_i2p_sessions_mutex);

    size_t GetNodeCount(ConnectionDirection) const;
    uint32_t GetMappedAS(const CNetAddr& addr) const;
    void GetNodeStats(std::vector<CNodeStats>& vstats) const;
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

    uint64_t GetMaxOutboundTarget() const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);
    std::chrono::seconds GetMaxOutboundTimeframe() const;

    //! check if the outbound target is reached
    //! if param historicalBlockServingLimit is set true, the function will
    //! response true if the limit for serving historical blocks has been reached
    bool OutboundTargetReached(bool historicalBlockServingLimit) const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);

    //! response the bytes left in the current max outbound cycle
    //! in case of no limit, it will always response 0
    uint64_t GetOutboundTargetBytesLeft() const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);

    std::chrono::seconds GetMaxOutboundTimeLeftInCycle() const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);

    uint64_t GetTotalBytesRecv() const;
    uint64_t GetTotalBytesSent() const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);

    /** Get a unique deterministic randomizer. */
    CSipHasher GetDeterministicRandomizer(uint64_t id) const;

    void WakeMessageHandler() EXCLUSIVE_LOCKS_REQUIRED(!mutexMsgProc);

    /** Return true if we should disconnect the peer for failing an inactivity check. */
    bool ShouldRunInactivityChecks(const CNode& node, std::chrono::seconds now) const;

    bool MultipleManualOrFullOutboundConns(Network net) const EXCLUSIVE_LOCKS_REQUIRED(m_nodes_mutex);

private:
    struct ListenSocket {
    public:
        std::shared_ptr<Sock> sock;
        inline void AddSocketPermissionFlags(NetPermissionFlags& flags) const { NetPermissions::AddFlag(flags, m_permissions); }
        ListenSocket(std::shared_ptr<Sock> sock_, NetPermissionFlags permissions_)
            : sock{sock_}, m_permissions{permissions_}
        {
        }

    private:
        NetPermissionFlags m_permissions;
    };

    //! returns the time left in the current max outbound cycle
    //! in case of no limit, it will always return 0
    std::chrono::seconds GetMaxOutboundTimeLeftInCycle_() const EXCLUSIVE_LOCKS_REQUIRED(m_total_bytes_sent_mutex);

    bool BindListenPort(const CService& bindAddr, bilingual_str& strError, NetPermissionFlags permissions);
    bool Bind(const CService& addr, unsigned int flags, NetPermissionFlags permissions);
    bool InitBinds(const Options& options);

    void ThreadOpenAddedConnections() EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex, !m_unused_i2p_sessions_mutex, !m_reconnections_mutex);
    void AddAddrFetch(const std::string& strDest) EXCLUSIVE_LOCKS_REQUIRED(!m_addr_fetches_mutex);
    void ProcessAddrFetch() EXCLUSIVE_LOCKS_REQUIRED(!m_addr_fetches_mutex, !m_unused_i2p_sessions_mutex);
    void ThreadOpenConnections(std::vector<std::string> connect) EXCLUSIVE_LOCKS_REQUIRED(!m_addr_fetches_mutex, !m_added_nodes_mutex, !m_nodes_mutex, !m_unused_i2p_sessions_mutex, !m_reconnections_mutex);
    void ThreadMessageHandler() EXCLUSIVE_LOCKS_REQUIRED(!mutexMsgProc);
    void ThreadI2PAcceptIncoming();
    void AcceptConnection(const ListenSocket& hListenSocket);

    /**
     * Create a `CNode` object from a socket that has just been accepted and add the node to
     * the `m_nodes` member.
     * @param[in] sock Connected socket to communicate with the peer.
     * @param[in] permission_flags The peer's permissions.
     * @param[in] addr_bind The address and port at our side of the connection.
     * @param[in] addr The address and port at the peer's side of the connection.
     */
    void CreateNodeFromAcceptedSocket(std::unique_ptr<Sock>&& sock,
                                      NetPermissionFlags permission_flags,
                                      const CAddress& addr_bind,
                                      const CAddress& addr);

    void DisconnectNodes() EXCLUSIVE_LOCKS_REQUIRED(!m_reconnections_mutex, !m_nodes_mutex);
    void NotifyNumConnectionsChanged();
    /** Return true if the peer is inactive and should be disconnected. */
    bool InactivityCheck(const CNode& node) const;

    /**
     * Generate a collection of sockets to check for IO readiness.
     * @param[in] nodes Select from these nodes' sockets.
     * @return sockets to check for readiness
     */
    Sock::EventsPerSock GenerateWaitSockets(Span<CNode* const> nodes);

    /**
     * Check connected and listening sockets for IO readiness and process them accordingly.
     */
    void SocketHandler() EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex, !mutexMsgProc);

    /**
     * Do the read/write for connected sockets that are ready for IO.
     * @param[in] nodes Nodes to process. The socket of each node is checked against `what`.
     * @param[in] events_per_sock Sockets that are ready for IO.
     */
    void SocketHandlerConnected(const std::vector<CNode*>& nodes,
                                const Sock::EventsPerSock& events_per_sock)
        EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex, !mutexMsgProc);

    /**
     * Accept incoming connections, one from each read-ready listening socket.
     * @param[in] events_per_sock Sockets that are ready for IO.
     */
    void SocketHandlerListening(const Sock::EventsPerSock& events_per_sock);

    void ThreadSocketHandler() EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex, !mutexMsgProc, !m_nodes_mutex, !m_reconnections_mutex);
    void ThreadDNSAddressSeed() EXCLUSIVE_LOCKS_REQUIRED(!m_addr_fetches_mutex, !m_nodes_mutex);

    uint64_t CalculateKeyedNetGroup(const CAddress& ad) const;

    CNode* FindNode(const CNetAddr& ip);
    CNode* FindNode(const std::string& addrName);
    CNode* FindNode(const CService& addr);

    /**
     * Determine whether we're already connected to a given address, in order to
     * avoid initiating duplicate connections.
     */
    bool AlreadyConnectedToAddress(const CAddress& addr);

    bool AttemptToEvictConnection();
    CNode* ConnectNode(CAddress addrConnect, const char *pszDest, bool fCountFailure, ConnectionType conn_type, bool use_v2transport) EXCLUSIVE_LOCKS_REQUIRED(!m_unused_i2p_sessions_mutex);
    void AddWhitelistPermissionFlags(NetPermissionFlags& flags, const CNetAddr &addr) const;

    void DeleteNode(CNode* pnode);

    NodeId GetNewNodeId();

    /** (Try to) send data from node's vSendMsg. Returns (bytes_sent, data_left). */
    std::pair<size_t, bool> SocketSendData(CNode& node) const EXCLUSIVE_LOCKS_REQUIRED(node.cs_vSend);

    void DumpAddresses();

    // Network stats
    void RecordBytesRecv(uint64_t bytes);
    void RecordBytesSent(uint64_t bytes) EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);

    /**
     Return reachable networks for which we have no addresses in addrman and therefore
     may require loading fixed seeds.
     */
    std::unordered_set<Network> GetReachableEmptyNetworks() const;

    /**
     * Return vector of current BLOCK_RELAY peers.
     */
    std::vector<CAddress> GetCurrentBlockRelayOnlyConns() const;

    /**
     * Search for a "preferred" network, a reachable network to which we
     * currently don't have any OUTBOUND_FULL_RELAY or MANUAL connections.
     * There needs to be at least one address in AddrMan for a preferred
     * network to be picked.
     *
     * @param[out]    network        Preferred network, if found.
     *
     * @return           bool        Whether a preferred network was found.
     */
    bool MaybePickPreferredNetwork(std::optional<Network>& network);

    // Whether the node should be passed out in ForEach* callbacks
    static bool NodeFullyConnected(const CNode* pnode);

    uint16_t GetDefaultPort(Network net) const;
    uint16_t GetDefaultPort(const std::string& addr) const;

    // Network usage totals
    mutable Mutex m_total_bytes_sent_mutex;
    std::atomic<uint64_t> nTotalBytesRecv{0};
    uint64_t nTotalBytesSent GUARDED_BY(m_total_bytes_sent_mutex) {0};

    // outbound limit & stats
    uint64_t nMaxOutboundTotalBytesSentInCycle GUARDED_BY(m_total_bytes_sent_mutex) {0};
    std::chrono::seconds nMaxOutboundCycleStartTime GUARDED_BY(m_total_bytes_sent_mutex) {0};
    uint64_t nMaxOutboundLimit GUARDED_BY(m_total_bytes_sent_mutex);

    // P2P timeout in seconds
    std::chrono::seconds m_peer_connect_timeout;

    // Whitelisted ranges. Any node connecting from these is automatically
    // whitelisted (as well as those connecting to whitelisted binds).
    std::vector<NetWhitelistPermissions> vWhitelistedRange;

    unsigned int nSendBufferMaxSize{0};
    unsigned int nReceiveFloodSize{0};

    std::vector<ListenSocket> vhListenSocket;
    std::atomic<bool> fNetworkActive{true};
    bool fAddressesInitialized{false};
    AddrMan& addrman;
    const NetGroupManager& m_netgroupman;
    std::deque<std::string> m_addr_fetches GUARDED_BY(m_addr_fetches_mutex);
    Mutex m_addr_fetches_mutex;

    // connection string and whether to use v2 p2p
    std::vector<AddedNodeParams> m_added_node_params GUARDED_BY(m_added_nodes_mutex);

    mutable Mutex m_added_nodes_mutex;
    std::vector<CNode*> m_nodes GUARDED_BY(m_nodes_mutex);
    std::list<CNode*> m_nodes_disconnected;
    mutable RecursiveMutex m_nodes_mutex;
    std::atomic<NodeId> nLastNodeId{0};
    unsigned int nPrevNodeCount{0};

    // Stores number of full-tx connections (outbound and manual) per network
    std::array<unsigned int, Network::NET_MAX> m_network_conn_counts GUARDED_BY(m_nodes_mutex) = {};

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
     * Services this node offers.
     *
     * This data is replicated in each Peer instance we create.
     *
     * This data is not marked const, but after being set it should not
     * change.
     *
     * \sa Peer::our_services
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
    CClientUIInterface* m_client_interface;
    NetEventsInterface* m_msgproc;
    /** Pointer to this node's banman. May be nullptr - check existence before dereferencing. */
    BanMan* m_banman;

    /**
     * Addresses that were saved during the previous clean shutdown. We'll
     * attempt to make block-relay-only connections to them.
     */
    std::vector<CAddress> m_anchors;

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
     * Used to accept incoming and make outgoing I2P connections from a persistent
     * address.
     */
    std::unique_ptr<i2p::sam::Session> m_i2p_sam_session;

    std::thread threadDNSAddressSeed;
    std::thread threadSocketHandler;
    std::thread threadOpenAddedConnections;
    std::thread threadOpenConnections;
    std::thread threadMessageHandler;
    std::thread threadI2PAcceptIncoming;

    /** flag for deciding to connect to an extra outbound peer,
     *  in excess of m_max_outbound_full_relay
     *  This takes the place of a feeler connection */
    std::atomic_bool m_try_another_outbound_peer;

    /** flag for initiating extra block-relay-only peer connections.
     *  this should only be enabled after initial chain sync has occurred,
     *  as these connections are intended to be short-lived and low-bandwidth.
     */
    std::atomic_bool m_start_extra_block_relay_peers{false};

    /**
     * A vector of -bind=<address>:<port>=onion arguments each of which is
     * an address and port that are designated for incoming Tor connections.
     */
    std::vector<CService> m_onion_binds;

    /**
     * Mutex protecting m_i2p_sam_sessions.
     */
    Mutex m_unused_i2p_sessions_mutex;

    /**
     * A pool of created I2P SAM transient sessions that should be used instead
     * of creating new ones in order to reduce the load on the I2P network.
     * Creating a session in I2P is not cheap, thus if this is not empty, then
     * pick an entry from it instead of creating a new session. If connecting to
     * a host fails, then the created session is put to this pool for reuse.
     */
    std::queue<std::unique_ptr<i2p::sam::Session>> m_unused_i2p_sessions GUARDED_BY(m_unused_i2p_sessions_mutex);

    /**
     * Mutex protecting m_reconnections.
     */
    Mutex m_reconnections_mutex;

    /** Struct for entries in m_reconnections. */
    struct ReconnectionInfo
    {
        CAddress addr_connect;
        CSemaphoreGrant grant;
        std::string destination;
        ConnectionType conn_type;
        bool use_v2transport;
    };

    /**
     * List of reconnections we have to make.
     */
    std::list<ReconnectionInfo> m_reconnections GUARDED_BY(m_reconnections_mutex);

    /** Attempt reconnections, if m_reconnections non-empty. */
    void PerformReconnections() EXCLUSIVE_LOCKS_REQUIRED(!m_reconnections_mutex, !m_unused_i2p_sessions_mutex);

    /**
     * Cap on the size of `m_unused_i2p_sessions`, to ensure it does not
     * unexpectedly use too much memory.
     */
    static constexpr size_t MAX_UNUSED_I2P_SESSIONS_SIZE{10};

    /**
     * RAII helper to atomically create a copy of `m_nodes` and add a reference
     * to each of the nodes. The nodes are released when this object is destroyed.
     */
    class NodesSnapshot
    {
    public:
        explicit NodesSnapshot(const CConnman& connman, bool shuffle)
        {
            {
                LOCK(connman.m_nodes_mutex);
                m_nodes_copy = connman.m_nodes;
                for (auto& node : m_nodes_copy) {
                    node->AddRef();
                }
            }
            if (shuffle) {
                Shuffle(m_nodes_copy.begin(), m_nodes_copy.end(), FastRandomContext{});
            }
        }

        ~NodesSnapshot()
        {
            for (auto& node : m_nodes_copy) {
                node->Release();
            }
        }

        const std::vector<CNode*>& Nodes() const
        {
            return m_nodes_copy;
        }

    private:
        std::vector<CNode*> m_nodes_copy;
    };

    const CChainParams& m_params;

    friend struct ConnmanTestMsg;
};

/** Defaults to `CaptureMessageToFile()`, but can be overridden by unit tests. */
extern std::function<void(const CAddress& addr,
                          const std::string& msg_type,
                          Span<const unsigned char> data,
                          bool is_incoming)>
    CaptureMessage;

#endif // BITCOIN_NET_H
