// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_BITCOINKERNEL_WRAPPER_H
#define BITCOIN_KERNEL_BITCOINKERNEL_WRAPPER_H

#include <kernel/bitcoinkernel.h>

#include <functional>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace btck {

class Transaction;
class TransactionOutput;

enum class LogCategory : btck_LogCategory {
    ALL = btck_LogCategory_ALL,
    BENCH = btck_LogCategory_BENCH,
    BLOCKSTORAGE = btck_LogCategory_BLOCKSTORAGE,
    COINDB = btck_LogCategory_COINDB,
    LEVELDB = btck_LogCategory_LEVELDB,
    MEMPOOL = btck_LogCategory_MEMPOOL,
    PRUNE = btck_LogCategory_PRUNE,
    RAND = btck_LogCategory_RAND,
    REINDEX = btck_LogCategory_REINDEX,
    VALIDATION = btck_LogCategory_VALIDATION,
    KERNEL = btck_LogCategory_KERNEL
};

enum class LogLevel : btck_LogLevel {
    TRACE_LEVEL = btck_LogLevel_TRACE,
    DEBUG_LEVEL = btck_LogLevel_DEBUG,
    INFO_LEVEL = btck_LogLevel_INFO
};

enum class ScriptVerifyStatus : btck_ScriptVerifyStatus {
    OK = btck_ScriptVerifyStatus_SCRIPT_VERIFY_OK,
    ERROR_INVALID_FLAGS_COMBINATION = btck_ScriptVerifyStatus_ERROR_INVALID_FLAGS_COMBINATION,
    ERROR_SPENT_OUTPUTS_REQUIRED = btck_ScriptVerifyStatus_ERROR_SPENT_OUTPUTS_REQUIRED,
};

enum class ScriptVerificationFlags : btck_ScriptVerificationFlags {
    NONE = btck_ScriptVerificationFlags_NONE,
    P2SH = btck_ScriptVerificationFlags_P2SH,
    DERSIG = btck_ScriptVerificationFlags_DERSIG,
    NULLDUMMY = btck_ScriptVerificationFlags_NULLDUMMY,
    CHECKLOCKTIMEVERIFY = btck_ScriptVerificationFlags_CHECKLOCKTIMEVERIFY,
    CHECKSEQUENCEVERIFY = btck_ScriptVerificationFlags_CHECKSEQUENCEVERIFY,
    WITNESS = btck_ScriptVerificationFlags_WITNESS,
    TAPROOT = btck_ScriptVerificationFlags_TAPROOT,
    ALL = btck_ScriptVerificationFlags_ALL
};

template <typename T>
struct is_bitmask_enum : std::false_type {
};

template <>
struct is_bitmask_enum<ScriptVerificationFlags> : std::true_type {
};

template <typename T>
concept BitmaskEnum = is_bitmask_enum<T>::value;

template <BitmaskEnum T>
constexpr T operator|(T lhs, T rhs)
{
    return static_cast<T>(
        static_cast<std::underlying_type_t<T>>(lhs) | static_cast<std::underlying_type_t<T>>(rhs));
}

template <BitmaskEnum T>
constexpr T operator&(T lhs, T rhs)
{
    return static_cast<T>(
        static_cast<std::underlying_type_t<T>>(lhs) & static_cast<std::underlying_type_t<T>>(rhs));
}

template <BitmaskEnum T>
constexpr T operator^(T lhs, T rhs)
{
    return static_cast<T>(
        static_cast<std::underlying_type_t<T>>(lhs) ^ static_cast<std::underlying_type_t<T>>(rhs));
}

template <BitmaskEnum T>
constexpr T operator~(T value)
{
    return static_cast<T>(~static_cast<std::underlying_type_t<T>>(value));
}

template <BitmaskEnum T>
constexpr T& operator|=(T& lhs, T rhs)
{
    return lhs = lhs | rhs;
}

template <BitmaskEnum T>
constexpr T& operator&=(T& lhs, T rhs)
{
    return lhs = lhs & rhs;
}

template <BitmaskEnum T>
constexpr T& operator^=(T& lhs, T rhs)
{
    return lhs = lhs ^ rhs;
}

template <typename T>
T check(T ptr)
{
    if (ptr == nullptr) {
        throw std::runtime_error("failed to instantiate btck object");
    }
    return ptr;
}

template <typename Collection, typename ValueType>
class Iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using iterator_concept = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = ValueType;

private:
    const Collection* m_collection;
    size_t m_idx;

public:
    Iterator() = default;
    Iterator(const Collection* ptr) : m_collection{ptr}, m_idx{0} {}
    Iterator(const Collection* ptr, size_t idx) : m_collection{ptr}, m_idx{idx} {}

    // This is just a view, so return a copy.
    auto operator*() const { return (*m_collection)[m_idx]; }
    auto operator->() const { return (*m_collection)[m_idx]; }

    auto& operator++() { m_idx++; return *this; }
    auto operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

    auto& operator--() { m_idx--; return *this; }
    auto operator--(int) { auto temp = *this; --m_idx; return temp; }

    auto& operator+=(difference_type n) { m_idx += n; return *this; }
    auto& operator-=(difference_type n) { m_idx -= n; return *this; }

    auto operator+(difference_type n) const { return Iterator(m_collection, m_idx + n); }
    auto operator-(difference_type n) const { return Iterator(m_collection, m_idx - n); }

    auto operator-(const Iterator& other) const { return static_cast<difference_type>(m_idx) - static_cast<difference_type>(other.m_idx); }

    ValueType operator[](difference_type n) const { return (*m_collection)[m_idx + n]; }

    auto operator<=>(const Iterator& other) const { return m_idx <=> other.m_idx; }

    bool operator==(const Iterator& other) const { return m_collection == other.m_collection && m_idx == other.m_idx; }

private:
    friend Iterator operator+(difference_type n, const Iterator& it) { return it + n; }
};

template <typename Container, typename SizeFunc, typename GetFunc>
concept IndexedContainer = requires(const Container& c, SizeFunc size_func, GetFunc get_func, std::size_t i) {
    { std::invoke(size_func, c) } -> std::convertible_to<std::size_t>;
    { std::invoke(get_func, c, i) }; // Return type is deduced
};

template <typename Container, auto SizeFunc, auto GetFunc>
    requires IndexedContainer<Container, decltype(SizeFunc), decltype(GetFunc)>
class Range
{
public:
    using value_type = std::invoke_result_t<decltype(GetFunc), const Container&, size_t>;
    using difference_type = std::ptrdiff_t;
    using iterator = Iterator<Range, value_type>;
    using const_iterator = iterator;

private:
    const Container* m_container;

public:
    explicit Range(const Container& container) : m_container(&container)
    {
        static_assert(std::ranges::random_access_range<Range>);
    }

    iterator begin() const { return iterator(this, 0); }
    iterator end() const { return iterator(this, size()); }

    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    size_t size() const { return std::invoke(SizeFunc, *m_container); }

    bool empty() const { return size() == 0; }

    value_type operator[](size_t index) const { return std::invoke(GetFunc, *m_container, index); }

    value_type at(size_t index) const
    {
        if (index >= size()) {
            throw std::out_of_range("Index out of range");
        }
        return (*this)[index];
    }

    value_type front() const { return (*this)[0]; }
    value_type back() const { return (*this)[size() - 1]; }
};

template <typename T>
std::vector<std::byte> write_bytes(const T* object, int (*to_bytes)(const T*, btck_WriteBytes, void*))
{
    std::vector<std::byte> bytes;
    struct UserData {
        std::vector<std::byte>* bytes;
        std::exception_ptr exception;
    };
    UserData user_data = UserData{.bytes = &bytes, .exception = nullptr};

    constexpr auto const write = +[](const void* buffer, size_t len, void* user_data) -> int {
        auto& data = *reinterpret_cast<UserData*>(user_data);
        auto& bytes = *data.bytes;
        try {
            auto const* first = static_cast<const std::byte*>(buffer);
            auto const* last = first + len;
            bytes.insert(bytes.end(), first, last);
            return 0;
        } catch (...) {
            data.exception = std::current_exception();
            return -1;
        }
    };

    if (to_bytes(object, write, &user_data) != 0) {
        std::rethrow_exception(user_data.exception);
    }
    return bytes;
}

template <typename CType>
class View
{
protected:
    const CType* m_ptr;

public:
    explicit View(const CType* ptr) : m_ptr{check(ptr)} {}

    const CType* get() const { return m_ptr; }
};

template <typename CType, CType* (*CopyFunc)(const CType*), void (*DestroyFunc)(CType*)>
class Handle
{
protected:
    CType* m_ptr;

public:
    explicit Handle(CType* ptr) : m_ptr{check(ptr)} {}

    // Copy constructors
    Handle(const Handle& other)
        : m_ptr{check(CopyFunc(other.m_ptr))} {}
    Handle& operator=(const Handle& other)
    {
        if (this != &other) {
            Handle temp(other);
            std::swap(m_ptr, temp.m_ptr);
        }
        return *this;
    }

    // Move constructors
    Handle(Handle&& other) noexcept : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }
    Handle& operator=(Handle&& other) noexcept
    {
        DestroyFunc(m_ptr);
        m_ptr = std::exchange(other.m_ptr, nullptr);
        return *this;
    }

    template <typename ViewType>
        requires std::derived_from<ViewType, View<CType>>
    Handle(const ViewType& view)
        : Handle{CopyFunc(view.get())}
    {
    }

    ~Handle() { DestroyFunc(m_ptr); }

    CType* get() { return m_ptr; }
    const CType* get() const { return m_ptr; }
};

template <typename CType, void (*DestroyFunc)(CType*)>
class UniqueHandle
{
protected:
    struct Deleter {
        void operator()(CType* ptr) const noexcept
        {
            if (ptr) DestroyFunc(ptr);
        }
    };
    std::unique_ptr<CType, Deleter> m_ptr;

public:
    explicit UniqueHandle(CType* ptr) : m_ptr{check(ptr)} {}

    CType* get() { return m_ptr; }
    const CType* get() const { return m_ptr; }
};

class Transaction;
class TransactionOutput;

template <typename Derived>
class ScriptPubkeyApi
{
private:
    auto impl() const
    {
        return static_cast<const Derived*>(this)->get();
    }

    friend Derived;
    ScriptPubkeyApi() = default;

public:
    bool Verify(int64_t amount,
                const Transaction& tx_to,
                std::span<const TransactionOutput> spent_outputs,
                unsigned int input_index,
                ScriptVerificationFlags flags,
                ScriptVerifyStatus& status) const;

    std::vector<std::byte> ToBytes() const
    {
        return write_bytes(impl(), btck_script_pubkey_to_bytes);
    }
};

class ScriptPubkeyView : public View<btck_ScriptPubkey>, public ScriptPubkeyApi<ScriptPubkeyView>
{
public:
    explicit ScriptPubkeyView(const btck_ScriptPubkey* ptr) : View{ptr} {}
};

class ScriptPubkey : public Handle<btck_ScriptPubkey, btck_script_pubkey_copy, btck_script_pubkey_destroy>, public ScriptPubkeyApi<ScriptPubkey>
{
public:
    explicit ScriptPubkey(std::span<const std::byte> raw)
        : Handle{btck_script_pubkey_create(raw.data(), raw.size())} {}

    ScriptPubkey(const ScriptPubkeyView& view)
        : Handle(view) {}
};

template <typename Derived>
class TransactionOutputApi
{
private:
    auto impl() const
    {
        return static_cast<const Derived*>(this)->get();
    }

    friend Derived;
    TransactionOutputApi() = default;

public:
    int64_t Amount() const
    {
        return btck_transaction_output_get_amount(impl());
    }

    ScriptPubkeyView GetScriptPubkey() const
    {
        return ScriptPubkeyView{btck_transaction_output_get_script_pubkey(impl())};
    }
};

class TransactionOutputView : public View<btck_TransactionOutput>, public TransactionOutputApi<TransactionOutputView>
{
public:
    explicit TransactionOutputView(const btck_TransactionOutput* ptr) : View{ptr} {}
};

class TransactionOutput : public Handle<btck_TransactionOutput, btck_transaction_output_copy, btck_transaction_output_destroy>, public TransactionOutputApi<TransactionOutput>
{
public:
    explicit TransactionOutput(const ScriptPubkey& script_pubkey, int64_t amount)
        : Handle{btck_transaction_output_create(script_pubkey.get(), amount)} {}

    TransactionOutput(const TransactionOutputView& view)
        : Handle(view) {}
};

template <typename Derived>
class TransactionApi
{
private:
    auto impl() const
    {
        return static_cast<const Derived*>(this)->get();
    }

public:
    size_t CountOutputs() const
    {
        return btck_transaction_count_outputs(impl());
    }

    size_t CountInputs() const
    {
        return btck_transaction_count_inputs(impl());
    }

    TransactionOutputView GetOutput(size_t index) const
    {
        return TransactionOutputView{btck_transaction_get_output_at(impl(), index)};
    }

    auto Outputs() const
    {
        return Range<Derived, &TransactionApi<Derived>::CountOutputs, &TransactionApi<Derived>::GetOutput>{*static_cast<const Derived*>(this)};
    }

    std::vector<std::byte> ToBytes() const
    {
        return write_bytes(impl(), btck_transaction_to_bytes);
    }
};

class Transaction : public Handle<btck_Transaction, btck_transaction_copy, btck_transaction_destroy>, public TransactionApi<Transaction>
{
public:
    explicit Transaction(std::span<const std::byte> raw_transaction)
        : Handle{btck_transaction_create(raw_transaction.data(), raw_transaction.size())} {}
};

template <typename Derived>
bool ScriptPubkeyApi<Derived>::Verify(int64_t amount,
                                      const Transaction& tx_to,
                                      const std::span<const TransactionOutput> spent_outputs,
                                      unsigned int input_index,
                                      ScriptVerificationFlags flags,
                                      ScriptVerifyStatus& status) const
{
    const btck_TransactionOutput** spent_outputs_ptr = nullptr;
    std::vector<const btck_TransactionOutput*> raw_spent_outputs;
    if (spent_outputs.size() > 0) {
        raw_spent_outputs.reserve(spent_outputs.size());

        for (const auto& output : spent_outputs) {
            raw_spent_outputs.push_back(output.get());
        }
        spent_outputs_ptr = raw_spent_outputs.data();
    }
    auto result = btck_script_pubkey_verify(
        impl(),
        amount,
        tx_to.get(),
        spent_outputs_ptr, spent_outputs.size(),
        input_index,
        static_cast<btck_ScriptVerificationFlags>(flags),
        reinterpret_cast<btck_ScriptVerifyStatus*>(&status));
    return result == 1;
}

inline void logging_disable()
{
    btck_logging_disable();
}

inline void logging_set_level_category(LogCategory category, LogLevel level)
{
    btck_logging_set_level_category(static_cast<btck_LogCategory>(category), static_cast<btck_LogLevel>(level));
}

inline void logging_enable_category(LogCategory category)
{
    btck_logging_enable_category(static_cast<btck_LogCategory>(category));
}

inline void logging_disable_category(LogCategory category)
{
    btck_logging_disable_category(static_cast<btck_LogCategory>(category));
}

template <typename T>
concept Log = requires(T a, std::string_view message) {
    { a.LogMessage(message) } -> std::same_as<void>;
};

template <Log T>
class Logger : public UniqueHandle<btck_LoggingConnection, btck_logging_connection_destroy>
{
public:
    Logger(std::unique_ptr<T> log, const btck_LoggingOptions& logging_options)
        : UniqueHandle{btck_logging_connection_create(
              +[](void* user_data, const char* message, size_t message_len) { static_cast<T*>(user_data)->LogMessage({message, message_len}); },
              log.release(),
              +[](void* user_data) { delete static_cast<T*>(user_data); },
              logging_options)}
    {
    }
};

} // namespace btck

#endif // BITCOIN_KERNEL_BITCOINKERNEL_WRAPPER_H
