// Copyright (c) 2014-2017 Statoshi Developers
// Copyright (c) 2017-2023 Vincent Thiery
// Copyright (c) 2020-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STATS_CLIENT_H
#define BITCOIN_STATS_CLIENT_H

#include <cstdint>
#include <memory>
#include <string>

class ArgsManager;

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
    static std::unique_ptr<StatsdClient> make(const ArgsManager& args);
    virtual ~StatsdClient() = default;

    /* Statsd-defined APIs */
    virtual bool dec(const std::string& key, float sample_rate = 1.f) { return false; }
    virtual bool inc(const std::string& key, float sample_rate = 1.f) { return false; }
    virtual bool count(const std::string& key, int64_t delta, float sample_rate = 1.f) { return false; }
    virtual bool gauge(const std::string& key, int64_t value, float sample_rate = 1.f) { return false; }
    virtual bool gaugeDouble(const std::string& key, double value, float sample_rate = 1.f) { return false; }
    virtual bool timing(const std::string& key, uint64_t ms, float sample_rate = 1.f) { return false; }

    /* Statsd-compatible APIs */
    virtual bool send(const std::string& key, double value, const std::string& type, float sample_rate = 1.f) { return false; }
    virtual bool send(const std::string& key, int32_t value, const std::string& type, float sample_rate = 1.f) { return false; }
    virtual bool send(const std::string& key, int64_t value, const std::string& type, float sample_rate = 1.f) { return false; }
    virtual bool send(const std::string& key, uint32_t value, const std::string& type, float sample_rate = 1.f) { return false; }
    virtual bool send(const std::string& key, uint64_t value, const std::string& type, float sample_rate = 1.f) { return false; }

    /* Check if a StatsdClient instance is ready to send messages */
    virtual bool active() const { return false; }
};

/** Global smart pointer containing StatsdClient instance */
extern std::unique_ptr<StatsdClient> g_stats_client;

#endif // BITCOIN_STATS_CLIENT_H
