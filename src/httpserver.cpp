// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpserver.h>

#include <chainparamsbase.h>
#include <common/args.h>
#include <common/messages.h>
#include <compat/compat.h>
#include <logging.h>
#include <netbase.h>
#include <node/interface_ui.h>
#include <rpc/protocol.h> // For HTTP status codes
#include <span.h>
#include <sync.h>
#include <util/check.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>
#include <util/threadnames.h>
#include <util/translation.h>

#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>

#include <sys/types.h>
#include <sys/stat.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/keyvalq_struct.h>
#include <event2/thread.h>
#include <event2/util.h>

#include <support/events.h>

using common::InvalidPortErrMsg;
using http_libevent::HTTPRequest;

/** Maximum size of http request (request line + headers) */
static const size_t MAX_HEADERS_SIZE = 8192;

/** HTTP request work item */
class HTTPWorkItem final : public HTTPClosure
{
public:
    HTTPWorkItem(std::unique_ptr<HTTPRequest> _req, const std::string &_path, const HTTPRequestHandler& _func):
        req(std::move(_req)), path(_path), func(_func)
    {
    }
    void operator()() override
    {
        func(req.get(), path);
    }

    std::unique_ptr<HTTPRequest> req;

private:
    std::string path;
    HTTPRequestHandler func;
};

/** Simple work queue for distributing work over multiple threads.
 * Work items are simply callable objects.
 */
template <typename WorkItem>
class WorkQueue
{
private:
    Mutex cs;
    std::condition_variable cond GUARDED_BY(cs);
    std::deque<std::unique_ptr<WorkItem>> queue GUARDED_BY(cs);
    bool running GUARDED_BY(cs){true};
    const size_t maxDepth;

public:
    explicit WorkQueue(size_t _maxDepth) : maxDepth(_maxDepth)
    {
    }
    /** Precondition: worker threads have all stopped (they have been joined).
     */
    ~WorkQueue() = default;
    /** Enqueue a work item */
    bool Enqueue(WorkItem* item) EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        if (!running || queue.size() >= maxDepth) {
            return false;
        }
        queue.emplace_back(std::unique_ptr<WorkItem>(item));
        cond.notify_one();
        return true;
    }
    /** Thread function */
    void Run() EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        while (true) {
            std::unique_ptr<WorkItem> i;
            {
                WAIT_LOCK(cs, lock);
                while (running && queue.empty())
                    cond.wait(lock);
                if (!running && queue.empty())
                    break;
                i = std::move(queue.front());
                queue.pop_front();
            }
            (*i)();
        }
    }
    /** Interrupt and exit loops */
    void Interrupt() EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        running = false;
        cond.notify_all();
    }
};

struct HTTPPathHandler
{
    HTTPPathHandler(std::string _prefix, bool _exactMatch, HTTPRequestHandler _handler):
        prefix(_prefix), exactMatch(_exactMatch), handler(_handler)
    {
    }
    std::string prefix;
    bool exactMatch;
    HTTPRequestHandler handler;
};

/** HTTP module state */

//! libevent event loop
static struct event_base* eventBase = nullptr;
//! HTTP server
static struct evhttp* eventHTTP = nullptr;
//! List of subnets to allow RPC connections from
static std::vector<CSubNet> rpc_allow_subnets;
//! Work queue for handling longer requests off the event loop thread
static std::unique_ptr<WorkQueue<HTTPClosure>> g_work_queue{nullptr};
//! Handlers for (sub)paths
static GlobalMutex g_httppathhandlers_mutex;
static std::vector<HTTPPathHandler> pathHandlers GUARDED_BY(g_httppathhandlers_mutex);
//! Bound listening sockets
static std::vector<evhttp_bound_socket *> boundSockets;

/**
 * @brief Helps keep track of open `evhttp_connection`s with active `evhttp_requests`
 *
 */
class HTTPRequestTracker
{
private:
    mutable Mutex m_mutex;
    mutable std::condition_variable m_cv;
    //! For each connection, keep a counter of how many requests are open
    std::unordered_map<const evhttp_connection*, size_t> m_tracker GUARDED_BY(m_mutex);

    void RemoveConnectionInternal(const decltype(m_tracker)::iterator it) EXCLUSIVE_LOCKS_REQUIRED(m_mutex)
    {
        m_tracker.erase(it);
        if (m_tracker.empty()) m_cv.notify_all();
    }
public:
    //! Increase request counter for the associated connection by 1
    void AddRequest(evhttp_request* req) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        const evhttp_connection* conn{Assert(evhttp_request_get_connection(Assert(req)))};
        WITH_LOCK(m_mutex, ++m_tracker[conn]);
    }
    //! Decrease request counter for the associated connection by 1, remove connection if counter is 0
    void RemoveRequest(evhttp_request* req) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        const evhttp_connection* conn{Assert(evhttp_request_get_connection(Assert(req)))};
        LOCK(m_mutex);
        auto it{m_tracker.find(conn)};
        if (it != m_tracker.end() && it->second > 0) {
            if (--(it->second) == 0) RemoveConnectionInternal(it);
        }
    }
    //! Remove a connection entirely
    void RemoveConnection(const evhttp_connection* conn) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        auto it{m_tracker.find(Assert(conn))};
        if (it != m_tracker.end()) RemoveConnectionInternal(it);
    }
    size_t CountActiveConnections() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        return WITH_LOCK(m_mutex, return m_tracker.size());
    }
    //! Wait until there are no more connections with active requests in the tracker
    void WaitUntilEmpty() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        WAIT_LOCK(m_mutex, lock);
        m_cv.wait(lock, [this]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_tracker.empty(); });
    }
};
//! Track active requests
static HTTPRequestTracker g_requests;

/** Check if a network address is allowed to access the HTTP server */
static bool ClientAllowed(const CNetAddr& netaddr)
{
    if (!netaddr.IsValid())
        return false;
    for(const CSubNet& subnet : rpc_allow_subnets)
        if (subnet.Match(netaddr))
            return true;
    return false;
}

/** Initialize ACL list for HTTP server */
static bool InitHTTPAllowList()
{
    rpc_allow_subnets.clear();
    rpc_allow_subnets.emplace_back(LookupHost("127.0.0.1", false).value(), 8);  // always allow IPv4 local subnet
    rpc_allow_subnets.emplace_back(LookupHost("::1", false).value());  // always allow IPv6 localhost
    for (const std::string& strAllow : gArgs.GetArgs("-rpcallowip")) {
        const CSubNet subnet{LookupSubNet(strAllow)};
        if (!subnet.IsValid()) {
            uiInterface.ThreadSafeMessageBox(
                Untranslated(strprintf("Invalid -rpcallowip subnet specification: %s. Valid are a single IP (e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0) or a network/CIDR (e.g. 1.2.3.4/24).", strAllow)),
                "", CClientUIInterface::MSG_ERROR);
            return false;
        }
        rpc_allow_subnets.push_back(subnet);
    }
    std::string strAllowed;
    for (const CSubNet& subnet : rpc_allow_subnets)
        strAllowed += subnet.ToString() + " ";
    LogDebug(BCLog::HTTP, "Allowing HTTP connections from: %s\n", strAllowed);
    return true;
}

/** HTTP request method as string - use for logging only */
std::string RequestMethodString(HTTPRequest::RequestMethod m)
{
    switch (m) {
    case HTTPRequest::GET:
        return "GET";
    case HTTPRequest::POST:
        return "POST";
    case HTTPRequest::HEAD:
        return "HEAD";
    case HTTPRequest::PUT:
        return "PUT";
    case HTTPRequest::UNKNOWN:
        return "unknown";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

/** HTTP request callback */
static void http_request_cb(struct evhttp_request* req, void* arg)
{
    evhttp_connection* conn{evhttp_request_get_connection(req)};
    // Track active requests
    {
        g_requests.AddRequest(req);
        evhttp_request_set_on_complete_cb(req, [](struct evhttp_request* req, void*) {
            g_requests.RemoveRequest(req);
        }, nullptr);
        evhttp_connection_set_closecb(conn, [](evhttp_connection* conn, void* arg) {
            g_requests.RemoveConnection(conn);
        }, nullptr);
    }

    // Disable reading to work around a libevent bug, fixed in 2.1.9
    // See https://github.com/libevent/libevent/commit/5ff8eb26371c4dc56f384b2de35bea2d87814779
    // and https://github.com/bitcoin/bitcoin/pull/11593.
    if (event_get_version_number() >= 0x02010600 && event_get_version_number() < 0x02010900) {
        if (conn) {
            bufferevent* bev = evhttp_connection_get_bufferevent(conn);
            if (bev) {
                bufferevent_disable(bev, EV_READ);
            }
        }
    }
    auto hreq{std::make_unique<HTTPRequest>(req, *static_cast<const util::SignalInterrupt*>(arg))};

    // Early address-based allow check
    if (!ClientAllowed(hreq->GetPeer())) {
        LogDebug(BCLog::HTTP, "HTTP request from %s rejected: Client network is not allowed RPC access\n",
                 hreq->GetPeer().ToStringAddrPort());
        hreq->WriteReply(HTTP_FORBIDDEN);
        return;
    }

    // Early reject unknown HTTP methods
    if (hreq->GetRequestMethod() == HTTPRequest::UNKNOWN) {
        LogDebug(BCLog::HTTP, "HTTP request from %s rejected: Unknown HTTP request method\n",
                 hreq->GetPeer().ToStringAddrPort());
        hreq->WriteReply(HTTP_BAD_METHOD);
        return;
    }

    LogDebug(BCLog::HTTP, "Received a %s request for %s from %s\n",
             RequestMethodString(hreq->GetRequestMethod()), SanitizeString(hreq->GetURI(), SAFE_CHARS_URI).substr(0, 100), hreq->GetPeer().ToStringAddrPort());

    // Find registered handler for prefix
    std::string strURI = hreq->GetURI();
    std::string path;
    LOCK(g_httppathhandlers_mutex);
    std::vector<HTTPPathHandler>::const_iterator i = pathHandlers.begin();
    std::vector<HTTPPathHandler>::const_iterator iend = pathHandlers.end();
    for (; i != iend; ++i) {
        bool match = false;
        if (i->exactMatch)
            match = (strURI == i->prefix);
        else
            match = strURI.starts_with(i->prefix);
        if (match) {
            path = strURI.substr(i->prefix.size());
            break;
        }
    }

    // Dispatch to worker thread
    if (i != iend) {
        std::unique_ptr<HTTPWorkItem> item(new HTTPWorkItem(std::move(hreq), path, i->handler));
        assert(g_work_queue);
        if (g_work_queue->Enqueue(item.get())) {
            item.release(); /* if true, queue took ownership */
        } else {
            LogPrintf("WARNING: request rejected because http work queue depth exceeded, it can be increased with the -rpcworkqueue= setting\n");
            item->req->WriteReply(HTTP_SERVICE_UNAVAILABLE, "Work queue depth exceeded");
        }
    } else {
        hreq->WriteReply(HTTP_NOT_FOUND);
    }
}

/** Callback to reject HTTP requests after shutdown. */
static void http_reject_request_cb(struct evhttp_request* req, void*)
{
    LogDebug(BCLog::HTTP, "Rejecting request while shutting down\n");
    evhttp_send_error(req, HTTP_SERVUNAVAIL, nullptr);
}

/** Event dispatcher thread */
static void ThreadHTTP(struct event_base* base)
{
    util::ThreadRename("http");
    LogDebug(BCLog::HTTP, "Entering http event loop\n");
    event_base_dispatch(base);
    // Event loop will be interrupted by InterruptHTTPServer()
    LogDebug(BCLog::HTTP, "Exited http event loop\n");
}

/** Bind HTTP server to specified addresses */
static bool HTTPBindAddresses(struct evhttp* http)
{
    uint16_t http_port{static_cast<uint16_t>(gArgs.GetIntArg("-rpcport", BaseParams().RPCPort()))};
    std::vector<std::pair<std::string, uint16_t>> endpoints;

    // Determine what addresses to bind to
    // To prevent misconfiguration and accidental exposure of the RPC
    // interface, require -rpcallowip and -rpcbind to both be specified
    // together. If either is missing, ignore both values, bind to localhost
    // instead, and log warnings.
    if (gArgs.GetArgs("-rpcallowip").empty() || gArgs.GetArgs("-rpcbind").empty()) { // Default to loopback if not allowing external IPs
        endpoints.emplace_back("::1", http_port);
        endpoints.emplace_back("127.0.0.1", http_port);
        if (!gArgs.GetArgs("-rpcallowip").empty()) {
            LogPrintf("WARNING: option -rpcallowip was specified without -rpcbind; this doesn't usually make sense\n");
        }
        if (!gArgs.GetArgs("-rpcbind").empty()) {
            LogPrintf("WARNING: option -rpcbind was ignored because -rpcallowip was not specified, refusing to allow everyone to connect\n");
        }
    } else { // Specific bind addresses
        for (const std::string& strRPCBind : gArgs.GetArgs("-rpcbind")) {
            uint16_t port{http_port};
            std::string host;
            if (!SplitHostPort(strRPCBind, port, host)) {
                LogError("%s\n", InvalidPortErrMsg("-rpcbind", strRPCBind).original);
                return false;
            }
            endpoints.emplace_back(host, port);
        }
    }

    // Bind addresses
    for (std::vector<std::pair<std::string, uint16_t> >::iterator i = endpoints.begin(); i != endpoints.end(); ++i) {
        LogPrintf("Binding RPC on address %s port %i\n", i->first, i->second);
        evhttp_bound_socket *bind_handle = evhttp_bind_socket_with_handle(http, i->first.empty() ? nullptr : i->first.c_str(), i->second);
        if (bind_handle) {
            const std::optional<CNetAddr> addr{LookupHost(i->first, false)};
            if (i->first.empty() || (addr.has_value() && addr->IsBindAny())) {
                LogPrintf("WARNING: the RPC server is not safe to expose to untrusted networks such as the public internet\n");
            }
            // Set the no-delay option (disable Nagle's algorithm) on the TCP socket.
            evutil_socket_t fd = evhttp_bound_socket_get_fd(bind_handle);
            int one = 1;
            if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (sockopt_arg_type)&one, sizeof(one)) == SOCKET_ERROR) {
                LogInfo("WARNING: Unable to set TCP_NODELAY on RPC server socket, continuing anyway\n");
            }
            boundSockets.push_back(bind_handle);
        } else {
            LogPrintf("Binding RPC on address %s port %i failed.\n", i->first, i->second);
        }
    }
    return !boundSockets.empty();
}

/** Simple wrapper to set thread name and run work queue */
static void HTTPWorkQueueRun(WorkQueue<HTTPClosure>* queue, int worker_num)
{
    util::ThreadRename(strprintf("httpworker.%i", worker_num));
    queue->Run();
}

/** libevent event log callback */
static void libevent_log_cb(int severity, const char *msg)
{
    BCLog::Level level;
    switch (severity) {
    case EVENT_LOG_DEBUG:
        level = BCLog::Level::Debug;
        break;
    case EVENT_LOG_MSG:
        level = BCLog::Level::Info;
        break;
    case EVENT_LOG_WARN:
        level = BCLog::Level::Warning;
        break;
    default: // EVENT_LOG_ERR and others are mapped to error
        level = BCLog::Level::Error;
        break;
    }
    LogPrintLevel(BCLog::LIBEVENT, level, "%s\n", msg);
}

namespace http_libevent {
bool InitHTTPServer(const util::SignalInterrupt& interrupt)
{
    if (!InitHTTPAllowList())
        return false;

    // Redirect libevent's logging to our own log
    event_set_log_callback(&libevent_log_cb);
    // Update libevent's log handling.
    UpdateHTTPServerLogging(LogInstance().WillLogCategory(BCLog::LIBEVENT));

#ifdef WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif

    raii_event_base base_ctr = obtain_event_base();

    /* Create a new evhttp object to handle requests. */
    raii_evhttp http_ctr = obtain_evhttp(base_ctr.get());
    struct evhttp* http = http_ctr.get();
    if (!http) {
        LogPrintf("couldn't create evhttp. Exiting.\n");
        return false;
    }

    evhttp_set_timeout(http, gArgs.GetIntArg("-rpcservertimeout", DEFAULT_HTTP_SERVER_TIMEOUT));
    evhttp_set_max_headers_size(http, MAX_HEADERS_SIZE);
    evhttp_set_max_body_size(http, MAX_SIZE);
    evhttp_set_gencb(http, http_request_cb, (void*)&interrupt);

    if (!HTTPBindAddresses(http)) {
        LogPrintf("Unable to bind any endpoint for RPC server\n");
        return false;
    }

    LogDebug(BCLog::HTTP, "Initialized HTTP server\n");
    int workQueueDepth = std::max((long)gArgs.GetIntArg("-rpcworkqueue", DEFAULT_HTTP_WORKQUEUE), 1L);
    LogDebug(BCLog::HTTP, "creating work queue of depth %d\n", workQueueDepth);

    g_work_queue = std::make_unique<WorkQueue<HTTPClosure>>(workQueueDepth);
    // transfer ownership to eventBase/HTTP via .release()
    eventBase = base_ctr.release();
    eventHTTP = http_ctr.release();
    return true;
}

void UpdateHTTPServerLogging(bool enable) {
    if (enable) {
        event_enable_debug_logging(EVENT_DBG_ALL);
    } else {
        event_enable_debug_logging(EVENT_DBG_NONE);
    }
}

static std::thread g_thread_http;
static std::vector<std::thread> g_thread_http_workers;

void StartHTTPServer()
{
    int rpcThreads = std::max((long)gArgs.GetIntArg("-rpcthreads", DEFAULT_HTTP_THREADS), 1L);
    LogInfo("Starting HTTP server with %d worker threads\n", rpcThreads);
    g_thread_http = std::thread(ThreadHTTP, eventBase);

    for (int i = 0; i < rpcThreads; i++) {
        g_thread_http_workers.emplace_back(HTTPWorkQueueRun, g_work_queue.get(), i);
    }
}

void InterruptHTTPServer()
{
    LogDebug(BCLog::HTTP, "Interrupting HTTP server\n");
    if (eventHTTP) {
        // Reject requests on current connections
        evhttp_set_gencb(eventHTTP, http_reject_request_cb, nullptr);
    }
    if (g_work_queue) {
        g_work_queue->Interrupt();
    }
}

void StopHTTPServer()
{
    LogDebug(BCLog::HTTP, "Stopping HTTP server\n");
    if (g_work_queue) {
        LogDebug(BCLog::HTTP, "Waiting for HTTP worker threads to exit\n");
        for (auto& thread : g_thread_http_workers) {
            thread.join();
        }
        g_thread_http_workers.clear();
    }
    // Unlisten sockets, these are what make the event loop running, which means
    // that after this and all connections are closed the event loop will quit.
    for (evhttp_bound_socket *socket : boundSockets) {
        evhttp_del_accept_socket(eventHTTP, socket);
    }
    boundSockets.clear();
    {
        if (const auto n_connections{g_requests.CountActiveConnections()}; n_connections != 0) {
            LogDebug(BCLog::HTTP, "Waiting for %d connections to stop HTTP server\n", n_connections);
        }
        g_requests.WaitUntilEmpty();
    }
    if (eventHTTP) {
        // Schedule a callback to call evhttp_free in the event base thread, so
        // that evhttp_free does not need to be called again after the handling
        // of unfinished request connections that follows.
        event_base_once(eventBase, -1, EV_TIMEOUT, [](evutil_socket_t, short, void*) {
            evhttp_free(eventHTTP);
            eventHTTP = nullptr;
        }, nullptr, nullptr);
    }
    if (eventBase) {
        LogDebug(BCLog::HTTP, "Waiting for HTTP event thread to exit\n");
        if (g_thread_http.joinable()) g_thread_http.join();
        event_base_free(eventBase);
        eventBase = nullptr;
    }
    g_work_queue.reset();
    LogDebug(BCLog::HTTP, "Stopped HTTP server\n");
}
} // namespace http_libevent

struct event_base* EventBase()
{
    return eventBase;
}

static void httpevent_callback_fn(evutil_socket_t, short, void* data)
{
    // Static handler: simply call inner handler
    HTTPEvent *self = static_cast<HTTPEvent*>(data);
    self->handler();
    if (self->deleteWhenTriggered)
        delete self;
}

HTTPEvent::HTTPEvent(struct event_base* base, bool _deleteWhenTriggered, const std::function<void()>& _handler):
    deleteWhenTriggered(_deleteWhenTriggered), handler(_handler)
{
    ev = event_new(base, -1, 0, httpevent_callback_fn, this);
    assert(ev);
}
HTTPEvent::~HTTPEvent()
{
    event_free(ev);
}
void HTTPEvent::trigger(struct timeval* tv)
{
    if (tv == nullptr)
        event_active(ev, 0, 0); // immediately trigger event in main thread
    else
        evtimer_add(ev, tv); // trigger after timeval passed
}

namespace http_libevent {
HTTPRequest::HTTPRequest(struct evhttp_request* _req, const util::SignalInterrupt& interrupt, bool _replySent)
    : req(_req), m_interrupt(interrupt), replySent(_replySent)
{
}

HTTPRequest::~HTTPRequest()
{
    if (!replySent) {
        // Keep track of whether reply was sent to avoid request leaks
        LogPrintf("%s: Unhandled request\n", __func__);
        WriteReply(HTTP_INTERNAL_SERVER_ERROR, "Unhandled request");
    }
    // evhttpd cleans up the request, as long as a reply was sent.
}

std::pair<bool, std::string> HTTPRequest::GetHeader(const std::string& hdr) const
{
    const struct evkeyvalq* headers = evhttp_request_get_input_headers(req);
    assert(headers);
    const char* val = evhttp_find_header(headers, hdr.c_str());
    if (val)
        return std::make_pair(true, val);
    else
        return std::make_pair(false, "");
}

std::string HTTPRequest::ReadBody()
{
    struct evbuffer* buf = evhttp_request_get_input_buffer(req);
    if (!buf)
        return "";
    size_t size = evbuffer_get_length(buf);
    /** Trivial implementation: if this is ever a performance bottleneck,
     * internal copying can be avoided in multi-segment buffers by using
     * evbuffer_peek and an awkward loop. Though in that case, it'd be even
     * better to not copy into an intermediate string but use a stream
     * abstraction to consume the evbuffer on the fly in the parsing algorithm.
     */
    const char* data = (const char*)evbuffer_pullup(buf, size);
    if (!data) // returns nullptr in case of empty buffer
        return "";
    std::string rv(data, size);
    evbuffer_drain(buf, size);
    return rv;
}

void HTTPRequest::WriteHeader(const std::string& hdr, const std::string& value)
{
    struct evkeyvalq* headers = evhttp_request_get_output_headers(req);
    assert(headers);
    evhttp_add_header(headers, hdr.c_str(), value.c_str());
}

/** Closure sent to main thread to request a reply to be sent to
 * a HTTP request.
 * Replies must be sent in the main loop in the main http thread,
 * this cannot be done from worker threads.
 */
void HTTPRequest::WriteReply(int nStatus, std::span<const std::byte> reply)
{
    assert(!replySent && req);
    if (m_interrupt) {
        WriteHeader("Connection", "close");
    }
    // Send event to main http thread to send reply message
    struct evbuffer* evb = evhttp_request_get_output_buffer(req);
    assert(evb);
    evbuffer_add(evb, reply.data(), reply.size());
    auto req_copy = req;
    HTTPEvent* ev = new HTTPEvent(eventBase, true, [req_copy, nStatus]{
        evhttp_send_reply(req_copy, nStatus, nullptr, nullptr);
        // Re-enable reading from the socket. This is the second part of the libevent
        // workaround above.
        if (event_get_version_number() >= 0x02010600 && event_get_version_number() < 0x02010900) {
            evhttp_connection* conn = evhttp_request_get_connection(req_copy);
            if (conn) {
                bufferevent* bev = evhttp_connection_get_bufferevent(conn);
                if (bev) {
                    bufferevent_enable(bev, EV_READ | EV_WRITE);
                }
            }
        }
    });
    ev->trigger(nullptr);
    replySent = true;
    req = nullptr; // transferred back to main thread
}

CService HTTPRequest::GetPeer() const
{
    evhttp_connection* con = evhttp_request_get_connection(req);
    CService peer;
    if (con) {
        // evhttp retains ownership over returned address string
        const char* address = "";
        uint16_t port = 0;

#ifdef HAVE_EVHTTP_CONNECTION_GET_PEER_CONST_CHAR
        evhttp_connection_get_peer(con, &address, &port);
#else
        evhttp_connection_get_peer(con, (char**)&address, &port);
#endif // HAVE_EVHTTP_CONNECTION_GET_PEER_CONST_CHAR

        peer = MaybeFlipIPv6toCJDNS(LookupNumeric(address, port));
    }
    return peer;
}

std::string HTTPRequest::GetURI() const
{
    return evhttp_request_get_uri(req);
}

HTTPRequest::RequestMethod HTTPRequest::GetRequestMethod() const
{
    switch (evhttp_request_get_command(req)) {
    case EVHTTP_REQ_GET:
        return GET;
    case EVHTTP_REQ_POST:
        return POST;
    case EVHTTP_REQ_HEAD:
        return HEAD;
    case EVHTTP_REQ_PUT:
        return PUT;
    default:
        return UNKNOWN;
    }
}

std::optional<std::string> HTTPRequest::GetQueryParameter(const std::string& key) const
{
    const char* uri{evhttp_request_get_uri(req)};

    return GetQueryParameterFromUri(uri, key);
}

std::optional<std::string> GetQueryParameterFromUri(const char* uri, const std::string& key)
{
    evhttp_uri* uri_parsed{evhttp_uri_parse(uri)};
    if (!uri_parsed) {
        throw std::runtime_error("URI parsing failed, it likely contained RFC 3986 invalid characters");
    }
    const char* query{evhttp_uri_get_query(uri_parsed)};
    std::optional<std::string> result;

    if (query) {
        // Parse the query string into a key-value queue and iterate over it
        struct evkeyvalq params_q;
        evhttp_parse_query_str(query, &params_q);

        for (struct evkeyval* param{params_q.tqh_first}; param != nullptr; param = param->next.tqe_next) {
            if (param->key == key) {
                result = param->value;
                break;
            }
        }
        evhttp_clear_headers(&params_q);
    }
    evhttp_uri_free(uri_parsed);

    return result;
}
} // namespace http_libevent

void RegisterHTTPHandler(const std::string &prefix, bool exactMatch, const HTTPRequestHandler &handler)
{
    LogDebug(BCLog::HTTP, "Registering HTTP handler for %s (exactmatch %d)\n", prefix, exactMatch);
    LOCK(g_httppathhandlers_mutex);
    pathHandlers.emplace_back(prefix, exactMatch, handler);
}

void UnregisterHTTPHandler(const std::string &prefix, bool exactMatch)
{
    LOCK(g_httppathhandlers_mutex);
    std::vector<HTTPPathHandler>::iterator i = pathHandlers.begin();
    std::vector<HTTPPathHandler>::iterator iend = pathHandlers.end();
    for (; i != iend; ++i)
        if (i->prefix == prefix && i->exactMatch == exactMatch)
            break;
    if (i != iend)
    {
        LogDebug(BCLog::HTTP, "Unregistering HTTP handler for %s (exactmatch %d)\n", prefix, exactMatch);
        pathHandlers.erase(i);
    }
}


namespace http_bitcoin {
using util::SplitString;

std::optional<std::string> HTTPHeaders::Find(const std::string key) const
{
    const auto it = m_map.find(key);
    if (it == m_map.end()) return std::nullopt;
    return it->second;
}

void HTTPHeaders::Write(const std::string key, const std::string value)
{
    // If present, append value to list
    const auto existing_value = Find(key);
    if (existing_value) {
        m_map[key] = existing_value.value() + ", " + value;
    } else {
        m_map[key] = value;
    }
}

void HTTPHeaders::Remove(const std::string key)
{
    m_map.erase(key);
}

bool HTTPHeaders::Read(util::LineReader& reader)
{
    // Headers https://httpwg.org/specs/rfc9110.html#rfc.section.6.3
    // A sequence of Field Lines https://httpwg.org/specs/rfc9110.html#rfc.section.5.2
    do {
        auto maybe_line = reader.ReadLine();
        if (!maybe_line) return false;
        const std::string& line = *maybe_line;

        // An empty line indicates end of the headers section https://www.rfc-editor.org/rfc/rfc2616#section-4
        if (line.length() == 0) break;

        // Header line must have at least one ":"
        // keys are not allowed to have delimiters like ":" but values are
        // https://httpwg.org/specs/rfc9110.html#rfc.section.5.6.2
        const size_t pos{line.find(':')};
        if (pos == std::string::npos) throw std::runtime_error("HTTP header missing colon (:)");

        // Whitespace is optional
        std::string key = util::TrimString(line.substr(0, pos));
        std::string value = util::TrimString(line.substr(pos + 1));
        Write(key, value);
    } while (true);

    return true;
}

std::string HTTPHeaders::Stringify() const
{
    std::string out;
    for (auto it = m_map.begin(); it != m_map.end(); ++it) {
        out += it->first + ": " + it->second + "\r\n";
    }

    // Headers are terminated by an empty line
    out += "\r\n";

    return out;
}

std::string HTTPResponse::StringifyHeaders() const
{
    return strprintf("HTTP/%d.%d %d %s\r\n%s", m_version_major, m_version_minor, m_status, m_reason, m_headers.Stringify());
}

bool HTTPRequest::LoadControlData(LineReader& reader)
{
    auto maybe_line = reader.ReadLine();
    if (!maybe_line) return false;
    const std::string& request_line = *maybe_line;

    // Request Line aka Control Data https://httpwg.org/specs/rfc9110.html#rfc.section.6.2
    // Three words separated by spaces, terminated by \n or \r\n
    if (request_line.length() < MIN_REQUEST_LINE_LENGTH) throw std::runtime_error("HTTP request line too short");

    const std::vector<std::string> parts{SplitString(request_line, " ")};
    if (parts.size() != 3) throw std::runtime_error("HTTP request line malformed");
    m_method = parts[0];
    m_target = parts[1];

    if (parts[2].rfind("HTTP/") != 0) throw std::runtime_error("HTTP request line malformed");
    const std::vector<std::string> version_parts{SplitString(parts[2].substr(5), ".")};
    if (version_parts.size() != 2) throw std::runtime_error("HTTP request line malformed");
    auto major = ToIntegral<int>(version_parts[0]);
    auto minor = ToIntegral<int>(version_parts[1]);
    if (!major || !minor) throw std::runtime_error("HTTP request line malformed");
    m_version_major = major.value();
    m_version_minor = minor.value();

    return true;
}

bool HTTPRequest::LoadHeaders(LineReader& reader)
{
    return m_headers.Read(reader);
}

bool HTTPRequest::LoadBody(LineReader& reader)
{
    // https://httpwg.org/specs/rfc9112.html#message.body

    auto transfer_encoding_header = m_headers.Find("Transfer-Encoding");
    if (transfer_encoding_header && ToLower(transfer_encoding_header.value()) == "chunked") {
        // Transfer-Encoding: https://datatracker.ietf.org/doc/html/rfc7230.html#section-3.3.1
        // Chunked Transfer Coding: https://datatracker.ietf.org/doc/html/rfc7230.html#section-4.1
        // see evhttp_handle_chunked_read() in libevent http.c
        while (reader.Left() > 0) {
            auto maybe_chunk_size = reader.ReadLine();
            if (!maybe_chunk_size) return false;
            uint64_t chunk_size;

            if (!ParseUInt64Hex(maybe_chunk_size.value(), &chunk_size)) throw std::runtime_error("Invalid chunk size");

            bool last_chunk{chunk_size == 0};

            if (!last_chunk) {
                // We are still expecting more data for this chunk
                if (reader.Left() < chunk_size) {
                    return false;
                }
                // Pack chunk onto body
                m_body += reader.ReadLength(chunk_size);
            }

            // Even though every chunk size is explicitly declared,
            // they are still terminated by a CRLF we don't need.
            auto crlf = reader.ReadLine();
            if (!crlf || crlf.value().size() != 0) throw std::runtime_error("Improperly terminated chunk");

            if (last_chunk) return true;
        }

        // We read all the chunks but never got the last chunk, wait for client to send more
        return false;
    } else {
        // No Content-length or Transfer-Encoding header means no body, see libevent evhttp_get_body()
        auto content_length_value{m_headers.Find("Content-Length")};
        if (!content_length_value) return true;

        uint64_t content_length;
        if (!ParseUInt64(content_length_value.value(), &content_length)) throw std::runtime_error("Cannot parse Content-Length value");

        // Not enough data in buffer for expected body
        if (reader.Left() < content_length) return false;

        m_body = reader.ReadLength(content_length);

        return true;
    }
}

void HTTPRequest::WriteReply(HTTPStatusCode status, std::span<const std::byte> reply_body)
{
    HTTPResponse res;

    // Some response headers are determined in advance and stored in the request
    res.m_headers = std::move(m_response_headers);

    // Response version matches request version
    res.m_version_major = m_version_major;
    res.m_version_minor = m_version_minor;

    // Add response code and look up reason string
    res.m_status = status;
    res.m_reason = HTTPReason.find(status)->second;

    // See libevent evhttp_response_needs_body()
    // Response headers are different if no body is needed
    bool needs_body{status != HTTP_NO_CONTENT && (status < 100 || status >= 200)};

    // See libevent evhttp_make_header_response()
    // Expected response headers depend on protocol version
    if (m_version_major == 1) {
        // HTTP/1.0
        if (m_version_minor == 0) {
            auto connection_header{m_headers.Find("Connection")};
            if (connection_header && ToLower(connection_header.value()) == "keep-alive") {
                res.m_headers.Write("Connection", "keep-alive");
                res.m_keep_alive = true;
            }
        }

        // HTTP/1.1
        if (m_version_minor >= 1) {
            const int64_t now_seconds{TicksSinceEpoch<std::chrono::seconds>(NodeClock::now())};
            res.m_headers.Write("Date", FormatRFC7231DateTime(now_seconds));

            if (needs_body) {
                res.m_headers.Write("Content-Length", strprintf("%d", reply_body.size()));
            }

            // Default for HTTP/1.1
            res.m_keep_alive = true;
        }
    }

    if (needs_body && !res.m_headers.Find("Content-Type")) {
        // Default type from libevent evhttp_new_object()
        res.m_headers.Write("Content-Type", "text/html; charset=ISO-8859-1");
    }

    auto connection_header{m_headers.Find("Connection")};
    if (connection_header && ToLower(connection_header.value()) == "close") {
        // Might not exist already but we need to replace it, not append to it
        res.m_headers.Remove("Connection");
        res.m_headers.Write("Connection", "close");
        res.m_keep_alive = false;
    }

    // Serialize the response headers
    const std::string headers{res.StringifyHeaders()};
    const auto headers_bytes{std::as_bytes(std::span(headers.begin(), headers.end()))};

    // Fill the send buffer with the complete serialized response headers + body
    {
        LOCK(m_client->m_send_mutex);
        m_client->m_send_buffer.insert(m_client->m_send_buffer.end(), headers_bytes.begin(), headers_bytes.end());

        // We've been using std::span up until now but it is finally time to copy
        // data. The original data will go out of scope when WriteReply() returns.
        // This is analogous to the memcpy() in libevent's evbuffer_add()
        m_client->m_send_buffer.insert(m_client->m_send_buffer.end(), reply_body.begin(), reply_body.end());
    }

    // Inform Sockman I/O there is data that is ready to be sent to this client
    // in the next loop iteration.
    m_client->m_send_ready = true;

    LogDebug(
        BCLog::HTTP,
        "HTTPResponse (status code: %d size: %lld) added to send buffer for client %s (id=%lld)\n",
        status,
        headers_bytes.size() + reply_body.size(),
        m_client->m_origin,
        m_client->m_node_id);
}

bool HTTPClient::ReadRequest(std::unique_ptr<HTTPRequest>& req)
{
    LineReader reader(m_recv_buffer, MAX_HEADERS_SIZE);

    if (!req->LoadControlData(reader)) return false;
    if (!req->LoadHeaders(reader)) return false;
    if (!req->LoadBody(reader)) return false;

    // Remove the bytes read out of the buffer.
    // If one of the above calls throws an error, the caller must
    // catch it and disconnect the client.
    m_recv_buffer.erase(
        m_recv_buffer.begin(),
        m_recv_buffer.begin() + (reader.it - reader.start));

    return true;
}

bool HTTPClient::SendBytesFromBuffer()
{
    Assume(m_server);

    // Send as much data from this client's buffer as we can
    LOCK(m_send_mutex);
    if (!m_send_buffer.empty()) {
        std::string err;
        // We don't intend to "send more" because http responses are usually small and we want the kernel to send them right away.
        ssize_t bytes_sent = m_server->SendBytes(m_node_id, MakeUCharSpan(m_send_buffer), /*will_send_more=*/false, err);
        if (bytes_sent < 0) {
            LogDebug(
                BCLog::HTTP,
                "Error sending HTTP response data to client %s (id=%lld): %s\n",
                m_origin,
                m_node_id,
                err);
            // TODO: disconnect
            return false;
        }

        Assume(static_cast<size_t>(bytes_sent) <= m_send_buffer.size());
        m_send_buffer.erase(m_send_buffer.begin(), m_send_buffer.begin() + bytes_sent);

        LogDebug(
            BCLog::HTTP,
            "Sent %d bytes to client %s (id=%lld)\n",
            bytes_sent,
            m_origin,
            m_node_id);
    }

    return true;
}

bool HTTPServer::EventNewConnectionAccepted(NodeId node_id,
                                            const CService& me,
                                            const CService& them)
{
    auto client = std::make_shared<HTTPClient>(node_id, them);
    // Point back to the server
    client->m_server = this;
    LogDebug(BCLog::HTTP, "HTTP Connection accepted from %s (id=%d)\n", client->m_origin, client->m_node_id);
    m_connected_clients.emplace(client->m_node_id, std::move(client));
    m_no_clients = false;
    return true;
}

void HTTPServer::EventReadyToSend(NodeId node_id, bool& cancel_recv)
{
    // Next attempt to receive data from this node is permitted
    cancel_recv = false;

    // Get the HTTPClient
    auto client{GetClientById(node_id)};
    if (client == nullptr) {
        return;
    }

    // SendBytesFromBuffer() returns true if we should keep the client around,
    // false if we are done with it. Invert that boolean to inform Sockman
    // whether it should cancel the next receive attempt from this client.
    cancel_recv = !client->SendBytesFromBuffer();
}

void HTTPServer::EventGotData(NodeId node_id, std::span<const uint8_t> data)
{
    // Get the HTTPClient
    auto client{GetClientById(node_id)};
    if (client == nullptr) {
        return;
    }

    // Copy data from socket buffer to client receive buffer
    client->m_recv_buffer.insert(
        client->m_recv_buffer.end(),
        reinterpret_cast<const std::byte*>(data.data()),
        reinterpret_cast<const std::byte*>(data.data() + data.size())
    );

    // Try reading (potentially multiple) HTTP requests from the buffer
    while (client->m_recv_buffer.size() > 0) {
        // Create a new request object and try to fill it with data from the receive buffer
        auto req = std::make_unique<HTTPRequest>(client);
        try {
            // Stop reading if we need more data from the client to parse a complete request
            if (!client->ReadRequest(req)) break;
        } catch (const std::runtime_error& e) {
            LogDebug(
                BCLog::HTTP,
                "Error reading HTTP request from client %s (id=%lld): %s\n",
                client->m_origin,
                client->m_node_id,
                e.what());

            // We failed to read a complete request from the buffer
            // TODO: respond with HTTP_BAD_REQUEST and disconnect

            break;
        }

        // We read a complete request from the buffer into the queue
        LogDebug(
            BCLog::HTTP,
            "Received a %s request for %s from %s (id=%lld)\n",
            req->m_method,
            req->m_target,
            req->m_client->m_origin,
            req->m_client->m_node_id);

        // handle request
        m_request_dispatcher(std::move(req));
    }
}

bool HTTPServer::ShouldTryToSend(NodeId node_id) const
{
    // Get the HTTPClient
    auto client{GetClientById(node_id)};
    if (client == nullptr) {
        return false;
    }

    return client->m_send_ready;
}

bool HTTPServer::ShouldTryToRecv(NodeId node_id) const
{
    // Get the HTTPClient
    auto client{GetClientById(node_id)};
    if (client == nullptr) {
        return false;
    }

    // Don't try to receive again until we've cleared the send buffer to this client
    return !client->m_send_ready;
}

std::shared_ptr<HTTPClient> HTTPServer::GetClientById(NodeId node_id) const
{
    auto it{m_connected_clients.find(node_id)};
    if (it != m_connected_clients.end()) {
        return it->second;
    }
    return nullptr;
}
} // namespace http_bitcoin
