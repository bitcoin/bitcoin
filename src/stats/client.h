// Copyright (c) 2014-2017 Statoshi Developers
// Copyright (c) 2017-2023 Vincent Thiery
// Copyright (c) 2020-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STATS_CLIENT_H
#define BITCOIN_STATS_CLIENT_H

#include <random.h>
#include <sync.h>

#include <memory>
#include <string>

class ArgsManager;
class RawSender;

/** Default port used to connect to a Statsd server */
static constexpr uint16_t DEFAULT_STATSD_PORT{8125};
/** Default host assumed to be running a Statsd server */
static const std::string DEFAULT_STATSD_HOST{""};
/** Default prefix prepended to Statsd message keys */
static const std::string DEFAULT_STATSD_PREFIX{""};
/** Default suffix appended to Statsd message keys */
static const std::string DEFAULT_STATSD_SUFFIX{""};

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
                          const std::string& prefix, const std::string& suffix, bool enabled);
    ~StatsdClient();

public:
    /* Statsd-defined APIs */
    bool dec(const std::string& key, float sample_rate = 1.f);
    bool inc(const std::string& key, float sample_rate = 1.f);
    bool count(const std::string& key, int64_t delta, float sample_rate = 1.f);
    bool gauge(const std::string& key, int64_t value, float sample_rate = 1.f);
    bool gaugeDouble(const std::string& key, double value, float sample_rate = 1.f);
    bool timing(const std::string& key, uint64_t ms, float sample_rate = 1.f);

    /* Statsd-compatible APIs */
    template <typename T1>
    bool send(const std::string& key, T1 value, const std::string& type, float sample_rate = 1.f);

    /* Check if a StatsdClient instance is ready to send messages */
    bool active() const { return m_sender != nullptr; }

private:
    /* Mutex to protect PRNG */
    mutable Mutex cs;
    /* PRNG used to dice-roll messages that are 0 < f < 1 */
    mutable FastRandomContext insecure_rand GUARDED_BY(cs);

    /* Broadcasts messages crafted by StatsdClient */
    std::unique_ptr<RawSender> m_sender{nullptr};

    /* Phrase prepended to keys */
    const std::string m_prefix{""};
    /* Phrase appended to keys */
    const std::string m_suffix{""};
};

/** Parses arguments and constructs a StatsdClient instance */
std::unique_ptr<StatsdClient> InitStatsClient(const ArgsManager& args);

/** Global smart pointer containing StatsdClient instance */
extern std::unique_ptr<StatsdClient> g_stats_client;

#endif // BITCOIN_STATS_CLIENT_H
