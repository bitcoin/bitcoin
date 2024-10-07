// Copyright (c) 2014-2017 Statoshi Developers
// Copyright (c) 2017-2023 Vincent Thiery
// Copyright (c) 2020-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stats/client.h>

#include <stats/rawsender.h>
#include <util/system.h>

#include <cmath>
#include <cstdio>
#include <random>

namespace {
/** Threshold below which a value is considered effectively zero */
static constexpr float EPSILON{0.0001f};

/** Delimiter segmenting two fully formed Statsd messages */
static constexpr char STATSD_MSG_DELIMITER{'\n'};
/** Delimiter segmenting namespaces in a Statsd key */
static constexpr char STATSD_NS_DELIMITER{'.'};
/** Character used to denote Statsd message type as count */
static constexpr char STATSD_METRIC_COUNT[]{"c"};
/** Character used to denote Statsd message type as gauge */
static constexpr char STATSD_METRIC_GAUGE[]{"g"};
/** Characters used to denote Statsd message type as timing */
static constexpr char STATSD_METRIC_TIMING[]{"ms"};
} // anonymous namespace

std::unique_ptr<StatsdClient> g_stats_client;

std::unique_ptr<StatsdClient> InitStatsClient(const ArgsManager& args)
{
    auto is_enabled = args.GetBoolArg("-statsenabled", /*fDefault=*/false);
    auto host = args.GetArg("-statshost", /*fDefault=*/DEFAULT_STATSD_HOST);

    if (is_enabled && host.empty()) {
        // Stats are enabled but host has not been specified, then use
        // default legacy host. This is to preserve old behavior.
        host = "127.0.0.1";
    } else if (!host.empty()) {
        // Host is specified but stats are not explcitly enabled. Assume
        // that if a host has been specified, we want stats enabled. This
        // is new behaviour and will substitute old behaviour in a future
        // release.
        is_enabled = true;
    }

    auto sanitize_string = [](std::string& string) {
        // Remove key delimiters from the front and back as they're added back
        if (!string.empty()) {
            if (string.front() == STATSD_NS_DELIMITER) string.erase(string.begin());
            if (string.back() == STATSD_NS_DELIMITER) string.pop_back();
        }
    };

    // Get our prefix and suffix and if we get nothing, try again with the
    // deprecated argument. If we still get nothing, that's fine, they're optional.
    auto prefix = args.GetArg("-statsprefix", DEFAULT_STATSD_PREFIX);
    if (prefix.empty()) {
        prefix = args.GetArg("-statsns", DEFAULT_STATSD_PREFIX);
    } else {
        // We restrict sanitization logic to our newly added arguments to
        // prevent breaking changes.
        sanitize_string(prefix);
        // We need to add the delimiter here for backwards compatibility with
        // the deprecated argument.
        //
        // TODO: Move this step into the constructor when removing deprecated
        //       args support
        prefix += STATSD_NS_DELIMITER;
    }

    auto suffix = args.GetArg("-statssuffix", DEFAULT_STATSD_SUFFIX);
    if (suffix.empty()) {
        suffix = args.GetArg("-statshostname", DEFAULT_STATSD_SUFFIX);
    } else {
        // We restrict sanitization logic to our newly added arguments to
        // prevent breaking changes.
        sanitize_string(suffix);
    }

    return std::make_unique<StatsdClient>(
        host,
        args.GetArg("-statsport", DEFAULT_STATSD_PORT),
        args.GetArg("-statsbatchsize", DEFAULT_STATSD_BATCH_SIZE),
        args.GetArg("-statsduration", DEFAULT_STATSD_DURATION),
        prefix,
        suffix,
        is_enabled
    );
}

StatsdClient::StatsdClient(const std::string& host, uint16_t port, uint64_t batch_size, uint64_t interval_ms,
                           const std::string& prefix, const std::string& suffix, bool enabled) :
    m_prefix{prefix},
    m_suffix{[suffix]() { return !suffix.empty() ? STATSD_NS_DELIMITER + suffix : suffix; }()}
{
    if (!enabled) {
        LogPrintf("Transmitting stats are disabled, will not init StatsdClient\n");
        return;
    }

    std::optional<std::string> error_opt;
    m_sender = std::make_unique<RawSender>(host, port,
                                           std::make_pair(batch_size, static_cast<uint8_t>(STATSD_MSG_DELIMITER)),
                                           interval_ms, error_opt);
    if (error_opt.has_value()) {
        LogPrintf("ERROR: %s, cannot initialize StatsdClient.\n", error_opt.value());
        m_sender.reset();
        return;
    }

    LogPrintf("StatsdClient initialized to transmit stats to %s:%d\n", host, port);
}

StatsdClient::~StatsdClient() {}

bool StatsdClient::dec(const std::string& key, float sample_rate) { return count(key, -1, sample_rate); }

bool StatsdClient::inc(const std::string& key, float sample_rate) { return count(key, 1, sample_rate); }

bool StatsdClient::count(const std::string& key, int64_t delta, float sample_rate)
{
    return send(key, delta, STATSD_METRIC_COUNT, sample_rate);
}

bool StatsdClient::gauge(const std::string& key, int64_t value, float sample_rate)
{
    return send(key, value, STATSD_METRIC_GAUGE, sample_rate);
}

bool StatsdClient::gaugeDouble(const std::string& key, double value, float sample_rate)
{
    return send(key, value, STATSD_METRIC_GAUGE, sample_rate);
}

bool StatsdClient::timing(const std::string& key, uint64_t ms, float sample_rate)
{
    return send(key, ms, STATSD_METRIC_TIMING, sample_rate);
}

template <typename T1>
bool StatsdClient::send(const std::string& key, T1 value, const std::string& type, float sample_rate)
{
    static_assert(std::is_arithmetic<T1>::value, "Must specialize to an arithmetic type");

    if (!m_sender) {
        return false;
    }

    // Determine if we should send the message at all but claim that we did even if we don't
    sample_rate = std::clamp(sample_rate, 0.f, 1.f);
    bool always_send = std::fabs(sample_rate - 1.f) < EPSILON;
    bool never_send = std::fabs(sample_rate) < EPSILON;
    if (never_send || (!always_send &&
                       WITH_LOCK(cs, return sample_rate < std::uniform_real_distribution<float>(0.f, 1.f)(insecure_rand)))) {
        return true;
    }

    // Construct the message and if our message isn't always-send, report the sample rate
    RawMessage msg{strprintf("%s%s%s:%f|%s", m_prefix, key, m_suffix, value, type)};
    if (!always_send) {
        msg += strprintf("|@%.2f", sample_rate);
    }

    // Send it and report an error if we encounter one
    if (auto error_opt = m_sender->Send(msg); error_opt.has_value()) {
        LogPrintf("ERROR: %s.\n", error_opt.value());
        return false;
    }

    return true;
}

template bool StatsdClient::send(const std::string& key, double value, const std::string& type, float sample_rate);
template bool StatsdClient::send(const std::string& key, int32_t value, const std::string& type, float sample_rate);
template bool StatsdClient::send(const std::string& key, int64_t value, const std::string& type, float sample_rate);
template bool StatsdClient::send(const std::string& key, uint32_t value, const std::string& type, float sample_rate);
template bool StatsdClient::send(const std::string& key, uint64_t value, const std::string& type, float sample_rate);
