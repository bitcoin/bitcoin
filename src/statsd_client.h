#ifndef BITCOIN_STATSD_CLIENT_H
#define BITCOIN_STATSD_CLIENT_H

#include <string>

static const bool DEFAULT_STATSD_ENABLE = false;
static const int DEFAULT_STATSD_PORT = 8125;
static const std::string DEFAULT_STATSD_HOST = "127.0.0.1";
static const std::string DEFAULT_STATSD_HOSTNAME = "";
static const std::string DEFAULT_STATSD_NAMESPACE = "";

// schedule periodic measurements, in seconds: default - 1 minute, min - 5 sec, max - 1h.
static const int DEFAULT_STATSD_PERIOD = 60;
static const int MIN_STATSD_PERIOD = 5;
static const int MAX_STATSD_PERIOD = 60 * 60;

namespace statsd {

struct _StatsdClientData;

class StatsdClient {
    public:
        StatsdClient(const std::string& host = DEFAULT_STATSD_HOST, int port = DEFAULT_STATSD_PORT, const std::string& ns = DEFAULT_STATSD_NAMESPACE);
        ~StatsdClient();

    public:
        // you can config at anytime; client will use new address (useful for Singleton)
        void config(const std::string& host, int port, const std::string& ns = DEFAULT_STATSD_NAMESPACE);
        const char* errmsg();

    public:
        int inc(const std::string& key, float sample_rate = 1.0);
        int dec(const std::string& key, float sample_rate = 1.0);
        int count(const std::string& key, size_t value, float sample_rate = 1.0);
        int gauge(const std::string& key, size_t value, float sample_rate = 1.0);
        int gaugeDouble(const std::string& key, double value, float sample_rate = 1.0);
        int timing(const std::string& key, size_t ms, float sample_rate = 1.0);

    public:
        /**
         * (Low Level Api) manually send a message
         * which might be composed of several lines.
         */
        int send(const std::string& message);

        /* (Low Level Api) manually send a message
         * type = "c", "g" or "ms"
         */
        int send(std::string key, size_t value,
                const std::string& type, float sample_rate);
        int sendDouble(std::string key, double value,
                const std::string& type, float sample_rate);

    protected:
        int init();
        static void cleanup(std::string& key);

    protected:
        struct _StatsdClientData* d;
};

} // namespace statsd

extern statsd::StatsdClient statsClient;

#endif // BITCOIN_STATSD_CLIENT_H
