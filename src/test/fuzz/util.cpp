// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_processing.h>
#include <netaddress.h>
#include <netmessagemaker.h>
#include <test/fuzz/util.h>
#include <test/util/script.h>
#include <util/overflow.h>
#include <util/time.h>
#include <version.h>

#include <memory>

FuzzedSock::FuzzedSock(FuzzedDataProvider& fuzzed_data_provider)
    : m_fuzzed_data_provider{fuzzed_data_provider}, m_selectable{fuzzed_data_provider.ConsumeBool()}
{
    m_socket = fuzzed_data_provider.ConsumeIntegralInRange<SOCKET>(INVALID_SOCKET - 1, INVALID_SOCKET);
}

FuzzedSock::~FuzzedSock()
{
    // Sock::~Sock() will be called after FuzzedSock::~FuzzedSock() and it will call
    // close(m_socket) if m_socket is not INVALID_SOCKET.
    // Avoid closing an arbitrary file descriptor (m_socket is just a random very high number which
    // theoretically may concide with a real opened file descriptor).
    m_socket = INVALID_SOCKET;
}

FuzzedSock& FuzzedSock::operator=(Sock&& other)
{
    assert(false && "Move of Sock into FuzzedSock not allowed.");
    return *this;
}

ssize_t FuzzedSock::Send(const void* data, size_t len, int flags) const
{
    constexpr std::array send_errnos{
        EACCES,
        EAGAIN,
        EALREADY,
        EBADF,
        ECONNRESET,
        EDESTADDRREQ,
        EFAULT,
        EINTR,
        EINVAL,
        EISCONN,
        EMSGSIZE,
        ENOBUFS,
        ENOMEM,
        ENOTCONN,
        ENOTSOCK,
        EOPNOTSUPP,
        EPIPE,
        EWOULDBLOCK,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        return len;
    }
    const ssize_t r = m_fuzzed_data_provider.ConsumeIntegralInRange<ssize_t>(-1, len);
    if (r == -1) {
        SetFuzzedErrNo(m_fuzzed_data_provider, send_errnos);
    }
    return r;
}

ssize_t FuzzedSock::Recv(void* buf, size_t len, int flags) const
{
    // Have a permanent error at recv_errnos[0] because when the fuzzed data is exhausted
    // SetFuzzedErrNo() will always return the first element and we want to avoid Recv()
    // returning -1 and setting errno to EAGAIN repeatedly.
    constexpr std::array recv_errnos{
        ECONNREFUSED,
        EAGAIN,
        EBADF,
        EFAULT,
        EINTR,
        EINVAL,
        ENOMEM,
        ENOTCONN,
        ENOTSOCK,
        EWOULDBLOCK,
    };
    assert(buf != nullptr || len == 0);
    if (len == 0 || m_fuzzed_data_provider.ConsumeBool()) {
        const ssize_t r = m_fuzzed_data_provider.ConsumeBool() ? 0 : -1;
        if (r == -1) {
            SetFuzzedErrNo(m_fuzzed_data_provider, recv_errnos);
        }
        return r;
    }
    std::vector<uint8_t> random_bytes;
    bool pad_to_len_bytes{m_fuzzed_data_provider.ConsumeBool()};
    if (m_peek_data.has_value()) {
        // `MSG_PEEK` was used in the preceding `Recv()` call, return `m_peek_data`.
        random_bytes.assign({m_peek_data.value()});
        if ((flags & MSG_PEEK) == 0) {
            m_peek_data.reset();
        }
        pad_to_len_bytes = false;
    } else if ((flags & MSG_PEEK) != 0) {
        // New call with `MSG_PEEK`.
        random_bytes = m_fuzzed_data_provider.ConsumeBytes<uint8_t>(1);
        if (!random_bytes.empty()) {
            m_peek_data = random_bytes[0];
            pad_to_len_bytes = false;
        }
    } else {
        random_bytes = m_fuzzed_data_provider.ConsumeBytes<uint8_t>(
            m_fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, len));
    }
    if (random_bytes.empty()) {
        const ssize_t r = m_fuzzed_data_provider.ConsumeBool() ? 0 : -1;
        if (r == -1) {
            SetFuzzedErrNo(m_fuzzed_data_provider, recv_errnos);
        }
        return r;
    }
    std::memcpy(buf, random_bytes.data(), random_bytes.size());
    if (pad_to_len_bytes) {
        if (len > random_bytes.size()) {
            std::memset((char*)buf + random_bytes.size(), 0, len - random_bytes.size());
        }
        return len;
    }
    if (m_fuzzed_data_provider.ConsumeBool() && std::getenv("FUZZED_SOCKET_FAKE_LATENCY") != nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    return random_bytes.size();
}

int FuzzedSock::Connect(const sockaddr*, socklen_t) const
{
    // Have a permanent error at connect_errnos[0] because when the fuzzed data is exhausted
    // SetFuzzedErrNo() will always return the first element and we want to avoid Connect()
    // returning -1 and setting errno to EAGAIN repeatedly.
    constexpr std::array connect_errnos{
        ECONNREFUSED,
        EAGAIN,
        ECONNRESET,
        EHOSTUNREACH,
        EINPROGRESS,
        EINTR,
        ENETUNREACH,
        ETIMEDOUT,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, connect_errnos);
        return -1;
    }
    return 0;
}

int FuzzedSock::Bind(const sockaddr*, socklen_t) const
{
    // Have a permanent error at bind_errnos[0] because when the fuzzed data is exhausted
    // SetFuzzedErrNo() will always set the global errno to bind_errnos[0]. We want to
    // avoid this method returning -1 and setting errno to a temporary error (like EAGAIN)
    // repeatedly because proper code should retry on temporary errors, leading to an
    // infinite loop.
    constexpr std::array bind_errnos{
        EACCES,
        EADDRINUSE,
        EADDRNOTAVAIL,
        EAGAIN,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, bind_errnos);
        return -1;
    }
    return 0;
}

int FuzzedSock::Listen(int) const
{
    // Have a permanent error at listen_errnos[0] because when the fuzzed data is exhausted
    // SetFuzzedErrNo() will always set the global errno to listen_errnos[0]. We want to
    // avoid this method returning -1 and setting errno to a temporary error (like EAGAIN)
    // repeatedly because proper code should retry on temporary errors, leading to an
    // infinite loop.
    constexpr std::array listen_errnos{
        EADDRINUSE,
        EINVAL,
        EOPNOTSUPP,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, listen_errnos);
        return -1;
    }
    return 0;
}

std::unique_ptr<Sock> FuzzedSock::Accept(sockaddr* addr, socklen_t* addr_len) const
{
    constexpr std::array accept_errnos{
        ECONNABORTED,
        EINTR,
        ENOMEM,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, accept_errnos);
        return std::unique_ptr<FuzzedSock>();
    }
    return std::make_unique<FuzzedSock>(m_fuzzed_data_provider);
}

int FuzzedSock::GetSockOpt(int level, int opt_name, void* opt_val, socklen_t* opt_len) const
{
    constexpr std::array getsockopt_errnos{
        ENOMEM,
        ENOBUFS,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, getsockopt_errnos);
        return -1;
    }
    if (opt_val == nullptr) {
        return 0;
    }
    std::memcpy(opt_val,
                ConsumeFixedLengthByteVector(m_fuzzed_data_provider, *opt_len).data(),
                *opt_len);
    return 0;
}

int FuzzedSock::SetSockOpt(int, int, const void*, socklen_t) const
{
    constexpr std::array setsockopt_errnos{
        ENOMEM,
        ENOBUFS,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, setsockopt_errnos);
        return -1;
    }
    return 0;
}

int FuzzedSock::GetSockName(sockaddr* name, socklen_t* name_len) const
{
    constexpr std::array getsockname_errnos{
        ECONNRESET,
        ENOBUFS,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, getsockname_errnos);
        return -1;
    }
    *name_len = m_fuzzed_data_provider.ConsumeData(name, *name_len);
    return 0;
}

bool FuzzedSock::SetNonBlocking() const
{
    constexpr std::array setnonblocking_errnos{
        EBADF,
        EPERM,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, setnonblocking_errnos);
        return false;
    }
    return true;
}

bool FuzzedSock::IsSelectable(bool is_select) const
{
    return m_selectable;
}

bool FuzzedSock::Wait(std::chrono::milliseconds timeout, Event requested, SocketEventsMode event_mode, Event* occurred) const
{
    constexpr std::array wait_errnos{
        EBADF,
        EINTR,
        EINVAL,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, wait_errnos);
        return false;
    }
    if (occurred != nullptr) {
        *occurred = m_fuzzed_data_provider.ConsumeBool() ? requested : 0;
    }
    return true;
}

bool FuzzedSock::WaitMany(std::chrono::milliseconds timeout, EventsPerSock& events_per_sock, SocketEventsMode event_mode) const
{
    for (auto& [sock, events] : events_per_sock) {
        (void)sock;
        events.occurred = m_fuzzed_data_provider.ConsumeBool() ? events.requested : 0;
    }
    return true;
}

bool FuzzedSock::IsConnected(std::string& errmsg) const
{
    if (m_fuzzed_data_provider.ConsumeBool()) {
        return true;
    }
    errmsg = "disconnected at random by the fuzzer";
    return false;
}

void FillNode(FuzzedDataProvider& fuzzed_data_provider, ConnmanTestMsg& connman, CNode& node) noexcept
{
    connman.Handshake(node,
                      /*successfully_connected=*/fuzzed_data_provider.ConsumeBool(),
                      /*remote_services=*/ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS),
                      /*local_services=*/ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS),
                      /*version=*/fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(MIN_PEER_PROTO_VERSION, std::numeric_limits<int32_t>::max()),
                      /*relay_txs=*/fuzzed_data_provider.ConsumeBool());
}

CAmount ConsumeMoney(FuzzedDataProvider& fuzzed_data_provider, const std::optional<CAmount>& max) noexcept
{
    return fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, max.value_or(MAX_MONEY));
}

int64_t ConsumeTime(FuzzedDataProvider& fuzzed_data_provider, const std::optional<int64_t>& min, const std::optional<int64_t>& max) noexcept
{
    // Avoid t=0 (1970-01-01T00:00:00Z) since SetMockTime(0) disables mocktime.
    static const int64_t time_min{ParseISO8601DateTime("2000-01-01T00:00:01Z")};
    static const int64_t time_max{ParseISO8601DateTime("2100-12-31T23:59:59Z")};
    return fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(min.value_or(time_min), max.value_or(time_max));
}

CMutableTransaction ConsumeTransaction(FuzzedDataProvider& fuzzed_data_provider, const std::optional<std::vector<uint256>>& prevout_txids, const int max_num_in, const int max_num_out) noexcept
{
    CMutableTransaction tx_mut;
    tx_mut.nVersion = fuzzed_data_provider.ConsumeBool() ?
                          CTransaction::CURRENT_VERSION :
                          fuzzed_data_provider.ConsumeIntegral<int32_t>();
    tx_mut.nLockTime = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    const auto num_in = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, max_num_in);
    const auto num_out = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, max_num_out);
    for (int i = 0; i < num_in; ++i) {
        const auto& txid_prev = prevout_txids ?
                                    PickValue(fuzzed_data_provider, *prevout_txids) :
                                    ConsumeUInt256(fuzzed_data_provider);
        const auto index_out = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, max_num_out);
        const auto sequence = ConsumeSequence(fuzzed_data_provider);
        const auto script_sig = ConsumeScript(fuzzed_data_provider);
        CTxIn in;
        in.prevout = COutPoint{txid_prev, index_out};
        in.nSequence = sequence;
        in.scriptSig = script_sig;

        tx_mut.vin.push_back(in);
    }
    for (int i = 0; i < num_out; ++i) {
        const auto amount = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-10, 50 * COIN + 10);
        const auto script_pk = ConsumeScript(fuzzed_data_provider);
        tx_mut.vout.emplace_back(amount, script_pk);
    }
    return tx_mut;
}

CScript ConsumeScript(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    CScript r_script{};
    {
        // Keep a buffer of bytes to allow the fuzz engine to produce smaller
        // inputs to generate CScripts with repeated data.
        static constexpr unsigned MAX_BUFFER_SZ{128};
        std::vector<uint8_t> buffer(MAX_BUFFER_SZ, uint8_t{'a'});
        while (fuzzed_data_provider.ConsumeBool()) {
            CallOneOf(
                fuzzed_data_provider,
                [&] {
                    // Insert byte vector directly to allow malformed or unparsable scripts
                    r_script.insert(r_script.end(), buffer.begin(), buffer.begin() + fuzzed_data_provider.ConsumeIntegralInRange(0U, MAX_BUFFER_SZ));
                },
                [&] {
                    // Push a byte vector from the buffer
                    r_script << std::vector<uint8_t>{buffer.begin(), buffer.begin() + fuzzed_data_provider.ConsumeIntegralInRange(0U, MAX_BUFFER_SZ)};
                },
                [&] {
                    // Push multisig
                    // There is a special case for this to aid the fuzz engine
                    // navigate the highly structured multisig format.
                    r_script << fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, 22);
                    int num_data{fuzzed_data_provider.ConsumeIntegralInRange(1, 22)};
                    std::vector<uint8_t> pubkey_comp{buffer.begin(), buffer.begin() + CPubKey::COMPRESSED_SIZE};
                    pubkey_comp.front() = fuzzed_data_provider.ConsumeIntegralInRange(2, 3); // Set first byte for GetLen() to pass
                    std::vector<uint8_t> pubkey_uncomp{buffer.begin(), buffer.begin() + CPubKey::SIZE};
                    pubkey_uncomp.front() = fuzzed_data_provider.ConsumeIntegralInRange(4, 7); // Set first byte for GetLen() to pass
                    while (num_data--) {
                        auto& pubkey{fuzzed_data_provider.ConsumeBool() ? pubkey_uncomp : pubkey_comp};
                        if (fuzzed_data_provider.ConsumeBool()) {
                            pubkey.back() = num_data; // Make each pubkey different
                        }
                        r_script << pubkey;
                    }
                    r_script << fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, 22);
                },
                [&] {
                    // Mutate the buffer
                    const auto vec{ConsumeRandomLengthByteVector(fuzzed_data_provider, /*max_length=*/MAX_BUFFER_SZ)};
                    std::copy(vec.begin(), vec.end(), buffer.begin());
                },
                [&] {
                    // Push an integral
                    r_script << fuzzed_data_provider.ConsumeIntegral<int64_t>();
                },
                [&] {
                    // Push an opcode
                    r_script << ConsumeOpcodeType(fuzzed_data_provider);
                },
                [&] {
                    // Push a scriptnum
                    r_script << ConsumeScriptNum(fuzzed_data_provider);
                });
        }
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        uint256 script_hash;
        CSHA256().Write(r_script.data(), r_script.size()).Finalize(script_hash.begin());
        r_script.clear();
        r_script << OP_0 << ToByteVector(script_hash);
    }
    return r_script;
}

uint32_t ConsumeSequence(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return fuzzed_data_provider.ConsumeBool() ?
               fuzzed_data_provider.PickValueInArray({
                   CTxIn::SEQUENCE_FINAL,
                   CTxIn::MAX_SEQUENCE_NONFINAL,
               }) :
               fuzzed_data_provider.ConsumeIntegral<uint32_t>();
}

CKey ConsumePrivateKey(FuzzedDataProvider& fuzzed_data_provider, std::optional<bool> compressed) noexcept
{
    auto key_data = fuzzed_data_provider.ConsumeBytes<uint8_t>(32);
    key_data.resize(32);
    CKey key;
    bool compressed_value = compressed ? *compressed : fuzzed_data_provider.ConsumeBool();
    key.Set(key_data.begin(), key_data.end(), compressed_value);
    return key;
}

CTxMemPoolEntry ConsumeTxMemPoolEntry(FuzzedDataProvider& fuzzed_data_provider, const CTransaction& tx) noexcept
{
    // Avoid:
    // policy/feerate.cpp:28:34: runtime error: signed integer overflow: 34873208148477500 * 1000 cannot be represented in type 'long'
    //
    // Reproduce using CFeeRate(348732081484775, 10).GetFeePerK()
    const CAmount fee = std::min<CAmount>(ConsumeMoney(fuzzed_data_provider), std::numeric_limits<CAmount>::max() / static_cast<CAmount>(100000));
    assert(MoneyRange(fee));
    const int64_t time = fuzzed_data_provider.ConsumeIntegral<int64_t>();
    const unsigned int entry_height = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    const bool spends_coinbase = fuzzed_data_provider.ConsumeBool();
    const bool dip1_status = fuzzed_data_provider.ConsumeBool();
    const unsigned int sig_op_cost = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, MaxBlockSigOps(dip1_status));
    return CTxMemPoolEntry{MakeTransactionRef(tx), fee, time, entry_height, spends_coinbase, sig_op_cost, {}};
}

bool ContainsSpentInput(const CTransaction& tx, const CCoinsViewCache& inputs) noexcept
{
    for (const CTxIn& tx_in : tx.vin) {
        const Coin& coin = inputs.AccessCoin(tx_in.prevout);
        if (coin.IsSpent()) {
            return true;
        }
    }
    return false;
}

CAddress ConsumeAddress(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return {ConsumeService(fuzzed_data_provider), ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS), NodeSeconds{std::chrono::seconds{fuzzed_data_provider.ConsumeIntegral<uint32_t>()}}};
}

FILE* FuzzedFileProvider::open()
{
    SetFuzzedErrNo(m_fuzzed_data_provider);
    if (m_fuzzed_data_provider.ConsumeBool()) {
        return nullptr;
    }
    std::string mode;
    CallOneOf(
        m_fuzzed_data_provider,
        [&] {
            mode = "r";
        },
        [&] {
            mode = "r+";
        },
        [&] {
            mode = "w";
        },
        [&] {
            mode = "w+";
        },
        [&] {
            mode = "a";
        },
        [&] {
            mode = "a+";
        });
#if defined _GNU_SOURCE && (defined(__linux__) || defined(__FreeBSD__))
    const cookie_io_functions_t io_hooks = {
        FuzzedFileProvider::read,
        FuzzedFileProvider::write,
        FuzzedFileProvider::seek,
        FuzzedFileProvider::close,
    };
    return fopencookie(this, mode.c_str(), io_hooks);
#else
    (void)mode;
    return nullptr;
#endif
}

ssize_t FuzzedFileProvider::read(void* cookie, char* buf, size_t size)
{
    FuzzedFileProvider* fuzzed_file = (FuzzedFileProvider*)cookie;
    SetFuzzedErrNo(fuzzed_file->m_fuzzed_data_provider);
    if (buf == nullptr || size == 0 || fuzzed_file->m_fuzzed_data_provider.ConsumeBool()) {
        return fuzzed_file->m_fuzzed_data_provider.ConsumeBool() ? 0 : -1;
    }
    const std::vector<uint8_t> random_bytes = fuzzed_file->m_fuzzed_data_provider.ConsumeBytes<uint8_t>(size);
    if (random_bytes.empty()) {
        return 0;
    }
    std::memcpy(buf, random_bytes.data(), random_bytes.size());
    if (AdditionOverflow(fuzzed_file->m_offset, (int64_t)random_bytes.size())) {
        return fuzzed_file->m_fuzzed_data_provider.ConsumeBool() ? 0 : -1;
    }
    fuzzed_file->m_offset += random_bytes.size();
    return random_bytes.size();
}

ssize_t FuzzedFileProvider::write(void* cookie, const char* buf, size_t size)
{
    FuzzedFileProvider* fuzzed_file = (FuzzedFileProvider*)cookie;
    SetFuzzedErrNo(fuzzed_file->m_fuzzed_data_provider);
    const ssize_t n = fuzzed_file->m_fuzzed_data_provider.ConsumeIntegralInRange<ssize_t>(0, size);
    if (AdditionOverflow(fuzzed_file->m_offset, (int64_t)n)) {
        return 0;
    }
    fuzzed_file->m_offset += n;
    return n;
}

int FuzzedFileProvider::seek(void* cookie, int64_t* offset, int whence)
{
    assert(whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END);
    FuzzedFileProvider* fuzzed_file = (FuzzedFileProvider*)cookie;
    SetFuzzedErrNo(fuzzed_file->m_fuzzed_data_provider);
    int64_t new_offset = 0;
    if (whence == SEEK_SET) {
        new_offset = *offset;
    } else if (whence == SEEK_CUR) {
        if (AdditionOverflow(fuzzed_file->m_offset, *offset)) {
            return -1;
        }
        new_offset = fuzzed_file->m_offset + *offset;
    } else if (whence == SEEK_END) {
        const int64_t n = fuzzed_file->m_fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, 4096);
        if (AdditionOverflow(n, *offset)) {
            return -1;
        }
        new_offset = n + *offset;
    }
    if (new_offset < 0) {
        return -1;
    }
    fuzzed_file->m_offset = new_offset;
    *offset = new_offset;
    return fuzzed_file->m_fuzzed_data_provider.ConsumeIntegralInRange<int>(-1, 0);
}

int FuzzedFileProvider::close(void* cookie)
{
    FuzzedFileProvider* fuzzed_file = (FuzzedFileProvider*)cookie;
    SetFuzzedErrNo(fuzzed_file->m_fuzzed_data_provider);
    return fuzzed_file->m_fuzzed_data_provider.ConsumeIntegralInRange<int>(-1, 0);
}
