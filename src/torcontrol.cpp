#include "torcontrol.h"
#include "utilstrencodings.h"
#include "net.h"
#include "util.h"
#include "init.h" // Just for ShutdownRequested

#include <vector>
#include <deque>
#include <set>
#include <stdlib.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/event.h>

const std::string DEFAULT_TOR_CONTROL = "127.0.0.1:9051";

/****** Low-level TorControlConnection ********/

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
    typedef boost::function<void(TorControlConnection&)> ConnectionCB;
    typedef boost::function<void(TorControlConnection &,const TorControlReply &)> ReplyHandlerCB;

    /** Create a new TorControlConnection.
     */
    TorControlConnection(struct event_base *base);
    ~TorControlConnection();

    /**
     * Connect to a Tor control port.
     * target is address of the form host:port.
     * connected is the handler that is called when connection is succesfully established.
     * disconnected is a handler that is called when the connection is broken.
     * Return true on success.
     */
    bool Connect(const std::string &target, const ConnectionCB& connected, const ConnectionCB& disconnected);

    /**
     * Disconnect from Tor control port.
     */
    bool Disconnect();

    /** Send a command, register a handler for the reply.
     * A trailing CRLF is automatically added.
     * Return true on success.
     */
    bool Command(const std::string &cmd, const ReplyHandlerCB& reply_handler);

    /** Response handlers for async replies */
    boost::signals2::signal<void(TorControlConnection &,const TorControlReply &)> async_handler;
private:
    /** Callback when ready for use */
    boost::function<void(TorControlConnection&)> connected;
    /** Callback when connection lost */
    boost::function<void(TorControlConnection&)> disconnected;
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

TorControlConnection::TorControlConnection(struct event_base *base):
    base(base), b_conn(0)
{
}

TorControlConnection::~TorControlConnection()
{
    if (b_conn)
        bufferevent_free(b_conn);
}

void TorControlConnection::readcb(struct bufferevent *bev, void *ctx)
{
    TorControlConnection *self = (TorControlConnection*)ctx;
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t n_read_out = 0;
    char *line;
    assert(input);
    //  If there is not a whole line to read, evbuffer_readln returns NULL
    while((line = evbuffer_readln(input, &n_read_out, EVBUFFER_EOL_CRLF)) != NULL)
    {
        std::string s(line, n_read_out);
        free(line);
        if (s.size() < 4) // Short line
            continue;
        // <status>(-|+| )<data><CRLF>
        self->message.code = atoi(s.substr(0,3).c_str());
        self->message.lines.push_back(s.substr(4));
        char ch = s[3]; // '-','+' or ' '
        if (ch == ' ') {
            // Final line, dispatch reply and clean up
            if (self->message.code >= 600) {
                // Dispatch async notifications to async handler
                // Synchronous and asynchronous messages are never interleaved
                self->async_handler(*self, self->message);
            } else {
                if (!self->reply_handlers.empty()) {
                    // Invoke reply handler with message
                    self->reply_handlers.front()(*self, self->message);
                    self->reply_handlers.pop_front();
                } else {
                    LogPrintf("[tor] Received unexpected sync reply %i\n", self->message.code);
                }
            }
            self->message.Clear();
        }
    }
}

void TorControlConnection::eventcb(struct bufferevent *bev, short what, void *ctx)
{
    TorControlConnection *self = (TorControlConnection*)ctx;
    if (what & BEV_EVENT_CONNECTED) {
        LogPrintf("[tor] Succesfully connected!\n");
        self->connected(*self);
    } else if (what & (BEV_EVENT_EOF|BEV_EVENT_ERROR)) {
        if (what & BEV_EVENT_ERROR)
            LogPrintf("[tor] Error connecting to Tor control socket\n");
        else
            LogPrintf("[tor] End of stream\n");
        self->Disconnect();
        self->disconnected(*self);
    }
}

bool TorControlConnection::Connect(const std::string &target, const ConnectionCB& connected, const ConnectionCB& disconnected)
{
    if (b_conn)
        Disconnect();
    // Parse target address:port
    struct sockaddr_storage connect_to_addr;
    int connect_to_addrlen = sizeof(connect_to_addr);
    if (evutil_parse_sockaddr_port(target.c_str(),
        (struct sockaddr*)&connect_to_addr, &connect_to_addrlen)<0) {
        perror("evutil_parse_sockaddr_port\n");
        return false;
    }

    // Create a new socket, set up callbacks and enable notification bits
    b_conn = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (!b_conn)
        return false;
    bufferevent_setcb(b_conn, TorControlConnection::readcb, NULL, TorControlConnection::eventcb, this);
    bufferevent_enable(b_conn, EV_READ|EV_WRITE);
    this->connected = connected;
    this->disconnected = disconnected;

    // Finally, connect to target
    if (bufferevent_socket_connect(b_conn, (struct sockaddr*)&connect_to_addr, connect_to_addrlen) < 0) {
        perror("bufferevent_socket_connect");
        return false;
    }
    return true;
}

bool TorControlConnection::Disconnect()
{
    if (b_conn)
        bufferevent_free(b_conn);
    b_conn = 0;
    return true;
}

bool TorControlConnection::Command(const std::string &cmd, const ReplyHandlerCB& reply_handler)
{
    if (!b_conn)
        return false;
    struct evbuffer *buf = bufferevent_get_output(b_conn);
    if (!buf)
        return false;
    evbuffer_add(buf, cmd.data(), cmd.size());
    evbuffer_add(buf, "\r\n", 2);
    reply_handlers.push_back(reply_handler);
    return true;
}

/****** General parsing utilities ********/

/* Split reply line in the form 'AUTH METHODS=...' into a type
 * 'AUTH' and arguments 'METHODS=...'.
 */
static std::pair<std::string,std::string> SplitTorReplyLine(const std::string &s)
{
    size_t ptr=0;
    std::string type;
    while (ptr < s.size() && s[ptr] != ' ') {
        type.push_back(s[ptr]);
        ++ptr;
    }
    if (ptr < s.size())
        ++ptr; // skip ' '
    return make_pair(type, s.substr(ptr));
}

/** Parse reply arguments in the form 'METHODS=COOKIE,SAFECOOKIE COOKIEFILE=".../control_auth_cookie"'.
 */
static std::map<std::string,std::string> ParseTorReplyMapping(const std::string &s)
{
    std::map<std::string,std::string> mapping;
    size_t ptr=0;
    while (ptr < s.size()) {
        std::string key, value;
        while (ptr < s.size() && s[ptr] != '=') {
            key.push_back(s[ptr]);
            ++ptr;
        }
        if (ptr == s.size()) // unexpected end of line
            return std::map<std::string,std::string>();
        ++ptr; // skip '='
        if (ptr < s.size() && s[ptr] == '"') { // Quoted string
            ++ptr; // skip '='
            bool escape_next = false;
            while (ptr < s.size() && (!escape_next && s[ptr] != '"')) {
                escape_next = (s[ptr] == '\\');
                value.push_back(s[ptr]);
                ++ptr;
            }
            if (ptr == s.size()) // unexpected end of line
                return std::map<std::string,std::string>();
            ++ptr; // skip closing '"'
            /* TODO: unescape value - according to the spec this depends on the
             * context, some strings use C-LogPrintf style escape codes, some
             * don't. So may be better handled at the call site.
             */
        } else { // Unquoted value. Note that values can contain '=' at will, just no spaces
            while (ptr < s.size() && s[ptr] != ' ') {
                value.push_back(s[ptr]);
                ++ptr;
            }
        }
        if (ptr < s.size() && s[ptr] == ' ')
            ++ptr; // skip ' ' after key=value
        mapping[key] = value;
    }
    return mapping;
}

/** Read full contents of a file and return them in a std::string. */
static std::pair<bool,std::string> ReadBinaryFile(const std::string &filename)
{
    FILE *f = fopen(filename.c_str(), "rb");
    if (f == NULL)
        return std::make_pair(false,"");
    std::string retval;
    char buffer[128];
    size_t n;
    while ((n=fread(buffer, 1, sizeof(buffer), f)) > 0)
        retval.append(buffer, buffer+n);
    fclose(f);
    return std::make_pair(true,retval);
}

/** Write contents of std::string to a file.
 * @return true on success.
 */
static bool WriteBinaryFile(const std::string &filename, const std::string &data)
{
    FILE *f = fopen(filename.c_str(), "wb");
    if (f == NULL)
        return false;
    if (fwrite(data.data(), 1, data.size(), f) != data.size())
        return false;
    fclose(f);
    return true;
}

/****** Bitcoin specific TorController implementation ********/

/** Controller that connects to Tor control socket, authenticate, then create
 * and maintain a ephemeral hidden service.
 */
class TorController
{
public:
    TorController(struct event_base* base, const std::string& target);
    ~TorController();

    /** Get name fo file to store private key in */
    std::string GetPrivateKeyFile();

    /** Reconnect, after getting disconnected */
    void Reconnect();
private:
    struct event_base* base;
    std::string target;
    TorControlConnection conn;
    std::string private_key;
    std::string service_id;
    bool reconnect;
    struct event *shutdown_poll_ev;
    struct event *reconnect_ev;
    float reconnect_timeout;

    /** Callback for ADD_ONION result */
    void add_onion_cb(TorControlConnection& conn, const TorControlReply& reply);
    /** Callback for AUTHENTICATE result */
    void auth_cb(TorControlConnection& conn, const TorControlReply& reply);
    /** Callback for PROTOCOLINFO result */
    void protocolinfo_cb(TorControlConnection& conn, const TorControlReply& reply);
    /** Callback after succesful connection */
    void connected_cb(TorControlConnection& conn);
    /** Callback after connection lost or failed connection attempt */
    void disconnected_cb(TorControlConnection& conn);

    /** Callback for shutdown poll timer */
    static void shutdown_poll_cb(evutil_socket_t fd, short what, void *arg);
    /** Callback for reconnect timer */
    static void reconnect_cb(evutil_socket_t fd, short what, void *arg);
};

/** Exponential backoff configuration - initial timeout in seconds */
static const float RECONNECT_TIMEOUT_START = 1.0;
/** Exponential backoff configuration - growth factor */
static const float RECONNECT_TIMEOUT_EXP = 1.5;

TorController::TorController(struct event_base* base, const std::string& target):
    base(base),
    target(target), conn(base), reconnect(true), shutdown_poll_ev(0), reconnect_ev(0),
    reconnect_timeout(RECONNECT_TIMEOUT_START)
{
    // Start connection attempts immediately
    if (!conn.Connect(target, boost::bind(&TorController::connected_cb, this, _1),
         boost::bind(&TorController::disconnected_cb, this, _1) )) {
        LogPrintf("[tor] Initiating connection to Tor control port %s failed\n", target);
    }
    // Read service private key if cached
    std::pair<bool,std::string> pkf = ReadBinaryFile(GetPrivateKeyFile());
    if (pkf.first) {
        LogPrintf("[tor] Reading cached private key from %s\n", GetPrivateKeyFile());
        private_key = pkf.second;
    }
    // Periodic timer event to poll for shutdown
    // The same 200ms as in bitcoind. This is not the nicest solution, but we cannot exactly use
    // boost::interrupt here.
    struct timeval time;
    time.tv_usec = 200000;
    time.tv_sec = 0;
    shutdown_poll_ev = event_new(base, -1, EV_PERSIST, shutdown_poll_cb, this);
    event_add(shutdown_poll_ev, &time);
}

TorController::~TorController()
{
    if (shutdown_poll_ev)
        event_del(shutdown_poll_ev);
    if (reconnect_ev)
        event_del(reconnect_ev);
}

void TorController::add_onion_cb(TorControlConnection& conn, const TorControlReply& reply)
{
    if (reply.code == 250) {
        LogPrintf("[tor] ADD_ONION succesful\n");
        BOOST_FOREACH(const std::string &s, reply.lines) {
            std::map<std::string,std::string> m = ParseTorReplyMapping(s);
            std::map<std::string,std::string>::iterator i;
            if ((i = m.find("ServiceID")) != m.end())
                service_id = i->second;
            if ((i = m.find("PrivateKey")) != m.end())
                private_key = i->second;
        }

        CService service(service_id+".onion", GetListenPort(), false);
        LogPrintf("[tor] Got service ID %s, advertizing service %s\n", service_id, service.ToString());
        if (WriteBinaryFile(GetPrivateKeyFile(), private_key)) {
            LogPrintf("[tor] Cached service private key to %s\n", GetPrivateKeyFile());
        } else {
            LogPrintf("[tor] Error writing service private key to %s\n", GetPrivateKeyFile());
        }
        AddLocal(service, LOCAL_MANUAL);
        // ... onion requested - keep connection open
    } else {
        LogPrintf("[tor] Add onion failed\n");
    }
}

void TorController::auth_cb(TorControlConnection& conn, const TorControlReply& reply)
{
    if (reply.code == 250) {
        LogPrintf("[tor] Authentication succesful\n");
        // Finally - now create the service
        if (private_key.empty()) // No private key, generate one
            private_key = "NEW:BEST";
        // Request hidden service, redirect port.
        // Note that the 'virtual' port doesn't have to be the same as our internal port, but this is just a convenient
        // choice.  TODO; refactor the shutdown sequence some day.
        conn.Command(strprintf("ADD_ONION %s Port=%i,127.0.0.1:%i", private_key, GetListenPort(), GetListenPort()),
            boost::bind(&TorController::add_onion_cb, this, _1, _2));
    } else {
        LogPrintf("[tor] Authentication failed\n");
    }
}

void TorController::protocolinfo_cb(TorControlConnection& conn, const TorControlReply& reply)
{
    if (reply.code == 250) {
        std::set<std::string> methods;
        std::string cookiefile;
        /*
         * 250-AUTH METHODS=COOKIE,SAFECOOKIE COOKIEFILE="/home/x/.tor/control_auth_cookie"
         * 250-AUTH METHODS=NULL
         * 250-AUTH METHODS=HASHEDPASSWORD
         */
        BOOST_FOREACH(const std::string &s, reply.lines) {
            std::pair<std::string,std::string> l = SplitTorReplyLine(s);
            if (l.first == "AUTH") {
                std::map<std::string,std::string> m = ParseTorReplyMapping(l.second);
                std::map<std::string,std::string>::iterator i;
                if ((i = m.find("METHODS")) != m.end())
                    boost::split(methods, i->second, boost::is_any_of(","));
                if ((i = m.find("COOKIEFILE")) != m.end())
                    cookiefile = i->second;
            } else if (l.first == "VERSION") {
                std::map<std::string,std::string> m = ParseTorReplyMapping(l.second);
                std::map<std::string,std::string>::iterator i;
                if ((i = m.find("Tor")) != m.end()) {
                    LogPrintf("[tor] Connected to Tor version %s\n", i->second);
                }
            }
        }
        BOOST_FOREACH(const std::string &s, methods) {
            LogPrintf("[tor] Supported authentication method: %s\n", s);
        }
        // Prefer NULL, otherwise COOKIE. If a password is provided, use HASHEDPASSWORD
        // We do not support SAFECOOKIE
        /* Authentication:
         *   cookie:   hex-encoded ~/.tor/control_auth_cookie
         *   password: "password"
         */
        if (methods.count("NULL")) {
            LogPrintf("[tor] Using NULL authentication\n");
            conn.Command("AUTHENTICATE", boost::bind(&TorController::auth_cb, this, _1, _2));
        } else if (methods.count("COOKIE")) {
            // Cookie: hexdump -e '32/1 "%02x""\n"'  ~/.tor/control_auth_cookie
            LogPrintf("[tor] Using COOKIE authentication, reading cookie authentication from %s\n", cookiefile);
            std::string cookie = ReadBinaryFile(cookiefile).second;
            if (!cookie.empty()) {
                conn.Command("AUTHENTICATE " + HexStr(cookie), boost::bind(&TorController::auth_cb, this, _1, _2));
            } else {
                LogPrintf("[tor] Authentication cookie not found\n");
            }
        } else {
            /* TODO HASHEDPASSWORD w/ manual auth */
            LogPrintf("[tor] No supported authentication method\n");
        }
    } else {
        LogPrintf("[tor] Requesting protocol info failed\n");
    }
}

void TorController::connected_cb(TorControlConnection& conn)
{
    reconnect_timeout = RECONNECT_TIMEOUT_START;
    // First send a PROTOCOLINFO command to figure out what authentication is expected
    if (!conn.Command("PROTOCOLINFO 1", boost::bind(&TorController::protocolinfo_cb, this, _1, _2)))
        LogPrintf("[tor] Error sending initial protocolinfo command\n");
}

void TorController::disconnected_cb(TorControlConnection& conn)
{
    if (!reconnect)
        return;
    LogPrintf("[tor] Disconnected from Tor control port %s, trying to reconnect\n", target);
    // Single-shot timer for reconnect. Use exponential backoff.
    struct timeval time = MillisToTimeval(int64_t(reconnect_timeout * 1000.0));
    reconnect_ev = event_new(base, -1, 0, reconnect_cb, this);
    event_add(reconnect_ev, &time);
    reconnect_timeout *= RECONNECT_TIMEOUT_EXP;
}

void TorController::Reconnect()
{
    /* Try to reconnect and reestablish if we get booted - for example, Tor
     * may be restarting.
     */
    if (!conn.Connect(target, boost::bind(&TorController::connected_cb, this, _1),
         boost::bind(&TorController::disconnected_cb, this, _1) )) {
        LogPrintf("[tor] Re-initiating connection to Tor control port %s failed\n", target);
    }
}

std::string TorController::GetPrivateKeyFile()
{
    return (GetDataDir() / "onion_private_key").string();
}

void TorController::shutdown_poll_cb(evutil_socket_t fd, short what, void *arg)
{
    TorController *self = (TorController*)arg;
    if (ShutdownRequested()) {
        // Shutdown was requested. Stop timers, and request control connection to terminate
        LogPrintf("[tor] Thread interrupt\n");
        if (self->shutdown_poll_ev)
            event_del(self->shutdown_poll_ev);
        self->shutdown_poll_ev = 0;
        if (self->reconnect_ev)
            event_del(self->reconnect_ev);
        self->reconnect_ev = 0;
        self->reconnect = false;
        self->conn.Disconnect();
    }
}

void TorController::reconnect_cb(evutil_socket_t fd, short what, void *arg)
{
    TorController *self = (TorController*)arg;
    self->Reconnect();
}

/****** Thread ********/

static void TorControlThread()
{
    struct event_base *base = event_base_new();
    if (!base) {
        LogPrintf("[tor] Unable to create event_base_new");
        return;
    }
    TorController ctrl(base, GetArg("-torcontrol", DEFAULT_TOR_CONTROL));

    event_base_dispatch(base);
    event_base_free(base);
}

void StartTorControl(boost::thread_group& threadGroup, CScheduler& scheduler)
{
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "torcontrol", &TorControlThread));
}

void StopTorControl()
{
    /* Nothing to do actually. Everything is cleaned up when thread exits */
}

