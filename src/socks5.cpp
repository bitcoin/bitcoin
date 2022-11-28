#include <logging.h>
#include <netbase.h>
#include <node/proxy.h>
#include <socks5.h>
#include <util/sock.h>
#include <util/system.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace socks5 {

constexpr uint8_t VERSION{0x05};

// Need ample time for negotiation for very slow proxies such as Tor (milliseconds)
int g_recv_timeout = 20 * 1000;

static std::atomic<bool> g_interrupt_recv(false);

/** Values defined for METHOD in RFC1928 */
enum Method : uint8_t {
    NOAUTH = 0x00,        //!< No authentication required
    GSSAPI = 0x01,        //!< GSSAPI
    USER_PASS = 0x02,     //!< Username/password
    NO_ACCEPTABLE = 0xff, //!< No acceptable methods
};

/** Values defined for CMD in RFC1928 */
enum Command : uint8_t {
    CONNECT = 0x01,
    BIND = 0x02,
    UDP_ASSOCIATE = 0x03
};

/** Values defined for REP in RFC1928 */
enum Reply : uint8_t {
    SUCCEEDED = 0x00,        //!< Succeeded
    GENFAILURE = 0x01,       //!< General failure
    NOTALLOWED = 0x02,       //!< Connection not allowed by ruleset
    NETUNREACHABLE = 0x03,   //!< Network unreachable
    HOSTUNREACHABLE = 0x04,  //!< Network unreachable
    CONNREFUSED = 0x05,      //!< Connection refused
    TTLEXPIRED = 0x06,       //!< TTL expired
    CMDUNSUPPORTED = 0x07,   //!< Command not supported
    ATYPEUNSUPPORTED = 0x08, //!< Address type not supported
};

/** Values defined for ATYPE in RFC1928 */
enum Atype : uint8_t {
    IPV4 = 0x01,
    DOMAINNAME = 0x03,
    IPV6 = 0x04,
};

std::string ErrorString(uint8_t err)
{
    switch(err) {
        case Reply::GENFAILURE:
            return "general failure";
        case Reply::NOTALLOWED:
            return "connection not allowed";
        case Reply::NETUNREACHABLE:
            return "network unreachable";
        case Reply::HOSTUNREACHABLE:
            return "host unreachable";
        case Reply::CONNREFUSED:
            return "connection refused";
        case Reply::TTLEXPIRED:
            return "TTL expired";
        case Reply::CMDUNSUPPORTED:
            return "protocol error";
        case Reply::ATYPEUNSUPPORTED:
            return "address type not supported";
        default:
            return "unknown";
    }
}

void Interrupt(bool interrupt)
{
    g_interrupt_recv = interrupt;
}

/** Status codes that can be returned by InterruptibleRecv */
enum class IntrRecvError {
    OK,
    Timeout,
    Disconnected,
    NetworkError,
    Interrupted
};

/**
 * Try to read a specified number of bytes from a socket. Please read the "see
 * also" section for more detail.
 *
 * @param data The buffer where the read bytes should be stored.
 * @param len The number of bytes to read into the specified buffer.
 * @param timeout The total timeout in milliseconds for this read.
 * @param sock The socket (has to be in non-blocking mode) from which to read bytes.
 *
 * @returns An IntrRecvError indicating the resulting status of this read.
 *          IntrRecvError::OK only if all of the specified number of bytes were
 *          read.
 *
 * @see This function can be interrupted by calling InterruptSocks5(bool).
 *      Sockets can be made non-blocking with Sock::SetNonBlocking().
 */
static IntrRecvError InterruptibleRecv(uint8_t* data, size_t len, int timeout, const Sock& sock)
{
    int64_t curTime = GetTimeMillis();
    int64_t endTime = curTime + timeout;
    while (len > 0 && curTime < endTime) {
        ssize_t ret = sock.Recv(data, len, 0); // Optimistically try the recv first
        if (ret > 0) {
            len -= ret;
            data += ret;
        } else if (ret == 0) { // Unexpected disconnection
            return IntrRecvError::Disconnected;
        } else { // Other error or blocking
            int nErr = WSAGetLastError();
            if (nErr == WSAEINPROGRESS || nErr == WSAEWOULDBLOCK || nErr == WSAEINVAL) {
                // Only wait at most MAX_WAIT_FOR_IO at a time, unless
                // we're approaching the end of the specified total timeout
                const auto remaining = std::chrono::milliseconds{endTime - curTime};
                const auto timeout = std::min(remaining, std::chrono::milliseconds{MAX_WAIT_FOR_IO});
                if (!sock.Wait(timeout, Sock::RECV)) {
                    return IntrRecvError::NetworkError;
                }
            } else {
                return IntrRecvError::NetworkError;
            }
        }
        if (g_interrupt_recv)
            return IntrRecvError::Interrupted;
        curTime = GetTimeMillis();
    }
    return len == 0 ? IntrRecvError::OK : IntrRecvError::Timeout;
}

bool Connect(const std::string& strDest, uint16_t port, const ProxyCredentials* auth, const Sock& sock)
{
    IntrRecvError recvr;
    LogPrint(BCLog::NET, "SOCKS5 connecting %s\n", strDest);
    if (strDest.size() > 255) {
        return error("Hostname too long");
    }
    // Construct the version identifier/method selection message
    std::vector<uint8_t> vSocks5Init;
    vSocks5Init.push_back(VERSION); // We want the SOCK5 protocol
    if (auth) {
        vSocks5Init.push_back(0x02); // 2 method identifiers follow...
        vSocks5Init.push_back(Method::NOAUTH);
        vSocks5Init.push_back(Method::USER_PASS);
    } else {
        vSocks5Init.push_back(0x01); // 1 method identifier follows...
        vSocks5Init.push_back(Method::NOAUTH);
    }
    ssize_t ret = sock.Send(vSocks5Init.data(), vSocks5Init.size(), MSG_NOSIGNAL);
    if (ret != (ssize_t)vSocks5Init.size()) {
        return error("Error sending to proxy");
    }
    uint8_t pchRet1[2];
    if (InterruptibleRecv(pchRet1, 2, g_recv_timeout, sock) != IntrRecvError::OK) {
        LogPrintf("Socks5() connect to %s:%d failed: InterruptibleRecv() timeout or other failure\n", strDest, port);
        return false;
    }
    if (pchRet1[0] != VERSION) {
        return error("Proxy failed to initialize");
    }
    if (pchRet1[1] == Method::USER_PASS && auth) {
        // Perform username/password authentication (as described in RFC1929)
        std::vector<uint8_t> vAuth;
        vAuth.push_back(0x01); // Current (and only) version of user/pass subnegotiation
        if (auth->username.size() > 255 || auth->password.size() > 255)
            return error("Proxy username or password too long");
        vAuth.push_back(auth->username.size());
        vAuth.insert(vAuth.end(), auth->username.begin(), auth->username.end());
        vAuth.push_back(auth->password.size());
        vAuth.insert(vAuth.end(), auth->password.begin(), auth->password.end());
        ret = sock.Send(vAuth.data(), vAuth.size(), MSG_NOSIGNAL);
        if (ret != (ssize_t)vAuth.size()) {
            return error("Error sending authentication to proxy");
        }
        LogPrint(BCLog::PROXY, "SOCKS5 sending proxy authentication %s:%s\n", auth->username, auth->password);
        uint8_t pchRetA[2];
        if (InterruptibleRecv(pchRetA, 2, g_recv_timeout, sock) != IntrRecvError::OK) {
            return error("Error reading proxy authentication response");
        }
        if (pchRetA[0] != 0x01 || pchRetA[1] != 0x00) {
            return error("Proxy authentication unsuccessful");
        }
    } else if (pchRet1[1] == Method::NOAUTH) {
        // Perform no authentication
    } else {
        return error("Proxy requested wrong authentication method %02x", pchRet1[1]);
    }
    std::vector<uint8_t> vSocks5;
    vSocks5.push_back(VERSION); // VER protocol version
    vSocks5.push_back(Command::CONNECT); // CMD CONNECT
    vSocks5.push_back(0x00); // RSV Reserved must be 0
    vSocks5.push_back(Atype::DOMAINNAME); // ATYP DOMAINNAME
    vSocks5.push_back(strDest.size()); // Length<=255 is checked at beginning of function
    vSocks5.insert(vSocks5.end(), strDest.begin(), strDest.end());
    vSocks5.push_back((port >> 8) & 0xFF);
    vSocks5.push_back((port >> 0) & 0xFF);
    ret = sock.Send(vSocks5.data(), vSocks5.size(), MSG_NOSIGNAL);
    if (ret != (ssize_t)vSocks5.size()) {
        return error("Error sending to proxy");
    }
    uint8_t pchRet2[4];
    if ((recvr = InterruptibleRecv(pchRet2, 4, g_recv_timeout, sock)) != IntrRecvError::OK) {
        if (recvr == IntrRecvError::Timeout) {
            /* If a timeout happens here, this effectively means we timed out while connecting
             * to the remote node. This is very common for Tor, so do not print an
             * error message. */
            return false;
        } else {
            return error("Error while reading proxy response");
        }
    }
    if (pchRet2[0] != VERSION) {
        return error("Proxy failed to accept request");
    }
    if (pchRet2[1] != Reply::SUCCEEDED) {
        // Failures to connect to a peer that are not proxy errors
        LogPrintf("Socks5() connect to %s:%d failed: %s\n", strDest, port, ErrorString(pchRet2[1]));
        return false;
    }
    if (pchRet2[2] != 0x00) { // Reserved field must be 0
        return error("Error: malformed proxy response");
    }
    uint8_t pchRet3[256];
    switch (pchRet2[3])
    {
        case Atype::IPV4: recvr = InterruptibleRecv(pchRet3, 4, g_recv_timeout, sock); break;
        case Atype::IPV6: recvr = InterruptibleRecv(pchRet3, 16, g_recv_timeout, sock); break;
        case Atype::DOMAINNAME:
        {
            recvr = InterruptibleRecv(pchRet3, 1, g_recv_timeout, sock);
            if (recvr != IntrRecvError::OK) {
                return error("Error reading from proxy");
            }
            int nRecv = pchRet3[0];
            recvr = InterruptibleRecv(pchRet3, nRecv, g_recv_timeout, sock);
            break;
        }
        default: return error("Error: malformed proxy response");
    }
    if (recvr != IntrRecvError::OK) {
        return error("Error reading from proxy");
    }
    if (InterruptibleRecv(pchRet3, 2, g_recv_timeout, sock) != IntrRecvError::OK) {
        return error("Error reading from proxy");
    }
    LogPrint(BCLog::NET, "SOCKS5 connected %s\n", strDest);
    return true;
}

bool ConnectThroughProxy(const Proxy& proxy, const std::string& strDest, uint16_t port, const Sock& sock, int nTimeout, bool& outProxyConnectionFailed)
{
    // first connect to proxy server
    if (!ConnectSocketDirectly(proxy.proxy, sock, nTimeout, true)) {
        outProxyConnectionFailed = true;
        return false;
    }
    // do socks negotiation
    if (proxy.randomize_credentials) {
        ProxyCredentials random_auth;
        static std::atomic_int counter(0);
        random_auth.username = random_auth.password = strprintf("%i", counter++);
        if (!Connect(strDest, port, &random_auth, sock)) {
            return false;
        }
    } else {
        if (!Connect(strDest, port, nullptr, sock)) {
            return false;
        }
    }
    return true;
}
} // namespace socks5
