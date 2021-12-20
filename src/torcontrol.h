// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Functionality for communicating with Tor.
 */
#ifndef BITCOIN_TORCONTROL_H
#define BITCOIN_TORCONTROL_H

#include <fs.h>
#include <netaddress.h>

#include <boost/signals2/signal.hpp>

#include <event2/bufferevent.h>
#include <event2/event.h>

#include <cstdlib>
#include <deque>
#include <functional>
#include <string>
#include <vector>

class CService;

extern const std::string DEFAULT_TOR_CONTROL;
static const bool DEFAULT_LISTEN_ONION = true;

void StartTorControl(CService onion_service_target);
void InterruptTorControl();
void StopTorControl();

CService DefaultOnionServiceTarget();

/** Reply from Tor, can be single or multi-line */
class TorControlReply
{
public:
    TorControlReply() { Clear(); }

    int code;
    std::vector<std::string> lines;

    void Clear()
    {
        code = 0;
        lines.clear();
    }
};

/** Low-level handling for Tor control connection.
 * Speaks the SMTP-like protocol as defined in torspec/control-spec.txt
 */
class TorControlConnection
{
public:
    typedef std::function<void(TorControlConnection&)> ConnectionCB;
    typedef std::function<void(TorControlConnection &,const TorControlReply &)> ReplyHandlerCB;

    /** Create a new TorControlConnection.
     */
    explicit TorControlConnection(struct event_base *base);
    ~TorControlConnection();

    /**
     * Connect to a Tor control port.
     * tor_control_center is address of the form host:port.
     * connected is the handler that is called when connection is successfully established.
     * disconnected is a handler that is called when the connection is broken.
     * Return true on success.
     */
    bool Connect(const std::string& tor_control_center, const ConnectionCB& connected, const ConnectionCB& disconnected);

    /**
     * Disconnect from Tor control port.
     */
    void Disconnect();

    /** Send a command, register a handler for the reply.
     * A trailing CRLF is automatically added.
     * Return true on success.
     */
    bool Command(const std::string &cmd, const ReplyHandlerCB& reply_handler);

    /** Response handlers for async replies */
    boost::signals2::signal<void(TorControlConnection &,const TorControlReply &)> async_handler;
private:
    /** Callback when ready for use */
    std::function<void(TorControlConnection&)> connected;
    /** Callback when connection lost */
    std::function<void(TorControlConnection&)> disconnected;
    /** Libevent event base */
    struct event_base *base;
    /** Connection to control socket */
    struct bufferevent *b_conn;
    /** Message being received */
    TorControlReply message;
    /** Response handlers */
    std::deque<ReplyHandlerCB> reply_handlers;

    /** Libevent handlers: internal */
    static void readcb(struct bufferevent *bev, void *ctx);
    static void eventcb(struct bufferevent *bev, short what, void *ctx);
};

/****** Bitcoin specific TorController implementation ********/

/** Controller that connects to Tor control socket, authenticate, then create
 * and maintain an ephemeral onion service.
 */
class TorController
{
public:
    TorController(struct event_base* base, const std::string& tor_control_center, const CService& target);
    TorController() : conn{nullptr} {
        // Used for testing only.
    }
    ~TorController();

    /** Get name of file to store private key in */
    fs::path GetPrivateKeyFile();

    /** Reconnect, after getting disconnected */
    void Reconnect();
private:
    struct event_base* base;
    const std::string m_tor_control_center;
    TorControlConnection conn;
    std::string private_key;
    std::string service_id;
    bool reconnect;
    struct event *reconnect_ev = nullptr;
    float reconnect_timeout;
    CService service;
    const CService m_target;
    /** Cookie for SAFECOOKIE auth */
    std::vector<uint8_t> cookie;
    /** ClientNonce for SAFECOOKIE auth */
    std::vector<uint8_t> clientNonce;

public:
    /** Callback for ADD_ONION result */
    void add_onion_cb(TorControlConnection& conn, const TorControlReply& reply);
    /** Callback for AUTHENTICATE result */
    void auth_cb(TorControlConnection& conn, const TorControlReply& reply);
    /** Callback for AUTHCHALLENGE result */
    void authchallenge_cb(TorControlConnection& conn, const TorControlReply& reply);
    /** Callback for PROTOCOLINFO result */
    void protocolinfo_cb(TorControlConnection& conn, const TorControlReply& reply);
    /** Callback after successful connection */
    void connected_cb(TorControlConnection& conn);
    /** Callback after connection lost or failed connection attempt */
    void disconnected_cb(TorControlConnection& conn);

    /** Callback for reconnect timer */
    static void reconnect_cb(evutil_socket_t fd, short what, void *arg);
};

#endif // BITCOIN_TORCONTROL_H
