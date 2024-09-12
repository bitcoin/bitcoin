// Copyright (c) 2014-2017 Statoshi Developers
// Copyright (c) 2017-2023 Vincent Thiery
// Copyright (c) 2020-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STATS_CLIENT_H
#define BITCOIN_STATS_CLIENT_H

#include <random.h>
#include <sync.h>

#include <memory>
#include <string>

class RawSender;

static constexpr bool DEFAULT_STATSD_ENABLE{false};
static constexpr uint16_t DEFAULT_STATSD_PORT{8125};
static const std::string DEFAULT_STATSD_HOST{"127.0.0.1"};
static const std::string DEFAULT_STATSD_HOSTNAME{""};
static const std::string DEFAULT_STATSD_NAMESPACE{""};

/** Default number of milliseconds between flushing a queue of messages */
static constexpr int DEFAULT_STATSD_DURATION{1000};
/** Default number of seconds between recording periodic stats */
static constexpr int DEFAULT_STATSD_PERIOD{60};
/** Default size in bytes of a batch of messages */
static constexpr int DEFAULT_STATSD_BATCH_SIZE{1024};
/** Minimum number of seconds between recording periodic stats */
static constexpr int MIN_STATSD_PERIOD{5};
/** Maximum number of seconds between recording periodic stats */
static constexpr int MAX_STATSD_PERIOD{60 * 60};

class StatsdClient
{
public:
    explicit StatsdClient(const std::string& host, uint16_t port, uint64_t batch_size, uint64_t interval_ms,
                          const std::string& nodename, const std::string& ns, bool enabled);
    ~StatsdClient();

public:
    bool inc(const std::string& key, float sample_rate = 1.f);
    bool dec(const std::string& key, float sample_rate = 1.f);
    bool count(const std::string& key, int64_t value, float sample_rate = 1.f);
    bool gauge(const std::string& key, int64_t value, float sample_rate = 1.f);
    bool gaugeDouble(const std::string& key, double value, float sample_rate = 1.f);
    bool timing(const std::string& key, int64_t ms, float sample_rate = 1.f);

    /* (Low Level Api) manually send a message
        * type = "c", "g" or "ms"
        */
    bool send(std::string key, int64_t value, const std::string& type, float sample_rate);
    bool sendDouble(std::string key, double value, const std::string& type, float sample_rate);

private:
    void cleanup(std::string& key);
    bool ShouldSend(float sample_rate);

private:
    mutable Mutex cs;
    mutable FastRandomContext insecure_rand GUARDED_BY(cs);

    std::unique_ptr<RawSender> m_sender{nullptr};

    const std::string m_nodename;
    const std::string m_ns;
};

extern std::unique_ptr<StatsdClient> g_stats_client;

#endif // BITCOIN_STATS_CLIENT_H
