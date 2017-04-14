// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SUPPORT_EVENTS_H
#define BITCOIN_SUPPORT_EVENTS_H

#include <ios>
#include <memory>

#include <event2/event.h>
#include <event2/http.h>

#define MAKE_RAII(type) \
/* deleter */\
struct type##_deleter {\
    void operator()(struct type* ob) {\
        type##_free(ob);\
    }\
};\
/* unique ptr typedef */\
typedef std::unique_ptr<struct type, type##_deleter> raii_##type

MAKE_RAII(event_base);
MAKE_RAII(event);
MAKE_RAII(evhttp);
MAKE_RAII(evhttp_request);
MAKE_RAII(evhttp_connection);

inline raii_event_base obtain_event_base() {
    auto result = raii_event_base(event_base_new());
    if (!result.get())
        throw std::runtime_error("cannot create event_base");
    return result;
}

inline raii_event obtain_event(struct event_base* base, evutil_socket_t s, short events, event_callback_fn cb, void* arg) {
    return raii_event(event_new(base, s, events, cb, arg));
}

inline raii_evhttp obtain_evhttp(struct event_base* base) {
    return raii_evhttp(evhttp_new(base));
}

inline raii_evhttp_request obtain_evhttp_request(void(*cb)(struct evhttp_request *, void *), void *arg) {
    return raii_evhttp_request(evhttp_request_new(cb, arg));
}

inline raii_evhttp_connection obtain_evhttp_connection_base(struct event_base* base, std::string host, uint16_t port) {
    auto result = raii_evhttp_connection(evhttp_connection_base_new(base, NULL, host.c_str(), port));
    if (!result.get())
        throw std::runtime_error("create connection failed");
    return result;
}

#endif // BITCOIN_SUPPORT_EVENTS_H
