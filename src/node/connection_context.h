#ifndef BITCOIN_NODE_CONNECTION_CONTEXT
#define BITCOIN_NODE_CONNECTION_CONTEXT

#include <net_permissions.h>
#include <netaddress.h>
#include <node/connection_types.h>
#include <protocol.h>

#include <chrono>
#include <cstdint>

typedef int64_t NodeId;

namespace node {

/** ConnectionContext holds all constant connection data available from the
 * time of connection establishment. */
struct ConnectionContext {
    //! Unique identifier for the connection
    const NodeId id;
    //! Timestamp of connection establishment (pre version handshake)
    const std::chrono::seconds connected;
    //! Address of the connecting peer
    const CAddress addr;
    //! Bind address for the socket of this connection
    const CAddress addr_bind;
    //! Human readable representation of `addr`
    const std::string addr_name;
    //! Net permission flags that apply to the connection (e.g. NoBan)
    const NetPermissionFlags permission_flags{NetPermissionFlags::None};
    //! Connection type (e.g. outbound-full-relay, inbound, etc.)
    const ConnectionType conn_type;
    //! True for inbound onion connections
    const bool is_inbound_onion;

    /** Convenience helper for checking net permissions on this connection. */
    bool HasPermission(NetPermissionFlags permission) const;

    /** Convenience helpers for determining the connection type. */
    bool IsOutboundOrBlockRelayConn() const;
    bool IsFullOutboundConn() const;
    bool IsManualConn() const;
    bool IsBlockOnlyConn() const;
    bool IsFeelerConn() const;
    bool IsAddrFetchConn() const;
    bool IsInboundConn() const;
    std::string ConnectionTypeAsString() const;

    /** Whether we expect certain service flags to be set in the received
     * version message. */
    bool ExpectServicesFromConn() const;

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
};

} // namespace node

#endif // BITCOIN_NODE_CONNECTION_CONTEXT
