/* $Id: getgateway.c,v 1.24 2014/03/31 12:41:35 nanard Exp $ */
/* libnatpmp
Copyright (c) 2007-2014, Thomas BERNARD
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * The name of the author may not be used to endorse or promote products
      derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <ctype.h>
#ifndef WIN32
#include <netinet/in.h>
#endif
#if !defined(_MSC_VER)
#include <sys/param.h>
#endif
/* There is no portable method to get the default route gateway.
 * So below are four (or five ?) differents functions implementing this.
 * Parsing /proc/net/route is for linux.
 * sysctl is the way to access such informations on BSD systems.
 * Many systems should provide route information through raw PF_ROUTE
 * sockets.
 * In MS Windows, default gateway is found by looking into the registry
 * or by using GetBestRoute(). */
#ifdef __linux__
#define USE_PROC_NET_ROUTE
#undef USE_SOCKET_ROUTE
#undef USE_SYSCTL_NET_ROUTE
#endif

#if defined(__OpenBSD__) || defined(__FreeBSD__)
#undef USE_PROC_NET_ROUTE
#define USE_SOCKET_ROUTE
#undef USE_SYSCTL_NET_ROUTE
#endif

#ifdef __APPLE__
#undef USE_PROC_NET_ROUTE
#undef USE_SOCKET_ROUTE
#define USE_SYSCTL_NET_ROUTE
#endif

#if (defined(sun) && defined(__SVR4))
#undef USE_PROC_NET_ROUTE
#define USE_SOCKET_ROUTE
#undef USE_SYSCTL_NET_ROUTE
#endif

#ifdef WIN32
#undef USE_PROC_NET_ROUTE
#undef USE_SOCKET_ROUTE
#undef USE_SYSCTL_NET_ROUTE
//#define USE_WIN32_CODE
#define USE_WIN32_CODE_2
#endif

#ifdef __CYGWIN__
#undef USE_PROC_NET_ROUTE
#undef USE_SOCKET_ROUTE
#undef USE_SYSCTL_NET_ROUTE
#define USE_WIN32_CODE
#include <stdarg.h>
#include <w32api/windef.h>
#include <w32api/winbase.h>
#include <w32api/winreg.h>
#endif

#ifdef __HAIKU__
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/sockio.h>
#define USE_HAIKU_CODE
#endif

#ifdef USE_SYSCTL_NET_ROUTE
#include <stdlib.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <net/route.h>
#endif
#ifdef USE_SOCKET_ROUTE
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#endif

#ifdef USE_WIN32_CODE
#include <unknwn.h>
#include <winreg.h>
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_LENGTH 16383
#endif

#ifdef USE_WIN32_CODE_2
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#endif

#include <natpmp/getgateway.h>

#ifndef _WIN32
#define SUCCESS (0)
#define FAILED  (-1)
#endif

#if defined(USE_PROC_NET_ROUTE)

/*
 parse /proc/net/route which is as follow :

Iface   Destination     Gateway         Flags   RefCnt  Use     Metric  Mask            MTU     Window  IRTT
wlan0   0001A8C0        00000000        0001    0       0       0       00FFFFFF        0       0       0
eth0    0000FEA9        00000000        0001    0       0       0       0000FFFF        0       0       0
wlan0   00000000        0101A8C0        0003    0       0       0       00000000        0       0       0
eth0    00000000        00000000        0001    0       0       1000    00000000        0       0       0

 One header line, and then one line by route by route table entry.
*/
int getdefaultgateway(in_addr_t * addr)
{
    unsigned long d, g;
    char buf[256];
    int line = 0;
    FILE * f;
    char * p;
    f = fopen("/proc/net/route", "r");
    if(!f)
        return FAILED;
    while(fgets(buf, sizeof(buf), f)) {
        if(line > 0) {    /* skip the first line */
            p = buf;
            /* skip the interface name */
            while(*p && !isspace(*p))
                p++;
            while(*p && isspace(*p))
                p++;
            if(sscanf(p, "%lx%lx", &d, &g)==2) {
                if(d == 0 && g != 0) { /* default */
                    *addr = g;
                    fclose(f);
                    return SUCCESS;
                }
            }
        }
        line++;
    }
    /* default route not found ! */
    if(f)
        fclose(f);
    return FAILED;
}

#elif defined(USE_SYSCTL_NET_ROUTE)

#define ROUNDUP(a) \
    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

int getdefaultgateway(in_addr_t * addr)
{
#if 0
    /* net.route.0.inet.dump.0.0 ? */
    int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET,
                 NET_RT_DUMP, 0, 0/*tableid*/};
#endif
    /* net.route.0.inet.flags.gateway */
    int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET,
                 NET_RT_FLAGS, RTF_GATEWAY};
    size_t l;
    char * buf, * p;
    struct rt_msghdr * rt;
    struct sockaddr * sa;
    struct sockaddr * sa_tab[RTAX_MAX];
    int i;
    int r = FAILED;
    if(sysctl(mib, sizeof(mib)/sizeof(int), 0, &l, 0, 0) < 0) {
        return FAILED;
    }
    if(l>0) {
        buf = malloc(l);
        if(sysctl(mib, sizeof(mib)/sizeof(int), buf, &l, 0, 0) < 0) {
            free(buf);
            return FAILED;
        }
        for(p=buf; p<buf+l; p+=rt->rtm_msglen) {
            rt = (struct rt_msghdr *)p;
            sa = (struct sockaddr *)(rt + 1);
            for(i=0; i<RTAX_MAX; i++) {
                if(rt->rtm_addrs & (1 << i)) {
                    sa_tab[i] = sa;
                    sa = (struct sockaddr *)((char *)sa + ROUNDUP(sa->sa_len));
                } else {
                    sa_tab[i] = nullptr;
                }
            }
            if( ((rt->rtm_addrs & (RTA_DST|RTA_GATEWAY)) == (RTA_DST|RTA_GATEWAY))
              && sa_tab[RTAX_DST]->sa_family == AF_INET
              && sa_tab[RTAX_GATEWAY]->sa_family == AF_INET) {
                if(((struct sockaddr_in *)sa_tab[RTAX_DST])->sin_addr.s_addr == 0) {
                    *addr = ((struct sockaddr_in *)(sa_tab[RTAX_GATEWAY]))->sin_addr.s_addr;
                    r = SUCCESS;
                }
            }
        }
        free(buf);
    }
    return r;
}

#elif defined(USE_SOCKET_ROUTE)

/* Thanks to Darren Kenny for this code */
#define NEXTADDR(w, u) \
        if (rtm_addrs & (w)) {\
            l = sizeof(struct sockaddr); memmove(cp, &(u), l); cp += l;\
        }

#define rtm m_rtmsg.m_rtm

struct {
  struct rt_msghdr m_rtm;
  char       m_space[512];
} m_rtmsg;

int getdefaultgateway(in_addr_t *addr)
{
  int s, seq, l, rtm_addrs, i;
  pid_t pid;
  struct sockaddr so_dst, so_mask;
  char *cp = m_rtmsg.m_space;
  struct sockaddr *gate = nullptr, *sa;
  struct rt_msghdr *msg_hdr;

  pid = getpid();
  seq = 0;
  rtm_addrs = RTA_DST | RTA_NETMASK;

  memset(&so_dst, 0, sizeof(so_dst));
  memset(&so_mask, 0, sizeof(so_mask));
  memset(&rtm, 0, sizeof(struct rt_msghdr));

  rtm.rtm_type = RTM_GET;
  rtm.rtm_flags = RTF_UP | RTF_GATEWAY;
  rtm.rtm_version = RTM_VERSION;
  rtm.rtm_seq = ++seq;
  rtm.rtm_addrs = rtm_addrs;

  so_dst.sa_family = AF_INET;
  so_mask.sa_family = AF_INET;

  NEXTADDR(RTA_DST, so_dst);
  NEXTADDR(RTA_NETMASK, so_mask);

  rtm.rtm_msglen = l = cp - (char *)&m_rtmsg;

  s = socket(PF_ROUTE, SOCK_RAW, 0);

  if (write(s, (char *)&m_rtmsg, l) < 0) {
      close(s);
      return FAILED;
  }

  do {
    l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
  } while (l > 0 && (rtm.rtm_seq != seq || rtm.rtm_pid != pid));

  close(s);

  msg_hdr = &rtm;

  cp = ((char *)(msg_hdr + 1));
  if (msg_hdr->rtm_addrs) {
    for (i = 1; i; i <<= 1)
      if (i & msg_hdr->rtm_addrs) {
        sa = (struct sockaddr *)cp;
        if (i == RTA_GATEWAY )
          gate = sa;

        cp += sizeof(struct sockaddr);
      }
  } else {
      return FAILED;
  }


  if (gate != nullptr ) {
      *addr = ((struct sockaddr_in *)gate)->sin_addr.s_addr;
      return SUCCESS;
  } else {
      return FAILED;
  }
}

#elif defined(USE_WIN32_CODE)

int getdefaultgateway(in_addr_t * addr)
{
    HKEY networkCardsKey;
    HKEY networkCardKey;
    HKEY interfacesKey;
    HKEY interfaceKey;
    DWORD i = 0;
    DWORD numSubKeys = 0;
    TCHAR keyName[MAX_KEY_LENGTH];
    DWORD keyNameLength = MAX_KEY_LENGTH;
    TCHAR keyValue[MAX_VALUE_LENGTH];
    DWORD keyValueLength = MAX_VALUE_LENGTH;
    DWORD keyValueType = REG_SZ;
    TCHAR gatewayValue[MAX_VALUE_LENGTH];
    DWORD gatewayValueLength = MAX_VALUE_LENGTH;
    DWORD gatewayValueType = REG_MULTI_SZ;
    int done = 0;

    //const char * networkCardsPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";
    //const char * interfacesPath = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
#ifdef UNICODE
    LPCTSTR networkCardsPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";
    LPCTSTR interfacesPath = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
#define STR_SERVICENAME     L"ServiceName"
#define STR_DHCPDEFAULTGATEWAY L"DhcpDefaultGateway"
#define STR_DEFAULTGATEWAY    L"DefaultGateway"
#else
    LPCTSTR networkCardsPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";
    LPCTSTR interfacesPath = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
#define STR_SERVICENAME     "ServiceName"
#define STR_DHCPDEFAULTGATEWAY "DhcpDefaultGateway"
#define STR_DEFAULTGATEWAY    "DefaultGateway"
#endif
    // The windows registry lists its primary network devices in the following location:
    // HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\NetworkCards
    //
    // Each network device has its own subfolder, named with an index, with various properties:
    // -NetworkCards
    //   -5
    //     -Description = Broadcom 802.11n Network Adapter
    //     -ServiceName = {E35A72F8-5065-4097-8DFE-C7790774EE4D}
    //   -8
    //     -Description = Marvell Yukon 88E8058 PCI-E Gigabit Ethernet Controller
    //     -ServiceName = {86226414-5545-4335-A9D1-5BD7120119AD}
    //
    // The above service name is the name of a subfolder within:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces
    //
    // There may be more subfolders in this interfaces path than listed in the network cards path above:
    // -Interfaces
    //   -{3a539854-6a70-11db-887c-806e6f6e6963}
    //     -DhcpIPAddress = 0.0.0.0
    //     -[more]
    //   -{E35A72F8-5065-4097-8DFE-C7790774EE4D}
    //     -DhcpIPAddress = 10.0.1.4
    //     -DhcpDefaultGateway = 10.0.1.1
    //     -[more]
    //   -{86226414-5545-4335-A9D1-5BD7120119AD}
    //     -DhcpIpAddress = 10.0.1.5
    //     -DhcpDefaultGateay = 10.0.1.1
    //     -[more]
    //
    // In order to extract this information, we enumerate each network card, and extract the ServiceName value.
    // This is then used to open the interface subfolder, and attempt to extract a DhcpDefaultGateway value.
    // Once one is found, we're done.
    //
    // It may be possible to simply enumerate the interface folders until we find one with a DhcpDefaultGateway value.
    // However, the technique used is the technique most cited on the web, and we assume it to be more correct.

    if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, // Open registry key or predifined key
                                     networkCardsPath,   // Name of registry subkey to open
                                     0,                  // Reserved - must be zero
                                     KEY_READ,           // Mask - desired access rights
                                     &networkCardsKey))  // Pointer to output key
    {
        // Unable to open network cards keys
        return -1;
    }

    if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, // Open registry key or predefined key
                                     interfacesPath,     // Name of registry subkey to open
                                     0,                  // Reserved - must be zero
                                     KEY_READ,           // Mask - desired access rights
                                     &interfacesKey))    // Pointer to output key
    {
        // Unable to open interfaces key
        RegCloseKey(networkCardsKey);
        return -1;
    }

    // Figure out how many subfolders are within the NetworkCards folder
    RegQueryInfoKey(networkCardsKey, nullptr, nullptr, nullptr, &numSubKeys, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    //printf( "Number of subkeys: %u\n", (unsigned int)numSubKeys);

    // Enumrate through each subfolder within the NetworkCards folder
    for(i = 0; i < numSubKeys && !done; i++)
    {
        keyNameLength = MAX_KEY_LENGTH;
        if(ERROR_SUCCESS == RegEnumKeyEx(networkCardsKey, // Open registry key
                                         i,               // Index of subkey to retrieve
                                         keyName,         // Buffer that receives the name of the subkey
                                         &keyNameLength,  // Variable that receives the size of the above buffer
                                         nullptr,            // Reserved - must be nil 
                                         nullptr,            // Buffer that receives the class string
                                         nullptr,            // Variable that receives the size of the above buffer
                                         nullptr))           // Variable that receives the last write time of subkey
        {
            if(RegOpenKeyEx(networkCardsKey,  keyName, 0, KEY_READ, &networkCardKey) == ERROR_SUCCESS)
            {
                keyValueLength = MAX_VALUE_LENGTH;
                if(ERROR_SUCCESS == RegQueryValueEx(networkCardKey,   // Open registry key
                                                    STR_SERVICENAME,    // Name of key to query
                                                    nullptr,             // Reserved - must be nil 
                                                    &keyValueType,    // Receives value type
                                                    (LPBYTE)keyValue, // Receives value
                                                    &keyValueLength)) // Receives value length in bytes
                {
//                    printf("keyValue: %s\n", keyValue);
                    if(RegOpenKeyEx(interfacesKey, keyValue, 0, KEY_READ, &interfaceKey) == ERROR_SUCCESS)
                    {
                        gatewayValueLength = MAX_VALUE_LENGTH;
                        if(ERROR_SUCCESS == RegQueryValueEx(interfaceKey,         // Open registry key
                                                            STR_DHCPDEFAULTGATEWAY, // Name of key to query
                                                            nullptr,                 // Reserved - must be nil 
                                                            &gatewayValueType,    // Receives value type
                                                            (LPBYTE)gatewayValue, // Receives value
                                                            &gatewayValueLength)) // Receives value length in bytes
                        {
                            // Check to make sure it's a string
                            if((gatewayValueType == REG_MULTI_SZ || gatewayValueType == REG_SZ) && (gatewayValueLength > 1))
                            {
                                //printf("gatewayValue: %s\n", gatewayValue);
                                done = 1;
                            }
                        }
                        else if(ERROR_SUCCESS == RegQueryValueEx(interfaceKey,         // Open registry key
                                                            STR_DEFAULTGATEWAY, // Name of key to query
                                                            nullptr,                 // Reserved - must be nil 
                                                            &gatewayValueType,    // Receives value type
                                                            (LPBYTE)gatewayValue,// Receives value
                                                            &gatewayValueLength)) // Receives value length in bytes
                        {
                            // Check to make sure it's a string
                            if((gatewayValueType == REG_MULTI_SZ || gatewayValueType == REG_SZ) && (gatewayValueLength > 1))
                            {
                                //printf("gatewayValue: %s\n", gatewayValue);
                                done = 1;
                            }
                        }
                        RegCloseKey(interfaceKey);
                    }
                }
                RegCloseKey(networkCardKey);
            }
        }
    }

    RegCloseKey(interfacesKey);
    RegCloseKey(networkCardsKey);

    if(done)
    {
#if UNICODE
        char tmp[32];
        for(i = 0; i < 32; i++) {
            tmp[i] = (char)gatewayValue[i];
            if(!tmp[i])
                break;
        }
        tmp[31] = '\0';
        *addr = inet_addr(tmp);
#else
        *addr = inet_addr(gatewayValue);
#endif
        return 0;
    }

    return -1;
}

#elif defined(USE_WIN32_CODE_2)

int getdefaultgateway(in_addr_t *addr)
{
    MIB_IPFORWARDROW ip_forward;
    memset(&ip_forward, 0, sizeof(ip_forward));
    if(GetBestRoute(inet_addr("0.0.0.0"), 0, &ip_forward) != NO_ERROR)
        return -1;
    *addr = ip_forward.dwForwardNextHop;
    return 0;
}

#elif defined(USE_HAIKU_CODE)

int getdefaultgateway(in_addr_t *addr)
{
    int fd, ret = -1;
    struct ifconf config;
    void *buffer = nullptr;
    struct ifreq *interface;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }
    if (ioctl(fd, SIOCGRTSIZE, &config, sizeof(config)) != 0) {
        goto fail;
    }
    if (config.ifc_value < 1) {
        goto fail; /* No routes */
    }
    if ((buffer = malloc(config.ifc_value)) == nullptr) {
        goto fail;
    }
    config.ifc_len = config.ifc_value;
    config.ifc_buf = buffer;
    if (ioctl(fd, SIOCGRTTABLE, &config, sizeof(config)) != 0) {
        goto fail;
    }
    for (interface = buffer;
      (uint8_t *)interface < (uint8_t *)buffer + config.ifc_len; ) {
        struct route_entry route = interface->ifr_route;
        int intfSize;
        if (route.flags & (RTF_GATEWAY | RTF_DEFAULT)) {
            *addr = ((struct sockaddr_in *)route.gateway)->sin_addr.s_addr;
            ret = 0;
            break;
        }
        intfSize = sizeof(route) + IF_NAMESIZE;
        if (route.destination != nullptr) {
            intfSize += route.destination->sa_len;
        }
        if (route.mask != nullptr) {
            intfSize += route.mask->sa_len;
        }
        if (route.gateway != nullptr) {
            intfSize += route.gateway->sa_len;
        }
        interface = (struct ifreq *)((uint8_t *)interface + intfSize);
    }
fail:
    free(buffer);
    close(fd);
    return ret;
}

#else /* fallback */

int getdefaultgateway(in_addr_t * addr)
{
    (void)addr;
    return -1;
}

#endif
