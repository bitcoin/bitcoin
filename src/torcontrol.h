// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Functionality for communicating with Tor.
 */
#ifndef BITCOIN_TORCONTROL_H
#define BITCOIN_TORCONTROL_H

#include <netaddress.h>
#include <util/fs.h>
#include <util/sock.h>
#include <util/threadinterrupt.h>

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

constexpr uint16_t DEFAULT_TOR_SOCKS_PORT{9050};
constexpr int DEFAULT_TOR_CONTROL_PORT = 9051;
extern const std::string DEFAULT_TOR_CONTROL;
static const bool DEFAULT_LISTEN_ONION = true;

void StartTorControl(CService onion_service_target);
void InterruptTorControl();
void StopTorControl();

CService DefaultOnionServiceTarget(uint16_t port);

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
    typedef std::function<void(TorControlConnection &,const TorControlReply &)> ReplyHandlerCB;

    /** Create a new TorControlConnection.
     */
    explicit TorControlConnection(CThreadInterrupt& interrupt);
    ~TorControlConnection();

    /**
     * Connect to a Tor control port.
     * tor_control_center is address of the form host:port.
     * Return true on success.
     */
    bool Connect(const std::string& tor_control_center);

    /**
     * Disconnect from Tor control port.
     */
    void Disconnect();

    /** Send a command, register a handler for the reply.
     * A trailing CRLF is automatically added.
     * Return true on success.
     */
    bool Command(const std::string &cmd, const ReplyHandlerCB& reply_handler);

    /**
     * Check if the connection is established.
     */
    bool IsConnected() const;

    /**
     * Wait for data to be available on the socket.
     * @param[in] timeout Maximum time to wait
     * @return true if data is available or timeout occurred, false on error
     */
    bool WaitForData(std::chrono::milliseconds timeout);

    /**
     * Read available data from socket and process complete replies.
     * Dispatches to registered reply handlers.
     * @return true if connection is still open, false if connection was closed
     */
    bool ReceiveAndProcess();

private:
    /** Reference to interrupt object for clean shutdown */
    CThreadInterrupt& m_interrupt;
    /** Socket for the connection */
    std::unique_ptr<Sock> m_sock;
    /** Message being received */
    TorControlReply m_message;
    /** Response handlers */
    std::deque<ReplyHandlerCB> m_reply_handlers;
    /** Buffer for incoming data */
    std::vector<std::byte> m_recv_buffer;
    /** Process complete lines from the receive buffer */
    bool ProcessBuffer();
};

/****** Bitcoin specific TorController implementation ********/

/** Controller that connects to Tor control socket, authenticate, then create
 * and maintain an ephemeral onion service.
 */
class TorController
{
public:
    TorController(const std::string& tor_control_center, const CService& target);
    TorController() : m_conn(m_interrupt) {
        // Used for testing only.
    }
    ~TorController();

    /** Get name of file to store private key in */
    fs::path GetPrivateKeyFile();

    /** Interrupt the controller thread */
    void Interrupt();

    /** Wait for the controller thread to exit */
    void Join();
private:
    CThreadInterrupt m_interrupt;
    std::thread m_thread;
    const std::string m_tor_control_center;
    TorControlConnection m_conn;
    std::string m_private_key;
    std::string m_service_id;
    bool m_reconnect;
    std::chrono::duration<double> m_reconnect_timeout{1.0};
    CService m_service;
    const CService m_target;
    /** Cookie for SAFECOOKIE auth */
    std::vector<uint8_t> m_cookie;
    /** ClientNonce for SAFECOOKIE auth */
    std::vector<uint8_t> m_client_nonce;
    /** Main control thread */
    void ThreadControl();

public:
    /** Callback for GETINFO net/listeners/socks result */
    void get_socks_cb(TorControlConnection& conn, const TorControlReply& reply);
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
};

#endif // BITCOIN_TORCONTROL_H
