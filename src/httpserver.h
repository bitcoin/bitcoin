// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_HTTPSERVER_H
#define BITCOIN_HTTPSERVER_H

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <netaddress.h>
#include <rpc/protocol.h>
#include <util/result.h>
#include <util/sock.h>
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

//! Shortest valid request line, used by libevent in evhttp_parse_request_line()
static const size_t MIN_REQUEST_LINE_LENGTH{strlen("GET / HTTP/1.0")};

//! Maximum size of http request (request line + headers)
//! see https://github.com/bitcoin/bitcoin/issues/6425
static const size_t MAX_HEADERS_SIZE{8192};

class HTTPHeaders
{
public:
    std::optional<std::string> Find(const std::string& key) const;
    void Write(const std::string& key, const std::string& value);
    void Remove(const std::string& key);
    /*
     * @returns false if LineReader hits the end of the buffer before reading an
     *                \n, meaning that we are still waiting on more data from the client.
     *          true  after reading an entire HTTP headers section, terminated
     *                by an empty line and \n.
     * @throws on exceeded read limit and on bad headers syntax (e.g. no ":" in a line)
     */
    bool Read(util::LineReader& reader);
    std::string Stringify() const;

private:
    std::unordered_map<std::string, std::string, util::AsciiCaseInsensitiveHash, util::AsciiCaseInsensitiveKeyEqual> m_map;
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

    /**
     * Default HTTP protocol version 1.1 is used by error responses
     * when a request is unreadable.
     */
    /// @{
    int m_version_major{1};
    int m_version_minor{1};
    /// @}

    HTTPHeaders m_headers;
    std::string m_body;

    //! Pointer to the client that made the request so we know who to respond to.
    std::shared_ptr<HTTPClient> m_client;

    explicit HTTPRequest(std::shared_ptr<HTTPClient> client) : m_client(client) {};
    //! Construct with a null client for unit tests
    explicit HTTPRequest() : m_client(nullptr) {};

    /**
     * Methods that attempt to parse HTTP request fields line-by-line
     * from a receive buffer.
     * @param[in]   reader  A LineReader object constructed over a span of data.
     * @returns     true    If the request field was parsed.
     *              false   If there was not enough data in the buffer to complete the field.
     * @throws      std::runtime_error if data is invalid.
     */
    /// @{
    bool LoadControlData(LineReader& reader);
    bool LoadHeaders(LineReader& reader);
    bool LoadBody(LineReader& reader);
    /// @}
};

class HTTPServer
{
public:
    /**
     * Each connection is assigned an unique id of this type.
     */
    using Id = int64_t;

    explicit HTTPServer(std::function<void(std::unique_ptr<HTTPRequest>&&)> func) : m_request_dispatcher(func) {};

    virtual ~HTTPServer()
    {
        Assume(!m_thread_socket_handler.joinable()); // Missing call to JoinSocketsThreads()
        Assume(m_connected.empty()); // Missing call to CloseConnection()
        Assume(m_listen.empty()); // Missing call to StopListening()
    }

    /**
     * Bind to a new address:port, start listening and add the listen socket to `m_listen`.
     * @param[in] to Where to bind.
     * @returns {} or the reason for failure.
     */
    util::Result<void> BindAndStartListening(const CService& to);

    /**
     * Stop listening by closing all listening sockets.
     */
    void StopListening();

    /**
     * Get the number of sockets the server is bound to and listening on
     */
    size_t GetListeningSocketCount() const { return m_listen.size(); }

    /**
     * Get the number of HTTPClients we are connected to
     */
    size_t GetConnectionsCount() const { return m_connected_size.load(std::memory_order_acquire); }

    /**
     * Start the necessary threads for sockets IO.
     */
    void StartSocketsThreads();

    /**
     * Join (wait for) the threads started by `StartSocketsThreads()` to exit.
     */
    void JoinSocketsThreads();

    /**
     * Stop network activity
     */
    void InterruptNet() { m_interrupt_net(); }

    /**
     * Destroy a given connection by closing its socket and release resources occupied by it.
     * This is a temporary method that will be removed in a future commit when it is
     * replaced by DisconnectClients() and run automatically in the I/O thread.
     * @param[in] http_client Connection to destroy.
     * @return Whether the connection existed and its socket was closed by this call.
     */
    bool CloseConnection(std::shared_ptr<HTTPClient> http_client);

private:
    /**
     * List of listening sockets.
     */
    std::vector<std::shared_ptr<Sock>> m_listen;

    /**
     * The id to assign to the next created connection.
     */
    std::atomic<Id> m_next_id{0};

    /**
     * List of HTTPClients with connected sockets.
     * Connections will only be added and removed in the I/O thread, but
     * shared pointers may be passed to worker threads to handle requests
     * and send replies.
     */
    std::vector<std::shared_ptr<HTTPClient>> m_connected;

    /**
     * The number of connected sockets.
     * Updated from the I/O thread but safely readable from
     * the main thread without locks.
     */
    std::atomic<size_t> m_connected_size{0};

    /**
     * Info about which socket has which event ready and a reverse map
     * back to the HTTPClient that owns the socket.
     */
    struct IOReadiness {
        /**
         * Map of socket -> socket events. For example:
         * socket1 -> { requested = SEND|RECV, occurred = RECV }
         * socket2 -> { requested = SEND, occurred = SEND }
         */
        Sock::EventsPerSock events_per_sock;

        /**
         * Map of socket -> HTTPClient. For example:
         * socket1 -> HTTPClient{ id=23 }
         * socket2 -> HTTPClient{ id=56 }
         */
        std::unordered_map<Sock::EventsPerSock::key_type,
                           std::shared_ptr<HTTPClient>,
                           Sock::HashSharedPtrSock,
                           Sock::EqualSharedPtrSock>
            httpclients_per_sock;
    };

    /**
     * This is signaled when network activity should cease.
     */
    CThreadInterrupt m_interrupt_net;

    /**
     * Thread that sends to and receives from sockets and accepts connections.
     * Executes the I/O loop of the server.
     */
    std::thread m_thread_socket_handler;

    /*
     * What to do with HTTP requests once received, validated and parsed
     */
    std::function<void(std::unique_ptr<HTTPRequest>&&)> m_request_dispatcher;

    /**
     * Accept a connection.
     * @param[in] listen_sock Socket on which to accept the connection.
     * @param[out] addr Address of the peer that was accepted.
     * @return Newly created socket for the accepted connection.
     */
    std::unique_ptr<Sock> AcceptConnection(const Sock& listen_sock, CService& addr);

    /**
     * Generate an id for a newly created connection.
     */
    Id GetNewId();

    /**
     * After a new socket with a client has been created, configure its flags,
     * make a new HTTPClient and Id and save its shared pointer.
     * @param[in] sock The newly created socket.
     * @param[in] them Address of the new peer.
     */
    void NewSockAccepted(std::unique_ptr<Sock>&& sock, const CService& them);

    /**
     * Do the read/write for connected sockets that are ready for IO.
     * @param[in] io_readiness Which sockets are ready and their corresponding HTTPClients.
     */
    void SocketHandlerConnected(const IOReadiness& io_readiness) const;

    /**
     * Accept incoming connections, one from each read-ready listening socket.
     * @param[in] events_per_sock Sockets that are ready for IO.
     */
    void SocketHandlerListening(const Sock::EventsPerSock& events_per_sock);

    /**
     * Generate a collection of sockets to check for IO readiness.
     * @return Sockets to check for readiness plus an aux map to find the
     * corresponding HTTPClient given a socket.
     */
    IOReadiness GenerateWaitSockets() const;

    /**
     * Check connected and listening sockets for IO readiness and process them accordingly.
     * This is the main I/O loop of the server.
     */
    void ThreadSocketHandler();

    /**
     * Try to read HTTPRequests from a client's receive buffer.
     * Complete requests are dispatched, incomplete requests are
     * left in the buffer to wait for more data. Some read errors
     * will mark this client for disconnection.
     * @param[in] client The HTTPClient to read requests from
     */
    void MaybeDispatchRequestsFromClient(std::shared_ptr<HTTPClient> client) const;
};

class HTTPClient
{
public:
    //! ID provided by HTTPServer upon connection and instantiation
    HTTPServer::Id m_id;

    //! Remote address of connected client
    CService m_addr;

    //! IP:port of connected client, cached for logging purposes
    std::string m_origin;

    /**
     * In lieu of an intermediate transport class like p2p uses,
     * we copy data from the socket buffer to the client object
     * and attempt to read HTTP requests from here.
     */
    std::vector<std::byte> m_recv_buffer{};

    /**
     * Mutex that serializes the Send() and Recv() calls on `m_sock`. Reading
     * from the client occurs in the I/O thread but writing back to a client
     * may occur in a worker thread.
     */
    Mutex m_sock_mutex;

    /**
     * Underlying socket.
     * `shared_ptr` (instead of `unique_ptr`) is used to avoid premature close of the
     * underlying file descriptor by one thread while another thread is poll(2)-ing
     * it for activity.
     * @see https://github.com/bitcoin/bitcoin/issues/21744 for details.
     */
    std::shared_ptr<Sock> m_sock GUARDED_BY(m_sock_mutex);

    explicit HTTPClient(HTTPServer::Id id, const CService& addr, std::unique_ptr<Sock> socket)
        : m_id(id), m_addr(addr), m_origin(addr.ToStringAddrPort()), m_sock{std::move(socket)} {};

    // Disable copies (should only be used as shared pointers)
    HTTPClient(const HTTPClient&) = delete;
    HTTPClient& operator=(const HTTPClient&) = delete;

    /**
     * Try to read an HTTP request from the receive buffer.
     * @param[in]   req     A pointer to an HTTPRequest to read
     * @returns true on success, false on failure.
     */
    bool ReadRequest(const std::unique_ptr<HTTPRequest>& req);
};
} // namespace http_bitcoin

#endif // BITCOIN_HTTPSERVER_H
