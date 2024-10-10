// Copyright (c) 2024 The Bitcoin Core developers
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

namespace {

// Linux and FreeBSD 14.0+. For FreeBSD 13.2 the code can be compiled but
// running it requires loading a special kernel module, otherwise socket(AF_NETLINK,...)
// will fail, so we skip that.
#if defined(__linux__) || (defined(__FreeBSD__) && __FreeBSD_version >= 1400000)

std::optional<CNetAddr> QueryDefaultGatewayImpl(sa_family_t family)
{
    // Create a netlink socket.
    auto sock{CreateSock(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)};
    if (!sock) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "socket(AF_NETLINK): %s\n", NetworkErrorString(errno));
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
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "send() to netlink socket: %s\n", NetworkErrorString(errno));
        return std::nullopt;
    }

    // Receive response.
    char response[4096];
    int64_t recv_result;
    do {
        recv_result = sock->Recv(response, sizeof(response), 0);
    } while (recv_result < 0 && (errno == EINTR || errno == EAGAIN));
    if (recv_result < 0) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "recv() from netlink socket: %s\n", NetworkErrorString(errno));
        return std::nullopt;
    }

    for (nlmsghdr* hdr = (nlmsghdr*)response; NLMSG_OK(hdr, recv_result); hdr = NLMSG_NEXT(hdr, recv_result)) {
        rtmsg* r = (rtmsg*)NLMSG_DATA(hdr);
        int remaining_len = RTM_PAYLOAD(hdr);

        // Iterate over the attributes.
        rtattr *rta_gateway = nullptr;
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
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "Could not get best interface for default route: %s\n", NetworkErrorString(status));
        return std::nullopt;
    }

    // Get best route to default gateway.
    // Leave interface_luid at all-zeros to use interface index instead.
    status = GetBestRoute2(&interface_luid, best_if_idx, nullptr, &destination_address, 0, &best_route, &best_source_address);
    if (status != NO_ERROR) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "Could not get best route for default route for interface index %d: %s\n",
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

std::optional<CNetAddr> FromSockAddr(const struct sockaddr* addr)
{
    // Check valid length. Note that sa_len is not part of POSIX, and exists on MacOS and some BSDs only, so we can't
    // do this check in SetSockAddr.
    if (!(addr->sa_family == AF_INET && addr->sa_len == sizeof(struct sockaddr_in)) &&
        !(addr->sa_family == AF_INET6 && addr->sa_len == sizeof(struct sockaddr_in6))) {
        return std::nullopt;
    }

    // Fill in a CService from the sockaddr, then drop the port part.
    CService service;
    if (service.SetSockAddr(addr)) {
        return (CNetAddr)service;
    }
    return std::nullopt;
}

//! MacOS: Get default gateway from route table. See route(4) for the format.
std::optional<CNetAddr> QueryDefaultGatewayImpl(sa_family_t family)
{
    // net.route.0.inet[6].flags.gateway
    int mib[] = {CTL_NET, PF_ROUTE, 0, family, NET_RT_FLAGS, RTF_GATEWAY};
    // The size of the available data is determined by calling sysctl() with oldp=nullptr. See sysctl(3).
    size_t l = 0;
    if (sysctl(/*name=*/mib, /*namelen=*/sizeof(mib) / sizeof(int), /*oldp=*/nullptr, /*oldlenp=*/&l, /*newp=*/nullptr, /*newlen=*/0) < 0) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "Could not get sysctl length of routing table: %s\n", SysErrorString(errno));
        return std::nullopt;
    }
    std::vector<std::byte> buf(l);
    if (sysctl(/*name=*/mib, /*namelen=*/sizeof(mib) / sizeof(int), /*oldp=*/buf.data(), /*oldlenp=*/&l, /*newp=*/nullptr, /*newlen=*/0) < 0) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "Could not get sysctl data of routing table: %s\n", SysErrorString(errno));
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
                    dst = FromSockAddr(sa);
                } else if (i == RTAX_GATEWAY) {
                    gateway = FromSockAddr(sa);
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
    char pszHostName[256] = "";
    if (gethostname(pszHostName, sizeof(pszHostName)) != SOCKET_ERROR) {
        addresses = LookupHost(pszHostName, 0, true);
    }
#elif (HAVE_DECL_GETIFADDRS && HAVE_DECL_FREEIFADDRS)
    struct ifaddrs* myaddrs;
    if (getifaddrs(&myaddrs) == 0) {
        for (struct ifaddrs* ifa = myaddrs; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == nullptr) continue;
            if ((ifa->ifa_flags & IFF_UP) == 0) continue;
            if ((ifa->ifa_flags & IFF_LOOPBACK) != 0) continue;
            if (ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in* s4 = (struct sockaddr_in*)(ifa->ifa_addr);
                addresses.emplace_back(s4->sin_addr);
            } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                struct sockaddr_in6* s6 = (struct sockaddr_in6*)(ifa->ifa_addr);
                addresses.emplace_back(s6->sin6_addr);
            }
        }
        freeifaddrs(myaddrs);
    }
#endif
    return addresses;
}
