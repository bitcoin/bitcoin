#include <net_permissions.h>
#include <netaddress.h>
#include <node/connection_context.h>
#include <node/connection_types.h>
#include <protocol.h>

#include <cstdint>

namespace node {

bool ConnectionContext::HasPermission(NetPermissionFlags permission) const
{
    return NetPermissions::HasFlag(permission_flags, permission);
}

bool ConnectionContext::IsOutboundOrBlockRelayConn() const
{
    switch (conn_type) {
    case ConnectionType::OUTBOUND_FULL_RELAY:
    case ConnectionType::BLOCK_RELAY:
        return true;
    case ConnectionType::INBOUND:
    case ConnectionType::MANUAL:
    case ConnectionType::ADDR_FETCH:
    case ConnectionType::FEELER:
        return false;
    } // no default case, so the compiler can warn about missing cases

    assert(false);
}

bool ConnectionContext::IsFullOutboundConn() const
{
    return conn_type == ConnectionType::OUTBOUND_FULL_RELAY;
}

bool ConnectionContext::IsManualConn() const
{
    return conn_type == ConnectionType::MANUAL;
}

bool ConnectionContext::IsBlockOnlyConn() const
{
    return conn_type == ConnectionType::BLOCK_RELAY;
}

bool ConnectionContext::IsFeelerConn() const
{
    return conn_type == ConnectionType::FEELER;
}

bool ConnectionContext::IsAddrFetchConn() const
{
    return conn_type == ConnectionType::ADDR_FETCH;
}

bool ConnectionContext::IsInboundConn() const
{
    return conn_type == ConnectionType::INBOUND;
}

bool ConnectionContext::ExpectServicesFromConn() const
{
    switch (conn_type) {
    case ConnectionType::INBOUND:
    case ConnectionType::MANUAL:
    case ConnectionType::FEELER:
        return false;
    case ConnectionType::OUTBOUND_FULL_RELAY:
    case ConnectionType::BLOCK_RELAY:
    case ConnectionType::ADDR_FETCH:
        return true;
    } // no default case, so the compiler can warn about missing cases

    assert(false);
}

std::string ConnectionContext::ConnectionTypeAsString() const
{
    return ::ConnectionTypeAsString(conn_type);
}

Network ConnectionContext::ConnectedThroughNetwork() const
{
    return is_inbound_onion ? NET_ONION : addr.GetNetClass();
}

} // namespace node
