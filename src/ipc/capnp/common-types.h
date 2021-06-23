// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_COMMON_TYPES_H
#define BITCOIN_IPC_CAPNP_COMMON_TYPES_H

#include <chainparams.h>
#include <consensus/validation.h>
#include <ipc/capnp/common.capnp.proxy.h>
#include <mp/proxy-types.h>
#include <net_processing.h>
#include <netbase.h>
#include <util/translation.h>
#include <validation.h>
#include <wallet/coincontrol.h>

namespace ipc {
namespace capnp {
//! Convert kj::StringPtr to std::string.
inline std::string ToString(const kj::StringPtr& str) { return {str.cStr(), str.size()}; }

//! Convert kj::ArrayPtr to std::string.
inline std::string ToString(const kj::ArrayPtr<const kj::byte>& data)
{
    return {reinterpret_cast<const char*>(data.begin()), data.size()};
}

//! Convert array object to kj::ArrayPtr.
template <typename Array>
inline kj::ArrayPtr<const kj::byte> ToArray(const Array& array)
{
    return {reinterpret_cast<const kj::byte*>(array.data()), array.size()};
}

//! Convert base_blob to kj::ArrayPtr.
template <typename Blob>
inline kj::ArrayPtr<const kj::byte> FromBlob(const Blob& blob)
{
    return {blob.begin(), blob.size()};
}

//! Convert kj::ArrayPtr to base_blob
template <typename Blob>
inline Blob ToBlob(kj::ArrayPtr<const kj::byte> data)
{
    // TODO: Avoid temp vector.
    return Blob(std::vector<unsigned char>(data.begin(), data.begin() + data.size()));
}

//! Serialize bitcoin value.
template <typename T>
CDataStream Serialize(const T& value)
{
    CDataStream stream(SER_NETWORK, CLIENT_VERSION);
    value.Serialize(stream);
    return stream;
}

//! Deserialize bitcoin value.
template <typename T>
T Unserialize(T& value, const kj::ArrayPtr<const kj::byte>& data)
{
    // Could optimize, it unnecessarily copies the data into a temporary vector.
    CDataStream stream({data.begin(), data.end()}, SER_NETWORK, CLIENT_VERSION);
    value.Unserialize(stream);
    return value;
}

//! Deserialize bitcoin value.
template <typename T>
T Unserialize(const kj::ArrayPtr<const kj::byte>& data)
{
    T value;
    Unserialize(value, data);
    return value;
}

template <typename T>
using Deserializable = std::is_constructible<T, ::deserialize_type, ::CDataStream&>;

template <typename T>
struct Unserializable
{
private:
    template <typename C>
    static std::true_type test(decltype(std::declval<C>().Unserialize(std::declval<C&>()))*);
    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(nullptr))::value;
};

template <typename T>
struct Serializable
{
private:
    template <typename C>
    static std::true_type test(decltype(std::declval<C>().Serialize(std::declval<C&>()))*);
    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(nullptr))::value;
};
} // namespace capnp
} // namespace ipc

namespace mp {
//!@{
//! Functions to serialize / deserialize bitcoin objects that don't
//! already provide their own serialization.
void CustomBuildMessage(InvokeContext& invoke_context,
                        const UniValue& univalue,
                        ipc::capnp::messages::UniValue::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::UniValue::Reader& reader,
                       UniValue& univalue);
//!@}

template <typename LocalType, typename Reader, typename ReadDest>
decltype(auto) CustomReadField(
    TypeList<LocalType>,
    Priority<1>,
    InvokeContext& invoke_context,
    Reader&& reader,
    ReadDest&& read_dest,
    decltype(CustomReadMessage(invoke_context, reader.get(), std::declval<LocalType&>()))* enable = nullptr)
{
    return read_dest.update([&](auto& value) { CustomReadMessage(invoke_context, reader.get(), value); });
}

template <typename Accessor, typename LocalType, typename ServerContext, typename Fn, typename... Args>
auto CustomPassField(TypeList<LocalType>, ServerContext& server_context, Fn&& fn, Args&&... args)
    -> decltype(CustomPassMessage(server_context,
                                  Accessor::get(server_context.call_context.getParams()),
                                  Accessor::init(server_context.call_context.getResults()),
                                  nullptr))
{
    CustomPassMessage(server_context, Accessor::get(server_context.call_context.getParams()),
                      Accessor::init(server_context.call_context.getResults()),
                      [&](LocalType param) { fn.invoke(server_context, std::forward<Args>(args)..., param); });
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(
    TypeList<LocalType>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest,
    typename std::enable_if<ipc::capnp::Deserializable<LocalType>::value>::type* enable = nullptr)
{
    assert(input.has());
    auto data = input.get();
    // Note: stream copy here is unnecessary, and can be avoided in the future
    // when `VectorReader` from #12254 is added.
    CDataStream stream({data.begin(), data.end()}, SER_NETWORK, CLIENT_VERSION);
    return read_dest.construct(deserialize, stream);
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(
    TypeList<LocalType>,
    Priority<1>,
    InvokeContext& invoke_context,
    Input&& input,
    ReadDest&& read_dest,
    // FIXME instead of always preferring Deserialize implementation over Unserialize should prefer Deserializing when
    // emplacing, unserialize when updating
    typename std::enable_if<ipc::capnp::Unserializable<LocalType>::value &&
                            !ipc::capnp::Deserializable<LocalType>::value>::type* enable = nullptr)
{
    return read_dest.update([&](auto& value) {
        if (!input.has()) return;
        auto data = input.get();
        // Note: stream copy here is unnecessary, and can be avoided in the future
        // when `VectorReader` from #12254 is added.
        CDataStream stream({data.begin(), data.end()}, SER_NETWORK, CLIENT_VERSION);
        value.Unserialize(stream);
    });
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(
    TypeList<std::chrono::microseconds>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest)
{
    return read_dest.construct(input.get());
}

template <typename Value, typename Output>
void CustomBuildField(TypeList<std::chrono::microseconds>, Priority<1>, InvokeContext& invoke_context, Value&& value, Output&& output)
{
    output.set(value.count());
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(
    TypeList<SecureString>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest)
{
    auto data = input.get();
    // Copy input into SecureString. Caller needs to be responsible for calling
    // memory_cleanse on the input.
    return read_dest.construct(CharCast(data.begin()), data.size());
}

template <typename Value, typename Output>
void CustomBuildField(TypeList<SecureString>, Priority<1>, InvokeContext& invoke_context, Value&& str, Output&& output)
{
    auto result = output.init(str.size());
    // Copy SecureString into output. Caller needs to be responsible for calling
    // memory_cleanse later on the output after it is sent.
    memcpy(result.begin(), str.data(), str.size());
}

template <typename LocalType, typename Value, typename Output>
void CustomBuildField(
    TypeList<LocalType>,
    Priority<2>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output,
    typename std::enable_if<std::is_same<decltype(output.init(value.size())), ::capnp::Data::Builder>::value>::type*
        enable_output = nullptr,
    decltype(memcpy(output.init(value.size()).begin(), value.data(), value.size()))* enable_value = nullptr)
{
    auto result = output.init(value.size());
    memcpy(result.begin(), value.data(), value.size());
}

template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType>,
                      Priority<2>,
                      InvokeContext& invoke_context,
                      Value&& value,
                      Output&& output,
                      decltype(CustomBuildMessage(invoke_context, value, output.init()))* enable = nullptr)
{
    CustomBuildMessage(invoke_context, value, output.init());
}

template <typename LocalType, typename Value, typename Output>
void CustomBuildField(
    TypeList<LocalType>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output,
    typename std::enable_if<ipc::capnp::Serializable<
        typename std::remove_cv<typename std::remove_reference<Value>::type>::type>::value>::type* enable = nullptr)
{
    CDataStream stream(SER_NETWORK, CLIENT_VERSION);
    value.Serialize(stream);
    auto result = output.init(stream.size());
    memcpy(result.begin(), stream.data(), stream.size());
}

template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
auto CustomPassField(TypeList<>, ServerContext& server_context, const Fn& fn, Args&&... args) ->
    typename std::enable_if<std::is_same<decltype(Accessor::get(server_context.call_context.getParams())),
                                         ipc::capnp::messages::GlobalArgs::Reader>::value>::type
{
    ipc::capnp::ReadGlobalArgs(server_context, Accessor::get(server_context.call_context.getParams()));
    return fn.invoke(server_context, std::forward<Args>(args)...);
}

template <typename Output>
void CustomBuildField(
    TypeList<>,
    Priority<1>,
    InvokeContext& invoke_context,
    Output&& output,
    typename std::enable_if<std::is_same<decltype(output.init()),
                                         ipc::capnp::messages::GlobalArgs::Builder>::value>::type* enable = nullptr)
{
    ipc::capnp::BuildGlobalArgs(invoke_context, output.init());
}
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_COMMON_TYPES_H
