
#ifndef STATSD_CLIENT_H
#define STATSD_CLIENT_H

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>

namespace statsd {

struct _StatsdClientData;

class StatsdClient {
    public:
        StatsdClient(const std::string& host="127.0.0.1", int port=8125, const std::string& ns = "");
        ~StatsdClient();

    public:
        // you can config at anytime; client will use new address (useful for Singleton)
        void config(const std::string& host, int port, const std::string& ns = "");
        const char* errmsg();

    public:
        int inc(const std::string& key, float sample_rate = 1.0);
        int dec(const std::string& key, float sample_rate = 1.0);
        int count(const std::string& key, size_t value, float sample_rate = 1.0);
        int gauge(const std::string& key, size_t value, float sample_rate = 1.0);
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

    protected:
        int init();
        void cleanup(std::string& key);

    protected:
        struct _StatsdClientData* d;
};

}; // end namespace

#endif
