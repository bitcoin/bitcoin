// Copyright (c) 2015-2020 The Bitcoin Core developers
// Copyright (c) 2017 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <torcontrol.h>

#include <chainparams.h>
#include <chainparamsbase.h>
#include <compat.h>
#include <crypto/hmac_sha256.h>
#include <net.h>
#include <netaddress.h>
#include <netbase.h>
#include <util/readwritefile.h>
#include <util/strencodings.h>
#include <util/syscall_sandbox.h>
#include <util/system.h>
#include <util/thread.h>
#include <util/time.h>

#include <deque>
#include <functional>
#include <set>
#include <stdlib.h>
#include <vector>

#include <boost/signals2/signal.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>

/** Default control port */
const std::string DEFAULT_TOR_CONTROL = "127.0.0.1:9051";
/** Tor cookie size (from control-spec.txt) */
static const int TOR_COOKIE_SIZE = 32;
/** Size of client/server nonce for SAFECOOKIE */
static const int TOR_NONCE_SIZE = 32;
/** For computing serverHash in SAFECOOKIE */
static const std::string TOR_SAFE_SERVERKEY = "Tor safe cookie authentication server-to-controller hash";
/** For computing clientHash in SAFECOOKIE */
static const std::string TOR_SAFE_CLIENTKEY = "Tor safe cookie authentication controller-to-server hash";
/** Exponential backoff configuration - initial timeout in seconds */
static const float RECONNECT_TIMEOUT_START = 1.0;
/** Exponential backoff configuration - growth factor */
static const float RECONNECT_TIMEOUT_EXP = 1.5;
/** Maximum length for lines received on TorControlConnection.
 * tor-control-spec.txt mentions that there is explicitly no limit defined to line length,
 * this is belt-and-suspenders sanity limit to prevent memory exhaustion.
 */
static const int MAX_LINE_LENGTH = 100000;

/****** Low-level TorControlConnection ********/

TorControlConnection::TorControlConnection(struct event_base *_base):
    base(_base), b_conn(nullptr)
{
}

TorControlConnection::~TorControlConnection()
{
    if (b_conn)
        bufferevent_free(b_conn);
}

void TorControlConnection::readcb(struct bufferevent *bev, void *ctx)
{
    TorControlConnection *self = static_cast<TorControlConnection*>(ctx);
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t n_read_out = 0;
    char *line;
    assert(input);
    //  If there is not a whole line to read, evbuffer_readln returns nullptr
    while((line = evbuffer_readln(input, &n_read_out, EVBUFFER_EOL_CRLF)) != nullptr)
    {
        std::string s(line, n_read_out);
        free(line);
        if (s.size() < 4) // Short line
            continue;
        // <status>(-|+| )<data><CRLF>
        self->message.code = LocaleIndependentAtoi<int>(s.substr(0,3));
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
                    LogPrint(BCLog::TOR, "tor: Received unexpected sync reply %i\n", self->message.code);
                }
            }
            self->message.Clear();
        }
    }
    //  Check for size of buffer - protect against memory exhaustion with very long lines
    //  Do this after evbuffer_readln to make sure all full lines have been
    //  removed from the buffer. Everything left is an incomplete line.
    if (evbuffer_get_length(input) > MAX_LINE_LENGTH) {
        LogPrintf("tor: Disconnecting because MAX_LINE_LENGTH exceeded\n");
        self->Disconnect();
    }
}

void TorControlConnection::eventcb(struct bufferevent *bev, short what, void *ctx)
{
    TorControlConnection *self = static_cast<TorControlConnection*>(ctx);
    if (what & BEV_EVENT_CONNECTED) {
        LogPrint(BCLog::TOR, "tor: Successfully connected!\n");
        self->connected(*self);
    } else if (what & (BEV_EVENT_EOF|BEV_EVENT_ERROR)) {
        if (what & BEV_EVENT_ERROR) {
            LogPrint(BCLog::TOR, "tor: Error connecting to Tor control socket\n");
        } else {
            LogPrint(BCLog::TOR, "tor: End of stream\n");
        }
        self->Disconnect();
        self->disconnected(*self);
    }
}

bool TorControlConnection::Connect(const std::string& tor_control_center, const ConnectionCB& _connected, const ConnectionCB& _disconnected)
{
    if (b_conn) {
        Disconnect();
    }

    CService control_service;
    if (!Lookup(tor_control_center, control_service, 9051, fNameLookup)) {
        LogPrintf("tor: Failed to look up control center %s\n", tor_control_center);
        return false;
    }

    struct sockaddr_storage control_address;
    socklen_t control_address_len = sizeof(control_address);
    if (!control_service.GetSockAddr(reinterpret_cast<struct sockaddr*>(&control_address), &control_address_len)) {
        LogPrintf("tor: Error parsing socket address %s\n", tor_control_center);
        return false;
    }

    // Create a new socket, set up callbacks and enable notification bits
    b_conn = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (!b_conn) {
        return false;
    }
    bufferevent_setcb(b_conn, TorControlConnection::readcb, nullptr, TorControlConnection::eventcb, this);
    bufferevent_enable(b_conn, EV_READ|EV_WRITE);
    this->connected = _connected;
    this->disconnected = _disconnected;

    // Finally, connect to tor_control_center
    if (bufferevent_socket_connect(b_conn, reinterpret_cast<struct sockaddr*>(&control_address), control_address_len) < 0) {
        LogPrintf("tor: Error connecting to address %s\n", tor_control_center);
        return false;
    }
    return true;
}

void TorControlConnection::Disconnect()
{
    if (b_conn)
        bufferevent_free(b_conn);
    b_conn = nullptr;
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
 * Grammar is implicitly defined in https://spec.torproject.org/control-spec by
 * the server reply formats for PROTOCOLINFO (S3.21) and AUTHCHALLENGE (S3.24).
 */
std::pair<std::string,std::string> SplitTorReplyLine(const std::string &s)
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
 * Returns a map of keys to values, or an empty map if there was an error.
 * Grammar is implicitly defined in https://spec.torproject.org/control-spec by
 * the server reply formats for PROTOCOLINFO (S3.21), AUTHCHALLENGE (S3.24),
 * and ADD_ONION (S3.27). See also sections 2.1 and 2.3.
 */
std::map<std::string,std::string> ParseTorReplyMapping(const std::string &s)
{
    std::map<std::string,std::string> mapping;
    size_t ptr=0;
    while (ptr < s.size()) {
        std::string key, value;
        while (ptr < s.size() && s[ptr] != '=' && s[ptr] != ' ') {
            key.push_back(s[ptr]);
            ++ptr;
        }
        if (ptr == s.size()) // unexpected end of line
            return std::map<std::string,std::string>();
        if (s[ptr] == ' ') // The remaining string is an OptArguments
            break;
        ++ptr; // skip '='
        if (ptr < s.size() && s[ptr] == '"') { // Quoted string
            ++ptr; // skip opening '"'
            bool escape_next = false;
            while (ptr < s.size() && (escape_next || s[ptr] != '"')) {
                // Repeated backslashes must be interpreted as pairs
                escape_next = (s[ptr] == '\\' && !escape_next);
                value.push_back(s[ptr]);
                ++ptr;
            }
            if (ptr == s.size()) // unexpected end of line
                return std::map<std::string,std::string>();
            ++ptr; // skip closing '"'
            /**
             * Unescape value. Per https://spec.torproject.org/control-spec section 2.1.1:
             *
             *   For future-proofing, controller implementors MAY use the following
             *   rules to be compatible with buggy Tor implementations and with
             *   future ones that implement the spec as intended:
             *
             *     Read \n \t \r and \0 ... \377 as C escapes.
             *     Treat a backslash followed by any other character as that character.
             */
            std::string escaped_value;
            for (size_t i = 0; i < value.size(); ++i) {
                if (value[i] == '\\') {
                    // This will always be valid, because if the QuotedString
                    // ended in an odd number of backslashes, then the parser
                    // would already have returned above, due to a missing
                    // terminating double-quote.
                    ++i;
                    if (value[i] == 'n') {
                        escaped_value.push_back('\n');
                    } else if (value[i] == 't') {
                        escaped_value.push_back('\t');
                    } else if (value[i] == 'r') {
                        escaped_value.push_back('\r');
                    } else if ('0' <= value[i] && value[i] <= '7') {
                        size_t j;
                        // Octal escape sequences have a limit of three octal digits,
                        // but terminate at the first character that is not a valid
                        // octal digit if encountered sooner.
                        for (j = 1; j < 3 && (i+j) < value.size() && '0' <= value[i+j] && value[i+j] <= '7'; ++j) {}
                        // Tor restricts first digit to 0-3 for three-digit octals.
                        // A leading digit of 4-7 would therefore be interpreted as
                        // a two-digit octal.
                        if (j == 3 && value[i] > '3') {
                            j--;
                        }
                        escaped_value.push_back(strtol(value.substr(i, j).c_str(), nullptr, 8));
                        // Account for automatic incrementing at loop end
                        i += j - 1;
                    } else {
                        escaped_value.push_back(value[i]);
                    }
                } else {
                    escaped_value.push_back(value[i]);
                }
            }
            value = escaped_value;
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

TorController::TorController(struct event_base* _base, const std::string& tor_control_center, const CService& target):
    base(_base),
    m_tor_control_center(tor_control_center), conn(base), reconnect(true), reconnect_ev(0),
    reconnect_timeout(RECONNECT_TIMEOUT_START),
    m_target(target)
{
    reconnect_ev = event_new(base, -1, 0, reconnect_cb, this);
    if (!reconnect_ev)
        LogPrintf("tor: Failed to create event for reconnection: out of memory?\n");
    // Start connection attempts immediately
    if (!conn.Connect(m_tor_control_center, std::bind(&TorController::connected_cb, this, std::placeholders::_1),
         std::bind(&TorController::disconnected_cb, this, std::placeholders::_1) )) {
        LogPrintf("tor: Initiating connection to Tor control port %s failed\n", m_tor_control_center);
    }
    // Read service private key if cached
    std::pair<bool,std::string> pkf = ReadBinaryFile(GetPrivateKeyFile());
    if (pkf.first) {
        LogPrint(BCLog::TOR, "tor: Reading cached private key from %s\n", GetPrivateKeyFile().string());
        private_key = pkf.second;
    }
}

TorController::~TorController()
{
    if (reconnect_ev) {
        event_free(reconnect_ev);
        reconnect_ev = nullptr;
    }
    if (service.IsValid()) {
        RemoveLocal(service);
    }
}

void TorController::add_onion_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    if (reply.code == 250) {
        LogPrint(BCLog::TOR, "tor: ADD_ONION successful\n");
        for (const std::string &s : reply.lines) {
            std::map<std::string,std::string> m = ParseTorReplyMapping(s);
            std::map<std::string,std::string>::iterator i;
            if ((i = m.find("ServiceID")) != m.end())
                service_id = i->second;
            if ((i = m.find("PrivateKey")) != m.end())
                private_key = i->second;
        }
        if (service_id.empty()) {
            LogPrintf("tor: Error parsing ADD_ONION parameters:\n");
            for (const std::string &s : reply.lines) {
                LogPrintf("    %s\n", SanitizeString(s));
            }
            return;
        }
        service = LookupNumeric(std::string(service_id+".onion"), Params().GetDefaultPort());
        LogPrintf("tor: Got service ID %s, advertising service %s\n", service_id, service.ToString());
        if (WriteBinaryFile(GetPrivateKeyFile(), private_key)) {
            LogPrint(BCLog::TOR, "tor: Cached service private key to %s\n", GetPrivateKeyFile().string());
        } else {
            LogPrintf("tor: Error writing service private key to %s\n", GetPrivateKeyFile().string());
        }
        AddLocal(service, LOCAL_MANUAL);
        // ... onion requested - keep connection open
    } else if (reply.code == 510) { // 510 Unrecognized command
        LogPrintf("tor: Add onion failed with unrecognized command (You probably need to upgrade Tor)\n");
    } else {
        LogPrintf("tor: Add onion failed; error code %d\n", reply.code);
    }
}

void TorController::auth_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    if (reply.code == 250) {
        LogPrint(BCLog::TOR, "tor: Authentication successful\n");

        // Now that we know Tor is running setup the proxy for onion addresses
        // if -onion isn't set to something else.
        if (gArgs.GetArg("-onion", "") == "") {
            CService resolved(LookupNumeric("127.0.0.1", 9050));
            proxyType addrOnion = proxyType(resolved, true);
            SetProxy(NET_ONION, addrOnion);
            SetReachable(NET_ONION, true);
        }

        // Finally - now create the service
        if (private_key.empty()) { // No private key, generate one
            private_key = "NEW:ED25519-V3"; // Explicitly request key type - see issue #9214
        }
        // Request onion service, redirect port.
        // Note that the 'virtual' port is always the default port to avoid decloaking nodes using other ports.
        _conn.Command(strprintf("ADD_ONION %s Port=%i,%s", private_key, Params().GetDefaultPort(), m_target.ToStringIPPort()),
            std::bind(&TorController::add_onion_cb, this, std::placeholders::_1, std::placeholders::_2));
    } else {
        LogPrintf("tor: Authentication failed\n");
    }
}

/** Compute Tor SAFECOOKIE response.
 *
 *    ServerHash is computed as:
 *      HMAC-SHA256("Tor safe cookie authentication server-to-controller hash",
 *                  CookieString | ClientNonce | ServerNonce)
 *    (with the HMAC key as its first argument)
 *
 *    After a controller sends a successful AUTHCHALLENGE command, the
 *    next command sent on the connection must be an AUTHENTICATE command,
 *    and the only authentication string which that AUTHENTICATE command
 *    will accept is:
 *
 *      HMAC-SHA256("Tor safe cookie authentication controller-to-server hash",
 *                  CookieString | ClientNonce | ServerNonce)
 *
 */
static std::vector<uint8_t> ComputeResponse(const std::string &key, const std::vector<uint8_t> &cookie,  const std::vector<uint8_t> &clientNonce, const std::vector<uint8_t> &serverNonce)
{
    CHMAC_SHA256 computeHash((const uint8_t*)key.data(), key.size());
    std::vector<uint8_t> computedHash(CHMAC_SHA256::OUTPUT_SIZE, 0);
    computeHash.Write(cookie.data(), cookie.size());
    computeHash.Write(clientNonce.data(), clientNonce.size());
    computeHash.Write(serverNonce.data(), serverNonce.size());
    computeHash.Finalize(computedHash.data());
    return computedHash;
}

void TorController::authchallenge_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    if (reply.code == 250) {
        LogPrint(BCLog::TOR, "tor: SAFECOOKIE authentication challenge successful\n");
        std::pair<std::string,std::string> l = SplitTorReplyLine(reply.lines[0]);
        if (l.first == "AUTHCHALLENGE") {
            std::map<std::string,std::string> m = ParseTorReplyMapping(l.second);
            if (m.empty()) {
                LogPrintf("tor: Error parsing AUTHCHALLENGE parameters: %s\n", SanitizeString(l.second));
                return;
            }
            std::vector<uint8_t> serverHash = ParseHex(m["SERVERHASH"]);
            std::vector<uint8_t> serverNonce = ParseHex(m["SERVERNONCE"]);
            LogPrint(BCLog::TOR, "tor: AUTHCHALLENGE ServerHash %s ServerNonce %s\n", HexStr(serverHash), HexStr(serverNonce));
            if (serverNonce.size() != 32) {
                LogPrintf("tor: ServerNonce is not 32 bytes, as required by spec\n");
                return;
            }

            std::vector<uint8_t> computedServerHash = ComputeResponse(TOR_SAFE_SERVERKEY, cookie, clientNonce, serverNonce);
            if (computedServerHash != serverHash) {
                LogPrintf("tor: ServerHash %s does not match expected ServerHash %s\n", HexStr(serverHash), HexStr(computedServerHash));
                return;
            }

            std::vector<uint8_t> computedClientHash = ComputeResponse(TOR_SAFE_CLIENTKEY, cookie, clientNonce, serverNonce);
            _conn.Command("AUTHENTICATE " + HexStr(computedClientHash), std::bind(&TorController::auth_cb, this, std::placeholders::_1, std::placeholders::_2));
        } else {
            LogPrintf("tor: Invalid reply to AUTHCHALLENGE\n");
        }
    } else {
        LogPrintf("tor: SAFECOOKIE authentication challenge failed\n");
    }
}

void TorController::protocolinfo_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    if (reply.code == 250) {
        std::set<std::string> methods;
        std::string cookiefile;
        /*
         * 250-AUTH METHODS=COOKIE,SAFECOOKIE COOKIEFILE="/home/x/.tor/control_auth_cookie"
         * 250-AUTH METHODS=NULL
         * 250-AUTH METHODS=HASHEDPASSWORD
         */
        for (const std::string &s : reply.lines) {
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
                    LogPrint(BCLog::TOR, "tor: Connected to Tor version %s\n", i->second);
                }
            }
        }
        for (const std::string &s : methods) {
            LogPrint(BCLog::TOR, "tor: Supported authentication method: %s\n", s);
        }
        // Prefer NULL, otherwise SAFECOOKIE. If a password is provided, use HASHEDPASSWORD
        /* Authentication:
         *   cookie:   hex-encoded ~/.tor/control_auth_cookie
         *   password: "password"
         */
        std::string torpassword = gArgs.GetArg("-torpassword", "");
        if (!torpassword.empty()) {
            if (methods.count("HASHEDPASSWORD")) {
                LogPrint(BCLog::TOR, "tor: Using HASHEDPASSWORD authentication\n");
                boost::replace_all(torpassword, "\"", "\\\"");
                _conn.Command("AUTHENTICATE \"" + torpassword + "\"", std::bind(&TorController::auth_cb, this, std::placeholders::_1, std::placeholders::_2));
            } else {
                LogPrintf("tor: Password provided with -torpassword, but HASHEDPASSWORD authentication is not available\n");
            }
        } else if (methods.count("NULL")) {
            LogPrint(BCLog::TOR, "tor: Using NULL authentication\n");
            _conn.Command("AUTHENTICATE", std::bind(&TorController::auth_cb, this, std::placeholders::_1, std::placeholders::_2));
        } else if (methods.count("SAFECOOKIE")) {
            // Cookie: hexdump -e '32/1 "%02x""\n"'  ~/.tor/control_auth_cookie
            LogPrint(BCLog::TOR, "tor: Using SAFECOOKIE authentication, reading cookie authentication from %s\n", cookiefile);
            std::pair<bool,std::string> status_cookie = ReadBinaryFile(cookiefile, TOR_COOKIE_SIZE);
            if (status_cookie.first && status_cookie.second.size() == TOR_COOKIE_SIZE) {
                // _conn.Command("AUTHENTICATE " + HexStr(status_cookie.second), std::bind(&TorController::auth_cb, this, std::placeholders::_1, std::placeholders::_2));
                cookie = std::vector<uint8_t>(status_cookie.second.begin(), status_cookie.second.end());
                clientNonce = std::vector<uint8_t>(TOR_NONCE_SIZE, 0);
                GetRandBytes(clientNonce.data(), TOR_NONCE_SIZE);
                _conn.Command("AUTHCHALLENGE SAFECOOKIE " + HexStr(clientNonce), std::bind(&TorController::authchallenge_cb, this, std::placeholders::_1, std::placeholders::_2));
            } else {
                if (status_cookie.first) {
                    LogPrintf("tor: Authentication cookie %s is not exactly %i bytes, as is required by the spec\n", cookiefile, TOR_COOKIE_SIZE);
                } else {
                    LogPrintf("tor: Authentication cookie %s could not be opened (check permissions)\n", cookiefile);
                }
            }
        } else if (methods.count("HASHEDPASSWORD")) {
            LogPrintf("tor: The only supported authentication mechanism left is password, but no password provided with -torpassword\n");
        } else {
            LogPrintf("tor: No supported authentication method\n");
        }
    } else {
        LogPrintf("tor: Requesting protocol info failed\n");
    }
}

void TorController::connected_cb(TorControlConnection& _conn)
{
    reconnect_timeout = RECONNECT_TIMEOUT_START;
    // First send a PROTOCOLINFO command to figure out what authentication is expected
    if (!_conn.Command("PROTOCOLINFO 1", std::bind(&TorController::protocolinfo_cb, this, std::placeholders::_1, std::placeholders::_2)))
        LogPrintf("tor: Error sending initial protocolinfo command\n");
}

void TorController::disconnected_cb(TorControlConnection& _conn)
{
    // Stop advertising service when disconnected
    if (service.IsValid())
        RemoveLocal(service);
    service = CService();
    if (!reconnect)
        return;

    LogPrint(BCLog::TOR, "tor: Not connected to Tor control port %s, trying to reconnect\n", m_tor_control_center);

    // Single-shot timer for reconnect. Use exponential backoff.
    struct timeval time = MillisToTimeval(int64_t(reconnect_timeout * 1000.0));
    if (reconnect_ev)
        event_add(reconnect_ev, &time);
    reconnect_timeout *= RECONNECT_TIMEOUT_EXP;
}

void TorController::Reconnect()
{
    /* Try to reconnect and reestablish if we get booted - for example, Tor
     * may be restarting.
     */
    if (!conn.Connect(m_tor_control_center, std::bind(&TorController::connected_cb, this, std::placeholders::_1),
         std::bind(&TorController::disconnected_cb, this, std::placeholders::_1) )) {
        LogPrintf("tor: Re-initiating connection to Tor control port %s failed\n", m_tor_control_center);
    }
}

fs::path TorController::GetPrivateKeyFile()
{
    return gArgs.GetDataDirNet() / "onion_v3_private_key";
}

void TorController::reconnect_cb(evutil_socket_t fd, short what, void *arg)
{
    TorController *self = static_cast<TorController*>(arg);
    self->Reconnect();
}

/****** Thread ********/
static struct event_base *gBase;
static std::thread torControlThread;

static void TorControlThread(CService onion_service_target)
{
    SetSyscallSandboxPolicy(SyscallSandboxPolicy::TOR_CONTROL);
    TorController ctrl(gBase, gArgs.GetArg("-torcontrol", DEFAULT_TOR_CONTROL), onion_service_target);

    event_base_dispatch(gBase);
}

void StartTorControl(CService onion_service_target)
{
    assert(!gBase);
#ifdef WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif
    gBase = event_base_new();
    if (!gBase) {
        LogPrintf("tor: Unable to create event_base\n");
        return;
    }

    torControlThread = std::thread(&util::TraceThread, "torcontrol", [onion_service_target] {
        TorControlThread(onion_service_target);
    });
}

void InterruptTorControl()
{
    if (gBase) {
        LogPrintf("tor: Thread interrupt\n");
        event_base_once(gBase, -1, EV_TIMEOUT, [](evutil_socket_t, short, void*) {
            event_base_loopbreak(gBase);
        }, nullptr, nullptr);
    }
}

void StopTorControl()
{
    if (gBase) {
        torControlThread.join();
        event_base_free(gBase);
        gBase = nullptr;
    }
}

CService DefaultOnionServiceTarget()
{
    struct in_addr onion_service_target;
    onion_service_target.s_addr = htonl(INADDR_LOOPBACK);
    return {onion_service_target, BaseParams().OnionServiceTargetPort()};
}
