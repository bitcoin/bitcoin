// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_I2P_H
#define BITCOIN_I2P_H

#include <compat/compat.h>
#include <netaddress.h>
#include <netbase.h>
#include <sync.h>
#include <util/fs.h>
#include <util/sock.h>
#include <util/threadinterrupt.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace i2p {

/**
 * Binary data.
 */
using Binary = std::vector<uint8_t>;

/**
 * An established connection with another peer.
 */
struct Connection {
    /** Connected socket. */
    std::unique_ptr<Sock> sock;

    /** Our I2P address. */
    CService me;

    /** The peer's I2P address. */
    CService peer;
};

namespace sam {

/**
 * The maximum size of an incoming message from the I2P SAM proxy (in bytes).
 * Used to avoid a runaway proxy from sending us an "unlimited" amount of data without a terminator.
 * The longest known message is ~1400 bytes, so this is high enough not to be triggered during
 * normal operation, yet low enough to avoid a malicious proxy from filling our memory.
 */
static constexpr size_t MAX_MSG_SIZE{65536};

/**
 * I2P SAM session.
 */
class Session
{
public:
    /**
     * Construct a session. This will not initiate any IO, the session will be lazily created
     * later when first used.
     * @param[in] private_key_file Path to a private key file. If the file does not exist then the
     * private key will be generated and saved into the file.
     * @param[in] control_host Location of the SAM proxy.
     * @param[in,out] interrupt If this is signaled then all operations are canceled as soon as
     * possible and executing methods throw an exception.
     */
    Session(const fs::path& private_key_file,
            const Proxy& control_host,
            std::shared_ptr<CThreadInterrupt> interrupt);

    /**
     * Construct a transient session which will generate its own I2P private key
     * rather than read the one from disk (it will not be saved on disk either and
     * will be lost once this object is destroyed). This will not initiate any IO,
     * the session will be lazily created later when first used.
     * @param[in] control_host Location of the SAM proxy.
     * @param[in,out] interrupt If this is signaled then all operations are canceled as soon as
     * possible and executing methods throw an exception.
     */
    Session(const Proxy& control_host, std::shared_ptr<CThreadInterrupt> interrupt);

    /**
     * Destroy the session, closing the internally used sockets. The sockets that have been
     * returned by `Accept()` or `Connect()` will not be closed, but they will be closed by
     * the SAM proxy because the session is destroyed. So they will return an error next time
     * we try to read or write to them.
     */
    ~Session();

    /**
     * Start listening for an incoming connection.
     * @param[out] conn Upon successful completion the `sock` and `me` members will be set
     * to the listening socket and address.
     * @return true on success
     */
    bool Listen(Connection& conn) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Wait for and accept a new incoming connection.
     * @param[in,out] conn The `sock` member is used for waiting and accepting. Upon successful
     * completion the `peer` member will be set to the address of the incoming peer.
     * @return true on success
     */
    bool Accept(Connection& conn) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Connect to an I2P peer.
     * @param[in] to Peer to connect to.
     * @param[out] conn Established connection. Only set if `true` is returned.
     * @param[out] proxy_error If an error occurs due to proxy or general network failure, then
     * this is set to `true`. If an error occurs due to unreachable peer (likely peer is down), then
     * it is set to `false`. Only set if `false` is returned.
     * @return true on success
     */
    bool Connect(const CService& to, Connection& conn, bool& proxy_error) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    /**
     * A reply from the SAM proxy.
     */
    struct Reply {
        /**
         * Full, unparsed reply.
         */
        std::string full;

        /**
         * Request, used for detailed error reporting.
         */
        std::string request;

        /**
         * A map of keywords from the parsed reply.
         * For example, if the reply is "A=X B C=YZ", then the map will be
         * keys["A"] == "X"
         * keys["B"] == (empty std::optional)
         * keys["C"] == "YZ"
         */
        std::unordered_map<std::string, std::optional<std::string>> keys;

        /**
         * Get the value of a given key.
         * For example if the reply is "A=X B" then:
         * Value("A") -> "X"
         * Value("B") -> throws
         * Value("C") -> throws
         * @param[in] key Key whose value to retrieve
         * @returns the key's value
         * @throws std::runtime_error if the key is not present or if it has no value
         */
        std::string Get(const std::string& key) const;
    };

    /**
     * Send request and get a reply from the SAM proxy.
     * @param[in] sock A socket that is connected to the SAM proxy.
     * @param[in] request Raw request to send, a newline terminator is appended to it.
     * @param[in] check_result_ok If true then after receiving the reply a check is made
     * whether it contains "RESULT=OK" and an exception is thrown if it does not.
     * @throws std::runtime_error if an error occurs
     */
    Reply SendRequestAndGetReply(const Sock& sock,
                                 const std::string& request,
                                 bool check_result_ok = true) const;

    /**
     * Open a new connection to the SAM proxy.
     * @return a connected socket
     * @throws std::runtime_error if an error occurs
     */
    std::unique_ptr<Sock> Hello() const EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

    /**
     * Check the control socket for errors and possibly disconnect.
     */
    void CheckControlSock() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Generate a new destination with the SAM proxy and set `m_private_key` to it.
     * @param[in] sock Socket to use for talking to the SAM proxy.
     * @throws std::runtime_error if an error occurs
     */
    void DestGenerate(const Sock& sock) EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

    /**
     * Generate a new destination with the SAM proxy, set `m_private_key` to it and save
     * it on disk to `m_private_key_file`.
     * @param[in] sock Socket to use for talking to the SAM proxy.
     * @throws std::runtime_error if an error occurs
     */
    void GenerateAndSavePrivateKey(const Sock& sock) EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

    /**
     * Derive own destination from `m_private_key`.
     * @see https://geti2p.net/spec/common-structures#destination
     * @return an I2P destination
     */
    Binary MyDestination() const EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

    /**
     * Create the session if not already created. Reads the private key file and connects to the
     * SAM proxy.
     * @throws std::runtime_error if an error occurs
     */
    void CreateIfNotCreatedAlready() EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

    /**
     * Open a new connection to the SAM proxy and issue "STREAM ACCEPT" request using the existing
     * session id.
     * @return the idle socket that is waiting for a peer to connect to us
     * @throws std::runtime_error if an error occurs
     */
    std::unique_ptr<Sock> StreamAccept() EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

    /**
     * Destroy the session, closing the internally used sockets.
     */
    void Disconnect() EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

    /**
     * The name of the file where this peer's private key is stored (in binary).
     */
    const fs::path m_private_key_file;

    /**
     * The SAM control service proxy.
     */
    const Proxy m_control_host;

    /**
     * Cease network activity when this is signaled.
     */
    const std::shared_ptr<CThreadInterrupt> m_interrupt;

    /**
     * Mutex protecting the members that can be concurrently accessed.
     */
    mutable Mutex m_mutex;

    /**
     * The private key of this peer.
     * @see The reply to the "DEST GENERATE" command in https://geti2p.net/en/docs/api/samv3
     */
    Binary m_private_key GUARDED_BY(m_mutex);

    /**
     * SAM control socket.
     * Used to connect to the I2P SAM service and create a session
     * ("SESSION CREATE"). With the established session id we later open
     * other connections to the SAM service to accept incoming I2P
     * connections and make outgoing ones.
     * If not connected then this unique_ptr will be empty.
     * See https://geti2p.net/en/docs/api/samv3
     */
    std::unique_ptr<Sock> m_control_sock GUARDED_BY(m_mutex);

    /**
     * Our .b32.i2p address.
     * Derived from `m_private_key`.
     */
    CService m_my_addr GUARDED_BY(m_mutex);

    /**
     * SAM session id.
     */
    std::string m_session_id GUARDED_BY(m_mutex);

    /**
     * Whether this is a transient session (the I2P private key will not be
     * read or written to disk).
     */
    const bool m_transient;
};

} // namespace sam
} // namespace i2p

#endif // BITCOIN_I2P_H
