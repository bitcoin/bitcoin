// Copyright (c) 2017-2023 Vincent Thiery
// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STATS_RAWSENDER_H
#define BITCOIN_STATS_RAWSENDER_H

#include <compat.h>
#include <sync.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

class Sock;

struct RawMessage : public std::vector<uint8_t>
{
    using parent_type = std::vector<value_type>;
    using parent_type::parent_type;

    explicit RawMessage(const std::string& data) : parent_type{data.begin(), data.end()} {}

    parent_type& operator+=(value_type rhs) { return append(rhs); }
    parent_type& operator+=(std::string::value_type rhs) { return append(rhs); }
    parent_type& operator+=(const parent_type& rhs) { return append(rhs); }
    parent_type& operator+=(const std::string& rhs) { return append(rhs); }

    parent_type& append(value_type rhs)
    {
        push_back(rhs);
        return *this;
    }
    parent_type& append(std::string::value_type rhs)
    {
        push_back(static_cast<value_type>(rhs));
        return *this;
    }
    parent_type& append(const parent_type& rhs)
    {
        insert(end(), rhs.begin(), rhs.end());
        return *this;
    }
    parent_type& append(const std::string& rhs)
    {
        insert(end(), rhs.begin(), rhs.end());
        return *this;
    }
};

class RawSender
{
public:
    RawSender(const std::string& host, uint16_t port, std::optional<std::string>& error);
    ~RawSender();

    RawSender(const RawSender&) = delete;
    RawSender& operator=(const RawSender&) = delete;
    RawSender(RawSender&&) = delete;

    std::optional<std::string> Send(const RawMessage& msg);

    std::string ToStringHostPort() const;

private:
    /* Socket used to communicate with host */
    std::unique_ptr<Sock> m_sock{nullptr};
    /* Socket address containing host information */
    std::pair<struct sockaddr_storage, socklen_t> m_server{{}, sizeof(struct sockaddr_storage)};

    /* Hostname of server receiving messages */
    const std::string m_host;
    /* Port of server receiving messages */
    const uint16_t m_port;

    /* Number of messages sent */
    uint64_t m_successes{0};
    /* Number of messages not sent */
    uint64_t m_failures{0};
};

#endif // BITCOIN_STATS_RAWSENDER_H
