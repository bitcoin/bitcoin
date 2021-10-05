// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <pubkey.h>
#include <test/fuzz/util.h>
#include <test/util/script.h>
#include <util/rbf.h>
#include <util/time.h>
#include <version.h>

FuzzedSock::FuzzedSock(FuzzedDataProvider& fuzzed_data_provider)
    : m_fuzzed_data_provider{fuzzed_data_provider}
{
    m_socket = fuzzed_data_provider.ConsumeIntegralInRange<SOCKET>(INVALID_SOCKET - 1, INVALID_SOCKET);
}

FuzzedSock::~FuzzedSock()
{
    // Sock::~Sock() will be called after FuzzedSock::~FuzzedSock() and it will call
    // Sock::Reset() (not FuzzedSock::Reset()!) which will call CloseSocket(m_socket).
    // Avoid closing an arbitrary file descriptor (m_socket is just a random very high number which
    // theoretically may concide with a real opened file descriptor).
    Reset();
}

FuzzedSock& FuzzedSock::operator=(Sock&& other)
{
    assert(false && "Move of Sock into FuzzedSock not allowed.");
    return *this;
}

void FuzzedSock::Reset()
{
    m_socket = INVALID_SOCKET;
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

bool FuzzedSock::Wait(std::chrono::milliseconds timeout, Event requested, Event* occurred) const
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

bool FuzzedSock::IsConnected(std::string& errmsg) const
{
    if (m_fuzzed_data_provider.ConsumeBool()) {
        return true;
    }
    errmsg = "disconnected at random by the fuzzer";
    return false;
}

void FillNode(FuzzedDataProvider& fuzzed_data_provider, CNode& node, bool init_version) noexcept
{
    const ServiceFlags remote_services = ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS);
    const NetPermissionFlags permission_flags = ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS);
    const int32_t version = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(MIN_PEER_PROTO_VERSION, std::numeric_limits<int32_t>::max());
    const bool filter_txs = fuzzed_data_provider.ConsumeBool();

    node.nServices = remote_services;
    node.m_permissionFlags = permission_flags;
    if (init_version) {
        node.nVersion = version;
        node.SetCommonVersion(std::min(version, PROTOCOL_VERSION));
    }
    if (node.m_tx_relay != nullptr) {
        LOCK(node.m_tx_relay->cs_filter);
        node.m_tx_relay->fRelayTxes = filter_txs;
    }
}

CAmount ConsumeMoney(FuzzedDataProvider& fuzzed_data_provider, const std::optional<CAmount>& max) noexcept
{
    return fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, max.value_or(MAX_MONEY));
}

int64_t ConsumeTime(FuzzedDataProvider& fuzzed_data_provider, const std::optional<int64_t>& min, const std::optional<int64_t>& max) noexcept
{
    // Avoid t=0 (1970-01-01T00:00:00Z) since SetMockTime(0) disables mocktime.
    static const int64_t time_min = ParseISO8601DateTime("1970-01-01T00:00:01Z");
    static const int64_t time_max = ParseISO8601DateTime("9999-12-31T23:59:59Z");
    return fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(min.value_or(time_min), max.value_or(time_max));
}

CMutableTransaction ConsumeTransaction(FuzzedDataProvider& fuzzed_data_provider, const std::optional<std::vector<uint256>>& prevout_txids, const int max_num_in, const int max_num_out) noexcept
{
    CMutableTransaction tx_mut;
    const auto p2wsh_op_true = fuzzed_data_provider.ConsumeBool();
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
        const auto script_sig = p2wsh_op_true ? CScript{} : ConsumeScript(fuzzed_data_provider);
        CScriptWitness script_wit;
        if (p2wsh_op_true) {
            script_wit.stack = std::vector<std::vector<uint8_t>>{WITNESS_STACK_ELEM_OP_TRUE};
        } else {
            script_wit = ConsumeScriptWitness(fuzzed_data_provider);
        }
        CTxIn in;
        in.prevout = COutPoint{txid_prev, index_out};
        in.nSequence = sequence;
        in.scriptSig = script_sig;
        in.scriptWitness = script_wit;

        tx_mut.vin.push_back(in);
    }
    for (int i = 0; i < num_out; ++i) {
        const auto amount = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-10, 50 * COIN + 10);
        const auto script_pk = p2wsh_op_true ?
                                   P2WSH_OP_TRUE :
                                   ConsumeScript(fuzzed_data_provider, /* max_length */ 128, /* maybe_p2wsh */ true);
        tx_mut.vout.emplace_back(amount, script_pk);
    }
    return tx_mut;
}

CScriptWitness ConsumeScriptWitness(FuzzedDataProvider& fuzzed_data_provider, const size_t max_stack_elem_size) noexcept
{
    CScriptWitness ret;
    const auto n_elements = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, max_stack_elem_size);
    for (size_t i = 0; i < n_elements; ++i) {
        ret.stack.push_back(ConsumeRandomLengthByteVector(fuzzed_data_provider));
    }
    return ret;
}

CScript ConsumeScript(FuzzedDataProvider& fuzzed_data_provider, const std::optional<size_t>& max_length, const bool maybe_p2wsh) noexcept
{
    const std::vector<uint8_t> b = ConsumeRandomLengthByteVector(fuzzed_data_provider, max_length);
    CScript r_script{b.begin(), b.end()};
    if (maybe_p2wsh && fuzzed_data_provider.ConsumeBool()) {
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
                   CTxIn::SEQUENCE_FINAL - 1,
                   MAX_BIP125_RBF_SEQUENCE,
               }) :
               fuzzed_data_provider.ConsumeIntegral<uint32_t>();
}

CTxDestination ConsumeTxDestination(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    CTxDestination tx_destination;
    const size_t call_size{CallOneOf(
        fuzzed_data_provider,
        [&] {
            tx_destination = CNoDestination{};
        },
        [&] {
            tx_destination = PKHash{ConsumeUInt160(fuzzed_data_provider)};
        },
        [&] {
            tx_destination = ScriptHash{ConsumeUInt160(fuzzed_data_provider)};
        },
        [&] {
            tx_destination = WitnessV0ScriptHash{ConsumeUInt256(fuzzed_data_provider)};
        },
        [&] {
            tx_destination = WitnessV0KeyHash{ConsumeUInt160(fuzzed_data_provider)};
        },
        [&] {
            tx_destination = WitnessV1Taproot{XOnlyPubKey{ConsumeUInt256(fuzzed_data_provider)}};
        },
        [&] {
            WitnessUnknown witness_unknown{};
            witness_unknown.version = fuzzed_data_provider.ConsumeIntegralInRange(2, 16);
            std::vector<uint8_t> witness_unknown_program_1{fuzzed_data_provider.ConsumeBytes<uint8_t>(40)};
            if (witness_unknown_program_1.size() < 2) {
                witness_unknown_program_1 = {0, 0};
            }
            witness_unknown.length = witness_unknown_program_1.size();
            std::copy(witness_unknown_program_1.begin(), witness_unknown_program_1.end(), witness_unknown.program);
            tx_destination = witness_unknown;
        })};
    Assert(call_size == std::variant_size_v<CTxDestination>);
    return tx_destination;
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
    const unsigned int sig_op_cost = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, MAX_BLOCK_SIGOPS_COST);
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

CNetAddr ConsumeNetAddr(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    const Network network = fuzzed_data_provider.PickValueInArray({Network::NET_IPV4, Network::NET_IPV6, Network::NET_INTERNAL, Network::NET_ONION});
    CNetAddr net_addr;
    if (network == Network::NET_IPV4) {
        in_addr v4_addr = {};
        v4_addr.s_addr = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
        net_addr = CNetAddr{v4_addr};
    } else if (network == Network::NET_IPV6) {
        if (fuzzed_data_provider.remaining_bytes() >= 16) {
            in6_addr v6_addr = {};
            memcpy(v6_addr.s6_addr, fuzzed_data_provider.ConsumeBytes<uint8_t>(16).data(), 16);
            net_addr = CNetAddr{v6_addr, fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
        }
    } else if (network == Network::NET_INTERNAL) {
        net_addr.SetInternal(fuzzed_data_provider.ConsumeBytesAsString(32));
    } else if (network == Network::NET_ONION) {
        net_addr.SetSpecial(fuzzed_data_provider.ConsumeBytesAsString(32));
    }
    return net_addr;
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
#if defined _GNU_SOURCE && !defined __ANDROID__
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
        return fuzzed_file->m_fuzzed_data_provider.ConsumeBool() ? 0 : -1;
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
