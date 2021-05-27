// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <mapport.h>

#include <clientversion.h>
#include <logging.h>
#include <net.h>
#include <netaddress.h>
#include <netbase.h>
#include <threadinterrupt.h>
#include <util/system.h>
#include <util/thread.h>

#ifdef USE_NATPMP
#include <compat.h>
#include <natpmp.h>
#endif // USE_NATPMP

#ifdef USE_UPNP
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
// The minimum supported miniUPnPc API version is set to 10. This keeps compatibility
// with Ubuntu 16.04 LTS and Debian 8 libminiupnpc-dev packages.
static_assert(MINIUPNPC_API_VERSION >= 10, "miniUPnPc API version >= 10 assumed");
#endif // USE_UPNP

#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <string>
#include <thread>

#if defined(USE_NATPMP) || defined(USE_UPNP)
static CThreadInterrupt g_mapport_interrupt;
static std::thread g_mapport_thread;
static std::atomic_uint g_mapport_enabled_protos{MapPortProtoFlag::NONE};
static std::atomic<MapPortProtoFlag> g_mapport_current_proto{MapPortProtoFlag::NONE};

using namespace std::chrono_literals;
static constexpr auto PORT_MAPPING_REANNOUNCE_PERIOD{20min};
static constexpr auto PORT_MAPPING_RETRY_PERIOD{5min};

#ifdef USE_NATPMP
static uint16_t g_mapport_external_port = 0;
static bool NatpmpInit(natpmp_t* natpmp)
{
    const int r_init = initnatpmp(natpmp, /* detect gateway automatically */ 0, /* forced gateway - NOT APPLIED*/ 0);
    if (r_init == 0) return true;
    LogPrintf("natpmp: initnatpmp() failed with %d error.\n", r_init);
    return false;
}

static bool NatpmpDiscover(natpmp_t* natpmp, struct in_addr& external_ipv4_addr)
{
    const int r_send = sendpublicaddressrequest(natpmp);
    if (r_send == 2 /* OK */) {
        int r_read;
        natpmpresp_t response;
        do {
            r_read = readnatpmpresponseorretry(natpmp, &response);
        } while (r_read == NATPMP_TRYAGAIN);

        if (r_read == 0) {
            external_ipv4_addr = response.pnu.publicaddress.addr;
            return true;
        } else if (r_read == NATPMP_ERR_NOGATEWAYSUPPORT) {
            LogPrintf("natpmp: The gateway does not support NAT-PMP.\n");
        } else {
            LogPrintf("natpmp: readnatpmpresponseorretry() for public address failed with %d error.\n", r_read);
        }
    } else {
        LogPrintf("natpmp: sendpublicaddressrequest() failed with %d error.\n", r_send);
    }

    return false;
}

static bool NatpmpMapping(natpmp_t* natpmp, const struct in_addr& external_ipv4_addr, uint16_t private_port, bool& external_ip_discovered)
{
    const uint16_t suggested_external_port = g_mapport_external_port ? g_mapport_external_port : private_port;
    const int r_send = sendnewportmappingrequest(natpmp, NATPMP_PROTOCOL_TCP, private_port, suggested_external_port, 3600 /*seconds*/);
    if (r_send == 12 /* OK */) {
        int r_read;
        natpmpresp_t response;
        do {
            r_read = readnatpmpresponseorretry(natpmp, &response);
        } while (r_read == NATPMP_TRYAGAIN);

        if (r_read == 0) {
            auto pm = response.pnu.newportmapping;
            if (private_port == pm.privateport && pm.lifetime > 0) {
                g_mapport_external_port = pm.mappedpublicport;
                const CService external{external_ipv4_addr, pm.mappedpublicport};
                if (!external_ip_discovered && fDiscover) {
                    AddLocal(external, LOCAL_MAPPED);
                    external_ip_discovered = true;
                }
                LogPrintf("natpmp: Port mapping successful. External address = %s\n", external.ToString());
                return true;
            } else {
                LogPrintf("natpmp: Port mapping failed.\n");
            }
        } else if (r_read == NATPMP_ERR_NOGATEWAYSUPPORT) {
            LogPrintf("natpmp: The gateway does not support NAT-PMP.\n");
        } else {
            LogPrintf("natpmp: readnatpmpresponseorretry() for port mapping failed with %d error.\n", r_read);
        }
    } else {
        LogPrintf("natpmp: sendnewportmappingrequest() failed with %d error.\n", r_send);
    }

    return false;
}

static bool ProcessNatpmp()
{
    bool ret = false;
    natpmp_t natpmp;
    struct in_addr external_ipv4_addr;
    if (NatpmpInit(&natpmp) && NatpmpDiscover(&natpmp, external_ipv4_addr)) {
        bool external_ip_discovered = false;
        const uint16_t private_port = GetListenPort();
        do {
            ret = NatpmpMapping(&natpmp, external_ipv4_addr, private_port, external_ip_discovered);
        } while (ret && g_mapport_interrupt.sleep_for(PORT_MAPPING_REANNOUNCE_PERIOD));
        g_mapport_interrupt.reset();

        const int r_send = sendnewportmappingrequest(&natpmp, NATPMP_PROTOCOL_TCP, private_port, g_mapport_external_port, /* remove a port mapping */ 0);
        g_mapport_external_port = 0;
        if (r_send == 12 /* OK */) {
            LogPrintf("natpmp: Port mapping removed successfully.\n");
        } else {
            LogPrintf("natpmp: sendnewportmappingrequest(0) failed with %d error.\n", r_send);
        }
    }

    closenatpmp(&natpmp);
    return ret;
}
#endif // USE_NATPMP

#ifdef USE_UPNP
static bool ProcessUpnp()
{
    bool ret = false;
    std::string port = strprintf("%u", GetListenPort());
    const char * multicastif = nullptr;
    const char * minissdpdpath = nullptr;
    struct UPNPDev * devlist = nullptr;
    char lanaddr[64];

    int error = 0;
#if MINIUPNPC_API_VERSION < 14
    devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0, 0, &error);
#else
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
            if (r != UPNPCOMMAND_SUCCESS) {
                LogPrintf("UPnP: GetExternalIPAddress() returned %d\n", r);
            } else {
                if (externalIPAddress[0]) {
                    CNetAddr resolved;
                    if (LookupHost(externalIPAddress, resolved, false)) {
                        LogPrintf("UPnP: ExternalIPAddress = %s\n", resolved.ToString());
                        AddLocal(resolved, LOCAL_MAPPED);
                    }
                } else {
                    LogPrintf("UPnP: GetExternalIPAddress failed.\n");
                }
            }
        }

        std::string strDesc = PACKAGE_NAME " " + FormatFullVersion();

        do {
            r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, port.c_str(), port.c_str(), lanaddr, strDesc.c_str(), "TCP", 0, "0");

            if (r != UPNPCOMMAND_SUCCESS) {
                ret = false;
                LogPrintf("AddPortMapping(%s, %s, %s) failed with code %d (%s)\n", port, port, lanaddr, r, strupnperror(r));
                break;
            } else {
                ret = true;
                LogPrintf("UPnP Port Mapping successful.\n");
            }
        } while (g_mapport_interrupt.sleep_for(PORT_MAPPING_REANNOUNCE_PERIOD));
        g_mapport_interrupt.reset();

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

    return ret;
}
#endif // USE_UPNP

static void ThreadMapPort()
{
    bool ok;
    do {
        ok = false;

#ifdef USE_UPNP
        // High priority protocol.
        if (g_mapport_enabled_protos & MapPortProtoFlag::UPNP) {
            g_mapport_current_proto = MapPortProtoFlag::UPNP;
            ok = ProcessUpnp();
            if (ok) continue;
        }
#endif // USE_UPNP

#ifdef USE_NATPMP
        // Low priority protocol.
        if (g_mapport_enabled_protos & MapPortProtoFlag::NAT_PMP) {
            g_mapport_current_proto = MapPortProtoFlag::NAT_PMP;
            ok = ProcessNatpmp();
            if (ok) continue;
        }
#endif // USE_NATPMP

        g_mapport_current_proto = MapPortProtoFlag::NONE;
        if (g_mapport_enabled_protos == MapPortProtoFlag::NONE) {
            return;
        }

    } while (ok || g_mapport_interrupt.sleep_for(PORT_MAPPING_RETRY_PERIOD));
}

void StartThreadMapPort()
{
    if (!g_mapport_thread.joinable()) {
        assert(!g_mapport_interrupt);
        g_mapport_thread = std::thread(&util::TraceThread, "mapport", &ThreadMapPort);
    }
}

static void DispatchMapPort()
{
    if (g_mapport_current_proto == MapPortProtoFlag::NONE && g_mapport_enabled_protos == MapPortProtoFlag::NONE) {
        return;
    }

    if (g_mapport_current_proto == MapPortProtoFlag::NONE && g_mapport_enabled_protos != MapPortProtoFlag::NONE) {
        StartThreadMapPort();
        return;
    }

    if (g_mapport_current_proto != MapPortProtoFlag::NONE && g_mapport_enabled_protos == MapPortProtoFlag::NONE) {
        InterruptMapPort();
        StopMapPort();
        return;
    }

    if (g_mapport_enabled_protos & g_mapport_current_proto) {
        // Enabling another protocol does not cause switching from the currently used one.
        return;
    }

    assert(g_mapport_thread.joinable());
    assert(!g_mapport_interrupt);
    // Interrupt a protocol-specific loop in the ThreadUpnp() or in the ThreadNatpmp()
    // to force trying the next protocol in the ThreadMapPort() loop.
    g_mapport_interrupt();
}

static void MapPortProtoSetEnabled(MapPortProtoFlag proto, bool enabled)
{
    if (enabled) {
        g_mapport_enabled_protos |= proto;
    } else {
        g_mapport_enabled_protos &= ~proto;
    }
}

void StartMapPort(bool use_upnp, bool use_natpmp)
{
    MapPortProtoSetEnabled(MapPortProtoFlag::UPNP, use_upnp);
    MapPortProtoSetEnabled(MapPortProtoFlag::NAT_PMP, use_natpmp);
    DispatchMapPort();
}

void InterruptMapPort()
{
    g_mapport_enabled_protos = MapPortProtoFlag::NONE;
    if (g_mapport_thread.joinable()) {
        g_mapport_interrupt();
    }
}

void StopMapPort()
{
    if (g_mapport_thread.joinable()) {
        g_mapport_thread.join();
        g_mapport_interrupt.reset();
    }
}

#else  // #if defined(USE_NATPMP) || defined(USE_UPNP)
void StartMapPort(bool use_upnp, bool use_natpmp)
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
#endif // #if defined(USE_NATPMP) || defined(USE_UPNP)
