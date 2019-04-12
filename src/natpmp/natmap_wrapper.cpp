#include <natmap_wrapper.h>

NatMap::NatMap(in_addr_t* alternateGWAddr)
{
    memset(&m_NatPmpObj, 0, sizeof(m_NatPmpObj));
    in_addr_t gateway = {0};
    if (alternateGWAddr) {
        memcpy(&gateway, alternateGWAddr, sizeof(gateway));
    }
    m_State = InitNatPmp(&m_NatPmpObj,
        alternateGWAddr ? 1 : 0,
        gateway);
}

int NatMap::IsGood() const
{
    return m_State;
}

NatMap::~NatMap()
{
    int returnCode = CloseNatPmp(&m_NatPmpObj);
    LogPrint(BCLog::NET, "NAT-PMP: Closed Connection to gateway [%s]\n", GetErrMsg(returnCode));
}

int NatMap::GetPublicAddress(std::string& publicAddressOut)
{
    int returnCode = SendPublicAddressRequest(&m_NatPmpObj);
    if (returnCode < 0) {
        return returnCode;
    }
    fd_set fds;
    natpmpresp_t response;
    TimeValType timeout;
    memset(&response, 0, sizeof(response));
    memset(&timeout, 0, sizeof(timeout));
    do {
        FD_ZERO(&fds);
        FD_SET(m_NatPmpObj.s, &fds);
        GetNatPmpRequestTimeout(&m_NatPmpObj, &timeout);
        returnCode = select(FD_SETSIZE, &fds, nullptr, nullptr, &timeout);
        if (returnCode < 0) {
            return returnCode;
        }
        returnCode = ReadNatPmpResponseOrRetry(&m_NatPmpObj, &response);
    } while (returnCode == NATPMP_TRYAGAIN);

    if (returnCode < 0) {
        return returnCode;
    }
    std::array<char, INET_ADDRSTRLEN> address;
    address.fill('\0');
    inet_ntop(AF_INET, &(response.pnu.publicaddress.addr), address.data(), INET_ADDRSTRLEN);
    publicAddressOut = std::string(address.data());
    return 0;
}

int NatMap::SendMapReq(Protocol protocol, uint16_t privatePort, uint16_t& publicPortOut, uint32_t& lifeTimeOut)
{
    int baseProtocol = (protocol == Protocol::TCP) ? NATPMP_PROTOCOL_TCP : NATPMP_PROTOCOL_UDP;
    int returnCode = SendNewPortMappingRequest(&m_NatPmpObj, baseProtocol,
        privatePort, publicPortOut,
        lifeTimeOut);
    if (returnCode < 0) {
        return returnCode;
    }

    fd_set fds;
    TimeValType timeout;
    natpmpresp_t response;
    memset(&response, 0, sizeof(response));
    memset(&timeout, 0, sizeof(timeout));
    do {
        FD_ZERO(&fds);
        FD_SET(m_NatPmpObj.s, &fds);
        GetNatPmpRequestTimeout(&m_NatPmpObj, &timeout);
        returnCode = select(FD_SETSIZE, &fds, nullptr, nullptr, &timeout);
        if (returnCode < 0) {
            return NATMAP_ERR_SELECT;
        }
        returnCode = ReadNatPmpResponseOrRetry(&m_NatPmpObj, &response);
    } while (returnCode == NATPMP_TRYAGAIN);
    if (returnCode < 0) {
        return returnCode;
    }

    publicPortOut = response.pnu.newportmapping.mappedpublicport;
    lifeTimeOut = response.pnu.newportmapping.lifetime;
    return 0;
}

std::string NatMap::GetErrMsg(int err)
{
    return std::string(StrNatPmpErr(err));
}

int NatMap::GetDefaultGateway(std::string& addrOut) const
{
    struct in_addr gateway = {0};
    gateway.s_addr = m_NatPmpObj.gateway;
    std::array<char, INET_ADDRSTRLEN> address;
    address.fill('\0');
    inet_ntop(AF_INET, &(gateway), address.data(), INET_ADDRSTRLEN);
    addrOut = std::string(address.data());
    return 0;
}
