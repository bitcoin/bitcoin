// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETBASE_H
#define BITCOIN_NETBASE_H

#include "netaddress.h"  // For CService, serialize.h, etc.

extern int nConnectTimeout;
extern bool fNameLookup;

//! -timeout default
static const int DEFAULT_CONNECT_TIMEOUT = 5000;
//! -dns default
static const int DEFAULT_NAME_LOOKUP = true;

class proxyType
{
public:
    proxyType(): randomize_credentials(false) {}
    proxyType(const CService &proxy, bool randomize_credentials=false): proxy(proxy), randomize_credentials(randomize_credentials) {}

    bool IsValid() const { return proxy.IsValid(); }

    CService proxy;
    bool randomize_credentials;
};

enum Network ParseNetwork(std::string net);
std::string GetNetworkName(enum Network net);
void SplitHostPort(std::string in, int &portOut, std::string &hostOut);
bool SetProxy(enum Network net, const proxyType &addrProxy);
bool GetProxy(enum Network net, proxyType &proxyInfoOut);
bool IsProxy(const CNetAddr &addr);
bool SetNameProxy(const proxyType &addrProxy);
bool HaveNameProxy();

/**
 * Resolve a string hostname into an array of possible IP addresses.
 * @param[in]  pszName             The host name or dotted IP
 * @param[out] vIP                 The host's IP addresses
 * @param[in]  nMaxSolutions       Don't find more than this number of solutions
 * @param[in]  fAllowDnsResolution If true, pszName may be a dotted-name address, and an attempt will be made to
                                   resolve.  This can be very time consuming depending on your machine's DNS lookup time
 * @return True if resolution succeeded
 */
bool LookupHost(const char *pszName, std::vector<CNetAddr>& vIP, unsigned int nMaxSolutions, bool fAllowDnsResolution);

/**
 * Resolve a string hostname into an IP address:port "service".
 * @param[in]  pszName             The host name or dotted IP
 * @param[out] addr                The resolved IP address:port
 * @param[in]  portDefault         If a port is not specified in pszName, use this one.
 * @param[in]  fAllowDnsResolution If true, pszName may be a dotted-name address, and an attempt will be made to
                                   resolve.  This can be very time consuming depending on your machine's DNS lookup time
 * @return True if resolution succeeded
 */
bool Lookup(const char *pszName, CService& addr, int portDefault, bool fAllowDnsResolution);

/**
 * Resolve a string hostname into an array of possible IP address/port "services".
 * @param[in]  pszName             The host name or dotted IP
 * @param[out] vAddr               All resolved IP address:ports
 * @param[in]  nMaxSolutions       Don't find more than this number of solutions
 * @param[in]  fAllowDnsResolution If true, pszName may be a dotted-name address, and an attempt will be made to
                                   resolve.  This can be very time consuming depending on your machine's DNS lookup time
 * @return True if resolution succeeded
 */
bool Lookup(const char *pszName, std::vector<CService>& vAddr, int portDefault, unsigned int nMaxSolutions, bool fAllowDnsResolution);

/**
 * Resolve a string numeric hostname into an IP address:port "service".
 * @param[in]  pszName             The host name or dotted IP
 * @param[out] addr                The resolved IP address:port
 * @param[in]  portDefault         If a port is not specified in pszName, use this one.
 * @return True if resolution succeeded
 */
bool LookupNumeric(const char *pszName, CService& addr, int portDefault = 0);

bool ConnectSocket(const CService &addr, SOCKET& hSocketRet, int nTimeout, bool *outProxyConnectionFailed = 0);
bool ConnectSocketByName(CService &addr, SOCKET& hSocketRet, const char *pszDest, int portDefault, int nTimeout, bool *outProxyConnectionFailed = 0);
/** Return readable error string for a network error code */
std::string NetworkErrorString(int err);
/** Close socket and set hSocket to INVALID_SOCKET */
bool CloseSocket(SOCKET& hSocket);
/** Disable or enable blocking-mode for a socket */
bool SetSocketNonBlocking(SOCKET& hSocket, bool fNonBlocking);
/**
 * Convert milliseconds to a struct timeval for e.g. select.
 */
struct timeval MillisToTimeval(int64_t nTimeout);

#endif // BITCOIN_NETBASE_H
