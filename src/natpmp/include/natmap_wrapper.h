#ifndef BITCOIN_NATPMP_INCLUDE_NATMAP_WRAPPER_H
#define BITCOIN_NATPMP_INCLUDE_NATMAP_WRAPPER_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#if !defined(_MSC_VER)
#include <unistd.h>
#endif
#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <natpmp.h>
#include <string>
#include <iostream>
#include <logging.h>
#if defined(_WIN32) || defined(_WIN64)
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#endif
#include <array>

enum class Protocol {
    UDP = 1,
    TCP
};
using TimeValType = struct timeval;

/// @brief Wrapper over libnatpmp functions
class NatMap
{
private:
    natpmp_t m_NatPmpObj;
    int m_State;

public:
    /// @brief Constructor
    ///
    /// @param alternateGWAddr Pass a known gateway if default not to be used.
    explicit NatMap(in_addr_t* alternateGWAddr = nullptr);
    ~NatMap();

    /// @brief Expected to be called immediately after object construction.
    ///
    /// @return Zero or error code.
    int IsGood() const;

    /// @brief Ask gateway his public (internet facing) address. Does about
    ///        9 re-tries before giving up. So expect it to be a bit time consuming.
    ///
    /// @param publicAddressOut You would receive address in this
    ///
    /// @return Error code or success
    int GetPublicAddress(std::string& publicAddressOut);

    /// @brief Ask gateway to map your m/c port on the internet.
    ///        Pass same data to renew and zero life time to discontinue.
    ///
    /// @param protocol TCP/UDP
    /// @param privatePort your m/c port
    /// @param publicPortOut port on public IP, IN/Out Param ( Don't
    ///                      always expect to get what you wish ;))
    /// @param lifeTimeOut Lifetime of mapping, IN/Out Parameter
    ///
    /// @return Error code or success
    int SendMapReq(Protocol protocol, uint16_t privatePort, uint16_t& publicPortOut, uint32_t& lifeTimeOut);
    /// @brief Convert your error into meaningful message.
    ///
    /// @param err your error code
    ///
    /// @return Readable message
    std::string GetErrMsg(int err);

    /// @brief Get the default gateway
    ///
    /// @param addrOut Out parameter
    ///
    /// @return yes/no
    int GetDefaultGateway(std::string& addrOut) const;
};

#endif // BITCOIN_NATPMP_INCLUDE_NATMAP_WRAPPER_H
