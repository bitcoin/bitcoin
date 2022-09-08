#include <sv2_template_provider.h>
#include <netbase.h>
#include <validation.h>

#ifdef USE_POLL
#include <poll.h>
#endif

uint64_t TemplateId::Next() {
    uint64_t next = m_id;
    m_id += 1;

    return next;
}

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

void Sv2TemplateProvider::StopThreads()
{
    if (m_thread_sv2_handler.joinable()) {
        m_thread_sv2_handler.join();
    }
}

void Sv2TemplateProvider::Interrupt()
{
    m_flag_interrupt_sv2 = true;
}

void Sv2TemplateProvider::UpdateTemplate(bool future)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(m_mempool.cs);

    node::BlockAssembler::Options options;
    options.nBlockMaxWeight = MAX_BLOCK_WEIGHT;
    options.blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);

    std::unique_ptr<node::CBlockTemplate> blocktemplate = node::BlockAssembler(m_chainman.ActiveChainstate(), &m_mempool, options).CreateNewBlock(CScript());

    NewTemplate new_template{blocktemplate->block, m_template_id.Next(), future};
    m_blocks_cache.insert({new_template.m_template_id, std::move(blocktemplate)});
    m_new_template = new_template;
}

#ifdef USE_POLL
void Sv2TemplateProvider::GenerateSocketEvents(std::set<SOCKET> &recv_set, std::set<SOCKET> &err_set) {
    std::set<SOCKET> recv_select_set, error_select_set;

    recv_select_set.insert(m_listening_socket->Get());

    for (const Sv2Client *client : m_sv2_clients) {
        if (!client->m_disconnect_flag) {
            recv_select_set.insert(client->m_sock->Get());
            error_select_set.insert(client->m_sock->Get());
        }
    }

    std::unordered_map<SOCKET, struct pollfd> pollfds;
    for (const SOCKET socket_id : recv_select_set) {
        pollfds[socket_id].fd = socket_id;
        pollfds[socket_id].events |= POLLIN;
    }

    for (const SOCKET socket_id : error_select_set) {
        pollfds[socket_id].fd = socket_id;
        pollfds[socket_id].events |= POLLERR|POLLHUP;
    }

    std::vector<struct pollfd> vpollfds;
    vpollfds.reserve(pollfds.size());
    for (auto it : pollfds) {
        vpollfds.push_back(std::move(it.second));
    }

    if (poll(vpollfds.data(), vpollfds.size(), 500) < 0) return;

    for (struct pollfd pollfd_entry : vpollfds) {
        if (pollfd_entry.revents & POLLIN)            recv_set.insert(pollfd_entry.fd);
        if (pollfd_entry.revents & (POLLERR|POLLHUP)) err_set.insert(pollfd_entry.fd);
    }
}
#else
void Sv2TemplateProvider::GenerateSocketEvents(std::set<SOCKET> &recv_set, std::set<SOCKET> &err_set) {
    std::set<SOCKET> recv_select_set, err_select_set;

    recv_select_set.insert(m_listening_socket->Get());

    for (const Sv2Client* client : m_sv2_clients) {
        if (!client->m_disconnect_flag) {
            recv_select_set.insert(client->m_sock->Get());
            err_select_set.insert(client->m_sock->Get());
        }
    }

    fd_set fd_set_recv, fd_set_error;
    FD_ZERO(&fd_set_recv);
    FD_ZERO(&fd_set_error);
    int socket_max = 0;

    for (const auto socket : recv_select_set) {
        FD_SET(socket, &fd_set_recv);
        socket_max = std::max(socket_max, socket);
    }

    for (const auto socket : err_select_set) {
        FD_SET(socket, &fd_set_error);
        socket_max = std::max(socket_max, socket);
    }

    struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = 50 * 1000; // frequency to call select

    select(socket_max + 1, &fd_set_recv, nullptr, &fd_set_error, &timeout);

    for (const auto socket : recv_select_set) {
        if (FD_ISSET(socket, &fd_set_recv)) {
            recv_set.insert(socket);
        }
    }

    for (const auto socket : err_select_set) {
        if (FD_ISSET(socket, &fd_set_error)) {
            err_set.insert(socket);
        }
    }
}
#endif
