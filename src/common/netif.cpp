// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <common/netif.h>

#include <logging.h>
#include <netbase.h>
#include <util/check.h>
#include <util/sock.h>
#include <util/syserror.h>

#if defined(__linux__)
#include <linux/rtnetlink.h>
#elif defined(__FreeBSD__)
#include <osreldate.h>
#if __FreeBSD_version >= 1400000
// Workaround https://github.com/freebsd/freebsd-src/pull/1070.
#define typeof __typeof
#include <netlink/netlink.h>
#include <netlink/netlink_route.h>
#endif
#elif defined(WIN32)
#include <iphlpapi.h>
#elif defined(__APPLE__)
#include <net/route.h>
#include <sys/sysctl.h>
#endif

#ifdef HAVE_IFADDRS
#include <sys/types.h>
#include <ifaddrs.h>
#endif

namespace {

//! Return CNetAddr for the specified OS-level network address.
//! If a length is not given, it is taken to be sizeof(struct sockaddr_*) for the family.
std::optional<CNetAddr> FromSockAddr(const struct sockaddr* addr, std::optional<socklen_t> sa_len_opt)
{
    socklen_t sa_len = 0;
    if (sa_len_opt.has_value()) {
        sa_len = *sa_len_opt;
    } else {
        // If sockaddr length was not specified, determine it from the family.
        switch (addr->sa_family) {
        case AF_INET: sa_len = sizeof(struct sockaddr_in); break;
        case AF_INET6: sa_len = sizeof(struct sockaddr_in6); break;
        default:
            return std::nullopt;
        }
    }
    // Fill in a CService from the sockaddr, then drop the port part.
    CService service;
    if (service.SetSockAddr(addr, sa_len)) {
        return (CNetAddr)service;
    }
    return std::nullopt;
}

// Linux and FreeBSD 14.0+. For FreeBSD 13.2 the code can be compiled but
// running it requires loading a special kernel module, otherwise socket(AF_NETLINK,...)
// will fail, so we skip that.
#if defined(__linux__) || (defined(__FreeBSD__) && __FreeBSD_version >= 1400000)

// Good for responses containing ~ 10,000-15,000 routes.
static constexpr ssize_t NETLINK_MAX_RESPONSE_SIZE{1'048'576};

std::optional<CNetAddr> QueryDefaultGatewayImpl(sa_family_t family)
{
    // Create a netlink socket.
    auto sock{CreateSock(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)};
    if (!sock) {
        LogError("socket(AF_NETLINK): %s\n", NetworkErrorString(errno));
        return std::nullopt;
    }

    // Send request.
    struct {
        nlmsghdr hdr; ///< Request header.
        rtmsg data; ///< Request data, a "route message".
        nlattr dst_hdr; ///< One attribute, conveying the route destination address.
        char dst_data[16]; ///< Route destination address. To query the default route we use 0.0.0.0/0 or [::]/0. For IPv4 the first 4 bytes are used.
    } request{};

    // Whether to use the first 4 or 16 bytes from request.dst_data.
    const size_t dst_data_len = family == AF_INET ? 4 : 16;

    request.hdr.nlmsg_type = RTM_GETROUTE;
    request.hdr.nlmsg_flags = NLM_F_REQUEST;
#ifdef __linux__
    // Linux IPv4 / IPv6 - this must be present, otherwise no gateway is found
    // FreeBSD IPv4 - does not matter, the gateway is found with or without this
    // FreeBSD IPv6 - this must be absent, otherwise no gateway is found
    request.hdr.nlmsg_flags |= NLM_F_DUMP;
#endif
    request.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(rtmsg) + sizeof(nlattr) + dst_data_len);
    request.hdr.nlmsg_seq = 0; // Sequence number, used to match which reply is to which request. Irrelevant for us because we send just one request.
    request.data.rtm_family = family;
    request.data.rtm_dst_len = 0; // Prefix length.
#ifdef __FreeBSD__
    // Linux IPv4 / IPv6 this must be absent, otherwise no gateway is found
    // FreeBSD IPv4 - does not matter, the gateway is found with or without this
    // FreeBSD IPv6 - this must be present, otherwise no gateway is found
    request.data.rtm_flags = RTM_F_PREFIX;
#endif
    request.dst_hdr.nla_type = RTA_DST;
    request.dst_hdr.nla_len = sizeof(nlattr) + dst_data_len;

    if (sock->Send(&request, request.hdr.nlmsg_len, 0) != static_cast<ssize_t>(request.hdr.nlmsg_len)) {
        LogError("send() to netlink socket: %s\n", NetworkErrorString(errno));
        return std::nullopt;
    }

    // Receive response.
    char response[4096];
    ssize_t total_bytes_read{0};
    bool done{false};
    while (!done) {
        int64_t recv_result;
        do {
            recv_result = sock->Recv(response, sizeof(response), 0);
        } while (recv_result < 0 && (errno == EINTR || errno == EAGAIN));
        if (recv_result < 0) {
            LogError("recv() from netlink socket: %s\n", NetworkErrorString(errno));
            return std::nullopt;
        }

        total_bytes_read += recv_result;
        if (total_bytes_read > NETLINK_MAX_RESPONSE_SIZE) {
            LogWarning("Netlink response exceeded size limit (%zu bytes, family=%d)\n", NETLINK_MAX_RESPONSE_SIZE, family);
            return std::nullopt;
        }

#if defined(__FreeBSD_version) && __FreeBSD_version >= 1500029
        using recv_result_t = size_t;
#else
        using recv_result_t = int64_t;
#endif
        for (nlmsghdr* hdr = (nlmsghdr*)response; NLMSG_OK(hdr, static_cast<recv_result_t>(recv_result)); hdr = NLMSG_NEXT(hdr, recv_result)) {
            if (!(hdr->nlmsg_flags & NLM_F_MULTI)) {
                done = true;
            }

            if (hdr->nlmsg_type == NLMSG_DONE) {
                done = true;
                break;
            }

            rtmsg* r = (rtmsg*)NLMSG_DATA(hdr);
            int remaining_len = RTM_PAYLOAD(hdr);

            if (hdr->nlmsg_type != RTM_NEWROUTE) {
                continue; // Skip non-route messages
            }

            // Only consider default routes (destination prefix length of 0).
            if (r->rtm_dst_len != 0) {
                continue;
            }

            // Iterate over the attributes.
            rtattr* rta_gateway = nullptr;
            int scope_id = 0;
            for (rtattr* attr = RTM_RTA(r); RTA_OK(attr, remaining_len); attr = RTA_NEXT(attr, remaining_len)) {
                if (attr->rta_type == RTA_GATEWAY) {
                    rta_gateway = attr;
                } else if (attr->rta_type == RTA_OIF && sizeof(int) == RTA_PAYLOAD(attr)) {
                    std::memcpy(&scope_id, RTA_DATA(attr), sizeof(scope_id));
                }
            }

            // Found gateway?
            if (rta_gateway != nullptr) {
                if (family == AF_INET && sizeof(in_addr) == RTA_PAYLOAD(rta_gateway)) {
                    in_addr gw;
                    std::memcpy(&gw, RTA_DATA(rta_gateway), sizeof(gw));
                    return CNetAddr(gw);
                } else if (family == AF_INET6 && sizeof(in6_addr) == RTA_PAYLOAD(rta_gateway)) {
                    in6_addr gw;
                    std::memcpy(&gw, RTA_DATA(rta_gateway), sizeof(gw));
                    return CNetAddr(gw, scope_id);
                }
            }
        }
    }

    return std::nullopt;
}

#elif defined(WIN32)

std::optional<CNetAddr> QueryDefaultGatewayImpl(sa_family_t family)
{
    NET_LUID interface_luid = {};
    SOCKADDR_INET destination_address = {};
    MIB_IPFORWARD_ROW2 best_route = {};
    SOCKADDR_INET best_source_address = {};
    DWORD best_if_idx = 0;
    DWORD status = 0;

    // Pass empty destination address of the requested type (:: or 0.0.0.0) to get interface of default route.
    destination_address.si_family = family;
    status = GetBestInterfaceEx((sockaddr*)&destination_address, &best_if_idx);
    if (status != NO_ERROR) {
        LogError("Could not get best interface for default route: %s\n", NetworkErrorString(status));
        return std::nullopt;
    }

    // Get best route to default gateway.
    // Leave interface_luid at all-zeros to use interface index instead.
    status = GetBestRoute2(&interface_luid, best_if_idx, nullptr, &destination_address, 0, &best_route, &best_source_address);
    if (status != NO_ERROR) {
        LogError("Could not get best route for default route for interface index %d: %s\n",
                best_if_idx, NetworkErrorString(status));
        return std::nullopt;
    }

    Assume(best_route.NextHop.si_family == family);
    if (family == AF_INET) {
        return CNetAddr(best_route.NextHop.Ipv4.sin_addr);
    } else if(family == AF_INET6) {
        return CNetAddr(best_route.NextHop.Ipv6.sin6_addr, best_route.InterfaceIndex);
    }
    return std::nullopt;
}

#elif defined(__APPLE__)

#define ROUNDUP32(a) \
    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(uint32_t) - 1))) : sizeof(uint32_t))

//! MacOS: Get default gateway from route table. See route(4) for the format.
std::optional<CNetAddr> QueryDefaultGatewayImpl(sa_family_t family)
{
    // net.route.0.inet[6].flags.gateway
    int mib[] = {CTL_NET, PF_ROUTE, 0, family, NET_RT_FLAGS, RTF_GATEWAY};
    // The size of the available data is determined by calling sysctl() with oldp=nullptr. See sysctl(3).
    size_t l = 0;
    if (sysctl(/*name=*/mib, /*namelen=*/sizeof(mib) / sizeof(int), /*oldp=*/nullptr, /*oldlenp=*/&l, /*newp=*/nullptr, /*newlen=*/0) < 0) {
        LogError("Could not get sysctl length of routing table: %s\n", SysErrorString(errno));
        return std::nullopt;
    }
    std::vector<std::byte> buf(l);
    if (sysctl(/*name=*/mib, /*namelen=*/sizeof(mib) / sizeof(int), /*oldp=*/buf.data(), /*oldlenp=*/&l, /*newp=*/nullptr, /*newlen=*/0) < 0) {
        LogError("Could not get sysctl data of routing table: %s\n", SysErrorString(errno));
        return std::nullopt;
    }
    // Iterate over messages (each message is a routing table entry).
    for (size_t msg_pos = 0; msg_pos < buf.size(); ) {
        if ((msg_pos + sizeof(rt_msghdr)) > buf.size()) return std::nullopt;
        const struct rt_msghdr* rt = (const struct rt_msghdr*)(buf.data() + msg_pos);
        const size_t next_msg_pos = msg_pos + rt->rtm_msglen;
        if (rt->rtm_msglen < sizeof(rt_msghdr) || next_msg_pos > buf.size()) return std::nullopt;
        // Iterate over addresses within message, get destination and gateway (if present).
        // Address data starts after header.
        size_t sa_pos = msg_pos + sizeof(struct rt_msghdr);
        std::optional<CNetAddr> dst, gateway;
        for (int i = 0; i < RTAX_MAX; i++) {
            if (rt->rtm_addrs & (1 << i)) {
                // 2 is just sa_len + sa_family, the theoretical minimum size of a socket address.
                if ((sa_pos + 2) > next_msg_pos) return std::nullopt;
                const struct sockaddr* sa = (const struct sockaddr*)(buf.data() + sa_pos);
                if ((sa_pos + sa->sa_len) > next_msg_pos) return std::nullopt;
                if (i == RTAX_DST) {
                    dst = FromSockAddr(sa, sa->sa_len);
                } else if (i == RTAX_GATEWAY) {
                    gateway = FromSockAddr(sa, sa->sa_len);
                }
                // Skip sockaddr entries for bit flags we're not interested in,
                // move cursor.
                sa_pos += ROUNDUP32(sa->sa_len);
            }
        }
        // Found default gateway?
        if (dst && gateway && dst->IsBindAny()) { // Route to 0.0.0.0 or :: ?
            return *gateway;
        }
        // Skip to next message.
        msg_pos = next_msg_pos;
    }
    return std::nullopt;
}

#else

// Dummy implementation.
std::optional<CNetAddr> QueryDefaultGatewayImpl(sa_family_t)
{
    return std::nullopt;
}

#endif

}

std::optional<CNetAddr> QueryDefaultGateway(Network network)
{
    Assume(network == NET_IPV4 || network == NET_IPV6);

    sa_family_t family;
    if (network == NET_IPV4) {
        family = AF_INET;
    } else if(network == NET_IPV6) {
        family = AF_INET6;
    } else {
        return std::nullopt;
    }

    std::optional<CNetAddr> ret = QueryDefaultGatewayImpl(family);

    // It's possible for the default gateway to be 0.0.0.0 or ::0 on at least Windows
    // for some routing strategies. If so, return as if no default gateway was found.
    if (ret && !ret->IsBindAny()) {
        return ret;
    } else {
        return std::nullopt;
    }
}

std::vector<CNetAddr> GetLocalAddresses()
{
    std::vector<CNetAddr> addresses;
#ifdef WIN32
    DWORD status = 0;
    constexpr size_t MAX_ADAPTER_ADDR_SIZE = 4 * 1000 * 1000; // Absolute maximum size of adapter addresses structure we're willing to handle, as a precaution.
    std::vector<std::byte> out_buf(15000, {}); // Start with 15KB allocation as recommended in GetAdaptersAddresses documentation.
    while (true) {
        ULONG out_buf_len = out_buf.size();
        status = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
                nullptr, reinterpret_cast<PIP_ADAPTER_ADDRESSES>(out_buf.data()), &out_buf_len);
        if (status == ERROR_BUFFER_OVERFLOW && out_buf.size() < MAX_ADAPTER_ADDR_SIZE) {
            // If status == ERROR_BUFFER_OVERFLOW, out_buf_len will contain the needed size.
            // Unfortunately, this cannot be fully relied on, because another process may have added interfaces.
            // So to avoid getting stuck due to a race condition, double the buffer size at least
            // once before retrying (but only up to the maximum allowed size).
            out_buf.resize(std::min(std::max<size_t>(out_buf_len, out_buf.size()) * 2, MAX_ADAPTER_ADDR_SIZE));
        } else {
            break;
        }
    }

    if (status != NO_ERROR) {
        // This includes ERROR_NO_DATA if there are no addresses and thus there's not even one PIP_ADAPTER_ADDRESSES
        // record in the returned structure.
        LogError("Could not get local adapter addresses: %s\n", NetworkErrorString(status));
        return addresses;
    }

    // Iterate over network adapters.
    for (PIP_ADAPTER_ADDRESSES cur_adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(out_buf.data());
         cur_adapter != nullptr; cur_adapter = cur_adapter->Next) {
        if (cur_adapter->OperStatus != IfOperStatusUp) continue;
        if (cur_adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

        // Iterate over unicast addresses for adapter, the only address type we're interested in.
        for (PIP_ADAPTER_UNICAST_ADDRESS cur_address = cur_adapter->FirstUnicastAddress;
             cur_address != nullptr; cur_address = cur_address->Next) {
            // "The IP address is a cluster address and should not be used by most applications."
            if ((cur_address->Flags & IP_ADAPTER_ADDRESS_TRANSIENT) != 0) continue;

            if (std::optional<CNetAddr> addr = FromSockAddr(cur_address->Address.lpSockaddr, static_cast<socklen_t>(cur_address->Address.iSockaddrLength))) {
                addresses.push_back(*addr);
            }
        }
    }
#elif defined(HAVE_IFADDRS)
    struct ifaddrs* myaddrs;
    if (getifaddrs(&myaddrs) == 0) {
        for (struct ifaddrs* ifa = myaddrs; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == nullptr) continue;
            if ((ifa->ifa_flags & IFF_UP) == 0) continue;
            if ((ifa->ifa_flags & IFF_LOOPBACK) != 0) continue;

            if (std::optional<CNetAddr> addr = FromSockAddr(ifa->ifa_addr, std::nullopt)) {
                addresses.push_back(*addr);
            }
        }
        freeifaddrs(myaddrs);
    }
#endif
    return addresses;
}
