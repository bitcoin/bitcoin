// Copyright (c) 2017-2023 Vincent Thiery
// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stats/rawsender.h>

#include <logging.h>
#include <netaddress.h>
#include <netbase.h>
#include <util/sock.h>
#include <util/thread.h>

RawSender::RawSender(const std::string& host, uint16_t port, std::pair<uint64_t, uint8_t> batching_opts,
                     uint64_t interval_ms, std::optional<bilingual_str>& error) :
    m_host{host},
    m_port{port},
    m_batching_opts{batching_opts},
    m_interval_ms{interval_ms}
{
    if (host.empty()) {
        error = _("No host specified");
        return;
    }

    if (auto netaddr = LookupHost(m_host, /*fAllowLookup=*/true); netaddr.has_value()) {
        if (!netaddr->IsIPv4()) {
            error = strprintf(_("Host %s on unsupported network"), m_host);
            return;
        }
        if (!CService(*netaddr, port).GetSockAddr(reinterpret_cast<struct sockaddr*>(&m_server.first), &m_server.second)) {
            error = strprintf(_("Cannot get socket address for %s"), m_host);
            return;
        }
    } else {
        error = strprintf(_("Unable to lookup host %s"), m_host);
        return;
    }

    SOCKET hSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (hSocket == INVALID_SOCKET) {
        error = strprintf(_("Cannot create socket (socket() returned error %s)"), NetworkErrorString(WSAGetLastError()));
        return;
    }
    m_sock = std::make_unique<Sock>(hSocket);

    if (m_interval_ms == 0) {
        LogPrintf("Send interval is zero, not starting RawSender queueing thread.\n");
    } else {
        m_interrupt.reset();
        m_thread = std::thread(&util::TraceThread, "rawsender", [this] { QueueThreadMain(); });
    }

    LogPrintf("Started %sRawSender sending messages to %s:%d\n", m_thread.joinable() ? "threaded " : "", m_host, m_port);
}

RawSender::~RawSender()
{
    // If there is a thread, interrupt and stop it
    if (m_thread.joinable()) {
        m_interrupt();
        m_thread.join();
    }
    // Flush queue of uncommitted messages
    QueueFlush();

    LogPrintf("Stopped RawSender instance sending messages to %s:%d. %d successes, %d failures.\n",
              m_host, m_port, m_successes, m_failures);
}

std::optional<bilingual_str> RawSender::Send(const RawMessage& msg)
{
    // If there is a thread, append to queue
    if (m_thread.joinable()) {
        QueueAdd(msg);
        return std::nullopt;
    }
    // There isn't a queue, send directly
    return SendDirectly(msg);
}

std::optional<bilingual_str> RawSender::SendDirectly(const RawMessage& msg)
{
    if (!m_sock) {
        m_failures++;
        return _("Socket not initialized, cannot send message");
    }

    if (::sendto(m_sock->Get(), reinterpret_cast<const char*>(msg.data()),
#ifdef WIN32
                 static_cast<int>(msg.size()),
#else
                 msg.size(),
#endif // WIN32
                 /*flags=*/0, reinterpret_cast<struct sockaddr*>(&m_server.first), m_server.second) == SOCKET_ERROR) {
        m_failures++;
        return strprintf(_("Unable to send message to %s (::sendto() returned error %s)"), this->ToStringHostPort(),
                         NetworkErrorString(WSAGetLastError()));
    }

    m_successes++;
    return std::nullopt;
}

std::string RawSender::ToStringHostPort() const { return strprintf("%s:%d", m_host, m_port); }

void RawSender::QueueAdd(const RawMessage& msg)
{
    AssertLockNotHeld(cs);
    LOCK(cs);

    const auto& [batch_size, batch_delim] = m_batching_opts;
    // If no batch size has been specified, simply add to queue
    if (batch_size == 0) {
        m_queue.push_back(msg);
        return;
    }

    // We can batch, either create a new batch in queue or append to existing batch in queue
    if (m_queue.empty() || m_queue.back().size() + msg.size() >= batch_size) {
        // Either we don't have a place to batch our message or we exceeded the batch size, make a new batch
        m_queue.emplace_back();
        m_queue.back().reserve(batch_size);
    } else if (!m_queue.back().empty()) {
        // When there is already a batch open we need a delimiter when its not empty
        m_queue.back() += batch_delim;
    }

    // Add the new message to the batch
    m_queue.back() += msg;
}

void RawSender::QueueFlush()
{
    AssertLockNotHeld(cs);
    WITH_LOCK(cs, QueueFlush(m_queue));
}

void RawSender::QueueFlush(std::deque<RawMessage>& queue)
{
    while (!queue.empty()) {
        SendDirectly(queue.front());
        queue.pop_front();
    }
}

void RawSender::QueueThreadMain()
{
    AssertLockNotHeld(cs);

    while (!m_interrupt) {
        // Swap the queues to commit the existing queue of messages
        std::deque<RawMessage> queue;
        WITH_LOCK(cs, m_queue.swap(queue));

        // Flush the committed queue
        QueueFlush(queue);
        assert(queue.empty());

        if (!m_interrupt.sleep_for(std::chrono::milliseconds(m_interval_ms))) {
            return;
        }
    }
}
