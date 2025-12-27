// Copyright (c) 2015-present The Bitcoin Core developers
// Copyright (c) 2017 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <torcontrol.h>

#include <chainparams.h>
#include <chainparamsbase.h>
#include <common/args.h>
#include <compat/compat.h>
#include <crypto/hmac_sha256.h>
#include <logging.h>
#include <net.h>
#include <netaddress.h>
#include <netbase.h>
#include <random.h>
#include <tinyformat.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/readwritefile.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/thread.h>
#include <util/time.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <thread>
#include <utility>
#include <vector>

using util::ReplaceAll;
using util::SplitString;
using util::ToString;

/** Default control ip and port */
const std::string DEFAULT_TOR_CONTROL = "127.0.0.1:" + ToString(DEFAULT_TOR_CONTROL_PORT);
/** Tor cookie size (from control-spec.txt) */
constexpr int TOR_COOKIE_SIZE = 32;
/** Size of client/server nonce for SAFECOOKIE */
constexpr int TOR_NONCE_SIZE = 32;
/** Tor control reply code. Ref: https://spec.torproject.org/control-spec/replies.html */
constexpr int TOR_REPLY_OK = 250;
constexpr int TOR_REPLY_UNRECOGNIZED = 510;
/** For computing server_hash in SAFECOOKIE */
static const std::string TOR_SAFE_SERVERKEY = "Tor safe cookie authentication server-to-controller hash";
/** For computing clientHash in SAFECOOKIE */
static const std::string TOR_SAFE_CLIENTKEY = "Tor safe cookie authentication controller-to-server hash";
/** Exponential backoff configuration - initial timeout in seconds */
constexpr float RECONNECT_TIMEOUT_START = 1.0;
/** Exponential backoff configuration - growth factor */
constexpr float RECONNECT_TIMEOUT_EXP = 1.5;
/** Maximum reconnect timeout in seconds to prevent excessive delays */
constexpr float RECONNECT_TIMEOUT_MAX = 600.0;
/** Maximum length for lines received on TorControlConnection.
 * tor-control-spec.txt mentions that there is explicitly no limit defined to line length,
 * this is belt-and-suspenders sanity limit to prevent memory exhaustion.
 */
constexpr int MAX_LINE_LENGTH = 100000;
/** Timeout for socket operations */
constexpr auto SOCKET_SEND_TIMEOUT = 10s;

/****** Low-level TorControlConnection ********/

TorControlConnection::TorControlConnection(CThreadInterrupt& interrupt)
    : m_interrupt(interrupt)
{
}

TorControlConnection::~TorControlConnection()
{
    Disconnect();
}

bool TorControlConnection::Connect(const std::string& tor_control_center)
{
    if (m_sock) {
        Disconnect();
    }

    const std::optional<CService> control_service{Lookup(tor_control_center, DEFAULT_TOR_CONTROL_PORT, fNameLookup)};
    if (!control_service.has_value()) {
        LogWarning("tor: Failed to look up control center %s", tor_control_center);
        return false;
    }

    struct sockaddr_storage control_address;
    socklen_t control_address_len = sizeof(control_address);
    if (!control_service.value().GetSockAddr(reinterpret_cast<struct sockaddr*>(&control_address), &control_address_len)) {
        LogWarning("tor: Error parsing socket address %s", tor_control_center);
        return false;
    }

    // Create a new socket
    SOCKET s = socket(control_service->GetSAFamily(), SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        LogWarning("tor: Failed to create socket: %s", NetworkErrorString(WSAGetLastError()));
        return false;
    }

    m_sock = std::make_unique<Sock>(s);

    // Connect to Tor control port
    if (m_sock->Connect(reinterpret_cast<struct sockaddr*>(&control_address), control_address_len) == SOCKET_ERROR) {
        LogWarning("tor: Error connecting to address %s: %s", tor_control_center, NetworkErrorString(WSAGetLastError()));
        m_sock.reset();
        return false;
    }

    // Set non-blocking mode on socket
    if (!m_sock->SetNonBlocking()) {
        LogWarning("tor: Failed to set non-blocking mode: %s", NetworkErrorString(WSAGetLastError()));
        m_sock.reset();
        return false;
    }

    m_recv_buffer.clear();
    m_message.Clear();
    m_reply_handlers.clear();

    LogDebug(BCLog::TOR, "Successfully connected to Tor control port");
    return true;
}

void TorControlConnection::Disconnect()
{
    m_sock.reset();
    m_recv_buffer.clear();
    m_message.Clear();
    m_reply_handlers.clear();
}

bool TorControlConnection::IsConnected() const
{
    if (!m_sock) return false;
    std::string errmsg;
    return m_sock->IsConnected(errmsg);
}

bool TorControlConnection::WaitForData(std::chrono::milliseconds timeout) const
{
    if (!m_sock) return false;

    Sock::Event event{0};
    if (!m_sock->Wait(timeout, Sock::RECV, &event)) {
        return false;
    }
    if (event & Sock::ERR) {
        return false;
    }

    return true;
}

bool TorControlConnection::ReceiveAndProcess()
{
    if (!m_sock) return false;

    std::byte buf[4096];
    ssize_t nread = m_sock->Recv(buf, sizeof(buf), MSG_DONTWAIT);

    if (nread < 0) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAEINTR || err == WSAEINPROGRESS) {
            // No data available currently
            return true;
        }
        LogWarning("tor: Error reading from socket: %s", NetworkErrorString(err));
        return false;
    }

    if (nread == 0) {
        LogDebug(BCLog::TOR, "End of stream");
        return false;
    }

    m_recv_buffer.insert(m_recv_buffer.end(), buf, buf + nread);
    return ProcessBuffer();
}

bool TorControlConnection::ProcessBuffer()
{
    util::LineReader reader(m_recv_buffer, MAX_LINE_LENGTH);
    auto start = reader.it;

    while (auto line = reader.ReadLine()) {
        // Skip short lines
        if (line->size() < 4) continue;

        // Parse: <code><separator><data>
        // <status>(-|+| )<data>
        m_message.code = ToIntegral<int>(line->substr(0, 3)).value_or(0);
        m_message.lines.push_back(line->substr(4));
        char separator = (*line)[3]; // '-', '+', or ' '

        if (separator == ' ') {
            if (m_message.code >= 600) {
                // Async notifications are currently unused
                // Synchronous and asynchronous messages are never interleaved
                LogDebug(BCLog::TOR, "Received async notification %i", m_message.code);
            } else if (!m_reply_handlers.empty()) {
                // Invoke reply handler with message
                m_reply_handlers.front()(*this, m_message);
                m_reply_handlers.pop_front();
            } else {
                LogDebug(BCLog::TOR, "Received unexpected sync reply %i", m_message.code);
            }
            m_message.Clear();
        }
    }

    m_recv_buffer.erase(m_recv_buffer.begin(), m_recv_buffer.begin() + std::distance(start, reader.it));
    return true;
}

bool TorControlConnection::Command(const std::string &cmd, const ReplyHandlerCB& reply_handler)
{
    if (!m_sock) return false;

    std::string command = cmd + "\r\n";
    try {
        m_sock->SendComplete(std::span<const char>{command}, SOCKET_SEND_TIMEOUT, m_interrupt);
    } catch (const std::runtime_error& e) {
        LogWarning("tor: Error sending command: %s", e.what());
        return false;
    }

    m_reply_handlers.push_back(reply_handler);
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
             *   For future-proofing, controller implementers MAY use the following
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
                        const auto end{i + j};
                        uint8_t val{0};
                        while (i < end) {
                            val *= 8;
                            val += value[i++] - '0';
                        }
                        escaped_value.push_back(char(val));
                        // Account for automatic incrementing at loop end
                        --i;
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

TorController::TorController(const std::string& tor_control_center, const CService& target)
    : m_tor_control_center(tor_control_center),
      m_conn(m_interrupt),
      m_reconnect(true),
      m_reconnect_timeout(RECONNECT_TIMEOUT_START),
      m_target(target)
{
    // Read service private key if cached
    std::pair<bool,std::string> pkf = ReadBinaryFile(GetPrivateKeyFile());
    if (pkf.first) {
        LogDebug(BCLog::TOR, "Reading cached private key from %s", fs::PathToString(GetPrivateKeyFile()));
        m_private_key = pkf.second;
    }
    m_thread = std::thread(&util::TraceThread, "torcontrol", [this] { ThreadControl(); });
}

TorController::~TorController()
{
    Interrupt();
    Join();
    if (m_service.IsValid()) {
        RemoveLocal(m_service);
    }
}

void TorController::Interrupt()
{
    m_reconnect = false;
    m_interrupt();
}

void TorController::Join()
{
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void TorController::ThreadControl()
{
    LogDebug(BCLog::TOR, "Entering Tor control thread");

    while (!m_interrupt) {
        // Try to connect if not connected already
        if (!m_conn.IsConnected()) {
            LogDebug(BCLog::TOR, "Attempting to connect to Tor control port %s", m_tor_control_center);

            if (!m_conn.Connect(m_tor_control_center)) {
                LogWarning("tor: Initiating connection to Tor control port %s failed", m_tor_control_center);
                if (!m_reconnect) {
                    break;
                }
                // Wait before retrying with exponential backoff
                LogDebug(BCLog::TOR, "Retrying in %.1f seconds", m_reconnect_timeout);
                if (!m_interrupt.sleep_for(std::chrono::milliseconds(int64_t(m_reconnect_timeout * 1000.0)))) {
                    break;
                }
                m_reconnect_timeout = std::min(m_reconnect_timeout * RECONNECT_TIMEOUT_EXP, RECONNECT_TIMEOUT_MAX);
                continue;
            }
            // Successfully connected, reset timeout and trigger connected callback
            m_reconnect_timeout = RECONNECT_TIMEOUT_START;
            connected_cb(m_conn);
        }
        // Wait for data with a timeout
        if (!m_conn.WaitForData(std::chrono::seconds(1))) {
            // Check if still connected
            if (!m_conn.IsConnected()) {
                LogDebug(BCLog::TOR, "Lost connection to Tor control port");
                disconnected_cb(m_conn);
                continue;
            }
            // Just a timeout, continue waiting
            continue;
        }
        // Process incoming data
        if (!m_conn.ReceiveAndProcess()) {
            LogDebug(BCLog::TOR, "Error processing data from Tor control port");
            disconnected_cb(m_conn);
        }
    }
    LogDebug(BCLog::TOR, "Exited Tor control thread");
}

void TorController::get_socks_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    // NOTE: We can only get here if -onion is unset
    std::string socks_location;
    if (reply.code == TOR_REPLY_OK) {
        for (const auto& line : reply.lines) {
            if (line.starts_with("net/listeners/socks=")) {
                const std::string port_list_str = line.substr(20);
                std::vector<std::string> port_list = SplitString(port_list_str, ' ');

                for (auto& portstr : port_list) {
                    if (portstr.empty()) continue;
                    if ((portstr[0] == '"' || portstr[0] == '\'') && portstr.size() >= 2 && (*portstr.rbegin() == portstr[0])) {
                        portstr = portstr.substr(1, portstr.size() - 2);
                        if (portstr.empty()) continue;
                    }
                    socks_location = portstr;
                    if (portstr.starts_with("127.0.0.1:")) {
                        // Prefer localhost - ignore other ports
                        break;
                    }
                }
            }
        }
        if (!socks_location.empty()) {
            LogDebug(BCLog::TOR, "Get SOCKS port command yielded %s", socks_location);
        } else {
            LogWarning("tor: Get SOCKS port command returned nothing");
        }
    } else if (reply.code == TOR_REPLY_UNRECOGNIZED) {
        LogWarning("tor: Get SOCKS port command failed with unrecognized command (You probably should upgrade Tor)");
    } else {
        LogWarning("tor: Get SOCKS port command failed; error code %d", reply.code);
    }

    CService resolved;
    Assume(!resolved.IsValid());
    if (!socks_location.empty()) {
        resolved = LookupNumeric(socks_location, DEFAULT_TOR_SOCKS_PORT);
    }
    if (!resolved.IsValid()) {
        // Fallback to old behaviour
        resolved = LookupNumeric("127.0.0.1", DEFAULT_TOR_SOCKS_PORT);
    }

    Assume(resolved.IsValid());
    LogDebug(BCLog::TOR, "Configuring onion proxy for %s", resolved.ToStringAddrPort());

    // Add Tor as proxy for .onion addresses.
    // Enable stream isolation to prevent connection correlation and enhance privacy, by forcing a different Tor circuit for every connection.
    // For this to work, the IsolateSOCKSAuth flag must be enabled on SOCKSPort (which is the default, see the IsolateSOCKSAuth section of Tor's manual page).
    Proxy addrOnion = Proxy(resolved, /*tor_stream_isolation=*/ true);
    SetProxy(NET_ONION, addrOnion);

    const auto onlynets = gArgs.GetArgs("-onlynet");

    const bool onion_allowed_by_onlynet{
        onlynets.empty() ||
        std::any_of(onlynets.begin(), onlynets.end(), [](const auto& n) {
            return ParseNetwork(n) == NET_ONION;
        })};

    if (onion_allowed_by_onlynet) {
        // If NET_ONION is reachable, then the below is a noop.
        //
        // If NET_ONION is not reachable, then none of -proxy or -onion was given.
        // Since we are here, then -torcontrol and -torpassword were given.
        g_reachable_nets.Add(NET_ONION);
    }
}

void TorController::add_onion_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    if (reply.code == TOR_REPLY_OK) {
        LogDebug(BCLog::TOR, "ADD_ONION successful");
        for (const std::string &s : reply.lines) {
            std::map<std::string,std::string> m = ParseTorReplyMapping(s);
            std::map<std::string,std::string>::iterator i;
            if ((i = m.find("ServiceID")) != m.end())
                m_service_id = i->second;
            if ((i = m.find("PrivateKey")) != m.end())
                m_private_key = i->second;
        }
        if (m_service_id.empty()) {
            LogWarning("tor: Error parsing ADD_ONION parameters:");
            for (const std::string &s : reply.lines) {
                LogWarning("    %s", SanitizeString(s));
            }
            return;
        }
        m_service = LookupNumeric(std::string(m_service_id+".onion"), Params().GetDefaultPort());
        LogInfo("Got tor service ID %s, advertising service %s", m_service_id, m_service.ToStringAddrPort());
        if (WriteBinaryFile(GetPrivateKeyFile(), m_private_key)) {
            LogDebug(BCLog::TOR, "Cached service private key to %s", fs::PathToString(GetPrivateKeyFile()));
        } else {
            LogWarning("tor: Error writing service private key to %s", fs::PathToString(GetPrivateKeyFile()));
        }
        AddLocal(m_service, LOCAL_MANUAL);
        // ... onion requested - keep connection open
    } else if (reply.code == TOR_REPLY_UNRECOGNIZED) {
        LogWarning("tor: Add onion failed with unrecognized command (You probably need to upgrade Tor)");
    } else {
        LogWarning("tor: Add onion failed; error code %d", reply.code);
    }
}

void TorController::auth_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    if (reply.code == TOR_REPLY_OK) {
        LogDebug(BCLog::TOR, "Authentication successful");

        // Now that we know Tor is running setup the proxy for onion addresses
        // if -onion isn't set to something else.
        if (gArgs.GetArg("-onion", "") == "") {
            _conn.Command("GETINFO net/listeners/socks", std::bind_front(&TorController::get_socks_cb, this));
        }

        // Finally - now create the service
        if (m_private_key.empty()) { // No private key, generate one
            m_private_key = "NEW:ED25519-V3"; // Explicitly request key type - see issue #9214
        }
        // Request onion service, redirect port.
        // Note that the 'virtual' port is always the default port to avoid decloaking nodes using other ports.
        _conn.Command(strprintf("ADD_ONION %s Port=%i,%s", m_private_key, Params().GetDefaultPort(), m_target.ToStringAddrPort()),
            std::bind_front(&TorController::add_onion_cb, this));
    } else {
        LogWarning("tor: Authentication failed");
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
static std::vector<uint8_t> ComputeResponse(const std::string_view &key, const std::vector<uint8_t> &cookie,  const std::vector<uint8_t> &client_nonce, const std::vector<uint8_t> &server_nonce)
{
    CHMAC_SHA256 computeHash((const uint8_t*)key.data(), key.size());
    std::vector<uint8_t> computedHash(CHMAC_SHA256::OUTPUT_SIZE, 0);
    computeHash.Write(cookie.data(), cookie.size());
    computeHash.Write(client_nonce.data(), client_nonce.size());
    computeHash.Write(server_nonce.data(), server_nonce.size());
    computeHash.Finalize(computedHash.data());
    return computedHash;
}

void TorController::authchallenge_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    if (reply.code == TOR_REPLY_OK) {
        LogDebug(BCLog::TOR, "SAFECOOKIE authentication challenge successful");
        std::pair<std::string,std::string> l = SplitTorReplyLine(reply.lines[0]);
        if (l.first == "AUTHCHALLENGE") {
            std::map<std::string,std::string> m = ParseTorReplyMapping(l.second);
            if (m.empty()) {
                LogWarning("tor: Error parsing AUTHCHALLENGE parameters: %s", SanitizeString(l.second));
                return;
            }
            std::vector<uint8_t> server_hash = ParseHex(m["SERVERHASH"]);
            std::vector<uint8_t> server_nonce = ParseHex(m["SERVERNONCE"]);
            LogDebug(BCLog::TOR, "AUTHCHALLENGE ServerHash %s ServerNonce %s", HexStr(server_hash), HexStr(server_nonce));
            if (server_nonce.size() != 32) {
                LogWarning("tor: ServerNonce is not 32 bytes, as required by spec");
                return;
            }

            std::vector<uint8_t> computed_server_hash = ComputeResponse(TOR_SAFE_SERVERKEY, m_cookie, m_client_nonce, server_nonce);
            if (computed_server_hash != server_hash) {
                LogWarning("tor: ServerHash %s does not match expected ServerHash %s", HexStr(server_hash), HexStr(computed_server_hash));
                return;
            }

            std::vector<uint8_t> computedClientHash = ComputeResponse(TOR_SAFE_CLIENTKEY, m_cookie, m_client_nonce, server_nonce);
            _conn.Command("AUTHENTICATE " + HexStr(computedClientHash), std::bind_front(&TorController::auth_cb, this));
        } else {
            LogWarning("tor: Invalid reply to AUTHCHALLENGE");
        }
    } else {
        LogWarning("tor: SAFECOOKIE authentication challenge failed");
    }
}

void TorController::protocolinfo_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    if (reply.code == TOR_REPLY_OK) {
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
                if ((i = m.find("METHODS")) != m.end()) {
                    std::vector<std::string> m_vec = SplitString(i->second, ',');
                    methods = std::set<std::string>(m_vec.begin(), m_vec.end());
                }
                if ((i = m.find("COOKIEFILE")) != m.end())
                    cookiefile = i->second;
            } else if (l.first == "VERSION") {
                std::map<std::string,std::string> m = ParseTorReplyMapping(l.second);
                std::map<std::string,std::string>::iterator i;
                if ((i = m.find("Tor")) != m.end()) {
                    LogDebug(BCLog::TOR, "Connected to Tor version %s", i->second);
                }
            }
        }
        for (const std::string &s : methods) {
            LogDebug(BCLog::TOR, "Supported authentication method: %s", s);
        }
        // Prefer NULL, otherwise SAFECOOKIE. If a password is provided, use HASHEDPASSWORD
        /* Authentication:
         *   cookie:   hex-encoded ~/.tor/control_auth_cookie
         *   password: "password"
         */
        std::string torpassword = gArgs.GetArg("-torpassword", "");
        if (!torpassword.empty()) {
            if (methods.contains("HASHEDPASSWORD")) {
                LogDebug(BCLog::TOR, "Using HASHEDPASSWORD authentication");
                ReplaceAll(torpassword, "\"", "\\\"");
                _conn.Command("AUTHENTICATE \"" + torpassword + "\"", std::bind_front(&TorController::auth_cb, this));
            } else {
                LogWarning("tor: Password provided with -torpassword, but HASHEDPASSWORD authentication is not available");
            }
        } else if (methods.contains("NULL")) {
            LogDebug(BCLog::TOR, "Using NULL authentication");
            _conn.Command("AUTHENTICATE", std::bind_front(&TorController::auth_cb, this));
        } else if (methods.contains("SAFECOOKIE")) {
            // Cookie: hexdump -e '32/1 "%02x""\n"'  ~/.tor/control_auth_cookie
            LogDebug(BCLog::TOR, "Using SAFECOOKIE authentication, reading cookie authentication from %s", cookiefile);
            std::pair<bool,std::string> status_cookie = ReadBinaryFile(fs::PathFromString(cookiefile), TOR_COOKIE_SIZE);
            if (status_cookie.first && status_cookie.second.size() == TOR_COOKIE_SIZE) {
                // _conn.Command("AUTHENTICATE " + HexStr(status_cookie.second), std::bind_front(&TorController::auth_cb, this));
                m_cookie = std::vector<uint8_t>(status_cookie.second.begin(), status_cookie.second.end());
                m_client_nonce = std::vector<uint8_t>(TOR_NONCE_SIZE, 0);
                GetRandBytes(m_client_nonce);
                _conn.Command("AUTHCHALLENGE SAFECOOKIE " + HexStr(m_client_nonce), std::bind_front(&TorController::authchallenge_cb, this));
            } else {
                if (status_cookie.first) {
                    LogWarning("tor: Authentication cookie %s is not exactly %i bytes, as is required by the spec", cookiefile, TOR_COOKIE_SIZE);
                } else {
                    LogWarning("tor: Authentication cookie %s could not be opened (check permissions)", cookiefile);
                }
            }
        } else if (methods.contains("HASHEDPASSWORD")) {
            LogWarning("tor: The only supported authentication mechanism left is password, but no password provided with -torpassword");
        } else {
            LogWarning("tor: No supported authentication method");
        }
    } else {
        LogWarning("tor: Requesting protocol info failed");
    }
}

void TorController::connected_cb(TorControlConnection& _conn)
{
    m_reconnect_timeout = RECONNECT_TIMEOUT_START;
    // First send a PROTOCOLINFO command to figure out what authentication is expected
    if (!_conn.Command("PROTOCOLINFO 1", std::bind_front(&TorController::protocolinfo_cb, this)))
        LogWarning("tor: Error sending initial protocolinfo command");
}

void TorController::disconnected_cb(TorControlConnection& _conn)
{
    // Stop advertising service when disconnected
    if (m_service.IsValid())
        RemoveLocal(m_service);
    m_service = CService();
    if (!m_reconnect)
        return;

    LogDebug(BCLog::TOR, "Not connected to Tor control port %s, will retry", m_tor_control_center);
    _conn.Disconnect();
}

fs::path TorController::GetPrivateKeyFile()
{
    return gArgs.GetDataDirNet() / "onion_v3_private_key";
}

/****** Thread ********/

/**
 * TODO: TBD if introducing a global is the preferred approach here since we
 * usually try to avoid them. We could let init manage the lifecycle or make
 * this a part of NodeContext maybe instead.
 */
static std::unique_ptr<TorController> g_tor_controller;

void StartTorControl(CService onion_service_target)
{
    assert(!g_tor_controller);
    g_tor_controller = std::make_unique<TorController>(gArgs.GetArg("-torcontrol", DEFAULT_TOR_CONTROL), onion_service_target);
}

void InterruptTorControl()
{
    if (!g_tor_controller) return;
    LogInfo("tor: Thread interrupt");
    g_tor_controller->Interrupt();
}

void StopTorControl()
{
    if (!g_tor_controller) return;
    g_tor_controller->Join();
    g_tor_controller.reset();
}

CService DefaultOnionServiceTarget(uint16_t port)
{
    struct in_addr onion_service_target;
    onion_service_target.s_addr = htonl(INADDR_LOOPBACK);
    return {onion_service_target, port};
}
