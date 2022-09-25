#include <sv2_template_provider.h>
#include <consensus/merkle.h>
#include <netbase.h>
#include <validation.h>
#include <util/thread.h>

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
    LogPrintf("Sv2 Template Provider listening on port: %d\n", port);
};

void Sv2TemplateProvider::Start()
{
    TemplateId id;
    id.m_id = 0;
    m_template_id = id;
    m_blocks_cache = std::map<uint64_t, std::unique_ptr<node::CBlockTemplate>>();

    {
      LOCK2(cs_main, m_mempool.cs);
      UpdateTemplate(true);
    }

    UpdatePrevHash();

    m_thread_sv2_handler = std::thread(&util::TraceThread, "sv2", [this] { ThreadSv2Handler(); });
};

void Sv2TemplateProvider::ThreadSv2Handler()
{
    while (!m_flag_interrupt_sv2) {
        if (m_chainman.ActiveChainstate().IsInitialBlockDownload()) {
            m_interrupt_sv2.sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        {
            LOCK2(cs_main, m_mempool.cs);
            WAIT_LOCK(g_best_block_mutex, lock);
            {
                auto checktime = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
                if (g_best_block_cv.wait_until(lock, checktime) == std::cv_status::timeout)
                {
                    if (m_prev_hash.m_prev_hash != g_best_block) {
                        UpdateTemplate(true);
                        UpdatePrevHash();
                        OnNewBlock();
                    }
                }
            }
        }

        std::set<SOCKET> recv_set, err_set;
        GenerateSocketEvents(recv_set, err_set);

        if (m_listening_socket->Get() != INVALID_SOCKET && recv_set.count(m_listening_socket->Get()) > 0) {
            struct sockaddr_storage sockaddr;
            socklen_t sockaddr_len = sizeof(sockaddr);

            SOCKET hSocket = accept(m_listening_socket->Get(), (struct sockaddr*)&sockaddr, &sockaddr_len);
            auto sock = std::make_unique<Sock>(hSocket);

            Sv2Client* client = new Sv2Client(std::move(sock));
            m_sv2_clients.push_back(client);
        }

        std::vector<Sv2Client*> clients_copy = m_sv2_clients;
        for (Sv2Client* client: clients_copy) {
            if (client->m_disconnect_flag) {
                m_sv2_clients.erase(remove(m_sv2_clients.begin(), m_sv2_clients.end(), client), m_sv2_clients.end());
                delete client;
            }
        };

        for (Sv2Client* client : m_sv2_clients) {
            bool recv_flag = recv_set.count(client->m_sock->Get()) > 0;
            bool err_flag = err_set.count(client->m_sock->Get()) > 0;

            if (err_flag) {
                client->m_disconnect_flag = true;
            }

            if (recv_flag) {
                uint8_t bytes_recv_buf[0x10000];
                int num_bytes_recv = recv(client->m_sock->Get(), (char*)bytes_recv_buf, sizeof(bytes_recv_buf), MSG_DONTWAIT);

                if (num_bytes_recv <= 0) {
                    client->m_disconnect_flag = true;
                    continue;
                }

                CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
                ss << Span<uint8_t>(&bytes_recv_buf[0], num_bytes_recv);

                Sv2Header sv2_header;
                try {
                    ss >> sv2_header;
                } catch (const std::exception &e) {
                    client->m_disconnect_flag = true;
                    continue;
                }

                ProcessSv2Message(sv2_header, ss, client);
            }
        }
    }
}

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

void Sv2TemplateProvider::UpdatePrevHash()
{
    auto cached_block = m_blocks_cache.find(m_new_template.m_template_id);

    if (cached_block != m_blocks_cache.end()) {
        const CBlock block = cached_block->second->block;
        m_prev_hash = SetNewPrevHash{block, m_new_template.m_template_id};
    }
}

void Sv2TemplateProvider::UpdateTemplate(bool future)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(m_mempool.cs);

    node::BlockAssembler::Options options;
    options.nBlockMaxWeight = MAX_BLOCK_WEIGHT;
    options.blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);

    std::unique_ptr<node::CBlockTemplate> blocktemplate = node::BlockAssembler(m_chainman.ActiveChainstate(), &m_mempool, options).CreateNewBlock(CScript());

    uint64_t id = m_template_id.Next();
    NewTemplate new_template{blocktemplate->block, id, future};
    m_blocks_cache.insert({new_template.m_template_id, std::move(blocktemplate)});
    m_new_template = new_template;
}

void Sv2TemplateProvider::OnNewBlock() {
    for (Sv2Client* client : m_sv2_clients) {
        if (!client->m_setup_connection_confirmed) {
            continue;
        }

        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);

        try {
            ss << Sv2Header{Sv2MsgType::NEW_TEMPLATE, m_new_template.GetMsgLen()}
               << m_new_template;
        } catch(const std::exception &e) {
            LogPrintf("Error writing m_new_template\n");
        }

        write(client->m_sock->Get(), ss.data(), ss.size());
        ss.clear();

        try {
            ss << Sv2Header{Sv2MsgType::SET_NEW_PREV_HASH, m_prev_hash.GetMsgLen()}
               << m_prev_hash;
        } catch(const std::exception &e) {
            LogPrintf("Error writing m_prev_hash\n");
        }

        write(client->m_sock->Get(), ss.data(), ss.size());
    }
}

void Sv2TemplateProvider::ProcessSv2Message(const Sv2Header& sv2_header, CDataStream& ss, Sv2Client* client)
{
    if (!client) return;

    switch(sv2_header.m_msg_type)
    {
        case SETUP_CONNECTION:
        {
            if (client->m_setup_connection_confirmed) {
                return;
            }

            SetupConnection setup_conn;
            try {
                ss >> setup_conn;
            } catch(const std::exception& e) {
                client->m_disconnect_flag = true;
                return;
            }
            ss.clear();

            if (setup_conn.m_protocol == SETUP_CONN_TP_PROTOCOL) {
               client->m_setup_connection_confirmed = true;

               SetupConnectionSuccess setup_success{2, 0};

               ss << Sv2Header{Sv2MsgType::SETUP_CONNECTION_SUCCESS, setup_success.GetMsgLen()}
                  << setup_success;

               write(client->m_sock->Get(), ss.data(), ss.size());
               ss.clear();

               NewTemplate copy_new_template = m_new_template;
               copy_new_template.m_future_template = false;

               try {
                   ss << Sv2Header{Sv2MsgType::NEW_TEMPLATE, copy_new_template.GetMsgLen()}
                      << copy_new_template;
               } catch(const std::exception &e) {
                   LogPrintf("Error writing copy_new_template\n");
               }

               write(client->m_sock->Get(), ss.data(), ss.size());
               ss.clear();

               try {
                   ss << Sv2Header{Sv2MsgType::SET_NEW_PREV_HASH, m_prev_hash.GetMsgLen()}
                      << m_prev_hash;
               } catch(const std::exception &e) {
                   LogPrintf("Error writing prev_hash\n");
               }

               write(client->m_sock->Get(), ss.data(), ss.size());
               ss.clear();
            }
            break;
        }
        case SUBMIT_SOLUTION:
        {
            SubmitSolution submit_solution;
            try {
                ss >> submit_solution;
            } catch(const std::exception& e) {
                return;
            }
            ss.clear();

            auto cached_block = m_blocks_cache.find(submit_solution.m_template_id);
            if (cached_block != m_blocks_cache.end()) {
                auto block_template = *cached_block->second;
                CBlock& block = block_template.block;

                auto coinbase_tx = CTransaction(std::move(submit_solution.m_coinbase_tx));
                block.vtx[0] = std::make_shared<CTransaction>(std::move(coinbase_tx));

                block.nVersion = submit_solution.m_version;
                block.nTime = submit_solution.m_header_timestamp;
                block.nNonce = submit_solution.m_header_nonce;
                block.hashMerkleRoot = BlockMerkleRoot(block);

                auto blockptr = std::make_shared<CBlock>(std::move(block));

                bool new_block{true};
                m_chainman.ProcessNewBlock(blockptr, true /* force_processing */, true /* min_pow_checked */, &new_block);
                m_blocks_cache.erase(submit_solution.m_template_id);

                {
                    LOCK2(cs_main, m_mempool.cs);
                    UpdateTemplate(true);
                    UpdatePrevHash();
                }

                OnNewBlock();
            }
            break;
        }
        default:
        {
            break;
        }
    }
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
