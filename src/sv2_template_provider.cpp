#include <consensus/merkle.h>
#include <logging.h>
#include <netbase.h>
#include <netaddress.h>
#include <node/miner.h>
#include <node/sv2_messages.h>
#include <sv2_template_provider.h>
#include <txmempool.h>
#include <util/thread.h>
#include <validation.h>

void Sv2TemplateProvider::Start(const Sv2TemplateProviderOptions& options)
{
    Init(options);

    // Here we are checking if we can bind to the port. If we can't, then exit
    // early and shutdown the node gracefully. This would be called in init.cpp
    // and allows the caller to see that the node is unable to run with the current
    // sv2 config.
    //
    // The socket is dropped within this scope and re-opened on the same port in
    // ThreadSv2Handler() when the node has finished IBD.
    auto sock = BindListenPort(options.port);

    m_thread_sv2_handler = std::thread(&util::TraceThread, "sv2", [this] { ThreadSv2Handler(); });
}

void Sv2TemplateProvider::Init(const Sv2TemplateProviderOptions& options)
{
    m_protocol_version = options.protocol_version;
    m_optional_features = options.optional_features;
    m_default_coinbase_tx_additional_output_size = options.default_coinbase_tx_additional_output_size;
    m_default_future_templates = options.default_future_templates;
    m_port = options.port;
}

void Sv2TemplateProvider::Interrupt()
{
    m_flag_interrupt_sv2 = true;
}

void Sv2TemplateProvider::StopThreads()
{
    if (m_thread_sv2_handler.joinable()) {
        m_thread_sv2_handler.join();
    }
}

std::shared_ptr<Sock> Sv2TemplateProvider::BindListenPort(uint16_t port) const
{
    const CService addr_bind = LookupNumeric("0.0.0.0", port);

    auto sock = CreateSock(addr_bind);
    if (!sock) {
        throw std::runtime_error("Sv2 Template Provider cannot create socket");
    }

    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);

    if (!addr_bind.GetSockAddr(reinterpret_cast<struct sockaddr*>(&sockaddr), &len)) {
        throw std::runtime_error("Sv2 Template Provider failed to get socket address");
    }

    if (sock->Bind(reinterpret_cast<struct sockaddr*>(&sockaddr), len) == SOCKET_ERROR) {
        const int nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE) {
            throw std::runtime_error(strprintf("Unable to bind to %d on this computer. %s is probably already running.\n", port, PACKAGE_NAME));
        }

        throw std::runtime_error(strprintf("Unable to bind to %d on this computer (bind returned error %s )\n", port, NetworkErrorString(nErr)));
    }

    constexpr int max_pending_conns{4096};
    if (sock->Listen(max_pending_conns) == SOCKET_ERROR) {
        throw std::runtime_error("Sv2 Template Provider listening socket has an error listening");
    }

    return sock;
}

void Sv2TemplateProvider::ThreadSv2Handler()
{
    while (!m_flag_interrupt_sv2) {
        if (m_chainman.ActiveChainstate().IsInitialBlockDownload()) {
            m_interrupt_sv2.sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // If we've left IBD. Create the listening socket for new sv2 connections.
        if (!m_listening_socket) {
            try {
                auto socket = BindListenPort(m_port);
                m_listening_socket = std::move(socket);
            } catch (const std::runtime_error& e) {
                LogPrintf("sv2: thread shutting down due to exception: %s\n", e.what());
                Interrupt();
                continue;
            }

            LogPrintf("Sv2 Template Provider listening on port: %d\n", m_port);
        }

        // Remove clients that are flagged for disconnection.
        m_sv2_clients.erase(
            std::remove_if(m_sv2_clients.begin(), m_sv2_clients.end(), [](const auto &client) {
                return client->m_disconnect_flag;
        }), m_sv2_clients.end());

        bool best_block_changed = [this]() {
            WAIT_LOCK(g_best_block_mutex, lock);
            auto checktime = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
            g_best_block_cv.wait_until(lock, checktime);
            return m_best_prev_hash.m_prev_hash != g_best_block;
        }();

        if (best_block_changed) {
            // Clear the block cache when the best known block changes since all
            // previous work is now invalid.
            BlockCache block_cache;
            m_block_cache.swap(block_cache);

            // Build a new best template, best prev hash and update the block cache.
            ++m_template_id;
            auto new_work_set = BuildNewWorkSet(m_default_future_templates, m_default_coinbase_tx_additional_output_size, m_template_id);
            m_best_new_template = std::move(new_work_set.new_template);
            m_best_prev_hash = std::move(new_work_set.prev_hash);
            m_block_cache.insert({m_template_id, std::move(new_work_set.block_template)});

            // Update all clients with the new template and prev hash.
            for (const auto& client : m_sv2_clients) {
                if (!SendWork(*client.get(), m_best_new_template, m_best_prev_hash, m_block_cache, m_template_id))
                    continue;
            }
        }

        // Poll/Select the sockets that need handling.
        Sock::EventsPerSock events_per_sock = GenerateWaitSockets(m_listening_socket, m_sv2_clients);

        constexpr auto timeout = std::chrono::milliseconds(50);
        if (!events_per_sock.begin()->first->WaitMany(timeout, events_per_sock)) {
            continue;
        }

        // Accept any new connections for sv2 clients.
        const auto listening_sock = events_per_sock.find(m_listening_socket);
        if (listening_sock != events_per_sock.end() && listening_sock->second.occurred & Sock::RECV) {
            struct sockaddr_storage sockaddr;
            socklen_t sockaddr_len = sizeof(sockaddr);

            auto sock = m_listening_socket->Accept(reinterpret_cast<struct sockaddr*>(&sockaddr), &sockaddr_len);
            if (sock) {
                m_sv2_clients.emplace_back(std::make_unique<Sv2Client>(Sv2Client{std::move(sock)}));
            }
        }

        // Process messages from connected sv2_clients.
        for (auto& client : m_sv2_clients) {
            bool has_received_data = false;
            bool has_error_occurred = false;

            const auto it = events_per_sock.find(client->m_sock);
            if (it != events_per_sock.end()) {
                has_received_data = it->second.occurred & Sock::RECV;
                has_error_occurred = it->second.occurred & Sock::ERR;
            }

            if (has_error_occurred) {
                client->m_disconnect_flag = true;
            }

            if (has_received_data) {
                uint8_t bytes_received_buf[0x10000];
                const auto num_bytes_received = client->m_sock->Recv(bytes_received_buf, sizeof(bytes_received_buf), MSG_DONTWAIT);

                if (num_bytes_received <= 0) {
                    client->m_disconnect_flag = true;
                    continue;
                }

                CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
                ss << Span<uint8_t>(bytes_received_buf, num_bytes_received);

                node::Sv2NetHeader sv2_header;
                try {
                    ss >> sv2_header;
                } catch (const std::exception& e) {
                    LogPrintf("Received an invalid sv2 header: %s\n", e.what());
                    client->m_disconnect_flag = true;
                    continue;
                }

                ProcessSv2Message(sv2_header, ss, *client.get(), m_best_new_template, m_best_prev_hash, m_block_cache, m_template_id);
            }
        }
    }
}

Sv2TemplateProvider::NewWorkSet Sv2TemplateProvider::BuildNewWorkSet(bool future_template, unsigned int coinbase_output_max_additional_size, uint64_t template_id)
{
    node::BlockAssembler::Options options;

    // Reducing the size of nBlockMaxWeight by the coinbase output additional size allows the miner extra weighted bytes in their coinbase space.
    options.nBlockMaxWeight = MAX_BLOCK_WEIGHT - coinbase_output_max_additional_size;
    options.blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);

    auto blocktemplate = node::BlockAssembler(m_chainman.ActiveChainstate(), &m_mempool, options).CreateNewBlock(CScript());
    node::Sv2NewTemplateMsg new_template{blocktemplate->block, template_id, future_template};
    node::Sv2SetNewPrevHashMsg set_new_prev_hash{blocktemplate->block, template_id};

    return NewWorkSet { new_template, std::move(blocktemplate), set_new_prev_hash};
}

bool Sv2TemplateProvider::SendWork(const Sv2Client& client, const node::Sv2NewTemplateMsg& best_template, const node::Sv2SetNewPrevHashMsg& best_prev_hash, BlockCache& block_cache, uint64_t& template_id)
{
    if (client.m_coinbase_tx_outputs_size > 0) {
        ++template_id;
        auto new_work_set = BuildNewWorkSet(m_default_future_templates, client.m_coinbase_tx_outputs_size, template_id);
        block_cache.insert({template_id, std::move(new_work_set.block_template)});

        try {
            if (!Send(client, node::Sv2NetMsg{new_work_set.new_template})) {
                LogPrintf("Error sending Sv2NewTemplate message\n");
                return false;
            }
        } catch (const std::exception& e) {
            LogPrintf("Failed to serialize new template: %s\n", e.what());
        }

        try {
            if (!Send(client, node::Sv2NetMsg{new_work_set.prev_hash})) {
                LogPrintf("Error sending Sv2SetNewPrevHash message\n");
                return false;
            }
        } catch (const std::exception& e) {
            LogPrintf("Failed to serialize new prev hash: %s\n", e.what());
        }
    } else {
        try {
            if (!Send(client, node::Sv2NetMsg{best_template})) {
                LogPrintf("Error sending best Sv2NewTemplate message\n");
                return false;
            }
        } catch (const std::exception& e) {
            LogPrintf("Failed to serialize best new template: %s\n", e.what());
        }

        try {
            if (!Send(client, node::Sv2NetMsg{best_prev_hash})) {
                LogPrintf("Error sending best Sv2SetNewPrevHash message\n");
                return false;
            }
        } catch (const std::exception& e) {
            LogPrintf("Failed to serialize best new prev hash: %s\n", e.what());
        }
    }

    return true;
}

Sock::EventsPerSock Sv2TemplateProvider::GenerateWaitSockets(const std::shared_ptr<Sock>& listen_socket, const Clients& sv2_clients) const
{
    Sock::EventsPerSock events_per_sock;
    events_per_sock.emplace(listen_socket, Sock::Events(Sock::RECV));

    for (const auto& client : sv2_clients) {
        if (!client->m_disconnect_flag && client->m_sock) {
            events_per_sock.emplace(client->m_sock, Sock::Events{Sock::RECV | Sock::ERR});
        }
    }

    return events_per_sock;
}

void Sv2TemplateProvider::ProcessSv2Message(const node::Sv2NetHeader& sv2_header, CDataStream& ss, Sv2Client& client, const node::Sv2NewTemplateMsg& best_new_template, const node::Sv2SetNewPrevHashMsg& best_prev_hash, BlockCache& block_cache, uint64_t& template_id)
{
    switch (sv2_header.m_msg_type) {
    case node::Sv2MsgType::SETUP_CONNECTION: {
        if (client.m_setup_connection_confirmed) {
            return;
        }

        node::Sv2SetupConnectionMsg setup_conn;
        try {
            ss >> setup_conn;
        } catch (const std::exception& e) {
            LogPrintf("Received invalid SetupConnection message: %s\n", e.what());
            client.m_disconnect_flag = true;
            return;
        }

        // Disconnect a client that connects on the wrong subprotocol.
        if (setup_conn.m_protocol != TP_SUBPROTOCOL) {
          node::Sv2SetupConnectionErrorMsg setup_conn_err{setup_conn.m_flags, std::string{"unsupported-protocol"}};

          try {
            if (!Send(client, node::Sv2NetMsg{setup_conn_err})) {
              LogPrintf("Failed to send Sv2SetupConnectionError message\n");
            }
          } catch (const std::exception& e) {
            LogPrintf("Failed to serialize best new prev hash: %s\n", e.what());
          }

            client.m_disconnect_flag = true;
            return;
        }

        // Disconnect a client if they are not running a compatible protocol version.
        if ((m_protocol_version < setup_conn.m_min_version) || (m_protocol_version > setup_conn.m_max_version)) {
            node::Sv2SetupConnectionErrorMsg setup_conn_err{setup_conn.m_flags, std::string{"protocol-version-mismatch"}};

            try {
                if (!Send(client, node::Sv2NetMsg{setup_conn_err})) {
                    LogPrintf("Failed to send Sv2SetupConnectionError message\n");
                }
            } catch (const std::exception& e) {
                LogPrintf("Failed to serialize best new prev hash: %s\n", e.what());
            }

            LogPrintf("Received a connection with incompatible protocol_versions: min_version: %d, max_version: %d\n", setup_conn.m_min_version, setup_conn.m_max_version);
            client.m_disconnect_flag = true;
            return;
        }

        node::Sv2SetupConnectionSuccessMsg setup_success{m_protocol_version, m_optional_features};
        try {
            if (!Send(client, node::Sv2NetMsg{setup_success})) {
                LogPrintf("Failed to send Sv2SetupSuccess message\n");
                client.m_disconnect_flag = true;
                return;
            }
        } catch (const std::exception& e) {
            LogPrintf("Failed to serialize setup success message: %s\n", e.what());
            client.m_disconnect_flag = true;
            return;
        }

        client.m_setup_connection_confirmed = true;

        break;
    }
    case node::Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE: {
        if (!client.m_setup_connection_confirmed) {
            client.m_disconnect_flag = true;
            return;
        }

        node::Sv2CoinbaseOutputDataSizeMsg coinbase_output_data_size;
        try {
            ss >> coinbase_output_data_size;
            client.m_coinbase_output_data_size_recv = true;
        } catch (const std::exception& e) {
            LogPrintf("Received invalid CoinbaseOutputDataSize message: %s\n", e.what());
            client.m_disconnect_flag = true;
            return;
        }

        client.m_coinbase_tx_outputs_size = coinbase_output_data_size.m_coinbase_output_max_additional_size;

        if (!SendWork(client, best_new_template, best_prev_hash, block_cache, template_id)) {
            return;
        }

        break;
    }
    case node::Sv2MsgType::SUBMIT_SOLUTION: {
        if (!client.m_setup_connection_confirmed && !client.m_coinbase_output_data_size_recv) {
            client.m_disconnect_flag = true;
            return;
        }

        node::Sv2SubmitSolutionMsg submit_solution;
        try {
            ss >> submit_solution;
        } catch (const std::exception& e) {
            LogPrintf("Received invalid SubmitSolution message: %e\n", e.what());
            return;
        }

        auto cached_block = block_cache.find(submit_solution.m_template_id);
        if (cached_block != block_cache.end()) {
            CBlock& block = (*cached_block->second).block;

            auto coinbase_tx = CTransaction(std::move(submit_solution.m_coinbase_tx));
            block.vtx[0] = std::make_shared<CTransaction>(std::move(coinbase_tx));

            block.nVersion = submit_solution.m_version;
            block.nTime = submit_solution.m_header_timestamp;
            block.nNonce = submit_solution.m_header_nonce;
            block.hashMerkleRoot = BlockMerkleRoot(block);

            auto blockptr = std::make_shared<CBlock>(std::move(block));
            bool new_block{true};

            m_chainman.ProcessNewBlock(blockptr, true /* force_processing */, true /* min_pow_checked */, &new_block);
        }
        break;
    }
    default: {
        break;
    }
    }
}
