#ifndef BITCOIN_SOCKS5_H
#define BITCOIN_SOCKS5_H

#include <netaddress.h>

#include <cstdint>
#include <string>

class Sock;
class Proxy;
struct ProxyCredentials;

namespace socks5 {

void Interrupt(bool interrupt);

/**
 * Connect to a specified destination service through an already connected
 * SOCKS5 proxy.
 *
 * @param strDest The destination fully-qualified domain name.
 * @param port The destination port.
 * @param auth The credentials with which to authenticate with the specified
 *             SOCKS5 proxy.
 * @param sock The SOCKS5 proxy socket.
 *
 * @returns Whether or not the operation succeeded.
 *
 * @note The specified SOCKS5 proxy socket must already be connected to the
 *       SOCKS5 proxy.
 *
 * @see <a href="https://www.ietf.org/rfc/rfc1928.txt">RFC1928: SOCKS Protocol
 *      Version 5</a>
 */
bool Connect(const std::string& strDest, uint16_t port, const ProxyCredentials* auth, const Sock& sock);

/**
 * Connect to a specified destination service through a SOCKS5 proxy by first
 * connecting to the SOCKS5 proxy.
 *
 * @param proxy The SOCKS5 proxy.
 * @param strDest The destination service to which to connect.
 * @param port The destination port.
 * @param sock The socket on which to connect to the SOCKS5 proxy.
 * @param nTimeout Wait this many milliseconds for the connection to the SOCKS5
 *                 proxy to be established.
 * @param[out] outProxyConnectionFailed Whether or not the connection to the
 *                                      SOCKS5 proxy failed.
 *
 * @returns Whether or not the operation succeeded.
 */
bool ConnectThroughProxy(const Proxy& proxy, const std::string& strDest, uint16_t port, const Sock& sock, int nTimeout, bool& outProxyConnectionFailed);

} // namespace socks5

#endif // BITCOIN_SOCKS5_H
