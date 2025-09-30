// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/args.h>
#include <compat/compat.h>
#include <compat/endian.h>
#include <crypto/sha256.h>
#include <i2p.h>
#include <logging.h>
#include <netaddress.h>
#include <netbase.h>
#include <random.h>
#include <script/parsing.h>
#include <sync.h>
#include <tinyformat.h>
#include <util/fs.h>
#include <util/readwritefile.h>
#include <util/sock.h>
#include <util/strencodings.h>
#include <util/threadinterrupt.h>

#include <chrono>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>

using util::Split;

namespace i2p {

/**
 * Swap Standard Base64 <-> I2P Base64.
 * Standard Base64 uses `+` and `/` as last two characters of its alphabet.
 * I2P Base64 uses `-` and `~` respectively.
 * So it is easy to detect in which one is the input and convert to the other.
 * @param[in] from Input to convert.
 * @return converted `from`
 */
static std::string SwapBase64(const std::string& from)
{
    std::string to;
    to.resize(from.size());
    for (size_t i = 0; i < from.size(); ++i) {
        switch (from[i]) {
        case '-':
            to[i] = '+';
            break;
        case '~':
            to[i] = '/';
            break;
        case '+':
            to[i] = '-';
            break;
        case '/':
            to[i] = '~';
            break;
        default:
            to[i] = from[i];
            break;
        }
    }
    return to;
}

/**
 * Decode an I2P-style Base64 string.
 * @param[in] i2p_b64 I2P-style Base64 string.
 * @return decoded `i2p_b64`
 * @throw std::runtime_error if decoding fails
 */
static Binary DecodeI2PBase64(const std::string& i2p_b64)
{
    const std::string& std_b64 = SwapBase64(i2p_b64);
    auto decoded = DecodeBase64(std_b64);
    if (!decoded) {
        throw std::runtime_error(strprintf("Cannot decode Base64: \"%s\"", i2p_b64));
    }
    return std::move(*decoded);
}

/**
 * Derive the .b32.i2p address of an I2P destination (binary).
 * @param[in] dest I2P destination.
 * @return the address that corresponds to `dest`
 * @throw std::runtime_error if conversion fails
 */
static CNetAddr DestBinToAddr(const Binary& dest)
{
    CSHA256 hasher;
    hasher.Write(dest.data(), dest.size());
    unsigned char hash[CSHA256::OUTPUT_SIZE];
    hasher.Finalize(hash);

    CNetAddr addr;
    const std::string addr_str = EncodeBase32(hash, false) + ".b32.i2p";
    if (!addr.SetSpecial(addr_str)) {
        throw std::runtime_error(strprintf("Cannot parse I2P address: \"%s\"", addr_str));
    }

    return addr;
}

/**
 * Derive the .b32.i2p address of an I2P destination (I2P-style Base64).
 * @param[in] dest I2P destination.
 * @return the address that corresponds to `dest`
 * @throw std::runtime_error if conversion fails
 */
static CNetAddr DestB64ToAddr(const std::string& dest)
{
    const Binary& decoded = DecodeI2PBase64(dest);
    return DestBinToAddr(decoded);
}

namespace sam {

Session::Session(const fs::path& private_key_file,
                 const Proxy& control_host,
                 std::shared_ptr<CThreadInterrupt> interrupt)
    : m_private_key_file{private_key_file},
      m_control_host{control_host},
      m_interrupt{interrupt},
      m_transient{false}
{
}

Session::Session(const Proxy& control_host, std::shared_ptr<CThreadInterrupt> interrupt)
    : m_control_host{control_host},
      m_interrupt{interrupt},
      m_transient{true}
{
}

Session::~Session()
{
    LOCK(m_mutex);
    Disconnect();
}

bool Session::Listen(Connection& conn)
{
    try {
        LOCK(m_mutex);
        CreateIfNotCreatedAlready();
        conn.me = m_my_addr;
        conn.sock = StreamAccept();
        return true;
    } catch (const std::runtime_error& e) {
        LogPrintLevel(BCLog::I2P, BCLog::Level::Error, "Couldn't listen: %s\n", e.what());
        CheckControlSock();
    }
    return false;
}

bool Session::Accept(Connection& conn)
{
    AssertLockNotHeld(m_mutex);

    std::string errmsg;
    bool disconnect{false};

    while (!m_interrupt->interrupted()) {
        Sock::Event occurred;
        if (!conn.sock->Wait(MAX_WAIT_FOR_IO, Sock::RECV, &occurred)) {
            errmsg = "wait on socket failed";
            break;
        }

        if (occurred == 0) {
            // Timeout, no incoming connections or errors within MAX_WAIT_FOR_IO.
            continue;
        }

        std::string peer_dest;
        try {
            peer_dest = conn.sock->RecvUntilTerminator('\n', MAX_WAIT_FOR_IO, *m_interrupt, MAX_MSG_SIZE);
        } catch (const std::runtime_error& e) {
            errmsg = e.what();
            break;
        }

        CNetAddr peer_addr;
        try {
            peer_addr = DestB64ToAddr(peer_dest);
        } catch (const std::runtime_error& e) {
            // The I2P router is expected to send the Base64 of the connecting peer,
            // but it may happen that something like this is sent instead:
            // STREAM STATUS RESULT=I2P_ERROR MESSAGE="Session was closed"
            // In that case consider the session damaged and close it right away,
            // even if the control socket is alive.
            if (peer_dest.find("RESULT=I2P_ERROR") != std::string::npos) {
                errmsg = strprintf("unexpected reply that hints the session is unusable: %s", peer_dest);
                disconnect = true;
            } else {
                errmsg = e.what();
            }
            break;
        }

        conn.peer = CService(peer_addr, I2P_SAM31_PORT);

        return true;
    }

    if (m_interrupt->interrupted()) {
        LogPrintLevel(BCLog::I2P, BCLog::Level::Debug, "Accept was interrupted\n");
    } else {
        LogPrintLevel(BCLog::I2P, BCLog::Level::Debug, "Error accepting%s: %s\n", disconnect ? " (will close the session)" : "", errmsg);
    }
    if (disconnect) {
        LOCK(m_mutex);
        Disconnect();
    } else {
        CheckControlSock();
    }
    return false;
}

bool Session::Connect(const CService& to, Connection& conn, bool& proxy_error)
{
    // Refuse connecting to arbitrary ports. We don't specify any destination port to the SAM proxy
    // when connecting (SAM 3.1 does not use ports) and it forces/defaults it to I2P_SAM31_PORT.
    if (to.GetPort() != I2P_SAM31_PORT) {
        LogPrintLevel(BCLog::I2P, BCLog::Level::Debug, "Error connecting to %s, connection refused due to arbitrary port %s\n", to.ToStringAddrPort(), to.GetPort());
        proxy_error = false;
        return false;
    }

    proxy_error = true;

    std::string session_id;
    std::unique_ptr<Sock> sock;
    conn.peer = to;

    try {
        {
            LOCK(m_mutex);
            CreateIfNotCreatedAlready();
            session_id = m_session_id;
            conn.me = m_my_addr;
            sock = Hello();
        }

        const Reply& lookup_reply =
            SendRequestAndGetReply(*sock, strprintf("NAMING LOOKUP NAME=%s", to.ToStringAddr()));

        const std::string& dest = lookup_reply.Get("VALUE");

        const Reply& connect_reply = SendRequestAndGetReply(
            *sock, strprintf("STREAM CONNECT ID=%s DESTINATION=%s SILENT=false", session_id, dest),
            false);

        const std::string& result = connect_reply.Get("RESULT");

        if (result == "OK") {
            conn.sock = std::move(sock);
            return true;
        }

        if (result == "INVALID_ID") {
            LOCK(m_mutex);
            Disconnect();
            throw std::runtime_error("Invalid session id");
        }

        if (result == "CANT_REACH_PEER" || result == "TIMEOUT") {
            proxy_error = false;
        }

        throw std::runtime_error(strprintf("\"%s\"", connect_reply.full));
    } catch (const std::runtime_error& e) {
        LogPrintLevel(BCLog::I2P, BCLog::Level::Debug, "Error connecting to %s: %s\n", to.ToStringAddrPort(), e.what());
        CheckControlSock();
        return false;
    }
}

// Private methods

std::string Session::Reply::Get(const std::string& key) const
{
    const auto& pos = keys.find(key);
    if (pos == keys.end() || !pos->second.has_value()) {
        throw std::runtime_error(
            strprintf("Missing %s= in the reply to \"%s\": \"%s\"", key, request, full));
    }
    return pos->second.value();
}

Session::Reply Session::SendRequestAndGetReply(const Sock& sock,
                                               const std::string& request,
                                               bool check_result_ok) const
{
    sock.SendComplete(request + "\n", MAX_WAIT_FOR_IO, *m_interrupt);

    Reply reply;

    // Don't log the full "SESSION CREATE ..." because it contains our private key.
    reply.request = request.starts_with("SESSION CREATE") ? "SESSION CREATE ..." : request;

    // It could take a few minutes for the I2P router to reply as it is querying the I2P network
    // (when doing name lookup, for example). Notice: `RecvUntilTerminator()` is checking
    // `m_interrupt` more often, so we would not be stuck here for long if `m_interrupt` is
    // signaled.
    static constexpr auto recv_timeout = 3min;

    reply.full = sock.RecvUntilTerminator('\n', recv_timeout, *m_interrupt, MAX_MSG_SIZE);

    for (const auto& kv : Split(reply.full, ' ')) {
        const auto pos{std::ranges::find(kv, '=')};
        if (pos != kv.end()) {
            reply.keys.emplace(std::string{kv.begin(), pos}, std::string{pos + 1, kv.end()});
        } else {
            reply.keys.emplace(std::string{kv.begin(), kv.end()}, std::nullopt);
        }
    }

    if (check_result_ok && reply.Get("RESULT") != "OK") {
        throw std::runtime_error(
            strprintf("Unexpected reply to \"%s\": \"%s\"", request, reply.full));
    }

    return reply;
}

std::unique_ptr<Sock> Session::Hello() const
{
    auto sock = m_control_host.Connect();

    if (!sock) {
        throw std::runtime_error(strprintf("Cannot connect to %s", m_control_host.ToString()));
    }

    SendRequestAndGetReply(*sock, "HELLO VERSION MIN=3.1 MAX=3.1");

    return sock;
}

void Session::CheckControlSock()
{
    LOCK(m_mutex);

    std::string errmsg;
    if (m_control_sock && !m_control_sock->IsConnected(errmsg)) {
        LogPrintLevel(BCLog::I2P, BCLog::Level::Debug, "Control socket error: %s\n", errmsg);
        Disconnect();
    }
}

void Session::DestGenerate(const Sock& sock)
{
    // https://geti2p.net/spec/common-structures#key-certificates
    // "7" or "EdDSA_SHA512_Ed25519" - "Recent Router Identities and Destinations".
    // Use "7" because i2pd <2.24.0 does not recognize the textual form.
    // If SIGNATURE_TYPE is not specified, then the default one is DSA_SHA1.
    const Reply& reply = SendRequestAndGetReply(sock, "DEST GENERATE SIGNATURE_TYPE=7", false);

    m_private_key = DecodeI2PBase64(reply.Get("PRIV"));
}

void Session::GenerateAndSavePrivateKey(const Sock& sock)
{
    DestGenerate(sock);

    // umask is set to 0077 in common/system.cpp, which is ok.
    if (!WriteBinaryFile(m_private_key_file,
                         std::string(m_private_key.begin(), m_private_key.end()))) {
        throw std::runtime_error(
            strprintf("Cannot save I2P private key to %s", fs::quoted(fs::PathToString(m_private_key_file))));
    }
}

Binary Session::MyDestination() const
{
    // From https://geti2p.net/spec/common-structures#destination:
    // "They are 387 bytes plus the certificate length specified at bytes 385-386, which may be
    // non-zero"
    static constexpr size_t DEST_LEN_BASE = 387;
    static constexpr size_t CERT_LEN_POS = 385;

    uint16_t cert_len;

    if (m_private_key.size() < CERT_LEN_POS + sizeof(cert_len)) {
        throw std::runtime_error(strprintf("The private key is too short (%d < %d)",
                                           m_private_key.size(),
                                           CERT_LEN_POS + sizeof(cert_len)));
    }

    memcpy(&cert_len, &m_private_key.at(CERT_LEN_POS), sizeof(cert_len));
    cert_len = be16toh_internal(cert_len);

    const size_t dest_len = DEST_LEN_BASE + cert_len;

    if (dest_len > m_private_key.size()) {
        throw std::runtime_error(strprintf("Certificate length (%d) designates that the private key should "
                                           "be %d bytes, but it is only %d bytes",
                                           cert_len,
                                           dest_len,
                                           m_private_key.size()));
    }

    return Binary{m_private_key.begin(), m_private_key.begin() + dest_len};
}

void Session::CreateIfNotCreatedAlready()
{
    std::string errmsg;
    if (m_control_sock && m_control_sock->IsConnected(errmsg)) {
        return;
    }

    const auto session_type = m_transient ? "transient" : "persistent";
    const auto session_id = GetRandHash().GetHex().substr(0, 10); // full is overkill, too verbose in the logs

    LogPrintLevel(BCLog::I2P, BCLog::Level::Debug, "Creating %s SAM session %s with %s\n", session_type, session_id, m_control_host.ToString());

    auto sock = Hello();

    if (m_transient) {
        // The destination (private key) is generated upon session creation and returned
        // in the reply in DESTINATION=.
        const Reply& reply = SendRequestAndGetReply(
            *sock,
            strprintf("SESSION CREATE STYLE=STREAM ID=%s DESTINATION=TRANSIENT SIGNATURE_TYPE=7 "
                      "i2cp.leaseSetEncType=4,0 inbound.quantity=1 outbound.quantity=1",
                      session_id));

        m_private_key = DecodeI2PBase64(reply.Get("DESTINATION"));
    } else {
        // Read our persistent destination (private key) from disk or generate
        // one and save it to disk. Then use it when creating the session.
        const auto& [read_ok, data] = ReadBinaryFile(m_private_key_file);
        if (read_ok) {
            m_private_key.assign(data.begin(), data.end());
        } else {
            GenerateAndSavePrivateKey(*sock);
        }

        const std::string& private_key_b64 = SwapBase64(EncodeBase64(m_private_key));

        SendRequestAndGetReply(*sock,
                               strprintf("SESSION CREATE STYLE=STREAM ID=%s DESTINATION=%s "
                                         "i2cp.leaseSetEncType=4,0 inbound.quantity=3 outbound.quantity=3",
                                         session_id,
                                         private_key_b64));
    }

    m_my_addr = CService(DestBinToAddr(MyDestination()), I2P_SAM31_PORT);
    m_session_id = session_id;
    m_control_sock = std::move(sock);

    LogPrintLevel(BCLog::I2P, BCLog::Level::Info, "%s SAM session %s created, my address=%s\n",
        Capitalize(session_type),
        m_session_id,
        m_my_addr.ToStringAddrPort());
}

std::unique_ptr<Sock> Session::StreamAccept()
{
    auto sock = Hello();

    const Reply& reply = SendRequestAndGetReply(
        *sock, strprintf("STREAM ACCEPT ID=%s SILENT=false", m_session_id), false);

    const std::string& result = reply.Get("RESULT");

    if (result == "OK") {
        return sock;
    }

    if (result == "INVALID_ID") {
        // If our session id is invalid, then force session re-creation on next usage.
        Disconnect();
    }

    throw std::runtime_error(strprintf("\"%s\"", reply.full));
}

void Session::Disconnect()
{
    if (m_control_sock) {
        if (m_session_id.empty()) {
            LogPrintLevel(BCLog::I2P, BCLog::Level::Info, "Destroying incomplete SAM session\n");
        } else {
            LogPrintLevel(BCLog::I2P, BCLog::Level::Info, "Destroying SAM session %s\n", m_session_id);
        }
        m_control_sock.reset();
    }
    m_session_id.clear();
}
} // namespace sam
} // namespace i2p
