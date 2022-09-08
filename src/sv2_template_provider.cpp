#include <sv2_template_provider.h>
#include <netbase.h>

void Sv2TemplateProvider::BindListenPort(uint16_t port)
{
    CService addr_bind = LookupNumeric("0.0.0.0", port);

    std::unique_ptr<Sock> sock = CreateSock(addr_bind);
    if (!sock) {
        throw std::runtime_error("Sv2 Template Provider cannot create socket");
    }

    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);

    if (!addr_bind.GetSockAddr((struct sockaddr*)&sockaddr, &len)) {
        throw std::runtime_error("Sv2 Template Provider failed to get socket address");
    }

    if (bind(sock->Get(), (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR) {
        int nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE) {
            throw std::runtime_error(strprintf("Unable to bind to %s on this computer. %s is probably already running.\n", addr_bind.ToString(), PACKAGE_NAME));
        }

        throw std::runtime_error(strprintf("Unable to bind to %s on this computer (bind returned error %s on this computer (bind returned error %s )\n", addr_bind.ToString(), NetworkErrorString(nErr)));
    }

    if ((listen(sock->Get(), 4096)) == SOCKET_ERROR) {
        throw std::runtime_error("Sv2 Template Provider listening socket has an error");
    }

    m_listening_socket = std::move(sock);
};
