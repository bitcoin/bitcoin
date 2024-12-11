// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_HTTPSERVER_H
#define BITCOIN_HTTPSERVER_H

#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>

#include <rpc/protocol.h>
#include <common/sockman.h>
#include <util/strencodings.h>
#include <util/string.h>

namespace util {
class SignalInterrupt;
} // namespace util

/**
 * The default value for `-rpcthreads`. This number of threads will be created at startup.
 */
static const int DEFAULT_HTTP_THREADS=16;

/**
 * The default value for `-rpcworkqueue`. This is the maximum depth of the work queue,
 * we don't allocate this number of work queue items upfront.
 */
static const int DEFAULT_HTTP_WORKQUEUE=64;

static const int DEFAULT_HTTP_SERVER_TIMEOUT=30;

struct evhttp_request;
struct event_base;
class CService;

namespace http_libevent {
class HTTPRequest;

/** Initialize HTTP server.
 * Call this before RegisterHTTPHandler or EventBase().
 */
bool InitHTTPServer(const util::SignalInterrupt& interrupt);
/** Start HTTP server.
 * This is separate from InitHTTPServer to give users race-condition-free time
 * to register their handlers between InitHTTPServer and StartHTTPServer.
 */
void StartHTTPServer();
/** Interrupt HTTP server threads */
void InterruptHTTPServer();
/** Stop HTTP server */
void StopHTTPServer();

/** Change logging level for libevent. */
void UpdateHTTPServerLogging(bool enable);
} // namespace http_libevent

/** Handler for requests to a certain HTTP path */
typedef std::function<bool(http_libevent::HTTPRequest* req, const std::string&)> HTTPRequestHandler;
/** Register handler for prefix.
 * If multiple handlers match a prefix, the first-registered one will
 * be invoked.
 */
void RegisterHTTPHandler(const std::string &prefix, bool exactMatch, const HTTPRequestHandler &handler);
/** Unregister handler for prefix */
void UnregisterHTTPHandler(const std::string &prefix, bool exactMatch);

/** Return evhttp event base. This can be used by submodules to
 * queue timers or custom events.
 */
struct event_base* EventBase();

namespace http_libevent {
/** In-flight HTTP request.
 * Thin C++ wrapper around evhttp_request.
 */
class HTTPRequest
{
private:
    struct evhttp_request* req;
    const util::SignalInterrupt& m_interrupt;
    bool replySent;

public:
    explicit HTTPRequest(struct evhttp_request* req, const util::SignalInterrupt& interrupt, bool replySent = false);
    ~HTTPRequest();

    enum RequestMethod {
        UNKNOWN,
        GET,
        POST,
        HEAD,
        PUT
    };

    /** Get requested URI.
     */
    std::string GetURI() const;

    /** Get CService (address:ip) for the origin of the http request.
     */
    CService GetPeer() const;

    /** Get request method.
     */
    RequestMethod GetRequestMethod() const;

    /** Get the query parameter value from request uri for a specified key, or std::nullopt if the
     * key is not found.
     *
     * If the query string contains duplicate keys, the first value is returned. Many web frameworks
     * would instead parse this as an array of values, but this is not (yet) implemented as it is
     * currently not needed in any of the endpoints.
     *
     * @param[in] key represents the query parameter of which the value is returned
     */
    std::optional<std::string> GetQueryParameter(const std::string& key) const;

    /**
     * Get the request header specified by hdr, or an empty string.
     * Return a pair (isPresent,string).
     */
    std::pair<bool, std::string> GetHeader(const std::string& hdr) const;

    /**
     * Read request body.
     *
     * @note As this consumes the underlying buffer, call this only once.
     * Repeated calls will return an empty string.
     */
    std::string ReadBody();

    /**
     * Write output header.
     *
     * @note call this before calling WriteErrorReply or Reply.
     */
    void WriteHeader(const std::string& hdr, const std::string& value);

    /**
     * Write HTTP reply.
     * nStatus is the HTTP status code to send.
     * reply is the body of the reply. Keep it empty to send a standard message.
     *
     * @note Can be called only once. As this will give the request back to the
     * main thread, do not call any other HTTPRequest methods after calling this.
     */
    void WriteReply(int nStatus, std::string_view reply = "")
    {
        WriteReply(nStatus, std::as_bytes(std::span{reply}));
    }
    void WriteReply(int nStatus, std::span<const std::byte> reply);
};


/** Get the query parameter value from request uri for a specified key, or std::nullopt if the key
 * is not found.
 *
 * If the query string contains duplicate keys, the first value is returned. Many web frameworks
 * would instead parse this as an array of values, but this is not (yet) implemented as it is
 * currently not needed in any of the endpoints.
 *
 * Helper function for HTTPRequest::GetQueryParameter.
 *
 * @param[in] uri is the entire request uri
 * @param[in] key represents the query parameter of which the value is returned
 */
std::optional<std::string> GetQueryParameterFromUri(const char* uri, const std::string& key);
} // namespace http_libevent

/** Event handler closure.
 */
class HTTPClosure
{
public:
    virtual void operator()() = 0;
    virtual ~HTTPClosure() = default;
};

/** Event class. This can be used either as a cross-thread trigger or as a timer.
 */
class HTTPEvent
{
public:
    /** Create a new event.
     * deleteWhenTriggered deletes this event object after the event is triggered (and the handler called)
     * handler is the handler to call when the event is triggered.
     */
    HTTPEvent(struct event_base* base, bool deleteWhenTriggered, const std::function<void()>& handler);
    ~HTTPEvent();

    /** Trigger the event. If tv is 0, trigger it immediately. Otherwise trigger it after
     * the given time has elapsed.
     */
    void trigger(struct timeval* tv);

    bool deleteWhenTriggered;
    std::function<void()> handler;
private:
    struct event* ev;
};

namespace http_bitcoin {
using util::LineReader;
using NodeId = SockMan::Id;

// shortest valid request line, used by libevent in evhttp_parse_request_line()
static const size_t MIN_REQUEST_LINE_LENGTH{strlen("GET / HTTP/1.0")};
// maximum size of http request (request line + headers)
// see https://github.com/bitcoin/bitcoin/issues/6425
static const size_t MAX_HEADERS_SIZE{8192};

class HTTPHeaders
{
public:
    std::optional<std::string> Find(const std::string key) const;
    void Write(const std::string key, const std::string value);
    void Remove(const std::string key);
    bool Read(util::LineReader& reader);
    std::string Stringify() const;

private:
    std::map<std::string, std::string, util::CaseInsensitiveComparator> m_map;
};

class HTTPResponse
{
public:
    int m_version_major;
    int m_version_minor;
    HTTPStatusCode m_status;
    std::string m_reason;
    HTTPHeaders m_headers;
    std::vector<std::byte> m_body;
    bool m_keep_alive{false};

    std::string StringifyHeaders() const;
};

class HTTPClient;

class HTTPRequest
{
public:
    std::string m_method;
    std::string m_target;
    // Default protocol version is used by error responses to unreadable requests
    int m_version_major{1};
    int m_version_minor{1};
    HTTPHeaders m_headers;
    std::string m_body;

    // Keep a pointer to the client that made the request so
    // we know who to respond to.
    std::shared_ptr<HTTPClient> m_client;
    explicit HTTPRequest(std::shared_ptr<HTTPClient> client) : m_client(client) {};
    // Null client for unit tests
    explicit HTTPRequest() : m_client(nullptr) {};

    // Readers return false if they need more data from the
    // socket to parse properly. They throw errors if
    // the data is invalid.
    bool LoadControlData(LineReader& reader);
    bool LoadHeaders(LineReader& reader);
    bool LoadBody(LineReader& reader);

    // Response headers may be set in advance before response body is known
    HTTPHeaders m_response_headers;
    void WriteReply(HTTPStatusCode status, std::span<const std::byte> reply_body = {});
    void WriteReply(HTTPStatusCode status, const char* reply_body) {
        auto reply_body_view = std::string_view(reply_body);
        std::span<const std::byte> byte_span(reinterpret_cast<const std::byte*>(reply_body_view.data()), reply_body_view.size());
        WriteReply(status, byte_span);
    }
};

class HTTPServer;

class HTTPClient
{
public:
    // ID provided by SockMan, inherited by HTTPServer
    NodeId m_node_id;
    // Remote address of connected client
    CService m_addr;
    // IP:port of connected client, cached for logging purposes
    std::string m_origin;
    // Pointer back to the server so we can call Sockman I/O methods from the client
    // Ok to remain null for unit tests.
    HTTPServer* m_server;

    // In lieu of an intermediate transport class like p2p uses,
    // we copy data from the socket buffer to the client object
    // and attempt to read HTTP requests from here.
    std::vector<std::byte> m_recv_buffer{};

    // Response data destined for this client.
    // Written to directly by http worker threads, read and erased by Sockman I/O
    Mutex m_send_mutex;
    std::vector<std::byte> m_send_buffer GUARDED_BY(m_send_mutex);
    // Set true by worker threads after writing a response to m_send_buffer.
    // Set false by the Sockman I/O thread after flushing m_send_buffer.
    // Checked in the Sockman I/O loop to avoid locking m_send_mutex if there's nothing to send.
    std::atomic_bool m_send_ready{false};

    explicit HTTPClient(NodeId node_id, CService addr) : m_node_id(node_id), m_addr(addr)
    {
        m_origin = addr.ToStringAddrPort();
    };

    // Try to read an HTTP request from the receive buffer
    bool ReadRequest(std::unique_ptr<HTTPRequest>& req);

    // Push data from m_send_buffer to the connected socket via m_server
    // Returns false if we are done with this client and Sockman can
    // therefore skip the next read operation from it.
    bool SendBytesFromBuffer() EXCLUSIVE_LOCKS_REQUIRED(!m_send_mutex);

    // Disable copies (should only be used as shared pointers)
    HTTPClient(const HTTPClient&) = delete;
    HTTPClient& operator=(const HTTPClient&) = delete;
};

class HTTPServer : public SockMan
{
public:
    explicit HTTPServer(std::function<void(std::unique_ptr<HTTPRequest>)> func) : m_request_dispatcher(func) {};

    // Set in the Sockman I/O loop and only checked by main thread when shutting
    // down to wait for all clients to be disconnected.
    std::atomic_bool m_no_clients{true};
    //! Connected clients with live HTTP connections
    std::unordered_map<NodeId, std::shared_ptr<HTTPClient>> m_connected_clients;

    // What to do with HTTP requests once received, validated and parsed
    std::function<void(std::unique_ptr<HTTPRequest>)> m_request_dispatcher;

    std::shared_ptr<HTTPClient> GetClientById(NodeId node_id) const;

    /**
     * Be notified when a new connection has been accepted.
     * @param[in] node_id Id of the newly accepted connection.
     * @param[in] me The address and port at our side of the connection.
     * @param[in] them The address and port at the peer's side of the connection.
     * @retval true The new connection was accepted at the higher level.
     * @retval false The connection was refused at the higher level, so the
     * associated socket and node_id should be discarded by `SockMan`.
     */
    virtual bool EventNewConnectionAccepted(NodeId node_id, const CService& me, const CService& them) override;

    /**
     * Called when the socket is ready to send data and `ShouldTryToSend()` has
     * returned true. This is where the higher level code serializes its messages
     * and calls `SockMan::SendBytes()`.
     * @param[in] node_id Id of the node whose socket is ready to send.
     * @param[out] cancel_recv Should always be set upon return and if it is true,
     * then the next attempt to receive data from that node will be omitted.
     */
    virtual void EventReadyToSend(NodeId node_id, bool& cancel_recv) override;

    /**
     * Called when new data has been received.
     * @param[in] node_id Connection for which the data arrived.
     * @param[in] data Received data.
     */
    virtual void EventGotData(NodeId node_id, std::span<const uint8_t> data) override;

    /**
     * Called when the remote peer has sent an EOF on the socket. This is a graceful
     * close of their writing side, we can still send and they will receive, if it
     * makes sense at the application level.
     * @param[in] node_id Node whose socket got EOF.
     */
    virtual void EventGotEOF(NodeId node_id) override {};

    /**
     * Called when we get an irrecoverable error trying to read from a socket.
     * @param[in] node_id Node whose socket got an error.
     * @param[in] errmsg Message describing the error.
     */
    virtual void EventGotPermanentReadError(NodeId node_id, const std::string& errmsg) override {};

    /**
     * Can be used to temporarily pause sends on a connection.
     * SockMan would only call EventReadyToSend() if this returns true.
     * The implementation in SockMan always returns true.
     * @param[in] node_id Connection for which to confirm or omit the next call to EventReadyToSend().
     */
    virtual bool ShouldTryToSend(NodeId node_id) const override;

    /**
     * SockMan would only call Recv() on a connection's socket if this returns true.
     * Can be used to temporarily pause receives on a connection.
     * The implementation in SockMan always returns true.
     * @param[in] node_id Connection for which to confirm or omit the next receive.
     */
    virtual bool ShouldTryToRecv(NodeId node_id) const override;
};
} // namespace http_bitcoin

#endif // BITCOIN_HTTPSERVER_H
