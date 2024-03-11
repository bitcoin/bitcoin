// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <pubkey.h>
#include <test/fuzz/util.h>
#include <test/util/script.h>
#include <util/check.h>
#include <util/overflow.h>
#include <util/rbf.h>
#include <util/time.h>

#include <memory>

std::vector<uint8_t> ConstructPubKeyBytes(FuzzedDataProvider& fuzzed_data_provider, Span<const uint8_t> byte_data, const bool compressed) noexcept
{
    uint8_t pk_type;
    if (compressed) {
        pk_type = fuzzed_data_provider.PickValueInArray({0x02, 0x03});
    } else {
        pk_type = fuzzed_data_provider.PickValueInArray({0x04, 0x06, 0x07});
    }
    std::vector<uint8_t> pk_data{byte_data.begin(), byte_data.begin() + (compressed ? CPubKey::COMPRESSED_SIZE : CPubKey::SIZE)};
    pk_data[0] = pk_type;
    return pk_data;
}

CAmount ConsumeMoney(FuzzedDataProvider& fuzzed_data_provider, const std::optional<CAmount>& max) noexcept
{
    return fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, max.value_or(MAX_MONEY));
}

int64_t ConsumeTime(FuzzedDataProvider& fuzzed_data_provider, const std::optional<int64_t>& min, const std::optional<int64_t>& max) noexcept
{
    // Avoid t=0 (1970-01-01T00:00:00Z) since SetMockTime(0) disables mocktime.
    static const int64_t time_min{946684801}; // 2000-01-01T00:00:01Z
    static const int64_t time_max{4133980799}; // 2100-12-31T23:59:59Z
    return fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(min.value_or(time_min), max.value_or(time_max));
}

CMutableTransaction ConsumeTransaction(FuzzedDataProvider& fuzzed_data_provider, const std::optional<std::vector<Txid>>& prevout_txids, const int max_num_in, const int max_num_out) noexcept
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
                                    Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider));
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
                                   ConsumeScript(fuzzed_data_provider, /*maybe_p2wsh=*/true);
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

CScript ConsumeScript(FuzzedDataProvider& fuzzed_data_provider, const bool maybe_p2wsh) noexcept
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
                    while (num_data--) {
                        auto pubkey_bytes{ConstructPubKeyBytes(fuzzed_data_provider, buffer, fuzzed_data_provider.ConsumeBool())};
                        if (fuzzed_data_provider.ConsumeBool()) {
                            pubkey_bytes.back() = num_data; // Make each pubkey different
                        }
                        r_script << pubkey_bytes;
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
                   CTxIn::MAX_SEQUENCE_NONFINAL,
                   MAX_BIP125_RBF_SEQUENCE,
               }) :
               fuzzed_data_provider.ConsumeIntegral<uint32_t>();
}

std::map<COutPoint, Coin> ConsumeCoins(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    std::map<COutPoint, Coin> coins;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        const std::optional<COutPoint> outpoint{ConsumeDeserializable<COutPoint>(fuzzed_data_provider)};
        if (!outpoint) {
            break;
        }
        const std::optional<Coin> coin{ConsumeDeserializable<Coin>(fuzzed_data_provider)};
        if (!coin) {
            break;
        }
        coins[*outpoint] = *coin;
    }

    return coins;
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
            bool compressed = fuzzed_data_provider.ConsumeBool();
            CPubKey pk{ConstructPubKeyBytes(
                    fuzzed_data_provider,
                    ConsumeFixedLengthByteVector(fuzzed_data_provider, (compressed ? CPubKey::COMPRESSED_SIZE : CPubKey::SIZE)),
                    compressed
            )};
            tx_destination = PubKeyDestination{pk};
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
            CPubKey scan_pk{ConstructPubKeyBytes(fuzzed_data_provider, ConsumeFixedLengthByteVector(fuzzed_data_provider, CPubKey::COMPRESSED_SIZE), /*compressed=*/true)};
            CPubKey spend_pk{ConstructPubKeyBytes(fuzzed_data_provider, ConsumeFixedLengthByteVector(fuzzed_data_provider, CPubKey::COMPRESSED_SIZE), /*compressed=*/true)};
            tx_destination = V0SilentPaymentDestination{scan_pk, spend_pk};
        },
        [&] {
            std::vector<unsigned char> program{ConsumeRandomLengthByteVector(fuzzed_data_provider, /*max_length=*/40)};
            if (program.size() < 2) {
                program = {0, 0};
            }
            tx_destination = WitnessUnknown{fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(2, 16), program};
        })};
    Assert(call_size == std::variant_size_v<CTxDestination>);
    return tx_destination;
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
