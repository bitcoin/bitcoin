// Copyright (c) 2014-2017 Statoshi Developers
// Copyright (c) 2017-2023 Vincent Thiery
// Copyright (c) 2020-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stats/client.h>

#include <logging.h>
#include <random.h>
#include <stats/rawsender.h>
#include <sync.h>
#include <util/system.h>

#include <algorithm>
#include <cmath>
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

class StatsdClientImpl final : public StatsdClient
{
public:
    explicit StatsdClientImpl(const std::string& host, uint16_t port, uint64_t batch_size, uint64_t interval_ms,
                              const std::string& prefix, const std::string& suffix);
    ~StatsdClientImpl() = default;

public:
    bool dec(std::string_view key, float sample_rate) override { return count(key, -1, sample_rate); }
    bool inc(std::string_view key, float sample_rate) override { return count(key, 1, sample_rate); }
    bool count(std::string_view key, int64_t delta, float sample_rate) override { return _send(key, delta, STATSD_METRIC_COUNT, sample_rate); }
    bool gauge(std::string_view key, int64_t value, float sample_rate) override { return _send(key, value, STATSD_METRIC_GAUGE, sample_rate); }
    bool gaugeDouble(std::string_view key, double value, float sample_rate) override { return _send(key, value, STATSD_METRIC_GAUGE, sample_rate); }
    bool timing(std::string_view key, uint64_t ms, float sample_rate) override { return _send(key, ms, STATSD_METRIC_TIMING, sample_rate); }

    bool send(std::string_view key, double value, std::string_view type, float sample_rate) override { return _send(key, value, type, sample_rate); }
    bool send(std::string_view key, int32_t value, std::string_view type, float sample_rate) override { return _send(key, value, type, sample_rate); }
    bool send(std::string_view key, int64_t value, std::string_view type, float sample_rate) override { return _send(key, value, type, sample_rate); }
    bool send(std::string_view key, uint32_t value, std::string_view type, float sample_rate) override { return _send(key, value, type, sample_rate); }
    bool send(std::string_view key, uint64_t value, std::string_view type, float sample_rate) override { return _send(key, value, type, sample_rate); }

    bool active() const override { return m_sender != nullptr; }

private:
    template <typename T1>
    inline bool _send(std::string_view key, T1 value, std::string_view type, float sample_rate);

private:
    /* Mutex to protect PRNG */
    mutable Mutex cs;
    /* PRNG used to dice-roll messages that are 0 < f < 1 */
    mutable FastRandomContext insecure_rand GUARDED_BY(cs);

    /* Broadcasts messages crafted by StatsdClient */
    std::unique_ptr<RawSender> m_sender{nullptr};

    /* Phrase prepended to keys */
    const std::string m_prefix{};
    /* Phrase appended to keys */
    const std::string m_suffix{};
};
} // anonymous namespace

std::unique_ptr<StatsdClient> g_stats_client;

std::unique_ptr<StatsdClient> StatsdClient::make(const ArgsManager& args)
{
    auto sanitize_string = [](std::string string) {
        // Remove key delimiters from the front and back as they're added back in
        // the constructor
        if (!string.empty()) {
            if (string.front() == STATSD_NS_DELIMITER) string.erase(string.begin());
            if (string.back() == STATSD_NS_DELIMITER) string.pop_back();
        }
        return string;
    };

    return std::make_unique<StatsdClientImpl>(
        args.GetArg("-statshost", DEFAULT_STATSD_HOST),
        args.GetIntArg("-statsport", DEFAULT_STATSD_PORT),
        args.GetIntArg("-statsbatchsize", DEFAULT_STATSD_BATCH_SIZE),
        args.GetIntArg("-statsduration", DEFAULT_STATSD_DURATION),
        sanitize_string(args.GetArg("-statsprefix", DEFAULT_STATSD_PREFIX)),
        sanitize_string(args.GetArg("-statssuffix", DEFAULT_STATSD_SUFFIX))
    );
}

StatsdClientImpl::StatsdClientImpl(const std::string& host, uint16_t port, uint64_t batch_size, uint64_t interval_ms,
                                   const std::string& prefix, const std::string& suffix) :
    m_prefix{[prefix]() { return !prefix.empty() ? prefix + STATSD_NS_DELIMITER : prefix; }()},
    m_suffix{[suffix]() { return !suffix.empty() ? STATSD_NS_DELIMITER + suffix : suffix; }()}
{
    if (host.empty()) {
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

template <typename T1>
inline bool StatsdClientImpl::_send(std::string_view key, T1 value, std::string_view type, float sample_rate)
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
