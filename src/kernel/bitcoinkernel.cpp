// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BITCOINKERNEL_BUILD

#include <kernel/bitcoinkernel.h>

#include <consensus/amount.h>
#include <kernel/context.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <util/translation.h>

#include <cstddef>
#include <cstring>
#include <exception>
#include <functional>
#include <span>
#include <string>
#include <utility>
#include <vector>

// Define G_TRANSLATION_FUN symbol in libbitcoinkernel library so users of the
// library aren't required to export this symbol
extern const std::function<std::string(const char*)> G_TRANSLATION_FUN{nullptr};

static const kernel::Context btck_context_static{};

namespace {

bool is_valid_flag_combination(script_verify_flags flags)
{
    if (flags & SCRIPT_VERIFY_CLEANSTACK && ~flags & (SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS)) return false;
    if (flags & SCRIPT_VERIFY_WITNESS && ~flags & SCRIPT_VERIFY_P2SH) return false;
    return true;
}

class WriterStream
{
private:
    btck_WriteBytes m_writer;
    void* m_user_data;

public:
    WriterStream(btck_WriteBytes writer, void* user_data)
        : m_writer{writer}, m_user_data{user_data} {}

    //
    // Stream subset
    //
    void write(std::span<const std::byte> src)
    {
        if (m_writer(std::data(src), src.size(), m_user_data) != 0) {
            throw std::runtime_error("Failed to write serilization data");
        }
    }

    template <typename T>
    WriterStream& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return *this;
    }
};

template <typename C, typename CPP>
struct Handle {
    static C* ref(CPP* cpp_type)
    {
        return reinterpret_cast<C*>(cpp_type);
    }

    static const C* ref(const CPP* cpp_type)
    {
        return reinterpret_cast<const C*>(cpp_type);
    }

    template <typename... Args>
    static C* create(Args&&... args)
    {
        auto cpp_obj{std::make_unique<CPP>(std::forward<Args>(args)...)};
        return reinterpret_cast<C*>(cpp_obj.release());
    }

    static C* copy(const C* ptr)
    {
        auto cpp_obj{std::make_unique<CPP>(get(ptr))};
        return reinterpret_cast<C*>(cpp_obj.release());
    }

    static const CPP& get(const C* ptr)
    {
        return *reinterpret_cast<const CPP*>(ptr);
    }

    static void operator delete(void* ptr)
    {
        delete reinterpret_cast<CPP*>(ptr);
    }
};

} // namespace

struct btck_Transaction : Handle<btck_Transaction, std::shared_ptr<const CTransaction>> {};
struct btck_TransactionOutput : Handle<btck_TransactionOutput, CTxOut> {};
struct btck_ScriptPubkey : Handle<btck_ScriptPubkey, CScript> {};

btck_Transaction* btck_transaction_create(const void* raw_transaction, size_t raw_transaction_len)
{
    try {
        DataStream stream{std::span{reinterpret_cast<const std::byte*>(raw_transaction), raw_transaction_len}};
        return btck_Transaction::create(std::make_shared<const CTransaction>(deserialize, TX_WITH_WITNESS, stream));
    } catch (...) {
        return nullptr;
    }
}

size_t btck_transaction_count_outputs(const btck_Transaction* transaction)
{
    return btck_Transaction::get(transaction)->vout.size();
}

const btck_TransactionOutput* btck_transaction_get_output_at(const btck_Transaction* transaction, size_t output_index)
{
    const CTransaction& tx = *btck_Transaction::get(transaction);
    assert(output_index < tx.vout.size());
    return btck_TransactionOutput::ref(&tx.vout[output_index]);
}

size_t btck_transaction_count_inputs(const btck_Transaction* transaction)
{
    return btck_Transaction::get(transaction)->vin.size();
}

btck_Transaction* btck_transaction_copy(const btck_Transaction* transaction)
{
    return btck_Transaction::copy(transaction);
}

int btck_transaction_to_bytes(const btck_Transaction* transaction, btck_WriteBytes writer, void* user_data)
{
    try {
        WriterStream ws{writer, user_data};
        ws << TX_WITH_WITNESS(btck_Transaction::get(transaction));
        return 0;
    } catch (...) {
        return -1;
    }
}

void btck_transaction_destroy(btck_Transaction* transaction)
{
    delete transaction;
}

btck_ScriptPubkey* btck_script_pubkey_create(const void* script_pubkey, size_t script_pubkey_len)
{
    auto data = std::span{reinterpret_cast<const uint8_t*>(script_pubkey), script_pubkey_len};
    return btck_ScriptPubkey::create(data.begin(), data.end());
}

int btck_script_pubkey_to_bytes(const btck_ScriptPubkey* script_pubkey_, btck_WriteBytes writer, void* user_data)
{
    const auto& script_pubkey{btck_ScriptPubkey::get(script_pubkey_)};
    return writer(script_pubkey.data(), script_pubkey.size(), user_data);
}

btck_ScriptPubkey* btck_script_pubkey_copy(const btck_ScriptPubkey* script_pubkey)
{
    return btck_ScriptPubkey::copy(script_pubkey);
}

void btck_script_pubkey_destroy(btck_ScriptPubkey* script_pubkey)
{
    delete script_pubkey;
}

btck_TransactionOutput* btck_transaction_output_create(const btck_ScriptPubkey* script_pubkey, int64_t amount)
{
    return btck_TransactionOutput::create(amount, btck_ScriptPubkey::get(script_pubkey));
}

btck_TransactionOutput* btck_transaction_output_copy(const btck_TransactionOutput* output)
{
    return btck_TransactionOutput::copy(output);
}

const btck_ScriptPubkey* btck_transaction_output_get_script_pubkey(const btck_TransactionOutput* output)
{
    return btck_ScriptPubkey::ref(&btck_TransactionOutput::get(output).scriptPubKey);
}

int64_t btck_transaction_output_get_amount(const btck_TransactionOutput* output)
{
    return btck_TransactionOutput::get(output).nValue;
}

void btck_transaction_output_destroy(btck_TransactionOutput* output)
{
    delete output;
}

int btck_script_pubkey_verify(const btck_ScriptPubkey* script_pubkey,
                              const int64_t amount,
                              const btck_Transaction* tx_to,
                              const btck_TransactionOutput** spent_outputs_, size_t spent_outputs_len,
                              const unsigned int input_index,
                              const btck_ScriptVerificationFlags flags,
                              btck_ScriptVerifyStatus* status)
{
    // Assert that all specified flags are part of the interface before continuing
    assert((flags & ~btck_ScriptVerificationFlags_ALL) == 0);

    if (!is_valid_flag_combination(script_verify_flags::from_int(flags))) {
        if (status) *status = btck_ScriptVerifyStatus_ERROR_INVALID_FLAGS_COMBINATION;
        return 0;
    }

    if (flags & btck_ScriptVerificationFlags_TAPROOT && spent_outputs_ == nullptr) {
        if (status) *status = btck_ScriptVerifyStatus_ERROR_SPENT_OUTPUTS_REQUIRED;
        return 0;
    }

    const CTransaction& tx{*btck_Transaction::get(tx_to)};
    std::vector<CTxOut> spent_outputs;
    if (spent_outputs_ != nullptr) {
        assert(spent_outputs_len == tx.vin.size());
        spent_outputs.reserve(spent_outputs_len);
        for (size_t i = 0; i < spent_outputs_len; i++) {
            const CTxOut& tx_out{btck_TransactionOutput::get(spent_outputs_[i])};
            spent_outputs.push_back(tx_out);
        }
    }

    assert(input_index < tx.vin.size());
    PrecomputedTransactionData txdata{tx};

    if (spent_outputs_ != nullptr && flags & btck_ScriptVerificationFlags_TAPROOT) {
        txdata.Init(tx, std::move(spent_outputs));
    }

    bool result = VerifyScript(tx.vin[input_index].scriptSig,
                               btck_ScriptPubkey::get(script_pubkey),
                               &tx.vin[input_index].scriptWitness,
                               script_verify_flags::from_int(flags),
                               TransactionSignatureChecker(&tx, input_index, amount, txdata, MissingDataBehavior::FAIL),
                               nullptr);
    return result ? 1 : 0;
}
